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
#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QVector>

// The channels of an IPTV provider, read from its M3U playlist (the format
// every IPTV player takes: #EXTINF lines with tvg-* attributes) or built
// from an Xtream Codes account. Pure data, no Qt objects: parsed on a
// worker thread.
namespace iptv {

// How a channel's past programmes are reached (catch-up), from the
// playlist's catchup attributes or guessed from the stream address
enum class CatchupMode {
    None,
    // the address with ?utc=<start>&lutc=<now> added (catchup="default"/"append")
    Append,
    // the address of catchup-source, its {placeholders} filled in
    Template,
    // Flussonic: .../name.m3u8 becomes .../name-<start>-<duration>.m3u8
    Flussonic,
    // Xtream Codes: /timeshift/<user>/<pass>/<minutes>/<start>/<id>.ts
    Xtream,
};

struct Channel
{
    QString name;
    QString url;
    QString group;       // the category ("News", "Israel"...), empty when none
    QString tvgId;       // the channel's id in the guide (XMLTV)
    QString tvgName;     // the name in the guide when the id is missing
    QString logo;
    // the number the provider gives the channel (tvg-chno), 0 when none
    int number = 0;
    // tvg-shift: the guide of the channel is this many seconds off (the
    // playlist gives hours), the header's when the channel gives none
    int guideShiftSecs = 0;
    // catch-up: how, how many days back, and the template when there is one
    CatchupMode catchup = CatchupMode::None;
    int catchupDays = 0;
    QString catchupSource;
    // Xtream Codes: the server's offset from UTC, for its timeshift addresses
    int timeOffsetSecs = 0;
    // options VLC needs for this stream (#EXTVLCOPT, http-user-agent...)
    QStringList options;
    bool radio = false;
    // adult content (by its group or name), hidden unless allowed
    bool adult = false;
};

struct Playlist
{
    QVector<Channel> channels;
    // the guides the playlist names in its header (url-tvg, x-tvg-url)
    QStringList guideUrls;
    // why nothing was read, empty on success
    QString error;
};

// Reads an M3U / M3U8 playlist of channels. base resolves relative
// addresses.
Playlist parseM3u(const QByteArray& data, const QString& base = QString());

// Whether a playlist looks like a channel list, not an HLS media playlist
// (#EXT-X- segments): an HLS address is one channel, not a list.
bool isChannelList(const QByteArray& data);

// The address that plays the channel from start for duration seconds, or
// an empty string when the channel has no catch-up that far back.
QString catchupUrl(const Channel& channel, const QDateTime& start,
                   int durationSecs, const QDateTime& now);

// "1. Israel" -> "Israel": the numbers some providers put first so their
// groups sort in their order
QString displayGroup(const QString& group);

// Adult content by its group or name ("XXX", "18+", "Adult"...)
bool looksAdult(const QString& group, const QString& name);

}
