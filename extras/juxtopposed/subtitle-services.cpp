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

int main(int argc, char **argv)
{
    fixtures = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral("extras/juxtopposed/fixtures/subtitle-services");
    testCommon();
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
    std::printf("subtitle services: %d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
