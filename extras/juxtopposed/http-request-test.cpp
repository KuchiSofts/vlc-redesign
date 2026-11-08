/* Copyright (C) 2026 VLC authors and VideoLAN. GPL version 2 or later.
 *
 * The interface's HTTP client (modules/gui/qt/util/http_request.cpp): the
 * rules of its redirects (no secret header to another server, no HTTPS to
 * HTTP), then, when the build has a backend (libcurl outside Windows,
 * WinHTTP on Windows), the same through local servers
 * (extras/juxtopposed/http-test-server.py): what each server received,
 * deadlines, timeouts, cancels from another thread, sizes; the work of the
 * interface off its thread (runInBackground, an answer read on the worker
 * thread). With --shutdown, quitting: the requests cancelled, all of it
 * waited for at most 5 s, a stuck worker let go with what it holds (run it
 * without the leak checker: that worker is left on purpose).
 *
 * Linux, from the root of the tree, with g++, Qt 6 Core, libcurl, Python 3:
 *   g++ -std=c++17 -O1 -Wall -Wextra -fPIC -DQT_HAS_LIBCURL -I. -Imodules/gui/qt \
 *       $(pkg-config --cflags Qt6Core libcurl) extras/juxtopposed/http-request-test.cpp \
 *       modules/gui/qt/util/http_request.cpp $(pkg-config --libs Qt6Core libcurl) -o http-request-test
 *   ./http-request-test python3 extras/juxtopposed/http-test-server.py
 * Windows: the same sources with MinGW, Qt 6 Core and -lwinhttp (no libcurl).
 */
#include "modules/gui/qt/util/http_request.hpp"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QStringList>
#include <QThread>
#include <QTimer>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <functional>
#include <memory>
#include <random>
#include <thread>
#include <vector>

#ifdef _WIN32
# include <windows.h>
#endif

static int checks = 0;
static int failures = 0;
#define CHECK(condition) do { ++checks; if (!(condition)) { \
    std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); ++failures; } } while (0)

static QByteArray headerOf(const HttpRequest &request, const char *name)
{
    for (const auto &header : request.headers)
        if (header.first.compare(name, Qt::CaseInsensitive) == 0)
            return header.second;
    return {};
}

// a request carrying every kind of secret, and what is none
static HttpRequest secretRequest(const QUrl &url)
{
    HttpRequest request;
    request.url = url;
    request.headers = {
        { "User-Agent", "VLC-Redesign-test" }, { "Accept", "application/json" }, { "Referer", "https://a.example/" },
        { "Authorization", "Bearer TOKEN" }, { "X-API-Key", "KEY1" }, { "X-Subs-Api-Key", "KEY2" },
        { "Api-Key", "KEY3" }, { "Cookie", "session=1" }, { "X-User-Agent", "registered" },
    };
    request.username = QStringLiteral("me");
    request.password = QStringLiteral("secret");
    return request;
}

static const char *const secrets[] = { "Authorization", "X-API-Key", "X-Subs-Api-Key", "Api-Key", "Cookie",
                                       "X-User-Agent" };

