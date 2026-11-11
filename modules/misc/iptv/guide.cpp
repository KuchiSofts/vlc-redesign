/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "guide.hpp"

#include <QBuffer>
#include <QDateTime>
#include <QFile>
#include <QTimeZone>
#include <QXmlStreamReader>

#include <algorithm>
#ifdef HAVE_ZLIB
# include <zlib.h>
#endif

namespace iptv {

// (no regular expressions in this plugin: their per-thread JIT stacks of
// the plugin's static Qt crashed VLC when the threads ended)
QString guideKey(const QString& idOrName)
{
    QString key = idOrName.trimmed().toLower();
    // a country prefix "IL: " is part of a name only
    {
        qsizetype i = 0;
        while (i < key.size() && key[i] >= u'a' && key[i] <= u'z')
            ++i;
        qsizetype j = i;
        while (j < key.size() && key[j].isSpace())
            ++j;
        if (i >= 2 && i <= 3 && j < key.size() && (key[j] == u':' || key[j] == u'|')
            && j + 1 < key.size() && key[j + 1].isSpace())
            key.remove(0, j + 1);
    }
    // the separators as one space
    QString folded;
    folded.reserve(key.size());
    for (const QChar c : std::as_const(key))
    {
        const bool separator = c.isSpace() || c == u'.' || c == u'_' || c == u'-' || c == u':' || c == u'|';
        if (!separator)
            folded += c;
        else if (!folded.isEmpty() && !folded.endsWith(u' '))
            folded += u' ';
    }
    key = folded.trimmed();
    // the quality at the end ("HD", "FHD", "4K", "+1"...), twice
    static const char* const suffixes[] = { "uhd", "fhd", "hd", "sd", "4k", "hevc", "h265", "+1",
                                            "orig", "original" };
    for (int pass = 0; pass < 2; ++pass)
        for (const char* suffix : suffixes)
        {
            const QLatin1String s(suffix);
            if (key.size() > s.size() && key.endsWith(s))
            {
                const QChar before = key[key.size() - s.size() - 1];
                if (!before.isLetterOrNumber() || s == QLatin1String("+1"))
                {
                    key.chop(s.size());
                    key = key.trimmed();
                    break;
                }
            }
        }
    return key;
}

qint64 parseXmltvTime(QStringView text)
{
    // YYYYMMDDhhmmss [+-]hhmm, the seconds and the zone optional
    text = text.trimmed();
    if (text.size() < 12)
        return 0;
    auto num = [&](qsizetype from, qsizetype len) {
        bool ok = false;
        const int v = text.mid(from, len).toInt(&ok);
        return ok ? v : -1;
    };
    const int y = num(0, 4), mo = num(4, 2), d = num(6, 2), h = num(8, 2), mi = num(10, 2);
    const int s = text.size() >= 14 && text[12].isDigit() ? num(12, 2) : 0;
    if (y < 0 || mo < 0 || d < 0 || h < 0 || mi < 0 || s < 0)
        return 0;
    const QDateTime utc(QDate(y, mo, d), QTime(h, mi, s), QTimeZone::utc());
    if (!utc.isValid())
        return 0;
    qint64 secs = utc.toSecsSinceEpoch();
    const qsizetype sign = qMax(text.indexOf(u'+'), text.indexOf(u'-', 12));
    if (sign > 0 && text.size() >= sign + 5)
    {
        const int zh = text.mid(sign + 1, 2).toInt();
        const int zm = text.mid(sign + 3, 2).toInt();
        const int offset = zh * 3600 + zm * 60;
        secs -= text[sign] == u'+' ? offset : -offset;
    }
    return secs;
}

bool isGzip(const QByteArray& data)
{
    return data.size() > 2 && uchar(data[0]) == 0x1f && uchar(data[1]) == 0x8b;
}

QByteArray gunzip(const QByteArray& data, qint64 limit)
{
#ifndef HAVE_ZLIB
    Q_UNUSED(data);
    Q_UNUSED(limit);
    return QByteArray();
#else
    z_stream zs{};
    // 16 + MAX_WBITS: a gzip header
    if (inflateInit2(&zs, 16 + MAX_WBITS) != Z_OK)
        return QByteArray();
    QByteArray out;
    out.reserve(qMin<qint64>(limit, qint64(data.size()) * 6));
    zs.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(data.constData()));
    zs.avail_in = uInt(data.size());
    char chunk[64 * 1024];
    int ret = Z_OK;
    bool complete = false;   // a whole gzip member read
    while (ret != Z_STREAM_END)
    {
        zs.next_out = reinterpret_cast<Bytef*>(chunk);
        zs.avail_out = sizeof(chunk);
        ret = inflate(&zs, Z_NO_FLUSH);
        if (ret != Z_OK && ret != Z_STREAM_END)
        {
            // several gzip members one after the other: go on with the next
            if (ret == Z_BUF_ERROR && zs.avail_in == 0)
                break;
            // bytes after a whole member that are no other one (zeros some
            // servers pad with): the end of the data, not an error
            if (complete && zs.total_out == 0)
                break;
            out.clear();
            break;
        }
        out.append(chunk, int(sizeof(chunk) - zs.avail_out));
        if (out.size() > limit)
        {
            out.clear();
            break;
        }
        if (ret == Z_STREAM_END && zs.avail_in > 0)
        {
            complete = true;
            inflateReset(&zs);
            ret = Z_OK;
        }
    }
    inflateEnd(&zs);
    return out;
#endif
}

