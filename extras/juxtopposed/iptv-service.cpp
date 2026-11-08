/* Copyright (C) 2026 VLC authors and VideoLAN. GPL version 2 or later.
 *
 * Live TV: the service (modules/misc/iptv/service.cpp) with a host that
 * plays the network: the most each download may keep, a job that throws,
 * thousands of entries of one name, the back-off of failed refreshes, answers
 * cut short never replacing the ones kept, files written by several threads
 * at once, the automatic guide not undoing a removal made while it looked,
 * the sources removed, the programmes of a list whose ends are not in order,
 * and the tuners asked once each. On Linux, from the root of the tree:
 *   g++ -std=c++17 -O1 -Wall -Wextra -fPIC -DHAVE_ZLIB -I. $(pkg-config --cflags Qt6Core) \
 *       extras/juxtopposed/iptv-service.cpp modules/misc/iptv/service.cpp modules/misc/iptv/servers.cpp \
 *       modules/misc/iptv/autoguide.cpp modules/misc/iptv/playlist.cpp modules/misc/iptv/guide.cpp \
 *       modules/misc/iptv/xtream.cpp $(pkg-config --libs Qt6Core) -lz -o iptv-service && ./iptv-service
 */
#include "modules/misc/iptv/service.hpp"

#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSet>
#include <QTemporaryDir>
#include <QTimeZone>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <stdexcept>
#include <thread>
#include <zlib.h>

static std::atomic<int> checks { 0 };
static std::atomic<int> failures { 0 };
#define CHECK(condition) do { ++checks; if (!(condition)) { \
    std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); ++failures; } } while (0)

namespace {

using iptv::Datagram;
using iptv::HttpCall;
using iptv::HttpReply;

bool writeFile(const QString& path, const QByteArray& data)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
}

// The network, as each test wants it (its functions may be called from
// several threads at once)
class FakeHost : public iptv::Host
{
public:
    std::function<bool(const QString& url, const QString& path)> onDownload;
    std::function<QByteArray(const QString& url)> onFetch;
    std::function<HttpReply(const HttpCall&)> onCall;
    std::function<QVector<Datagram>(quint16 port)> onBroadcast;

    struct Download
    {
        QString url;
        qint64 limit;
    };

    bool download(const QString& url, const QString& path, QString* error, qint64 limit,
                  const std::function<void(qint64, qint64)>&) override
    {
        {
            std::lock_guard<std::mutex> guard(m_lock);
            m_downloads.append({ url, limit });
        }
        if (onDownload && onDownload(url, path))
            return true;
        if (error)
            *error = QStringLiteral("none here");
        return false;
    }
    QByteArray fetch(const QString& url, QString* error, qint64) override
    {
        {
            std::lock_guard<std::mutex> guard(m_lock);
            m_fetches << url;
        }
        const QByteArray answer = onFetch ? onFetch(url) : QByteArray();
        if (answer.isEmpty() && error)
            *error = QStringLiteral("none here");
        return answer;
    }
    bool storeSecret(const QString&, const QByteArray&) override { return false; }
    QByteArray loadSecret(const QString&, bool* found) override
    {
        *found = false;
        return {};
    }
    void removeSecret(const QString&) override {}
    void notify(const QJsonObject&) override {}
    void log(const QString& message) override
    {
        std::lock_guard<std::mutex> guard(m_lock);
        m_logs << message;
    }
    void debug(const QString&) override {}
    void enterWorker() override {}
    void killWorker() override {}
    HttpReply call(const HttpCall& request) override
    {
        {
            std::lock_guard<std::mutex> guard(m_lock);
            m_calls.append(request);
        }
        return onCall ? onCall(request) : HttpReply();
    }
    QVector<Datagram> broadcast(quint16 port, const QByteArray&, int, const QString&) override
    {
        return onBroadcast ? onBroadcast(port) : QVector<Datagram>();
    }
    QString deviceId() override { return QStringLiteral("test"); }

