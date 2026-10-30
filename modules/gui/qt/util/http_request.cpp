/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or later.
 *****************************************************************************/
#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "http_request.hpp"

#include <QCoreApplication>
#include <QDeadlineTimer>
#include <QHash>
#include <QObject>
#include <QPointer>
#include <QThread>

#include <algorithm>
#include <atomic>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#ifdef _WIN32
# include <windows.h>
# include <winhttp.h>
# ifndef WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY
#  define WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY 4
# endif
#elif defined(QT_HAS_LIBCURL)
# include <curl/curl.h>
#endif

namespace {
// A request in flight, shared with the UI thread that may cancel it (its
// context gone, the application quitting)
struct RequestState
{
    std::mutex lock;
    std::atomic<bool> cancelled { false };
#ifdef _WIN32
    // the handles open: closing them interrupts the call in progress
    std::vector<HINTERNET> handles;
#endif

    void cancel()
    {
        std::lock_guard<std::mutex> guard(lock);
        cancelled = true;
#ifdef _WIN32
        // the request first, then its connection and session
        for (auto it = handles.rbegin(); it != handles.rend(); ++it)
            WinHttpCloseHandle(*it);
        handles.clear();
#endif
    }
};
}

#ifdef _WIN32
namespace {
// Closed once: by its owner, or by RequestState::cancel() from another thread
struct InternetHandle
{
    HINTERNET handle;
    RequestState& state;
    InternetHandle(RequestState& s, HINTERNET h) : handle(h), state(s)
    {
        if (!handle)
            return;
        std::lock_guard<std::mutex> guard(state.lock);
        if (state.cancelled)
        {
            WinHttpCloseHandle(handle);
            handle = nullptr;
            return;
        }
        state.handles.push_back(handle);
    }
    ~InternetHandle()
    {
        if (!handle)
            return;
        std::lock_guard<std::mutex> guard(state.lock);
        const auto it = std::find(state.handles.begin(), state.handles.end(), handle);
        if (it == state.handles.end())
            return; // closed by cancel()
        state.handles.erase(it);
        WinHttpCloseHandle(handle);
    }
    InternetHandle(const InternetHandle&) = delete;
    InternetHandle& operator=(const InternetHandle&) = delete;
    explicit operator bool() const { return handle != nullptr; }
};

QString lastError(const char* step)
{
    return QStringLiteral("%1 failed (%2)").arg(QLatin1String(step)).arg(GetLastError());
}

HttpResponse perform(const HttpRequest& request, RequestState& state)
{
    HttpResponse response;
    const QUrl& url = request.url;
    const bool https = url.scheme() == QLatin1String("https");
    if (!https && url.scheme() != QLatin1String("http"))
    {
        response.error = QStringLiteral("Unsupported URL scheme");
        return response;
    }
    const QString cancelled = QStringLiteral("Cancelled");
    const QDeadlineTimer deadline = request.deadlineMs > 0 ? QDeadlineTimer(request.deadlineMs)
                                                           : QDeadlineTimer(QDeadlineTimer::Forever);

    // The proxy of the system (its settings, or the script it names), as
    // the browsers use; WinHTTP's own setting before Windows 8.1
    HINTERNET opened = WinHttpOpen(L"VLC", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                                   WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!opened)
        opened = WinHttpOpen(L"VLC", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                             WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    InternetHandle session(state, opened);
    if (!session)
    {
        response.error = state.cancelled ? cancelled : lastError("WinHttpOpen");
        return response;
    }
    // each step at most this long: a stalled read ends the request
    WinHttpSetTimeouts(session.handle, request.timeoutMs, request.timeoutMs,
                       request.timeoutMs, request.timeoutMs);

    const std::wstring host = url.host(QUrl::FullyEncoded).toStdWString();
    const auto port = static_cast<INTERNET_PORT>(url.port(https ? INTERNET_DEFAULT_HTTPS_PORT
                                                                : INTERNET_DEFAULT_HTTP_PORT));
    InternetHandle connection(state, WinHttpConnect(session.handle, host.c_str(), port, 0));
    if (!connection)
    {
        response.error = state.cancelled ? cancelled : lastError("WinHttpConnect");
        return response;
    }

    const std::wstring method = QString::fromLatin1(request.method).toStdWString();
    const std::wstring path = QString::fromLatin1(
        url.toEncoded(QUrl::RemoveScheme | QUrl::RemoveAuthority | QUrl::RemoveFragment)).toStdWString();
    // Redirects are followed, except from HTTPS to HTTP (WinHTTP default policy).
    InternetHandle handle(state, WinHttpOpenRequest(connection.handle, method.c_str(),
                                                    path.empty() ? L"/" : path.c_str(), nullptr,
                                                    WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                                    https ? WINHTTP_FLAG_SECURE : 0));
    if (!handle)
    {
        response.error = state.cancelled ? cancelled : lastError("WinHttpOpenRequest");
        return response;
    }

    QByteArray headers;
    for (const auto& header : request.headers)
        headers += header.first + ": " + header.second + "\r\n";
    const std::wstring wideHeaders = QString::fromUtf8(headers).toStdWString();
    const auto bodySize = static_cast<DWORD>(request.body.size());
    if (!WinHttpSendRequest(handle.handle,
                            wideHeaders.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : wideHeaders.c_str(),
                            static_cast<DWORD>(-1L),
                            bodySize ? const_cast<char*>(request.body.constData()) : WINHTTP_NO_REQUEST_DATA,
                            bodySize, bodySize, 0)
        || !WinHttpReceiveResponse(handle.handle, nullptr))
    {
        response.error = state.cancelled ? cancelled : lastError("HTTP request");
        return response;
    }

    DWORD status = 0;
    DWORD statusSize = sizeof(status);
    WinHttpQueryHeaders(handle.handle, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize, WINHTTP_NO_HEADER_INDEX);
    response.status = static_cast<int>(status);

    DWORD length = 0;
    DWORD lengthSize = sizeof(length);
    if (!WinHttpQueryHeaders(handle.handle, WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX, &length, &lengthSize, WINHTTP_NO_HEADER_INDEX))
        length = 0;
    if (length > 0 && static_cast<qsizetype>(length) <= request.maxBytes)
        response.body.reserve(length);

    char buffer[65536];
    while (response.body.size() <= request.maxBytes)
    {
        DWORD read = 0;
        if (state.cancelled)
        {
            response.error = cancelled;
            break;
        }
        if (deadline.hasExpired())
        {
            response.error = QStringLiteral("Timed out");
            break;
        }
        if (!WinHttpReadData(handle.handle, buffer, sizeof(buffer), &read))
        {
            response.error = state.cancelled ? cancelled : lastError("WinHttpReadData");
            break;
        }
        if (read == 0)
            break;
        response.body.append(buffer, read);
        if (request.progress)
            request.progress(response.body.size(), length);
    }
    if (response.body.size() > request.maxBytes)
        response.body.truncate(request.maxBytes + 1);
    return response;
}
}

bool httpRequestSupported()
{
    return true;
}
#elif defined(QT_HAS_LIBCURL)
namespace {
struct CurlTransfer
{
    const HttpRequest& request;
    RequestState& state;
    HttpResponse& response;
    QDeadlineTimer deadline;
    curl_off_t total = 0;
    bool overflow = false;
    bool expired = false;
};

size_t curlWrite(char* data, size_t size, size_t count, void* opaque)
{
    auto* t = static_cast<CurlTransfer*>(opaque);
    const size_t bytes = size * count;
    if (t->state.cancelled)
        return 0;
    if (t->deadline.hasExpired())
    {
        t->expired = true;
        return 0;
    }
    // as the WinHTTP loop: stop past maxBytes, keeping maxBytes + 1
    const qsizetype room = t->request.maxBytes + 1 - t->response.body.size();
    if (static_cast<qsizetype>(bytes) >= room)
    {
        t->response.body.append(data, room);
        t->overflow = true;
        return 0;
    }
    t->response.body.append(data, static_cast<qsizetype>(bytes));
    if (t->request.progress)
        t->request.progress(t->response.body.size(), t->total > 0 ? t->total : 0);
    return bytes;
}

int curlProgress(void* opaque, curl_off_t dltotal, curl_off_t, curl_off_t, curl_off_t)
{
    auto* t = static_cast<CurlTransfer*>(opaque);
    t->total = dltotal;
    if (t->deadline.hasExpired())
        t->expired = true;
    return (t->state.cancelled || t->expired) ? 1 : 0;
}

HttpResponse perform(const HttpRequest& request, RequestState& state)
{
    static std::once_flag initialized;
    std::call_once(initialized, []() { curl_global_init(CURL_GLOBAL_DEFAULT); });

    HttpResponse response;
    const QUrl& url = request.url;
    const bool https = url.scheme() == QLatin1String("https");
    if (!https && url.scheme() != QLatin1String("http"))
    {
        response.error = QStringLiteral("Unsupported URL scheme");
        return response;
    }
    const QString cancelled = QStringLiteral("Cancelled");
    if (state.cancelled)
    {
        response.error = cancelled;
        return response;
    }

    CURL* curl = curl_easy_init();
    if (!curl)
    {
        response.error = QStringLiteral("curl_easy_init failed");
        return response;
    }
    CurlTransfer transfer { request, state, response,
                            request.deadlineMs > 0 ? QDeadlineTimer(request.deadlineMs)
                                                   : QDeadlineTimer(QDeadlineTimer::Forever) };

    const QByteArray encodedUrl = url.toEncoded(QUrl::RemoveFragment);
    curl_easy_setopt(curl, CURLOPT_URL, encodedUrl.constData());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "VLC");
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    // Redirects are followed, except from HTTPS to HTTP (as WinHTTP)
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
#if LIBCURL_VERSION_NUM >= 0x075500 /* 7.85 */
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "http,https");
    curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS_STR, https ? "https" : "http,https");
