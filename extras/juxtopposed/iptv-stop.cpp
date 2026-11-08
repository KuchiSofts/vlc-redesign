/* Copyright (C) 2026 VLC authors and VideoLAN. GPL version 2 or later.
 *
 * Live TV: quitting never waits without bound (modules/misc/iptv/service.cpp).
 * A host whose download never returns, even interrupted (what an HTTPS
 * connection that never closes looked like: VLC hung at quit on macOS), and
 * one whose download stops when interrupted: Service::waitStopped() gives up
 * on the first within its bound and stops the second at once. The catalog's
 * newer copy is not downloaded at the start, but when the catalog is shown.
 * On Linux, from the root of the tree:
 *   g++ -std=c++17 -O1 -Wall -Wextra -fPIC -DHAVE_ZLIB -I. $(pkg-config --cflags Qt6Core) \
 *       extras/juxtopposed/iptv-stop.cpp modules/misc/iptv/service.cpp modules/misc/iptv/servers.cpp \
 *       modules/misc/iptv/autoguide.cpp modules/misc/iptv/playlist.cpp modules/misc/iptv/guide.cpp \
 *       modules/misc/iptv/xtream.cpp $(pkg-config --libs Qt6Core) -lz -o iptv-stop && ./iptv-stop
 */
#include "modules/misc/iptv/service.hpp"

#include <QDir>
#include <QJsonArray>
#include <QTemporaryDir>

#include <chrono>
#include <cstdio>
#include <thread>

static int checks = 0;
static int failures = 0;
#define CHECK(condition) do { ++checks; if (!(condition)) { \
    std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); ++failures; } } while (0)

namespace {

// downloads that wait until released (stuck) or until killed (stoppable)
class TestHost : public iptv::Host
{
public:
    explicit TestHost(bool stuck) : m_stuck(stuck) {}

    bool download(const QString&, const QString&, QString* error, qint64,
                  const std::function<void(qint64, qint64)>&) override
    {
        wait();
        if (error)
            *error = QStringLiteral("stopped");
        return false;
    }
    QByteArray fetch(const QString& url, QString* error, qint64) override
    {
        {
            std::lock_guard<std::mutex> guard(m_lock);
            m_fetched << url;
        }
        if (error)
            *error = QStringLiteral("none here");
        return QByteArray();
    }
    bool storeSecret(const QString&, const QByteArray&) override { return false; }
    QByteArray loadSecret(const QString&, bool* found) override { *found = false; return {}; }
    void removeSecret(const QString&) override {}
    void notify(const QJsonObject&) override {}
    void log(const QString&) override {}
    void debug(const QString&) override {}
    void enterWorker() override {}
    void killWorker() override
    {
        std::lock_guard<std::mutex> guard(m_lock);
        m_killed = true;
        m_wake.notify_all();
    }
    iptv::HttpReply call(const iptv::HttpCall&) override { return {}; }
    QVector<iptv::Datagram> broadcast(quint16, const QByteArray&, int, const QString&) override { return {}; }
    QString deviceId() override { return QStringLiteral("test"); }

    // the stuck download ends (before the test does)
    void release()
    {
        std::lock_guard<std::mutex> guard(m_lock);
        m_released = true;
        m_wake.notify_all();
    }
    bool downloading()
    {
        std::lock_guard<std::mutex> guard(m_lock);
        return m_inDownload;
    }
    QStringList fetched()
    {
        std::lock_guard<std::mutex> guard(m_lock);
        return m_fetched;
    }

private:
    void wait()
    {
        std::unique_lock<std::mutex> guard(m_lock);
        m_inDownload = true;
        m_wake.wait(guard, [this]() { return m_released || (m_killed && !m_stuck); });
        m_inDownload = false;
    }

    const bool m_stuck;
    std::mutex m_lock;
    std::condition_variable m_wake;
    bool m_killed = false;
    bool m_released = false;
    bool m_inDownload = false;
    QStringList m_fetched;
};

const QByteArray catalog = R"({"version":1,"updated":"2026-01-01","sources":[]})";

// a service with one playlist, its download started
iptv::Service* startDownloading(TestHost& host, const QString& dir)
{
    auto* service = new iptv::Service(host, dir + QStringLiteral("/data"), dir + QStringLiteral("/cache"), catalog);
    service->request(QJsonObject { { QStringLiteral("op"), QStringLiteral("start") } });
    QJsonObject source { { QStringLiteral("kind"), QStringLiteral("m3u") },
                         { QStringLiteral("url"), QStringLiteral("https://example.invalid/list.m3u") } };
    service->request(QJsonObject { { QStringLiteral("op"), QStringLiteral("addSource") },
                                   { QStringLiteral("source"), source } });
    for (int i = 0; i < 200 && !host.downloading(); ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return service;
}

double secondsOf(const std::function<void()>& f)
{
    const auto t0 = std::chrono::steady_clock::now();
    f();
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
}

}

int main()
{
    QTemporaryDir tmp;
    // a download that stops when interrupted: stopped at once
    {
        TestHost host(false);
        iptv::Service* service = startDownloading(host, tmp.path() + QStringLiteral("/a"));
        CHECK(host.downloading());
        bool stopped = false;
        const double took = secondsOf([&]() { stopped = service->waitStopped(5000); });
        CHECK(stopped);
        CHECK(took < 1.0);
        delete service;
    }
    // one that never returns: given up within the bound, the service kept
    {
        auto* host = new TestHost(true);
        iptv::Service* service = startDownloading(*host, tmp.path() + QStringLiteral("/b"));
        CHECK(host->downloading());
        bool stopped = true;
        const double took = secondsOf([&]() { stopped = service->waitStopped(1000); });
        CHECK(!stopped);
        CHECK(took >= 0.9 && took < 2.0);
        // (the worker, let go, ends when its download does; the service and
        // the host stay: it uses them)
        host->release();
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
    // the catalog: not downloaded at the start, but when it is shown
    {
        TestHost host(false);
        auto* service = new iptv::Service(host, tmp.path() + QStringLiteral("/c/data"),
                                          tmp.path() + QStringLiteral("/c/cache"), catalog);
        service->request(QJsonObject { { QStringLiteral("op"), QStringLiteral("start") } });
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
        CHECK(host.fetched().isEmpty());
        const QJsonObject answer = service->request(QJsonObject { { QStringLiteral("op"), QStringLiteral("catalog") } });
        CHECK(answer.value(QLatin1String("catalog")).toObject().value(QLatin1String("version")).toInt() == 1);
        for (int i = 0; i < 100 && host.fetched().isEmpty(); ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        CHECK(host.fetched().size() >= 1 && host.fetched().value(0).endsWith(QLatin1String("/catalog.json")));
        CHECK(service->waitStopped(5000));
        delete service;
    }
    std::printf("TV quit: %d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
