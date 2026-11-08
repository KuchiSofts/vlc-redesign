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
    // The event the worker of an asynchronous WinHTTP call waits on: a
    // cancel wakes it, and it closes its request itself (a handle is never
    // touched from another thread)
    HANDLE wake = nullptr;
#endif

    void cancel()
    {
        std::lock_guard<std::mutex> guard(lock);
        cancelled = true;
#ifdef _WIN32
        if (wake)
            SetEvent(wake);
#endif
    }
};

// at most this many redirects in a chain
constexpr int maxRedirects = 10;

bool isRedirect(int status)
{
    return status == 301 || status == 302 || status == 303 || status == 307 || status == 308;
}

// scheme, host and port (the scheme's own when none is said)
bool sameOrigin(const QUrl& a, const QUrl& b)
{
    const auto port = [](const QUrl& url) {
        return url.port(url.scheme().compare(QLatin1String("https"), Qt::CaseInsensitive) == 0 ? 443 : 80);
    };
    return a.scheme().compare(b.scheme(), Qt::CaseInsensitive) == 0
        && a.host().compare(b.host(), Qt::CaseInsensitive) == 0 && port(a) == port(b);
}
}

bool httpFollowRedirect(HttpRequest& request, int status, const QUrl& target, QString* error)
{
    const QString scheme = target.scheme().toLower();
    if (!target.isValid() || target.host().isEmpty()
        || (scheme != QLatin1String("http") && scheme != QLatin1String("https")))
    {
        *error = QStringLiteral("Redirect to an address that isn't http or https refused");
        return false;
    }
    if (request.url.scheme().compare(QLatin1String("https"), Qt::CaseInsensitive) == 0
        && scheme == QLatin1String("http"))
    {
        *error = QStringLiteral("Redirect from HTTPS to HTTP refused");
        return false;
    }
    // 303, and 301 and 302 of a POST (as the browsers and libcurl): asked
    // again with GET, without the body
    const bool toGet = status == 303 ? request.method != "HEAD"
                                     : (status == 301 || status == 302) && request.method == "POST";
    if (toGet)
    {
        request.method = "GET";
        request.body.clear();
    }
    if (!sameOrigin(request.url, target))
    {
        // Another server: only what is no secret goes with the request
        static const char* const kept[] = { "User-Agent", "Accept", "Accept-Language", "Referer" };
        QList<QPair<QByteArray, QByteArray>> headers;
        for (const auto& header : std::as_const(request.headers))
        {
            bool keep = std::any_of(std::begin(kept), std::end(kept), [&header](const char* name) {
                return header.first.compare(name, Qt::CaseInsensitive) == 0;
            });
            keep = keep || (header.first.compare("Content-Type", Qt::CaseInsensitive) == 0 && !request.body.isEmpty());
            if (keep)
                headers.append(header);
        }
        request.headers = headers;
        request.username.clear();
        request.password.clear();
    }
    else if (toGet)
    {
        // (no body, no type of it)
        request.headers.erase(std::remove_if(request.headers.begin(), request.headers.end(), [](const auto& header) {
            return header.first.compare("Content-Type", Qt::CaseInsensitive) == 0;
        }), request.headers.end());
    }
    request.url = target;
    return true;
}

#ifdef _WIN32
namespace {
// WinHTTP in its asynchronous mode. A synchronous call can't be stopped
// safely: closing its handle from another thread while the call runs is a
// race Microsoft warns of ("An application should never call
// WinHttpCloseHandle on a synchronous request"). Asynchronous, each call
// returns at once and its end comes to statusCallback; the worker thread
// waits for that end, or for a cancel, or for the deadline; and only the
// worker ever uses the handles: a cancel just wakes it, it closes the
// request itself (which ends what is pending, as documented for the
// asynchronous mode), then waits for WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING,
// the last callback of the handle, before it frees what the callback uses.

// What the callback reports of the calls of one request, to its worker
struct AsyncCall
{
    // an end of a call (or a cancel): auto-reset
    HANDLE event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    // the request handle is gone (HANDLE_CLOSING): manual-reset
    HANDLE closed = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    std::mutex lock;
    bool done = false;  // the call waited for has ended
    DWORD error = 0;    // and failed so (REQUEST_ERROR)
    DWORD bytes = 0;    // what it read, or has to read (DATA_AVAILABLE)
    // where the reads land: alive until HANDLE_CLOSING, as the call
    char buffer[65536];

