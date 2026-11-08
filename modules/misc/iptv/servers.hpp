/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
#pragma once

#include <QByteArray>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVector>

// The TV servers and tuners of the user (their own Jellyfin, Emby,
// Tvheadend, NextPVR, Channels DVR, HDHomeRun): what each is asked, and
// what it answers, read here without the network (Qt Core only: the tests
// of extras/juxtopposed/iptv-parsers.cpp run them by themselves). The
// service (service.cpp) asks them and writes what they answer as an M3U
// playlist and an XMLTV guide of the source, read then as any other.
namespace iptv {
namespace servers {

// A channel of a server, and a programme of its guide
struct Channel
{
    QString id;       // the server's
    QString name;
    QString number;   // "4.1", "101"
    QString logo;
    QString group;
    QString url;      // what plays it
    QStringList altNames;   // more names the guide gives it (a call sign)
};

struct Programme
{
    QString channel;  // Channel::id
    qint64 start = 0; // seconds since the epoch, UTC
    qint64 stop = 0;
    QString title;
    QString subtitle;
    QString description;
    QString category;
    int season = -1;  // from 1, -1 unknown
    int episode = -1;
};

// The address the user typed, as the base of the server's API: a scheme
// (http), the default port of the server when none, no path but its own
// ("192.168.1.5" → "http://192.168.1.5:8096")
QString baseUrl(const QString &address, int defaultPort);

// The playlist and guide written for the service
QByteArray m3uOf(const QVector<Channel> &channels, const QString &guideUrl);
QByteArray xmltvOf(const QVector<Channel> &channels, const QVector<Programme> &programmes);

// ---------------------------------------------------------------------------
// HDHomeRun: a tuner of the network (no account); its free guide by the
// DeviceAuth of discover.json

struct Tuner
{
    QString id;
    QString name;
    QString baseUrl;
    QString lineupUrl;
    QString deviceAuth;
};
Tuner parseHdhrDiscover(const QByteArray &json);
// api.hdhomerun.com/discover: the tuners of the same network
QVector<Tuner> parseHdhrCloudDiscover(const QByteArray &json);
QVector<Channel> parseHdhrLineup(const QByteArray &json);
QUrl hdhrGuideUrl(const QString &deviceAuth);
// its guide's channels are the lineup's by their GuideNumber
QVector<Programme> parseHdhrGuide(const QByteArray &json);
// The discovery request of the LAN (UDP 65001), and whether an answer is one
QByteArray hdhrDiscoverPacket();
bool isHdhrDiscoverReply(const QByteArray &packet);

// ---------------------------------------------------------------------------
// Jellyfin and Emby (the same API, Emby's under /emby): signed in by the
// user name and password (or an API key), the Live TV of the server

// The header the server wants of every client (Authorization; Emby: X-Emby-
// Authorization), with the session's token once signed in
QByteArray mediaBrowserAuthorization(const QString &deviceId, const QString &version, const QString &token);
QByteArray mediaBrowserLoginBody(const QString &username, const QString &password);
struct Session
{
    QString token;
    QString userId;
    QString userName;
};
Session parseMediaBrowserLogin(const QByteArray &json);
QUrl mediaBrowserChannelsUrl(const QString &base, const QString &userId, bool emby);
QVector<Channel> parseMediaBrowserChannels(const QByteArray &json, const QString &base, bool emby);
QUrl mediaBrowserProgrammesUrl(const QString &base, const QString &userId, qint64 from, qint64 to, bool emby);
QVector<Programme> parseMediaBrowserProgrammes(const QByteArray &json);
// A channel's stream, opened when it plays: its playback information (a
// live stream the server opens), then its address
QUrl mediaBrowserPlaybackInfoUrl(const QString &base, const QString &channelId, const QString &userId, bool emby);
QByteArray mediaBrowserPlaybackInfoBody();
QString mediaBrowserStreamUrl(const QByteArray &playbackInfo, const QString &base, const QString &channelId,
                              const QString &token, bool emby);
// The discovery of the LAN (UDP 7359): the request, and its answer's server
QByteArray mediaBrowserDiscoverPacket(bool emby);
struct Found
{
    QString kind;     // "jellyfin", "emby", "hdhomerun"
    QString name;
    QString address;
    QString id;
};
Found parseMediaBrowserDiscoverReply(const QByteArray &packet, bool emby);

// ---------------------------------------------------------------------------
// Tvheadend: its M3U and XMLTV, behind its user's name and password (Basic
// or Digest); the streams carry the user, for the player

QUrl tvheadendPlaylistUrl(const QString &base);
QUrl tvheadendGuideUrl(const QString &base);
// the playlist with each stream's address carrying the user
QByteArray tvheadendWithUser(const QByteArray &m3u, const QString &username, const QString &password);

// ---------------------------------------------------------------------------
// Channels DVR: its M3U and XMLTV (no account on the network)

QUrl channelsPlaylistUrl(const QString &base);
QUrl channelsGuideUrl(const QString &base);

// ---------------------------------------------------------------------------
// NextPVR (5 and later, JSON): a session signed in by the PIN, its channels
// and their listings

QUrl nextPvrInitiateUrl(const QString &base);
struct Salted
{
    QString sid;
    QString salt;
};
Salted parseNextPvrInitiate(const QByteArray &json);
// md5(":" + md5(pin) + ":" + salt)
QString nextPvrLoginKey(const QString &pin, const QString &salt);
QUrl nextPvrLoginUrl(const QString &base, const QString &sid, const QString &key);
bool parseNextPvrLogin(const QByteArray &json);
QUrl nextPvrChannelsUrl(const QString &base, const QString &sid);
QVector<Channel> parseNextPvrChannels(const QByteArray &json, const QString &base, const QString &sid);
QUrl nextPvrListingsUrl(const QString &base, const QString &sid, const QString &channelId, qint64 from, qint64 to);
QVector<Programme> parseNextPvrListings(const QByteArray &json, const QString &channelId);

// ---------------------------------------------------------------------------
// Multicast DNS (Bonjour): what Channels DVR is found by on the network. A
// question to 224.0.0.251:5353 from another port is answered to that port
// (RFC 6762, 6.7); the answer names the service, its port and address.

QByteArray mdnsQuery(const QString &service);   // "_channels_dvr._tcp.local"
struct MdnsService
{
    QString name;      // its instance's first label ("Channels DVR on Den")
    QString host;      // the host its SRV names
    QString address;   // its IPv4 address, when the answer gives it
    int port = 0;
};
QVector<MdnsService> parseMdnsAnswer(const QByteArray &packet, const QString &service);

// ---------------------------------------------------------------------------
// Schedules Direct (json.schedulesdirect.org, its API 20141201): a paid
// guide (the US, Canada and more) for the channels of every source. A token
// for the user (the password as its SHA-1), the lineups of the account,
// their stations, the schedules of the stations, then the programmes they
// name.

QString sdBase();
// the password as Schedules Direct takes it (kept so: the password itself
// is never needed)
QString sdPasswordHash(const QString &password);
QByteArray sdTokenBody(const QString &username, const QString &passwordHash);
struct SdAnswer
{
    QString token;
    int code = -1;     // 0: done; 4003 a wrong user or password...
    QString message;
};
SdAnswer parseSdToken(const QByteArray &json);
// the lineups of the account (their uri), the deleted ones left out
QStringList parseSdLineups(const QByteArray &json, int *code);
// the stations of a lineup as channels: the station id; its name, call
// sign and network as names; the number of the lineup
QVector<Channel> parseSdLineup(const QByteArray &json);
// the days of the stations asked, from the day of from (UTC)
QByteArray sdSchedulesBody(const QStringList &stationIds, qint64 from, int days);
struct SdAiring
{
    QString station;
    QString program;
    qint64 start = 0;
    int duration = 0;
};
QVector<SdAiring> parseSdSchedules(const QByteArray &json);
QByteArray sdProgramsBody(const QStringList &programIds);
// the programmes, by their programID (their channel and times not set)
QHash<QString, Programme> parseSdPrograms(const QByteArray &json);

} // namespace servers
} // namespace iptv
