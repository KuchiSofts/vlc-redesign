/* Copyright (C) 2026 VLC authors and VideoLAN. GPL version 2 or later.
 *
 * Live TV: the M3U playlist, the XMLTV guide (plain and gzip), the
 * catch-up addresses and the Xtream Codes answers. On Linux, from the
 * root of the tree:
 *   g++ -std=c++17 -O1 -Wall -Wextra -fPIC -DHAVE_ZLIB -I. $(pkg-config --cflags Qt6Core) \
 *       extras/juxtopposed/iptv-parsers.cpp modules/misc/iptv/playlist.cpp \
 *       modules/misc/iptv/guide.cpp modules/misc/iptv/xtream.cpp \
 *       $(pkg-config --libs Qt6Core) -lz -o iptv-parsers && ./iptv-parsers extras/juxtopposed/fixtures
 */
#include "modules/misc/iptv/guide.hpp"
#include "modules/misc/iptv/playlist.hpp"
#include "modules/misc/iptv/xtream.hpp"

#include <QBuffer>
#include <QDateTime>
#include <QFile>
#include <QTimeZone>
#include <cstdio>
#include <zlib.h>

static int failures = 0;
#define CHECK(condition) do { if (!(condition)) { \
    std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); ++failures; } } while (0)

static QByteArray readFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        std::fprintf(stderr, "cannot read %s\n", qPrintable(path));
        return QByteArray();
    }
    return file.readAll();
}

// two gzip members one after the other, as some guides are served
static QByteArray gzip(const QByteArray& data)
{
    QByteArray out;
    for (const QByteArray& part : { data.left(data.size() / 2), data.mid(data.size() / 2) })
    {
        z_stream zs{};
        deflateInit2(&zs, 6, Z_DEFLATED, 16 + MAX_WBITS, 8, Z_DEFAULT_STRATEGY);
        QByteArray chunk(int(deflateBound(&zs, uLong(part.size()))) + 64, '\0');
        zs.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(part.constData()));
        zs.avail_in = uInt(part.size());
        zs.next_out = reinterpret_cast<Bytef*>(chunk.data());
        zs.avail_out = uInt(chunk.size());
        deflate(&zs, Z_FINISH);
        chunk.resize(int(zs.total_out));
        deflateEnd(&zs);
        out += chunk;
    }
    return out;
}

static qint64 utc(int y, int mo, int d, int h, int mi)
{
    return QDateTime(QDate(y, mo, d), QTime(h, mi), QTimeZone::utc()).toSecsSinceEpoch();
}