    ~AsyncCall()
    {
        if (event)
            CloseHandle(event);
        if (closed)
            CloseHandle(closed);
    }

    void finish(DWORD failure, DWORD count)
    {
        {
            std::lock_guard<std::mutex> guard(lock);
            done = true;
            error = failure;
            bytes = count;
        }
        SetEvent(event);
    }
};

void CALLBACK statusCallback(HINTERNET, DWORD_PTR context, DWORD status, LPVOID info, DWORD length)
{
    // (the session's and the connection's own: no context, nothing to do)
    auto* const call = reinterpret_cast<AsyncCall*>(context);
    if (!call)
        return;
    switch (status)
    {
        case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE:
        case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE:
            call->finish(0, 0);
            break;
        case WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE:
            call->finish(0, info ? *static_cast<const DWORD*>(info) : 0);
            break;
        case WINHTTP_CALLBACK_STATUS_READ_COMPLETE:
            call->finish(0, length);
            break;
        case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR:
        {
            const auto* const result = static_cast<const WINHTTP_ASYNC_RESULT*>(info);
            call->finish(result && result->dwError ? result->dwError : DWORD(ERROR_WINHTTP_INTERNAL_ERROR), 0);
            break;
        }
        case WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING:
        {
            // the last callback of the request: call is the worker's to
            // free once this is set (nothing of it is touched after)
            const HANDLE closed = call->closed;
            SetEvent(closed);
            break;
        }
        default:
            break;
    }
}

QString lastError(const char* step, DWORD error = GetLastError())
{
    return QStringLiteral("%1 failed (%2)").arg(QLatin1String(step)).arg(error);
}

// A handle of the session or of the connection (no context, no callback
// of its own to wait for), closed at the end
struct InternetHandle
{
    HINTERNET handle = nullptr;
    explicit InternetHandle(HINTERNET h) : handle(h) {}
    ~InternetHandle()
    {
        if (handle)
            WinHttpCloseHandle(handle);
    }
    InternetHandle(const InternetHandle&) = delete;
    InternetHandle& operator=(const InternetHandle&) = delete;
    explicit operator bool() const { return handle != nullptr; }
};

// The request handle and its call: closed by the worker (whatever ended
// the request), then freed once WinHTTP says it is gone; registered with the
// state meanwhile, for a cancel to wake the worker
class AsyncRequest
{
public:
    AsyncRequest(RequestState& state, HINTERNET handle) : m_state(state), m_handle(handle) {}

    ~AsyncRequest()
    {
        if (m_registered)
        {
            std::lock_guard<std::mutex> guard(m_state.lock);
            m_state.wake = nullptr;
        }
        if (!m_handle)
        {
            delete m_call;
            return;
        }
        WinHttpCloseHandle(m_handle);
        // with a context the request has its HANDLE_CLOSING to come (what
        // was pending ends first, in error): only then is call free. (A
        // handle that never says so keeps it: a leak, not a crash)
        if (!m_withContext || WaitForSingleObject(m_call->closed, 10000) == WAIT_OBJECT_0)
            delete m_call;
    }

    AsyncRequest(const AsyncRequest&) = delete;
    AsyncRequest& operator=(const AsyncRequest&) = delete;

    HINTERNET handle() const { return m_handle; }
    AsyncCall* call() const { return m_call; }

    // the callbacks of the request given call, and a cancel able to wake its
    // worker: false when cancelled already, or when it can't be
    bool start()
    {
        if (!m_handle || !m_call || !m_call->event || !m_call->closed)
            return false;
        DWORD_PTR context = reinterpret_cast<DWORD_PTR>(m_call);
        if (!WinHttpSetOption(m_handle, WINHTTP_OPTION_CONTEXT_VALUE, &context, sizeof(context)))
            return false;
        m_withContext = true;
        std::lock_guard<std::mutex> guard(m_state.lock);
        if (m_state.cancelled)
            return false;
        m_state.wake = m_call->event;
        m_registered = true;
        return true;
    }