struct GunzipDevice::State
{
    QIODevice* source = nullptr;
    qint64 limit = 0;
    qint64 produced = 0;
    QByteArray input;
    bool ended = false;
    bool failed = false;
    bool complete = false;   // a whole gzip member read
#ifdef HAVE_ZLIB
    z_stream zs{};
    bool initialized = false;
#endif
};

GunzipDevice::GunzipDevice(QIODevice* source, qint64 limit)
    : d(std::make_unique<State>())
{
    d->source = source;
    d->limit = limit;
    d->input.resize(256 * 1024);
#ifdef HAVE_ZLIB
    // 16 + MAX_WBITS: a gzip header
    d->initialized = inflateInit2(&d->zs, 16 + MAX_WBITS) == Z_OK;
    d->failed = !d->initialized;
#else
    d->failed = true;
#endif
}

GunzipDevice::~GunzipDevice()
{
#ifdef HAVE_ZLIB
    if (d->initialized)
        inflateEnd(&d->zs);
#endif
}

bool GunzipDevice::atEnd() const
{
    return (d->ended || d->failed) && QIODevice::atEnd();
}

qint64 GunzipDevice::readData(char* data, qint64 maxSize)
{
#ifndef HAVE_ZLIB
    Q_UNUSED(data);
    Q_UNUSED(maxSize);
    return -1;
#else
    if (d->failed)
        return -1;
    if (d->ended || maxSize <= 0)
        return 0;
    z_stream& zs = d->zs;
    zs.next_out = reinterpret_cast<Bytef*>(data);
    const uInt room = uInt(qMin<qint64>(maxSize, 1 << 30));
    zs.avail_out = room;
    while (zs.avail_out > 0)
    {
        if (zs.avail_in == 0)
        {
            const qint64 got = d->source->read(d->input.data(), d->input.size());
            if (got <= 0)
            {
                // the end of the file: what was inflated so far is all
                d->ended = true;
                break;
            }
            zs.next_in = reinterpret_cast<Bytef*>(d->input.data());
            zs.avail_in = uInt(got);
        }
        const int ret = inflate(&zs, Z_NO_FLUSH);
        if (ret == Z_STREAM_END)
        {
            // several gzip members one after the other: go on with the next
            d->complete = true;
            inflateReset(&zs);
            continue;
        }
        if (ret != Z_OK && ret != Z_BUF_ERROR)
        {
            // bytes after a whole member that are no other one (zeros some
            // servers pad with): the end of the data, what came is kept
            if (d->complete && zs.total_out == 0)
            {
                d->ended = true;
                break;
            }
            d->failed = true;
            return -1;
        }
    }
    const qint64 produced = qint64(room - zs.avail_out);
    d->produced += produced;
    if (d->produced > d->limit)
    {
        d->failed = true;
        return -1;
    }
    return produced;
#endif
}

Guide parseXmltvFile(const QString& path, const GuideFilter& filter)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        Guide guide;
        guide.error = file.errorString();
        return guide;
    }
    const QByteArray magic = file.peek(2);
    if (isGzip(magic + QByteArray(1, '\0')))
    {
        GunzipDevice inflated(&file);
        inflated.open(QIODevice::ReadOnly);
        return parseXmltv(inflated, filter);
    }
    return parseXmltv(file, filter);
}