static void testRules()
{
    QString error;
    // the same server: everything goes
    HttpRequest same = secretRequest(QUrl(QStringLiteral("https://api.example/a")));
    CHECK(httpFollowRedirect(same, 302, QUrl(QStringLiteral("https://API.example:443/b")), &error));
    CHECK(headerOf(same, "Authorization") == "Bearer TOKEN" && headerOf(same, "X-API-Key") == "KEY1");
    CHECK(same.username == QLatin1String("me") && same.url.path() == QLatin1String("/b"));
    // another host, port or scheme: no secret
    const struct { const char *from, *to; } others[] = {
        { "https://api.example/a", "https://cdn.example/b" },      // host
        { "https://api.example/a", "https://api.example:8443/b" }, // port
        { "http://api.example/a", "https://api.example/b" },      // scheme
        { "http://api.example:443/a", "https://api.example/b" },  // scheme, the same number
    };
    for (const auto &other : others)
    {
        HttpRequest request = secretRequest(QUrl(QString::fromLatin1(other.from)));
        CHECK(httpFollowRedirect(request, 302, QUrl(QString::fromLatin1(other.to)), &error));
        for (const char *name : secrets)
            CHECK(headerOf(request, name).isEmpty());
        CHECK(headerOf(request, "User-Agent") == "VLC-Redesign-test" && headerOf(request, "Accept") == "application/json"
              && headerOf(request, "Referer") == "https://a.example/");
        CHECK(request.username.isEmpty() && request.password.isEmpty());
        CHECK(request.url == QUrl(QString::fromLatin1(other.to)));
    }
    // http to https (another origin) goes; https to http never
    HttpRequest up = secretRequest(QUrl(QStringLiteral("http://api.example/a")));
    CHECK(httpFollowRedirect(up, 301, QUrl(QStringLiteral("https://api.example/a")), &error));
    CHECK(headerOf(up, "Authorization").isEmpty());
    HttpRequest down = secretRequest(QUrl(QStringLiteral("https://api.example/a")));
    CHECK(!httpFollowRedirect(down, 301, QUrl(QStringLiteral("http://api.example/a")), &error));
    CHECK(error.contains(QLatin1String("HTTPS to HTTP")) && down.url.scheme() == QLatin1String("https"));
    // other schemes, no host
    for (const char *target : { "file:///etc/passwd", "ftp://api.example/x", "javascript:alert(1)", "https:///x" })
    {
        HttpRequest request = secretRequest(QUrl(QStringLiteral("https://api.example/a")));
        CHECK(!httpFollowRedirect(request, 302, QUrl(QString::fromLatin1(target)), &error));
    }
    // the methods: 303 to GET (HEAD stays), 301/302 of a POST to GET, 307/308 keep it
    const auto post = []() {
        HttpRequest request;
        request.url = QUrl(QStringLiteral("https://api.example/a"));
        request.method = "POST";
        request.body = "{\"a\":1}";
        request.headers = { { "Content-Type", "application/json" }, { "Accept", "*/*" } };
        return request;
    };
    for (int status : { 301, 302, 303 })
    {
        HttpRequest request = post();
        CHECK(httpFollowRedirect(request, status, QUrl(QStringLiteral("https://api.example/b")), &error));
        CHECK(request.method == "GET" && request.body.isEmpty() && headerOf(request, "Content-Type").isEmpty());
        CHECK(headerOf(request, "Accept") == "*/*");
    }
    for (int status : { 307, 308 })
    {
        HttpRequest request = post();
        CHECK(httpFollowRedirect(request, status, QUrl(QStringLiteral("https://cdn.example/b")), &error));
        CHECK(request.method == "POST" && request.body == "{\"a\":1}"
              && headerOf(request, "Content-Type") == "application/json");
    }
    HttpRequest head;
    head.method = "HEAD";
    head.url = QUrl(QStringLiteral("https://api.example/a"));
    CHECK(httpFollowRedirect(head, 303, QUrl(QStringLiteral("https://api.example/b")), &error) && head.method == "HEAD");
    HttpRequest put = post();
    put.method = "PUT";
    CHECK(httpFollowRedirect(put, 302, QUrl(QStringLiteral("https://api.example/b")), &error) && put.method == "PUT"
          && !put.body.isEmpty());
}

// ---------------------------------------------------------------------------
// Through the local servers

struct Servers
{
    QProcess process;
    int first = 0;  // 127.0.0.1
    int second = 0; // 127.0.0.2: another host
    int third = 0;  // 127.0.0.1: the same host, another port

    QString url(int port, const char *path, const char *host = "127.0.0.1") const
    {
        return QStringLiteral("http://%1:%2%3").arg(QLatin1String(host)).arg(port).arg(QLatin1String(path));
    }
};

static bool startServers(Servers &servers, const QString &python, const QString &script)
{
    servers.process.setProcessChannelMode(QProcess::ForwardedErrorChannel);
    servers.process.start(python, { script });
    if (!servers.process.waitForStarted(10000))
        return false;
    QByteArray line;
    QElapsedTimer timer;
    timer.start();
    while (!line.contains('\n') && timer.elapsed() < 15000)
        if (servers.process.waitForReadyRead(1000))
            line += servers.process.readAll();
    const QList<QByteArray> words = line.trimmed().split(' ');
    if (words.size() != 4 || words[0] != "PORTS")
        return false;
    servers.first = words[1].toInt();
    servers.second = words[2].toInt();
    servers.third = words[3].toInt();
    return servers.first > 0 && servers.second > 0 && servers.third > 0;
}

static QJsonObject echoOf(const HttpResponse &response)
{
    return QJsonDocument::fromJson(response.body).object();
}

static bool received(const QJsonObject &echo, const char *name)
{
    const QJsonObject headers = echo.value(QLatin1String("headers")).toObject();
    for (auto it = headers.begin(); it != headers.end(); ++it)
        if (it.key().compare(QLatin1String(name), Qt::CaseInsensitive) == 0)
            return true;
    return false;
}