    QVector<Download> downloads()
    {
        std::lock_guard<std::mutex> guard(m_lock);
        return m_downloads;
    }
    qint64 limitOf(const QString& part)
    {
        for (const Download& d : downloads())
            if (d.url.contains(part))
                return d.limit;
        return -1;
    }
    int fetchesOf(const QString& part)
    {
        std::lock_guard<std::mutex> guard(m_lock);
        int n = 0;
        for (const QString& url : std::as_const(m_fetches))
            n += url.contains(part) ? 1 : 0;
        return n;
    }
    QVector<HttpCall> calls()
    {
        std::lock_guard<std::mutex> guard(m_lock);
        return m_calls;
    }
    QStringList logs()
    {
        std::lock_guard<std::mutex> guard(m_lock);
        return m_logs;
    }

private:
    std::mutex m_lock;
    QVector<Download> m_downloads;
    QStringList m_fetches;
    QVector<HttpCall> m_calls;
    QStringList m_logs;
};

bool waitFor(const std::function<bool()>& done, int ms = 10000)
{
    for (int waited = 0; waited < ms; waited += 10)
    {
        if (done())
            return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return done();
}

const QByteArray kCatalog = R"({"version":1,"updated":"2026-01-01","sources":[
    {"id":"epg-gb","kind":"epg","url":"https://iptv-epg.org/files/epg-gb.xml.gz","countries":["GB"]}]})";

struct Tv
{
    FakeHost& host;
    QString dir;
    iptv::Service service;

    Tv(FakeHost& h, const QString& d)
        : host(h), dir(d), service(h, d + QStringLiteral("/data"), d + QStringLiteral("/cache"), kCatalog)
    {
        ask({ { QStringLiteral("op"), QStringLiteral("start") } });
    }
    ~Tv() { CHECK(service.waitStopped(5000)); }

    QJsonObject ask(const QJsonObject& r) { return service.request(r); }
    QString add(const QJsonObject& source)
    {
        return ask({ { QStringLiteral("op"), QStringLiteral("addSource") }, { QStringLiteral("source"), source } })
            .value(QLatin1String("id")).toString();
    }
    QJsonArray channels()
    {
        return ask({ { QStringLiteral("op"), QStringLiteral("channels") } }).value(QLatin1String("channels")).toArray();
    }
    QJsonObject status(const QString& id)
    {
        for (const QJsonValue& v : ask({ { QStringLiteral("op"), QStringLiteral("status") } })
                                       .value(QLatin1String("sources")).toArray())
            if (v.toObject().value(QLatin1String("id")).toString() == id)
                return v.toObject();
        return {};
    }
    QString sourceDir(const QString& id) const { return dir + QStringLiteral("/cache/sources/") + id; }
};

QJsonObject m3uSource(const QString& url = QStringLiteral("https://lists.example/list.m3u"))
{
    return { { QStringLiteral("kind"), QStringLiteral("m3u") }, { QStringLiteral("url"), url } };
}

QByteArray xmltvTime(qint64 secs)
{
    return QDateTime::fromSecsSinceEpoch(secs, QTimeZone::utc()).toString(QStringLiteral("yyyyMMddHHmmss")).toLatin1()
           + " +0000";
}

// 1: each download asked to keep its most: a playlist, a guide, a logo
void testLimits(const QString& dir)
{
    FakeHost host;
    host.onDownload = [](const QString& url, const QString& path) {
        if (!url.endsWith(QLatin1String("/list.m3u")))
            return false;
        return writeFile(path, "#EXTM3U url-tvg=\"https://epg.example/g.xml\"\n"
                               "#EXTINF:-1 tvg-id=\"a.uk\" tvg-logo=\"https://logo.example/a.png\",A\nhttp://s/a\n");
    };
    Tv tv(host, dir);
    tv.add(m3uSource());
    CHECK(waitFor([&]() { return host.limitOf(QStringLiteral("epg.example/g.xml")) >= 0; }));
    tv.ask({ { QStringLiteral("op"), QStringLiteral("logo") }, { QStringLiteral("url"), QStringLiteral("https://logo.example/a.png") } });
    CHECK(host.limitOf(QStringLiteral("/list.m3u")) == iptv::kPlaylistMaxBytes);
    CHECK(host.limitOf(QStringLiteral("epg.example/g.xml")) == iptv::kGuideMaxBytes);
    CHECK(host.limitOf(QStringLiteral("logo.example/a.png")) == iptv::kLogoMaxBytes);
    CHECK(iptv::kPlaylistMaxBytes == 512LL << 20 && iptv::kGuideMaxBytes == 2LL << 30
          && iptv::kLogoMaxBytes == 2LL << 20);
}

