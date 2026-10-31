/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
#pragma once

#include "iptv_playlist.hpp"

#include <QByteArray>
#include <QString>
#include <QUrl>
#include <QVector>

// Xtream Codes, the account API most IPTV providers offer next to (or
// instead of) an M3U link: player_api.php with the user name and the
// password, which lists the live channels, the films (VOD) and the series
// by category. Pure functions: the addresses to ask, and the answers read.
namespace iptv::xtream {

struct Account
{
    QString server;     // http(s)://host[:port], as the provider gives it
    QString username;
    QString password;
};

// The server address, normalized: a scheme added, a trailing
// "/player_api.php", "/get.php..." or "/" removed
QString normalizeServer(const QString& server);

// An Xtream account read from an M3U link of the provider
// (".../get.php?username=..&password=..&type=m3u_plus"): the same
// provider, with films and series. Empty when the link is not one.
Account accountFromM3uUrl(const QString& url);

QUrl apiUrl(const Account& account, const QString& action = QString(),
            const QList<QPair<QString, QString>>& params = {});

// The account's state ("Active"), its expiry and connections, and the
// server's time zone (for catch-up addresses)
struct AccountInfo
{
    bool authenticated = false;
    QString status;
    qint64 expires = 0;          // seconds since the epoch, 0: never
    int maxConnections = 0;
    int activeConnections = 0;
    int serverOffsetSecs = 0;
    QStringList outputFormats;   // "m3u8", "ts"
    QString error;
};
AccountInfo parseAccountInfo(const QByteArray& json);

struct Category
{
    QString id;
    QString name;
};
QVector<Category> parseCategories(const QByteArray& json);

// The live channels, with the addresses to play them (HLS when the account
// allows it) and their catch-up.
QVector<Channel> parseLiveStreams(const QByteArray& json, const Account& account,
                                  const QVector<Category>& categories,
                                  const AccountInfo& info);

// A film or a series of the provider's library
struct VodItem
{
    QString id;          // stream_id of a film, series_id of a series
    QString name;
    QString poster;
    QString category;
    QString extension;   // the film's container ("mkv", "mp4")
    QString url;         // a film's address (empty for a series)
    double rating = 0;
    int year = 0;
    qint64 added = 0;    // seconds since the epoch
    bool adult = false;
};
QVector<VodItem> parseVodStreams(const QByteArray& json, const Account& account,
                                 const QVector<Category>& categories);
QVector<VodItem> parseSeries(const QByteArray& json, const QVector<Category>& categories);

// The episodes of a series, by season
struct Episode
{
    QString id;
    QString title;
    QString plot;
    QString image;
    QString url;
    int season = 0;
    int number = 0;
    int durationSecs = 0;
};
struct SeriesInfo
{
    QString plot;
    QString cover;
    QVector<Episode> episodes;   // by season, then number
};
SeriesInfo parseSeriesInfo(const QByteArray& json, const Account& account);

}