namespace {

// In front of the XML reader: it keeps a whole text (a run of spaces, a
// comment) in memory, an inflated guide of 4 GB of spaces asked for 16 GB.
// The reading fails after `longest` bytes without a '<' (no guide has a text
// of megabytes), and stops when it is cancelled (VLC quitting), even inside
// such a text.
class XmlGuard : public QIODevice
{
public:
    XmlGuard(QIODevice* source, const std::atomic<bool>* cancel, qint64 longest)
        : m_source(source), m_cancel(cancel), m_longest(longest)
    {
    }
    bool isSequential() const override { return true; }
    bool atEnd() const override { return (m_ended || m_stopped) && QIODevice::atEnd(); }
    bool cancelled() const { return m_cancelled; }
    bool tooLong() const { return m_tooLong; }

protected:
    qint64 readData(char* data, qint64 maxSize) override
    {
        if (m_stopped)
            return -1;
        if (m_cancel && m_cancel->load())
        {
            m_cancelled = m_stopped = true;
            return -1;
        }
        const qint64 got = m_source->read(data, maxSize);
        if (got <= 0)
        {
            m_ended = true;
            return got;
        }
        qint64 last = got - 1;
        while (last >= 0 && data[last] != '<')
            --last;
        m_since = last >= 0 ? got - 1 - last : m_since + got;
        if (m_since > m_longest)
        {
            m_tooLong = m_stopped = true;
            return -1;
        }
        return got;
    }
    qint64 writeData(const char*, qint64) override { return -1; }

private:
    QIODevice* const m_source;
    const std::atomic<bool>* const m_cancel;
    const qint64 m_longest;
    qint64 m_since = 0;
    bool m_ended = false;
    bool m_stopped = false;
    bool m_cancelled = false;
    bool m_tooLong = false;
};

}