// 2: a job that throws (out of memory) is given up, the worker goes on and
// stops as always; no source stays busy
void testThrowingJob(const QString& dir)
{
    FakeHost host;
    std::atomic<int> attempts { 0 };
    host.onDownload = [&](const QString& url, const QString& path) {
        if (!url.endsWith(QLatin1String("/list.m3u")))
            return false;
        if (attempts++ == 0)
            throw std::runtime_error("out of memory, say");
        return writeFile(path, "#EXTM3U\n#EXTINF:-1,A\nhttp://s/a\n");
    };
    Tv tv(host, dir);
    const QString id = tv.add(m3uSource());
    CHECK(waitFor([&]() { return !host.logs().filter(QStringLiteral("a task failed")).isEmpty(); }));
    CHECK(waitFor([&]() { return !tv.status(id).value(QLatin1String("busy")).toBool(); }));
    tv.ask({ { QStringLiteral("op"), QStringLiteral("refresh") }, { QStringLiteral("id"), id } });
    CHECK(waitFor([&]() { return tv.channels().size() == 1; }));
    // (the start may have refreshed it too, before the refresh asked)
    CHECK(attempts >= 2);
}

// 4: thousands of entries of one name: their keys #2, #3... at once (the
// suffix looked for from #2 each time took minutes under the lock)
void testDuplicates(const QString& dir)
{
    const int count = 30000;
    // the first, then one whose own name is the key the second would get
    // (skipped, as before), then the others of that name, and one more whose
    // name is a key given already
    QByteArray m3u = "#EXTM3U\n#EXTINF:-1,Same\nhttp://s/first\n#EXTINF:-1,Same#2\nhttp://s/own\n";
    for (int i = 1; i < count; ++i)
        m3u += "#EXTINF:-1,Same\nhttp://s/" + QByteArray::number(i) + "\n";
    m3u += "#EXTINF:-1,Same#5\nhttp://s/late\n";
    const int total = count + 2;
    FakeHost host;
    host.onDownload = [&](const QString& url, const QString& path) {
        return url.endsWith(QLatin1String("/list.m3u")) && writeFile(path, m3u);
    };
    Tv tv(host, dir);
    QElapsedTimer took;
    took.start();
    const QString id = tv.add(m3uSource());
    QJsonArray list;
    // (asked every 100 ms, two minutes at most: the list is large to answer)
    while (took.elapsed() < 120000)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        list = tv.channels();
        if (list.size() == total)
            break;
    }
    CHECK(list.size() == total);
    std::printf("%d entries of one name: their channels in %lld ms\n", total, (long long)took.elapsed());
    CHECK(took.elapsed() < 30000);
    if (list.size() != total)
        return;
    QSet<QString> keys;
    for (const QJsonValue& v : list)
        keys.insert(v.toObject().value(QLatin1String("key")).toString());
    CHECK(keys.size() == total);
    auto keyAt = [&](int i) { return list.at(i).toObject().value(QLatin1String("key")).toString(); };
    const QString base = id + QStringLiteral("/n:Same");
    // the same keys as before (favourites and the last channel are kept by
    // them): #2 is the list's own, the next ones #3, #4...
    CHECK(keyAt(0) == base);
    CHECK(keyAt(1) == base + QStringLiteral("#2"));
    CHECK(keyAt(2) == base + QStringLiteral("#3"));
    CHECK(keyAt(3) == base + QStringLiteral("#4"));
    CHECK(keyAt(count) == base + QStringLiteral("#%1").arg(count + 1));
    CHECK(keyAt(count + 1) == base + QStringLiteral("#5#2"));
}

