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

#include "service.hpp"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>

#include <cstdlib>
#include <cstring>
#include <memory>
#include <mutex>

// the catalog of free, legal sources this build knows (catalog/catalog.json)
#include "catalog.h"

namespace {

// What the service needs of VLC. The links of the sources hold personal
// tokens: they are never written to the log, only their host.
class VlcHost : public iptv::Host
{
public:
    explicit VlcHost(vlc_iptv_t* obj) : m_obj(obj)
    {
        // a keystore that keeps the secrets (not the one in memory)
        var_Create(obj, "keystore", VLC_VAR_STRING);
        var_SetString(obj, "keystore", "secret,kwallet,keychain,file_crypt,none");
        m_keystore = vlc_keystore_create(obj);
        m_interrupt = vlc_interrupt_create();
    }

    ~VlcHost() override
    {
        if (m_keystore)
            vlc_keystore_release(m_keystore);
        if (m_interrupt)
            vlc_interrupt_destroy(m_interrupt);
    }

    bool download(const QString& url, const QString& path, QString* error,
                  const std::function<void(qint64, qint64)>& progress) override
    {
        // the access alone: a guide.xml.gz stays compressed on disk (read
        // inflated as it is parsed)
        stream_t* s = vlc_access_NewMRL(VLC_OBJECT(m_obj), url.toUtf8().constData());
        if (!s)
        {
            *error = vlc_killed() ? QStringLiteral("cancelled") : QStringLiteral("cannot be reached");
            return false;
        }
        uint64_t size = 0;
        if (vlc_stream_GetSize(s, &size) != VLC_SUCCESS)
            size = 0;
        QFile out(path + QStringLiteral(".part"));
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
            const ssize_t got = vlc_stream_Read(s, buffer.get(), 256 * 1024);
            if (got <= 0)
                break;
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
        out.close();
        if (vlc_killed())
        {
            failed = true;
            *error = QStringLiteral("cancelled");
        }
        else if (!failed && total == 0)
        {
            failed = true;
            *error = QStringLiteral("empty answer");
        }
        if (failed)
        {
            QFile::remove(out.fileName());
            return false;
        }
        QFile::remove(path);
        if (!QFile::rename(out.fileName(), path))
        {
            *error = QStringLiteral("cannot be saved");
            return false;
        }
        return true;
    }

    QByteArray fetch(const QString& url, QString* error, qint64 limit) override
    {
        stream_t* s = vlc_access_NewMRL(VLC_OBJECT(m_obj), url.toUtf8().constData());
        if (!s)
        {
            *error = QStringLiteral("cannot be reached");
            return QByteArray();
        }
        QByteArray out;
        char buffer[64 * 1024];
        for (;;)
        {
            const ssize_t got = vlc_stream_Read(s, buffer, sizeof(buffer));
            if (got <= 0)
                break;
            out.append(buffer, got);
            if (out.size() > limit)
            {
                out.clear();
                *error = QStringLiteral("too large");
                break;
            }
        }
        vlc_stream_Delete(s);
        return out;
    }

    bool storeSecret(const QString& id, const QByteArray& secret) override
    {
        if (!m_keystore)
            return false;
        const QByteArray server = id.toUtf8();
        const char* values[KEY_MAX];
        VLC_KEYSTORE_VALUES_INIT(values);
        values[KEY_PROTOCOL] = "vlc-iptv";
        values[KEY_SERVER] = server.constData();
        std::lock_guard<std::mutex> guard(m_keystoreLock);
        return vlc_keystore_store(m_keystore, values,
                                  reinterpret_cast<const uint8_t*>(secret.constData()), secret.size(),
                                  "VLC Live TV source") == VLC_SUCCESS;
    }

    QByteArray loadSecret(const QString& id, bool* found) override
    {
        *found = false;
        if (!m_keystore)
            return QByteArray();
        const QByteArray server = id.toUtf8();
        const char* values[KEY_MAX];
        VLC_KEYSTORE_VALUES_INIT(values);
        values[KEY_PROTOCOL] = "vlc-iptv";
        values[KEY_SERVER] = server.constData();
        vlc_keystore_entry* entries = nullptr;
        std::lock_guard<std::mutex> guard(m_keystoreLock);
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
        if (!m_keystore)
            return;
        const QByteArray server = id.toUtf8();
        const char* values[KEY_MAX];
        VLC_KEYSTORE_VALUES_INIT(values);
        values[KEY_PROTOCOL] = "vlc-iptv";
        values[KEY_SERVER] = server.constData();
        std::lock_guard<std::mutex> guard(m_keystoreLock);
        vlc_keystore_remove(m_keystore, values);
    }

    void notify(const QJsonObject& event) override
    {
        const QByteArray json = QJsonDocument(event).toJson(QJsonDocument::Compact);
        if (m_obj->owner_cbs && m_obj->owner_cbs->on_event)
            m_obj->owner_cbs->on_event(m_obj, json.constData(), m_obj->owner_data);
    }

    void log(const QString& message) override
    {
        msg_Warn(m_obj, "%s", message.toUtf8().constData());
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
    }

private:
    vlc_iptv_t* m_obj;
    vlc_keystore* m_keystore = nullptr;
    std::mutex m_keystoreLock;
    vlc_interrupt_t* m_interrupt = nullptr;
};

struct Sys
{
    std::unique_ptr<VlcHost> host;
    std::unique_ptr<iptv::Service> service;
};

QString userDir(vlc_userdir_t type)
{
    char* dir = config_GetUserDir(type);
    const QString out = dir ? QString::fromUtf8(dir) + QStringLiteral("/iptv") : QString();
    free(dir);
    return out;
}

char* Request(vlc_iptv_t* obj, const char* json)
{
    auto* sys = static_cast<Sys*>(obj->sys);
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(QByteArray(json), &error);
    QJsonObject answer;
    if (!doc.isObject())
        answer.insert(QLatin1String("error"), QStringLiteral("not a JSON object"));
    else
        answer = sys->service->request(doc.object());
    const QByteArray out = QJsonDocument(answer).toJson(QJsonDocument::Compact);
    return strdup(out.constData());
}

int Open(vlc_object_t* p_this)
{
    auto* obj = reinterpret_cast<vlc_iptv_t*>(p_this);
    const QString data = userDir(VLC_USERDATA_DIR);
    const QString cache = userDir(VLC_CACHE_DIR);
    if (data.isEmpty() || cache.isEmpty())
        return VLC_EGENERIC;
    try
    {
        auto sys = std::make_unique<Sys>();
        sys->host = std::make_unique<VlcHost>(obj);
        sys->service = std::make_unique<iptv::Service>(
            *sys->host, data, cache, QByteArray::fromRawData(iptv_catalog, int(sizeof(iptv_catalog) - 1)));
        obj->sys = sys.release();
    }
    catch (const std::exception&)
    {
        return VLC_ENOMEM;
    }
    obj->request = Request;
    return VLC_SUCCESS;
}

void Close(vlc_object_t* p_this)
{
    auto* obj = reinterpret_cast<vlc_iptv_t*>(p_this);
    auto* sys = static_cast<Sys*>(obj->sys);
    // the worker stops first (its downloads are interrupted), then the host
    sys->service.reset();
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
