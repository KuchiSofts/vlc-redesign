/* Copyright (C) 2026 VLC authors and VideoLAN. GPL version 2 or later.
 *
 * The subtitle services asked with the user's own key or account
 * (modules/gui/qt/player/subtitle_services.cpp): the addresses they are
 * asked, their answers read from saved ones (fixtures/subtitle-services:
 * written from the shapes their maintained clients read, the refusals
 * recorded from the services with a made-up key), their errors; YIFY's
 * page read in one pass (against the expressions it was read with before,
 * and against hostile pages, timed), and the archives a download comes in
 * (gzip, zip: whole or nothing, within their bounds). On Linux, from the
 * root of the tree:
 *   g++ -std=c++17 -O1 -Wall -Wextra -fPIC -DHAVE_ZLIB -I. -Imodules/gui/qt $(pkg-config --cflags Qt6Core) \
 *       extras/juxtopposed/subtitle-services.cpp modules/gui/qt/player/subtitle_services.cpp \
 *       $(pkg-config --libs Qt6Core) -lz -o subtitle-services
 *   ./subtitle-services extras/juxtopposed/fixtures/subtitle-services
 */
#include "modules/gui/qt/player/subtitle_services.hpp"

#include <QElapsedTimer>
#include <QFile>
#include <QHash>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QUrlQuery>
#include <QtEndian>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <random>
#include <zlib.h>

using namespace SubtitleProviders;

static int checks = 0;
static int failures = 0;
#define CHECK(condition) do { ++checks; if (!(condition)) { \
    std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); ++failures; } } while (0)

static QString fixtures;

static QByteArray readFixture(const char *name)
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

// The app's SubtitlePreferences::normalize, for the names the tests use
static QString languageOf(const QString &name)
{
    const QString n = name.trimmed().toLower();
    if (n.contains(QLatin1String("brazil")))
        return QStringLiteral("pt-BR");
    static const QHash<QString, QString> names = {
        { "arabic", "ar" }, { "english", "en" }, { "french", "fr" }, { "german", "de" }, { "hebrew", "he" },
    };
    if (names.contains(n))
        return names.value(n);
    return n.size() == 2 ? n : QString();
}

static QString item(const QUrl &url, const char *key)
{
    return QUrlQuery(url).queryItemValue(QLatin1String(key), QUrl::FullyDecoded);
}

static const Result *byId(const QList<Result> &results, const char *id)
{
    for (const Result &r : results)
        if (r.id == QLatin1String(id))
            return &r;
    return nullptr;
}

static Query film()
{
    Query q;
    q.imdb = QStringLiteral("tt0133093");
    q.title = QStringLiteral("The Matrix");
    q.year = 1999;
    q.fileName = QStringLiteral("The.Matrix.1999.1080p.BluRay.x264.mkv");
    q.hash = QStringLiteral("8e245d9679d31e12");
    q.languages = { QStringLiteral("ar"), QStringLiteral("pt-BR"), QStringLiteral("zh-TW") };
    q.credentials.key = QStringLiteral("my-key");
    return q;
}

static Query episode()
{
    Query q;
    q.imdb = QStringLiteral("tt0903747");
    q.title = QStringLiteral("Breaking Bad");
    q.series = true;
    q.season = 2;
    q.episode = 3;
    q.fileName = QStringLiteral("Breaking.Bad.S02E03.720p.BluRay.x264-DEMAND.mkv");
    q.languages = { QStringLiteral("ar"), QStringLiteral("en") };
    q.credentials.key = QStringLiteral("my-key");
    return q;
}

// A video from the network (#73): whether its address names it, the size of
// a byte range's answer, the hash from its first and last 64 KiB
static void testNetworkMedia()
{
    CHECK(namesSomething(QStringLiteral("Sintel.2010.1080p.mp4")));
    CHECK(namesSomething(QStringLiteral("Breaking Bad S01E02.mkv")));
    CHECK(namesSomething(QStringLiteral("סינטל.mp4")));
    CHECK(!namesSomething(QStringLiteral("500.mp4")));
    CHECK(!namesSomething(QStringLiteral("index.m3u8")));
    CHECK(!namesSomething(QStringLiteral("MASTER.m3u8")));
    CHECK(!namesSomething(QStringLiteral("stream")));
    CHECK(!namesSomething(QStringLiteral("a1b2c3.mp4")));
    CHECK(!namesSomething(QString()));
    CHECK(namesSomething(QStringLiteral("1917.Film.mp4")));
    CHECK(contentRangeTotal("bytes 0-65535/1200000") == 1200000);
    CHECK(contentRangeTotal(" Bytes 134464-199999/200000 ") == 200000);
    CHECK(contentRangeTotal("bytes 0-65535/*") == -1);
    CHECK(contentRangeTotal("items 0-1/2") == -1);
    CHECK(contentRangeTotal("") == -1);
    // the hash of a file is the same from its pieces: 200000 bytes, byte i
    // (7 i + 3) & 255, as the test server of http-request-test sends it
    QByteArray file(200000, Qt::Uninitialized);
    for (int i = 0; i < file.size(); ++i)
        file[i] = char((7 * i + 3) & 255);
    const QByteArray head = file.left(65536), tail = file.right(65536);
    quint64 sum = 200000;
    for (int i = 0; i < 65536; i += 8)
        sum += qFromLittleEndian<quint64>(head.constData() + i) + qFromLittleEndian<quint64>(tail.constData() + i);
    CHECK(openSubtitlesHashOf(200000, head, tail) == QStringLiteral("%1").arg(sum, 16, 16, QLatin1Char('0')));
    CHECK(openSubtitlesHashOf(200000, head, tail).size() == 16);
    CHECK(openSubtitlesHashOf(200000, head.left(100), tail).isEmpty());
    CHECK(openSubtitlesHashOf(1000, head, tail).isEmpty());
}

// Languages to search after the preferred or the second language was chosen
static void testChosenLanguagesFirst()
{
    const auto list = [](std::initializer_list<const char *> codes) {
        QStringList out;
        for (const char *c : codes)
            out << QString::fromLatin1(c);
        return out;
    };
    // a second language turned off before: on, after the preferred
    CHECK(withChosenLanguagesFirst(list({ "he", "en", "-fr", "de" }), QStringLiteral("he"), QStringLiteral("fr"))
          == list({ "he", "fr", "en", "de" }));
    // a preferred one lower in the list, or off: first and on
    CHECK(withChosenLanguagesFirst(list({ "he", "en", "-fr" }), QStringLiteral("fr"), QStringLiteral("en"))
          == list({ "fr", "en", "he" }));
    // a new one: added; the rest in its order, off ones stay off
    CHECK(withChosenLanguagesFirst(list({ "he", "-ar", "en" }), QStringLiteral("es"), QString())
          == list({ "es", "he", "-ar", "en" }));
    // no second, the same twice, duplicates and empty entries dropped
    CHECK(withChosenLanguagesFirst(list({ "en", "en", "-", "", "-he" }), QStringLiteral("en"), QStringLiteral("en"))
          == list({ "en", "-he" }));
    CHECK(withChosenLanguagesFirst({}, QStringLiteral("pt-BR"), QStringLiteral("pt"))
          == list({ "pt-BR", "pt" }));
}

static void testCommon()
{
    CHECK(problemOf(200) == Problem::None);
    CHECK(problemOf(0) == Problem::NoAnswer);
    CHECK(problemOf(401) == Problem::KeyRefused);
    CHECK(problemOf(403) == Problem::KeyRefused);
    CHECK(problemOf(429) == Problem::Busy);
    CHECK(problemOf(500) == Problem::Failed);
    CHECK(checkOf(200) == Check::Works);
    CHECK(checkOf(401) == Check::KeyRefused);
    CHECK(checkOf(429) == Check::Busy);
    CHECK(checkOf(0) == Check::NoAnswer);
    CHECK(checkOf(502) == Check::NoAnswer);

    CHECK(cleanRelease(QStringLiteral(" * The.Matrix.1999 - ")) == QLatin1String("The.Matrix.1999"));
    // the episode named, a range, the season as a whole, another one
    CHECK(releaseOfEpisode(QStringLiteral("Breaking.Bad.S02E03.720p"), 2, 3));
    CHECK(releaseOfEpisode(QStringLiteral("breaking bad s2e3"), 2, 3));
    CHECK(releaseOfEpisode(QStringLiteral("Breaking.Bad.2x03.HDTV"), 2, 3));
    CHECK(releaseOfEpisode(QStringLiteral("Breaking.Bad.S02E01-E05"), 2, 3));
    CHECK(releaseOfEpisode(QStringLiteral("Breaking.Bad.S02E02E03"), 2, 3));
    CHECK(releaseOfEpisode(QStringLiteral("Breaking.Bad.S02.COMPLETE.720p"), 2, 3));
    CHECK(!releaseOfEpisode(QStringLiteral("Breaking.Bad.S02E04.720p"), 2, 3));
    CHECK(!releaseOfEpisode(QStringLiteral("Breaking.Bad.S01E03.720p"), 2, 3));
    CHECK(!releaseOfEpisode(QStringLiteral("Breaking.Bad.S02E06-E09"), 2, 3));
    // (a film: everything)
    CHECK(releaseOfEpisode(QStringLiteral("Breaking.Bad.S02E04"), -1, -1));
    // (1080p is no episode, nor x264)
    CHECK(releaseOfEpisode(QStringLiteral("Show.1080p.x264"), 2, 3));
    CHECK(!releaseOfEpisode(QStringLiteral("Show.S02E01-720p"), 2, 3));
}

