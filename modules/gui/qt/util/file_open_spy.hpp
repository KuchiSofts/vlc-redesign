/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
#ifndef VLC_QT_FILE_OPEN_SPY_HPP
#define VLC_QT_FILE_OPEN_SPY_HPP

#include <QCoreApplication>
#include <QFileOpenEvent>
#include <QMetaObject>
#include <QObject>
#include <QTimer>
#include <QUrl>
#include <QVariantList>

#include <functional>
#include <utility>

/* Files opened from Finder (a double click, Open With, a drop on the Dock
 * icon) come to a macOS app as Apple events, not as arguments: Qt turns each
 * into a QFileOpenEvent of the application. Nothing read them, so no file
 * opened from Finder played (#84).
 *
 * The files are played together: Finder sends one event per file, so the
 * ones of the same turn of the event loop make one list. The events of a
 * launch can come before the playlist exists: `sink` then returns false and
 * the files are kept and offered again a little later (`retryMs`). */
class FileOpenSpy : public QObject
{
public:
    using Sink = std::function<bool(const QVariantList &files)>;

    FileOpenSpy(QCoreApplication *app, Sink sink, int retryMs = 100)
        : QObject(app), m_app(app), m_sink(std::move(sink)), m_retryMs(retryMs)
    {
        app->installEventFilter(this);
    }

    bool eventFilter(QObject *o, QEvent *e) override
    {
        if (o != m_app || e->type() != QEvent::FileOpen)
            return false;
        const auto *open = static_cast<QFileOpenEvent *>(e);
        // a link of a scheme the app opens (vlc://...) or a file
        const QUrl url = open->url().isValid() && !open->url().isEmpty()
                         ? open->url() : QUrl::fromLocalFile(open->file());
        if (!url.isEmpty())
        {
            m_pending.append(url);
            queueFlush(0);
        }
        return true;
    }

private:
    void queueFlush(int delayMs)
    {
        if (m_flushQueued)
            return;
        m_flushQueued = true;
        if (delayMs > 0)
            QTimer::singleShot(delayMs, this, [this]() { flush(); });
        else
            QMetaObject::invokeMethod(this, [this]() { flush(); }, Qt::QueuedConnection);
    }

    void flush()
    {
        m_flushQueued = false;
        if (m_pending.isEmpty())
            return;
        if (!m_sink(m_pending))
        {
            // the interface is still being made
            queueFlush(m_retryMs);
            return;
        }
        m_pending.clear();
    }

    QCoreApplication *m_app;
    Sink m_sink;
    int m_retryMs;
    QVariantList m_pending;
    bool m_flushQueued = false;
};

#endif
