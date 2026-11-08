/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or later.
 *****************************************************************************/
#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "mlopensubtitles.hpp"
#include "opensubtitles_url.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>

namespace {
constexpr qsizetype maxJsonBytes = 2 * 1024 * 1024;
const QUrl apiUrl(QStringLiteral("https://api.opensubtitles.com/api/v1/"));

QString apiMessage(const QByteArray& body, const QString& fallback)
{
    const auto json = QJsonDocument::fromJson(body).object();
    const QString message = json.value(QStringLiteral("message")).toString();
    return message.isEmpty() ? fallback : message;
}

bool replyOk(const HttpResponse& reply)
{
    return reply.error.isEmpty() && reply.status >= 200 && reply.status < 300
        && reply.body.size() <= maxJsonBytes;
}

// The account of the session, shared by the dialogs (in memory only)
struct Account
{
    QString apiKey;
    QString username;
    QString password;
};

Account& sessionAccount()
{
    static Account account;
    return account;
}
}

HttpRequest MLOpenSubtitles::request(const QUrl& url, const QString& apiKey) const
{
    HttpRequest result;
    result.url = url;
    result.headers = {
        {"Api-Key", apiKey.toUtf8()},
        {"User-Agent", "VLC-Redesign/1.0"},
        {"Accept", "application/json"},
        {"Content-Type", "application/json"},
    };
    result.timeoutMs = 30000;
    // the whole request, however slowly it is answered: the dialog is never
    // left busy
    result.deadlineMs = 45 * 1000;
    result.maxBytes = maxJsonBytes;
    return result;
}

void MLOpenSubtitles::begin()
{
    m_busy = true;
    m_error.clear();
    emit changed();
}

void MLOpenSubtitles::fail(const QString& message)
{
    m_busy = false;
    m_error = message;
    emit changed();
}

void MLOpenSubtitles::send(const HttpRequest& request,
                           std::function<void(const HttpResponse&)> done)
{
    begin();
    const quint64 revision = m_revision;
    httpRequestAsync(this, request, [this, revision, done = std::move(done)](const HttpResponse& reply) {
        if (revision == m_revision)
            done(reply);
    });
}

void MLOpenSubtitles::cancel()
{
    // A request in flight cannot be interrupted; its reply is ignored.
    ++m_revision;
    m_busy = false;
    m_error.clear();
    emit changed();
}

void MLOpenSubtitles::rememberAccount(const QString& apiKey, const QString& username,
                                      const QString& password)
{
    Account& account = sessionAccount();
    account.apiKey = apiKey;
    account.username = username;
    account.password = password;
}

QVariantMap MLOpenSubtitles::rememberedAccount() const
{
    const Account& account = sessionAccount();
    return {
        {QStringLiteral("apiKey"), account.apiKey},
        {QStringLiteral("username"), account.username},
        {QStringLiteral("password"), account.password},
    };
}

void MLOpenSubtitles::search(const QString& query, const QString& language,
                             const QString& apiKey)
{
    cancel();
    m_results.clear();
    if (!httpRequestSupported())
    {
        fail(tr("Online subtitle search is not available in this build."));
        return;
    }
    if (apiKey.trimmed().isEmpty() || query.trimmed().isEmpty())
    {
        fail(tr("Enter an OpenSubtitles API key and a title to search."));
        return;
    }
    QUrl url = apiUrl.resolved(QUrl(QStringLiteral("subtitles")));
    QUrlQuery params;
    params.addQueryItem(QStringLiteral("query"), query.trimmed());
    if (!language.trimmed().isEmpty())
        params.addQueryItem(QStringLiteral("languages"), language.trimmed().toLower());
    url.setQuery(params);
    send(request(url, apiKey.trimmed()), [this](const HttpResponse& reply) {
        if (!replyOk(reply))
        {
            fail(apiMessage(reply.body, tr("OpenSubtitles search failed. Check the API key and network.")));
            return;
        }
        const QJsonDocument doc = QJsonDocument::fromJson(reply.body);
        if (!doc.isObject())
        {
            fail(tr("OpenSubtitles returned an invalid search response."));
            return;
        }
        QVariantList results;
        for (const auto& entry : doc.object().value(QStringLiteral("data")).toArray())
        {
            const auto attributes = entry.toObject().value(QStringLiteral("attributes")).toObject();
            const auto files = attributes.value(QStringLiteral("files")).toArray();
            if (files.isEmpty())
                continue;
            const auto file = files.first().toObject();
            const int fileId = file.value(QStringLiteral("file_id")).toInt();
            if (fileId <= 0)
                continue;
            results.push_back(QVariantMap{
                {"fileId", fileId},
                {"filename", file.value(QStringLiteral("file_name")).toString()},
                {"language", attributes.value(QStringLiteral("language")).toString()},
                {"release", attributes.value(QStringLiteral("release")).toString()}
            });
        }
        m_results = results;
        m_busy = false;
        emit changed();
    });
}

void MLOpenSubtitles::download(int index, const QString& apiKey,
                               const QString& username, const QString& password)
{
    if (m_busy || index < 0 || index >= m_results.size())
        return;
    if (apiKey.trimmed().isEmpty() || username.trimmed().isEmpty() || password.isEmpty())
    {
        fail(tr("Enter your API key and OpenSubtitles account credentials to download."));
        return;
    }
    const auto choice = m_results[index].toMap();
    const int fileId = choice.value(QStringLiteral("fileId")).toInt();
    HttpRequest login = request(apiUrl.resolved(QUrl(QStringLiteral("login"))), apiKey.trimmed());
    login.method = "POST";
    login.body = QJsonDocument(QJsonObject{{"username", username.trimmed()}, {"password", password}})
                     .toJson(QJsonDocument::Compact);
    send(login, [this, fileId, apiKey, choice](const HttpResponse& reply) {
        if (!replyOk(reply))
        {
            fail(apiMessage(reply.body, tr("OpenSubtitles login failed. Check your account credentials.")));
            return;
        }
        const auto loginData = QJsonDocument::fromJson(reply.body).object();
        const QString token = loginData.value(QStringLiteral("token")).toString();
        const QUrl base = openSubtitlesApiBase(loginData.value(QStringLiteral("base_url")).toString());
        if (token.isEmpty() || base.isEmpty())
        {
            fail(tr("OpenSubtitles returned an invalid login session."));
            return;
        }
        HttpRequest download = request(base.resolved(QUrl(QStringLiteral("download"))), apiKey.trimmed());
        download.method = "POST";
        download.headers.append({"Authorization", QByteArray("Bearer ") + token.toUtf8()});
        download.body = QJsonDocument(QJsonObject{{"file_id", fileId}, {"sub_format", "srt"}})
                            .toJson(QJsonDocument::Compact);
        send(download, [this, choice](const HttpResponse& reply) {
            if (!replyOk(reply))
            {
                fail(apiMessage(reply.body, tr("OpenSubtitles download failed. Check your quota.")));
                return;
            }
            const auto payload = QJsonDocument::fromJson(reply.body).object();
            const QUrl link(payload.value(QStringLiteral("link")).toString());
            QString filename = payload.value(QStringLiteral("file_name")).toString();
            if (filename.isEmpty())
                filename = choice.value(QStringLiteral("filename")).toString();
            if (link.scheme() != QStringLiteral("https") || link.host().isEmpty() || filename.isEmpty())
            {
                fail(tr("OpenSubtitles did not return a valid subtitle download link."));
                return;
            }
            m_busy = false;
            emit changed();
            emit fileReady(link, filename);
        });
    });
}
