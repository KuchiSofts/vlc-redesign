/* Copyright (C) 2026 VLC authors and VideoLAN. GPL version 2 or later.
 *
 * The subtitle services asked with the user's own key or account
 * (modules/gui/qt/player/subtitle_services.cpp): the addresses they are
 * asked, their answers read from saved ones (fixtures/subtitle-services:
 * written from the shapes their maintained clients read, the refusals
 * recorded from the services with a made-up key), their errors. On Linux,
 * from the root of the tree:
 *   g++ -std=c++17 -O1 -Wall -Wextra -fPIC -I. -Imodules/gui/qt $(pkg-config --cflags Qt6Core) \
 *       extras/juxtopposed/subtitle-services.cpp modules/gui/qt/player/subtitle_services.cpp \
 *       $(pkg-config --libs Qt6Core) -o subtitle-services
 *   ./subtitle-services extras/juxtopposed/fixtures/subtitle-services
 */
#include "modules/gui/qt/player/subtitle_services.hpp"

#include <QFile>
#include <QHash>
#include <QUrlQuery>
#include <cstdio>

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

int main(int argc, char **argv)
{
    fixtures = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral("extras/juxtopposed/fixtures/subtitle-services");
    testCommon();
    testSubSource();
    testSubDl();
    testOpenSubtitlesCom();
    std::printf("subtitle services: %d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
