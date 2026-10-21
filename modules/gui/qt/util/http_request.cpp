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
