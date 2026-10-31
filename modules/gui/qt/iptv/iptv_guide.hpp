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
#include <QSet>
#include <QString>
#include <QVector>

class QIODevice;

// The programme guide (EPG) of the channels, read from XMLTV (the format
// IPTV guides use). A guide covers thousands of channels and weeks: only
// the channels of the playlist and the days around now are kept.
namespace iptv {

struct Programme
{
    qint64 start = 0;   // seconds since the epoch, UTC
    qint64 stop = 0;
    QString title;
    QString subtitle;   // the episode title
    QString description;
    QString category;
};

struct Guide
{
    // by the channel's key (see guideKey), sorted by start
    QHash<QString, QVector<Programme>> programmes;
    // the guide's display names of a channel id, to match channels that
    // have no tvg-id by their name
    QHash<QString, QString> idByName;
    QString error;
};

// What the guide is asked to keep
struct GuideFilter
{
    // the keys of the channels wanted (tvg-id, or the name when there is
    // none): see guideKey. Empty: every channel.
    QSet<QString> keys;
    qint64 from = 0;     // programmes ending before are dropped
    qint64 until = 0;    // programmes starting after are dropped (0: none)
};

// The key a channel or a guide entry is matched by: the id when there is
// one, else the name, both folded (case, spaces, "HD"/"FHD" suffixes).
QString guideKey(const QString& idOrName);

// Reads an XMLTV guide; a gzip-compressed one (.xml.gz) is inflated
// first. Runs for seconds on a large guide: call it on a worker thread.
Guide parseXmltv(QIODevice& device, const GuideFilter& filter);
Guide parseXmltv(const QByteArray& data, const GuideFilter& filter);

// Whether the data is gzip-compressed, and inflates it (empty on an
// error). limit protects against a decompression bomb.
bool isGzip(const QByteArray& data);
QByteArray gunzip(const QByteArray& data, qint64 limit = 1024LL * 1024 * 1024);

// "20261025203000 +0300" -> seconds since the epoch, UTC (0 on an error)
qint64 parseXmltvTime(QStringView text);

// The programme playing at time t in a sorted list (-1: none)
int programmeAt(const QVector<Programme>& list, qint64 t);

}
