/*****************************************************************************
 * iptv.cpp: the Live TV (IPTV) add-on
 *****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_access.h>
#include <vlc_configuration.h>
#include <vlc_interrupt.h>
#include <vlc_iptv.h>
#include <vlc_keystore.h>
#include <vlc_stream.h>
#include <vlc_fs.h>
#include <vlc_network.h>
#ifndef _WIN32
# include <arpa/inet.h>  // inet_pton, inet_ntop (LAN discovery)
# include <poll.h>
# include <dlfcn.h>      // dladdr, dlopen (the plug-in kept loaded)
#else
# include <windows.h>
#endif

#include "idle_watch.hpp"
#include "service.hpp"
// (the HTTP client of the interface: the servers' APIs, POST and sign-ins)
#include "../../gui/qt/util/http_request.hpp"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QRandomGenerator>
#include <QSaveFile>
#include <QUrl>
#include <atomic>
#include <chrono>

#include <cstdlib>
#include <cstring>
#include <condition_variable>
#include <deque>
#include <exception>
#include <functional>
#include <future>
#include <string>
#include <thread>
#include <vector>
#include <memory>
#include <mutex>

// the catalog of free, legal sources this build knows (catalog/catalog.json)
#include "catalog.h"

namespace {

// The reading of one download, on the thread that downloads: an interrupt
// context of its own, made the thread's for the time of the download, that
// the thread's context forwards to (VLC quitting kills the worker's and the
// request threads' contexts: the download ends at once), and that the idle
// watch raises when nothing came for the idle time (VLC's reads wait without
// bound: a server that stalls kept the download forever). The watch never
// raises it once the reading ended (the transfer is unregistered first), and
// nothing of it is left on the thread's own context.
class Reading
{
public:
    Reading(iptv::IdleWatch& watch, int idleSecs)
    {
        // (killed already, VLC quitting: forwarding would not tell it)
        m_cancelled = vlc_killed();
        if (m_cancelled)
            return;
        m_context = vlc_interrupt_create();
        if (!m_context)
            return;
        vlc_interrupt_forward_start(m_context, m_forward);
        m_outer = vlc_interrupt_set(m_context);
        vlc_interrupt_t* const context = m_context;
        m_transfer = std::make_unique<iptv::IdleWatch::Transfer>(
            watch, std::chrono::seconds(qMax(1, idleSecs)), [context]() { vlc_interrupt_raise(context); });
    }

    ~Reading()
    {
        // (unregistered first: no raise after this, then the thread's own
        // context back, its forwarding undone, ours destroyed)
        m_transfer.reset();
        if (!m_context)
            return;
        vlc_interrupt_set(m_outer);
        vlc_interrupt_forward_stop(m_forward);
        vlc_interrupt_destroy(m_context);
    }

    Reading(const Reading&) = delete;
    Reading& operator=(const Reading&) = delete;

    // VLC quits (before the download or while it reads)
    bool cancelled() const { return m_cancelled || vlc_killed(); }
    // nothing came for the idle time
    bool silent() const { return m_transfer && m_transfer->expired(); }
    void received()
    {
        if (m_transfer)
            m_transfer->received();
    }

private:
    bool m_cancelled = false;
    vlc_interrupt_t* m_context = nullptr;
    vlc_interrupt_t* m_outer = nullptr;
    void* m_forward[2] = { nullptr, nullptr };
    std::unique_ptr<iptv::IdleWatch::Transfer> m_transfer;
};

// What the service needs of VLC. The links of the sources hold personal
// tokens: they are never written to the log, only their host.
class VlcHost : public iptv::Host
{
public:
    VlcHost(vlc_iptv_t* obj, const std::string& keystoreFile)
        : m_obj(obj)
        , m_folder(QFileInfo(QString::fromStdString(keystoreFile)).absolutePath())
    {
        // a keystore that keeps the secrets (not the one in memory), made at
        // its first use, on the worker (not here, on the interface's thread:
        // opening a wallet may ask for its password)
        var_Create(obj, "keystore", VLC_VAR_STRING);
        var_SetString(obj, "keystore", "secret,kwallet,keychain,file_crypt,none");
        // (the encrypted file of Windows and macOS needs its path: VLC sets none)
        var_Create(obj, "keystore-file", VLC_VAR_STRING);
        var_SetString(obj, "keystore-file", keystoreFile.c_str());
        m_interrupt = vlc_interrupt_create();
    }

    ~VlcHost() override
    {
        if (m_keystore)
            vlc_keystore_release(m_keystore);
        if (m_interrupt)
            vlc_interrupt_destroy(m_interrupt);
    }

    bool download(const QString& url, const QString& path, QString* error, qint64 limit, int idleSecs,
                  const std::function<void(qint64, qint64)>& progress) override
    {
        // the access alone: a guide.xml.gz stays compressed on disk (read
        // inflated as it is parsed)
        if (m_detached)
        {
            *error = QStringLiteral("cancelled");
            return false;
        }
        Reading reading(m_watch, idleSecs);
        if (reading.cancelled())
        {
            *error = QStringLiteral("cancelled");
            return false;
        }
        stream_t* s = vlc_access_NewMRL(VLC_OBJECT(m_obj), url.toUtf8().constData());
        if (!s)
        {
            *error = reading.cancelled() ? QStringLiteral("cancelled")
                   : reading.silent()    ? QStringLiteral("no answer")
                                         : QStringLiteral("cannot be reached");
            return false;
        }
        uint64_t size = 0;
        if (vlc_stream_GetSize(s, &size) != VLC_SUCCESS)
            size = 0;
        // (a file of its own, put in place at once when complete: two
        // threads downloading the same logo never share one, the one there
        // stays until it is replaced)
        QSaveFile out(path);
        if (!out.open(QIODevice::WriteOnly))
        {
            vlc_stream_Delete(s);
            *error = out.errorString();
            return false;
        }
        std::unique_ptr<char[]> buffer(new char[256 * 1024]);
        qint64 total = 0, reported = 0;
        bool failed = false;
        for (;;)
        {
            // (what has come, at once: a full buffer waited for would make a
            // slow server look silent)
            const ssize_t got = vlc_stream_ReadPartial(s, buffer.get(), 256 * 1024);
            if (got == 0 || reading.silent() || reading.cancelled())
                break;
            if (got < 0)
                continue;
            reading.received();
            // (a link that is a stream, not a file, would fill the disk)
            if (total + got > limit)
            {
                failed = true;
                *error = QStringLiteral("too large");
                break;
            }
            if (out.write(buffer.get(), got) != got)
            {
                failed = true;
                *error = out.errorString();
                break;
            }
            total += got;
            if (progress && total - reported >= 512 * 1024)
            {
                reported = total;
                progress(total, qint64(size));
            }
        }
        vlc_stream_Delete(s);
        // (an error reads as the end of the stream: stopped, silent, or
        // shorter than the size announced, it is not what was asked for)
        if (!failed && reading.cancelled())
        {
            failed = true;
            *error = QStringLiteral("cancelled");
        }
        else if (!failed && reading.silent())
        {
            failed = true;
            *error = QStringLiteral("no answer");
        }
        else if (!failed && total == 0)
        {
            failed = true;
            *error = QStringLiteral("empty answer");
        }
        else if (!failed && size > 0 && uint64_t(total) != size)
        {
            failed = true;
            *error = QStringLiteral("cut short");
        }
        if (failed)
        {
            out.cancelWriting();
            return false;
        }
        if (!out.commit())
        {
            *error = QStringLiteral("cannot be saved");
            return false;
        }
        return true;
    }

    QByteArray fetch(const QString& url, QString* error, qint64 limit, int idleSecs) override
    {
        if (m_detached)
        {
            *error = QStringLiteral("cancelled");
            return QByteArray();
        }
        Reading reading(m_watch, idleSecs);
        if (reading.cancelled())
        {
            *error = QStringLiteral("cancelled");
            return QByteArray();
        }
        stream_t* s = vlc_access_NewMRL(VLC_OBJECT(m_obj), url.toUtf8().constData());
        if (!s)
        {
            *error = reading.cancelled() ? QStringLiteral("cancelled")
                   : reading.silent()    ? QStringLiteral("no answer")
                                         : QStringLiteral("cannot be reached");
            return QByteArray();
        }
        uint64_t size = 0;
        if (vlc_stream_GetSize(s, &size) != VLC_SUCCESS)
            size = 0;
        QByteArray out;
        char buffer[64 * 1024];
        for (;;)
        {
            const ssize_t got = vlc_stream_ReadPartial(s, buffer, sizeof(buffer));
            if (got == 0 || reading.silent() || reading.cancelled())
                break;
            if (got < 0)
                continue;
            reading.received();
            out.append(buffer, got);
            if (out.size() > limit)
            {
                out.clear();
                *error = QStringLiteral("too large");
                break;
            }
        }
        vlc_stream_Delete(s);
        // (stopped, silent, or shorter than announced: an answer cut short, none)
        if (reading.cancelled())
        {
            out.clear();
            *error = QStringLiteral("cancelled");
        }
        else if (reading.silent())
        {
            out.clear();
            *error = QStringLiteral("no answer");
        }
        else if (!out.isEmpty() && size > 0 && uint64_t(out.size()) != size)
        {
            out.clear();
            *error = QStringLiteral("cut short");
        }
        return out;
    }

    bool storeSecret(const QString& id, const QByteArray& secret) override
    {
        std::lock_guard<std::mutex> guard(m_keystoreLock);
        if (!keystore())
            return false;
        const QByteArray server = id.toUtf8();
        const char* values[KEY_MAX];
        VLC_KEYSTORE_VALUES_INIT(values);
        values[KEY_PROTOCOL] = "vlc-iptv";
        values[KEY_SERVER] = server.constData();
        return vlc_keystore_store(m_keystore, values,
                                  reinterpret_cast<const uint8_t*>(secret.constData()), secret.size(),
                                  "VLC Live TV source") == VLC_SUCCESS;
    }

    QByteArray loadSecret(const QString& id, bool* found) override
    {
        *found = false;
        std::lock_guard<std::mutex> guard(m_keystoreLock);
        if (!keystore())
            return QByteArray();
        const QByteArray server = id.toUtf8();
        const char* values[KEY_MAX];
        VLC_KEYSTORE_VALUES_INIT(values);
        values[KEY_PROTOCOL] = "vlc-iptv";
        values[KEY_SERVER] = server.constData();
        vlc_keystore_entry* entries = nullptr;
        const unsigned count = vlc_keystore_find(m_keystore, values, &entries);
        QByteArray out;
        if (count > 0 && entries[0].p_secret)
        {
            out = QByteArray(reinterpret_cast<const char*>(entries[0].p_secret), int(entries[0].i_secret_len));
            *found = true;
        }
        if (entries)
            vlc_keystore_release_entries(entries, count);
        return out;
    }

    void removeSecret(const QString& id) override
    {
        std::lock_guard<std::mutex> guard(m_keystoreLock);
        if (!keystore())
            return;
        const QByteArray server = id.toUtf8();
        const char* values[KEY_MAX];
        VLC_KEYSTORE_VALUES_INIT(values);
        values[KEY_PROTOCOL] = "vlc-iptv";
        values[KEY_SERVER] = server.constData();
        vlc_keystore_remove(m_keystore, values);
    }

    // (under m_keystoreLock) the keystore, made at its first use: on the
    // worker or a request's thread, never the interface's
    vlc_keystore* keystore()
    {
        if (!m_keystoreTried && !m_detached)
        {
            m_keystoreTried = true;
            m_keystore = vlc_keystore_create(VLC_OBJECT(m_obj));
        }
        return m_keystore;
    }

    void notify(const QJsonObject& event) override
    {
        const QByteArray json = QJsonDocument(event).toJson(QJsonDocument::Compact);
        if (!m_detached && m_obj->owner_cbs && m_obj->owner_cbs->on_event)
            m_obj->owner_cbs->on_event(m_obj, json.constData(), m_obj->owner_data);
    }

    void log(const QString& message) override
    {
        if (!m_detached)
            msg_Warn(m_obj, "%s", message.toUtf8().constData());
    }

    void debug(const QString& message) override
    {
        if (!m_detached)
            msg_Dbg(m_obj, "%s", message.toUtf8().constData());
    }

    // Close let go of a thread it could not stop: the VLC object is
    // destroyed, what is left uses nothing of it anymore
    void detach()
    {
        m_detached = true;
    }

    void enterWorker() override
    {
        if (m_interrupt)
            vlc_interrupt_set(m_interrupt);
    }

    void killWorker() override
    {
        if (m_interrupt)
            vlc_interrupt_kill(m_interrupt);
        // the calls of the servers in progress end now, no more starts
        std::lock_guard<std::mutex> guard(m_callsLock);
        m_closing = true;
        for (HttpSyncRequest* c : m_calls)
            c->cancel();
    }

    iptv::HttpReply call(const iptv::HttpCall& c) override
    {
        iptv::HttpReply reply;
        HttpSyncRequest sync;
        {
            std::lock_guard<std::mutex> guard(m_callsLock);
            if (m_closing)
            {
                reply.error = QStringLiteral("closing");
                return reply;
            }
            m_calls.push_back(&sync);
        }
        HttpRequest request;
        request.method = c.method;
        request.url = QUrl(c.url);
        request.headers = c.headers;
        request.headers.append(qMakePair(QByteArray("User-Agent"), QByteArray("VLC-Redesign")));
        request.body = c.body;
        request.username = c.username;
        request.password = c.password;
        request.timeoutMs = c.timeoutMs;
        request.deadlineMs = c.timeoutMs * 4;
        request.maxBytes = c.maxBytes;
        const HttpResponse response = sync.run(request);
        {
            std::lock_guard<std::mutex> guard(m_callsLock);
            m_calls.erase(std::find(m_calls.begin(), m_calls.end(), &sync));
        }
        reply.status = response.status;
        reply.body = response.body;
        reply.error = response.error;
        if (response.body.size() > c.maxBytes)
        {
            reply.body.clear();
            reply.error = QStringLiteral("too large");
        }
        return reply;
    }

    QVector<iptv::Datagram> broadcast(quint16 port, const QByteArray& packet, int ms,
                                      const QString& address) override
    {
        QVector<iptv::Datagram> answers;
        const int fd = vlc_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, true);
        if (fd == -1)
            return answers;
        int yes = 1;
        setsockopt(fd, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char*>(&yes), sizeof(yes));
        // from the address this computer has on the network of its default
        // route (a UDP "connect" sends nothing): Windows sends a broadcast or
        // a multicast on its first adapter otherwise, often a virtual one
        // (Hyper-V, Docker, WSL) where nothing answers
        struct in_addr local = {};
        {
            const int probe = vlc_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, true);
            if (probe != -1)
            {
                struct sockaddr_in away = {};
                away.sin_family = AF_INET;
                away.sin_port = htons(9);
                // (TEST-NET-1: an address never reached)
                inet_pton(AF_INET, "192.0.2.1", &away.sin_addr);
                struct sockaddr_in name = {};
                socklen_t nameLength = sizeof(name);
                if (connect(probe, reinterpret_cast<const struct sockaddr*>(&away), sizeof(away)) == 0
                    && getsockname(probe, reinterpret_cast<struct sockaddr*>(&name), &nameLength) == 0)
                    local = name.sin_addr;
                net_Close(probe);
            }
        }
        if (local.s_addr != 0)
        {
            struct sockaddr_in bound = {};
            bound.sin_family = AF_INET;
            bound.sin_addr = local;
            bind(fd, reinterpret_cast<const struct sockaddr*>(&bound), sizeof(bound));
            setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, reinterpret_cast<const char*>(&local), sizeof(local));
        }
        struct sockaddr_in to = {};
        to.sin_family = AF_INET;
        to.sin_port = htons(port);
        to.sin_addr.s_addr = htonl(INADDR_BROADCAST);
        if (!address.isEmpty())
        {
            // a multicast group (mDNS): the hops multicast DNS asks for
            if (inet_pton(AF_INET, qUtf8Printable(address), &to.sin_addr) != 1)
            {
                net_Close(fd);
                return answers;
            }
#ifdef _WIN32
            DWORD ttl = 255;
#else
            unsigned char ttl = 255;
#endif
            setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, reinterpret_cast<const char*>(&ttl), sizeof(ttl));
        }
        if (sendto(fd, packet.constData(), int(packet.size()), 0, reinterpret_cast<const struct sockaddr*>(&to),
                   sizeof(to)) < 0)
        {
            net_Close(fd);
            return answers;
        }
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
        char buffer[8192];
        for (;;)
        {
            const auto left = std::chrono::duration_cast<std::chrono::milliseconds>(
                deadline - std::chrono::steady_clock::now()).count();
            if (left <= 0)
                break;
            {
                std::lock_guard<std::mutex> guard(m_callsLock);
                if (m_closing)
                    break;
            }
            struct pollfd ufd = {};
            ufd.fd = fd;
            ufd.events = POLLIN;
            // (in slices: VLC quitting ends the wait)
            const int ready = poll(&ufd, 1, int(std::min<long long>(left, 100)));
            if (ready < 0)
                break;
            if (ready == 0)
                continue;
            struct sockaddr_in from = {};
            socklen_t fromLength = sizeof(from);
            const auto got = recvfrom(fd, buffer, sizeof(buffer), 0, reinterpret_cast<struct sockaddr*>(&from),
                                      &fromLength);
            if (got <= 0)
                continue;
            char address[64] = {};
            if (!inet_ntop(AF_INET, &from.sin_addr, address, sizeof(address)))
                continue;
            answers.append({ QString::fromLatin1(address), QByteArray(buffer, int(got)) });
            if (answers.size() >= 32)
                break;
        }
        net_Close(fd);
        return answers;
    }

    QString deviceId() override
    {
        // made once, kept next to the sources (what a server names this
        // client by, in its list of devices)
        std::lock_guard<std::mutex> guard(m_deviceLock);
        if (!m_deviceId.isEmpty())
            return m_deviceId;
        QFile file(m_folder + QStringLiteral("/device-id"));
        if (file.open(QIODevice::ReadOnly))
            m_deviceId = QString::fromLatin1(file.readAll()).trimmed();
        file.close();
        if (m_deviceId.size() < 16)
        {
            m_deviceId = QString::number(QRandomGenerator::global()->generate64(), 16)
                       + QString::number(QRandomGenerator::global()->generate64(), 16);
            if (file.open(QIODevice::WriteOnly))
                file.write(m_deviceId.toLatin1());
        }
        return m_deviceId;
    }

private:
    vlc_iptv_t* m_obj;
    std::atomic<bool> m_detached { false };
    // the calls of the servers in progress (killWorker() cancels them)
    std::mutex m_callsLock;
    std::vector<HttpSyncRequest*> m_calls;
    bool m_closing = false;
    QString m_folder;
    std::mutex m_deviceLock;
    QString m_deviceId;
    vlc_keystore* m_keystore = nullptr;
    bool m_keystoreTried = false;
    std::mutex m_keystoreLock;
    vlc_interrupt_t* m_interrupt = nullptr;
    // the downloads watched for silence (its thread made at the first one)
    iptv::IdleWatch m_watch;
};

// The threads the requests of the interface are answered on. They come
// from the interface's threads, but the Qt of this plugin (its own copy,
// static on Windows) keeps data per thread, freed when a thread ends: on
// threads it does not own that came too late (a crash at exit). Every
// answer is made on a thread of the plugin, which ends before it closes.
class Executor
{
public:
    explicit Executor(int count)
    {
        for (int i = 0; i < count; ++i)
        {
            vlc_interrupt_t* interrupt = vlc_interrupt_create();
            m_interrupts.push_back(interrupt);
            m_running++;
            m_threads.emplace_back([this, interrupt]() {
                if (interrupt)
                    vlc_interrupt_set(interrupt);
                run();
                std::lock_guard<std::mutex> guard(m_lock);
                m_running--;
                m_ended.notify_all();
            });
        }
    }

    ~Executor()
    {
        if (m_threads.empty())
        {
            for (vlc_interrupt_t* interrupt : m_interrupts)
                if (interrupt)
                    vlc_interrupt_destroy(interrupt);
            return;
        }
        {
            std::lock_guard<std::mutex> guard(m_lock);
            m_quit = true;
        }
        // a download in progress (a logo, the films of an account) stops
        for (vlc_interrupt_t* interrupt : m_interrupts)
            if (interrupt)
                vlc_interrupt_kill(interrupt);
        m_wait.notify_all();
        for (std::thread& t : m_threads)
            t.join();
        for (vlc_interrupt_t* interrupt : m_interrupts)
            if (interrupt)
                vlc_interrupt_destroy(interrupt);
    }

    // VLC quits: a download in progress on one of the threads (a logo, the
    // films of an account) stops, and the waits for the jobs are bounded
    void interrupt()
    {
        {
            std::lock_guard<std::mutex> guard(m_lock);
            if (m_stopping)
                return;
            m_stopping = true;
            m_stoppedAt = std::chrono::steady_clock::now();
        }
        for (vlc_interrupt_t* interrupt : m_interrupts)
            if (interrupt)
                vlc_interrupt_kill(interrupt);
    }

    // Runs job on one of the threads and waits for it; false when closing,
    // or when VLC quits and the job is still not done `bound` later (it may
    // run later: it must own what it uses)
    bool runAndWait(const std::function<void()>& job,
                    std::chrono::milliseconds bound = std::chrono::seconds(3))
    {
        auto done = std::make_shared<std::promise<void>>();
        std::future<void> result = done->get_future();
        {
            std::lock_guard<std::mutex> guard(m_lock);
            if (m_quit)
                return false;
            m_jobs.push_back([job, done]() {
                // (a job that throws, out of memory on an enormous answer,
                // still ends its wait; an exception out of a thread ends VLC)
                try
                {
                    job();
                }
                catch (...)
                {
                }
                done->set_value();
            });
        }
        m_wait.notify_one();
        while (result.wait_for(std::chrono::milliseconds(100)) != std::future_status::ready)
        {
            std::lock_guard<std::mutex> guard(m_lock);
            if (m_stopping && std::chrono::steady_clock::now() - m_stoppedAt > bound)
                return false;
        }
        return true;
    }

    // The threads end, waited for at most ms; false when one is still in a
    // download VLC could not stop (they are then let go, with this object)
    bool shutdown(int ms)
    {
        {
            std::lock_guard<std::mutex> guard(m_lock);
            m_quit = true;
        }
        interrupt();
        m_wait.notify_all();
        std::unique_lock<std::mutex> guard(m_lock);
        if (!m_ended.wait_for(guard, std::chrono::milliseconds(ms),
                              [this]() { return m_running == 0; }))
        {
            for (std::thread& t : m_threads)
                t.detach();
            m_threads.clear();
            return false;
        }
        guard.unlock();
        for (std::thread& t : m_threads)
            t.join();
        m_threads.clear();
        return true;
    }

private:
    void run()
    {
        for (;;)
        {
            std::function<void()> job;
            {
                std::unique_lock<std::mutex> guard(m_lock);
                m_wait.wait(guard, [this]() { return m_quit || !m_jobs.empty(); });
                if (m_jobs.empty())
                    return;
                job = std::move(m_jobs.front());
                m_jobs.pop_front();
            }
            try
            {
                job();
            }
            catch (...)
            {
                // (the jobs of runAndWait catch their own; nothing may leave
                // a thread)
            }
        }
    }

    std::mutex m_lock;
    std::condition_variable m_wait;
    std::deque<std::function<void()>> m_jobs;
    bool m_quit = false;
    std::vector<std::thread> m_threads;
    std::vector<vlc_interrupt_t*> m_interrupts;
    std::condition_variable m_ended;
    int m_running = 0;
    bool m_stopping = false;
    std::chrono::steady_clock::time_point m_stoppedAt;
};

struct Sys
{
    std::unique_ptr<VlcHost> host;
    // (shared: a request that VLC's quitting gave up on may still use it)
    std::mutex serviceLock;
    std::shared_ptr<iptv::Service> service;
    std::unique_ptr<Executor> executor;
};

std::shared_ptr<iptv::Service> serviceOf(Sys* sys)
{
    std::lock_guard<std::mutex> guard(sys->serviceLock);
    return sys->service;
}

QString userDir(const char* dir)
{
    return dir ? QString::fromUtf8(dir) + QStringLiteral("/iptv") : QString();
}

char* Request(vlc_iptv_t* obj, const char* json)
{
    auto* sys = static_cast<Sys*>(obj->sys);
    // (owned by the job: VLC quitting, the wait for it may give up first)
    auto request = std::make_shared<const std::string>(json);
    auto answer = std::make_shared<std::string>();
    // (on a thread of the plugin, see Executor)
    if (!sys->executor->runAndWait([sys, request, answer]() {
            try
            {
                const QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(*request));
                const std::shared_ptr<iptv::Service> service = serviceOf(sys);
                QJsonObject out;
                if (!doc.isObject())
                    out.insert(QLatin1String("error"), QStringLiteral("not a JSON object"));
                else if (!service)
                    out.insert(QLatin1String("error"), QStringLiteral("closing"));
                else
                    out = service->request(doc.object());
                *answer = QJsonDocument(out).toJson(QJsonDocument::Compact).toStdString();
            }
            catch (const std::exception&)
            {
                // (out of memory on an enormous answer: an error, VLC goes on)
                *answer = "{\"error\":\"failed\"}";
            }
        }))
        return strdup("{\"error\":\"closing\"}");
    return strdup(answer->empty() ? "{\"error\":\"failed\"}" : answer->c_str());
}

void Stop(vlc_iptv_t* obj)
{
    // (from a thread of the interface: flags, locks and interrupts only,
    // nothing of Qt made here)
    auto* sys = static_cast<Sys*>(obj->sys);
    if (const std::shared_ptr<iptv::Service> service = serviceOf(sys))
        service->stop();
    sys->executor->interrupt();
}

int Open(vlc_object_t* p_this)
{
    auto* obj = reinterpret_cast<vlc_iptv_t*>(p_this);
    char* data = config_GetUserDir(VLC_USERDATA_DIR);
    char* cache = config_GetUserDir(VLC_CACHE_DIR);
    const bool dirs = data && cache;
    std::string dataDir = data ? data : "", cacheDir = cache ? cache : "";
    free(data);
    free(cache);
    if (!dirs)
        return VLC_EGENERIC;
    try
    {
        auto sys = std::make_unique<Sys>();
        const std::string folder = dataDir + "/iptv";
        vlc_mkdir(folder.c_str(), 0700);
        sys->host = std::make_unique<VlcHost>(obj, folder + "/keystore");
        // the requests, the logos and the films of an account at the same time
        sys->executor = std::make_unique<Executor>(3);
        Sys* s = sys.get();
        s->executor->runAndWait([&]() {
            // (failed, its thread not made, out of memory: no service, the
            // module is not loaded; the exception would end VLC on this thread)
            try
            {
                s->service = std::make_shared<iptv::Service>(
                    *s->host, userDir(dataDir.c_str()), userDir(cacheDir.c_str()),
                    QByteArray::fromRawData(iptv_catalog, int(sizeof(iptv_catalog) - 1)));
            }
            catch (const std::exception&)
            {
                s->service.reset();
            }
        });
        if (!s->service)
            return VLC_EGENERIC;
        obj->sys = sys.release();
    }
    catch (const std::exception&)
    {
        return VLC_ENOMEM;
    }
    obj->request = Request;
    obj->stop = Stop;
    return VLC_SUCCESS;
}

// The plug-in stays loaded until the process ends: VLC unloads its
// plug-ins at quit (module_EndBank: FreeLibrary, dlclose), and a thread let
// go by Close still runs its code (it would crash there, waking up later,
// and the static Qt's destructors would run under it)
void keepLoaded()
{
#ifdef _WIN32
    HMODULE self = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                       reinterpret_cast<LPCWSTR>(&keepLoaded), &self);
#else
    Dl_info info;
    if (dladdr(reinterpret_cast<void*>(&keepLoaded), &info) && info.dli_fname)
        dlopen(info.dli_fname, RTLD_LAZY | RTLD_NOLOAD | RTLD_NODELETE);
#endif
}

void Close(vlc_object_t* p_this)
{
    auto* obj = reinterpret_cast<vlc_iptv_t*>(p_this);
    auto* sys = static_cast<Sys*>(obj->sys);
    // Whatever a download does, VLC's quitting never waits for it without
    // bound: what is in progress stops (the worker's and the threads'
    // downloads are interrupted); the service ends on a thread of the plugin,
    // its worker waited for 5 s at most, then those threads 3 s at most,
    // then the host. A download VLC could not stop (an HTTPS connection
    // that never closes) keeps what it uses: let go, not destroyed.
    {
        // (not kept: the service is destroyed on a thread of the plugin)
        const std::shared_ptr<iptv::Service> service = serviceOf(sys);
        if (service)
            service->stop();
    }
    sys->executor->interrupt();
    auto stopped = std::make_shared<std::atomic<bool>>(false);
    const bool ran = sys->executor->runAndWait([sys, stopped]() {
        std::shared_ptr<iptv::Service> service;
        {
            std::lock_guard<std::mutex> guard(sys->serviceLock);
            service = std::move(sys->service);
        }
        if (!service || service->waitStopped(5000))
            *stopped = true;
        else
            new std::shared_ptr<iptv::Service>(service);   // (let go)
    }, std::chrono::seconds(8));
    if (!ran || !*stopped || !sys->executor->shutdown(3000))
    {
        msg_Warn(obj, "a download could not be stopped: Live TV let go of it");
        sys->host->detach();
        keepLoaded();
        return;   // (sys, its host and threads let go too)
    }
    sys->executor.reset();
    delete sys;
}

}

#define ENABLED_TEXT N_("Live TV (IPTV) add-on")
#define ENABLED_LONGTEXT N_("Shows TV in the library: the channels of your IPTV playlists " \
    "and accounts, with their programme guide. Off, nothing of it is loaded.")

vlc_module_begin()
    set_shortname(N_("Live TV"))
    set_description(N_("Live TV (IPTV) channels and programme guide"))
    set_subcategory(SUBCAT_ADVANCED_MISC)
    add_bool("iptv-enabled", true, ENABLED_TEXT, ENABLED_LONGTEXT)
    set_capability("iptv", 10)
    set_callbacks(Open, Close)
    add_shortcut("iptv")
vlc_module_end()