static QString redirectTo(const Servers &s, int port, const char *code, const QString &to,
                          const char *host = "127.0.0.1")
{
    return s.url(port, "/redirect", host) + QStringLiteral("?code=%1&to=").arg(QLatin1String(code))
           + QString::fromLatin1(QUrl::toPercentEncoding(to));
}

static void testRedirects(const Servers &s)
{
    // the same server (a relative Location): its secrets go
    HttpResponse r = httpRequestSync(secretRequest(QUrl(redirectTo(s, s.first, "302", QStringLiteral("/echo")))));
    QJsonObject echo = echoOf(r);
    CHECK(r.error.isEmpty() && r.status == 200 && echo.value(QLatin1String("port")).toInt() == s.first);
    for (const char *name : secrets)
        CHECK(received(echo, name));

    // another host and port, the same host on another port: none of them
    for (const QString &target : { s.url(s.second, "/echo", "127.0.0.2"), s.url(s.third, "/echo") })
        for (const char *code : { "301", "302", "303", "307", "308" })
        {
            r = httpRequestSync(secretRequest(QUrl(redirectTo(s, s.first, code, target))));
            echo = echoOf(r);
            CHECK(r.error.isEmpty() && r.status == 200 && echo.value(QLatin1String("path")).toString() == QLatin1String("/echo"));
            for (const char *name : secrets)
                CHECK(!received(echo, name));
            CHECK(received(echo, "User-Agent") && received(echo, "Referer") && received(echo, "Accept"));
        }
    // away and back: dropped they stay
    const QString back = redirectTo(s, s.second, "302", s.url(s.first, "/echo"), "127.0.0.2");
    r = httpRequestSync(secretRequest(QUrl(redirectTo(s, s.first, "302", back))));
    echo = echoOf(r);
    CHECK(r.status == 200 && echo.value(QLatin1String("port")).toInt() == s.first && !received(echo, "Authorization"));

    // the methods and bodies
    HttpRequest post;
    post.method = "POST";
    post.body = "{\"a\":1}";
    post.headers = { { "Content-Type", "application/json" }, { "X-API-Key", "KEY1" } };
    post.url = QUrl(redirectTo(s, s.first, "307", s.url(s.second, "/echo", "127.0.0.2")));
    echo = echoOf(httpRequestSync(post));
    CHECK(echo.value(QLatin1String("method")).toString() == QLatin1String("POST")
          && echo.value(QLatin1String("body")).toString() == QLatin1String("{\"a\":1}")
          && received(echo, "Content-Type") && !received(echo, "X-API-Key"));
    post.url = QUrl(redirectTo(s, s.first, "303", s.url(s.first, "/echo")));
    echo = echoOf(httpRequestSync(post));
    CHECK(echo.value(QLatin1String("method")).toString() == QLatin1String("GET")
          && echo.value(QLatin1String("body")).toString().isEmpty() && received(echo, "X-API-Key"));

    // a loop, another scheme
    r = httpRequestSync(secretRequest(QUrl(s.url(s.first, "/loop"))));
    CHECK(r.error == QLatin1String("Too many redirects"));
    for (const char *target : { "file:///etc/passwd", "ftp://127.0.0.1/x" })
    {
        r = httpRequestSync(secretRequest(QUrl(redirectTo(s, s.first, "302", QString::fromLatin1(target)))));
        CHECK(!r.error.isEmpty() && r.body.isEmpty());
    }
    // a redirect without its address: the answer as it is
    r = httpRequestSync(secretRequest(QUrl(s.url(s.first, "/redirect?code=302&to="))));
    CHECK(r.error.isEmpty() && r.status == 302);

    // the account: to its server, not to another one
    HttpRequest account;
    account.url = QUrl(s.url(s.first, "/auth"));
    account.username = QStringLiteral("me");
    account.password = QStringLiteral("secret");
    r = httpRequestSync(account);
    CHECK(r.status == 200 && received(echoOf(r), "Authorization"));
    account.url = QUrl(redirectTo(s, s.first, "302", s.url(s.second, "/auth", "127.0.0.2")));
    r = httpRequestSync(account);
    CHECK(r.status == 401);
}

static long long msSince(const std::chrono::steady_clock::time_point &start)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
}

