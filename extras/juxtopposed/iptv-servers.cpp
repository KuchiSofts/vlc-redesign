/* Copyright (C) 2026 VLC authors and VideoLAN. GPL version 2 or later.
 *
 * Live TV: the user's servers and tuners (modules/misc/iptv/servers.cpp):
 * the addresses they are asked, their answers read from saved ones
 * (fixtures/iptv-servers, in the shapes their APIs document and their
 * open-source clients read), and the playlist and guide written of them,
 * read back by the service's own M3U and XMLTV parsers. On Linux, from the
 * root of the tree:
 *   g++ -std=c++17 -O1 -Wall -Wextra -fPIC -DHAVE_ZLIB -I. $(pkg-config --cflags Qt6Core) \
 *       extras/juxtopposed/iptv-servers.cpp modules/misc/iptv/servers.cpp \
 *       modules/misc/iptv/playlist.cpp modules/misc/iptv/guide.cpp \
 *       $(pkg-config --libs Qt6Core) -lz -o iptv-servers && ./iptv-servers extras/juxtopposed/fixtures/iptv-servers
 */
#include "modules/misc/iptv/guide.hpp"
#include "modules/misc/iptv/playlist.hpp"
#include "modules/misc/iptv/servers.hpp"

#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>
#include <cstdio>

using namespace iptv::servers;
using iptv::Guide;
using iptv::GuideFilter;
using iptv::Playlist;
using iptv::guideKey;
using iptv::parseM3u;
using iptv::parseXmltv;

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

static QString item(const QUrl &url, const char *key)
{
    return QUrlQuery(url).queryItemValue(QLatin1String(key), QUrl::FullyDecoded);
}

static const qint64 T0 = 1794200400;   // 2026-11-09 05:00 UTC

static void testCommon()
{
    CHECK(baseUrl(QStringLiteral("192.168.1.5"), 8096) == QLatin1String("http://192.168.1.5:8096"));
    CHECK(baseUrl(QStringLiteral(" http://jelly.lan:8920/ "), 8096) == QLatin1String("http://jelly.lan:8920"));
    CHECK(baseUrl(QStringLiteral("https://tv.example/emby/"), 8096) == QLatin1String("https://tv.example/emby"));
    CHECK(baseUrl(QStringLiteral("http://me:pw@nas:9981?x=1"), 9981) == QLatin1String("http://nas:9981"));
    CHECK(baseUrl(QStringLiteral("ftp://nas"), 21).isEmpty());
    CHECK(baseUrl(QString(), 80).isEmpty());

    // a playlist and a guide written, read back by the service's parsers
    const QVector<Channel> channels = {
        { QStringLiteral("2.1"), QStringLiteral("Demo \"One\""), QStringLiteral("2.1"), {}, QStringLiteral("HD"),
          QStringLiteral("http://192.168.1.50:5004/auto/v2.1"), {} },
        { QStringLiteral("4.1"), QStringLiteral("Sample"), QStringLiteral("4.1"), QStringLiteral("http://logo.lan/s.png"),
          {}, QStringLiteral("http://192.168.1.50:5004/auto/v4.1"), {} },
        { QStringLiteral("9"), QStringLiteral("No stream"), {}, {}, {}, {}, {} },
    };
    const QByteArray m3u = m3uOf(channels, QStringLiteral("file:///tmp/guide.xml"));
    CHECK(m3u.startsWith("#EXTM3U url-tvg=\"file:///tmp/guide.xml\""));
    const Playlist list = parseM3u(m3u, QStringLiteral("http://192.168.1.50"));
    CHECK(list.error.isEmpty() && list.channels.size() == 2);
    CHECK(list.guideUrls == QStringList{ QStringLiteral("file:///tmp/guide.xml") });
    if (list.channels.size() == 2)
    {
        CHECK(list.channels[0].tvgId == QLatin1String("2.1") && list.channels[0].name == QLatin1String("Demo \"One\""));
        CHECK(list.channels[0].group == QLatin1String("HD"));
        CHECK(list.channels[1].logo == QLatin1String("http://logo.lan/s.png"));
        CHECK(list.channels[1].url == QLatin1String("http://192.168.1.50:5004/auto/v4.1"));
    }
    QVector<Programme> programmes = {
        { QStringLiteral("2.1"), T0, T0 + 1800, QStringLiteral("News & <Weather>"), QStringLiteral("Part 1"),
          QStringLiteral("A \"quoted\" day."), QStringLiteral("News"), 2, 5 },
        { QStringLiteral("2.1"), T0 + 1800, T0 + 1800, QStringLiteral("Zero length"), {}, {}, {}, -1, -1 },
        { QStringLiteral("4.1"), T0, T0 + 3600, QStringLiteral("Sample Show"), {}, {}, {}, -1, -1 },
    };
    const QByteArray xml = xmltvOf(channels, programmes);
    GuideFilter filter;
    const Guide guide = parseXmltv(xml, filter);
    CHECK(guide.error.isEmpty());
    const auto news = guide.programmes.value(guideKey(QStringLiteral("2.1")));
    CHECK(news.size() == 1);
    if (!news.isEmpty())
    {
        CHECK(news[0].start == T0 && news[0].stop == T0 + 1800);
        CHECK(news[0].title == QLatin1String("News & <Weather>") && news[0].subtitle == QLatin1String("Part 1"));
        CHECK(news[0].description == QLatin1String("A \"quoted\" day.") && news[0].category == QLatin1String("News"));
        CHECK(news[0].episode == QLatin1String("S2E5"));
    }
    CHECK(guide.programmes.value(guideKey(QStringLiteral("4.1"))).size() == 1);
    // (a channel without a tvg-id is matched by its name)
    CHECK(guide.idByName.value(guideKey(QStringLiteral("Sample"))) == guideKey(QStringLiteral("4.1")));
}