static void testSubSource()
{
    // the names of its languages, both ways
    CHECK(subSourceNames(QStringLiteral("pt-BR")) == QStringList{ QStringLiteral("brazilian_portuguese") });
    CHECK(subSourceNames(QStringLiteral("fa")) == QStringList{ QStringLiteral("farsi_persian") });
    CHECK(subSourceNames(QStringLiteral("zh-TW")).contains(QStringLiteral("chinese_traditional")));
    CHECK(subSourceNames(QStringLiteral("xx")).isEmpty());
    CHECK(languageOfSubSource(QStringLiteral("farsi_persian")) == QLatin1String("fa"));
    CHECK(languageOfSubSource(QStringLiteral("Brazilian Portuguese")) == QLatin1String("pt-BR"));
    CHECK(languageOfSubSource(QStringLiteral("spanish_latin_america")) == QLatin1String("es-419"));
    CHECK(languageOfSubSource(QStringLiteral("big_5_code")) == QLatin1String("zh-TW"));
    CHECK(languageOfSubSource(QStringLiteral("Arabic")) == QLatin1String("ar"));
    CHECK(languageOfSubSource(QStringLiteral("klingon")).isEmpty());

    // the title: by its IMDb id, else its name (with its year, a film)
    const Query q = film();
    QUrl url = subSourceSearchUrl(q, false);
    CHECK(url.host() == QLatin1String("api.subsource.net"));
    CHECK(url.path() == QLatin1String("/api/v1/movies/search"));
    CHECK(item(url, "searchType") == QLatin1String("imdb"));
    CHECK(item(url, "imdb") == QLatin1String("tt0133093"));
    CHECK(!url.toString().contains(QLatin1String("my-key"))); // (in its header)
    url = subSourceSearchUrl(q, true);
    CHECK(item(url, "searchType") == QLatin1String("text"));
    CHECK(item(url, "q") == QLatin1String("The Matrix"));
    CHECK(item(url, "year") == QLatin1String("1999"));
    CHECK(item(url, "type") == QLatin1String("movie"));
    CHECK(url.toEncoded().contains("q=The%20Matrix"));
    Query unknown = q;
    unknown.imdb.clear();
    CHECK(item(subSourceSearchUrl(unknown, false), "searchType") == QLatin1String("text"));
    unknown.title.clear();
    CHECK(subSourceSearchUrl(unknown, false).isEmpty());
    CHECK(item(subSourceSearchUrl(episode(), true), "type") == QLatin1String("tvseries"));
    CHECK(item(subSourceCheckUrl(), "imdb") == QLatin1String("tt0133093"));

    // the film of its IMDb id; the season of the episode
    QString error;
    CHECK(subSourceMovieId(readFixture("subsource-movies.json"), q, &error) == QLatin1String("123"));
    CHECK(error.isEmpty());
    CHECK(subSourceMovieId(readFixture("subsource-series.json"), episode(), &error) == QLatin1String("902"));
    Query seasonless = episode();
    seasonless.season = seasonless.episode = -1;
    CHECK(subSourceMovieId(readFixture("subsource-series.json"), seasonless, &error) == QLatin1String("901"));
    Query fifth = episode();
    fifth.season = 5;
    CHECK(subSourceMovieId(readFixture("subsource-series.json"), fifth, &error).isEmpty());
    CHECK(subSourceMovieId(readFixture("subsource-empty.json"), q, &error).isEmpty());
    CHECK(error.isEmpty());
    CHECK(subSourceMovieId(readFixture("subsource-key-refused.json"), q, &error).isEmpty());
    CHECK(error == QLatin1String("The provided API key is invalid or inactive"));

    // its subtitles in the user's languages, by their names
    url = subSourceSubtitlesUrl(QStringLiteral("902"), episode());
    CHECK(url.path() == QLatin1String("/api/v1/subtitles"));
    CHECK(item(url, "movieId") == QLatin1String("902"));
    CHECK(item(url, "language") == QLatin1String("arabic,english"));
    CHECK(item(subSourceSubtitlesUrl(QStringLiteral("1"), q), "language")
          == QLatin1String("arabic,brazilian_portuguese,chinese_traditional,big_5_code"));

    // those of this episode (or the whole season); a bad id, an unknown language left out
    error.clear();
    const QList<Result> results = parseSubSource(readFixture("subsource-subtitles.json"), episode(), languageOf, &error);
    CHECK(error.isEmpty());
    CHECK(results.size() == 4);
    const Result *arabic = byId(results, "subsource:9");
    CHECK(arabic && arabic->language == QLatin1String("ar"));
    CHECK(arabic && arabic->release == QLatin1String("Breaking.Bad.S02E03.720p.BluRay.x264-DEMAND"));
    CHECK(arabic && arabic->url == QUrl(QStringLiteral("https://api.subsource.net/api/v1/subtitles/9/download")));
    CHECK(arabic && arabic->provider == QLatin1String("subsource"));
    CHECK(arabic && arabic->downloads == 340);
    CHECK(!byId(results, "subsource:10")); // S02E04
    const Result *farsi = byId(results, "subsource:11");
    CHECK(farsi && farsi->language == QLatin1String("fa") && farsi->hearingImpaired);
    const Result *brazil = byId(results, "subsource:12");
    CHECK(brazil && brazil->language == QLatin1String("pt-BR"));
    CHECK(brazil && brazil->release == QLatin1String("Breaking.Bad.S02E01-E03.WEB"));
    CHECK(brazil && arabic && brazil->rank < arabic->rank); // (rated bad)
    CHECK(byId(results, "subsource:15") && byId(results, "subsource:15")->language == QLatin1String("es-419"));
    CHECK(results.size() == 4 && !byId(results, "subsource:14"));
    // a film: all of them
    CHECK(parseSubSource(readFixture("subsource-subtitles.json"), q, languageOf, &error).size() == 5);
    // refused
    CHECK(parseSubSource(readFixture("subsource-key-refused.json"), q, languageOf, &error).isEmpty());
    CHECK(error == QLatin1String("The provided API key is invalid or inactive"));
}

static void testSubDl()
{
    CHECK(subDlCode(QStringLiteral("pt-BR")) == QLatin1String("BR_PT"));
    CHECK(subDlCode(QStringLiteral("zh-TW")) == QLatin1String("ZH_BG"));
    CHECK(subDlCode(QStringLiteral("ar")) == QLatin1String("AR"));
    CHECK(subDlCode(QStringLiteral("es-419")) == QLatin1String("ES"));
    CHECK(subDlCode(QStringLiteral("xx")).isEmpty());
    CHECK(languageOfSubDl(QStringLiteral("BR_PT"), languageOf) == QLatin1String("pt-BR"));
    CHECK(languageOfSubDl(QStringLiteral("AR"), languageOf) == QLatin1String("ar"));
    CHECK(languageOfSubDl(QStringLiteral("arabic"), languageOf) == QLatin1String("ar"));
    CHECK(languageOfSubDl(QStringLiteral("BG_EN"), languageOf).isEmpty());

    // v2: the key in its header; a film by its IMDb id
    const Query q = film();
    QUrl url = subDlSearchUrl(q, true);
    CHECK(url.toString(QUrl::RemoveQuery) == QLatin1String("https://api.subdl.com/api/v2/subtitles/search"));
    CHECK(item(url, "type") == QLatin1String("movie"));
    CHECK(item(url, "imdb_id") == QLatin1String("tt0133093"));
    CHECK(item(url, "year") == QLatin1String("1999"));
    CHECK(item(url, "languages") == QLatin1String("ar,br_pt,zh_bg"));
    CHECK(!url.toString().contains(QLatin1String("my-key")));
    // an episode by the series' name, its season, episode and file
    url = subDlSearchUrl(episode(), true);
    CHECK(item(url, "type") == QLatin1String("tv"));
    CHECK(item(url, "film_name") == QLatin1String("Breaking Bad"));
    CHECK(item(url, "season") == QLatin1String("2"));
    CHECK(item(url, "episode") == QLatin1String("3"));
    CHECK(item(url, "file_name") == QLatin1String("Breaking.Bad.S02E03.720p.BluRay.x264-DEMAND"));
    CHECK(item(url, "imdb_id").isEmpty());
    // v1: the key in the address, its own names of the parameters
    url = subDlSearchUrl(episode(), false);
    CHECK(url.toString(QUrl::RemoveQuery) == QLatin1String("https://api.subdl.com/api/v1/subtitles"));
    CHECK(item(url, "api_key") == QLatin1String("my-key"));
    CHECK(item(url, "imdb_id") == QLatin1String("tt0903747"));
    CHECK(item(url, "type") == QLatin1String("tv"));
    CHECK(item(url, "season_number") == QLatin1String("2"));
    CHECK(item(url, "episode_number") == QLatin1String("3"));
    CHECK(item(url, "languages") == QLatin1String("AR,EN"));
    CHECK(item(url, "subs_per_page") == QLatin1String("30"));
    CHECK(item(subDlCheckUrl(QStringLiteral("k"), true), "imdb_id") == QLatin1String("tt0133093"));
    CHECK(item(subDlCheckUrl(QStringLiteral("k"), false), "api_key") == QLatin1String("k"));

    // a film: its languages, once each, on dl.subdl.com only
    QString error;
    QList<Result> results = parseSubDl(readFixture("subdl-movie.json"), q, languageOf, &error);
    CHECK(error.isEmpty());
    CHECK(results.size() == 4);
    const Result *arabic = byId(results, "subdl:3197651-3213944");
    CHECK(arabic && arabic->language == QLatin1String("ar"));
    CHECK(arabic && arabic->url == QUrl(QStringLiteral("https://dl.subdl.com/subtitle/3197651-3213944.zip")));
    CHECK(arabic && arabic->release == QLatin1String("The.Matrix.1999.1080p.BluRay.x264-AMIABLE"));
    CHECK(arabic && arabic->downloads == 1200);
    const Result *brazil = byId(results, "subdl:3197652-3213945");
    CHECK(brazil && brazil->language == QLatin1String("pt-BR") && brazil->hearingImpaired);
    CHECK(byId(results, "subdl:3197653-3213946") && byId(results, "subdl:3197653-3213946")->language == QLatin1String("zh-TW"));
    // (no release: its name; "English" read from lang)
    const Result *english = byId(results, "subdl:3197655-3213947");
    CHECK(english && english->language == QLatin1String("en") && english->release == QLatin1String("The Matrix (English)"));
    for (const Result &r : results)
        CHECK(r.url.host() == QLatin1String("dl.subdl.com") && !r.url.path().contains(QLatin1String("..")));
    // an episode: this one, its season's pack, the range it is in
    results = parseSubDl(readFixture("subdl-episode.json"), episode(), languageOf, &error);
    CHECK(error.isEmpty());
    CHECK(results.size() == 4);
    CHECK(byId(results, "subdl:1-1") && byId(results, "subdl:1-3") && byId(results, "subdl:1-4") && byId(results, "subdl:1-7"));
    CHECK(!byId(results, "subdl:1-2") && !byId(results, "subdl:1-5") && !byId(results, "subdl:1-6"));
    // nothing found: no error; refused: why
    CHECK(parseSubDl(readFixture("subdl-not-found.json"), q, languageOf, &error).isEmpty());
    CHECK(error.isEmpty());
    CHECK(parseSubDl(readFixture("subdl-v1-key-refused.json"), q, languageOf, &error).isEmpty());
    CHECK(error == QLatin1String("Not Authorized"));
    error.clear();
    CHECK(parseSubDl(readFixture("subdl-v2-key-refused.json"), q, languageOf, &error).isEmpty());
    CHECK(error == QLatin1String("Not Authorized"));
}