static void testBounds(const Servers &s)
{
    // A deadline over a slow answer (a byte every 100 ms, 20 s in all),
    // then across redirects; a step that never ends (20 s of nothing).
    // (The bounds leave seconds for the system: WinHTTP may first look for
    // the proxy of the network)
    HttpRequest slow;
    slow.url = QUrl(s.url(s.first, "/slow-body?ms=100&bytes=200"));
    slow.deadlineMs = 1000;
    auto start = std::chrono::steady_clock::now();
    HttpResponse r = httpRequestSync(slow);
    const long long slowMs = msSince(start);
    CHECK(r.error == QLatin1String("Timed out") && slowMs < 8000);
    HttpRequest chain;
    chain.url = QUrl(redirectTo(s, s.first, "302", s.url(s.first, "/slow-body?ms=100&bytes=200")));
    chain.deadlineMs = 800;
    start = std::chrono::steady_clock::now();
    r = httpRequestSync(chain);
    const long long chainMs = msSince(start);
    CHECK(r.error == QLatin1String("Timed out") && chainMs < 8000);
    HttpRequest stall;
    stall.url = QUrl(s.url(s.first, "/stall?ms=20000"));
    stall.timeoutMs = 1000;
    start = std::chrono::steady_clock::now();
    r = httpRequestSync(stall);
    const long long stalled = msSince(start);
    CHECK(!r.error.isEmpty() && stalled < 10000);
    std::printf("deadline of 1 s: %lld ms; across a redirect, of 0.8 s: %lld ms; a stalled answer, steps of "
                "1 s: %lld ms (%s)\n", slowMs, chainMs, stalled, qPrintable(r.error));
    // too large: maxBytes + 1, no error
    HttpRequest big;
    big.url = QUrl(s.url(s.first, "/big?bytes=50000"));
    big.maxBytes = 1000;
    r = httpRequestSync(big);
    CHECK(r.error.isEmpty() && r.body.size() == 1001);

    // Cancelled from another thread, in each step (connecting, waiting for
    // the headers, reading, between two hops): it ends within a moment
    // (libcurl looks at it about once a second while nothing comes), never
    // after the whole of its answer (3 s and more)
    const char *const slowPaths[] = { "/slow-headers?ms=3000", "/slow-body?ms=20&bytes=200",
                                      "/redirect?code=302&to=/slow-headers%3Fms%3D3000" };
    int late = 0;
    long long longest = 0;
    for (int n = 0; n < 24; ++n)
    {
        HttpSyncRequest request;
        HttpRequest what;
        what.url = QUrl(s.url(s.first, slowPaths[n % 3]));
        const int after = (n * 37) % 250;
        std::thread canceller([&request, after]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(after));
            request.cancel();
        });
        start = std::chrono::steady_clock::now();
        r = request.run(what);
        const long long took = msSince(start);
        canceller.join();
        late += r.error != QLatin1String("Cancelled") || took > after + 1500;
        longest = std::max(longest, took - after);
    }
    CHECK(late == 0);
    // (and racing the end of quick ones, hundreds of times: cancelled or
    // done, never stuck nor crashed)
    int cancelled = 0, completed = 0;
    for (int n = 0; n < 300; ++n)
    {
        HttpSyncRequest request;
        HttpRequest what;
        what.url = QUrl(s.url(s.first, n % 2 ? "/echo" : "/big?bytes=200000"));
        const int after = n % 7;
        std::thread canceller([&request, after]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(after));
            request.cancel();
        });
        r = request.run(what);
        canceller.join();
        cancelled += r.error == QLatin1String("Cancelled");
        completed += r.error.isEmpty() && r.status == 200;
    }
    CHECK(cancelled + completed == 300);
    std::printf("cancels: the slow ones ended within %lld ms of it; of the quick ones %d cancelled, %d done first\n",
                longest, cancelled, completed);
    // cancelled before it starts
    HttpSyncRequest before;
    before.cancel();
    HttpRequest echo;
    echo.url = QUrl(s.url(s.first, "/echo"));
    CHECK(before.run(echo).error == QLatin1String("Cancelled"));
}