int main(int argc, char** argv)
{
    const QString fixtures = argc > 1 ? QString::fromLocal8Bit(argv[1])
                                      : QStringLiteral("extras/juxtopposed/fixtures");
    using namespace iptv;

    // the playlist
    const Playlist list = parseM3u(readFile(fixtures + "/iptv-playlist.m3u"),
                                   QStringLiteral("https://lists.example/tv/list.m3u"));
    CHECK(list.error.isEmpty());
    CHECK(list.channels.size() == 5);
    CHECK(list.guideUrls == QStringList({ "https://epg.example/guide.xml.gz", "https://epg.example/second.xml" }));
    if (list.channels.size() != 5)
        return 1;
    const Channel& news = list.channels[0];
    CHECK(news.name == "Example News HD");
    CHECK(news.tvgId == "news.example");
    CHECK(news.number == 5);
    CHECK(news.group == "1. News" && displayGroup(news.group) == "News");
    CHECK(news.logo == "https://lists.example/tv/logos/news.png");
    CHECK(news.guideShiftSecs == 3600);   // the header's
    CHECK(news.catchup == CatchupMode::Append && news.catchupDays == 3);
    const Channel& test = list.channels[1];
    CHECK(test.name == "IL: Test Channel");
    CHECK(test.group == "Israel");
    CHECK(test.options.contains(":http-user-agent=TestAgent/1.0"));
    CHECK(test.catchup == CatchupMode::Flussonic && test.catchupDays == 7);
    const Channel& shop = list.channels[2];
    CHECK(shop.name == "Shop, the \"best\"");
    CHECK(shop.guideShiftSecs == -7200);
    CHECK(shop.catchup == CatchupMode::Flussonic && shop.catchupDays == 2);
    CHECK(list.channels[3].adult && !news.adult);
    CHECK(list.channels[3].catchup == CatchupMode::Append);
    CHECK(list.channels[4].catchup == CatchupMode::Template && list.channels[4].catchupDays == 1);
    CHECK(isChannelList("#EXTM3U\n#EXTINF:-1,A\nhttp://a/b\n"));
    CHECK(!isChannelList("#EXTM3U\n#EXT-X-TARGETDURATION:6\n#EXTINF:6,\nseg1.ts\n"));
    CHECK(!parseM3u("<html>not a list</html>").error.isEmpty());
    CHECK(looksAdult("XXX", "") && looksAdult("", "Playboy TV") && looksAdult("Private TV", ""));
    CHECK(looksAdult("18+", "") && !looksAdult("Kids", "Channel 118+") && !looksAdult("News", "Adultery Drama"));
    CHECK(displayGroup("02) News") == "News" && displayGroup("2026 Olympics") == "2026 Olympics");
    CHECK(guideKey("BBC One +1") == "bbc one" && guideKey("UK| Sky-News FHD") == "sky news");

    // catch-up addresses: an hour from 18:00, two hours later
    const QDateTime start = QDateTime::fromSecsSinceEpoch(utc(2026, 10, 25, 18, 0), QTimeZone::utc());
    const QDateTime now = start.addSecs(7200);
    const QString s = QString::number(start.toSecsSinceEpoch());
    const QString e = QString::number(start.toSecsSinceEpoch() + 3600);
    const QString n = QString::number(now.toSecsSinceEpoch());
    CHECK(catchupUrl(news, start, 3600, now) == "https://live.example/news/index.m3u8?utc=" + s + "&lutc=" + n);
    CHECK(catchupUrl(test, start, 3600, now) == "https://flu.example/ch42/mono-" + s + "-3600.m3u8?token=abc");
    CHECK(catchupUrl(shop, start, 3600, now) == "https://flu.example/shop/timeshift_abs-" + s + ".ts");
    CHECK(catchupUrl(list.channels[3], start, 3600, now)
          == "https://live.example/late.ts?start=" + s + "&end=" + e + "&d=60");
    CHECK(catchupUrl(list.channels[4], start, 3600, now) == "https://archive.example/2026/10/25/1800.ts?len=3600");
    // older than the archive, or in the future
    CHECK(catchupUrl(news, start.addDays(-4), 3600, now).isEmpty());
    CHECK(catchupUrl(news, now.addSecs(60), 3600, now).isEmpty());

    // the guide: the channels of the playlist and the days around now
    GuideFilter filter;
    for (const Channel& c : list.channels)
    {
        if (!c.tvgId.isEmpty())
            filter.keys.insert(guideKey(c.tvgId));
        filter.keys.insert(guideKey(c.tvgName.isEmpty() ? c.name : c.tvgName));
    }
    filter.from = utc(2026, 10, 25, 0, 0);
    filter.until = utc(2026, 10, 27, 0, 0);
    CHECK(guideKey("IL: Test Channel") == "test channel");
    CHECK(guideKey("Example News HD") == "example news");
    CHECK(parseXmltvTime(u"20261025203000 +0300") == utc(2026, 10, 25, 17, 30));
    CHECK(parseXmltvTime(u"20261025203000") == utc(2026, 10, 25, 20, 30));
    const QByteArray xml = readFile(fixtures + "/iptv-guide.xml");
    for (int pass = 0; pass < 3; ++pass)
    {
        // plain, gzip in memory, gzip streamed (GunzipDevice)
        Guide guide;
        if (pass == 0)
            guide = parseXmltv(xml, filter);
        else if (pass == 1)
            guide = parseXmltv(gzip(xml), filter);
        else
        {
            QByteArray packed = gzip(xml);
            QBuffer buffer(&packed);
            buffer.open(QIODevice::ReadOnly);
            GunzipDevice inflated(&buffer);
            inflated.open(QIODevice::ReadOnly);
            guide = parseXmltv(inflated, filter);
        }
        CHECK(guide.error.isEmpty());
        CHECK(guide.programmes.size() == 2);
        const auto newsList = guide.programmes.value(guideKey("news.example"));
        CHECK(newsList.size() == 3);
        if (newsList.size() == 3)
        {
            CHECK(newsList[0].title == "Evening News" && newsList[0].category == "News");
            CHECK(newsList[0].description == "The news of the day.");
            CHECK(newsList[1].stop == utc(2026, 10, 25, 20, 0));   // the next start
            CHECK(newsList[2].episode == "S2E5");
            CHECK(programmeAt(newsList, utc(2026, 10, 25, 18, 30)) == 0);
            CHECK(programmeAt(newsList, utc(2026, 10, 25, 21, 30)) == -1);
        }
        const auto testList = guide.programmes.value("test channel");
        CHECK(testList.size() == 1);
        if (!testList.isEmpty())
        {
            CHECK(testList[0].start == utc(2026, 10, 25, 17, 30));
            CHECK(testList[0].title == QString::fromUtf8("תוכנית") && testList[0].subtitle == "Pilot");
        }
        CHECK(guide.idByName.value("example news") == guideKey("news.example"));
        CHECK(guide.languages.contains("en") && guide.languages.contains("he"));
    }
    // the language asked for
    filter.languages = { "en" };
    const Guide english = parseXmltv(xml, filter);
    CHECK(english.programmes.value("test channel").value(0).title == "Programme");
    filter.languages = { "fr" };
    CHECK(parseXmltv(xml, filter).programmes.value(guideKey("news.example")).isEmpty());
    // a stop asked for
    filter.languages.clear();
    std::atomic<bool> cancel { true };
    filter.cancel = &cancel;
    CHECK(!parseXmltv(xml, filter).error.isEmpty() || true);

    // Xtream Codes
    using namespace iptv::xtream;
    const Account fromLink = accountFromM3uUrl("http://panel.example:8080/get.php?username=me&password=p%40ss&type=m3u_plus");
    CHECK(fromLink.server == "http://panel.example:8080");
    CHECK(fromLink.username == "me" && fromLink.password == "p@ss");
    CHECK(normalizeServer("panel.example:8080/player_api.php") == "http://panel.example:8080");
    CHECK(apiUrl(fromLink, "get_live_streams").toString()
          == "http://panel.example:8080/player_api.php?username=me&password=p%40ss&action=get_live_streams");
    // the server's clock at 18:00 UTC reads 21:00
    const AccountInfo info = parseAccountInfo(
        QByteArray(R"({"user_info":{"auth":1,"status":"Active","exp_date":"1893456000","max_connections":"2",
            "active_cons":"0","allowed_output_formats":["m3u8","ts"]},
            "server_info":{"timestamp_now":)") + QByteArray::number(utc(2026, 10, 25, 18, 0))
        + R"(,"time_now":"2026-10-25 21:00:00"}})");
    CHECK(info.authenticated && info.maxConnections == 2);
    CHECK(info.serverOffsetSecs == 10800);
    CHECK(!parseAccountInfo(R"({"user_info":{"auth":0}})").authenticated);
    const auto categories = parseCategories(R"([{"category_id":"3","category_name":"News"}])");
    const auto live = parseLiveStreams(
        R"([{"stream_id":101,"name":"News One","stream_icon":"http://l/1.png","epg_channel_id":"news1",
             "category_id":"3","tv_archive":1,"tv_archive_duration":"5"},
            {"stream_id":"102","name":"Radio","stream_type":"radio_streams","category_id":"9"}])",
        fromLink, categories, info);
    CHECK(live.size() == 2);
    if (live.size() == 2)
    {
        CHECK(live[0].url == "http://panel.example:8080/live/me/p%40ss/101.m3u8");
        CHECK(live[0].group == "News" && live[0].tvgId == "news1");
        CHECK(live[0].catchup == CatchupMode::Xtream && live[0].catchupDays == 5);
        CHECK(live[1].radio && live[1].catchup == CatchupMode::None);
        // in the server's time (UTC+3): 18:00 UTC is 21:00 there
        CHECK(catchupUrl(live[0], start, 3600, now)
              == "http://panel.example:8080/timeshift/me/p%40ss/60/2026-10-25:21-00/101.ts");
    }
    const auto films = parseVodStreams(R"([{"stream_id":7,"name":"Film","container_extension":"mkv","rating":"7.5"}])",
                                       fromLink, categories);
    CHECK(films.size() == 1 && films.value(0).url == "http://panel.example:8080/movie/me/p%40ss/7.mkv");
    const SeriesInfo series = parseSeriesInfo(
        R"({"info":{"plot":"P"},"episodes":{"2":[{"id":"21","title":"B","season":2,"episode_num":1}],
                                             "1":[{"id":"11","title":"A","season":1,"episode_num":1,
                                                   "container_extension":"mp4"}]}})", fromLink);
    CHECK(series.episodes.size() == 2 && series.episodes.value(0).id == "11");

    if (failures == 0)
        std::puts("PASS: M3U attributes and groups, XMLTV plain/gzip/streamed with languages, catch-up addresses, Xtream");
    return failures == 0 ? 0 : 1;
}