static void testOpenSubtitlesCom()
{
    CHECK(openSubtitlesComCode(QStringLiteral("pt-BR")) == QLatin1String("pt-br"));
    CHECK(openSubtitlesComCode(QStringLiteral("zh")) == QLatin1String("zh-cn"));
    CHECK(openSubtitlesComCode(QStringLiteral("zh-TW")) == QLatin1String("zh-tw"));
    CHECK(openSubtitlesComCode(QStringLiteral("pt")) == QLatin1String("pt-pt"));
    CHECK(openSubtitlesComCode(QStringLiteral("es-419")) == QLatin1String("ea"));
    CHECK(openSubtitlesComCode(QStringLiteral("he")) == QLatin1String("he"));
    CHECK(languageOfOpenSubtitlesCom(QStringLiteral("pt-BR"), languageOf) == QLatin1String("pt-BR"));
    CHECK(languageOfOpenSubtitlesCom(QStringLiteral("zh-CN"), languageOf) == QLatin1String("zh"));
    CHECK(languageOfOpenSubtitlesCom(QStringLiteral("ea"), languageOf) == QLatin1String("es-419"));

    // its parameters in alphabetical order, the languages sorted
    QUrl url = openSubtitlesComSearchUrl(film());
    CHECK(url.toString(QUrl::RemoveQuery) == QLatin1String("https://api.opensubtitles.com/api/v1/subtitles"));
    CHECK(url.query() == QLatin1String("imdb_id=133093&languages=ar,pt-br,zh-tw&moviehash=8e245d9679d31e12")
          || url.query() == QLatin1String("imdb_id=133093&languages=ar%2Cpt-br%2Czh-tw&moviehash=8e245d9679d31e12"));
    url = openSubtitlesComSearchUrl(episode());
    const QStringList names = [&url]() {
        QStringList list;
        for (const auto &pair : QUrlQuery(url).queryItems())
            list.append(pair.first);
        return list;
    }();
    CHECK(names == (QStringList{ "episode_number", "languages", "parent_imdb_id", "season_number" }));
    CHECK(item(url, "parent_imdb_id") == QLatin1String("903747"));
    Query byName = film();
    byName.imdb.clear();
    byName.hash.clear();
    CHECK(item(openSubtitlesComSearchUrl(byName), "query") == QLatin1String("the matrix"));
    byName.title.clear();
    CHECK(openSubtitlesComSearchUrl(byName).isEmpty());
    CHECK(item(openSubtitlesComCheckUrl(), "imdb_id") == QLatin1String("133093"));

    // the subtitles: not those made by a machine nor without a file
    const QList<Result> results = parseOpenSubtitlesCom(readFixture("oscom-search.json"), languageOf);
    CHECK(results.size() == 4);
    const Result *brazil = byId(results, "opensubtitlescom:4567");
    CHECK(brazil && brazil->language == QLatin1String("pt-BR") && brazil->exact);
    CHECK(brazil && brazil->downloads == 1000 && brazil->fileName == QLatin1String("The.Matrix.1999.1080p.BluRay.x264.srt"));
    const Result *english = byId(results, "opensubtitlescom:4568");
    CHECK(english && english->hearingImpaired && english->release == QLatin1String("The.Matrix.1999.720p"));
    CHECK(english && brazil && english->rank < brazil->rank);
    const Result *chinese = byId(results, "opensubtitlescom:4569");
    CHECK(chinese && chinese->language == QLatin1String("zh") && chinese->release == QLatin1String("matrix.chs"));
    CHECK(byId(results, "opensubtitlescom:4570") && byId(results, "opensubtitlescom:4570")->language == QLatin1String("es-419"));
    CHECK(!byId(results, "opensubtitlescom:4571") && !byId(results, "opensubtitlescom:4572"));
    CHECK(parseOpenSubtitlesCom(readFixture("oscom-key-refused.json"), languageOf).isEmpty());

    // signed in: its token, the address of its members; another host refused
    const Session session = openSubtitlesComSession(readFixture("oscom-login.json"));
    CHECK(session.token == QLatin1String("eyJhbGciOiJIUzI1NiJ9.test.token"));
    CHECK(session.base == QUrl(QStringLiteral("https://vip-api.opensubtitles.com/api/v1/")));
    CHECK(openSubtitlesComSession(readFixture("oscom-login-other-host.json")).token.isEmpty());
    CHECK(openSubtitlesComSession(readFixture("oscom-key-refused.json")).token.isEmpty());
    CHECK(openSubtitlesComLoginBody(QStringLiteral("me"), QStringLiteral("p\"w")) == "{\"password\":\"p\\\"w\",\"username\":\"me\"}");
    CHECK(openSubtitlesComDownloadBody(QStringLiteral("4567")) == "{\"file_id\":4567}");
    // the link of a download: https only
    CHECK(openSubtitlesComLink(readFixture("oscom-download.json"))
          == QUrl(QStringLiteral("https://www.opensubtitles.com/download/ABCDEF/subfile/The.Matrix.srt")));
    CHECK(openSubtitlesComLink(readFixture("oscom-download-http.json")).isEmpty());
    CHECK(openSubtitlesComLink("not json").isEmpty());
}

static void testTitlovi()
{
    CHECK(titloviLanguage(QStringLiteral("hr")) == QLatin1String("Hrvatski"));
    CHECK(languageOfTitlovi(QStringLiteral("Cirilica")) == QLatin1String("sr"));
    CHECK(languageOfTitlovi(QStringLiteral("Slovenski")) == QLatin1String("sl"));
    CHECK(languageOfTitlovi(QStringLiteral("Klingon")).isEmpty());
    const QUrl login = titloviLoginUrl(QStringLiteral("me"), QStringLiteral("p&w"));
    CHECK(login.toString(QUrl::RemoveQuery) == QLatin1String("https://kodi.titlovi.com/api/subtitles/gettoken"));
    CHECK(item(login, "password") == QLatin1String("p&w") && item(login, "json") == QLatin1String("true"));
    const Session session = titloviSession(readFixture("titlovi-login.json"));
    CHECK(session.token == QLatin1String("0a1b2c3d-token") && session.userId == QLatin1String("12345"));
    CHECK(titloviSession("{}").token.isEmpty());

    // the user's Balkan languages, by their names, signed in
    Query q = episode();
    q.languages = { QStringLiteral("hr"), QStringLiteral("en"), QStringLiteral("sr") };
    QUrl url = titloviSearchUrl(q, session);
    CHECK(url.toString(QUrl::RemoveQuery) == QLatin1String("https://kodi.titlovi.com/api/subtitles/search"));
    CHECK(item(url, "lang") == QLatin1String("Hrvatski|Srpski"));
    CHECK(item(url, "query") == QLatin1String("Breaking Bad"));
    CHECK(item(url, "season") == QLatin1String("2"));
    CHECK(item(url, "imdbID") == QLatin1String("tt0903747"));
    CHECK(item(url, "token") == QLatin1String("0a1b2c3d-token") && item(url, "userid") == QLatin1String("12345"));
    Query english = q;
    english.languages = { QStringLiteral("en") };
    CHECK(titloviSearchUrl(english, session).isEmpty()); // (none of its own)

    // this episode, the whole season (Episode 0); not another one, nor elsewhere
    QList<Result> results = parseTitlovi(readFixture("titlovi-search.json"), q);
    CHECK(results.size() == 3);
    const Result *croatian = byId(results, "titlovi:101");
    CHECK(croatian && croatian->language == QLatin1String("hr") && croatian->release == QLatin1String("Breaking.Bad.S02E03.720p"));
    CHECK(croatian && croatian->url == QUrl(QStringLiteral("https://titlovi.com/download/?type=1&mediaid=101")));
    CHECK(byId(results, "titlovi:102") && byId(results, "titlovi:102")->language == QLatin1String("sr"));
    CHECK(byId(results, "titlovi:103") && byId(results, "titlovi:103")->release == QLatin1String("Breaking Bad"));
    CHECK(!byId(results, "titlovi:104") && !byId(results, "titlovi:105") && !byId(results, "titlovi:106"));
    // a film: every one of a known language and link
    CHECK(parseTitlovi(readFixture("titlovi-search.json"), film()).size() == 5);
}

static void testAssrt()
{
    Query q = film();
    q.languages = { QStringLiteral("zh-TW"), QStringLiteral("en") };
    QUrl url = assrtSearchUrl(q);
    CHECK(url.toString(QUrl::RemoveQuery) == QLatin1String("https://api.assrt.net/v1/sub/search"));
    CHECK(item(url, "q") == QLatin1String("The Matrix 1999"));
    CHECK(item(url, "token") == QLatin1String("my-key") && item(url, "is_file") == QLatin1String("1"));
    CHECK(item(assrtSearchUrl(episode()), "q") == QLatin1String("Breaking Bad S02E03"));

    QString error;
    bool refused = true;
    QList<Result> results = parseAssrt(readFixture("assrt-search.json"), q, &error, &refused);
    CHECK(error.isEmpty() && !refused);
    CHECK(results.size() == 3);
    // the user's first language it has
    CHECK(byId(results, "assrt:6001") && byId(results, "assrt:6001")->language == QLatin1String("en"));
    CHECK(byId(results, "assrt:6001") && byId(results, "assrt:6001")->release == QLatin1String("The.Matrix.1999.1080p.BluRay.x264"));
    const Result *traditional = byId(results, "assrt:6002");
    CHECK(traditional && traditional->language == QLatin1String("zh-TW") && traditional->release == QStringLiteral("黑客帝国"));
    CHECK(byId(results, "assrt:6004") && byId(results, "assrt:6004")->language == QLatin1String("zh"));
    CHECK(!byId(results, "assrt:6003"));
    CHECK(parseAssrt(readFixture("assrt-invalid-token.json"), q, &error, &refused).isEmpty());
    CHECK(refused && error == QLatin1String("invalid token"));

    CHECK(item(assrtDetailUrl(QStringLiteral("k"), QStringLiteral("6001")), "id") == QLatin1String("6001"));
    const QByteArray detail = readFixture("assrt-detail.json");
    CHECK(assrtFileOf(detail, QStringLiteral("en")).toString().endsWith(QLatin1String("The.Matrix.1999.eng.srt")));
    CHECK(assrtFileOf(detail, QStringLiteral("zh")).toString().endsWith(QLatin1String("The.Matrix.1999.chs.srt")));
    // (its file elsewhere is not taken: the first of its own)
    CHECK(assrtFileOf(detail, QStringLiteral("zh-TW")).toString().endsWith(QLatin1String("The.Matrix.1999.chs.srt")));
    CHECK(assrtFileOf("{}", QStringLiteral("zh")).isEmpty());
    CHECK(item(assrtCheckUrl(QStringLiteral("k")), "token") == QLatin1String("k"));
}

