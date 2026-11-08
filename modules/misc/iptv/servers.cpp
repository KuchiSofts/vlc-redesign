/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
#include "servers.hpp"

#include <QCryptographicHash>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimeZone>
#include <QUrlQuery>
#include <QXmlStreamWriter>

#include <algorithm>

namespace iptv {
namespace servers {

namespace {

QString text(const QJsonObject &o, const char *key)
{
    return o.value(QLatin1String(key)).toVariant().toString().trimmed();
}

// (no regular expressions in this plugin: their per-thread JIT stacks of the
// plugin's static Qt crashed VLC when the threads ended; see guide.cpp)
// Whether s is only ASCII letters and digits (and dashes when dashes)
bool isPlainId(const QString &s, bool dashes)
{
    if (s.isEmpty())
        return false;
    for (const QChar c : s)
    {
        const char16_t u = c.unicode();
        if (!((u >= u'0' && u <= u'9') || (u >= u'a' && u <= u'z') || (u >= u'A' && u <= u'Z') || (dashes && u == u'-')))
            return false;
    }
    return true;
}

bool isDigits(const QString &s)
{
    if (s.isEmpty())
        return false;
    for (const QChar c : s)
        if (c.unicode() < u'0' || c.unicode() > u'9')
            return false;
    return true;
}

// "S2E5" -> 2, 5 (false when it is not that)
bool seasonEpisode(const QString &s, int *season, int *episode)
{
    const qsizetype e = s.indexOf(u'E');
    if (!s.startsWith(u'S') || e < 2)
        return false;
    const QString a = s.mid(1, e - 1), b = s.mid(e + 1);
    if (!isDigits(a) || !isDigits(b))
        return false;
    *season = a.toInt();
    *episode = b.toInt();
    return true;
}

QUrl withQuery(const QString &address, const QList<QPair<QString, QString>> &items)
{
    QUrl url(address);
    QUrlQuery query;
    for (const auto &item : items)
        query.addQueryItem(item.first, QString::fromUtf8(QUrl::toPercentEncoding(item.second)));
    url.setQuery(query.query(QUrl::FullyEncoded), QUrl::StrictMode);
    return url;
}

QString attribute(QString value)
{
    // (a quote would end the attribute of the M3U line)
    return value.replace(QLatin1Char('"'), QLatin1Char('\'')).replace(QLatin1Char('\n'), QLatin1Char(' '));
}

QString xmltvTime(qint64 secs)
{
    return QDateTime::fromSecsSinceEpoch(secs, QTimeZone::utc()).toString(QStringLiteral("yyyyMMddHHmmss")) +
           QStringLiteral(" +0000");
}

qint64 isoSecs(const QString &iso)
{
    QDateTime t = QDateTime::fromString(iso, Qt::ISODateWithMs);
    if (!t.isValid())
        t = QDateTime::fromString(iso, Qt::ISODate);
    // ("2026-11-08T10:00:00.0000000Z": seven digits of a second)
    if (!t.isValid() && iso.size() > 23)
        t = QDateTime::fromString(iso.left(23) + QLatin1Char('Z'), Qt::ISODateWithMs);
    return t.isValid() ? t.toSecsSinceEpoch() : 0;
}

QString isoOf(qint64 secs)
{
    return QDateTime::fromSecsSinceEpoch(secs, QTimeZone::utc()).toString(QStringLiteral("yyyy-MM-dd'T'HH:mm:ss'Z'"));
}

quint32 crc32Of(const QByteArray &data)
{
    quint32 crc = 0xFFFFFFFFu;
    for (const char c : data)
    {
        crc ^= quint8(c);
        for (int k = 0; k < 8; ++k)
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}

QString mediaBrowserBase(const QString &base, bool emby)
{
    // (Emby's API under /emby, typed or not)
    if (emby && !base.endsWith(QLatin1String("/emby"), Qt::CaseInsensitive))
        return base + QStringLiteral("/emby");
    return base;
}

} // namespace

QString baseUrl(const QString &address, int defaultPort)
{
    QString a = address.trimmed();
    if (a.isEmpty())
        return {};
    if (!a.contains(QLatin1String("://")))
        a.prepend(QStringLiteral("http://"));
    QUrl url(a);
    if (!url.isValid() || url.host().isEmpty()
        || (url.scheme() != QLatin1String("http") && url.scheme() != QLatin1String("https")))
        return {};
    if (url.port() == -1 && url.scheme() == QLatin1String("http") && defaultPort > 0)
        url.setPort(defaultPort);
    url.setUserInfo(QString());
    url.setQuery(QString());
    url.setFragment(QString());
    QString path = url.path();
    while (path.endsWith(QLatin1Char('/')))
        path.chop(1);
    url.setPath(path);
    return url.toString(QUrl::FullyEncoded);
}

QByteArray m3uOf(const QVector<Channel> &channels, const QString &guideUrl)
{
    QString out = QStringLiteral("#EXTM3U");
    if (!guideUrl.isEmpty())
        out += QStringLiteral(" url-tvg=\"%1\"").arg(attribute(guideUrl));
    out += QLatin1Char('\n');
    for (const Channel &c : channels)
    {
        if (c.url.isEmpty())
            continue;
        out += QStringLiteral("#EXTINF:-1 tvg-id=\"%1\"").arg(attribute(c.id));
        if (!c.number.isEmpty())
            out += QStringLiteral(" tvg-chno=\"%1\"").arg(attribute(c.number));
        out += QStringLiteral(" tvg-name=\"%1\"").arg(attribute(c.name));
        if (!c.logo.isEmpty())
            out += QStringLiteral(" tvg-logo=\"%1\"").arg(attribute(c.logo));
        if (!c.group.isEmpty())
            out += QStringLiteral(" group-title=\"%1\"").arg(attribute(c.group));
        out += QLatin1Char(',') + QString(c.name).replace(QLatin1Char('\n'), QLatin1Char(' ')) + QLatin1Char('\n');
        out += c.url + QLatin1Char('\n');
    }
    return out.toUtf8();
}

QByteArray xmltvOf(const QVector<Channel> &channels, const QVector<Programme> &programmes)
{
    QByteArray out;
    QXmlStreamWriter xml(&out);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement(QStringLiteral("tv"));
    xml.writeAttribute(QStringLiteral("generator-info-name"), QStringLiteral("VLC Redesign"));
    for (const Channel &c : channels)
    {
        xml.writeStartElement(QStringLiteral("channel"));
        xml.writeAttribute(QStringLiteral("id"), c.id);
        xml.writeTextElement(QStringLiteral("display-name"), c.name);
        for (const QString &name : c.altNames)
            if (!name.isEmpty() && name != c.name)
                xml.writeTextElement(QStringLiteral("display-name"), name);
        if (!c.number.isEmpty())
            xml.writeTextElement(QStringLiteral("display-name"), c.number);
        if (!c.logo.isEmpty())
        {
            xml.writeStartElement(QStringLiteral("icon"));
            xml.writeAttribute(QStringLiteral("src"), c.logo);
            xml.writeEndElement();
        }
        xml.writeEndElement();
    }
    for (const Programme &p : programmes)
    {
        if (p.channel.isEmpty() || p.stop <= p.start || p.title.isEmpty())
            continue;
        xml.writeStartElement(QStringLiteral("programme"));
        xml.writeAttribute(QStringLiteral("start"), xmltvTime(p.start));
        xml.writeAttribute(QStringLiteral("stop"), xmltvTime(p.stop));
        xml.writeAttribute(QStringLiteral("channel"), p.channel);
        xml.writeTextElement(QStringLiteral("title"), p.title);
        if (!p.subtitle.isEmpty())
            xml.writeTextElement(QStringLiteral("sub-title"), p.subtitle);
        if (!p.description.isEmpty())
            xml.writeTextElement(QStringLiteral("desc"), p.description);
        if (!p.category.isEmpty())
            xml.writeTextElement(QStringLiteral("category"), p.category);
        if (p.season > 0 || p.episode > 0)
        {
            // xmltv_ns counts from 0
            xml.writeStartElement(QStringLiteral("episode-num"));
            xml.writeAttribute(QStringLiteral("system"), QStringLiteral("xmltv_ns"));
            xml.writeCharacters(QStringLiteral("%1.%2.")
                                    .arg(p.season > 0 ? QString::number(p.season - 1) : QString())
                                    .arg(p.episode > 0 ? QString::number(p.episode - 1) : QString()));
            xml.writeEndElement();
        }
        xml.writeEndElement();
    }
    xml.writeEndElement();
    xml.writeEndDocument();
    return out;
}

// --- HDHomeRun

namespace {
Tuner tunerOf(const QJsonObject &o)
{
    Tuner t;
    t.id = text(o, "DeviceID");
    t.name = text(o, "FriendlyName");
    t.baseUrl = baseUrl(text(o, "BaseURL"), 0);
    if (t.baseUrl.isEmpty() && !text(o, "LocalIP").isEmpty())
        t.baseUrl = baseUrl(text(o, "LocalIP"), 0);
    t.lineupUrl = text(o, "LineupURL");
    if (t.lineupUrl.isEmpty() && !t.baseUrl.isEmpty())
        t.lineupUrl = t.baseUrl + QStringLiteral("/lineup.json");
    t.deviceAuth = text(o, "DeviceAuth");
    if (t.name.isEmpty())
        t.name = text(o, "ModelNumber").isEmpty() ? QStringLiteral("HDHomeRun") : text(o, "ModelNumber");
    return t;
}
}

Tuner parseHdhrDiscover(const QByteArray &json)
{
    return tunerOf(QJsonDocument::fromJson(json).object());
}

QVector<Tuner> parseHdhrCloudDiscover(const QByteArray &json)
{
    QVector<Tuner> tuners;
    for (const QJsonValue v : QJsonDocument::fromJson(json).array())
    {
        const QJsonObject o = v.toObject();
        // (its storage devices (DVR) have no lineup: not tuners)
        if (text(o, "DeviceID").isEmpty())
            continue;
        const Tuner t = tunerOf(o);
        if (!t.baseUrl.isEmpty())
            tuners.append(t);
    }
    return tuners;
}

QVector<Channel> parseHdhrLineup(const QByteArray &json)
{
    QVector<Channel> channels;
    for (const QJsonValue v : QJsonDocument::fromJson(json).array())
    {
        const QJsonObject o = v.toObject();
        const QUrl url(text(o, "URL"));
        if (text(o, "GuideNumber").isEmpty() || url.scheme() != QLatin1String("http")
            || o.value(QLatin1String("DRM")).toVariant().toInt() == 1)
            continue;
        Channel c;
        c.id = text(o, "GuideNumber");
        c.number = c.id;
        c.name = text(o, "GuideName").isEmpty() ? c.id : text(o, "GuideName");
        c.group = o.value(QLatin1String("HD")).toVariant().toInt() == 1 ? QStringLiteral("HD") : QString();
        c.url = url.toString(QUrl::FullyEncoded);
        channels.append(c);
    }
    return channels;
}

QUrl hdhrGuideUrl(const QString &deviceAuth)
{
    return withQuery(QStringLiteral("https://api.hdhomerun.com/api/guide"), { { QStringLiteral("DeviceAuth"), deviceAuth } });
}

QVector<Programme> parseHdhrGuide(const QByteArray &json)
{
    QVector<Programme> programmes;
    for (const QJsonValue v : QJsonDocument::fromJson(json).array())
    {
        const QJsonObject channel = v.toObject();
        const QString number = text(channel, "GuideNumber");
        if (number.isEmpty())
            continue;
        for (const QJsonValue g : channel.value(QLatin1String("Guide")).toArray())
        {
            const QJsonObject o = g.toObject();
            Programme p;
            p.channel = number;
            p.start = o.value(QLatin1String("StartTime")).toVariant().toLongLong();
            p.stop = o.value(QLatin1String("EndTime")).toVariant().toLongLong();
            p.title = text(o, "Title");
            p.subtitle = text(o, "EpisodeTitle");
            p.description = text(o, "Synopsis");
            p.category = o.value(QLatin1String("Filter")).toArray().at(0).toString();
            seasonEpisode(text(o, "EpisodeNumber"), &p.season, &p.episode);
            if (p.stop > p.start && !p.title.isEmpty())
                programmes.append(p);
        }
    }
    return programmes;
}

QByteArray hdhrDiscoverPacket()
{
    // libhdhomerun: type (discover request), length, the tags (any device
    // type, any device id), then the CRC-32 of all that, little-endian
    QByteArray payload;
    payload.append(char(0x01)).append(char(0x04)).append(QByteArray::fromHex("ffffffff"));
    payload.append(char(0x02)).append(char(0x04)).append(QByteArray::fromHex("ffffffff"));
    QByteArray packet;
    packet.append(char(0x00)).append(char(0x02));
    packet.append(char((payload.size() >> 8) & 0xFF)).append(char(payload.size() & 0xFF));
    packet.append(payload);
    const quint32 crc = crc32Of(packet);
    for (int i = 0; i < 4; ++i)
        packet.append(char((crc >> (8 * i)) & 0xFF));
    return packet;
}

bool isHdhrDiscoverReply(const QByteArray &packet)
{
    if (packet.size() < 8 || quint8(packet[0]) != 0x00 || quint8(packet[1]) != 0x03)
        return false;
    const int length = (quint8(packet[2]) << 8) | quint8(packet[3]);
    if (4 + length + 4 != packet.size())
        return false;
    const quint32 crc = crc32Of(packet.left(4 + length));
    for (int i = 0; i < 4; ++i)
        if (quint8(packet[4 + length + i]) != ((crc >> (8 * i)) & 0xFF))
            return false;
    return true;
}

// --- Jellyfin and Emby

QByteArray mediaBrowserAuthorization(const QString &deviceId, const QString &version, const QString &token)
{
    QString value = QStringLiteral("MediaBrowser Client=\"VLC Redesign\", Device=\"VLC Redesign\", DeviceId=\"%1\", "
                                   "Version=\"%2\"").arg(deviceId, version);
    if (!token.isEmpty())
        value += QStringLiteral(", Token=\"%1\"").arg(token);
    return value.toUtf8();
}

QByteArray mediaBrowserLoginBody(const QString &username, const QString &password)
{
    return QJsonDocument(QJsonObject{ { QStringLiteral("Username"), username }, { QStringLiteral("Pw"), password } })
        .toJson(QJsonDocument::Compact);
}

Session parseMediaBrowserLogin(const QByteArray &json)
{
    const QJsonObject o = QJsonDocument::fromJson(json).object();
    Session s;
    s.token = text(o, "AccessToken");
    const QJsonObject user = o.value(QLatin1String("User")).toObject();
    s.userId = text(user, "Id");
    s.userName = text(user, "Name");
    if (s.token.isEmpty() || s.userId.isEmpty())
        return {};
    return s;
}

QUrl mediaBrowserChannelsUrl(const QString &base, const QString &userId, bool emby)
{
    return withQuery(mediaBrowserBase(base, emby) + QStringLiteral("/LiveTv/Channels"),
                     { { QStringLiteral("UserId"), userId }, { QStringLiteral("EnableImages"), QStringLiteral("true") },
                       { QStringLiteral("EnableImageTypes"), QStringLiteral("Primary") },
                       { QStringLiteral("ImageTypeLimit"), QStringLiteral("1") },
                       { QStringLiteral("EnableUserData"), QStringLiteral("false") } });
}

QVector<Channel> parseMediaBrowserChannels(const QByteArray &json, const QString &base, bool emby)
{
    QVector<Channel> channels;
    const QString api = mediaBrowserBase(base, emby);
    for (const QJsonValue v : QJsonDocument::fromJson(json).object().value(QLatin1String("Items")).toArray())
    {
        const QJsonObject o = v.toObject();
        Channel c;
        c.id = text(o, "Id");
        if (!isPlainId(c.id, true))
            continue;
        c.name = text(o, "Name");
        c.number = text(o, "ChannelNumber").isEmpty() ? text(o, "Number") : text(o, "ChannelNumber");
        const QString tag = text(o.value(QLatin1String("ImageTags")).toObject(), "Primary");
        if (!tag.isEmpty())
            c.logo = withQuery(api + QStringLiteral("/Items/%1/Images/Primary").arg(c.id), { { QStringLiteral("tag"), tag } })
                         .toString(QUrl::FullyEncoded);
        c.group = text(o, "ChannelType") == QLatin1String("Radio") ? QStringLiteral("Radio") : QString();
        // (its stream, opened when it plays: mediaBrowserStreamUrl)
        c.url = api + QStringLiteral("/Videos/%1/stream.ts").arg(c.id);
        if (!c.name.isEmpty())
            channels.append(c);
    }
    return channels;
}

QUrl mediaBrowserProgrammesUrl(const QString &base, const QString &userId, qint64 from, qint64 to, bool emby)
{
    return withQuery(mediaBrowserBase(base, emby) + QStringLiteral("/LiveTv/Programs"),
                     { { QStringLiteral("UserId"), userId }, { QStringLiteral("MinEndDate"), isoOf(from) },
                       { QStringLiteral("MaxStartDate"), isoOf(to) }, { QStringLiteral("Fields"), QStringLiteral("Overview,Genres") },
                       { QStringLiteral("EnableImages"), QStringLiteral("false") },
                       { QStringLiteral("EnableTotalRecordCount"), QStringLiteral("false") },
                       { QStringLiteral("SortBy"), QStringLiteral("StartDate") } });
}

QVector<Programme> parseMediaBrowserProgrammes(const QByteArray &json)
{
    QVector<Programme> programmes;
    for (const QJsonValue v : QJsonDocument::fromJson(json).object().value(QLatin1String("Items")).toArray())
    {
        const QJsonObject o = v.toObject();
        Programme p;
        p.channel = text(o, "ChannelId");
        p.start = isoSecs(text(o, "StartDate"));
        p.stop = isoSecs(text(o, "EndDate"));
        p.title = text(o, "Name");
        p.subtitle = text(o, "EpisodeTitle");
        p.description = text(o, "Overview");
        p.category = o.value(QLatin1String("Genres")).toArray().at(0).toString();
        if (o.contains(QLatin1String("ParentIndexNumber")))
            p.season = o.value(QLatin1String("ParentIndexNumber")).toInt(-1);
        if (o.contains(QLatin1String("IndexNumber")))
            p.episode = o.value(QLatin1String("IndexNumber")).toInt(-1);
        if (!p.channel.isEmpty() && p.stop > p.start && !p.title.isEmpty())
            programmes.append(p);
    }
    return programmes;
}

QUrl mediaBrowserPlaybackInfoUrl(const QString &base, const QString &channelId, const QString &userId, bool emby)
{
    return withQuery(mediaBrowserBase(base, emby) + QStringLiteral("/Items/%1/PlaybackInfo").arg(channelId),
                     { { QStringLiteral("UserId"), userId }, { QStringLiteral("StartTimeTicks"), QStringLiteral("0") },
                       { QStringLiteral("IsPlayback"), QStringLiteral("true") },
                       { QStringLiteral("AutoOpenLiveStream"), QStringLiteral("true") },
                       { QStringLiteral("MaxStreamingBitrate"), QStringLiteral("140000000") } });
}

QByteArray mediaBrowserPlaybackInfoBody()
{
    // what VLC plays as it is: the server streams the tuner's own TS (or
    // HLS) rather than transcoding it
    const QJsonObject profile{
        { QStringLiteral("MaxStreamingBitrate"), 140000000 },
        { QStringLiteral("DirectPlayProfiles"), QJsonArray{ QJsonObject{
              { QStringLiteral("Container"), QStringLiteral("ts,mpegts,mp4,mkv,m4v,mov,webm,avi,hls") },
              { QStringLiteral("Type"), QStringLiteral("Video") } } } },
        { QStringLiteral("TranscodingProfiles"), QJsonArray{ QJsonObject{
              { QStringLiteral("Container"), QStringLiteral("ts") }, { QStringLiteral("Type"), QStringLiteral("Video") },
              { QStringLiteral("VideoCodec"), QStringLiteral("h264") }, { QStringLiteral("AudioCodec"), QStringLiteral("aac") },
              { QStringLiteral("Protocol"), QStringLiteral("hls") } } } },
    };
    return QJsonDocument(QJsonObject{ { QStringLiteral("DeviceProfile"), profile } }).toJson(QJsonDocument::Compact);
}

QString mediaBrowserStreamUrl(const QByteArray &playbackInfo, const QString &base, const QString &channelId,
                              const QString &token, bool emby)
{
    const QJsonObject info = QJsonDocument::fromJson(playbackInfo).object();
    const QJsonObject source = info.value(QLatin1String("MediaSources")).toArray().at(0).toObject();
    if (source.isEmpty())
        return {};
    const QString api = mediaBrowserBase(base, emby);
    const QString liveStreamId = text(source, "LiveStreamId");
    const QString session = text(info, "PlaySessionId");
    const QString tokenName = emby ? QStringLiteral("api_key") : QStringLiteral("ApiKey");
    if (source.value(QLatin1String("SupportsDirectStream")).toBool())
    {
        QList<QPair<QString, QString>> items = { { QStringLiteral("Static"), QStringLiteral("true") },
                                                 { QStringLiteral("MediaSourceId"), text(source, "Id") } };
        if (!liveStreamId.isEmpty())
            items.append({ QStringLiteral("LiveStreamId"), liveStreamId });
        if (!session.isEmpty())
            items.append({ QStringLiteral("PlaySessionId"), session });
        items.append({ tokenName, token });
        return withQuery(api + QStringLiteral("/Videos/%1/stream.ts").arg(channelId), items).toString(QUrl::FullyEncoded);
    }
    const QUrl path(text(source, "Path"));
    if (text(source, "Container") == QLatin1String("hls")
        && (path.scheme() == QLatin1String("http") || path.scheme() == QLatin1String("https")))
        return path.toString(QUrl::FullyEncoded);
    const QString transcoding = text(source, "TranscodingUrl");
    if (transcoding.isEmpty() || !transcoding.startsWith(QLatin1Char('/')))
        return {};
    // (its path from the server's root: under /emby for Emby)
    QUrl url(QUrl(api).resolved(QUrl(emby && !transcoding.startsWith(QLatin1String("/emby"), Qt::CaseInsensitive)
                                         ? QStringLiteral("/emby") + transcoding : transcoding)));
    QUrlQuery query(url);
    if (!query.hasQueryItem(QStringLiteral("LiveStreamId")) && !liveStreamId.isEmpty())
        query.addQueryItem(QStringLiteral("LiveStreamId"), liveStreamId);
    if (!query.hasQueryItem(tokenName) && !query.hasQueryItem(QStringLiteral("api_key")))
        query.addQueryItem(tokenName, token);
    url.setQuery(query);
    return url.toString(QUrl::FullyEncoded);
}

QByteArray mediaBrowserDiscoverPacket(bool emby)
{
    return emby ? QByteArray("who is EmbyServer?") : QByteArray("who is JellyfinServer?");
}

Found parseMediaBrowserDiscoverReply(const QByteArray &packet, bool emby)
{
    const QJsonObject o = QJsonDocument::fromJson(packet).object();
    Found f;
    f.address = baseUrl(text(o, "Address"), 0);
    f.name = text(o, "Name");
    f.id = text(o, "Id");
    if (f.address.isEmpty() || f.id.isEmpty())
        return {};
    f.kind = emby ? QStringLiteral("emby") : QStringLiteral("jellyfin");
    if (f.name.isEmpty())
        f.name = emby ? QStringLiteral("Emby") : QStringLiteral("Jellyfin");
    return f;
}

// --- Tvheadend

QUrl tvheadendPlaylistUrl(const QString &base)
{
    return QUrl(base + QStringLiteral("/playlist/channels.m3u"));
}

QUrl tvheadendGuideUrl(const QString &base)
{
    return QUrl(base + QStringLiteral("/xmltv/channels"));
}

QByteArray tvheadendWithUser(const QByteArray &m3u, const QString &username, const QString &password)
{
    if (username.isEmpty())
        return m3u;
    QList<QByteArray> lines = m3u.split('\n');
    for (QByteArray &line : lines)
    {
        const QByteArray trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith('#'))
            continue;
        QUrl url(QString::fromUtf8(trimmed));
        if (url.scheme() != QLatin1String("http") && url.scheme() != QLatin1String("https"))
            continue;
        url.setUserName(username);
        url.setPassword(password);
        line = url.toString(QUrl::FullyEncoded).toUtf8();
    }
    return lines.join('\n');
}

// --- Channels DVR

QUrl channelsPlaylistUrl(const QString &base)
{
    return QUrl(base + QStringLiteral("/devices/ANY/channels.m3u"));
}

QUrl channelsGuideUrl(const QString &base)
{
    return QUrl(base + QStringLiteral("/devices/ANY/guide/xmltv"));
}

// --- NextPVR

QUrl nextPvrInitiateUrl(const QString &base)
{
    return withQuery(base + QStringLiteral("/service"),
                     { { QStringLiteral("method"), QStringLiteral("session.initiate") }, { QStringLiteral("ver"), QStringLiteral("1.0") },
                       { QStringLiteral("device"), QStringLiteral("vlc-redesign") }, { QStringLiteral("format"), QStringLiteral("json") } });
}

Salted parseNextPvrInitiate(const QByteArray &json)
{
    const QJsonObject o = QJsonDocument::fromJson(json).object();
    return { text(o, "sid"), text(o, "salt") };
}

QString nextPvrLoginKey(const QString &pin, const QString &salt)
{
    const QByteArray pinHash = QCryptographicHash::hash(pin.toUtf8(), QCryptographicHash::Md5).toHex();
    return QString::fromLatin1(QCryptographicHash::hash(":" + pinHash + ":" + salt.toUtf8(), QCryptographicHash::Md5).toHex());
}

QUrl nextPvrLoginUrl(const QString &base, const QString &sid, const QString &key)
{
    return withQuery(base + QStringLiteral("/service"),
                     { { QStringLiteral("method"), QStringLiteral("session.login") }, { QStringLiteral("sid"), sid },
                       { QStringLiteral("md5"), key }, { QStringLiteral("format"), QStringLiteral("json") } });
}

bool parseNextPvrLogin(const QByteArray &json)
{
    const QJsonObject o = QJsonDocument::fromJson(json).object();
    return text(o, "stat") == QLatin1String("ok");
}

QUrl nextPvrChannelsUrl(const QString &base, const QString &sid)
{
    return withQuery(base + QStringLiteral("/service"),
                     { { QStringLiteral("method"), QStringLiteral("channel.list") }, { QStringLiteral("sid"), sid },
                       { QStringLiteral("format"), QStringLiteral("json") } });
}

QVector<Channel> parseNextPvrChannels(const QByteArray &json, const QString &base, const QString &sid)
{
    QVector<Channel> channels;
    for (const QJsonValue v : QJsonDocument::fromJson(json).object().value(QLatin1String("channels")).toArray())
    {
        const QJsonObject o = v.toObject();
        Channel c;
        c.id = text(o, "channelId");
        if (!isDigits(c.id))
            continue;
        c.name = text(o, "channelName");
        c.number = text(o, "channelNumberFormated").isEmpty() ? text(o, "channelNumber") : text(o, "channelNumberFormated");
        if (o.value(QLatin1String("channelIcon")).toBool())
            c.logo = withQuery(base + QStringLiteral("/service"),
                               { { QStringLiteral("method"), QStringLiteral("channel.icon") }, { QStringLiteral("channel_id"), c.id } })
                         .toString(QUrl::FullyEncoded);
        c.group = text(o, "channelType").compare(QLatin1String("radio"), Qt::CaseInsensitive) == 0 ? QStringLiteral("Radio")
                                                                                                     : QString();
        c.url = withQuery(base + QStringLiteral("/live"), { { QStringLiteral("channeloid"), c.id },
                                                            { QStringLiteral("client"), QStringLiteral("vlc-redesign") },
                                                            { QStringLiteral("sid"), sid } })
                    .toString(QUrl::FullyEncoded);
        if (!c.name.isEmpty())
            channels.append(c);
    }
    return channels;
}

QUrl nextPvrListingsUrl(const QString &base, const QString &sid, const QString &channelId, qint64 from, qint64 to)
{
    return withQuery(base + QStringLiteral("/service"),
                     { { QStringLiteral("method"), QStringLiteral("channel.listings") }, { QStringLiteral("sid"), sid },
                       { QStringLiteral("start"), QString::number(from) }, { QStringLiteral("end"), QString::number(to) },
                       { QStringLiteral("channel_id"), channelId }, { QStringLiteral("format"), QStringLiteral("json") } });
}

QVector<Programme> parseNextPvrListings(const QByteArray &json, const QString &channelId)
{
    QVector<Programme> programmes;
    for (const QJsonValue v : QJsonDocument::fromJson(json).object().value(QLatin1String("listings")).toArray())
    {
        const QJsonObject o = v.toObject();
        Programme p;
        p.channel = channelId;
        p.start = o.value(QLatin1String("start")).toVariant().toLongLong();
        p.stop = o.value(QLatin1String("end")).toVariant().toLongLong();
        // (milliseconds in some versions)
        if (p.start > 100000000000LL)
        {
            p.start /= 1000;
            p.stop /= 1000;
        }
        p.title = text(o, "name");
        p.subtitle = text(o, "subtitle");
        p.description = text(o, "description");
        p.category = o.value(QLatin1String("genres")).toArray().at(0).toString();
        p.season = o.value(QLatin1String("season")).toInt(-1);
        p.episode = o.value(QLatin1String("episode")).toInt(-1);
        // (the season and episode repeated as the subtitle: none of its own)
        if (p.season > 0 && p.episode > 0
            && p.subtitle == QStringLiteral("S%1E%2").arg(p.season, 2, 10, QLatin1Char('0')).arg(p.episode, 2, 10, QLatin1Char('0')))
            p.subtitle.clear();
        if (p.stop > p.start && !p.title.isEmpty())
            programmes.append(p);
    }
    return programmes;
}

// ---------------------------------------------------------------------------
// Multicast DNS

namespace {

void appendName(QByteArray &packet, const QString &name)
{
    for (const QString &label : name.split(u'.', Qt::SkipEmptyParts))
    {
        const QByteArray bytes = label.toUtf8().left(63);
        packet.append(char(bytes.size())).append(bytes);
    }
    packet.append(char(0));
}

quint16 be16(const QByteArray &p, qsizetype at)
{
    return quint16((quint8(p[at]) << 8) | quint8(p[at + 1]));
}

// A name at offset (pointers followed, at most 32), where the record goes
// on in next; false when the packet is broken
bool readName(const QByteArray &p, qsizetype offset, QString *name, qsizetype *next)
{
    QStringList labels;
    qsizetype at = offset;
    qsizetype after = -1;
    for (int jumps = 0; jumps < 32;)
    {
        if (at >= p.size())
            return false;
        const quint8 length = quint8(p[at]);
        if (length == 0)
        {
            if (after < 0)
                after = at + 1;
            if (name)
                *name = labels.join(u'.');
            if (next)
                *next = after;
            return true;
        }
        if ((length & 0xc0) == 0xc0)
        {
            if (at + 1 >= p.size())
                return false;
            if (after < 0)
                after = at + 2;
            at = qsizetype(((length & 0x3f) << 8) | quint8(p[at + 1]));
            ++jumps;
            continue;
        }
        if (at + 1 + length > p.size())
            return false;
        labels << QString::fromUtf8(p.mid(at + 1, length));
        at += 1 + length;
    }
    return false;
}

}

QByteArray mdnsQuery(const QString &service)
{
    // the header: no id, a question; the question: PTR, class IN with the
    // "answer me directly" bit
    QByteArray packet = QByteArray::fromHex("000000000001000000000000");
    appendName(packet, service);
    packet.append(QByteArray::fromHex("000c8001"));
    return packet;
}

QVector<MdnsService> parseMdnsAnswer(const QByteArray &p, const QString &service)
{
    QVector<MdnsService> found;
    if (p.size() < 12)
        return found;
    const int questions = be16(p, 4);
    const int records = be16(p, 6) + be16(p, 8) + be16(p, 10);
    qsizetype at = 12;
    for (int i = 0; i < questions; ++i)
    {
        if (!readName(p, at, nullptr, &at) || at + 4 > p.size())
            return found;
        at += 4;
    }
    QStringList instances;
    QHash<QString, QPair<QString, int>> targets;   // instance -> host, port
    QHash<QString, QString> addresses;             // host -> IPv4
    for (int i = 0; i < records; ++i)
    {
        QString name;
        if (!readName(p, at, &name, &at) || at + 10 > p.size())
            break;
        const quint16 type = be16(p, at);
        const quint16 length = be16(p, at + 8);
        const qsizetype data = at + 10;
        if (data + length > p.size())
            break;
        if (type == 12 && name.compare(service, Qt::CaseInsensitive) == 0)
        {
            QString instance;
            if (readName(p, data, &instance, nullptr) && !instances.contains(instance))
                instances << instance;
        }
        else if (type == 33 && length > 6)
        {
            QString host;
            if (readName(p, data + 6, &host, nullptr))
                targets.insert(name.toLower(), { host, be16(p, data + 4) });
        }
        else if (type == 1 && length == 4)
            addresses.insert(name.toLower(), QStringLiteral("%1.%2.%3.%4").arg(quint8(p[data])).arg(quint8(p[data + 1]))
                                                 .arg(quint8(p[data + 2])).arg(quint8(p[data + 3])));
        at = data + length;
    }
    for (const QString &instance : std::as_const(instances))
    {
        MdnsService s;
        s.name = instance.section(u'.', 0, 0);
        const auto target = targets.value(instance.toLower());
        s.host = target.first;
        s.port = target.second;
        s.address = addresses.value(s.host.toLower());
        found.append(s);
    }
    return found;
}

// ---------------------------------------------------------------------------
// Schedules Direct

QString sdBase()
{
    return QStringLiteral("https://json.schedulesdirect.org/20141201");
}

QString sdPasswordHash(const QString &password)
{
    // (already one: 40 hexadecimal digits)
    if (password.size() == 40
        && std::all_of(password.cbegin(), password.cend(),
                       [](QChar ch) { return ch.isDigit() || (ch.unicode() >= u'a' && ch.unicode() <= u'f'); }))
        return password;
    return QString::fromLatin1(QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha1).toHex());
}

QByteArray sdTokenBody(const QString &username, const QString &passwordHash)
{
    return QJsonDocument(QJsonObject { { QStringLiteral("username"), username },
                                       { QStringLiteral("password"), passwordHash } })
        .toJson(QJsonDocument::Compact);
}

SdAnswer parseSdToken(const QByteArray &json)
{
    const QJsonObject o = QJsonDocument::fromJson(json).object();
    SdAnswer answer;
    answer.code = o.contains(QLatin1String("code")) ? o.value(QLatin1String("code")).toInt() : -1;
    answer.message = text(o, "message");
    if (answer.code == 0)
        answer.token = text(o, "token");
    return answer;
}

QStringList parseSdLineups(const QByteArray &json, int *code)
{
    const QJsonObject o = QJsonDocument::fromJson(json).object();
    if (code)
        *code = o.contains(QLatin1String("code")) ? o.value(QLatin1String("code")).toInt() : -1;
    QStringList uris;
    for (const QJsonValue v : o.value(QLatin1String("lineups")).toArray())
    {
        const QJsonObject l = v.toObject();
        const QString uri = text(l, "uri");
        // (its own path only: what is fetched stays on its server)
        if (!l.value(QLatin1String("isDeleted")).toBool() && uri.startsWith(QLatin1String("/20141201/lineups/")))
            uris << uri;
    }
    return uris;
}

QVector<Channel> parseSdLineup(const QByteArray &json)
{
    const QJsonObject o = QJsonDocument::fromJson(json).object();
    QHash<QString, QString> numbers;
    for (const QJsonValue v : o.value(QLatin1String("map")).toArray())
    {
        const QJsonObject m = v.toObject();
        QString number = text(m, "channel");
        // (an antenna lineup: its major.minor, else its RF channel)
        if (number.isEmpty() && m.contains(QLatin1String("atscMajor")))
            number = QStringLiteral("%1.%2").arg(m.value(QLatin1String("atscMajor")).toInt())
                         .arg(m.value(QLatin1String("atscMinor")).toInt());
        if (number.isEmpty())
            number = text(m, "uhfVhf");
        const QString station = text(m, "stationID");
        if (!station.isEmpty() && !numbers.contains(station))
            numbers.insert(station, number);
    }
    QVector<Channel> channels;
    for (const QJsonValue v : o.value(QLatin1String("stations")).toArray())
    {
        const QJsonObject s = v.toObject();
        Channel c;
        c.id = text(s, "stationID");
        if (!isDigits(c.id))
            continue;
        c.name = text(s, "name");
        const QString callsign = text(s, "callsign");
        if (c.name.isEmpty())
            c.name = callsign;
        c.altNames << callsign << text(s, "affiliate");
        c.altNames.removeAll(QString());
        c.number = numbers.value(c.id);
        c.logo = text(s.value(QLatin1String("logo")).toObject(), "URL");
        if (!c.name.isEmpty())
            channels.append(c);
    }
    return channels;
}

QByteArray sdSchedulesBody(const QStringList &stationIds, qint64 from, int days)
{
    QJsonArray dates;
    const QDate first = QDateTime::fromSecsSinceEpoch(from, QTimeZone::utc()).date();
    for (int i = 0; i < days; ++i)
        dates.append(first.addDays(i).toString(Qt::ISODate));
    QJsonArray body;
    for (const QString &id : stationIds)
        body.append(QJsonObject { { QStringLiteral("stationID"), id }, { QStringLiteral("date"), dates } });
    return QJsonDocument(body).toJson(QJsonDocument::Compact);
}

QVector<SdAiring> parseSdSchedules(const QByteArray &json)
{
    QVector<SdAiring> airings;
    for (const QJsonValue v : QJsonDocument::fromJson(json).array())
    {
        const QJsonObject s = v.toObject();
        const QString station = text(s, "stationID");
        for (const QJsonValue p : s.value(QLatin1String("programs")).toArray())
        {
            const QJsonObject o = p.toObject();
            SdAiring a;
            a.station = station;
            a.program = text(o, "programID");
            a.start = QDateTime::fromString(text(o, "airDateTime"), Qt::ISODate).toSecsSinceEpoch();
            a.duration = o.value(QLatin1String("duration")).toInt();
            if (!a.station.isEmpty() && !a.program.isEmpty() && a.start > 0 && a.duration > 0)
                airings.append(a);
        }
    }
    return airings;
}

QByteArray sdProgramsBody(const QStringList &programIds)
{
    return QJsonDocument(QJsonArray::fromStringList(programIds)).toJson(QJsonDocument::Compact);
}

QHash<QString, Programme> parseSdPrograms(const QByteArray &json)
{
    QHash<QString, Programme> programmes;
    for (const QJsonValue v : QJsonDocument::fromJson(json).array())
    {
        const QJsonObject o = v.toObject();
        const QString id = text(o, "programID");
        Programme p;
        p.title = text(o.value(QLatin1String("titles")).toArray().at(0).toObject(), "title120");
        p.subtitle = text(o, "episodeTitle150");
        // the long description, else the short one
        const QJsonObject descriptions = o.value(QLatin1String("descriptions")).toObject();
        for (const char *kind : { "description1000", "description100" })
        {
            const QJsonArray list = descriptions.value(QLatin1String(kind)).toArray();
            if (!list.isEmpty())
            {
                p.description = text(list.at(0).toObject(), "description");
                break;
            }
        }
        p.category = o.value(QLatin1String("genres")).toArray().at(0).toString();
        for (const QJsonValue m : o.value(QLatin1String("metadata")).toArray())
        {
            const QJsonObject gracenote = m.toObject().value(QLatin1String("Gracenote")).toObject();
            if (gracenote.contains(QLatin1String("season")))
            {
                p.season = gracenote.value(QLatin1String("season")).toInt();
                p.episode = gracenote.contains(QLatin1String("episode")) ? gracenote.value(QLatin1String("episode")).toInt()
                                                                          : -1;
            }
        }
        if (!id.isEmpty() && !p.title.isEmpty())
            programmes.insert(id, p);
    }
    return programmes;
}

} // namespace servers
} // namespace iptv