#else
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS, static_cast<long>(CURLPROTO_HTTP | CURLPROTO_HTTPS));
    curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS,
                     static_cast<long>(https ? CURLPROTO_HTTPS : (CURLPROTO_HTTP | CURLPROTO_HTTPS)));
#endif
    // each step at most timeoutMs: connecting, then no data for that long
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, static_cast<long>(request.timeoutMs));
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1L);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME,
                     std::max(1L, static_cast<long>(request.timeoutMs / 1000)));

    if (request.method == "GET")
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    else if (request.method == "HEAD")
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    else
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, request.method.constData());
    if (!request.body.isEmpty() || request.method == "POST" || request.method == "PUT")
    {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE,
                         static_cast<curl_off_t>(request.body.size()));
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.body.constData());
    }

    curl_slist* headers = nullptr;
    for (const auto& header : request.headers)
        headers = curl_slist_append(headers, QByteArray(header.first + ": " + header.second).constData());
    // no "Expect: 100-continue" round trip for bodies
    headers = curl_slist_append(headers, "Expect:");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWrite);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &transfer);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, curlProgress);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &transfer);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

    const CURLcode code = curl_easy_perform(curl);
    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    response.status = static_cast<int>(status);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (code != CURLE_OK && !transfer.overflow)
    {
        if (state.cancelled)
            response.error = cancelled;
        else if (transfer.expired)
            response.error = QStringLiteral("Timed out");
        else
            response.error = QStringLiteral("HTTP request failed (%1)")
                                 .arg(QString::fromUtf8(curl_easy_strerror(code)));
    }
    return response;
}
}