static void testJimaku()
{
    Query q = episode();
    q.title = QStringLiteral("Sousou no Frieren");
    q.season = 1;
    QUrl url = jimakuSearchUrl(q);
    CHECK(url.toString(QUrl::RemoveQuery) == QLatin1String("https://jimaku.cc/api/entries/search"));
    CHECK(item(url, "query") == QLatin1String("sousou no frieren") && item(url, "anime") == QLatin1String("true"));
    Query second = q;
    second.season = 2;
    CHECK(item(jimakuSearchUrl(second), "query") == QLatin1String("sousou no frieren 2"));
    // the series of that name (not its film)
    CHECK(jimakuEntryId(readFixture("jimaku-entries.json"), q) == QLatin1String("21"));
    Query movie = film();
    movie.title = QStringLiteral("Frieren: The Movie");
    CHECK(jimakuEntryId(readFixture("jimaku-entries.json"), movie) == QLatin1String("22"));
    CHECK(jimakuEntryId("[]", q).isEmpty());
    url = jimakuFilesUrl(QStringLiteral("21"), q);
    CHECK(url.toString(QUrl::RemoveQuery) == QLatin1String("https://jimaku.cc/api/entries/21/files"));
    CHECK(item(url, "episode") == QLatin1String("3"));
    CHECK(jimakuFilesUrl(QStringLiteral("../x"), q).isEmpty());
    // Japanese; no archives it can't open, no machine-made ones, none broken or elsewhere
    const QList<Result> results = parseJimaku(readFixture("jimaku-files.json"), q);
    CHECK(results.size() == 2);
    for (const Result &r : results)
        CHECK(r.language == QLatin1String("ja") && r.url.host() == QLatin1String("jimaku.cc"));
    CHECK(byId(results, "jimaku:[SubsPlease] Sousou no Frieren - 03.ja.ass"));
    CHECK(parseJimaku(readFixture("jimaku-unauthorized.json"), q).isEmpty());
    // (its downloads carry the user's key: never a link in clear text)
    QByteArray clear = readFixture("jimaku-files.json");
    clear.replace("https://jimaku.cc", "http://jimaku.cc");
    CHECK(parseJimaku(clear, q).isEmpty());
}

static void testBetaSeries()
{
    Query q = episode();
    QUrl url = betaSeriesShowUrl(q);
    CHECK(url.toString(QUrl::RemoveQuery) == QLatin1String("https://api.betaseries.com/shows/display"));
    CHECK(item(url, "imdb_id") == QLatin1String("tt0903747") && item(url, "key") == QLatin1String("my-key"));
    CHECK(betaSeriesShowUrl(film()).isEmpty()); // (series only)
    CHECK(betaSeriesShowId(readFixture("betaseries-show.json")) == QLatin1String("481"));
    url = betaSeriesEpisodeUrl(QStringLiteral("481"), q);
    CHECK(item(url, "id") == QLatin1String("481") && item(url, "season") == QLatin1String("2")
          && item(url, "episode") == QLatin1String("3") && item(url, "subtitles") == QLatin1String("1"));
    QString error;
    bool refused = true;
    const QList<Result> results = parseBetaSeries(readFixture("betaseries-episodes.json"), &error, &refused);
    CHECK(error.isEmpty() && !refused);
    CHECK(results.size() == 2);
    CHECK(byId(results, "betaseries:9001") && byId(results, "betaseries:9001")->language == QLatin1String("fr"));
    CHECK(byId(results, "betaseries:9002") && byId(results, "betaseries:9002")->language == QLatin1String("en"));
    CHECK(byId(results, "betaseries:9001") && byId(results, "betaseries:9001")->release == QLatin1String("Breaking.Bad.S02E03.720p.fr"));
    CHECK(parseBetaSeries(readFixture("betaseries-bad-key.json"), &error, &refused).isEmpty());
    CHECK(refused && error == QStringLiteral("Mauvaise clé API."));
    error.clear();
    CHECK(parseBetaSeries(readFixture("betaseries-not-found.json"), &error, &refused).isEmpty());
    CHECK(!refused && error.isEmpty());
}

static void testSubsRo()
{
    QUrl url = subsRoSearchUrl(film());
    CHECK(url.toString(QUrl::RemoveQuery) == QLatin1String("https://api.subs.ro/v1.0/search/imdbid/0133093"));
    CHECK(item(url, "language") == QLatin1String("ro"));
    Query unknown = film();
    unknown.imdb.clear();
    CHECK(subsRoSearchUrl(unknown).isEmpty());
    QString error;
    QList<Result> results = parseSubsRo(readFixture("subsro-search.json"), film(), &error);
    CHECK(error.isEmpty() && results.size() == 3);
    const Result *matrix = byId(results, "subsro:501");
    CHECK(matrix && matrix->language == QLatin1String("ro") && matrix->release == QLatin1String("The.Matrix.1999.1080p.BluRay.x264"));
    CHECK(matrix && matrix->downloads == 1200);
    // an episode: its season's
    results = parseSubsRo(readFixture("subsro-search.json"), episode(), &error);
    CHECK(byId(results, "subsro:502") && !byId(results, "subsro:503") && !byId(results, "subsro:504"));
    CHECK(parseSubsRo(readFixture("subsro-invalid-key.json"), film(), &error).isEmpty());
    CHECK(error == QLatin1String("Invalid API key"));
    // (its downloads carry the user's key: never a link in clear text)
    QByteArray clear = readFixture("subsro-search.json");
    clear.replace("https://subs.ro", "http://subs.ro");
    error.clear();
    CHECK(parseSubsRo(clear, film(), &error).isEmpty() && error.isEmpty());
}

static void testNapisy24()
{
    Query q = film();
    q.size = 734003200;
    const QByteArray body = napisy24Body(q, QStringLiteral("me@x"), QStringLiteral("p&w"));
    CHECK(body.contains("postAction=CheckSub") && body.contains("fh=8e245d9679d31e12") && body.contains("fs=734003200"));
    CHECK(body.contains("ua=me%40x") && body.contains("ap=p%26w") && body.contains("n24pref=1"));
    CHECK(body.contains("fn=The.Matrix.1999.1080p.BluRay.x264.mkv"));
    const Napisy24Answer found = parseNapisy24(readFixture("napisy24-found.bin"));
    CHECK(found.found && !found.loginRefused);
    CHECK(found.imdb == QLatin1String("tt0133093") && found.title == QLatin1String("The Matrix"));
    CHECK(found.archive.startsWith("PK\x03\x04"));
    const Napisy24Answer refused = parseNapisy24(readFixture("napisy24-login-error.txt"));
    CHECK(refused.loginRefused && !refused.found);
    const Napisy24Answer info = parseNapisy24(readFixture("napisy24-video-info.txt"));
    CHECK(!info.found && !info.loginRefused && info.archive.isEmpty());
}

static void testLegendas()
{
    CHECK(legendasLoginBody(QStringLiteral("me@x.org"), QStringLiteral("p\"w")) == "{\"email\":\"me@x.org\",\"password\":\"p\\\"w\"}");
    CHECK(legendasSession(readFixture("legendas-login.json")).token == QLatin1String("eyJhbGciOiJIUzI1NiJ9.legendas.token"));
    CHECK(legendasSession(readFixture("legendas-login-refused.json")).token.isEmpty());
    CHECK(legendasSearchUrl(film()) == QUrl(QStringLiteral("https://legendas.net/api/v1/search/movie")));
    CHECK(legendasSearchUrl(episode()) == QUrl(QStringLiteral("https://legendas.net/api/v1/search/tv")));
    CHECK(legendasSearchBody(film()) == "{\"imdb_id\":\"tt0133093\",\"page\":1,\"per_page\":25}");
    CHECK(legendasSearchBody(episode()) == "{\"imdb_id\":\"tt0903747\",\"page\":1,\"per_page\":25,\"tv_episode\":3,\"tv_season\":2}");
    QList<Result> results = parseLegendas(readFixture("legendas-movies.json"), film());
    CHECK(results.size() == 2);
    const Result *matrix = byId(results, "legendasnet:701");
    CHECK(matrix && matrix->language == QLatin1String("pt-BR")
          && matrix->url == QUrl(QStringLiteral("https://legendas.net/uploads/legendas/the-matrix-1999.zip")));
    CHECK(byId(results, "legendasnet:704") && byId(results, "legendasnet:704")->foreignOnly);
    results = parseLegendas(readFixture("legendas-tv.json"), episode());
    CHECK(results.size() == 1 && byId(results, "legendasnet:801"));
}

// ---------------------------------------------------------------------------
// Answers that aren't the service's: its failure, never "nothing found"

// What subtitle_providers.cpp makes of an answer of 200 (readAnswer, then
// readFailed): Failed when it isn't the service's (a page where JSON comes, a
// check of the browser, nothing, JSON cut short) or says an error in place
// of its results; else its results, none being "nothing found"
enum class Outcome { Failed, Nothing, Found };
static Outcome outcomeOf(const QByteArray &body, bool json, const std::function<QList<Result>(QString *)> &read)
{
    if (answerKind(body, json) != AnswerKind::Readable)
        return Outcome::Failed;
    QString error;
    const QList<Result> results = read(&error);
    if (!error.isEmpty())
        return Outcome::Failed;
    return results.isEmpty() ? Outcome::Nothing : Outcome::Found;
}

