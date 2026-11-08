/* Copyright (C) 2026 VLC authors and VideoLAN. GPL version 2 or later.
 *
 * Live TV: the automatic guide (modules/misc/iptv/autoguide.cpp): channel
 * names folded for matching, a list's channels matched to iptv-org's
 * database by their guide id, else their name and country, and the free
 * guides of the catalog picked for their countries. The fixtures
 * (fixtures/iptv-autoguide) have made-up channels in the shapes of
 * iptv-org's channels.json and of the catalog. On Linux, from the root of
 * the tree:
 *   g++ -std=c++17 -O1 -Wall -Wextra -fPIC -I. $(pkg-config --cflags Qt6Core) \
 *       extras/juxtopposed/iptv-autoguide.cpp modules/misc/iptv/autoguide.cpp \
 *       $(pkg-config --libs Qt6Core) -o iptv-autoguide && ./iptv-autoguide extras/juxtopposed/fixtures/iptv-autoguide
 */
#include "modules/misc/iptv/autoguide.hpp"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <cstdio>

using namespace iptv::autoguide;

static int checks = 0;
static int failures = 0;
#define CHECK(condition) do { ++checks; if (!(condition)) { \
    std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); ++failures; } } while (0)

static QString fixtures;

static QByteArray fixture(const char *name)
{
    QFile file(fixtures + QLatin1Char('/') + QLatin1String(name));
    if (!file.open(QIODevice::ReadOnly))
    {
        std::fprintf(stderr, "cannot read %s\n", qPrintable(file.fileName()));
        ++failures;
        return {};
    }
    return file.readAll();
}

static void testNames()
{
    QString country;
    int plus = -1;
    CHECK(matchName(QStringLiteral("UK: Open Movie One HD"), &country, &plus) == QLatin1String("openmovieone"));
    CHECK(country == QLatin1String("GB") && plus == 0);
    CHECK(matchName(QStringLiteral("Open Movie One (1080p) [Geo-blocked]"), &country) == QLatin1String("openmovieone")
          && country.isEmpty());
    CHECK(matchName(QStringLiteral("|DE| Demo Kids FHD"), &country) == QLatin1String("demokids") && country == QLatin1String("DE"));
    CHECK(matchName(QStringLiteral("DE - Demo Kids"), &country) == QLatin1String("demokids") && country == QLatin1String("DE"));
    CHECK(matchName(QStringLiteral("Sample Weather (FR)"), &country) == QLatin1String("sampleweather") && country == QLatin1String("FR"));
    CHECK(matchName(QStringLiteral("Démo Kinder")) == QLatin1String("demokinder"));
    CHECK(matchName(QStringLiteral("Sample Sport +1"), nullptr, &plus) == QLatin1String("samplesport") && plus == 1);
    CHECK(matchName(QStringLiteral("Sample Sport+2 HD"), nullptr, &plus) == QLatin1String("samplesport") && plus == 2);
    CHECK(matchName(QStringLiteral("Canal+ Demo"), nullptr, &plus) == QLatin1String("canalplusdemo") && plus == 0);
    CHECK(matchName(QStringLiteral("Salt & Pepper TV")) == QLatin1String("saltandpeppertv"));
    // (words that are names, not quality or countries)
    CHECK(matchName(QStringLiteral("Channel TV"), &country) == QLatin1String("channeltv") && country.isEmpty());
    CHECK(matchName(QStringLiteral("TF1"), &country) == QLatin1String("tf1") && country.isEmpty());
    CHECK(matchName(QStringLiteral("HD")) == QLatin1String("hd"));
    CHECK(matchName(QStringLiteral("חדשות דמו")) == QStringLiteral("חדשותדמו"));
    CHECK(countryOfId(QStringLiteral("OpenMovieOne.uk@HD")) == QLatin1String("GB"));
    CHECK(countryOfId(QStringLiteral("DemoKids.de")) == QLatin1String("DE"));
    CHECK(countryOfId(QStringLiteral("NoCountry")).isEmpty());
}