static void testHdhomerun()
{
    const Tuner tuner = parseHdhrDiscover(fixture("hdhr-discover.json"));
    CHECK(tuner.id == QLatin1String("1050ABCD") && tuner.name == QLatin1String("HDHomeRun CONNECT"));
    CHECK(tuner.baseUrl == QLatin1String("http://192.168.1.50:80"));
    CHECK(tuner.lineupUrl == QLatin1String("http://192.168.1.50:80/lineup.json"));
    CHECK(tuner.deviceAuth == QLatin1String("made-up-device-auth"));
    const QVector<Tuner> cloud = parseHdhrCloudDiscover(fixture("hdhr-cloud-discover.json"));
    CHECK(cloud.size() == 1 && cloud.value(0).baseUrl == QLatin1String("http://192.168.1.50"));
    const QVector<Channel> lineup = parseHdhrLineup(fixture("hdhr-lineup.json"));
    CHECK(lineup.size() == 2);   // (not the DRM one, nor one without a number)
    CHECK(lineup.value(0).id == QLatin1String("2.1") && lineup.value(0).name == QLatin1String("DEMO-HD"));
    CHECK(lineup.value(0).group == QLatin1String("HD") && lineup.value(1).group.isEmpty());
    CHECK(lineup.value(1).url == QLatin1String("http://192.168.1.50:5004/auto/v4.1"));
    CHECK(item(hdhrGuideUrl(QStringLiteral("a b")), "DeviceAuth") == QLatin1String("a b"));
    const QVector<Programme> guide = parseHdhrGuide(fixture("hdhr-guide.json"));
    CHECK(guide.size() == 3);    // (not the one of no length)
    CHECK(guide.value(0).channel == QLatin1String("2.1") && guide.value(0).title == QLatin1String("Morning News"));
    CHECK(guide.value(0).category == QLatin1String("News") && guide.value(0).start == T0);
    CHECK(guide.value(1).season == 2 && guide.value(1).episode == 5 && guide.value(1).subtitle == QLatin1String("The Fifth"));
    // the discovery packet: libhdhomerun's, its CRC its own
    const QByteArray packet = hdhrDiscoverPacket();
    CHECK(packet.mid(4, 12).toHex() == "0104ffffffff0204ffffffff");
    CHECK(packet.size() == 20 && packet.left(4).toHex() == "0002000c");
    QByteArray reply = packet;
    reply[1] = 0x03;
    CHECK(!isHdhrDiscoverReply(reply));   // (its CRC no longer matches)
    CHECK(!isHdhrDiscoverReply(packet));  // (a request, not an answer)
    CHECK(!isHdhrDiscoverReply("short"));
}