static void testAnswers()
{
    const QByteArray errorPage = readFixture("error-page.html");
    const QByteArray cloudflare = readFixture("cloudflare-challenge.html");
    const QByteArray ddosGuard = readFixture("ddos-guard.html");
    const QByteArray yifyEmpty = readFixture("yify-empty.html");

    // what an answer is
    CHECK(answerKind(errorPage, true) == AnswerKind::Page);
    CHECK(answerKind(errorPage, false) == AnswerKind::Readable); // (a page where a page comes: read)
    CHECK(answerKind(cloudflare, true) == AnswerKind::Challenge);
    CHECK(answerKind(cloudflare, false) == AnswerKind::Challenge);
    CHECK(answerKind(ddosGuard, true) == AnswerKind::Challenge);
    CHECK(answerKind(ddosGuard, false) == AnswerKind::Challenge);
    // (a page that loads a script of cdnjs.cloudflare.com is no challenge)
    CHECK(answerKind(yifyEmpty, false) == AnswerKind::Readable);
    CHECK(answerKind(readFixture("yify-movie.html"), false) == AnswerKind::Readable);
    CHECK(answerKind(QByteArray(), true) == AnswerKind::Empty);
    CHECK(answerKind(QByteArray(), false) == AnswerKind::Empty);
    CHECK(answerKind(" \r\n\t", true) == AnswerKind::Empty);
    CHECK(answerKind("\xEF\xBB\xBF\n", true) == AnswerKind::Empty);
    CHECK(answerKind("\xEF\xBB\xBF {\"data\":[]}\r\n", true) == AnswerKind::Readable);
    CHECK(answerKind("[]", true) == AnswerKind::Readable);
    // (cut short, even just after a "}"; its brackets counted, not those of its texts)
    CHECK(answerKind("{\"data\":[{\"id\":1}", true) == AnswerKind::Unreadable);
    CHECK(answerKind("{\"data\":[{\"id\":1},", true) == AnswerKind::Unreadable);
    CHECK(answerKind("{\"data\":[1,2}", true) == AnswerKind::Unreadable);
    CHECK(answerKind("{\"a\":1}}", true) == AnswerKind::Unreadable);
    CHECK(answerKind("{\"a\":1}{\"b\":2}", true) == AnswerKind::Unreadable);
    CHECK(answerKind("{\"a\":\"}\"", true) == AnswerKind::Unreadable);
    CHECK(answerKind("{\"a\":\"}\"}", true) == AnswerKind::Readable);
    CHECK(answerKind("{\"a\":\"\\\"}\"}", true) == AnswerKind::Readable);
    CHECK(answerKind("{\"a\":\"\\\\\"}", true) == AnswerKind::Readable);
    CHECK(answerKind("[{\"a\":[]},{\"b\":{}}]\n", true) == AnswerKind::Readable);
    CHECK(answerKind(readFixture("oscom-search.json").chopped(2), true) == AnswerKind::Unreadable);
    // every answer of the fixtures cut anywhere: never readable
    {
        int cuts = 0, readable = 0;
        for (const char *name : { "oscom-search.json", "titlovi-search.json", "subdl-movie.json", "subsource-subtitles.json",
                                  "jimaku-files.json", "betaseries-episodes.json", "legendas-movies.json", "assrt-search.json" })
        {
            const QByteArray whole = readFixture(name);
            CHECK(answerKind(whole, true) == AnswerKind::Readable && !QJsonDocument::fromJson(whole).isNull());
            const QByteArray body = whole.trimmed();
            for (qsizetype n = 1; n < body.size(); ++n, ++cuts)
                if (answerKind(body.left(n), true) == AnswerKind::Readable)
                    ++readable;
        }
        CHECK(cuts > 10000 && readable == 0);
    }
    CHECK(answerKind("Service Unavailable", true) == AnswerKind::Unreadable);
    CHECK(answerKind("Service Unavailable", false) == AnswerKind::Readable);
    // (JSON that names a captcha is JSON: its message is read)
    CHECK(answerKind("{\"message\":\"g-recaptcha required\"}", true) == AnswerKind::Readable);
    // the real empty answers of each service stay readable
    for (const char *name : { "subsource-empty.json", "subdl-not-found.json", "oscom-empty.json", "titlovi-empty.json",
                              "jimaku-empty.json", "betaseries-not-found.json", "legendas-none.json",
                              "legendas-empty.json", "assrt-search.json", "subsro-search.json" })
    {
        const bool readable = answerKind(readFixture(name), true) == AnswerKind::Readable;
        if (!readable)
            std::fprintf(stderr, "not readable: %s\n", name);
        CHECK(readable);
    }

    // the error a JSON answer says, in each service's shape
    const auto doc = [](const QByteArray &body) { return QJsonDocument::fromJson(body); };
    // results as an array (OpenSubtitles.org, Wizdom, Jimaku): an object is an error
    CHECK(answerError(doc(readFixture("opensubtitles-error.json")), true) == QLatin1String("Service Temporarily Unavailable"));
    CHECK(answerError(doc(readFixture("wizdom-error.json")), true) == QLatin1String("Server Error"));
    CHECK(answerError(doc(readFixture("jimaku-unauthorized.json")), true) == QLatin1String("unauthorized"));
    CHECK(answerError(doc(readFixture("jimaku-empty.json")), true).isEmpty());
    CHECK(answerError(doc("{}"), true) == QLatin1String("?"));
    CHECK(answerError(doc("not json"), true) == QLatin1String("?"));
    // results in an object's key (Gestdown for Addic7ed, OpenSubtitles.com, Titlovi)
    CHECK(answerError(doc(readFixture("addic7ed-error.json")), false, "shows")
          == QLatin1String("An error occurred while processing your request."));
    CHECK(answerError(doc("{\"shows\":[]}"), false, "shows").isEmpty());
    CHECK(answerError(doc("{\"matchingSubtitles\":[],\"episode\":null}"), false, "matchingSubtitles").isEmpty());
    CHECK(answerError(doc("{\"shows\":[],\"message\":\"cached\"}"), false, "shows").isEmpty()); // (its results)
    CHECK(answerError(doc("{}"), false, "shows").isEmpty());              // (no results, no error: nothing)
    CHECK(answerError(doc("[]"), false, "shows") == QLatin1String("?"));  // (not its shape)
    CHECK(answerError(doc("not json"), false, "shows") == QLatin1String("?"));
    CHECK(answerError(doc(readFixture("titlovi-error.json")), false, "SubtitleResults") == QLatin1String("An error has occurred."));
    CHECK(answerError(doc("{\"error\":{\"code\":403,\"message\":\"Quota exceeded\"}}"), false, "data") == QLatin1String("Quota exceeded"));
    CHECK(answerError(doc("{\"errors\":[{\"code\":4001,\"text\":\"Show not found.\"}]}"), false, "shows")
          == QLatin1String("Show not found."));
    CHECK(answerError(doc("{\"errors\":[{\"message\":\"Bad request\"}]}"), false, "data") == QLatin1String("Bad request"));
    CHECK(answerError(doc("{\"detail\":\"Not authenticated\"}"), false, "data") == QLatin1String("Not authenticated"));
    CHECK(answerError(doc("{\"errmsg\":\"server busy\",\"status\":30900}"), false, "sub") == QLatin1String("server busy"));
    CHECK(answerError(doc("{\"status\":500}"), false, "data") == QLatin1String("?"));
    CHECK(answerError(doc("{\"status\":false}"), false, "data") == QLatin1String("?"));
    CHECK(answerError(doc("{\"success\":false}"), false, "data") == QLatin1String("?"));
    CHECK(answerError(doc("{\"status\":200,\"success\":true}"), false, "data").isEmpty());
    CHECK(answerError(doc("{\"message\":\"" + QByteArray(500, 'x') + "\"}"), false, "data").size() == 200);

    // each parser: the error in place of its results, none for a real empty answer
    QString error;
    CHECK(parseOpenSubtitlesCom(readFixture("oscom-empty.json"), languageOf, &error).isEmpty() && error.isEmpty());
    CHECK(!parseOpenSubtitlesCom(readFixture("oscom-search.json"), languageOf, &error).isEmpty() && error.isEmpty());
    CHECK(parseOpenSubtitlesCom(readFixture("oscom-key-refused.json"), languageOf, &error).isEmpty()
          && error == QLatin1String("You cannot consume this service"));
    CHECK(parseTitlovi(readFixture("titlovi-error.json"), film(), &error).isEmpty() && error == QLatin1String("An error has occurred."));
    CHECK(parseTitlovi(readFixture("titlovi-empty.json"), film(), &error).isEmpty() && error.isEmpty());
    CHECK(!parseTitlovi(readFixture("titlovi-search.json"), film(), &error).isEmpty() && error.isEmpty());
    Query anime = film();
    anime.title = QStringLiteral("Cowboy Bebop");
    CHECK(jimakuEntryId(readFixture("jimaku-empty.json"), anime, &error).isEmpty() && error.isEmpty());
    CHECK(jimakuEntryId(readFixture("jimaku-unauthorized.json"), anime, &error).isEmpty() && error == QLatin1String("unauthorized"));
    CHECK(parseJimaku(readFixture("jimaku-empty.json"), anime, &error).isEmpty() && error.isEmpty());
    CHECK(parseJimaku("{\"error\":\"Too many requests\"}", anime, &error).isEmpty() && error == QLatin1String("Too many requests"));
    CHECK(parseLegendas(readFixture("legendas-error.json"), film(), &error).isEmpty()
          && error == QLatin1String("Too many requests, try again later"));
    error.clear();
    CHECK(parseLegendas(readFixture("legendas-none.json"), film(), &error).isEmpty() && error.isEmpty());
    CHECK(parseLegendas(readFixture("legendas-empty.json"), film(), &error).isEmpty() && error.isEmpty());
    CHECK(parseLegendas("{\"detail\":\"Internal error\"}", film(), &error).isEmpty() && error == QLatin1String("Internal error"));
    error.clear();
    CHECK(!parseLegendas(readFixture("legendas-movies.json"), film(), &error).isEmpty() && error.isEmpty());
    CHECK(parseSubDl(readFixture("subdl-error.json"), film(), languageOf, &error).isEmpty()
          && error == QLatin1String("rate limit exceeded"));

    // what the search makes of each: a failure, nothing found, results
    const auto oscom = [](const QByteArray &body) {
        return [body](QString *e) { return parseOpenSubtitlesCom(body, languageOf, e); };
    };
    const auto titlovi = [](const QByteArray &body) { return [body](QString *e) { return parseTitlovi(body, film(), e); }; };
    const auto legendas = [](const QByteArray &body) { return [body](QString *e) { return parseLegendas(body, film(), e); }; };
    const auto subDl = [](const QByteArray &body) {
        return [body](QString *e) { return parseSubDl(body, film(), languageOf, e); };
    };
    const auto subSource = [](const QByteArray &body) {
        return [body](QString *e) {
            const QString id = subSourceMovieId(body, film(), e);
            return id.isEmpty() ? QList<Result>() : QList<Result>{ Result() };
        };
    };
    const auto jimaku = [anime](const QByteArray &body) { return [body, anime](QString *e) { return parseJimaku(body, anime, e); }; };
    const auto betaSeries = [](const QByteArray &body) {
        return [body](QString *e) { bool refused = false; return parseBetaSeries(body, e, &refused); };
    };
    const auto yify = [](const QByteArray &body) {
        return [body](QString *e) {
            if (!isYifyPage(body)) // (subtitle_providers.cpp: an error page of a server, not its own)
                *e = QStringLiteral("?");
            return parseYify(body, languageOf);
        };
    };
    // an HTML error page where JSON comes, for every service that answers JSON
    CHECK(outcomeOf(errorPage, true, oscom(errorPage)) == Outcome::Failed);
    CHECK(outcomeOf(errorPage, true, titlovi(errorPage)) == Outcome::Failed);
    CHECK(outcomeOf(errorPage, true, legendas(errorPage)) == Outcome::Failed);
    CHECK(outcomeOf(errorPage, true, subDl(errorPage)) == Outcome::Failed);
    CHECK(outcomeOf(errorPage, true, subSource(errorPage)) == Outcome::Failed);
    CHECK(outcomeOf(errorPage, true, jimaku(errorPage)) == Outcome::Failed);
    CHECK(outcomeOf(errorPage, true, betaSeries(errorPage)) == Outcome::Failed);
    // (where a page comes, YIFY: a page not its own)
    CHECK(outcomeOf(errorPage, false, yify(errorPage)) == Outcome::Failed);
    // a check of the browser, wherever it comes
    CHECK(outcomeOf(cloudflare, true, oscom(cloudflare)) == Outcome::Failed);
    CHECK(outcomeOf(cloudflare, false, yify(cloudflare)) == Outcome::Failed);
    CHECK(outcomeOf(ddosGuard, true, subSource(ddosGuard)) == Outcome::Failed);
    CHECK(outcomeOf(ddosGuard, false, yify(ddosGuard)) == Outcome::Failed);
    // nothing, a 200 with no body, where JSON comes
    CHECK(outcomeOf(QByteArray(), true, oscom(QByteArray())) == Outcome::Failed);
    CHECK(outcomeOf(QByteArray(), true, jimaku(QByteArray())) == Outcome::Failed);
    CHECK(outcomeOf("\r\n", true, legendas("\r\n")) == Outcome::Failed);
    // JSON cut short, or a server's text
    CHECK(outcomeOf("{\"data\":[{\"id\":", true, oscom("{\"data\":[{\"id\":")) == Outcome::Failed);
    CHECK(outcomeOf("Bad Gateway", true, titlovi("Bad Gateway")) == Outcome::Failed);
    // JSON whole but not JSON: each parser says so
    const QByteArray broken = "{\"data\":}";
    CHECK(answerKind(broken, true) == AnswerKind::Readable);
    CHECK(outcomeOf(broken, true, oscom(broken)) == Outcome::Failed);
    CHECK(outcomeOf(broken, true, titlovi(broken)) == Outcome::Failed);
    CHECK(outcomeOf(broken, true, legendas(broken)) == Outcome::Failed);
    CHECK(outcomeOf(broken, true, subDl(broken)) == Outcome::Failed);
    CHECK(outcomeOf(broken, true, subSource(broken)) == Outcome::Failed);
    CHECK(outcomeOf(broken, true, jimaku(broken)) == Outcome::Failed);
    CHECK(outcomeOf(broken, true, betaSeries(broken)) == Outcome::Failed);
    CHECK(subSourceMovieId("{\"success\":false}", film(), &error).isEmpty() && error == QLatin1String("?"));
    {
        bool refused = true;
        CHECK(parseBetaSeries(broken, &error, &refused).isEmpty() && error == QLatin1String("?") && !refused);
        error.clear();
        CHECK(parseAssrt(broken, film(), &error, &refused).isEmpty() && error == QLatin1String("?") && !refused);
        error.clear();
        CHECK(parseSubsRo(broken, film(), &error).isEmpty() && error == QLatin1String("?"));
        error.clear();
        CHECK(parseSubSource(broken, film(), languageOf, &error).isEmpty() && error == QLatin1String("?"));
        error.clear();
    }
    // a JSON error in place of the results, in each shape
    const QByteArray titloviError = readFixture("titlovi-error.json"), legendasError = readFixture("legendas-error.json");
    const QByteArray subDlError = readFixture("subdl-error.json"), oscomError = readFixture("oscom-key-refused.json");
    CHECK(outcomeOf(titloviError, true, titlovi(titloviError)) == Outcome::Failed);
    CHECK(outcomeOf(legendasError, true, legendas(legendasError)) == Outcome::Failed);
    CHECK(outcomeOf(subDlError, true, subDl(subDlError)) == Outcome::Failed);
    CHECK(outcomeOf(oscomError, true, oscom(oscomError)) == Outcome::Failed);
    CHECK(outcomeOf("{\"error\":\"unauthorized\"}", true, jimaku("{\"error\":\"unauthorized\"}")) == Outcome::Failed);
    // the real empty answers: nothing found
    const QByteArray oscomEmpty = readFixture("oscom-empty.json"), titloviEmpty = readFixture("titlovi-empty.json");
    const QByteArray legendasNone = readFixture("legendas-none.json"), legendasEmpty = readFixture("legendas-empty.json");
    const QByteArray subDlNone = readFixture("subdl-not-found.json"), subSourceNone = readFixture("subsource-empty.json");
    const QByteArray jimakuNone = readFixture("jimaku-empty.json"), betaSeriesNone = readFixture("betaseries-not-found.json");
    CHECK(outcomeOf(oscomEmpty, true, oscom(oscomEmpty)) == Outcome::Nothing);
    CHECK(outcomeOf(titloviEmpty, true, titlovi(titloviEmpty)) == Outcome::Nothing);
    CHECK(outcomeOf(legendasNone, true, legendas(legendasNone)) == Outcome::Nothing);
    CHECK(outcomeOf(legendasEmpty, true, legendas(legendasEmpty)) == Outcome::Nothing);
    CHECK(outcomeOf(subDlNone, true, subDl(subDlNone)) == Outcome::Nothing);
    CHECK(outcomeOf(subSourceNone, true, subSource(subSourceNone)) == Outcome::Nothing);
    CHECK(outcomeOf(jimakuNone, true, jimaku(jimakuNone)) == Outcome::Nothing);
    CHECK(outcomeOf(betaSeriesNone, true, betaSeries(betaSeriesNone)) == Outcome::Nothing);
    CHECK(outcomeOf(yifyEmpty, false, yify(yifyEmpty)) == Outcome::Nothing);
    // and results stay results
    const QByteArray oscomFound = readFixture("oscom-search.json"), yifyFound = readFixture("yify-movie.html");
    CHECK(outcomeOf(oscomFound, true, oscom(oscomFound)) == Outcome::Found);
    CHECK(outcomeOf(yifyFound, false, yify(yifyFound)) == Outcome::Found);

    // YIFY's own page, or another
    CHECK(isYifyPage(yifyEmpty) && isYifyPage(readFixture("yify-movie.html")));
    CHECK(!isYifyPage(errorPage) && !isYifyPage(cloudflare));
    CHECK(parseYify(yifyEmpty, languageOf).isEmpty());

    // Napisy24: its own answers, or not one
    const Napisy24Answer page = parseNapisy24(readFixture("napisy24-error.html"));
    CHECK(page.unreadable && !page.found && !page.loginRefused);
    CHECK(parseNapisy24(QByteArray()).unreadable);
    CHECK(parseNapisy24(cloudflare).unreadable);
    CHECK(!parseNapisy24(readFixture("napisy24-video-info.txt")).unreadable);
    CHECK(!parseNapisy24("OK-0|").unreadable && !parseNapisy24("OK-0|").found);
    CHECK(!parseNapisy24(readFixture("napisy24-found.bin")).unreadable);
    const Napisy24Answer refused = parseNapisy24(readFixture("napisy24-login-error.txt"));
    CHECK(refused.loginRefused && !refused.unreadable); // (its own refusal kept)

    // a download that is a page or a JSON error, not a subtitle
    CHECK(isErrorDocument(errorPage) && isErrorDocument(cloudflare) && isErrorDocument(ddosGuard));
    CHECK(isErrorDocument(readFixture("opensubtitles-error.json")) && isErrorDocument("[]"));
    CHECK(isErrorDocument("\xEF\xBB\xBF\r\n<!DOCTYPE html><html><body>Not Found</body></html>"));
    CHECK(!isErrorDocument("1\r\n00:00:01,000 --> 00:00:02,000\r\nHello\r\n"));
    CHECK(!isErrorDocument("\xEF\xBB\xBF" "1\n00:00:01,000 --> 00:00:02,000\n[Music]\n"));
    CHECK(!isErrorDocument("WEBVTT\n\n00:01.000 --> 00:02.000\nHello\n"));
    CHECK(!isErrorDocument("{1}{25}Hello|world\n{30}{50}{y:i}Again\n"));
    CHECK(!isErrorDocument("[Script Info]\nTitle: x\nScriptType: v4.00+\n"));
    CHECK(!isErrorDocument("<SAMI><HEAD><TITLE>x</TITLE></HEAD><BODY><SYNC Start=0><P>Hi</BODY></SAMI>"));
    CHECK(!isErrorDocument("<?xml version=\"1.0\"?><tt xmlns=\"http://www.w3.org/ns/ttml\"></tt>"));
    CHECK(!isErrorDocument(QByteArray()));
}