// 8: the refreshes by themselves: after the period; failed, 15 minutes after
// the last, then 30, 60... up to the period (never a sign-in every minute)
void testBackOff()
{
    using iptv::refreshDue;
    using iptv::retryDelaySecs;
    CHECK(retryDelaySecs(0, 24) == 86400);
    CHECK(retryDelaySecs(1, 24) == 900);
    CHECK(retryDelaySecs(2, 24) == 1800);
    CHECK(retryDelaySecs(3, 24) == 3600);
    CHECK(retryDelaySecs(7, 24) == 57600);
    CHECK(retryDelaySecs(8, 24) == 86400);
    CHECK(retryDelaySecs(1000, 24) == 86400);
    CHECK(retryDelaySecs(3, 6) == 3600 && retryDelaySecs(6, 6) == 21600);
    CHECK(retryDelaySecs(1, 0) < 0);
    const qint64 now = 1800000000;
    // never refreshed, or older than the period
    CHECK(refreshDue(now, 0, 0, 0, 24));
    CHECK(!refreshDue(now, now - 100, now - 100, 0, 24));
    CHECK(refreshDue(now, now - 86401, now - 86401, 0, 24));
    // failed once (a password refused): not a minute later, nor the next
    // checks, but 15 minutes after
    CHECK(!refreshDue(now, 0, now - 60, 1, 24));
    CHECK(!refreshDue(now, now - 90000, now - 120, 1, 24));
    CHECK(!refreshDue(now, 0, now - 899, 1, 24));
    CHECK(refreshDue(now, 0, now - 900, 1, 24));
    CHECK(!refreshDue(now, 0, now - 3599, 3, 24));
    CHECK(refreshDue(now, 0, now - 3600, 3, 24));
    CHECK(!refreshDue(now, 0, now - 86399, 40, 24));
    CHECK(refreshDue(now, 0, now - 86400, 40, 24));
    // "Never" refreshed by itself
    CHECK(!refreshDue(now, 0, 0, 0, 0));
    CHECK(!refreshDue(now, 0, 0, 5, 0));
    // the checks of one day of a source always refused: a sign-in a minute
    // was 1440; now at most 15, 30, 60... minutes apart
    int signIns = 0, failed = 0;
    qint64 last = 0;
    for (qint64 t = now; t < now + 86400; t += 60)
        if (refreshDue(t, 0, last, failed, 24))
        {
            ++signIns;
            ++failed;
            last = t;
        }
    std::printf("a source always refused: %d sign-ins in a day\n", signIns);
    CHECK(signIns <= 8);
}

const QByteArray kAccount = R"({"user_info":{"auth":1,"status":"Active","exp_date":"1893456000"},"server_info":{}})";
const QByteArray kStreams = R"([{"stream_id":1,"name":"One","category_id":"3"},{"stream_id":2,"name":"Two","category_id":"3"}])";
const QByteArray kCategories = R"([{"category_id":"3","category_name":"News"}])";
const QByteArray kFilms = R"([{"stream_id":7,"name":"Film","category_id":"5"}])";