static void testMediaBrowser()
{
    const QByteArray header = mediaBrowserAuthorization(QStringLiteral("dev1"), QStringLiteral("1.0"), QStringLiteral("tok"));
    CHECK(header.startsWith("MediaBrowser Client=\"VLC Redesign\"") && header.contains("DeviceId=\"dev1\"")
          && header.endsWith("Token=\"tok\""));
    CHECK(!mediaBrowserAuthorization(QStringLiteral("d"), QStringLiteral("1"), {}).contains("Token"));
    CHECK(mediaBrowserLoginBody(QStringLiteral("me"), QStringLiteral("p\"w")) == "{\"Pw\":\"p\\\"w\",\"Username\":\"me\"}");
    const Session session = parseMediaBrowserLogin(fixture("jellyfin-login.json"));
    CHECK(session.token == QLatin1String("made-up-access-token") && session.userName == QLatin1String("demo"));
    CHECK(session.userId == QLatin1String("4d5e6f708192a3b4c5d6e7f8091a2b3c"));
    CHECK(parseMediaBrowserLogin(fixture("jellyfin-login-no-token.json")).token.isEmpty());

    const QString base = QStringLiteral("http://192.168.1.10:8096");
    QUrl url = mediaBrowserChannelsUrl(base, QStringLiteral("u1"), false);
    CHECK(url.toString(QUrl::RemoveQuery) == QLatin1String("http://192.168.1.10:8096/LiveTv/Channels") && item(url, "UserId") == QLatin1String("u1"));
    CHECK(mediaBrowserChannelsUrl(base, QStringLiteral("u1"), true).path() == QLatin1String("/emby/LiveTv/Channels"));
    CHECK(mediaBrowserChannelsUrl(base + QStringLiteral("/emby"), QStringLiteral("u1"), true).path() == QLatin1String("/emby/LiveTv/Channels"));
    const QVector<Channel> channels = parseMediaBrowserChannels(fixture("jellyfin-channels.json"), base, false);
    CHECK(channels.size() == 3);     // (not the one of a bad id)
    CHECK(channels.value(0).number == QLatin1String("101") && channels.value(1).number == QLatin1String("102"));
    CHECK(channels.value(0).logo == QLatin1String("http://192.168.1.10:8096/Items/a1b2c3d4e5f60718293a4b5c6d7e8f90/Images/Primary?tag=0123abcd"));
    CHECK(channels.value(1).logo.isEmpty() && channels.value(2).group == QLatin1String("Radio"));
    CHECK(channels.value(0).url == QLatin1String("http://192.168.1.10:8096/Videos/a1b2c3d4e5f60718293a4b5c6d7e8f90/stream.ts"));
    url = mediaBrowserProgrammesUrl(base, QStringLiteral("u1"), T0, T0 + 86400, false);
    CHECK(item(url, "MinEndDate") == QLatin1String("2026-11-09T05:00:00Z") && item(url, "Fields") == QLatin1String("Overview,Genres"));
    const QVector<Programme> programmes = parseMediaBrowserProgrammes(fixture("jellyfin-programs.json"));
    CHECK(programmes.size() == 2);
    CHECK(programmes.value(0).start == 1794117600 && programmes.value(0).stop == 1794123000);
    CHECK(programmes.value(0).description == QLatin1String("A film of the open movie projects.") && programmes.value(0).category == QLatin1String("Movie"));
    CHECK(programmes.value(1).season == 3 && programmes.value(1).episode == 7 && programmes.value(1).subtitle == QLatin1String("The Meadow"));

    url = mediaBrowserPlaybackInfoUrl(base, QStringLiteral("c1"), QStringLiteral("u1"), false);
    CHECK(url.path() == QLatin1String("/Items/c1/PlaybackInfo") && item(url, "AutoOpenLiveStream") == QLatin1String("true"));
    CHECK(mediaBrowserPlaybackInfoBody().contains("DeviceProfile"));
    // the stream: direct (the tuner's own), the HLS of a FAST channel, else the server's transcode
    const QUrl direct(mediaBrowserStreamUrl(fixture("jellyfin-playbackinfo-direct.json"), base, QStringLiteral("a1"),
                                            QStringLiteral("tok"), false));
    CHECK(direct.path() == QLatin1String("/Videos/a1/stream.ts"));
    CHECK(item(direct, "Static") == QLatin1String("true") && item(direct, "MediaSourceId") == QLatin1String("ms-0123"));
    CHECK(item(direct, "LiveStreamId") == QLatin1String("live-4567") && item(direct, "PlaySessionId") == QLatin1String("play-89ab"));
    CHECK(item(direct, "ApiKey") == QLatin1String("tok"));
    const QUrl directEmby(mediaBrowserStreamUrl(fixture("jellyfin-playbackinfo-direct.json"), base, QStringLiteral("a1"),
                                                QStringLiteral("tok"), true));
    CHECK(directEmby.path() == QLatin1String("/emby/Videos/a1/stream.ts") && item(directEmby, "api_key") == QLatin1String("tok"));
    const QUrl transcode(mediaBrowserStreamUrl(fixture("jellyfin-playbackinfo-transcode.json"), base, QStringLiteral("b1"),
                                               QStringLiteral("tok"), false));
    CHECK(transcode.path() == QLatin1String("/videos/b1b2/master.m3u8") && item(transcode, "LiveStreamId") == QLatin1String("live-4568"));
    CHECK(item(transcode, "ApiKey") == QLatin1String("tok") && item(transcode, "MediaSourceId") == QLatin1String("ms-0124"));
    CHECK(mediaBrowserStreamUrl(fixture("jellyfin-playbackinfo-hls.json"), base, QStringLiteral("c1"), QStringLiteral("tok"), false)
          == QLatin1String("https://fast.example/live/master.m3u8"));
    CHECK(mediaBrowserStreamUrl("{}", base, QStringLiteral("c1"), QStringLiteral("tok"), false).isEmpty());

    CHECK(mediaBrowserDiscoverPacket(false) == "who is JellyfinServer?" && mediaBrowserDiscoverPacket(true) == "who is EmbyServer?");
    const Found jellyfin = parseMediaBrowserDiscoverReply(fixture("jellyfin-discover-reply.json"), false);
    CHECK(jellyfin.kind == QLatin1String("jellyfin") && jellyfin.name == QLatin1String("Living Room"));
    CHECK(jellyfin.address == QLatin1String("http://192.168.1.10:8096") && jellyfin.id == QLatin1String("5e6f708192a3b4c5"));
    const Found emby = parseMediaBrowserDiscoverReply(fixture("emby-discover-reply.json"), true);
    CHECK(emby.kind == QLatin1String("emby") && emby.name == QLatin1String("Emby Den"));
    CHECK(parseMediaBrowserDiscoverReply("who is JellyfinServer?", false).address.isEmpty());
}