    // How the call just started ended: its end, a cancel, or the deadline.
    // Its error (0: none) and count in error and bytes
    enum Wait { Ended, Cancelled, Expired };
    Wait wait(const QDeadlineTimer& deadline, DWORD* error, DWORD* bytes)
    {
        for (;;)
        {
            if (m_state.cancelled)
                return Cancelled;
            {
                std::lock_guard<std::mutex> guard(m_call->lock);
                if (m_call->done)
                {
                    m_call->done = false;
                    *error = m_call->error;
                    *bytes = m_call->bytes;
                    return Ended;
                }
            }
            if (deadline.hasExpired())
                return Expired;
            const qint64 left = deadline.remainingTime();
            WaitForSingleObject(m_call->event, left < 0 ? INFINITE : DWORD(std::min<qint64>(left + 1, 0x7FFFFFFF)));
        }
    }

private:
    RequestState& m_state;
    HINTERNET m_handle = nullptr;
    AsyncCall* m_call = new AsyncCall;
    bool m_withContext = false;
    bool m_registered = false;
};

// One hop of a request: its redirects are not followed (perform() does, by
// httpFollowRedirect), their address in location
HttpResponse performHop(const HttpRequest& request, RequestState& state, const QDeadlineTimer& deadline,
                        QUrl* location)
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
    if (state.cancelled)
    {
        response.error = cancelled;
        return response;
    }

    // The proxy of the system (its settings, or the script it names), as
    // the browsers use; WinHTTP's own setting before Windows 8.1
    HINTERNET opened = WinHttpOpen(L"VLC", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                                   WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, WINHTTP_FLAG_ASYNC);
    if (!opened)
        opened = WinHttpOpen(L"VLC", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                             WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, WINHTTP_FLAG_ASYNC);
    InternetHandle session(opened);
    if (!session)
    {
        response.error = lastError("WinHttpOpen");
        return response;
    }
    // each step at most this long: a stalled read ends the request
    WinHttpSetTimeouts(session.handle, request.timeoutMs, request.timeoutMs,
                       request.timeoutMs, request.timeoutMs);
    // (set on the session: its connection and request have it too)
    if (WinHttpSetStatusCallback(session.handle, statusCallback,
                                 WINHTTP_CALLBACK_FLAG_ALL_COMPLETIONS | WINHTTP_CALLBACK_FLAG_HANDLES, 0)
        == WINHTTP_INVALID_STATUS_CALLBACK)
    {
        response.error = lastError("WinHttpSetStatusCallback");
        return response;
    }

    const std::wstring host = url.host(QUrl::FullyEncoded).toStdWString();
    const auto port = static_cast<INTERNET_PORT>(url.port(https ? INTERNET_DEFAULT_HTTPS_PORT
                                                                : INTERNET_DEFAULT_HTTP_PORT));
    InternetHandle connection(WinHttpConnect(session.handle, host.c_str(), port, 0));
    if (!connection)
    {
        response.error = lastError("WinHttpConnect");
        return response;
    }

