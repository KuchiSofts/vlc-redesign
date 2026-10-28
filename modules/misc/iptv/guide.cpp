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
#include <QRegularExpression>
#include <QXmlStreamReader>

#include <algorithm>
#ifdef HAVE_ZLIB
# include <zlib.h>
#endif

namespace iptv {

QString guideKey(const QString& idOrName)
{
    static const QRegularExpression suffix(
        QStringLiteral("\\s*\\b(uhd|fhd|hd|sd|4k|hevc|h265|\\+1|orig|original)\\b\\s*$"),
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression space(QStringLiteral("[\\s._\\-:|]+"));
    QString key = idOrName.trimmed().toLower();
    // a country prefix "IL: " is part of a name only
    static const QRegularExpression prefix(QStringLiteral("^[a-z]{2,3}\\s*[:|]\\s+"));
    key.remove(prefix);
    for (int i = 0; i < 2; ++i)
        key.remove(suffix);
    key.replace(space, QStringLiteral(" "));
    return key.trimmed();
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
    zs.avail_out = uInt(qMin<qint64>(maxSize, 1 << 30));
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
            inflateReset(&zs);
            continue;
        }
        if (ret != Z_OK && ret != Z_BUF_ERROR)
        {
            d->failed = true;
            return -1;
        }
    }
    const qint64 produced = maxSize - zs.avail_out;
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

Guide parseXmltv(QIODevice& device, const GuideFilter& filter)
{
    Guide guide;
    QXmlStreamReader xml(&device);
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
                    // "season.episode.part", counted from 0
                    const QString system = xml.attributes().value(QLatin1String("system")).toString();
                    const QString text = xml.readElementText().simplified();
                    if (system == u"xmltv_ns")
                    {
                        const QStringList parts = text.split(u'.');
                        const int season = parts.value(0).section(u'/', 0, 0).trimmed().toInt() + 1;
                        const int episode = parts.value(1).section(u'/', 0, 0).trimmed().toInt() + 1;
                        if (!parts.value(1).trimmed().isEmpty())
                            p.episode = parts.value(0).trimmed().isEmpty()
                                            ? QStringLiteral("E%1").arg(episode)
                                            : QStringLiteral("S%1E%2").arg(season).arg(episode);
                    }
                    else
                        p.episode = text;
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
    if (xml.hasError() && guide.programmes.isEmpty())
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
    QByteArray xml = isGzip(data) ? gunzip(data) : data;
    if (xml.isEmpty())
    {
        Guide guide;
        guide.error = QStringLiteral("the guide could not be read");
        return guide;
    }
    QBuffer buffer(&xml);
    buffer.open(QIODevice::ReadOnly);
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