// Many requests at once, each cancelled at a moment of its own (connecting,
// reading a large answer, between two hops, after its end): they all end,
// and nothing is left of them (on Windows: the handles of the process, the
// two events of each request's asynchronous calls among them)
static void testStress(const Servers &s)
{
#ifdef _WIN32
    DWORD handlesBefore = 0;
    GetProcessHandleCount(GetCurrentProcess(), &handlesBefore);
#endif
    const char *const paths[] = { "/echo", "/big?bytes=300000", "/slow-body?ms=1&bytes=400",
                                  "/redirect?code=302&to=/big%3Fbytes%3D100000", "/slow-headers?ms=50" };
    std::atomic<int> ended { 0 }, odd { 0 };
    std::vector<std::thread> threads;
    for (int t = 0; t < 8; ++t)
        threads.emplace_back([&, t]() {
            std::mt19937 random(unsigned(t + 1));
            for (int n = 0; n < 150; ++n)
            {
                HttpSyncRequest request;
                HttpRequest what;
                what.url = QUrl(s.url(s.first, paths[random() % 5]));
                const int after = int(random() % 16);
                std::thread canceller([&request, after]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(after));
                    request.cancel();
                });
                const HttpResponse r = request.run(what);
                canceller.join();
                ++ended;
                if (!(r.error == QLatin1String("Cancelled") || (r.error.isEmpty() && r.status == 200)))
                    ++odd;
            }
        });
    for (std::thread &thread : threads)
        thread.join();
    CHECK(ended == 8 * 150 && odd == 0);
#ifdef _WIN32
    DWORD handlesAfter = 0;
    GetProcessHandleCount(GetCurrentProcess(), &handlesAfter);
    // (WinHTTP keeps a few of its own: its threads, its pools)
    CHECK(handlesAfter < handlesBefore + 200);
    std::printf("stress: %d requests cancelled at random, 8 at once; handles %lu before, %lu after\n",
                ended.load(), handlesBefore, handlesAfter);
#else
    std::printf("stress: %d requests cancelled at random, 8 at once\n", ended.load());
#endif
}

// ---------------------------------------------------------------------------
// The work of the interface off its thread (runInBackground), and the
// answers read on the worker thread (httpRequestAsync's onWorker)

// the events of this thread, until condition or ms
static bool waitUntil(const std::function<bool()> &condition, int ms)
{
    QElapsedTimer timer;
    timer.start();
    while (!condition() && timer.elapsed() < ms)
    {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        QThread::msleep(2);
    }
    return condition();
}

static void testWorkers(const Servers *s)
{
    const QThread *const main = QThread::currentThread();
    // the work on another thread, then done on this one, in that order
    {
        QObject context;
        auto workThread = std::make_shared<const QThread *>(nullptr);
        auto doneThread = std::make_shared<const QThread *>(nullptr);
        auto order = std::make_shared<QByteArray>();
        CHECK(runInBackground(&context, [workThread, order]() {
            *workThread = QThread::currentThread();
            *order += 'w';
        }, [doneThread, order]() {
            *doneThread = QThread::currentThread();
            *order += 'd';
        }));
        CHECK(waitUntil([&]() { return *doneThread != nullptr; }, 5000));
        CHECK(*workThread && *workThread != main && *doneThread == main && *order == "wd");
    }
    // done dropped once its context is gone; the work done all the same
    {
        auto context = std::make_unique<QObject>();
        auto worked = std::make_shared<std::atomic<bool>>(false);
        auto called = std::make_shared<bool>(false);
        CHECK(runInBackground(context.get(), [worked]() {
            QThread::msleep(100);
            *worked = true;
        }, [called]() { *called = true; }));
        context.reset();
        CHECK(waitUntil([&]() { return worked->load(); }, 5000));
        waitUntil([]() { return false; }, 200);
        CHECK(!*called);
    }
    if (!s)
        return;
    // an answer read on the worker thread, then done on this one
    {
        QObject context;
        auto readOn = std::make_shared<const QThread *>(nullptr);
        auto method = std::make_shared<QString>();
        auto doneOn = std::make_shared<const QThread *>(nullptr);
        HttpRequest request;
        request.url = QUrl(s->url(s->first, "/echo"));
        httpRequestAsync(&context, request, [readOn, method](const HttpResponse &reply) {
            *readOn = QThread::currentThread();
            *method = echoOf(reply).value(QLatin1String("method")).toString();
        }, [doneOn, method](const HttpResponse &reply) {
            *doneOn = QThread::currentThread();
            CHECK(reply.status == 200 && *method == QLatin1String("GET"));
        });
        CHECK(waitUntil([&]() { return *doneOn != nullptr; }, 10000));
        CHECK(*readOn && *readOn != main && *doneOn == main);
    }
    // the context gone: its request cancelled at once, done dropped
    {
        auto context = std::make_unique<QObject>();
        struct End { std::atomic<bool> ended { false }; QString error; long long ms = 0; };
        auto end = std::make_shared<End>();
        auto called = std::make_shared<bool>(false);
        HttpRequest request;
        request.url = QUrl(s->url(s->first, "/slow-headers?ms=8000"));
        const auto start = std::chrono::steady_clock::now();
        httpRequestAsync(context.get(), request, [end, start](const HttpResponse &reply) {
            end->error = reply.error;
            end->ms = msSince(start);
            end->ended = true;
        }, [called](const HttpResponse &) { *called = true; });
        waitUntil([]() { return false; }, 200);
        context.reset();
        CHECK(waitUntil([&]() { return end->ended.load(); }, 5000));
        waitUntil([]() { return false; }, 100);
        CHECK(end->error == QLatin1String("Cancelled") && end->ms < 2500 && !*called);
    }
}

