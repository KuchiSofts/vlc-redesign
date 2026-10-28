/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
#include "iptv_playlist.hpp"

#include <QRegularExpression>
#include <QUrl>
#include <QUrlQuery>

namespace iptv {

namespace {

// key="value" pairs of an #EXTINF line, and the title after the comma that
// ends them (a comma inside quotes belongs to a value)
struct ExtInf
{
    QHash<QString, QString> attrs;
    QString title;
};

ExtInf parseExtInf(QStringView line)
{
    ExtInf out;
    // skip "#EXTINF:" and the duration
    qsizetype i = line.indexOf(u':');
    if (i < 0)
        return out;
    ++i;
    const qsizetype n = line.size();
    while (i < n && (line[i].isDigit() || line[i] == u'-' || line[i] == u'.'))
        ++i;
    while (i < n)
    {
        while (i < n && line[i].isSpace())
            ++i;
        if (i >= n)
            break;
        if (line[i] == u',')
        {
            out.title = line.mid(i + 1).trimmed().toString();
            break;
        }
        // a key: up to '=' (or a bare word)
        const qsizetype keyStart = i;
        while (i < n && line[i] != u'=' && line[i] != u',' && !line[i].isSpace())
            ++i;
        const QString key = line.mid(keyStart, i - keyStart).toString().toLower();
        if (i < n && line[i] == u'=')
        {
            ++i;
            QString value;
            if (i < n && (line[i] == u'"' || line[i] == u'\''))
            {
                const QChar quote = line[i++];
                const qsizetype valueStart = i;
                while (i < n && line[i] != quote)
                    ++i;
                value = line.mid(valueStart, i - valueStart).toString();
                if (i < n)
                    ++i;
            }
            else
            {
                const qsizetype valueStart = i;
                while (i < n && line[i] != u',' && !line[i].isSpace())
                    ++i;
                value = line.mid(valueStart, i - valueStart).toString();
            }
            if (!key.isEmpty())
                out.attrs.insert(key, value.trimmed());
        }
    }
    return out;
}

QString resolve(const QString& address, const QString& base)
{
    if (base.isEmpty())
        return address;
    const QUrl url(address);
    if (!url.isRelative())
        return address;
    return QUrl(base).resolved(url).toString();
}

CatchupMode modeOf(const QString& type)
{
    const QString t = type.toLower();
    if (t == "default" || t == "append")
        return CatchupMode::Append;
    if (t == "shift")
        return CatchupMode::Append;
    if (t == "flussonic" || t == "flussonic-hls" || t == "flussonic-ts" || t == "fs")
        return CatchupMode::Flussonic;
    if (t == "xc")
        return CatchupMode::Xtream;
    return CatchupMode::None;
}

// A Flussonic stream: its last path element is "<name>.m3u8" (mono, index,
// video, tracks-v1a1...) or "mpegts"
bool looksFlussonic(const QString& url)
{
    const QString path = QUrl(url).path();
    static const QRegularExpression re(
        QStringLiteral("/(mono|index|video|tracks-[^/]+)\\.m3u8$|/mpegts$"));
    return re.match(path).hasMatch();
}

QString fillTemplate(QString tpl, const QDateTime& start, int duration, const QDateTime& now)
{
    const QDateTime end = start.addSecs(duration);
    const QDateTime utc = start.toUTC();
    const auto num = [](qint64 v) { return QString::number(v); };
    // {duration:60} -> minutes, {offset:1} -> seconds ago
    static const QRegularExpression scaled(QStringLiteral("\\{(duration|offset):(\\d+)\\}"));
    QRegularExpressionMatch m;
    while ((m = scaled.match(tpl)).hasMatch())
    {
        const qint64 divisor = qMax<qint64>(1, m.captured(2).toLongLong());
        const qint64 value = m.captured(1) == "duration"
                                 ? duration
                                 : start.secsTo(now);
        tpl.replace(m.capturedStart(), m.capturedLength(), num(value / divisor));
    }
    const QList<QPair<QString, QString>> keys = {
        { "{utc}", num(start.toSecsSinceEpoch()) },
        { "${start}", num(start.toSecsSinceEpoch()) },
        { "{start}", num(start.toSecsSinceEpoch()) },
        { "{timestamp}", num(start.toSecsSinceEpoch()) },
        { "{utcend}", num(end.toSecsSinceEpoch()) },
        { "${end}", num(end.toSecsSinceEpoch()) },
        { "{end}", num(end.toSecsSinceEpoch()) },
        { "{lutc}", num(now.toSecsSinceEpoch()) },
        { "{now}", num(now.toSecsSinceEpoch()) },
        { "${timestamp}", num(now.toSecsSinceEpoch()) },
        { "{duration}", num(duration) },
        { "${duration}", num(duration) },
        { "{offset}", num(start.secsTo(now)) },
        { "${offset}", num(start.secsTo(now)) },
        { "{Y}", utc.toString("yyyy") }, { "{m}", utc.toString("MM") },
        { "{d}", utc.toString("dd") }, { "{H}", utc.toString("HH") },
        { "{M}", utc.toString("mm") }, { "{S}", utc.toString("ss") },
    };
    for (const auto& [key, value] : keys)
        tpl.replace(key, value);
    return tpl;
}

}

bool isChannelList(const QByteArray& data)
{
    const QByteArray head = data.left(64 * 1024);
    if (head.contains("#EXT-X-TARGETDURATION") || head.contains("#EXT-X-STREAM-INF")
        || head.contains("#EXT-X-MEDIA-SEQUENCE"))
        return false;
    return head.contains("#EXTINF");
}

Playlist parseM3u(const QByteArray& data, const QString& base)
{
    Playlist out;
    QString text = QString::fromUtf8(data);
    if (text.startsWith(QChar(0xFEFF)))
        text.remove(0, 1);
    if (!text.trimmed().startsWith(QLatin1String("#EXTM3U"), Qt::CaseInsensitive)
        && !text.contains(QLatin1String("#EXTINF"), Qt::CaseInsensitive))
    {
        out.error = QStringLiteral("not an M3U playlist");
        return out;
    }

    Channel pending;
    bool havePending = false;
    bool pendingHasTitle = false;
    QString pendingGroup;
    const auto lines = QStringView(text).split(u'\n');
    for (QStringView raw : lines)
    {
        const QStringView line = raw.trimmed();
        if (line.isEmpty())
            continue;
        if (line.startsWith(u"#EXTM3U", Qt::CaseInsensitive))
        {
            const ExtInf header = parseExtInf(QString(QStringLiteral("#EXTM3U:0 ") + line.mid(7).toString()));
            for (const char* key : { "url-tvg", "x-tvg-url", "tvg-url" })
                for (const QString& u : header.attrs.value(key).split(u',', Qt::SkipEmptyParts))
                    if (!out.guideUrls.contains(u.trimmed()))
                        out.guideUrls << u.trimmed();
            continue;
        }
        if (line.startsWith(u"#EXTINF", Qt::CaseInsensitive))
        {
            const ExtInf inf = parseExtInf(line);
            pending = Channel{};
            havePending = true;
            const auto& a = inf.attrs;
            pending.name = inf.title.isEmpty() ? a.value("tvg-name") : inf.title;
            pending.tvgId = a.value("tvg-id");
            pending.tvgName = a.value("tvg-name");
            pending.logo = resolve(a.value("tvg-logo", a.value("logo")), base);
            pending.group = a.value("group-title");
            pendingHasTitle = !pending.group.isEmpty();
            if (!pendingHasTitle)
                pending.group = pendingGroup;
            pending.radio = a.value("radio").compare("true", Qt::CaseInsensitive) == 0;
            pending.catchup = modeOf(a.value("catchup", a.value("catchup-type")));
            pending.catchupSource = a.value("catchup-source");
            if (!pending.catchupSource.isEmpty() && pending.catchup != CatchupMode::Xtream)
                pending.catchup = a.value("catchup").compare("append", Qt::CaseInsensitive) == 0
                                      ? CatchupMode::Append : CatchupMode::Template;
            pending.catchupDays = a.value("catchup-days", a.value("tvg-rec", a.value("timeshift"))).toInt();
            if (pending.catchupDays > 0 && pending.catchup == CatchupMode::None)
                pending.catchup = CatchupMode::Flussonic;   // checked against the address below
            if (!a.value("user-agent").isEmpty())
                pending.options << QStringLiteral(":http-user-agent=") + a.value("user-agent");
            if (!a.value("http-referrer").isEmpty())
                pending.options << QStringLiteral(":http-referrer=") + a.value("http-referrer");
            continue;
        }
        if (line.startsWith(u"#EXTGRP:", Qt::CaseInsensitive))
        {
            pendingGroup = line.mid(8).trimmed().toString();
            // after its #EXTINF it names the entry's group (before it, the
            // group of the entries that follow)
            if (havePending && !pendingHasTitle)
                pending.group = pendingGroup;
            continue;
        }
        if (line.startsWith(u"#EXTVLCOPT:", Qt::CaseInsensitive))
        {
            if (havePending)
                pending.options << QStringLiteral(":") + line.mid(11).trimmed().toString();
            continue;
        }
        if (line.startsWith(u'#'))
            continue;

        // the address of the pending entry (or a bare address)
        Channel channel = havePending ? pending : Channel{};
        channel.url = resolve(line.toString(), base);
        if (channel.name.isEmpty())
            channel.name = QUrl(channel.url).fileName();
        if (channel.catchup == CatchupMode::Flussonic && !looksFlussonic(channel.url))
            channel.catchup = channel.catchupDays > 0 ? CatchupMode::Append : CatchupMode::None;
        if (channel.catchup != CatchupMode::None && channel.catchupDays <= 0)
            channel.catchupDays = 7;
        channel.adult = looksAdult(channel.group, channel.name);
        out.channels.append(std::move(channel));
        havePending = false;
    }
    if (out.channels.isEmpty())
        out.error = QStringLiteral("no channels");
    return out;
}

QString catchupUrl(const Channel& channel, const QDateTime& start, int durationSecs,
                   const QDateTime& now)
{
    if (channel.catchup == CatchupMode::None || durationSecs <= 0)
        return QString();
    if (start.secsTo(now) > qint64(channel.catchupDays) * 86400 || start >= now)
        return QString();
    const qint64 utc = start.toSecsSinceEpoch();
    switch (channel.catchup)
    {
    case CatchupMode::Template:
        return fillTemplate(channel.catchupSource, start, durationSecs, now);
    case CatchupMode::Append:
    {
        if (!channel.catchupSource.isEmpty())
            return channel.url + fillTemplate(channel.catchupSource, start, durationSecs, now);
        QUrl url(channel.url);
        QUrlQuery query(url);
        query.addQueryItem("utc", QString::number(utc));
        query.addQueryItem("lutc", QString::number(now.toSecsSinceEpoch()));
        url.setQuery(query);
        return url.toString();
    }
    case CatchupMode::Flussonic:
    {
        QUrl url(channel.url);
        QString path = url.path();
        const qsizetype slash = path.lastIndexOf(u'/');
        const QString last = path.mid(slash + 1);
        if (last == QLatin1String("mpegts"))
            path = path.left(slash + 1) + QStringLiteral("timeshift_abs-%1.ts").arg(utc);
        else if (last.endsWith(QLatin1String(".m3u8")))
        {
            QString name = last.chopped(5);
            // index.m3u8 of a multi-bitrate stream: its archive is "archive"
            if (name == QLatin1String("index"))
                name = QStringLiteral("archive");
            path = path.left(slash + 1)
                   + QStringLiteral("%1-%2-%3.m3u8").arg(name).arg(utc).arg(durationSecs);
        }
        else
            return QString();
        url.setPath(path);
        return url.toString();
    }
    case CatchupMode::Xtream:
    {
        // .../live/<user>/<pass>/<id>.<ext> -> .../timeshift/<user>/<pass>/<minutes>/<Y-m-d:H-M>/<id>.ts
        QUrl url(channel.url);
        const QStringList parts = url.path().split(u'/', Qt::SkipEmptyParts);
        if (parts.size() < 3)
            return QString();
        const QString id = parts.last().section(u'.', 0, 0);
        const QString user = parts.at(parts.size() - 3);
        const QString pass = parts.at(parts.size() - 2);
        // in the provider's local time: Xtream panels take the server's zone
        const QDateTime local = start.toUTC().addSecs(channel.timeOffsetSecs);
        url.setPath(QStringLiteral("/timeshift/%1/%2/%3/%4/%5.ts")
                        .arg(user, pass)
                        .arg((durationSecs + 59) / 60)
                        .arg(local.toString("yyyy-MM-dd:HH-mm"), id));
        url.setQuery(QString());
        return url.toString();
    }
    case CatchupMode::None:
        break;
    }
    return QString();
}

QString displayGroup(const QString& group)
{
    static const QRegularExpression numbered(QStringLiteral("^\\s*\\d{1,3}\\s*[.)\\-:]\\s*"));
    QString out = group;
    out.remove(numbered);
    return out.trimmed();
}

bool looksAdult(const QString& group, const QString& name)
{
    static const QRegularExpression re(
        QStringLiteral("\\b(xxx|adults?|porn\\w*|erotic\\w*|eroluxe|erox\\w*|hustler|"
                       "playboy|brazzers|private\\s?tv|redlight|dorcel|vivid)\\b"
                       "|(^|[^\\w+])(18\\+|\\+18)([^\\w+]|$)"),
        QRegularExpression::CaseInsensitiveOption);
    return re.match(group).hasMatch() || re.match(name).hasMatch();
}

}
