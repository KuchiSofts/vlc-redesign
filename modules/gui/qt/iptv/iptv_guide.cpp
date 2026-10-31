/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
#include "iptv_guide.hpp"

#include <QBuffer>
#include <QDateTime>
#include <QTimeZone>
#include <QRegularExpression>
#include <QXmlStreamReader>

#include <algorithm>
#include <zlib.h>

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
}

Guide parseXmltv(QIODevice& device, const GuideFilter& filter)
{
    Guide guide;
    QXmlStreamReader xml(&device);
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
        if (xml.name() == u"channel")
        {
            const QString id = xml.attributes().value(u"id").toString();
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
            const QString key = keyFor(attrs.value(u"channel").toString());
            const qint64 start = parseXmltvTime(attrs.value(u"start"));
            qint64 stop = parseXmltvTime(attrs.value(u"stop"));
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
            while (xml.readNextStartElement())
            {
                const auto name = xml.name();
                if (name == u"title" && p.title.isEmpty())
                    p.title = xml.readElementText().simplified();
                else if (name == u"sub-title" && p.subtitle.isEmpty())
                    p.subtitle = xml.readElementText().simplified();
                else if (name == u"desc" && p.description.isEmpty())
                    p.description = xml.readElementText().trimmed().left(2000);
                else if (name == u"category" && p.category.isEmpty())
                    p.category = xml.readElementText().simplified();
                else
                    xml.skipCurrentElement();
            }
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