// When the application quits: the requests cancelled, then all of them and
// the work waited for together, at most 5 s; what still runs then let go,
// what it holds alive. (Its own run: the stuck work is left on purpose, so
// the leak checker is off for it.)
static int testShutdown(QCoreApplication &app, const Servers *s)
{
    struct Held
    {
        std::atomic<bool> *freed;
        ~Held() { *freed = true; }
    };
    static std::atomic<bool> freed { false };
    static std::atomic<bool> release { false };
    static std::atomic<bool> stuckEnded { false };
    QObject context;
    std::shared_ptr<Held> held(new Held { &freed });
    CHECK(runInBackground(&context, [held]() {
        // stuck (a share that no longer answers): far past the bound
        for (int i = 0; i < 30000 && !release; ++i)
            QThread::msleep(1);
        stuckEnded = true;
    }));
    held.reset();
    auto quickEnded = std::make_shared<std::atomic<bool>>(false);
    CHECK(runInBackground(&context, [quickEnded]() {
        QThread::msleep(300);
        *quickEnded = true;
    }));
    auto requestError = std::make_shared<QString>();
    if (s)
    {
        HttpRequest request;
        request.url = QUrl(s->url(s->first, "/slow-headers?ms=20000"));
        httpRequestAsync(&context, request, [requestError](const HttpResponse &reply) { *requestError = reply.error; },
                         [](const HttpResponse &) {});
    }
    QTimer::singleShot(200, &app, &QCoreApplication::quit);
    const auto start = std::chrono::steady_clock::now();
    app.exec();
    const long long quitMs = msSince(start);
    // the quick work and the request waited for, the stuck work let go at
    // the bound, its function and what it holds alive
    CHECK(quitMs >= 200 + 4500 && quitMs < 200 + 7000);
    CHECK(*quickEnded && !stuckEnded && !freed);
    if (s)
        CHECK(*requestError == QLatin1String("Cancelled"));
    // nothing more is started once quitting
    CHECK(!runInBackground(&context, []() {}));
    std::printf("quitting: %lld ms with a stuck one (the bound: 5 s); the request %s\n", quitMs,
                qPrintable(requestError->isEmpty() ? QStringLiteral("-") : *requestError));
    // (let it end now, before the process does)
    release = true;
    for (int i = 0; i < 5000 && !stuckEnded; ++i)
        QThread::msleep(1);
    return failures;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    const QString python = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral("python3");
    const QString script = argc > 2 ? QString::fromLocal8Bit(argv[2]) : QStringLiteral("extras/juxtopposed/http-test-server.py");
    const bool shutdown = argc > 3 && QByteArray(argv[3]) == "--shutdown";
    if (!shutdown)
        testRules();
    Servers servers;
    const bool network = httpRequestSupported();
    if (network && !startServers(servers, python, script))
    {
        std::fprintf(stderr, "the test servers didn't start (%s %s)\n", qPrintable(python), qPrintable(script));
        ++failures;
    }
    else if (shutdown)
        testShutdown(app, network ? &servers : nullptr);
    else
    {
        if (network)
        {
            testRedirects(servers);
            testBounds(servers);
            testStress(servers);
        }
        else
            std::printf("no HTTP backend in this build: its rules and the work off the thread only\n");
        testWorkers(network ? &servers : nullptr);
    }
    if (network)
    {
        servers.process.closeWriteChannel();
        if (!servers.process.waitForFinished(5000))
            servers.process.kill();
        servers.process.waitForFinished(2000);
    }
    std::printf("http requests%s: %d checks, %d failed\n", shutdown ? " (quitting)" : "", checks, failures);
    return failures ? 1 : 0;
}