// 9, 10: an answer cut short never replaces the one kept; the films written
// by several threads at once are always there whole
void testCutShortAndConcurrent(const QString& dir)
{
    FakeHost host;
    std::atomic<bool> cutShort { false };
    std::atomic<int> delayMs { 0 };
    host.onFetch = [&](const QString& url) -> QByteArray {
        if (delayMs > 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
        auto truncated = [](const QByteArray& json) { return json.left(json.size() / 2); };
        if (url.contains(QLatin1String("action=get_live_streams")))
            return cutShort ? truncated(kStreams) : kStreams;
        if (url.contains(QLatin1String("action=get_live_categories")))
            return cutShort ? truncated(kCategories) : kCategories;
        if (url.contains(QLatin1String("action=get_vod_streams")))
            return cutShort ? truncated(kFilms) : kFilms;
        if (url.contains(QLatin1String("action=get_vod_categories")))
            return cutShort ? QByteArray("[{\"categ") : QByteArray(R"([{"category_id":"5","category_name":"Films"}])");
        if (url.contains(QLatin1String("action=")))
            return "[]";
        return kAccount;
    };
    Tv tv(host, dir);
    const QString id = tv.add({ { QStringLiteral("kind"), QStringLiteral("xtream") },
                                { QStringLiteral("server"), QStringLiteral("http://panel.example") },
                                { QStringLiteral("username"), QStringLiteral("me") },
                                { QStringLiteral("password"), QStringLiteral("secret") } });
    CHECK(waitFor([&]() { return tv.channels().size() == 2; }));
    CHECK(waitFor([&]() { return !tv.status(id).value(QLatin1String("busy")).toBool(); }));
    const QString streamsPath = tv.sourceDir(id) + QStringLiteral("/live_streams.json");
    QFile kept(streamsPath);
    CHECK(kept.open(QIODevice::ReadOnly) && kept.readAll() == kStreams);
    kept.close();
    auto films = [&](bool force) {
        return tv.ask({ { QStringLiteral("op"), QStringLiteral("vod") }, { QStringLiteral("id"), id },
                        { QStringLiteral("kind"), QStringLiteral("movie") }, { QStringLiteral("force"), force } })
            .value(QLatin1String("items")).toArray();
    };
    CHECK(films(false).size() == 1);
    CHECK(films(false).at(0).toObject().value(QLatin1String("category")).toString() == QStringLiteral("Films"));

    // the connection lost in the middle of the answers: what was kept stays
    cutShort = true;
    const int before = host.fetchesOf(QStringLiteral("action=get_live_streams"));
    tv.ask({ { QStringLiteral("op"), QStringLiteral("refresh") }, { QStringLiteral("id"), id } });
    CHECK(waitFor([&]() { return host.fetchesOf(QStringLiteral("action=get_live_streams")) > before; }));
    CHECK(waitFor([&]() {
        const QJsonObject s = tv.status(id);
        return !s.value(QLatin1String("busy")).toBool() && !s.value(QLatin1String("error")).toString().isEmpty();
    }));
    CHECK(tv.channels().size() == 2);
    CHECK(kept.open(QIODevice::ReadOnly) && kept.readAll() == kStreams);
    kept.close();
    QFile categories(tv.sourceDir(id) + QStringLiteral("/live_categories.json"));
    CHECK(categories.open(QIODevice::ReadOnly) && categories.readAll() == kCategories);
    const QJsonArray stillThere = films(true);
    CHECK(stillThere.size() == 1);
    CHECK(stillThere.at(0).toObject().value(QLatin1String("category")).toString() == QStringLiteral("Films"));
    cutShort = false;

    // the films asked again by several threads at once: written each time,
    // always read whole (one shared part file, the one there removed before
    // the rename, made some answers empty)
    delayMs = 2;
    std::atomic<int> empty { 0 };
    std::vector<std::thread> threads;
    for (int t = 0; t < 8; ++t)
        threads.emplace_back([&]() {
            for (int i = 0; i < 25; ++i)
                if (films(true).size() != 1)
                    ++empty;
        });
    for (std::thread& t : threads)
        t.join();
    delayMs = 0;
    std::printf("films written by 8 threads at once: %d of 200 answers without them\n", int(empty));
    CHECK(empty == 0);
    const QStringList left = QDir(tv.sourceDir(id)).entryList(QDir::Files);
    for (const QString& name : left)
        CHECK(name.endsWith(QLatin1String(".json")));
}

// 11: the automatic guide removed while it looked: what it found is not put
// back (nor its guide downloaded)
void testAutoGuideRemovedMeanwhile(const QString& dir)
{
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    QByteArray m3u = "#EXTM3U\n";
    QByteArray known = "[";
    QByteArray xmltv = "<tv>";
    const char* const names[] = { "One", "Two", "Three", "Four", "Five" };
    for (int i = 0; i < 5; ++i)
    {
        const QByteArray name(names[i]);
        m3u += "#EXTINF:-1 tvg-id=\"" + name + ".uk\"," + name + "\nhttp://s/" + name + "\n";
        known += QByteArray(i ? "," : "") + R"({"id":")" + name + R"(.uk","name":")" + name
                 + R"(","alt_names":[],"country":"UK","closed":null})";
        xmltv += "<channel id=\"" + name + ".uk\"><display-name>" + name + "</display-name></channel>"
                 + "<programme start=\"" + xmltvTime(now - 3600) + "\" stop=\"" + xmltvTime(now + 3600)
                 + "\" channel=\"" + name + ".uk\"><title>T</title></programme>";
    }
    known += "]";
    xmltv += "</tv>";
    FakeHost host;
    std::mutex lock;
    std::condition_variable wake;
    bool holding = false, released = false;
    host.onDownload = [&](const QString& url, const QString& path) {
        if (url.endsWith(QLatin1String("/list.m3u")))
            return writeFile(path, m3u);
        if (url.contains(QLatin1String("iptv-org.github.io")))
        {
            std::unique_lock<std::mutex> guard(lock);
            holding = true;
            wake.notify_all();
            wake.wait(guard, [&]() { return released; });
            return writeFile(path, known);
        }
        if (url.contains(QLatin1String("iptv-epg.org")))
            return writeFile(path, xmltv);
        return false;
    };
    Tv tv(host, dir);
    const QString id = tv.add(m3uSource());
    {
        std::unique_lock<std::mutex> guard(lock);
        CHECK(wake.wait_for(guard, std::chrono::seconds(10), [&]() { return holding; }));
    }
    // the user removes the automatic guide while it looks
    const QJsonObject removed = tv.ask({ { QStringLiteral("op"), QStringLiteral("removeAutoGuide") }, { QStringLiteral("id"), id } });
    CHECK(!removed.contains(QLatin1String("error")));
    {
        std::lock_guard<std::mutex> guard(lock);
        released = true;
        wake.notify_all();
    }
    CHECK(waitFor([&]() { return !tv.status(id).value(QLatin1String("busy")).toBool(); }));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    const QJsonObject automatic = tv.status(id).value(QLatin1String("autoGuide")).toObject();
    CHECK(automatic.value(QLatin1String("off")).toBool());
    CHECK(automatic.value(QLatin1String("matched")).toInt() == 0);
    CHECK(automatic.value(QLatin1String("guides")).toInt() == 0);
    CHECK(host.limitOf(QStringLiteral("iptv-epg.org")) < 0);

    // looked for again by the user, nothing removed meanwhile: found
    tv.ask({ { QStringLiteral("op"), QStringLiteral("findGuide") }, { QStringLiteral("id"), id } });
    CHECK(waitFor([&]() { return tv.status(id).value(QLatin1String("autoGuide")).toObject()
                                     .value(QLatin1String("guides")).toInt() == 1; }));
    CHECK(waitFor([&]() { return tv.status(id).value(QLatin1String("withGuide")).toInt() == 5; }));
    CHECK(!tv.status(id).value(QLatin1String("autoGuide")).toObject().value(QLatin1String("off")).toBool());
}