static void testTvheadendAndChannels()
{
    const QString base = QStringLiteral("http://192.168.1.20:9981");
    CHECK(tvheadendPlaylistUrl(base) == QUrl(QStringLiteral("http://192.168.1.20:9981/playlist/channels.m3u")));
    CHECK(tvheadendGuideUrl(base) == QUrl(QStringLiteral("http://192.168.1.20:9981/xmltv/channels")));
    const QByteArray m3u = tvheadendWithUser(fixture("tvheadend-playlist.m3u"), QStringLiteral("me"), QStringLiteral("p@ss"));
    CHECK(m3u.contains("http://me:p%40ss@192.168.1.20:9981/stream/channelid/12345?profile=pass"));
    CHECK(m3u.contains("logo=\"http://192.168.1.20:9981/imagecache/1\""));   // (only the streams)
    const Playlist list = parseM3u(m3u, base);
    CHECK(list.channels.size() == 2 && list.channels.value(1).name == QLatin1String("Demo Two"));
    CHECK(tvheadendWithUser(fixture("tvheadend-playlist.m3u"), {}, {}) == fixture("tvheadend-playlist.m3u"));
    CHECK(channelsPlaylistUrl(QStringLiteral("http://nas:8089")).toString() == QLatin1String("http://nas:8089/devices/ANY/channels.m3u"));
    CHECK(channelsGuideUrl(QStringLiteral("http://nas:8089")).toString() == QLatin1String("http://nas:8089/devices/ANY/guide/xmltv"));
}