    const std::wstring method = QString::fromLatin1(request.method).toStdWString();
    const std::wstring path = QString::fromLatin1(
        url.toEncoded(QUrl::RemoveScheme | QUrl::RemoveAuthority | QUrl::RemoveFragment)).toStdWString();
    QByteArray headers;
    for (const auto& header : request.headers)
        headers += header.first + ": " + header.second + "\r\n";
    const std::wstring wideHeaders = QString::fromUtf8(headers).toStdWString();
    const auto bodySize = static_cast<DWORD>(request.body.size());
    const std::wstring user = request.username.toStdWString();
    const std::wstring password = request.password.toStdWString();
    {
        // (the strings above outlive the request: what is pending of them
        // ends before ~AsyncRequest returns)
        AsyncRequest call(state, WinHttpOpenRequest(connection.handle, method.c_str(),
                                                    path.empty() ? L"/" : path.c_str(), nullptr,
                                                    WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                                    https ? WINHTTP_FLAG_SECURE : 0));
        if (!call.handle())
        {
            response.error = lastError("WinHttpOpenRequest");
            return response;
        }
        // Redirects are not WinHTTP's to follow: perform() does, without the
        // secret headers to another server (WinHTTP would send them all)
        DWORD redirects = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
        if (!WinHttpSetOption(call.handle(), WINHTTP_OPTION_REDIRECT_POLICY, &redirects, sizeof(redirects)))
        {
            response.error = lastError("WinHttpSetOption");
            return response;
        }
        if (!call.start())
        {
            response.error = state.cancelled ? cancelled : lastError("WinHttpSetOption");
            return response;
        }
        // how a call ended: true to go on, else response.error says why
        DWORD error = 0, count = 0;
        const auto ended = [&](BOOL started, const char* step) {
            if (!started)
            {
                response.error = lastError(step);
                return false;
            }
            switch (call.wait(deadline, &error, &count))
            {
                case AsyncRequest::Cancelled:
                    response.error = cancelled;
                    return false;
                case AsyncRequest::Expired:
                    response.error = QStringLiteral("Timed out");
                    return false;
                case AsyncRequest::Ended:
                    break;
            }
            if (error)
            {
                response.error = state.cancelled ? cancelled : lastError(step, error);
                return false;
            }
            return true;
        };

        DWORD status = 0;
        for (int attempt = 0;; ++attempt)
        {
            const DWORD_PTR context = reinterpret_cast<DWORD_PTR>(call.call());
            if (!ended(WinHttpSendRequest(call.handle(),
                                          wideHeaders.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : wideHeaders.c_str(),
                                          static_cast<DWORD>(-1L),
                                          bodySize ? const_cast<char*>(request.body.constData()) : WINHTTP_NO_REQUEST_DATA,
                                          bodySize, bodySize, context), "HTTP request")
                || !ended(WinHttpReceiveResponse(call.handle(), nullptr), "HTTP request"))
                return response;
            DWORD statusSize = sizeof(status);
            WinHttpQueryHeaders(call.handle(), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize, WINHTTP_NO_HEADER_INDEX);
            // the server asks who: the request's user, once, in the way it
            // says (Digest rather than Basic)
            if (status != 401 || attempt > 0 || user.empty())
                break;
            DWORD supported = 0, first = 0, target = 0;
            if (!WinHttpQueryAuthSchemes(call.handle(), &supported, &first, &target))
                break;
            const DWORD scheme = (supported & WINHTTP_AUTH_SCHEME_DIGEST) ? WINHTTP_AUTH_SCHEME_DIGEST
                               : (supported & WINHTTP_AUTH_SCHEME_BASIC) ? WINHTTP_AUTH_SCHEME_BASIC : 0;
            if (!scheme || !WinHttpSetCredentials(call.handle(), target, scheme, user.c_str(), password.c_str(), nullptr))
                break;
        }
        response.status = static_cast<int>(status);
        if (isRedirect(response.status))
        {
            // where to (perform() decides); its body isn't read
            DWORD size = 0;
            WinHttpQueryHeaders(call.handle(), WINHTTP_QUERY_LOCATION, WINHTTP_HEADER_NAME_BY_INDEX,
                                WINHTTP_NO_OUTPUT_BUFFER, &size, WINHTTP_NO_HEADER_INDEX);
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER && size > 0 && size <= 64 * 1024)
            {
                std::wstring value(size / sizeof(wchar_t) + 1, L'\0');
                if (WinHttpQueryHeaders(call.handle(), WINHTTP_QUERY_LOCATION, WINHTTP_HEADER_NAME_BY_INDEX,
                                        value.data(), &size, WINHTTP_NO_HEADER_INDEX))
                    *location = QUrl(QString::fromWCharArray(value.c_str(), int(size / sizeof(wchar_t))).trimmed());
            }
            return response;
        }

        DWORD length = 0;
        DWORD lengthSize = sizeof(length);
        if (!WinHttpQueryHeaders(call.handle(), WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
                                 WINHTTP_HEADER_NAME_BY_INDEX, &length, &lengthSize, WINHTTP_NO_HEADER_INDEX))
            length = 0;
        if (length > 0 && static_cast<qsizetype>(length) <= request.maxBytes)
            response.body.reserve(length);

        // What has come, a buffer at most at a time (not a full buffer
        // waited for: an answer a byte at a time ends at its deadline)
        while (response.body.size() <= request.maxBytes)
        {
            if (!ended(WinHttpQueryDataAvailable(call.handle(), nullptr), "WinHttpQueryDataAvailable"))
                break;
            const DWORD available = count;
            if (available == 0)
                break;
            if (!ended(WinHttpReadData(call.handle(), call.call()->buffer,
                                       std::min<DWORD>(available, sizeof(call.call()->buffer)), nullptr),
                       "WinHttpReadData"))
                break;
            if (count == 0)
                break;
            response.body.append(call.call()->buffer, count);
            if (request.progress)
                request.progress(response.body.size(), length);
        }
        if (response.body.size() > request.maxBytes)
            response.body.truncate(request.maxBytes + 1);
    }
    // (the request is gone by now: its connection and session next, which
    // have no context, so nothing of this code is called back for them)
    WinHttpSetStatusCallback(session.handle, nullptr, 0, 0);
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

