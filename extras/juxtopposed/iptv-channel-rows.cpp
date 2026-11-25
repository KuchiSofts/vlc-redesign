/* Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "modules/gui/qt/iptv/iptv_channel_rows.hpp"

#include <QHash>

#include <cstdio>

static int failures = 0;
#define CHECK(condition) do { if (!(condition)) { \
    std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); ++failures; } } while (0)

int main()
{
    struct Channel
    {
        QString nameFolded;
        QString group;
        int number;
        bool adult;
    };
    const QString news = QStringLiteral("News");
    const QString arts = QStringLiteral("Arts");
    QVector<Channel> channels = {
        { QStringLiteral("local news"), news, 7, false },
        { QStringLiteral("evening news"), news, 12, false },
        { QStringLiteral("studio 7"), arts, 27, false },
        { QStringLiteral("restricted"), news, 7, true },
    };
    const QHash<QString, int> index = {
        { QStringLiteral("local"), 0 }, { QStringLiteral("evening"), 1 }, { QStringLiteral("studio"), 2 },
    };
    const auto rows = [&](const IptvChannelSearch& search, const QString& group = {}, const QStringList& keys = {}) {
        return iptvChannelRows(channels, group, keys, search, false,
                               [&](const QString& key) { return index.value(key, -1); });
    };

    const IptvChannelSearch seven(QStringLiteral(" 7 "));
    const IptvChannelSearch twelve(QStringLiteral("12"));
    const IptvChannelSearch byName(QStringLiteral(" NEWS "));
    const auto oldSeven = rows(seven);
    const auto oldTwelve = rows(twelve);
    const auto oldArts = rows(seven, arts);
    const auto oldNewsSeven = rows(seven, news);
    const auto oldByName = rows(byName);
    CHECK((oldSeven == QVector<int>{0, 2}));
    CHECK(oldTwelve == QVector<int>{1});
    CHECK(oldArts == QVector<int>{2});
    CHECK((oldByName == QVector<int>{0, 1}));

    channels[0].number = 12;
    CHECK(seven.rowsAfterRenumbering(oldSeven, [&]() { return rows(seven); }) == QVector<int>{2});
    CHECK((twelve.rowsAfterRenumbering(oldTwelve, [&]() { return rows(twelve); }) == QVector<int>{0, 1}));
    CHECK(seven.rowsAfterRenumbering(oldNewsSeven, [&]() { return rows(seven, news); }) == QVector<int>{});
    CHECK(!seven.rowsAfterRenumbering(oldArts, [&]() { return rows(seven, arts); }));
    CHECK(!byName.rowsAfterRenumbering(oldByName, [&]() { return rows(byName); }));

    // A different channel can take the old number without changing the result count.
    channels[1].number = 7;
    CHECK((seven.rowsAfterRenumbering(oldSeven, [&]() { return rows(seven); }) == QVector<int>{1, 2}));
    const QStringList recent = { QStringLiteral("studio"), QStringLiteral("missing"), QStringLiteral("evening") };
    CHECK((rows(seven, QStringLiteral("recent"), recent) == QVector<int>{2, 1}));
    return failures != 0;
}