Guide parseXmltv(QIODevice& device, const GuideFilter& filter)
{
    Guide guide;
    // (8 MB at most without a '<': a text the reader would hold whole)
    XmlGuard guarded(&device, filter.cancel, 8 << 20);
    guarded.open(QIODevice::ReadOnly | QIODevice::Unbuffered);
    QXmlStreamReader xml(&guarded);
    int sinceCheck = 0;
    // a guide id -> the key its programmes are kept under ("" dropped)
    QHash<QString, QString> keyOfId;
    const bool all = filter.keys.isEmpty();

    auto keyFor = [&](const QString& id) -> QString {
        auto it = keyOfId.constFind(id);
        if (it != keyOfId.cend())
            return *it;
        // a programme of a channel the header did not list
        const QString k = guideKey(id);
        const QString key = all || filter.keys.contains(k) ? k : QString();
        keyOfId.insert(id, key);
        return key;
    };

    while (!xml.atEnd())
    {
        if (xml.readNext() != QXmlStreamReader::StartElement)
            continue;
        if (filter.cancel && ++sinceCheck >= 4096)
        {
            sinceCheck = 0;
            if (filter.cancel->load())
            {
                guide.error = QStringLiteral("cancelled");
                return guide;
            }
        }
        if (xml.name() == u"channel")
        {
            const QString id = xml.attributes().value(QLatin1String("id")).toString();
            QString key;
            const QString idKey = guideKey(id);
            if (all || filter.keys.contains(idKey))
                key = idKey;
            // its display names: a channel without tvg-id is matched by one
            while (xml.readNextStartElement())
            {
                if (xml.name() == u"display-name")
                {
                    const QString name = xml.readElementText();
                    const QString nameKey = guideKey(name);
                    guide.idByName.insert(nameKey, idKey);
                    if (key.isEmpty() && filter.keys.contains(nameKey))
                        key = nameKey;
                }
                else
                    xml.skipCurrentElement();
            }
            keyOfId.insert(id, key);
        }
        else if (xml.name() == u"programme")
        {
            const auto attrs = xml.attributes();
            const QString key = keyFor(attrs.value(QLatin1String("channel")).toString());
            const qint64 start = parseXmltvTime(attrs.value(QLatin1String("start")));
            qint64 stop = parseXmltvTime(attrs.value(QLatin1String("stop")));
            if (key.isEmpty() || start <= 0
                || (stop > 0 && stop < filter.from)
                || (filter.until > 0 && start > filter.until))
            {
                xml.skipCurrentElement();
                continue;
            }
            Programme p;
            p.start = start;
            p.stop = stop;
            // a guide may give each text in several languages: the first one,
            // or the first in a language asked for
            bool titleWanted = false, descWanted = false;
            while (xml.readNextStartElement())
            {
                const auto name = xml.name();
                const QString lang = xml.attributes().value(QLatin1String("lang")).toString().toLower();
                const bool wanted = !lang.isEmpty() && filter.languages.contains(lang);
                if (name == u"title" && (p.title.isEmpty() || (wanted && !titleWanted)))
                {
                    p.title = xml.readElementText().simplified();
                    p.language = lang;
                    titleWanted = wanted;
                    if (!lang.isEmpty())
                        guide.languages.insert(lang);
                }
                else if (name == u"sub-title" && p.subtitle.isEmpty())
                    p.subtitle = xml.readElementText().simplified();
                else if (name == u"desc" && (p.description.isEmpty() || (wanted && !descWanted)))
                {
                    p.description = xml.readElementText().trimmed().left(2000);
                    descWanted = wanted;
                }
                else if (name == u"category" && p.category.isEmpty())
                    p.category = xml.readElementText().simplified();
                else if (name == u"episode-num" && p.episode.isEmpty())
                {
                    // "onscreen" (S01E02) reads as it is; xmltv_ns is
                    // "season.episode.part", counted from 0; the other
                    // systems are skipped (a guide gives several)
                    const QString system = xml.attributes().value(QLatin1String("system")).toString();
                    const QString text = xml.readElementText().simplified();
                    if (system == u"xmltv_ns")
                    {
                        // (numbers of 0 to 9999 only: "2147483647" plus one
                        // overflowed, a guide's garbage is no episode)
                        const QStringList parts = text.split(u'.');
                        auto numberOf = [](const QString& part) {
                            bool ok = false;
                            const int n = part.section(u'/', 0, 0).trimmed().toInt(&ok);
                            return ok && n >= 0 && n <= 9999 ? n : -1;
                        };
                        const int season = numberOf(parts.value(0));
                        const int episode = numberOf(parts.value(1));
                        if (episode >= 0)
                            p.episode = season < 0 ? QStringLiteral("E%1").arg(episode + 1)
                                                   : QStringLiteral("S%1E%2").arg(season + 1).arg(episode + 1);
                    }
                    else if (system.isEmpty() || system == u"onscreen" || system == u"common")
                        p.episode = text;
                    // the others are no episode to show: "original-air-date"
                    // (19990101080000 +0000), "dd_progid", a provider's ids
                }
                else
                    xml.skipCurrentElement();
            }
            // only the languages asked for (a programme in none of them is
            // kept when the guide names no language)
            if (!filter.languages.isEmpty() && !p.language.isEmpty()
                && !filter.languages.contains(p.language))
                continue;
            guide.programmes[key].append(std::move(p));
        }
    }
    // (cancelled, or a text too long to read: an error, the guide not kept
    // as read whole even with the programmes before it)
    if (guarded.cancelled())
        guide.error = QStringLiteral("cancelled");
    else if (guarded.tooLong())
        guide.error = QStringLiteral("a text of the guide is too long");
    else if (xml.hasError() && guide.programmes.isEmpty())
        guide.error = xml.errorString();

    // sorted, and the missing ends taken from the next start
    for (auto it = guide.programmes.begin(); it != guide.programmes.end(); ++it)
    {
        auto& list = it.value();
        std::sort(list.begin(), list.end(),
                  [](const Programme& a, const Programme& b) { return a.start < b.start; });
        for (qsizetype i = 0; i < list.size(); ++i)
            if (list[i].stop <= list[i].start)
                list[i].stop = i + 1 < list.size() ? list[i + 1].start : list[i].start + 3600;
        list.squeeze();
    }
    return guide;
}

Guide parseXmltv(const QByteArray& data, const GuideFilter& filter)
{
    if (data.isEmpty())
    {
        Guide guide;
        guide.error = QStringLiteral("the guide could not be read");
        return guide;
    }
    QBuffer buffer;
    buffer.setData(data);
    buffer.open(QIODevice::ReadOnly);
    // (a gzip one read inflated a piece at a time, as a file is: inflated
    // whole first, a bomb of a few hundred kilobytes took gigabytes)
    if (isGzip(data))
    {
        GunzipDevice inflated(&buffer, 1LL << 30);
        inflated.open(QIODevice::ReadOnly);
        return parseXmltv(inflated, filter);
    }
    return parseXmltv(buffer, filter);
}

int programmeAt(const QVector<Programme>& list, qint64 t)
{
    auto it = std::upper_bound(list.cbegin(), list.cend(), t,
                               [](qint64 v, const Programme& p) { return v < p.start; });
    if (it == list.cbegin())
        return -1;
    --it;
    return t < it->stop ? int(it - list.cbegin()) : -1;
}

}