bool httpRequestSupported()
{
    return true;
}
#else
namespace {
HttpResponse perform(const HttpRequest&, RequestState&)
{
    HttpResponse response;
    response.error = QStringLiteral("No HTTP backend in this build");
    return response;
}
}

bool httpRequestSupported()
{
    return false;
}
#endif

namespace {
// The requests in flight. Their threads run the code of this module: when
// the application quits they are cancelled and waited for, rather than
// left running (detached) while the interface and its module go away.
class RequestRegistry : public QObject
{
public:
    // In the thread of the application, its child
    static RequestRegistry* instance()
    {
        static QPointer<RequestRegistry> registry;
        QCoreApplication* const app = QCoreApplication::instance();
        if (!registry && app)
        {
            registry = new RequestRegistry(app);
            QObject::connect(app, &QCoreApplication::aboutToQuit,
                             registry.data(), &RequestRegistry::shutdown);
        }
        return registry.data();
    }

    ~RequestRegistry() override { shutdown(); }

    bool isShutDown() const { return m_shutDown; }

    void add(QThread* thread, const std::shared_ptr<RequestState>& state)
    {
        m_requests.insert(thread, state);
    }

    void remove(QThread* thread) { m_requests.remove(thread); }

    void shutdown()
    {
        m_shutDown = true;
        for (const auto& state : std::as_const(m_requests))
            state->cancel();
        for (auto it = m_requests.cbegin(); it != m_requests.cend(); ++it)
        {
            QThread* const thread = it.key();
            // cancelled, a request ends at once; the bound only keeps a
            // stuck system call from blocking the exit
            if (thread->wait(QDeadlineTimer(10000)))
                delete thread; // its deleteLater would come after the event loop
        }
        m_requests.clear();
    }

private:
    explicit RequestRegistry(QObject* parent) : QObject(parent) {}

    QHash<QThread*, std::shared_ptr<RequestState>> m_requests;
    bool m_shutDown = false;
};
}

HttpCancel httpRequestAsync(QObject* context, HttpRequest request,
                            std::function<void(const HttpResponse&)> done)
{
    RequestRegistry* const registry = RequestRegistry::instance();
    // the application quitting: no new request (done is dropped, as when
    // context goes away)
    if (registry && registry->isShutDown())
        return []() {};

    auto response = std::make_shared<HttpResponse>();
    auto state = std::make_shared<RequestState>();
    QThread* thread = QThread::create([request = std::move(request), response, state]() {
        *response = perform(request, *state);
    });
    // The context connection drops done if context goes away first.
    QObject::connect(thread, &QThread::finished, context,
                     [response, done = std::move(done)]() { done(*response); });
    QObject::connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    if (registry)
    {
        registry->add(thread, state);
        // nobody waits for the answer any more: the request stops
        const QMetaObject::Connection cancelWithContext =
            QObject::connect(context, &QObject::destroyed, registry, [state]() { state->cancel(); });
        QObject::connect(thread, &QThread::finished, registry,
                         [registry, thread, cancelWithContext]() {
            registry->remove(thread);
            QObject::disconnect(cancelWithContext);
        });
    }
    thread->start();
    // (the state goes with the request: nothing to stop once it has ended)
    return [weak = std::weak_ptr<RequestState>(state)]() {
        if (const auto alive = weak.lock())
            alive->cancel();
    };
}
