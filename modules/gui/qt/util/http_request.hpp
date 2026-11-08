/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or later.
 *****************************************************************************/
#pragma once

#include <QByteArray>
#include <QList>
#include <QPair>
#include <QString>
#include <QUrl>
#include <QtGlobal>

#include <functional>
#include <memory>

class QObject;

// Small HTTPS client for the interface. VLC's Qt is built without
// QtNetwork, so requests go through the system stack (WinHTTP on Windows,
// libcurl elsewhere).
struct HttpResponse
{
    int status = 0;    // HTTP status, 0 when no response was received
    QByteArray body;   // at most maxBytes + 1 bytes, to detect oversized replies
    QString error;     // transport error, empty on success
};

struct HttpRequest
{
    QByteArray method = "GET";
    QUrl url;
    QList<QPair<QByteArray, QByteArray>> headers;
    QByteArray body;
    // of each step: connecting, sending, waiting for the answer, each read
    int timeoutMs = 30000;
    // of the whole request, 0 for none: a slow download ends there
    int deadlineMs = 0;
    qsizetype maxBytes = 2 * 1024 * 1024;
    // who the server asks for (Basic or Digest, as it says), when it does
    QString username;
    QString password;
    // Called on the worker thread as the body arrives: the bytes received,
    // and the size the server announced (0 when unknown)
    std::function<void(qint64 received, qint64 total)> progress;
};

// Whether this build can perform requests at all.
bool httpRequestSupported();

// Redirects (301, 302, 303, 307, 308; at most 10) are followed by this code,
// not by the backend, in both (WinHTTP, libcurl), by these rules:
//  - to http or https only, never from https to http (an error then);
//  - to another origin (scheme, host or port), only the headers that are no
//    secret go (User-Agent, Accept, Accept-Language, Referer; Content-Type
//    with a body): an API key, an Authorization, a session's token, and the
//    username and password, stay with the server they were for, for the
//    rest of the chain;
//  - 303 (and 301, 302 of a POST) asks again with GET (HEAD stays), without
//    the body; 307 and 308 keep the method and the body.
// The next hop of request, redirected (status) to target: request changed so;
// false, and why, when the redirect is refused.
bool httpFollowRedirect(HttpRequest& request, int status, const QUrl& target, QString* error);

// Stops a request: done then gets the error "Cancelled" (nothing happens
// once it has ended)
using HttpCancel = std::function<void()>;

// The request, here and now (it blocks: a worker thread's, the Live TV
// module's). Its timeouts bound how long.
HttpResponse httpRequestSync(const HttpRequest& request);

// The same, stoppable from another thread: cancel() (any thread, before,
// during or after) ends it at once with the error "Cancelled". What a
// module that quits stops its requests with (a guide of a server waits
// for minutes otherwise).
class HttpSyncRequest
{
public:
    HttpSyncRequest();
    ~HttpSyncRequest();
    HttpSyncRequest(const HttpSyncRequest&) = delete;
    HttpSyncRequest& operator=(const HttpSyncRequest&) = delete;
    HttpResponse run(const HttpRequest& request);
    void cancel();

private:
    std::shared_ptr<void> m_state;
};

// Runs the request on a worker thread and calls done in the thread of
// context. done is dropped if context is destroyed first.
HttpCancel httpRequestAsync(QObject* context, HttpRequest request,
                            std::function<void(const HttpResponse&)> done);
