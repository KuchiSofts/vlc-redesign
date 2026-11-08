/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
#include "autoguide.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QUrl>

#include <algorithm>

namespace iptv {
namespace autoguide {

namespace {

bool isAsciiLetter(QChar c)
{
    return c.unicode() >= u'a' && c.unicode() <= u'z';
}

// "uk", "us", "usa" -> "GB", "US"; empty when the token is no country
QString countryCode(const QString& token)
{
    if (token == QLatin1String("uk") || token == QLatin1String("gb"))
        return QStringLiteral("GB");
    if (token == QLatin1String("usa"))
        return QStringLiteral("US");
    // (two letters that are rather what the channel is)
    static const char* const notCountries[] = { "tv", "hd", "sd", "fm", "am", "uhd", "4k" };
    for (const char* word : notCountries)
        if (token == QLatin1String(word))
            return QString();
    if (token.size() == 2 && isAsciiLetter(token[0]) && isAsciiLetter(token[1]))
        return token.toUpper();
    return QString();
}

// "+1" -> 1 (0: it is not a later copy)
int plusOf(const QString& token)
{
    if (token.size() < 2 || token.size() > 3 || token[0] != u'+')
        return 0;
    bool ok = false;
    const int hours = token.mid(1).toInt(&ok);
    return ok && hours >= 1 && hours <= 12 ? hours : 0;
}

bool isQuality(const QString& token)
{
    static const char* const words[] = { "hd", "fhd", "uhd", "sd", "4k", "8k", "hevc", "h264", "h265", "hdr",
                                         "1080p", "1080i", "720p", "2160p", "576p", "576i", "480p", "360p",
                                         "orig", "original", "backup" };
    for (const char* word : words)
        if (token == QLatin1String(word))
            return true;
    return false;
}

QString text(const QJsonValue& v)
{
    return v.toString().trimmed();
}

}

QVector<Known> parseIptvOrgChannels(const QByteArray& json)
{
    QVector<Known> channels;
    const QJsonArray list = QJsonDocument::fromJson(json).array();
    channels.reserve(list.size());
    for (const QJsonValue v : list)
    {
        const QJsonObject o = v.toObject();
        // closed: off the air (its guide too)
        if (!text(o.value(QLatin1String("closed"))).isEmpty())
            continue;
        Known k;
        k.id = text(o.value(QLatin1String("id")));
        k.name = text(o.value(QLatin1String("name")));
        for (const QJsonValue a : o.value(QLatin1String("alt_names")).toArray())
            if (!text(a).isEmpty())
                k.altNames << text(a);
        // (iptv-org names the United Kingdom "UK")
        k.country = countryCode(text(o.value(QLatin1String("country"))).toLower());
        if (!k.id.isEmpty() && !k.name.isEmpty())
            channels.append(k);
    }
    return channels;
}

QByteArray compactChannels(const QVector<Known>& channels)
{
    QJsonArray list;
    for (const Known& k : channels)
        list.append(QJsonArray { k.id, k.name, QJsonArray::fromStringList(k.altNames), k.country });
    return QJsonDocument(list).toJson(QJsonDocument::Compact);
}

QVector<Known> readCompactChannels(const QByteArray& data)
{
    QVector<Known> channels;
    const QJsonArray list = QJsonDocument::fromJson(data).array();
    channels.reserve(list.size());
    for (const QJsonValue v : list)
    {
        const QJsonArray a = v.toArray();
        Known k;
        k.id = a.at(0).toString();
        k.name = a.at(1).toString();
        for (const QJsonValue n : a.at(2).toArray())
            k.altNames << n.toString();
        k.country = a.at(3).toString();
        if (!k.id.isEmpty())
            channels.append(k);
    }
    return channels;
}

QString matchName(const QString& rawName, QString* country, int* plusHours, bool* endsWithTv)
{
    QString found;
    int plus = 0;
    // (a name of 512 characters at most: no channel has more, and a megabyte
    // of '(' never closed, each looked for to the end, took a minute)
    const QString name = rawName.left(512);
    // lower case, the accents off
    QString folded;
    {
        const QString decomposed = name.normalized(QString::NormalizationForm_KD);
        folded.reserve(decomposed.size());
        for (const QChar c : decomposed)
        {
            const QChar::Category category = c.category();
            if (category == QChar::Mark_NonSpacing || category == QChar::Mark_SpacingCombining
                || category == QChar::Mark_Enclosing)
                continue;
            folded += c.toLower();
        }
    }
    // what is in brackets: a country, a "+1", or notes ("(1080p)",
    // "[Geo-blocked]", "[Not 24/7]") dropped
    QString rest;
    rest.reserve(folded.size());
    for (qsizetype i = 0; i < folded.size(); ++i)
    {
        const QChar c = folded[i];
        if (c == u'(' || c == u'[' || c == u'{')
        {
            const QChar close = c == u'(' ? u')' : c == u'[' ? u']' : u'}';
            const qsizetype end = folded.indexOf(close, i + 1);
            if (end > i)
            {
                const QString inside = folded.mid(i + 1, end - i - 1).trimmed();
                const QString code = countryCode(inside);
                if (!code.isEmpty() && found.isEmpty())
                    found = code;
                else if (plusOf(inside) > 0)
                    plus = plusOf(inside);
                rest += u' ';
                i = end;
                continue;
            }
        }
        rest += c;
    }
    // a country first: "UK: ", "US | ", "|FR| ", "DE - "
    {
        qsizetype i = 0;
        while (i < rest.size() && (rest[i].isSpace() || rest[i] == u'|'))
            ++i;
        qsizetype j = i;
        while (j < rest.size() && isAsciiLetter(rest[j]))
            ++j;
        qsizetype k = j;
        while (k < rest.size() && rest[k].isSpace())
            ++k;
        if (j - i >= 2 && j - i <= 3 && k < rest.size()
            && (rest[k] == u':' || rest[k] == u'|' || (rest[k] == u'-' && k > j)))
        {
            const QString code = countryCode(rest.mid(i, j - i));
            if (!code.isEmpty())
            {
                if (found.isEmpty())
                    found = code;
                rest.remove(0, k + 1);
            }
        }
    }
    // the words: letters and digits ("&" as "and"), a "+1" apart
    QStringList tokens;
    QString token;
    auto flush = [&]() {
        if (!token.isEmpty())
            tokens << token;
        token.clear();
    };
    for (qsizetype i = 0; i < rest.size(); ++i)
    {
        const QChar c = rest[i];
        if (c == u'+')
        {
            qsizetype j = i + 1;
            while (j < rest.size() && rest[j].isDigit())
                ++j;
            const int hours = plusOf(rest.mid(i, j - i));
            if (hours > 0 && (j >= rest.size() || !rest[j].isLetterOrNumber()))
            {
                flush();
                plus = hours;
                i = j - 1;
                continue;
            }
            flush();
            tokens << QStringLiteral("plus");
        }
        else if (c == u'&')
        {
            flush();
            tokens << QStringLiteral("and");
        }
        else if (c.isLetterOrNumber())
            token += c;
        else
            flush();
    }
    flush();
    // the quality at the end ("HD", "4K", "1080p"), as many as there are
    while (tokens.size() > 1 && isQuality(tokens.constLast()))
        tokens.removeLast();
    if (country)
        *country = found;
    if (plusHours)
        *plusHours = plus;
    // ("Arte TV": its last word, the key "artetv"; not "MTV", one word)
    if (endsWithTv)
        *endsWithTv = tokens.size() > 1 && tokens.constLast() == QLatin1String("tv");
    return tokens.join(QString());
}

QString countryOfId(const QString& id)
{
    const QString plain = id.section(u'@', 0, 0);
    const qsizetype dot = plain.lastIndexOf(u'.');
    if (dot < 0)
        return QString();
    return countryCode(plain.mid(dot + 1).toLower());
}

Matcher::Matcher(QVector<Known> channels)
    : m_channels(std::move(channels))
{
    m_byId.reserve(m_channels.size());
    for (int i = 0; i < int(m_channels.size()); ++i)
    {
        const Known& k = m_channels[i];
        m_byId.insert(k.id.toLower(), i);
        QSet<QString> keys, keysWithoutTv;
        QStringList names = k.altNames;
        names << k.name;
        // (what a match needs of the channels of a name, made once: in the
        // order of the database, the first of each country; sorted and
        // looked through at every match, thousands of channels of one name
        // took hours)
        auto add = [&](QHash<QString, Named>& table, QSet<QString>& seen, const QString& key) {
            if (seen.contains(key))
                return;
            seen.insert(key);
            Named& named = table[key];
            const QString& c = k.country;
            if (named.first < 0)
                named.first = i;
            else if (c != m_channels[named.first].country)
                named.oneCountry = false;
            ++named.count;
            if (!c.isEmpty() && !named.byCountry.contains(c))
                named.byCountry.insert(c, i);
        };
        for (const QString& name : std::as_const(names))
        {
            int plus = 0;
            bool tv = false;
            const QString key = matchName(name, nullptr, &plus, &tv);
            if (key.isEmpty())
                continue;
            const QString later = plus > 0 ? QStringLiteral("+%1").arg(plus) : QString();
            add(m_byName, keys, key + later);
            // "Arte TV" also as "arte": a list's "Arte" finds it
            if (tv)
                add(m_byNameWithoutTv, keysWithoutTv, key.chopped(2) + later);
        }
    }
}

const Matcher::Named* Matcher::named(const QString& key, const QString& keyWithoutTv) const
{
    // the name as it is ("Channel 5 TV HD": "channel5tv"); without its "TV"
    // when it ends so ("Rai 1 TV": "Rai 1"); a channel whose name ends in
    // "TV" without it ("Arte": "Arte TV"). The two sides fold the same way:
    // the "TV" is never dropped on one of them only
    auto it = m_byName.constFind(key);
    if (it != m_byName.cend())
        return &*it;
    if (!keyWithoutTv.isEmpty())
    {
        it = m_byName.constFind(keyWithoutTv);
        if (it != m_byName.cend())
            return &*it;
    }
    it = m_byNameWithoutTv.constFind(key);
    if (it != m_byNameWithoutTv.cend())
        return &*it;
    return nullptr;
}

Match Matcher::match(const QString& tvgId, const QString& name, const QString& listCountry,
                     const QString& preferred) const
{
    Match m;
    // its own guide id, when iptv-org has it ("BBCOne.uk", "BBCOne.uk@HD")
    if (!tvgId.isEmpty())
    {
        const auto it = m_byId.constFind(tvgId.section(u'@', 0, 0).toLower());
        if (it != m_byId.cend())
        {
            m.id = m_channels[*it].id;
            m.country = m_channels[*it].country;
            m.byId = true;
            return m;
        }
    }
    QString nameCountry;
    int plus = 0;
    bool tv = false;
    const QString key = matchName(name, &nameCountry, &plus, &tv);
    if (key.isEmpty())
        return m;
    const QString keyWithoutTv = tv ? key.chopped(2) : QString();
    // the country the list says: by the guide id, the name, the list
    QString hint = countryOfId(tvgId);
    if (hint.isEmpty())
        hint = nameCountry;
    if (hint.isEmpty())
        hint = listCountry.toUpper();
    // a "+1" channel: the one of that name, else the one it repeats
    const Named* candidates = nullptr;
    int shift = 0;
    if (plus > 0)
    {
        const QString later = QStringLiteral("+%1").arg(plus);
        candidates = named(key + later, keyWithoutTv.isEmpty() ? QString() : keyWithoutTv + later);
    }
    if (!candidates)
    {
        candidates = named(key, keyWithoutTv);
        if (!candidates)
            return m;
        shift = plus * 3600;
    }
    // (the first in the order of the database, not of the hash)
    auto inCountry = [&](const QString& country) -> int {
        return country.isEmpty() ? -1 : candidates->byCountry.value(country, -1);
    };
    int chosen = inCountry(hint);
    if (chosen < 0 && hint.isEmpty())
        chosen = inCountry(preferred.toUpper());
    // one channel of that name, or all in one country
    if (chosen < 0 && (candidates->count == 1 || (candidates->oneCountry && hint.isEmpty())))
        chosen = candidates->first;
    if (chosen < 0)
        return m;
    m.id = m_channels[chosen].id;
    m.country = m_channels[chosen].country;
    m.shiftSecs = shift;
    return m;
}

QStringList pickGuides(const QJsonArray& catalogSources, const QHash<QString, int>& matchedByCountry,
                       const QStringList& skip, int most, int perCountry)
{
    QVector<QPair<int, QString>> countries;
    int matched = 0;
    for (auto it = matchedByCountry.cbegin(); it != matchedByCountry.cend(); ++it)
    {
        countries.append({ it.value(), it.key() });
        matched += it.value();
    }
    std::sort(countries.begin(), countries.end(), [](const auto& a, const auto& b) {
        return a.first != b.first ? a.first > b.first : a.second < b.second;
    });
    // the hosts in the order they are preferred: iptv-epg.org names the
    // channels by iptv-org's ids, the others by their names (epgshare01's
    // display names are the channels' own)
    static const char* const hosts[] = { "iptv-epg.org", "epgshare01.online", "open-epg.com", "epg.pw" };
    auto rank = [](const QString& url) -> int {
        const QString host = QUrl(url).host().toLower();
        for (int i = 0; i < 4; ++i)
            if (host == QLatin1String(hosts[i]) || host.endsWith(QLatin1Char('.') + QLatin1String(hosts[i])))
                return i;
        return -1;
    };
    // the guides that can be offered, read and ranked once (for each country
    // they were again: thousands of guides of hosts it does not rank took
    // minutes)
    struct Offered { int rank; QSet<QString> countries; int countryCount; QString url; };
    QVector<Offered> guides;
    {
        const QSet<QString> skipped(skip.cbegin(), skip.cend());
        for (const QJsonValue v : catalogSources)
        {
            const QJsonObject o = v.toObject();
            const QString url = o.value(QLatin1String("url")).toString();
            if (o.value(QLatin1String("kind")).toString() != QLatin1String("epg")
                || o.value(QLatin1String("status")).toString() == QLatin1String("not answering")
                || skipped.contains(url))
                continue;
            const int r = rank(url);
            if (r < 0)
                continue;
            const QJsonArray covers = o.value(QLatin1String("countries")).toArray();
            Offered g { r, {}, int(covers.size()), url };
            for (const QJsonValue c : covers)
                g.countries.insert(c.toString());
            guides.append(std::move(g));
        }
    }
    QStringList picked;
    for (const auto& country : std::as_const(countries))
    {
        if (int(picked.size()) >= most)
            break;
        // (a country of one channel out of many: not worth a guide)
        if (country.second.isEmpty() || (country.first < 2 && matched > 4))
            continue;
        // this country's guides, the preferred first (of one host, the one of
        // this country alone before the one of a region)
        struct Offer { int rank; int countries; QString url; };
        QVector<Offer> offers;
        for (const Offered& g : std::as_const(guides))
            if (g.countries.contains(country.second) && !picked.contains(g.url))
                offers.append({ g.rank, g.countryCount, g.url });
        std::sort(offers.begin(), offers.end(), [](const Offer& a, const Offer& b) {
            return a.rank != b.rank ? a.rank < b.rank : a.countries < b.countries;
        });
        // (one of each host: the second is for the names the first misses)
        QSet<int> hostsTaken;
        int taken = 0;
        for (const Offer& offer : std::as_const(offers))
        {
            if (taken >= perCountry || int(picked.size()) >= most)
                break;
            if (hostsTaken.contains(offer.rank))
                continue;
            hostsTaken.insert(offer.rank);
            picked << offer.url;
            ++taken;
        }
    }
    return picked;
}

}
}