// One hop of a request: its redirects are not followed (perform() does, by
// httpFollowRedirect), their address in location
HttpResponse performHop(const HttpRequest& request, RequestState& state, const QDeadlineTimer& deadline,
                        QUrl* location)
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
    CurlTransfer transfer { request, state, response, deadline };

    const QByteArray encodedUrl = url.toEncoded(QUrl::RemoveFragment);
    curl_easy_setopt(curl, CURLOPT_URL, encodedUrl.constData());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "VLC");
    // the server asks who: Basic or Digest, as it says
    const QByteArray user = request.username.toUtf8();
    const QByteArray password = request.password.toUtf8();
    if (!user.isEmpty())
    {
        curl_easy_setopt(curl, CURLOPT_HTTPAUTH, static_cast<long>(CURLAUTH_ANY));
        curl_easy_setopt(curl, CURLOPT_USERNAME, user.constData());
        curl_easy_setopt(curl, CURLOPT_PASSWORD, password.constData());
    }
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    // Redirects are not libcurl's to follow: perform() does, without the
    // secret headers to another server (libcurl keeps all but
    // Authorization and Cookie)
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
#if LIBCURL_VERSION_NUM >= 0x075500 /* 7.85 */
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "http,https");
#else
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS, static_cast<long>(CURLPROTO_HTTP | CURLPROTO_HTTPS));
#endif
    // each step at most timeoutMs: connecting, then no data for that long
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, static_cast<long>(request.timeoutMs));
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1L);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME,
                     std::max(1L, static_cast<long>(request.timeoutMs / 1000)));

    // (a GET with a body, as some services ask for their searches, stays a GET)
    if (request.method == "GET" && request.body.isEmpty())
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
    if (code == CURLE_OK && isRedirect(response.status))
    {
        // where to (perform() decides); its body is no answer
        char* target = nullptr;
        if (curl_easy_getinfo(curl, CURLINFO_REDIRECT_URL, &target) == CURLE_OK && target)
            *location = QUrl::fromEncoded(QByteArray(target));
        response.body.clear();
    }
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
HttpResponse performHop(const HttpRequest&, RequestState&, const QDeadlineTimer&, QUrl*)
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
// The request, its redirects followed (httpFollowRedirect), all of it
// within its deadline
HttpResponse perform(const HttpRequest& original, RequestState& state)
{
    const QDeadlineTimer deadline = original.deadlineMs > 0 ? QDeadlineTimer(original.deadlineMs)
                                                            : QDeadlineTimer(QDeadlineTimer::Forever);
    HttpRequest request = original;
    for (int hop = 0;; ++hop)
    {
        QUrl location;
        HttpResponse response = performHop(request, state, deadline, &location);
        if (!response.error.isEmpty() || !isRedirect(response.status) || location.isEmpty())
            return response;
        QString error;
        if (hop == maxRedirects)
            error = QStringLiteral("Too many redirects");
        else if (deadline.hasExpired())
            error = QStringLiteral("Timed out");
        else if (state.cancelled)
            error = QStringLiteral("Cancelled");
        else if (httpFollowRedirect(request, response.status, request.url.resolved(location), &error))
            continue;
        response.error = error;
        response.body.clear();
        return response;
    }
}
}

namespace {
// The requests in flight and the background work (runInBackground). Their
// threads run the code of this module: when the application quits (or
// goes: the registry is its child) the requests are cancelled, then all of
// them are waited for together, at most shutdownMs, rather than left
// running (detached) while the interface and its module go away. One still
// running then (a call of the system that doesn't return) is let go: its
// thread is never deleted, nor what it uses.
constexpr int shutdownMs = 5000;

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

    // (state: null for background work, nothing to cancel)
    void add(QThread* thread, const std::shared_ptr<RequestState>& state)
    {
        m_requests.insert(thread, state);
    }

    void remove(QThread* thread) { m_requests.remove(thread); }

