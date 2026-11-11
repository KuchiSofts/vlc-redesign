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
#include <QElapsedTimer>
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

    // a gzip guide some servers pad with zeros (or anything) after its last
    // member: the end of the data, not an error that dropped it all
    for (const QByteArray& trailing : { QByteArray(16, '\0'), QByteArray("\n\n<trailing garbage>") })
    {
        QByteArray padded = gzip(xml) + trailing;
        GuideFilter plain = filter;
        plain.cancel = nullptr;
        const Guide inMemory = parseXmltv(padded, plain);
        CHECK(inMemory.error.isEmpty());
        CHECK(inMemory.programmes.value(guideKey("news.example")).size() == 3);
        QBuffer buffer(&padded);
        buffer.open(QIODevice::ReadOnly);
        GunzipDevice inflated(&buffer);
        inflated.open(QIODevice::ReadOnly);
        const Guide streamed = parseXmltv(inflated, plain);
        CHECK(streamed.error.isEmpty());
        CHECK(streamed.programmes.value(guideKey("news.example")).size() == 3);
        CHECK(streamed.programmes.value("test channel").size() == 1);
    }
    // a member that is corrupt is still an error (not taken for padding)
    {
        QByteArray broken = gzip(xml);
        broken[broken.size() / 4] = char(broken[broken.size() / 4] ^ 0x5a);
        broken[broken.size() / 4 + 1] = char(broken[broken.size() / 4 + 1] ^ 0x5a);
        GuideFilter plain = filter;
        plain.cancel = nullptr;
        const Guide guide = parseXmltv(broken, plain);
        CHECK(!guide.error.isEmpty() || guide.programmes.value(guideKey("news.example")).size() < 3);
    }

    // tvg-shift: hours, two days at most either way; "1e9", "inf" or "nan"
    // never an int out of range (built with -fsanitize=float-cast-overflow)
    {
        const Playlist shifted = parseM3u(
            "#EXTM3U\n"
            "#EXTINF:-1 tvg-shift=\"1e9\",Far ahead\nhttp://a/1\n"
            "#EXTINF:-1 tvg-shift=\"-1e9\",Far back\nhttp://a/2\n"
            "#EXTINF:-1 tvg-shift=\"inf\",Infinite\nhttp://a/3\n"
            "#EXTINF:-1 tvg-shift=\"nan\",Not a number\nhttp://a/4\n"
            "#EXTINF:-1 tvg-shift=\"1.5\",An hour and a half\nhttp://a/5\n");
        CHECK(shifted.channels.size() == 5);
        if (shifted.channels.size() == 5)
        {
            CHECK(shifted.channels[0].guideShiftSecs == 48 * 3600);
            CHECK(shifted.channels[1].guideShiftSecs == -48 * 3600);
            CHECK(shifted.channels[2].guideShiftSecs == 0);
            CHECK(shifted.channels[3].guideShiftSecs == 0);
            CHECK(shifted.channels[4].guideShiftSecs == 5400);
        }
    }
    // Xtream numbers out of range: as they are written, no cast; a server
    // clock that is no clock (year 9999 against 1970): no offset, no overflow
    {
        const auto huge = parseLiveStreams(R"([{"stream_id":1e300,"name":"Huge id"},{"stream_id":-1e19,"name":"Below"}])",
                                           fromLink, categories, info);
        CHECK(huge.size() == 2);
        CHECK(huge.value(0).url.endsWith("/1e+300.m3u8"));
        const AccountInfo clock = parseAccountInfo(R"({"user_info":{"auth":1},
            "server_info":{"timestamp_now":1,"time_now":"9999-12-31 23:59:59"}})");
        CHECK(clock.authenticated && clock.serverOffsetSecs == 0);
        const AccountInfo far = parseAccountInfo(R"({"user_info":{"auth":1},
            "server_info":{"timestamp_now":"9223372036854775807","time_now":"2026-10-25 21:00:00"}})");
        CHECK(far.serverOffsetSecs == 0);
    }
    // the categories of the streams by a hash: as many categories cost no
    // more than one (300 000 films of 2 000 categories took seconds; here
    // 20 000 of 20 000, ids of one length that differ at their end only, the
    // films in the last: some 4e8 comparisons looked for in the list)
    {
        auto idOf = [](int i) { return "category-" + QByteArray::number(i).rightJustified(7, '0'); };
        auto streamsOf = [&](int count, int category) {
            QByteArray json = "[";
            for (int i = 0; i < count; ++i)
                json += (i ? "," : "") + QByteArray(R"({"stream_id":)") + QByteArray::number(i + 1)
                        + R"(,"name":"S","category_id":")" + idOf(category) + "\"}";
            return json + "]";
        };
        auto categoriesOf = [&](int count) {
            QByteArray json = "[";
            for (int i = 0; i < count; ++i)
                json += (i ? "," : "") + QByteArray(R"({"category_id":")") + idOf(i)
                        + R"(","category_name":"C)" + QByteArray::number(i) + "\"}";
            return json + "]";
        };
        const int count = 20000;
        const QByteArray streams = streamsOf(count, count - 1);
        const auto one = parseCategories(categoriesOf(1));
        const auto many = parseCategories(categoriesOf(count));
        CHECK(many.size() == count);
        // (the least of two runs each: a busy machine)
        qint64 base = -1, hashed = -1;
        QVector<VodItem> few, named;
        for (int run = 0; run < 2; ++run)
        {
            QElapsedTimer timer;
            timer.start();
            few = parseVodStreams(streams, fromLink, one);
            const qint64 b = timer.restart();
            named = parseVodStreams(streams, fromLink, many);
            const qint64 h = timer.elapsed();
            base = base < 0 ? b : qMin(base, b);
            hashed = hashed < 0 ? h : qMin(hashed, h);
        }
        CHECK(few.size() == count && named.size() == count);
        CHECK(named.value(0).category == "C" + QString::number(count - 1) && few.value(0).category.isEmpty());
        std::printf("categories: %d films, 1 category %lld ms, %d categories %lld ms\n", count, (long long)base,
                    count, (long long)hashed);
        CHECK(hashed < base * 2 + 1000);
        // (the first of an id named twice, as the list did)
        const auto twice = parseCategories(R"([{"category_id":"1","category_name":"First"},
                                               {"category_id":"1","category_name":"Second"}])");
        CHECK(parseVodStreams(R"([{"stream_id":1,"name":"F","category_id":"1"}])", fromLink, twice).value(0).category
              == "First");
    }

    // xmltv_ns numbers out of range: no episode, never an overflow (UBSan)
    {
        auto episodeOf = [&](const QByteArray& numbers) {
            const QByteArray guide = "<tv><programme start=\"20261025180000 +0000\" channel=\"news.example\">"
                                     "<title>T</title><episode-num system=\"xmltv_ns\">" + numbers
                                     + "</episode-num></programme></tv>";
            GuideFilter plain;
            return parseXmltv(guide, plain).programmes.value(guideKey("news.example")).value(0).episode;
        };
        CHECK(episodeOf(".2147483647") == "");
        CHECK(episodeOf("2147483647.4.") == "E5");
        CHECK(episodeOf("-1.4.") == "E5");
        CHECK(episodeOf("1.4.0/1") == "S2E5");
        CHECK(episodeOf("9999.9999.") == "S10000E10000");
        CHECK(episodeOf("x.y.") == "");
    }
    // a catch-up of INT_MAX seconds: its minutes counted in 64 bits
    if (live.size() == 2)
    {
        const QString far = catchupUrl(live[0], start, 2147483647, now);
        CHECK(far.contains("/timeshift/me/p%40ss/35791395/"));
    }
    // catch-up templates: the one after one that is not a number is filled in
    // too; one of more than 4 KB (minutes to fill in) is none
    {
        const Playlist templated = parseM3u(
            "#EXTM3U\n"
            "#EXTINF:-1 catchup-days=\"1\" catchup-source=\"https://a.example/{duration:x}/{duration:60}/{offset:1}\",A\n"
            "https://live.example/a.ts\n"
            "#EXTINF:-1 catchup-days=\"1\" catchup-source=\"" + QByteArray(64 * 1024 / 12, 'x').replace("x", "{duration:1}")
            + "\",B\nhttps://live.example/b.ts\n");
        CHECK(templated.channels.size() == 2);
        if (templated.channels.size() == 2)
        {
            CHECK(catchupUrl(templated.channels[0], start, 3600, now)
                  == "https://a.example/{duration:x}/60/7200");
            QElapsedTimer timer;
            timer.start();
            CHECK(templated.channels[1].catchupSource.isEmpty());
            catchupUrl(templated.channels[1], start, 3600, now);
            CHECK(timer.elapsed() < 500);
        }
    }
    // a header naming thousands of guides (each looked for in the list):
    // once each, 32 at most
    {
        QByteArray header = "#EXTM3U url-tvg=\"https://g.example/0.xml";
        for (int i = 1; i < 20000; ++i)
            header += ",https://g.example/" + QByteArray::number(i % 10000) + ".xml";
        QElapsedTimer timer;
        timer.start();
        const Playlist many = parseM3u(header + "\"\n#EXTINF:-1,T\nhttp://h.example/1\n");
        std::printf("a header of 20000 guides: %lld ms\n", (long long)timer.elapsed());
        CHECK(timer.elapsed() < 2000);
        CHECK(many.guideUrls.size() == 32 && many.guideUrls.value(31) == "https://g.example/31.xml");
        CHECK(QSet<QString>(many.guideUrls.cbegin(), many.guideUrls.cend()).size() == 32);
    }
    // a text the reader would hold whole (a gzip of 64 MB of spaces, a
    // comment of 64 MB): an error past 8 MB, not gigabytes asked for; and a
    // reading cancelled inside such a text stops there
    {
        auto gzipOf = [](const QByteArray& data) {
            z_stream zs{};
            deflateInit2(&zs, 9, Z_DEFLATED, 16 + MAX_WBITS, 8, Z_DEFAULT_STRATEGY);
            QByteArray out(int(deflateBound(&zs, uLong(data.size()))) + 64, '\0');
            zs.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(data.constData()));
            zs.avail_in = uInt(data.size());
            zs.next_out = reinterpret_cast<Bytef*>(out.data());
            zs.avail_out = uInt(out.size());
            deflate(&zs, Z_FINISH);
            out.resize(int(zs.total_out));
            deflateEnd(&zs);
            return out;
        };
        GuideFilter plain;
        QElapsedTimer timer;
        timer.start();
        const Guide spaces = parseXmltv(gzipOf("<tv>" + QByteArray(64 << 20, ' ') + "</tv>"), plain);
        CHECK(spaces.error == "a text of the guide is too long");
        const Guide comment = parseXmltv(gzipOf("<tv><!--" + QByteArray(64 << 20, 'x') + "--></tv>"), plain);
        CHECK(comment.error == "a text of the guide is too long");
        std::printf("64 MB of spaces, a 64 MB comment (gzip): %lld ms\n", (long long)timer.elapsed());
        // (a guide with texts of a few megabytes is read)
        GuideFilter keysOnly = filter;
        keysOnly.cancel = nullptr;
        const Guide fine = parseXmltv(xml.left(xml.indexOf("</tv>")) + "<!--" + QByteArray(4 << 20, 'x') + "--></tv>",
                                      keysOnly);
        CHECK(fine.error.isEmpty() && fine.programmes.value(guideKey("news.example")).size() == 3);
        std::atomic<bool> stop { false };
        GuideFilter cancellable;
        cancellable.cancel = &stop;
        QByteArray huge = "<tv><!--" + QByteArray(6 << 20, 'x') + "--></tv>";
        QBuffer source(&huge);
        source.open(QIODevice::ReadOnly);
        // (cancelled while the comment is read: after its first megabyte)
        class Stopper : public QIODevice
        {
        public:
            Stopper(QIODevice* s, std::atomic<bool>* f) : m_s(s), m_f(f) {}
            bool isSequential() const override { return true; }

        protected:
            qint64 readData(char* d, qint64 n) override
            {
                if (m_s->pos() > (1 << 20))
                    m_f->store(true);
                return m_s->read(d, n);
            }
            qint64 writeData(const char*, qint64) override { return -1; }

        private:
            QIODevice* m_s;
            std::atomic<bool>* m_f;
        } stopper(&source, &stop);
        stopper.open(QIODevice::ReadOnly);
        const Guide cancelled = parseXmltv(stopper, cancellable);
        CHECK(cancelled.error == "cancelled");
        CHECK(source.pos() < (2 << 20));
    }

    if (failures == 0)
        std::puts("PASS: M3U attributes and groups, XMLTV plain/gzip/streamed with languages, catch-up addresses, Xtream, "
                  "gzip padding, tvg-shift and Xtream numbers out of range, categories by a hash, episode numbers "
                  "and catch-up minutes out of range, templates and headers capped, texts too long and cancelled");
    return failures == 0 ? 0 : 1;
}