static void testNextPvr()
{
    const QString base = QStringLiteral("http://192.168.1.30:8866");
    QUrl url = nextPvrInitiateUrl(base);
    CHECK(url.path() == QLatin1String("/service") && item(url, "method") == QLatin1String("session.initiate")
          && item(url, "format") == QLatin1String("json"));
    const Salted salted = parseNextPvrInitiate(fixture("nextpvr-initiate.json"));
    CHECK(salted.sid == QLatin1String("made-up-sid") && salted.salt == QLatin1String("made-up-salt"));
    // md5(":" + md5("0000") + ":" + salt), as its clients make it
    CHECK(nextPvrLoginKey(QStringLiteral("0000"), QStringLiteral("made-up-salt"))
          == QString::fromLatin1(QCryptographicHash::hash(":" + QCryptographicHash::hash("0000", QCryptographicHash::Md5).toHex()
                                                              + ":made-up-salt", QCryptographicHash::Md5).toHex()));
    url = nextPvrLoginUrl(base, QStringLiteral("s"), QStringLiteral("k"));
    CHECK(item(url, "method") == QLatin1String("session.login") && item(url, "md5") == QLatin1String("k"));
    CHECK(parseNextPvrLogin(fixture("nextpvr-login-ok.json")) && !parseNextPvrLogin(fixture("nextpvr-login-fail.json")));
    const QVector<Channel> channels = parseNextPvrChannels(fixture("nextpvr-channels.json"), base, QStringLiteral("made-up-sid"));
    CHECK(channels.size() == 2);
    CHECK(channels.value(0).id == QLatin1String("7101") && channels.value(0).number == QLatin1String("2.1"));
    CHECK(QUrl(channels.value(0).url).path() == QLatin1String("/live") && item(QUrl(channels.value(0).url), "channeloid") == QLatin1String("7101")
          && item(QUrl(channels.value(0).url), "sid") == QLatin1String("made-up-sid"));
    CHECK(item(QUrl(channels.value(0).logo), "method") == QLatin1String("channel.icon") && channels.value(1).logo.isEmpty());
    CHECK(channels.value(1).group == QLatin1String("Radio"));
    url = nextPvrListingsUrl(base, QStringLiteral("s"), QStringLiteral("7101"), T0, T0 + 3600);
    CHECK(item(url, "channel_id") == QLatin1String("7101") && item(url, "start") == QString::number(T0));
    const QVector<Programme> listings = parseNextPvrListings(fixture("nextpvr-listings.json"), QStringLiteral("7101"));
    CHECK(listings.size() == 3);
    CHECK(listings.value(0).category == QLatin1String("News") && listings.value(0).channel == QLatin1String("7101"));
    CHECK(listings.value(1).season == 1 && listings.value(1).episode == 2 && listings.value(1).subtitle.isEmpty());
    CHECK(listings.value(2).start == T0 + 3600 && listings.value(2).stop == T0 + 5400);   // (milliseconds)
}

