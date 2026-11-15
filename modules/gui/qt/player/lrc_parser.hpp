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
#include <QList>
#include <QString>

// The lyrics of an .lrc file beside a song ("song.lrc" next to "song.mp3"),
// as the Android app reads them: "[01:02.50]text" lines, several times on a
// line, [offset:ms] (a positive one shows the lines sooner); the other tags
// ([ar:], [ti:]...) and the word times of enhanced .lrc ("<01:02.50>") are
// left out. The player shows them as it shows an MP3's synchronized lyrics.
namespace Lrc
{

struct Line
{
    qint64 ms;
    QString text;
};

// The largest .lrc read (a song's lyrics are a few KB)
inline constexpr qint64 maxFileSize = 1024 * 1024;

// The timed lines of an .lrc's text, sorted by time; none when it has no
// timed line.
QList<Line> parse(const QString &text);

// The text of an .lrc file's bytes: UTF-8 (with or without its BOM), UTF-16
// or UTF-32 with a BOM, else the system's 8-bit code page.
QString decode(const QByteArray &data);

// The lines in the form of VLC's "sylt-data" (the milliseconds, 0x1F, the
// text, each entry ended by 0x1E), the form the player reads.
QByteArray toSylt(const QList<Line> &lines);

// The .lrc beside a local file (the same name with .lrc or .LRC), or an
// empty string.
QString besideFile(const QString &audioPath);

}
