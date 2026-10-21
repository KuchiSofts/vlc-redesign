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

#include <functional>

class QObject;

// Small HTTPS client for the interface. VLC's Qt is built without
// QtNetwork, so requests go through the system stack (WinHTTP on Windows).
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
    // Called on the worker thread as the body arrives: the bytes received,
    // and the size the server announced (0 when unknown)
    std::function<void(qint64 received, qint64 total)> progress;
};

// Whether this build can perform requests at all.
bool httpRequestSupported();

// Stops a request: done then gets the error "Cancelled" (nothing happens
// once it has ended)
using HttpCancel = std::function<void()>;

// Runs the request on a worker thread and calls done in the thread of
// context. done is dropped if context is destroyed first.
HttpCancel httpRequestAsync(QObject* context, HttpRequest request,
                            std::function<void(const HttpResponse&)> done);