static void testMdns()
{
    // the question: PTR of the service, class IN with the unicast bit
    const QByteArray query = mdnsQuery(QStringLiteral("_channels_dvr._tcp.local"));
    CHECK(query.left(12) == QByteArray::fromHex("000000000001000000000000"));
    CHECK(query.mid(12, 14) == QByteArray("\x0d_channels_dvr", 14) && query.endsWith(QByteArray::fromHex("056c6f63616c00000c8001")));
    // a made-up Channels DVR's answer, its names compressed
    const QVector<MdnsService> found = parseMdnsAnswer(fixture("mdns-channels-reply.bin"), QStringLiteral("_channels_dvr._tcp.local"));
    CHECK(found.size() == 1);
    CHECK(found.value(0).name == QLatin1String("Demo DVR") && found.value(0).port == 8089);
    CHECK(found.value(0).host == QLatin1String("demo-dvr.local") && found.value(0).address == QLatin1String("192.168.50.20"));
    // another service's answer, and broken packets: nothing
    CHECK(parseMdnsAnswer(fixture("mdns-other-reply.bin"), QStringLiteral("_channels_dvr._tcp.local")).isEmpty());
    CHECK(parseMdnsAnswer(QByteArray("\x00\x00", 2), QStringLiteral("_channels_dvr._tcp.local")).isEmpty());
    QByteArray cut = fixture("mdns-channels-reply.bin");
    cut.chop(30);
    CHECK(parseMdnsAnswer(cut, QStringLiteral("_channels_dvr._tcp.local")).size() <= 1);
    // a pointer to itself: no endless loop
    QByteArray loop = QByteArray::fromHex("000084000000000100000000") + QByteArray::fromHex("c00c000c000100001194000200");
    CHECK(parseMdnsAnswer(loop, QStringLiteral("_channels_dvr._tcp.local")).isEmpty());
}

static void testSchedulesDirect()
{
    // the password as its SHA-1, kept so (a hash given again stays one)
    const QString hash = sdPasswordHash(QStringLiteral("demo-pass"));
    CHECK(hash == QString::fromLatin1(QCryptographicHash::hash("demo-pass", QCryptographicHash::Sha1).toHex()));
    CHECK(sdPasswordHash(hash) == hash);
    const QJsonObject body = QJsonDocument::fromJson(sdTokenBody(QStringLiteral("demo"), hash)).object();
    CHECK(body.value(QLatin1String("username")).toString() == QLatin1String("demo")
          && body.value(QLatin1String("password")).toString() == hash);
    SdAnswer answer = parseSdToken(fixture("sd-token-ok.json"));
    CHECK(answer.code == 0 && answer.token == QLatin1String("made-up-token-0001"));
    answer = parseSdToken(fixture("sd-token-refused.json"));
    CHECK(answer.code == 4003 && answer.token.isEmpty() && answer.message.startsWith(QLatin1String("Invalid")));
    CHECK(parseSdToken("not json").code == -1);
    // the lineups: the deleted one and one off its server left out
    int code = -1;
    QStringList lineups = parseSdLineups(fixture("sd-lineups.json"), &code);
    CHECK(code == 0 && lineups == QStringList { QStringLiteral("/20141201/lineups/USA-OTA-00000") });
    lineups = parseSdLineups(fixture("sd-lineups-none.json"), &code);
    CHECK(code == 4102 && lineups.isEmpty());
    // the stations: numbers of the map (antenna: major.minor, else RF)
    const QVector<Channel> stations = parseSdLineup(fixture("sd-lineup.json"));
    CHECK(stations.size() == 3);
    CHECK(stations.value(0).id == QLatin1String("90001") && stations.value(0).name == QLatin1String("Demo Broadcasting"));
    CHECK(stations.value(0).number == QLatin1String("4.1") && stations.value(0).altNames.contains(QLatin1String("KDMO"))
          && stations.value(0).altNames.contains(QLatin1String("DEMO")));
    CHECK(stations.value(0).logo == QLatin1String("https://example.invalid/kdmo.png"));
    CHECK(stations.value(1).name == QLatin1String("KSMP") && stations.value(1).number == QLatin1String("007"));
    CHECK(stations.value(2).number == QLatin1String("33"));
    // the days asked, from the day of the start (UTC)
    const QJsonArray days = QJsonDocument::fromJson(sdSchedulesBody({ QStringLiteral("90001") }, T0, 3)).array();
    CHECK(days.size() == 1 && days.at(0).toObject().value(QLatin1String("stationID")).toString() == QLatin1String("90001"));
    CHECK(days.at(0).toObject().value(QLatin1String("date")).toArray()
          == QJsonArray({ QStringLiteral("2026-11-09"), QStringLiteral("2026-11-10"), QStringLiteral("2026-11-11") }));
    const QVector<SdAiring> airings = parseSdSchedules(fixture("sd-schedules.json"));
    CHECK(airings.size() == 3);
    CHECK(airings.value(0).station == QLatin1String("90001") && airings.value(0).start == T0 && airings.value(0).duration == 1800);
    CHECK(airings.value(2).station == QLatin1String("90002") && airings.value(2).start == T0 + 3600);
    CHECK(QJsonDocument::fromJson(sdProgramsBody({ QStringLiteral("EP1"), QStringLiteral("SH2") })).array().size() == 2);
    const QHash<QString, Programme> programs = parseSdPrograms(fixture("sd-programs.json"));
    CHECK(programs.size() == 2);
    const Programme movie = programs.value(QStringLiteral("EP000000010001"));
    CHECK(movie.title == QLatin1String("Open Movie Night") && movie.subtitle == QLatin1String("Big Buck Bunny"));
    CHECK(movie.description == QLatin1String("A long one.") && movie.category == QLatin1String("Animation"));
    CHECK(movie.season == 2 && movie.episode == 5);
    CHECK(programs.value(QStringLiteral("SH000000020000")).description == QLatin1String("Today.")
          && programs.value(QStringLiteral("SH000000020000")).season == -1);
    // the guide written: the stations matched by their names and call signs
    QVector<Programme> guide;
    for (const SdAiring &a : airings)
    {
        Programme p = programs.value(a.program);
        p.channel = a.station;
        p.start = a.start;
        p.stop = a.start + a.duration;
        guide.append(p);
    }
    GuideFilter all;
    const Guide read = parseXmltv(xmltvOf(stations, guide), all);
    CHECK(read.programmes.value(guideKey(QStringLiteral("90001"))).size() == 2);
    CHECK(read.idByName.value(guideKey(QStringLiteral("KDMO"))) == guideKey(QStringLiteral("90001")));
    CHECK(read.idByName.value(guideKey(QStringLiteral("Demo Broadcasting"))) == guideKey(QStringLiteral("90001")));
}