static void testMatcher()
{
    const QVector<Known> known = parseIptvOrgChannels(fixture("iptv-org-channels.json"));
    // the closed one and the one without an id left out; "UK" read as "GB"
    CHECK(known.size() == 8);
    CHECK(known.value(0).id == QLatin1String("OpenMovieOne.uk") && known.value(0).country == QLatin1String("GB")
          && known.value(0).altNames == QStringList { QStringLiteral("OMO") });
    // the cache of a week: the same channels back
    const QVector<Known> cached = readCompactChannels(compactChannels(known));
    CHECK(cached.size() == known.size() && cached.value(2).altNames == known.value(2).altNames
          && cached.value(7).country == known.value(7).country);

    const Matcher matcher(known);
    CHECK(matcher.size() == 8);
    // its own guide id (a feed of it too)
    Match m = matcher.match(QStringLiteral("OpenMovieOne.uk@SD"), QStringLiteral("whatever"), {}, {});
    CHECK(m.id == QLatin1String("OpenMovieOne.uk") && m.byId && m.country == QLatin1String("GB"));
    // a name two countries have: the one of the list, of its name, of the user
    m = matcher.match({}, QStringLiteral("Open Movie One"), QStringLiteral("GB"), QStringLiteral("US"));
    CHECK(m.id == QLatin1String("OpenMovieOne.uk") && !m.byId);
    m = matcher.match({}, QStringLiteral("US: Open Movie One HD"), {}, QStringLiteral("GB"));
    CHECK(m.id == QLatin1String("OpenMovieOne.us"));
    m = matcher.match(QStringLiteral("unknown.uk"), QStringLiteral("Open Movie One"), {}, {});
    CHECK(m.id == QLatin1String("OpenMovieOne.uk"));
    m = matcher.match({}, QStringLiteral("Open Movie One"), {}, QStringLiteral("US"));
    CHECK(m.id == QLatin1String("OpenMovieOne.us"));
    // and nothing to tell them apart: none rather than a wrong one
    CHECK(!matcher.match({}, QStringLiteral("Open Movie One"), {}, QStringLiteral("IL")).isValid());
    // a name of one channel, its other names, accents
    CHECK(matcher.match({}, QStringLiteral("Demo News"), {}, {}).id == QLatin1String("DemoNews.il"));
    CHECK(matcher.match({}, QStringLiteral("חדשות דמו HD"), {}, {}).id == QLatin1String("DemoNews.il"));
    CHECK(matcher.match({}, QStringLiteral("Demo Kinder"), {}, {}).id == QLatin1String("DemoKids.de"));
    CHECK(matcher.match({}, QStringLiteral("OMO"), QStringLiteral("GB"), {}).id == QLatin1String("OpenMovieOne.uk"));
    // a list of another country: a name of one channel still matches
    CHECK(matcher.match({}, QStringLiteral("Demo News"), QStringLiteral("US"), {}).id == QLatin1String("DemoNews.il"));
    // "+1": the channel of that name, else the one it repeats an hour later
    m = matcher.match({}, QStringLiteral("Sample Sport +1"), {}, {});
    CHECK(m.id == QLatin1String("SampleSportPlus1.fr") && m.shiftSecs == 0);
    m = matcher.match({}, QStringLiteral("Demo Kids +1"), {}, {});
    CHECK(m.id == QLatin1String("DemoKids.de") && m.shiftSecs == 3600);
    CHECK(matcher.match({}, QStringLiteral("Canal+ Demo HD"), {}, {}).id == QLatin1String("CanalDemo.fr"));
    // off the air, or unknown: none
    CHECK(!matcher.match({}, QStringLiteral("Old Demo"), {}, {}).isValid());
    CHECK(!matcher.match({}, QStringLiteral("Nothing Like It"), {}, {}).isValid());
    CHECK(!matcher.match({}, QString(), {}, {}).isValid());
}

static void testGuides()
{
    const QJsonArray catalog = QJsonDocument::fromJson(fixture("catalog-guides.json")).object()
                                   .value(QLatin1String("sources")).toArray();
    // per country iptv-epg.org first (iptv-org's ids), a country's own before
    // a region's, then another host (names); "not answering" left out; a
    // country of one channel out of many skipped
    QHash<QString, int> byCountry { { QStringLiteral("GB"), 5 }, { QStringLiteral("FR"), 3 }, { QStringLiteral("IL"), 1 } };
    QStringList picked = pickGuides(catalog, byCountry, {}, 4, 2);
    CHECK(picked == (QStringList { QStringLiteral("https://iptv-epg.org/files/epg-gb.xml.gz"),
                                   QStringLiteral("https://www.open-epg.com/files/unitedkingdom1.xml.gz"),
                                   QStringLiteral("https://iptv-epg.org/files/epg-region.xml.gz"),
                                   QStringLiteral("https://epgshare01.online/epgshare01/epg_ripper_FR1.xml.gz") }));
    // one a country: the preferred
    picked = pickGuides(catalog, byCountry, {}, 3, 1);
    CHECK(picked == (QStringList { QStringLiteral("https://iptv-epg.org/files/epg-gb.xml.gz"),
                                   QStringLiteral("https://iptv-epg.org/files/epg-region.xml.gz") }));
    // the guides the source has already: the next ones
    picked = pickGuides(catalog, byCountry, { QStringLiteral("https://iptv-epg.org/files/epg-gb.xml.gz") }, 4, 1);
    CHECK(picked.value(0) == QLatin1String("https://iptv-epg.org/files/epg-region.xml.gz"));
    CHECK(picked.value(1) == QLatin1String("https://epgshare01.online/epgshare01/epg_ripper_FR1.xml.gz"));
    // at most `most`; few channels: every country counts
    CHECK(pickGuides(catalog, byCountry, {}, 1, 2).size() == 1);
    picked = pickGuides(catalog, { { QStringLiteral("IL"), 1 } }, {}, 4, 2);
    CHECK(picked == QStringList { QStringLiteral("https://iptv-epg.org/files/epg-il.xml.gz") });
    // no guide of that country, nothing matched: nothing
    CHECK(pickGuides(catalog, { { QStringLiteral("JP"), 9 } }, {}, 4, 2).isEmpty());
    CHECK(pickGuides(catalog, {}, {}, 4, 2).isEmpty());
}

int main(int argc, char **argv)
{
    fixtures = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral("extras/juxtopposed/fixtures/iptv-autoguide");
    testNames();
    testMatcher();
    testGuides();
    std::printf("TV automatic guide: %d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