// 12: a source removed: never an id that is a path or empty, an error when
// there is none, its folder removed on the worker
void testRemoveSource(const QString& dir)
{
    FakeHost host;
    host.onDownload = [](const QString& url, const QString& path) {
        return url.endsWith(QLatin1String("/list.m3u")) && writeFile(path, "#EXTM3U\n#EXTINF:-1,A\nhttp://s/a\n");
    };
    Tv tv(host, dir);
    const QString other = dir + QStringLiteral("/cache/sources/keep");
    CHECK(QDir().mkpath(other) && writeFile(other + QStringLiteral("/playlist.m3u"), "#EXTM3U\n"));
    for (const QString& bad : { QString(), QStringLiteral(".."), QStringLiteral("../sources"), QStringLiteral("a/b"),
                                QStringLiteral("a\\b"), QStringLiteral("/"), QStringLiteral("nope") })
    {
        const QJsonObject answer = tv.ask({ { QStringLiteral("op"), QStringLiteral("removeSource") }, { QStringLiteral("id"), bad } });
        CHECK(answer.value(QLatin1String("error")).toString() == QStringLiteral("no such source"));
    }
    CHECK(QFileInfo::exists(other + QStringLiteral("/playlist.m3u")));
    CHECK(QFileInfo::exists(dir + QStringLiteral("/cache/guides")));
    const QString id = tv.add(m3uSource());
    CHECK(waitFor([&]() { return tv.channels().size() == 1; }));
    CHECK(QFileInfo::exists(tv.sourceDir(id) + QStringLiteral("/playlist.m3u")));
    const QJsonObject answer = tv.ask({ { QStringLiteral("op"), QStringLiteral("removeSource") }, { QStringLiteral("id"), id } });
    CHECK(!answer.contains(QLatin1String("error")));
    CHECK(waitFor([&]() { return !QFileInfo::exists(tv.sourceDir(id)); }));
    CHECK(waitFor([&]() { return tv.channels().isEmpty(); }));
    CHECK(QFileInfo::exists(other + QStringLiteral("/playlist.m3u")));
    // (removed already: an error, nothing else removed)
    CHECK(tv.ask({ { QStringLiteral("op"), QStringLiteral("removeSource") }, { QStringLiteral("id"), id } })
              .value(QLatin1String("error")).toString() == QStringLiteral("no such source"));
}