// Times out of range of a qint64 (1e300, infinity as text): no programme of
// them, never a cast out of range (built with -fsanitize=float-cast-overflow)
static void testOutOfRangeTimes()
{
    const QVector<Programme> hdhr = parseHdhrGuide(R"([{"GuideNumber":"5.1","Guide":[
        {"StartTime":1e300,"EndTime":1e301,"Title":"Huge"},
        {"StartTime":-1e300,"EndTime":1700003600,"Title":"Negative"},
        {"StartTime":1700000000,"EndTime":1700003600,"Title":"Fine"}]}])");
    CHECK(hdhr.size() == 2);
    CHECK(hdhr.value(hdhr.size() - 1).title == QStringLiteral("Fine") && hdhr.value(hdhr.size() - 1).start == 1700000000);
    for (const Programme &p : hdhr)
        CHECK(p.title != QStringLiteral("Huge"));
    const QVector<Programme> nextpvr = parseNextPvrListings(R"({"listings":[
        {"start":1e300,"end":1e300,"name":"Huge"},
        {"start":"1700000000000","end":"1700003600000","name":"Milliseconds as text"}]})", QStringLiteral("7"));
    CHECK(nextpvr.size() == 1);
    CHECK(nextpvr.value(0).start == 1700000000 && nextpvr.value(0).stop == 1700003600);
}

