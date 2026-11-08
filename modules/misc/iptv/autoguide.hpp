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
#include <QJsonArray>
#include <QMultiHash>
#include <QString>
#include <QStringList>
#include <QVector>

// The automatic guide: the channels of a source without a guide matched to
// iptv-org's database of channels (https://iptv-org.github.io/api/), by
// their guide id, else by their name and country, and the free guides of
// the catalog that carry their countries picked for them. Pure functions,
// no Qt objects (and no regular expressions: see guide.cpp).
namespace iptv {
namespace autoguide {

// A channel of iptv-org's database: what the matching needs of it
struct Known
{
    QString id;          // "BBCOne.uk": the id iptv-epg.org's guides give it
    QString name;
    QStringList altNames;
    QString country;     // "GB", upper case
};

// iptv-org's channels.json (8 MB): the channels still on air, the fields
// kept
QVector<Known> parseIptvOrgChannels(const QByteArray& json);
// The kept fields as a small file of their own (the cache of a week), and
// read back
QByteArray compactChannels(const QVector<Known>& channels);
QVector<Known> readCompactChannels(const QByteArray& data);

// A channel's name folded for matching: lower case, accents off, the
// separators and the quality ("HD", "FHD", "4K", "(1080p)", "[Geo-blocked]")
// dropped; a country it names ("UK: ", "(US)") in country (ISO, upper case,
// "UK" as "GB"), a "+1" (the same channel an hour later) in plusHours
QString matchName(const QString& name, QString* country = nullptr, int* plusHours = nullptr);

// The country an iptv-org id ends with ("BBCOne.uk@HD" -> "GB"), or empty
QString countryOfId(const QString& id);

// What a channel was matched to: an iptv-org id, its country, and the
// shift of its guide (a "+1" channel matched to the one it repeats)
struct Match
{
    QString id;
    QString country;
    int shiftSecs = 0;
    bool byId = false;   // by its own guide id, not by its name
    bool isValid() const { return !id.isEmpty(); }
};

class Matcher
{
public:
    explicit Matcher(QVector<Known> channels);
    // listCountry: the country of the list (a catalog entry's), preferred:
    // the user's; a name several countries have is matched only with one
    // of them
    Match match(const QString& tvgId, const QString& name, const QString& listCountry,
                const QString& preferred) const;
    int size() const { return int(m_channels.size()); }

private:
    QVector<Known> m_channels;
    QHash<QString, int> m_byId;            // the id, lower case
    QMultiHash<QString, int> m_byName;     // matchName, with "+1" when it has one
};

// The guides of the catalog for the countries of the channels matched:
// for each country (the most channels first), up to perCountry of its
// guides: iptv-epg.org's first (it names the channels by iptv-org's ids),
// then epgshare01's, Open-EPG's, EPG.PW's (by their names); at most `most`
// in all; guides "not answering" and those in skip left out
QStringList pickGuides(const QJsonArray& catalogSources, const QHash<QString, int>& matchedByCountry,
                       const QStringList& skip, int most = 4, int perCountry = 2);

}
}