    void shutdown()
    {
        m_shutDown = true;
        for (const auto& state : std::as_const(m_requests))
            if (state)
                state->cancel();
        // cancelled, a request ends at once; the bound only keeps a stuck
        // system call (or work) from blocking the exit: one bound for all
        // of them (not so long each, one after the other)
        const QDeadlineTimer deadline(shutdownMs);
        for (auto it = m_requests.cbegin(); it != m_requests.cend(); ++it)
        {
            QThread* const thread = it.key();
            if (thread->wait(deadline))
                delete thread; // its deleteLater would come after the event loop
            // else let go: deleting a running QThread aborts, and its work
            // still uses what its function holds
        }
        m_requests.clear();
    }

    // A thread of this registry: done in the thread of context once it has
    // ended (dropped if context is gone first), then the thread deleted
    void start(QThread* thread, QObject* context, std::function<void()> done,
               const std::shared_ptr<RequestState>& state)
    {
        QObject::connect(thread, &QThread::finished, context, std::move(done));
        QObject::connect(thread, &QThread::finished, thread, &QObject::deleteLater);
        add(thread, state);
        // nobody waits for the answer any more: the request stops
        QMetaObject::Connection cancelWithContext;
        if (state)
            cancelWithContext = QObject::connect(context, &QObject::destroyed, this, [state]() { state->cancel(); });
        QObject::connect(thread, &QThread::finished, this, [this, thread, cancelWithContext]() {
            remove(thread);
            QObject::disconnect(cancelWithContext);
        });
        thread->start();
    }

private:
    explicit RequestRegistry(QObject* parent) : QObject(parent) {}

    QHash<QThread*, std::shared_ptr<RequestState>> m_requests;
    bool m_shutDown = false;
};
}

HttpResponse httpRequestSync(const HttpRequest& request)
{
    RequestState state;
    return perform(request, state);
}

HttpSyncRequest::HttpSyncRequest()
    : m_state(std::make_shared<RequestState>())
{
}

HttpSyncRequest::~HttpSyncRequest() = default;

HttpResponse HttpSyncRequest::run(const HttpRequest& request)
{
    return perform(request, *std::static_pointer_cast<RequestState>(m_state));
}

void HttpSyncRequest::cancel()
{
    std::static_pointer_cast<RequestState>(m_state)->cancel();
}

HttpCancel httpRequestAsync(QObject* context, HttpRequest request,
                            std::function<void(const HttpResponse&)> done)
{
    return httpRequestAsync(context, std::move(request), {}, std::move(done));
}

HttpCancel httpRequestAsync(QObject* context, HttpRequest request,
                            std::function<void(const HttpResponse&)> onWorker,
                            std::function<void(const HttpResponse&)> done)
{
    RequestRegistry* const registry = RequestRegistry::instance();
    // the application quitting: no new request (done is dropped, as when
    // context goes away)
    if (registry && registry->isShutDown())
        return []() {};

    auto response = std::make_shared<HttpResponse>();
    auto state = std::make_shared<RequestState>();
    QThread* thread = QThread::create([request = std::move(request), response, state,
                                       onWorker = std::move(onWorker)]() {
        *response = perform(request, *state);
        if (onWorker)
            onWorker(*response);
    });
    // The context connection drops done if context goes away first.
    auto finished = [response, done = std::move(done)]() { done(*response); };
    if (registry)
        registry->start(thread, context, std::move(finished), state);
    else
    {
        QObject::connect(thread, &QThread::finished, context, std::move(finished));
        QObject::connect(thread, &QThread::finished, thread, &QObject::deleteLater);
        thread->start();
    }
    // (the state goes with the request: nothing to stop once it has ended)
    return [weak = std::weak_ptr<RequestState>(state)]() {
        if (const auto alive = weak.lock())
            alive->cancel();
    };
}

bool runInBackground(QObject* context, std::function<void()> work, std::function<void()> done)
{
    RequestRegistry* const registry = RequestRegistry::instance();
    // the application quitting: no new work (done is dropped)
    if (registry && registry->isShutDown())
        return false;
    QThread* thread = QThread::create(std::move(work));
    if (!done)
        done = []() {};
    if (registry)
        registry->start(thread, context, std::move(done), nullptr);
    else
    {
        QObject::connect(thread, &QThread::finished, context, std::move(done));
        QObject::connect(thread, &QThread::finished, thread, &QObject::deleteLater);
        thread->start();
    }
    return true;
}