// What a server or a packet could send that the parsers took badly: a name
// of mDNS longer than 255 bytes read again by every record (quadratic), a
// lone surrogate in a programme (the guide written did not read back), a
// TranscodingUrl of another host (sent the token)
static void testHostile()
{
    // one long name (32 KB of labels), then as many records pointing to it
    QByteArray records;
    QByteArray name;
    while (name.size() < 32 * 1024)
        name += char(63) + QByteArray(63, 'a');
    name += '\0';
    auto record = [](const QByteArray &owner, quint16 type, const QByteArray &data) {
        QByteArray r = owner;
        r += char(type >> 8);
        r += char(type & 0xff);
        r += QByteArray::fromHex("0001000000780000");
        r[r.size() - 2] = char(data.size() >> 8);
        r[r.size() - 1] = char(data.size() & 0xff);
        return r + data;
    };
    records += record(name, 16, QByteArray(1, '\0'));
    int count = 1;
    while (records.size() < 64 * 1024 && count < 65535)
    {
        records += record(QByteArray::fromHex("c00c"), 12, QByteArray::fromHex("c00c"));
        ++count;
    }
    QByteArray packet = QByteArray::fromHex("000084000000");
    packet += char(count >> 8);
    packet += char(count & 0xff);
    packet += QByteArray::fromHex("00000000");
    packet += records;
    QElapsedTimer timer;
    timer.start();
    CHECK(parseMdnsAnswer(packet, QStringLiteral("_channels_dvr._tcp.local")).isEmpty());
    std::printf("mDNS: a name of 32 KB read by %d records: %lld ms\n", count, (long long)timer.elapsed());
    CHECK(timer.elapsed() < 500);
    // (the fixture's own names, of the usual lengths, still read)
    CHECK(parseMdnsAnswer(fixture("mdns-channels-reply.bin"), QStringLiteral("_channels_dvr._tcp.local")).size() == 1);

    // a lone surrogate and control characters in what a server sent: dropped
    // from the guide written, which reads back
    QVector<Channel> channels { Channel {} };
    channels[0].id = QStringLiteral("1");
    channels[0].name = QStringLiteral("One") + QChar(0xD800);
    QVector<Programme> programmes { Programme {} };
    programmes[0].channel = QStringLiteral("1");
    programmes[0].start = 1700000000;
    programmes[0].stop = 1700003600;
    programmes[0].title = QStringLiteral("Title") + QChar(0xDC00) + QChar(0x01) + QChar(0xFFFF);
    programmes[0].description = QStringLiteral("A \U0001F600 face, a ") + QChar(0xD800) + QStringLiteral(" alone");
    GuideFilter all;
    const Guide read = parseXmltv(xmltvOf(channels, programmes), all);
    CHECK(read.error.isEmpty());
    const QVector<iptv::Programme> list = read.programmes.value(guideKey(QStringLiteral("1")));
    CHECK(list.size() == 1);
    CHECK(list.value(0).title == QStringLiteral("Title"));
    CHECK(list.value(0).description == QStringLiteral("A \U0001F600 face, a  alone"));
    CHECK(read.idByName.value(guideKey(QStringLiteral("One"))) == guideKey(QStringLiteral("1")));

    // a TranscodingUrl on another host ("//host", "/\host"): none
    const QString base = QStringLiteral("http://192.168.1.10:8096");
    for (const char *other : { "//evil.example/videos/x/master.m3u8?a=1", "/\\evil.example/x", "///evil.example/x" })
    {
        const QByteArray info = QByteArray(R"({"MediaSources":[{"SupportsDirectStream":false,"TranscodingUrl":")")
                                + QByteArray(other).replace("\\", "\\\\") + R"(","LiveStreamId":"l"}],"PlaySessionId":"s"})";
        CHECK(mediaBrowserStreamUrl(info, base, QStringLiteral("b1"), QStringLiteral("tok"), false).isEmpty());
    }
    const QUrl own(mediaBrowserStreamUrl(
        R"({"MediaSources":[{"SupportsDirectStream":false,"TranscodingUrl":"/videos/b1/master.m3u8?a=1"}]})", base,
        QStringLiteral("b1"), QStringLiteral("tok"), false));
    CHECK(own.host() == QStringLiteral("192.168.1.10") && own.port() == 8096 && own.path() == QStringLiteral("/videos/b1/master.m3u8"));
}

int main(int argc, char **argv)
{
    fixtures = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral("extras/juxtopposed/fixtures/iptv-servers");
    testOutOfRangeTimes();
    testHostile();
    testCommon();
    testHdhomerun();
    testMediaBrowser();
    testTvheadendAndChannels();
    testNextPvr();
    testMdns();
    testSchedulesDirect();
    std::printf("TV servers: %d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
