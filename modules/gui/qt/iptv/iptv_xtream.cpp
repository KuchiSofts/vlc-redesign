/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
#include "iptv_xtream.hpp"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimeZone>
#include <QUrlQuery>

#include <algorithm>

namespace iptv::xtream {

namespace {

// Xtream panels send numbers as numbers or as strings, and null for none
QString str(const QJsonValue& v)
{
    if (v.isString())
        return v.toString().trimmed();
    if (v.isDouble())
    {
        const double d = v.toDouble();
        return d == qint64(d) ? QString::number(qint64(d)) : QString::number(d);
    }
    if (v.isBool())
        return v.toBool() ? QStringLiteral("1") : QStringLiteral("0");
    return QString();
}

qint64 num(const QJsonValue& v)
{
    return str(v).toLongLong();
}

double real(const QJsonValue& v)
{
    return str(v).toDouble();
}

QJsonArray array(const QByteArray& json)
{
    const QJsonDocument doc = QJsonDocument::fromJson(json);
    if (doc.isArray())
        return doc.array();
    // some panels answer an object keyed by index
    QJsonArray out;
    if (doc.isObject())
        for (const QJsonValue& v : doc.object())
            out.append(v);
    return out;
}

QString categoryName(const QVector<Category>& categories, const QString& id)
{
    for (const Category& c : categories)
        if (c.id == id)
            return c.name;
    return QString();
}

QString streamUrl(const Account& a, const char* kind, const QString& id, const QString& ext)
{
    return QStringLiteral("%1/%2/%3/%4/%5.%6")
        .arg(normalizeServer(a.server), QLatin1String(kind),
             QString::fromUtf8(QUrl::toPercentEncoding(a.username)),
             QString::fromUtf8(QUrl::toPercentEncoding(a.password)), id, ext);
}

}

QString normalizeServer(const QString& server)
{
    QString s = server.trimmed();
    if (!s.contains(QLatin1String("://")))
        s.prepend(QStringLiteral("http://"));
    QUrl url(s);
    QString path = url.path();
    for (const char* tail : { "/player_api.php", "/get.php", "/panel_api.php", "/xmltv.php" })
        if (path.endsWith(QLatin1String(tail)))
            path.chop(int(qstrlen(tail)));
    while (path.endsWith(u'/'))
        path.chop(1);
    url.setPath(path);
    url.setQuery(QString());
    url.setFragment(QString());
    return url.toString();
}

Account accountFromM3uUrl(const QString& address)
{
    const QUrl url(address);
    if (!url.path().endsWith(QLatin1String("/get.php")))
        return {};
    const QUrlQuery q(url);
    Account a;
    a.username = q.queryItemValue("username", QUrl::FullyDecoded);
    a.password = q.queryItemValue("password", QUrl::FullyDecoded);
    if (a.username.isEmpty() || a.password.isEmpty())
        return {};
    a.server = normalizeServer(address);
    return a;
}

QUrl apiUrl(const Account& account, const QString& action,
            const QList<QPair<QString, QString>>& params)
{
    QUrl url(normalizeServer(account.server) + QStringLiteral("/player_api.php"));
    QUrlQuery q;
    q.addQueryItem("username", QString::fromUtf8(QUrl::toPercentEncoding(account.username)));
    q.addQueryItem("password", QString::fromUtf8(QUrl::toPercentEncoding(account.password)));
    if (!action.isEmpty())
        q.addQueryItem("action", action);
    for (const auto& [k, v] : params)
        q.addQueryItem(k, v);
    url.setQuery(q);
    return url;
}

AccountInfo parseAccountInfo(const QByteArray& json)
{
    AccountInfo info;
    const QJsonDocument doc = QJsonDocument::fromJson(json);
    if (!doc.isObject())
    {
        info.error = QStringLiteral("not an Xtream Codes server");
        return info;
    }
    const QJsonObject user = doc.object().value("user_info").toObject();
    const QJsonObject server = doc.object().value("server_info").toObject();
    info.authenticated = num(user.value("auth")) == 1;
    info.status = str(user.value("status"));
    info.expires = num(user.value("exp_date"));
    info.maxConnections = int(num(user.value("max_connections")));
    info.activeConnections = int(num(user.value("active_cons")));
    for (const QJsonValue& f : user.value("allowed_output_formats").toArray())
        info.outputFormats << f.toString();
    // the server's clock: its local time next to the UTC timestamp
    const qint64 nowUtc = num(server.value("timestamp_now"));
    const QDateTime local = QDateTime::fromString(str(server.value("time_now")),
                                                  QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    if (nowUtc > 0 && local.isValid())
    {
        QDateTime asUtc = local;
        asUtc.setTimeZone(QTimeZone::utc());
        const qint64 diff = asUtc.toSecsSinceEpoch() - nowUtc;
        // to the quarter hour: the two clocks are read a moment apart
        info.serverOffsetSecs = int(qRound(diff / 900.0) * 900);
    }
    if (!info.authenticated)
        info.error = info.status.isEmpty() ? QStringLiteral("wrong user name or password")
                                           : info.status;
    return info;
}

QVector<Category> parseCategories(const QByteArray& json)
{
    QVector<Category> out;
    for (const QJsonValue& v : array(json))
    {
        const QJsonObject o = v.toObject();
        Category c{ str(o.value("category_id")), str(o.value("category_name")) };
        if (!c.id.isEmpty())
            out.append(c);
    }
    return out;
}

QVector<Channel> parseLiveStreams(const QByteArray& json, const Account& account,
                                  const QVector<Category>& categories, const AccountInfo& info)
{
    const QString ext = info.outputFormats.isEmpty() || info.outputFormats.contains("m3u8")
                            ? QStringLiteral("m3u8") : QStringLiteral("ts");
    QVector<Channel> out;
    for (const QJsonValue& v : array(json))
    {
        const QJsonObject o = v.toObject();
        const QString id = str(o.value("stream_id"));
        if (id.isEmpty())
            continue;
        Channel c;
        c.name = str(o.value("name"));
        c.logo = str(o.value("stream_icon"));
        c.tvgId = str(o.value("epg_channel_id"));
        c.group = categoryName(categories, str(o.value("category_id")));
        c.url = streamUrl(account, "live", id, ext);
        c.radio = str(o.value("stream_type")) == QLatin1String("radio_streams");
        if (num(o.value("tv_archive")) == 1)
        {
            c.catchup = CatchupMode::Xtream;
            c.catchupDays = qMax<int>(1, int(num(o.value("tv_archive_duration"))));
            c.timeOffsetSecs = info.serverOffsetSecs;
        }
        c.adult = num(o.value("is_adult")) == 1 || looksAdult(c.group, c.name);
        out.append(std::move(c));
    }
    return out;
}

QVector<VodItem> parseVodStreams(const QByteArray& json, const Account& account,
                                 const QVector<Category>& categories)
{
    QVector<VodItem> out;
    for (const QJsonValue& v : array(json))
    {
        const QJsonObject o = v.toObject();
        VodItem item;
        item.id = str(o.value("stream_id"));
        if (item.id.isEmpty())
            continue;
        item.name = str(o.value("name"));
        item.poster = str(o.value("stream_icon"));
        item.category = categoryName(categories, str(o.value("category_id")));
        item.extension = str(o.value("container_extension"));
        if (item.extension.isEmpty())
            item.extension = QStringLiteral("mp4");
        item.url = streamUrl(account, "movie", item.id, item.extension);
        item.rating = real(o.value("rating"));
        item.added = num(o.value("added"));
        item.year = int(num(o.value("year")));
        item.adult = num(o.value("is_adult")) == 1 || looksAdult(item.category, item.name);
        out.append(std::move(item));
    }
    return out;
}

QVector<VodItem> parseSeries(const QByteArray& json, const QVector<Category>& categories)
{
    QVector<VodItem> out;
    for (const QJsonValue& v : array(json))
    {
        const QJsonObject o = v.toObject();
        VodItem item;
        item.id = str(o.value("series_id"));
        if (item.id.isEmpty())
            continue;
        item.name = str(o.value("name"));
        item.poster = str(o.value("cover"));
        item.category = categoryName(categories, str(o.value("category_id")));
        item.rating = real(o.value("rating"));
        item.added = num(o.value("last_modified"));
        item.year = str(o.value("releaseDate")).left(4).toInt();
        item.adult = looksAdult(item.category, item.name);
        out.append(std::move(item));
    }
    return out;
}

SeriesInfo parseSeriesInfo(const QByteArray& json, const Account& account)
{
    SeriesInfo out;
    const QJsonObject root = QJsonDocument::fromJson(json).object();
    const QJsonObject info = root.value("info").toObject();
    out.plot = str(info.value("plot"));
    out.cover = str(info.value("cover"));
    const QJsonValue episodes = root.value("episodes");
    auto addSeason = [&](const QJsonArray& list) {
        for (const QJsonValue& v : list)
        {
            const QJsonObject o = v.toObject();
            const QJsonObject ei = o.value("info").toObject();
            Episode e;
            e.id = str(o.value("id"));
            if (e.id.isEmpty())
                continue;
            e.title = str(o.value("title"));
            e.season = int(num(o.value("season")));
            e.number = int(num(o.value("episode_num")));
            e.plot = str(ei.value("plot"));
            e.image = str(ei.value("movie_image"));
            e.durationSecs = int(num(ei.value("duration_secs")));
            QString ext = str(o.value("container_extension"));
            if (ext.isEmpty())
                ext = QStringLiteral("mp4");
            e.url = streamUrl(account, "series", e.id, ext);
            out.episodes.append(std::move(e));
        }
    };
    if (episodes.isObject())
        for (const QJsonValue& season : episodes.toObject())
            addSeason(season.toArray());
    else if (episodes.isArray())
        for (const QJsonValue& season : episodes.toArray())
            addSeason(season.isArray() ? season.toArray() : QJsonArray{ season });
    std::sort(out.episodes.begin(), out.episodes.end(), [](const Episode& a, const Episode& b) {
        return a.season != b.season ? a.season < b.season : a.number < b.number;
    });
    return out;
}

}
