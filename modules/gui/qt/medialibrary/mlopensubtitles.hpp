/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or later.
 *****************************************************************************/
#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QUrl>

#include "util/http_request.hpp"

class MLOpenSubtitles : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList results READ results NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)

public:
    explicit MLOpenSubtitles(QObject* parent = nullptr) : QObject(parent) {}
    QVariantList results() const { return m_results; }
    QString error() const { return m_error; }
    bool busy() const { return m_busy; }

    Q_INVOKABLE void search(const QString& query, const QString& language, const QString& apiKey);
    Q_INVOKABLE void download(int index, const QString& apiKey,
                              const QString& username, const QString& password);
    Q_INVOKABLE void cancel();

    // The account entered last ({ apiKey, username, password }), kept in
    // memory for the session only: it is never written to disk
    Q_INVOKABLE void rememberAccount(const QString& apiKey, const QString& username,
                                     const QString& password);
    Q_INVOKABLE QVariantMap rememberedAccount() const;

signals:
    void changed();
    void fileReady(const QUrl& url, const QString& filename);

private:
    void begin();
    void fail(const QString& message);
    // Sends request; done runs only if no cancel/new request happened meanwhile.
    void send(const HttpRequest& request, std::function<void(const HttpResponse&)> done);
    HttpRequest request(const QUrl& url, const QString& apiKey) const;
    QVariantList m_results;
    QString m_error;
    bool m_busy = false;
    quint64 m_revision = 0;
    HttpCancel m_cancelRequest;
};