// 16: the programmes between two times when the ends are not in order (a
// long programme before shorter ones): the long one still on is there
void testProgrammesBetween()
{
    auto entry = [](qint64 start, qint64 stop) {
        iptv::Entry e;
        e.start = start;
        e.stop = stop;
        return e;
    };
    // A runs all along, B and C are over by 5000, D starts at 9000
    const iptv::EntryList overlapping { entry(0, 10000), entry(100, 200), entry(300, 400), entry(9000, 9500) };
    CHECK(iptv::programmesBetween(overlapping, 5000, 9600) == QVector<int>({ 0, 3 }));
    CHECK(iptv::programmesBetween(overlapping, 150, 350) == QVector<int>({ 0, 1, 2 }));
    CHECK(iptv::programmesBetween(overlapping, 10000, 20000).isEmpty());
    CHECK(iptv::programmesBetween(overlapping, -500, 50) == QVector<int>({ 0 }));
    // a list in order, as most are
    const iptv::EntryList ordered { entry(0, 100), entry(100, 200), entry(200, 300) };
    CHECK(iptv::programmesBetween(ordered, 150, 250) == QVector<int>({ 1, 2 }));
    CHECK(iptv::programmesBetween(ordered, 100, 101) == QVector<int>({ 1 }));
    CHECK(iptv::programmesBetween(ordered, 300, 400).isEmpty());
    CHECK(iptv::programmesBetween(iptv::EntryList(), 0, 100).isEmpty());
    // a long one, started 22 hours before, before many short ones that ended
    iptv::EntryList day { entry(0, 200000) };
    for (qint64 t = 1000; t < 80000; t += 1000)
        day.append(entry(t, t + 500));
    day.append(entry(80000, 81000));
    CHECK(iptv::programmesBetween(day, 79800, 80500) == QVector<int>({ 0, int(day.size()) - 1 }));
}

// 18: the tuners found are asked once each, briefly (any computer can answer
// the discovery, a tuner answers from each of its addresses)
QByteArray hdhrReply()
{
    QByteArray payload;
    payload.append(char(0x01)).append(char(0x04)).append(QByteArray::fromHex("00000001"));
    QByteArray packet;
    packet.append(char(0x00)).append(char(0x03));
    packet.append(char((payload.size() >> 8) & 0xff)).append(char(payload.size() & 0xff));
    packet.append(payload);
    const uLong crc = crc32(0L, reinterpret_cast<const Bytef*>(packet.constData()), uInt(packet.size()));
    for (int i = 0; i < 4; ++i)
        packet.append(char((crc >> (8 * i)) & 0xff));
    return packet;
}

void testDiscover(const QString& dir)
{
    FakeHost host;
    host.onBroadcast = [](quint16 port) {
        QVector<Datagram> answers;
        if (port == 65001)
            for (const char* from : { "192.168.1.20", "192.168.1.20", "192.168.1.21", "192.168.1.20" })
                answers.append({ QString::fromLatin1(from), hdhrReply() });
        return answers;
    };
    host.onCall = [](const HttpCall& c) {
        HttpReply reply;
        if (c.url.endsWith(QLatin1String("/discover.json")))
        {
            reply.status = 200;
            reply.body = R"({"DeviceID":"1","FriendlyName":"Tuner"})";
        }
        return reply;
    };
    Tv tv(host, dir);
    const QJsonArray found = tv.ask({ { QStringLiteral("op"), QStringLiteral("discover") } })
                                 .value(QLatin1String("found")).toArray();
    CHECK(found.size() == 2);
    int asked = 0;
    for (const HttpCall& c : host.calls())
        if (c.url.endsWith(QLatin1String("/discover.json")))
        {
            ++asked;
            CHECK(c.timeoutMs == 2000);
        }
    CHECK(asked == 2);
}

}

int main()
{
    QTemporaryDir tmp;
    const QString root = tmp.path();
    testBackOff();
    testProgrammesBetween();
    testLimits(root + QStringLiteral("/limits"));
    testThrowingJob(root + QStringLiteral("/throw"));
    testDuplicates(root + QStringLiteral("/duplicates"));
    testCutShortAndConcurrent(root + QStringLiteral("/xtream"));
    testAutoGuideRemovedMeanwhile(root + QStringLiteral("/autoguide"));
    testRemoveSource(root + QStringLiteral("/remove"));
    testDiscover(root + QStringLiteral("/discover"));
    std::printf("TV service: %d checks, %d failed\n", int(checks), int(failures));
    return failures ? 1 : 0;
}