// ---------------------------------------------------------------------------
// YIFY: its page read in one pass

// The page as it was read before (subtitle_providers.cpp before 0.6.0): the
// same expressions, over the whole page. Its results are what parseYify must
// give
static QList<Result> yifyByExpressions(const QByteArray &body)
{
    static const QRegularExpression rowRe(QStringLiteral(R"re(<tr data-id="(\d+)">(.*?)</tr>)re"),
                                          QRegularExpression::DotMatchesEverythingOption);
    static const QRegularExpression ratingRe(QStringLiteral(R"re(<span class="label[^"]*">\s*(-?\d+)\s*</span>)re"));
    static const QRegularExpression languageRe(QStringLiteral(R"re(<span class="sub-lang">([^<]+)</span>)re"));
    static const QRegularExpression linkRe(QStringLiteral(
        R"re(<a href="/subtitles/([A-Za-z0-9._\-]+)">\s*(?:<span class="text-muted">[^<]*</span>)?\s*([^<]*)</a>)re"));
    const QString page = QString::fromUtf8(body);
    QList<Result> results;
    for (QRegularExpressionMatchIterator it = rowRe.globalMatch(page); it.hasNext();)
    {
        const QRegularExpressionMatch row = it.next();
        const QString cells = row.captured(2);
        const QRegularExpressionMatch link = linkRe.match(cells);
        const QRegularExpressionMatch language = languageRe.match(cells);
        if (!link.hasMatch() || !language.hasMatch())
            continue;
        Result r;
        r.id = QStringLiteral("yify:") + row.captured(1);
        r.provider = QStringLiteral("yify");
        r.language = languageOf(language.captured(1));
        if (r.language.isEmpty())
            continue;
        r.release = link.captured(2).trimmed();
        r.url = QUrl(QStringLiteral("https://yifysubtitles.ch/subtitle/") + link.captured(1) + QStringLiteral(".zip"));
        r.hearingImpaired = cells.contains(QLatin1String("hi-subtitle"));
        const QRegularExpressionMatch rating = ratingRe.match(cells);
        // (a rating of more digits than a double holds: 0, as parseYify)
        const double value = rating.hasMatch() ? rating.captured(1).toDouble() : 0.;
        r.rank = std::isfinite(value) ? std::min(value, 30.) * 0.5 : 0.;
        results.append(r);
    }
    return results;
}

static bool sameResults(const QList<Result> &a, const QList<Result> &b)
{
    if (a.size() != b.size())
        return false;
    for (qsizetype i = 0; i < a.size(); ++i)
        if (a[i].id != b[i].id || a[i].provider != b[i].provider || a[i].language != b[i].language
            || a[i].release != b[i].release || a[i].url != b[i].url
            || a[i].hearingImpaired != b[i].hearingImpaired || a[i].rank != b[i].rank)
            return false;
    return true;
}

// (where a byte starts a character: cuts and insertions there keep the
// page's UTF-8 whole, as the two readings decode it alike)
static bool charStart(const QByteArray &page, qsizetype at)
{
    return at >= page.size() || (quint8(page.at(at)) & 0xC0) != 0x80;
}

// how long parseYify takes over a page, in ms (and its results)
static qint64 timedYify(const QByteArray &page, QList<Result> *results = nullptr)
{
    QElapsedTimer timer;
    timer.start();
    const QList<Result> found = parseYify(page, languageOf);
    if (results)
        *results = found;
    return timer.elapsed();
}

static void testYify()
{
    const QByteArray page = readFixture("yify-movie.html");
    const QList<Result> results = parseYify(page, languageOf);
    CHECK(results.size() == 5);
    const Result *hebrew = byId(results, "yify:101");
    CHECK(hebrew && hebrew->provider == QLatin1String("yify") && hebrew->language == QLatin1String("he"));
    CHECK(hebrew && hebrew->release == QLatin1String("The.Matrix.1999.1080p.BluRay.x264-YIFY"));
    CHECK(hebrew && hebrew->url == QUrl(QStringLiteral("https://yifysubtitles.ch/subtitle/the-matrix-1999-hebrew-yify-101.zip")));
    CHECK(hebrew && hebrew->rank == 6. && !hebrew->hearingImpaired);
    const Result *english = byId(results, "yify:102");
    CHECK(english && english->language == QLatin1String("en") && english->hearingImpaired && english->rank == -1.5);
    CHECK(english && english->release == QLatin1String("The Matrix 1999 720p WEB"));
    // a link that isn't one, then its own; the rating at most 30
    const Result *arabic = byId(results, "yify:105");
    CHECK(arabic && arabic->language == QLatin1String("ar") && arabic->release == QStringLiteral("الماتريكس 1999"));
    CHECK(arabic && arabic->url.path().endsWith(QLatin1String("arabic-yify-105.zip")) && arabic->rank == 15.);
    // a label without its quote, a muted label without its end: the next ones
    const Result *german = byId(results, "yify:107");
    CHECK(german && german->language == QLatin1String("de") && german->rank == 3.5);
    CHECK(german && german->release == QLatin1String("Matrix.German")
          && german->url.path().endsWith(QLatin1String("german-yify-107b.zip")));
    CHECK(byId(results, "yify:108") && byId(results, "yify:108")->language == QLatin1String("pt-BR")
          && byId(results, "yify:108")->rank == 0.);
    // an unknown language, no link, not a row, a row never closed
    CHECK(!byId(results, "yify:103") && !byId(results, "yify:104") && !byId(results, "yify:106")
          && !byId(results, "yify:109"));
    CHECK(parseYify({}, languageOf).isEmpty() && parseYify("<tr data-id=\"", languageOf).isEmpty());

    // What the expressions found, on the page and on thousands of pages
    // made of it (fixed seed): cut, doubled, with pieces of rows put in
    CHECK(sameResults(results, yifyByExpressions(page)));
    static const char *const pieces[] = {
        "</tr>", "<tr data-id=\"7\">", "<tr data-id=\"", "\"", "\">", "<", "</span>", "<span class=\"label",
        "<span class=\"label x\">", "<span class=\"sub-lang\">", "<span class=\"sub-lang\">English</span>",
        "<a href=\"/subtitles/", "<a href=\"/subtitles/s-1\">", "<span class=\"text-muted\">", "</a>", " ", "\n",
        "x", "5", "-", "hi-subtitle", "<tr data-id=\"55\"><a href=\"/subtitles/z\">Z</a><span class=\"sub-lang\">Arabic</span></tr>",
    };
    std::mt19937 random(5);
    const auto boundary = [&random](const QByteArray &text) {
        for (;;)
        {
            const qsizetype at = qsizetype(random() % quint32(text.size() + 1));
            if (charStart(text, at))
                return at;
        }
    };
    int different = 0;
    for (int n = 0; n < 3000; ++n)
    {
        QByteArray variant = page;
        const int steps = 1 + int(random() % 4);
        for (int s = 0; s < steps; ++s)
        {
            const qsizetype a = boundary(variant);
            switch (random() % 3)
            {
                case 0: // a part cut
                {
                    qsizetype b = std::min(variant.size(), a + 1 + qsizetype(random() % 60));
                    while (!charStart(variant, b))
                        ++b;
                    variant.remove(a, b - a);
                    break;
                }
                case 1: // a piece of a row
                    variant.insert(a, pieces[random() % (sizeof(pieces) / sizeof(*pieces))]);
                    break;
                default: // a part doubled
                {
                    qsizetype b = std::min(variant.size(), a + 1 + qsizetype(random() % 200));
                    while (!charStart(variant, b))
                        ++b;
                    variant.insert(b, variant.mid(a, b - a));
                    break;
                }
            }
        }
        if (!sameResults(parseYify(variant, languageOf), yifyByExpressions(variant)))
        {
            if (++different == 1)
                std::fprintf(stderr, "YIFY: another reading than the expressions' of:\n%s\n", variant.constData());
        }
    }
    CHECK(different == 0);

    // Hostile pages of 1.5 to 2.9 MB (the page is asked for up to 2 MB):
    // milliseconds each, the bound only catches a reading that would go
    // back over the page
    QList<Result> found;
    CHECK(timedYify(QByteArray("<tr data-id=\"1\">").repeated(150000), &found) < 2000 && found.isEmpty());
    CHECK(timedYify(QByteArray("<tr data-id=\"").repeated(180000), &found) < 2000 && found.isEmpty());
    CHECK(timedYify(QByteArray("<tr data-id=\"12345678901234567890").repeated(80000), &found) < 2000 && found.isEmpty());
    const QByteArray row("<tr data-id=\"1\">");
    CHECK(timedYify(row + QByteArray("<span class=\"label").repeated(100000) + "</tr>") < 2000);
    CHECK(timedYify(row + QByteArray("<span class=\"label x").repeated(80000) + "\"</tr>") < 2000);
    CHECK(timedYify(row + QByteArray("<a href=\"/subtitles/a\">").repeated(80000) + "</tr>") < 2000);
    CHECK(timedYify(row + QByteArray("<a href=\"/subtitles/").repeated(80000) + "</tr>") < 2000);
    CHECK(timedYify(row + QByteArray("<span class=\"sub-lang\">x").repeated(80000) + "</tr>") < 2000);
    CHECK(timedYify(row + "<a href=\"/subtitles/a\">" + QByteArray("<span class=\"text-muted\">").repeated(80000)
                    + "</tr>") < 2000);
    // (and many good rows: all of them)
    const QByteArray good("<tr data-id=\"9\"><span class=\"label a\">3</span><span class=\"sub-lang\">English</span>"
                          "<a href=\"/subtitles/m-9\">M</a></tr>\n");
    CHECK(timedYify(good.repeated(15000), &found) < 2000 && found.size() == 15000);
}

// ---------------------------------------------------------------------------
// Archives

static QByteArray deflated(const QByteArray &data, int windowBits)
{
    z_stream s = {};
    deflateInit2(&s, 9, Z_DEFLATED, windowBits, 8, Z_DEFAULT_STRATEGY);
    QByteArray out(qsizetype(deflateBound(&s, uLong(data.size()))), '\0');
    s.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(data.constData()));
    s.avail_in = uInt(data.size());
    s.next_out = reinterpret_cast<Bytef *>(out.data());
    s.avail_out = uInt(out.size());
    deflate(&s, Z_FINISH);
    out.resize(qsizetype(s.total_out));
    deflateEnd(&s);
    return out;
}

struct ZipEntry
{
    QByteArray name;
    QByteArray stored; // its bytes in the archive
    quint16 method = 0;
    quint32 crc = 0;
    quint32 packed = 0;   // the sizes it says
    quint32 unpacked = 0;
};

static ZipEntry zipEntry(const QByteArray &name, const QByteArray &text, bool compressed)
{
    ZipEntry e;
    e.name = name;
    e.method = compressed ? 8 : 0;
    e.stored = compressed ? deflated(text, -15) : text;
    e.crc = quint32(crc32(0, reinterpret_cast<const Bytef *>(text.constData()), uInt(text.size())));
    e.packed = quint32(e.stored.size());
    e.unpacked = quint32(text.size());
    return e;
}

static void put16(QByteArray &b, quint16 v)
{
    char c[2];
    qToLittleEndian(v, c);
    b.append(c, 2);
}

static void put32(QByteArray &b, quint32 v)
{
    char c[4];
    qToLittleEndian(v, c);
    b.append(c, 4);
}

static QByteArray makeZip(const QList<ZipEntry> &entries)
{
    QByteArray zip, central;
    for (const ZipEntry &e : entries)
    {
        const quint32 offset = quint32(zip.size());
        put32(zip, 0x04034b50); put16(zip, 20); put16(zip, 0); put16(zip, e.method); put16(zip, 0); put16(zip, 0);
        put32(zip, e.crc); put32(zip, e.packed); put32(zip, e.unpacked); put16(zip, quint16(e.name.size()));
        put16(zip, 0);
        zip += e.name;
        zip += e.stored;
        put32(central, 0x02014b50); put16(central, 20); put16(central, 20); put16(central, 0); put16(central, e.method);
        put16(central, 0); put16(central, 0); put32(central, e.crc); put32(central, e.packed); put32(central, e.unpacked);
        put16(central, quint16(e.name.size())); put16(central, 0); put16(central, 0); put16(central, 0);
        put16(central, 0); put32(central, 0); put32(central, offset);
        central += e.name;
    }
    const quint32 centralOffset = quint32(zip.size());
    zip += central;
    put32(zip, 0x06054b50); put16(zip, 0); put16(zip, 0); put16(zip, quint16(entries.size()));
    put16(zip, quint16(entries.size())); put32(zip, quint32(central.size())); put32(zip, centralOffset); put16(zip, 0);
    return zip;
}

// the data unzip() gives the file of that name ("-" when it isn't listed)
static QByteArray dataOf(const QList<QPair<QString, QByteArray>> &files, const char *name)
{
    for (const auto &file : files)
        if (file.first == QLatin1String(name))
            return file.second;
    return "-";
}

static void testArchives()
{
    CHECK(isSubtitleName(QStringLiteral("Movie.SRT")) && isSubtitleName(QStringLiteral("a/b.ass")));
    CHECK(!isSubtitleName(QStringLiteral("cover.jpg")) && !isSubtitleName(QStringLiteral("srt")));

    QByteArray text;
    for (int i = 1; i <= 400; ++i)
        text += QByteArray::number(i) + "\n00:00:01,000 --> 00:00:02,000\nLine " + QByteArray::number(i * 7) + "\n\n";

    // gzip: whole, or nothing
    const QByteArray gz = deflated(text, 16 + MAX_WBITS);
    CHECK(gunzip(gz, 1 << 20) == text);
    CHECK(gunzip(gz, text.size()) == text);       // (exactly its limit)
    CHECK(gunzip(gz, text.size() - 1).isEmpty()); // (past it)
    bool cutShort = false;
    for (qsizetype n = 0; n < gz.size(); n += 3)
        cutShort = cutShort || !gunzip(gz.left(n), 1 << 20).isEmpty();
    CHECK(!cutShort);                                       // every part of it
    CHECK(gunzip(gz.left(gz.size() - 1), 1 << 20).isEmpty()); // (its size, last)
    CHECK(gunzip(gz.left(gz.size() - 8), 1 << 20).isEmpty()); // (its CRC and size)
    QByteArray damaged = gz;
    damaged[damaged.size() - 8] = char(damaged.at(damaged.size() - 8) ^ 1); // (its CRC)
    CHECK(gunzip(damaged, 1 << 20).isEmpty());
    CHECK(gunzip("not gzip at all", 100).isEmpty() && gunzip({}, 100).isEmpty());
    // a bomb: 32 MiB of zeros in 32 KiB, stopped at its limit
    const QByteArray zeros(32 * 1024 * 1024, '\0');
    const QByteArray bomb = deflated(zeros, 16 + MAX_WBITS);
    QElapsedTimer timer;
    timer.start();
    CHECK(gunzip(bomb, 16 * 1024 * 1024).isEmpty());
    CHECK(timer.elapsed() < 2000);

    // zip: the subtitles, each whole or not at all
    const QByteArray ass("[Script Info]\nTitle: x\n\n[Events]\n");
    QList<QPair<QString, QByteArray>> files = unzip(makeZip({
        zipEntry("Movie.srt", text, true), zipEntry("Movie.ass", ass, false), zipEntry("cover.jpg", "JPEG", false),
        zipEntry("__MACOSX/._Movie.srt", "x", false), zipEntry("folder/", "", false) }));
    CHECK(files.size() == 3);
    CHECK(dataOf(files, "Movie.srt") == text && dataOf(files, "Movie.ass") == ass);
    CHECK(dataOf(files, "cover.jpg").isEmpty() && dataOf(files, "folder/") == "-");

    ZipEntry cut = zipEntry("Cut.srt", text, true); // its stream cut short
    cut.stored.chop(10);
    cut.packed = quint32(cut.stored.size());
    ZipEntry smaller = zipEntry("Smaller.srt", text, true); // a size that lies
    --smaller.unpacked;
    ZipEntry larger = zipEntry("Larger.srt", text, true);
    larger.unpacked += 5;
    ZipEntry crc = zipEntry("Crc.srt", text, true); // another CRC
    crc.crc ^= 1;
    ZipEntry stored = zipEntry("Stored.srt", text, false); // stored, its sizes lie
    stored.unpacked += 3;
    ZipEntry shortStored = zipEntry("Short.srt", text, false);
    shortStored.packed = 5;
    ZipEntry bombed = zipEntry("Bomb.srt", zeros, true); // 32 MiB said to be 1000 bytes
    bombed.unpacked = 1000;
    ZipEntry huge = zipEntry("Huge.srt", "x", false); // more than a file may be
    huge.unpacked = 9 * 1024 * 1024;
    timer.restart();
    files = unzip(makeZip({ cut, smaller, larger, crc, stored, shortStored, bombed, huge,
                            zipEntry("Good.srt", text, true) }));
    CHECK(timer.elapsed() < 2000);
    CHECK(files.size() == 9);
    for (const char *name : { "Cut.srt", "Smaller.srt", "Larger.srt", "Crc.srt", "Stored.srt", "Short.srt",
                              "Bomb.srt", "Huge.srt" })
        CHECK(dataOf(files, name).isEmpty());
    CHECK(dataOf(files, "Good.srt") == text);

    // at most 16 MiB in all, and 64 subtitles
    const QByteArray seven(7 * 1024 * 1024, 'a');
    files = unzip(makeZip({ zipEntry("1.srt", seven, true), zipEntry("2.srt", seven, true), zipEntry("3.srt", seven, true) }));
    CHECK(dataOf(files, "1.srt") == seven && dataOf(files, "2.srt") == seven && dataOf(files, "3.srt").isEmpty());
    QList<ZipEntry> many;
    for (int i = 0; i < 70; ++i)
        many.append(zipEntry(QByteArray::number(i) + ".srt", "1\n", false));
    files = unzip(makeZip(many));
    CHECK(files.size() == 70 && dataOf(files, "63.srt") == "1\n" && dataOf(files, "64.srt").isEmpty());

    // cut anywhere, or any byte changed: whole files or none, never a crash
    const QByteArray archive = makeZip({ zipEntry("A.srt", text, true), zipEntry("B.srt", ass, false) });
    bool partial = false;
    for (qsizetype n = 0; n <= archive.size(); n += 5)
        for (const auto &file : unzip(archive.left(n)))
            partial = partial || !(file.second.isEmpty() || file.second == text || file.second == ass);
    std::mt19937 random(9);
    for (int n = 0; n < 3000; ++n)
    {
        QByteArray changed = archive;
        for (int k = 1 + int(random() % 3); k > 0; --k)
            changed[qsizetype(random() % quint32(changed.size()))] = char(random());
        for (const auto &file : unzip(changed))
            partial = partial || !(file.second.isEmpty() || file.second == text || file.second == ass);
    }
    CHECK(!partial);
    CHECK(unzip({}).isEmpty() && unzip("PK\x05\x06").isEmpty());

    // What a file costs counts whether or not it is kept: two of 8 MiB that
    // fail their CRC use the 16 MiB, the subtitle after them isn't unpacked
    const QByteArray eight(8 * 1024 * 1024, '\0');
    ZipEntry wrong = zipEntry("Wrong.srt", eight, true);
    wrong.crc ^= 1;
    ZipEntry wrongToo = wrong;
    wrongToo.name = "WrongToo.srt";
    files = unzip(makeZip({ wrong, wrongToo, zipEntry("After.srt", text, true) }));
    CHECK(files.size() == 3 && dataOf(files, "Wrong.srt").isEmpty() && dataOf(files, "WrongToo.srt").isEmpty());
    CHECK(dataOf(files, "After.srt").isEmpty());
    // a thousand names of one such file (an "overlap" bomb): inflated twice,
    // not 64 times (the fuzz test's: 3 s)
    QByteArray overlap;
    put32(overlap, 0x04034b50); put16(overlap, 20); put16(overlap, 0); put16(overlap, 8); put16(overlap, 0);
    put16(overlap, 0); put32(overlap, wrong.crc); put32(overlap, wrong.packed); put32(overlap, wrong.unpacked);
    put16(overlap, 5); put16(overlap, 0);
    overlap += "x.srt";
    overlap += wrong.stored;
    QByteArray names;
    for (int i = 0; i < 1000; ++i)
    {
        const QByteArray name = QByteArray::number(i) + ".srt";
        put32(names, 0x02014b50); put16(names, 20); put16(names, 20); put16(names, 0); put16(names, 8);
        put16(names, 0); put16(names, 0); put32(names, wrong.crc); put32(names, wrong.packed);
        put32(names, wrong.unpacked); put16(names, quint16(name.size())); put16(names, 0); put16(names, 0);
        put16(names, 0); put16(names, 0); put32(names, 0); put32(names, 0);
        names += name;
    }
    const quint32 namesAt = quint32(overlap.size());
    overlap += names;
    put32(overlap, 0x06054b50); put16(overlap, 0); put16(overlap, 0); put16(overlap, 1000); put16(overlap, 1000);
    put32(overlap, quint32(names.size())); put32(overlap, namesAt); put16(overlap, 0);
    timer.restart();
    files = unzip(overlap);
    CHECK(timer.elapsed() < 1500);
    bool none = files.size() == 1000;
    for (const auto &file : files)
        none = none && file.second.isEmpty();
    CHECK(none);
}

// ---------------------------------------------------------------------------
// The services' scores: "nan", "inf" and "-inf" are numbers to toDouble(),
// none of them a rank (a NaN one would leave the results without an order)

static void testRanks()
{
    for (const char *value : { "nan", "inf", "-inf", "NaN", "-Infinity" })
    {
        const QByteArray v(value);
        QString error;
        const QList<Result> subdl = parseSubDl("{\"status\": true, \"subtitles\": [{\"url\": \"/subtitle/1-2.zip\", "
                                               "\"language\": \"EN\", \"release_name\": \"x\", \"rating\": \"" + v + "\"}]}",
                                               film(), languageOf, &error);
        CHECK(subdl.size() == 1 && subdl[0].rank == 0.);
        Query croatian = film();
        croatian.languages = { QStringLiteral("hr") };
        const QList<Result> titlovi = parseTitlovi("{\"SubtitleResults\": [{\"Id\": \"1\", \"Link\": "
                                                   "\"https://titlovi.com/download/?type=1&mediaid=1\", \"Lang\": "
                                                   "\"Hrvatski\", \"Release\": \"x\", \"Rating\": \"" + v + "\"}]}",
                                                   croatian);
        CHECK(titlovi.size() == 1 && titlovi[0].rank == 0.);
        bool refused = false;
        const QList<Result> assrt = parseAssrt("{\"status\": 0, \"sub\": {\"subs\": [{\"id\": \"1\", \"lang\": "
                                               "{\"langlist\": {\"langeng\": true}}, \"videoname\": \"x\", "
                                               "\"vote_score\": \"" + v + "\"}]}}", film(), &error, &refused);
        CHECK(assrt.size() == 1 && assrt[0].rank == 0.);
        const QList<Result> beta = parseBetaSeries("{\"episode\": {\"subtitles\": [{\"id\": \"1\", \"url\": "
                                                   "\"https://www.betaseries.com/srt/1\", \"language\": \"VF\", "
                                                   "\"file\": \"x.srt\", \"quality\": \"" + v + "\"}]}}", &error, &refused);
        CHECK(beta.size() == 1 && beta[0].rank == 0.);
    }
    // (a rating of more digits than a double holds)
    const QList<Result> yify = parseYify("<tr data-id=\"1\"><span class=\"label a\">-" + QByteArray(400, '9')
                                         + "</span><span class=\"sub-lang\">English</span>"
                                         "<a href=\"/subtitles/m-1\">M</a></tr>", languageOf);
    CHECK(yify.size() == 1 && yify[0].rank == 0.);
}

int main(int argc, char **argv)
{
    fixtures = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral("extras/juxtopposed/fixtures/subtitle-services");
    testCommon();
    testChosenLanguagesFirst();
    testNetworkMedia();
    testSubSource();
    testSubDl();
    testOpenSubtitlesCom();
    testTitlovi();
    testAssrt();
    testJimaku();
    testBetaSeries();
    testSubsRo();
    testNapisy24();
    testLegendas();
    testAnswers();
    testYify();
    testArchives();
    testRanks();
    std::printf("subtitle services: %d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
