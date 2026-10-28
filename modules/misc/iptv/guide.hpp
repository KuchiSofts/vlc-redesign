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
#include <QIODevice>
#include <QSet>
#include <QString>
#include <QVector>

#include <atomic>
#include <memory>

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
    QString episode;    // "S01E02", as the guide numbers it
    QString language;   // of the title ("he", "en"), empty when not given
};

struct Guide
{
    // by the channel's key (see guideKey), sorted by start
    QHash<QString, QVector<Programme>> programmes;
    // the guide's display names of a channel id, to match channels that
    // have no tvg-id by their name
    QHash<QString, QString> idByName;
    // the languages of the titles the guide gives
    QSet<QString> languages;
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
    // the languages kept ("he", "en"), lower case; empty: all of them
    QSet<QString> languages;
    // set from another thread to stop the reading (VLC quitting)
    const std::atomic<bool>* cancel = nullptr;
};

// A gzip-compressed device read inflated, a piece at a time: a guide of
// hundreds of megabytes is read without holding it whole in memory. Reads
// fail past limit bytes (a decompression bomb) or on corrupt data.
class GunzipDevice : public QIODevice
{
public:
    explicit GunzipDevice(QIODevice* source, qint64 limit = 4LL * 1024 * 1024 * 1024);
    ~GunzipDevice() override;
    bool isSequential() const override { return true; }
    bool atEnd() const override;

protected:
    qint64 readData(char* data, qint64 maxSize) override;
    qint64 writeData(const char*, qint64) override { return -1; }

private:
    struct State;
    std::unique_ptr<State> d;
};

// Reads the guide of a file, gzip-compressed or not (on a worker thread)
Guide parseXmltvFile(const QString& path, const GuideFilter& filter);

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
