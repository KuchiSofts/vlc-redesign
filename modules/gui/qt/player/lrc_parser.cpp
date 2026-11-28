/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
#include "lrc_parser.hpp"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QStringConverter>
#include <QStringDecoder>

#include <algorithm>

namespace Lrc
{

namespace {

// A number of at most maxDigits digits at text[pos], moved past; -1 when none
qint64 number(const QString &text, qsizetype &pos, int maxDigits)
{
    qint64 value = 0;
    int digits = 0;
    while (pos < text.size() && digits < maxDigits && text[pos].isDigit() && text[pos].unicode() < 128)
    {
        value = value * 10 + (text[pos].unicode() - '0');
        ++pos;
        ++digits;
    }
    return digits ? value : -1;
}

// A time tag "[mm:ss]", "[mm:ss.x]", "[mm:ss.xx]", "[mm:ss.xxx]" (or ':' for
// the '.') at text[pos]: its milliseconds and pos moved past it; -1 otherwise.
qint64 timeTag(const QString &text, qsizetype &pos, QChar open, QChar close)
{
    qsizetype p = pos;
    if (p >= text.size() || text[p] != open)
        return -1;
    ++p;
    const qint64 minutes = number(text, p, 3);
    if (minutes < 0 || p >= text.size() || text[p] != u':')
        return -1;
    ++p;
    const qint64 seconds = number(text, p, 2);
    if (seconds < 0 || seconds > 59)
        return -1;
    qint64 ms = 0;
    if (p < text.size() && (text[p] == u'.' || text[p] == u':'))
    {
        ++p;
        const qsizetype start = p;
        const qint64 fraction = number(text, p, 3);
        if (fraction < 0)
            return -1;
        const qsizetype length = p - start;
        ms = length == 1 ? fraction * 100 : length == 2 ? fraction * 10 : fraction;
        // more digits than milliseconds: left out
        while (p < text.size() && text[p].isDigit())
            ++p;
    }
    if (p >= text.size() || text[p] != close)
        return -1;
    pos = p + 1;
    return minutes * 60000 + seconds * 1000 + ms;
}

// The text of a line without the word times of enhanced .lrc ("<00:01.20>")
QString withoutWordTimes(const QString &text)
{
    if (!text.contains(u'<'))
        return text;
    QString out;
    out.reserve(text.size());
    for (qsizetype i = 0; i < text.size();)
    {
        qsizetype p = i;
        if (timeTag(text, p, u'<', u'>') >= 0)
        {
            i = p;
            continue;
        }
        out += text[i];
        ++i;
    }
    return out;
}

}

QList<Line> parse(const QString &text)
{
    QList<Line> lines;
    qint64 offset = 0;
    const QString normalized = QString(text).replace(QLatin1String("\r\n"), QLatin1String("\n")).replace(u'\r', u'\n');
    for (const QStringView raw : QStringView(normalized).split(u'\n'))
    {
        const QString line = raw.trimmed().toString();
        if (!line.startsWith(u'['))
            continue;

        // [offset:+500]
        if (line.size() > 8 && line.mid(1, 7).compare(QLatin1String("offset:"), Qt::CaseInsensitive) == 0)
        {
            const qsizetype end = line.indexOf(u']');
            bool ok = false;
            const qint64 value = line.mid(8, end - 8).trimmed().toLongLong(&ok);
            // (a day at most: a huge one overflowed the times it is taken from)
            if (end > 8 && ok)
                offset = std::clamp<qint64>(value, -86400000, 86400000);
            continue;
        }

        QList<qint64> times;
        qsizetype pos = 0;
        for (;;)
        {
            const qint64 ms = timeTag(line, pos, u'[', u']');
            if (ms < 0)
                break;
            times.append(ms);
            while (pos < line.size() && line[pos].isSpace())
                ++pos;
        }
        if (times.isEmpty())
            continue; // [ar:], [ti:]...

        const QString words = withoutWordTimes(line.mid(pos)).trimmed();
        // (the limit within a line too: one line of 150,000 time tags made as
        // many lines, each a text item of the lyrics view)
        for (const qint64 ms : times)
        {
            if (lines.size() >= 20000)
                break;
            lines.append({ ms, words });
        }
        if (lines.size() >= 20000)
            break;
    }
    for (Line &l : lines)
        l.ms = std::max<qint64>(0, l.ms - offset);
    std::stable_sort(lines.begin(), lines.end(), [](const Line &a, const Line &b) { return a.ms < b.ms; });
    return lines;
}

QString decode(const QByteArray &data)
{
    if (const auto bom = QStringConverter::encodingForData(data))
    {
        QStringDecoder decoder(*bom);
        return decoder(data);
    }
    QStringDecoder utf8(QStringConverter::Utf8, QStringConverter::Flag::Stateless);
    const QString text = utf8(data);
    if (!utf8.hasError())
        return text;
    return QString::fromLocal8Bit(data);
}

QByteArray toSylt(const QList<Line> &lines)
{
    QByteArray out;
    for (const Line &l : lines)
    {
        QString text = l.text;
        // the separators of the form are not in the text
        text.remove(QChar(0x1E)).remove(QChar(0x1F));
        out += QByteArray::number(l.ms);
        out += '\x1F';
        out += text.toUtf8();
        out += '\x1E';
    }
    return out;
}

QString besideFile(const QString &audioPath)
{
    const QFileInfo audio(audioPath);
    const QString base = audio.dir().filePath(audio.completeBaseName());
    for (const char *suffix : { ".lrc", ".LRC" })
    {
        const QFileInfo lrc(base + QLatin1String(suffix));
        if (lrc.isFile())
            return lrc.filePath();
    }
    return {};
}

QByteArray loadBesideFile(const QString &audioPath)
{
    const QString path = besideFile(audioPath);
    if (path.isEmpty())
        return {};

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.size() > maxFileSize)
        return {};

    // The file can grow after the size check.
    const QByteArray data = file.read(maxFileSize + 1);
    if (data.size() > maxFileSize || file.error() != QFileDevice::NoError)
        return {};

    return toSylt(parse(decode(data)));
}

}
