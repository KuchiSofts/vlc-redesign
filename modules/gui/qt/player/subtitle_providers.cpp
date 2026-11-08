/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "subtitle_providers.hpp"
#include "subtitle_services.hpp"

#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QRegularExpression>
#include <QSet>
#include <QStringDecoder>
#include <QTimer>
#include <QUrlQuery>
#include <QtEndian>

#include <algorithm>
#include <cmath>
#include <memory>

#ifdef _WIN32
# include <windows.h>
#else
# include <cerrno>
# include <cstring>
#endif
#include <vlc_common.h>
#include <vlc_iso_lang.h>
#ifndef _WIN32
# include <vlc_charset.h>
#endif

#include "util/http_request.hpp"
#include "util/redesign_version.hpp"
#include "util/subtitle_preferences.hpp"

namespace SubtitleProviders
{

namespace {

constexpr qsizetype maxListBytes = 6 * 1024 * 1024;
// a page of YIFY's (a film's, a few hundred KiB)
constexpr qsizetype maxPageBytes = 2 * 1024 * 1024;
// what a gzip download may unpack to (a small file can unpack to gigabytes;
// zip archives have their own bounds, unzip())
constexpr qsizetype maxUnpackedBytes = 16 * 1024 * 1024;
// between two requests to the same service (they ask for one at a time)
constexpr int requestGapMs = 600;
// The whole of a request to a service, however slowly it answers (each
// step has its own timeout, but an answer a byte at a time would go on for
// days): its search then ends without it
constexpr int requestDeadlineMs = 45 * 1000;

using Done = std::function<void(const QList<Result> &, const QString &, Problem)>;

HttpRequest makeRequest(const QUrl &url, qsizetype maxBytes = maxListBytes, int timeoutMs = 15000)
{
    HttpRequest request;
    request.url = url;
    request.timeoutMs = timeoutMs;
    request.deadlineMs = requestDeadlineMs;
    request.maxBytes = maxBytes;
    request.headers.append(qMakePair(QByteArray("User-Agent"), userAgent()));
    return request;
}

bool ok(const HttpResponse &reply, qsizetype maxBytes = maxListBytes)
{
    return reply.error.isEmpty() && reply.status == 200 && reply.body.size() <= maxBytes;
}

// What went wrong with an answer that isn't ok()
Problem problemOfReply(const HttpResponse &reply)
{
    if (reply.status == 0)
        return Problem::NoAnswer;
    const Problem problem = problemOf(reply.status);
    return problem == Problem::None ? Problem::Failed : problem;
}

QString failure(const QString &name, const HttpResponse &reply, Problem problem = Problem::None)
{
    if (problem == Problem::KeyRefused)
        return QStringLiteral("%1: %2").arg(name, qtr("Key refused: check it"));
    if (problem == Problem::LoginRefused)
        return QStringLiteral("%1: %2").arg(name, qtr("Sign-in refused: check it"));
    if (reply.status == 0)
        return qtr("%1 didn't answer.").arg(name);
    if (reply.status == 429)
        return qtr("%1 asks to wait a moment (too many searches).").arg(name);
    return qtr("%1 didn't answer (%2).").arg(name).arg(reply.status);
}

// A service that didn't answer as it should: why. keyed: one asked with
// the user's key (401 and 403 say it is refused; the others' mean a refusal
// of another kind)
void failed(const Done &done, const QString &name, const HttpResponse &reply, bool keyed = false)
{
    Problem problem = problemOfReply(reply);
    if (problem == Problem::KeyRefused && !keyed)
        problem = Problem::Failed;
    done({}, failure(name, reply, problem), problem);
}

void nothing(const Done &done)
{
    done({}, {}, Problem::None);
}

// A request whose answer is read by parse on the worker thread of the
// request (a list of a few MB: not on the thread of the interface; parse
// touches nothing of it), then done(answer, what parse made) on the thread
// of context
template <typename T, typename Parse, typename Then>
void askParsed(QObject *context, HttpRequest request, Parse parse, Then done)
{
    auto parsed = std::make_shared<T>();
    httpRequestAsync(context, std::move(request),
        [parsed, parse](const HttpResponse &reply) { *parsed = parse(reply); },
        [parsed, done](const HttpResponse &reply) { done(reply, *parsed); });
}

// what a search of the keyed services reads: its results, or why not
struct Parsed
{
    QList<Result> results;
    QString error;
    bool refused = false;
    QString id; // (of the title, the show, the entry: a first request's)
};

// Lower case words, without accents: names compare in any script
QString simplified(const QString &text)
{
    QString decomposed = text.normalized(QString::NormalizationForm_D).toLower();
    decomposed.replace(QLatin1Char('&'), QStringLiteral(" and "));
    QString out;
    out.reserve(decomposed.size());
    for (const QChar c : decomposed)
    {
        if (c.category() == QChar::Mark_NonSpacing)
            continue;
        out.append(c.isLetterOrNumber() ? c : QLatin1Char(' '));
    }
    return out.simplified();
}

// A language a service names in English ("Hebrew", "Portuguese (Brazilian)",
// "Brazilian Portuguese", "Big 5 code")
QString languageFromName(const QString &name)
{
    const QString n = name.trimmed().toLower();
    if (n.contains(QLatin1String("brazil")))
        return QStringLiteral("pt-BR");
    if (n.contains(QLatin1String("traditional")) || n.contains(QLatin1String("big 5")))
        return QStringLiteral("zh-TW");
    if (n.contains(QLatin1String("latin america")) || n.contains(QLatin1String("latino")))
        return QStringLiteral("es-419");
    if (n.startsWith(QLatin1String("farsi")) || n.startsWith(QLatin1String("persian")))
        return QStringLiteral("fa");
    if (n.startsWith(QLatin1String("chinese")))
        return QStringLiteral("zh");
    if (n.startsWith(QLatin1String("serbian")))
        return QStringLiteral("sr");
    if (n.startsWith(QLatin1String("norwegian")))
        return QStringLiteral("no");
    // "French (Canada)": the language
    QString base = n.section(QLatin1Char('('), 0, 0).trimmed();
    QString code = SubtitlePreferences::normalize(base);
    if (code.isEmpty())
        code = SubtitlePreferences::normalize(n);
    return code;
}

// The code OpenSubtitles.org has for a language (ISO 639-2/B, and its own)
QString openSubtitlesCode(const QString &code)
{
    static const QHash<QString, QString> special = {
        { "pt-BR", "pob" }, { "zh-TW", "zht" }, { "es-419", "spl" }, { "sr", "scc" },
        { "cnr", "mne" }, { "zh", "chi" }, { "he", "heb" },
    };
    const auto it = special.constFind(code);
    if (it != special.constEnd())
        return it.value();
    const iso639_lang_t *l = vlc_find_iso639(qtu(code), false);
    if (l && *l->psz_iso639_2B)
        return QString::fromLatin1(l->psz_iso639_2B);
    return code.size() == 3 ? code : QString();
}

// ---------------------------------------------------------------------------
// OpenSubtitles.org: its open interface (rest.opensubtitles.org), by the
// file's hash (those made for this very file), then by the video's IMDb id
// in the user's languages, then in all of them; by the title when the video
// was not identified. The parameters go in the path, in alphabetical order.

QUrl openSubtitlesUrl(QStringList segments)
{
    std::sort(segments.begin(), segments.end());
    QByteArray address("https://rest.opensubtitles.org/search/");
    address += segments.join(QLatin1Char('/')).toUtf8();
    return QUrl::fromEncoded(address);
}

void parseOpenSubtitles(const QByteArray &body, QList<Result> &results, QSet<QString> &seen)
{
    for (const QJsonValue item : QJsonDocument::fromJson(body).array())
    {
        const QJsonObject o = item.toObject();
        const auto text = [&o](const char *key) { return o.value(QLatin1String(key)).toVariant().toString().trimmed(); };
        const QString id = text("IDSubtitleFile");
        if (id.isEmpty() || seen.contains(id))
            continue;
        // made by a machine: not offered
        if (text("SubAutoTranslation") == QLatin1String("1"))
            continue;
        QString language = SubtitlePreferences::normalize(text("SubLanguageID"));
        if (language.isEmpty())
            language = SubtitlePreferences::normalize(text("ISO639"));
        const QUrl url(text("SubDownloadLink"));
        if (language.isEmpty() || url.scheme() != QLatin1String("https")
            || !url.host().endsWith(QLatin1String("opensubtitles.org")))
            continue;
        seen.insert(id);
        Result r;
        r.id = QStringLiteral("opensubtitles:") + id;
        r.provider = QStringLiteral("opensubtitles");
        r.language = language;
        r.fileName = text("SubFileName");
        r.release = text("MovieReleaseName");
        // the uploaders' decorations ("* <a note> * The Matrix…")
        r.release.remove(QRegularExpression(QStringLiteral(R"(^[\s*\-:.|]+|[\s*\-:.|]+$)")));
        if (r.release.isEmpty())
            r.release = QFileInfo(r.fileName).completeBaseName();
        r.url = url;
        r.encoding = text("SubEncoding");
        r.exact = text("MatchedBy") == QLatin1String("moviehash");
        r.hearingImpaired = text("SubHearingImpaired") == QLatin1String("1");
        r.foreignOnly = text("SubForeignPartsOnly") == QLatin1String("1");
        r.downloads = text("SubDownloadsCnt").toInt();
        r.fps = text("MovieFPS").toDouble();
        r.rank = std::log10(double(r.downloads) + 1.) * 3.
               + (text("SubFromTrusted") == QLatin1String("1") ? 5. : 0.)
               + (text("SubBad") == QLatin1String("1") ? -30. : 0.);
        results.append(r);
    }
}

struct Chain
{
    QList<QUrl> urls;
    QList<Result> results;
    QSet<QString> seen;
    QString error;
    Problem problem = Problem::None;
    int answered = 0;
};

void runOpenSubtitles(QPointer<QObject> context, std::shared_ptr<Chain> chain, Done done)
{
    if (!context)
        return;
    if (chain->urls.isEmpty())
    {
        if (chain->answered == 0)
            done(chain->results, chain->error, chain->problem);
        else
            done(chain->results, {}, Problem::None);
        return;
    }
    HttpRequest request = makeRequest(chain->urls.takeFirst());
    request.headers.append(qMakePair(QByteArray("X-User-Agent"), userAgent()));
    // (read on the worker thread, with the files seen so far: one request
    // of the chain at a time)
    using Found = QPair<QList<Result>, QSet<QString>>;
    const QSet<QString> seen = chain->seen;
    askParsed<Found>(context, request, [seen](const HttpResponse &reply) {
        Found found { {}, seen };
        if (ok(reply))
            parseOpenSubtitles(reply.body, found.first, found.second);
        return found;
    }, [context, chain, done](const HttpResponse &reply, const Found &found) {
        if (ok(reply))
        {
            ++chain->answered;
            chain->results.append(found.first);
            chain->seen = found.second;
        }
        else if (chain->error.isEmpty())
        {
            chain->problem = problemOfReply(reply);
            if (chain->problem == Problem::KeyRefused)
                chain->problem = Problem::Failed;
            chain->error = failure(QStringLiteral("OpenSubtitles"), reply);
        }
        if (chain->urls.isEmpty())
            runOpenSubtitles(context, chain, done);
        else
            QTimer::singleShot(requestGapMs, context, [context, chain, done]() {
                runOpenSubtitles(context, chain, done);
            });
    });
}

void searchOpenSubtitles(QObject *context, const Query &query, Done done)
{
    auto chain = std::make_shared<Chain>();
    // one language a request (it refuses lists), all of them without one
    QStringList wanted;
    for (const QString &language : query.languages)
    {
        const QString code = openSubtitlesCode(language);
        if (!code.isEmpty() && !wanted.contains(code))
            wanted.append(code);
    }
    QStringList episode;
    if (query.series && query.season >= 0 && query.episode >= 0)
        episode << QStringLiteral("episode-%1").arg(query.episode) << QStringLiteral("season-%1").arg(query.season);

    // those made for this very file
    if (!query.hash.isEmpty() && query.size > 0)
        chain->urls.append(openSubtitlesUrl({ QStringLiteral("moviebytesize-%1").arg(query.size),
                                              QStringLiteral("moviehash-") + query.hash }));
    const QString imdb = query.imdb.startsWith(QLatin1String("tt")) ? query.imdb.mid(2) : QString();
    if (!imdb.isEmpty())
    {
        // the user's languages (complete), then every language
        const QString padded = QStringLiteral("imdbid-%1").arg(imdb.toLongLong(), 7, 10, QLatin1Char('0'));
        for (const QString &code : wanted)
            chain->urls.append(openSubtitlesUrl(QStringList(episode) << padded << QStringLiteral("sublanguageid-") + code));
        // (only the user's languages: a faster search)
        if (query.everyLanguage || wanted.isEmpty())
            chain->urls.append(openSubtitlesUrl(QStringList(episode) << padded));
    }
    else if (!query.title.isEmpty())
    {
        const QString encoded = QString::fromUtf8(QUrl::toPercentEncoding(query.title.toLower()));
        for (const QString &code : wanted)
            chain->urls.append(openSubtitlesUrl(QStringList(episode) << QStringLiteral("query-") + encoded
                                                                     << QStringLiteral("sublanguageid-") + code));
    }
    if (chain->urls.isEmpty())
    {
        nothing(done);
        return;
    }
    runOpenSubtitles(QPointer<QObject>(context), chain, done);
}

} // namespace

Identity identityOfHash(const QByteArray &openSubtitlesReply)
{
    // a few subtitles of a file are filed under another title: the one most of them name
    QHash<QString, int> votes;
    QHash<QString, Identity> identities;
    int total = 0;
    static const QRegularExpression quoted(QStringLiteral(R"re(^"([^"]+)")re"));
    for (const QJsonValue item : QJsonDocument::fromJson(openSubtitlesReply).array())
    {
        const QJsonObject o = item.toObject();
        const auto text = [&o](const char *key) { return o.value(QLatin1String(key)).toVariant().toString().trimmed(); };
        if (text("MatchedBy") != QLatin1String("moviehash"))
            continue;
        Identity identity;
        identity.series = text("MovieKind") == QLatin1String("episode");
        const qlonglong number = (identity.series ? text("SeriesIMDBParent") : text("IDMovieImdb")).toLongLong();
        if (number <= 0)
            continue;
        identity.imdb = QStringLiteral("tt%1").arg(number, 7, 10, QLatin1Char('0'));
        identity.title = text("MovieName");
        if (identity.series)
        {
            // ("\"Breaking Bad\" Pilot": the series in quotes, the episode after it)
            const QRegularExpressionMatch m = quoted.match(identity.title);
            if (m.hasMatch())
                identity.title = m.captured(1);
            identity.season = text("SeriesSeason").toInt();
            identity.episode = text("SeriesEpisode").toInt();
            if (identity.season <= 0 && identity.episode <= 0)
                identity.season = identity.episode = -1;
        }
        else
            identity.year = text("MovieYear").toInt();
        if (identity.title.isEmpty())
            continue;
        const QString key = QStringLiteral("%1/%2/%3").arg(identity.imdb).arg(identity.season).arg(identity.episode);
        ++votes[key];
        ++total;
        if (!identities.contains(key))
            identities.insert(key, identity);
    }
    QString best;
    for (auto it = votes.constBegin(); it != votes.constEnd(); ++it)
        if (best.isEmpty() || it.value() > votes.value(best))
            best = it.key();
    if (best.isEmpty())
        return {};
    Identity identity = identities.value(best);
    identity.sure = votes.value(best) * 2 > total;
    return identity;
}

void identifyByHash(QObject *context, const QString &hash, qint64 size, std::function<void(const Identity &)> done)
{
    if (hash.isEmpty() || size <= 0)
    {
        done({});
        return;
    }
    HttpRequest request = makeRequest(openSubtitlesUrl({ QStringLiteral("moviebytesize-%1").arg(size),
                                                         QStringLiteral("moviehash-") + hash }));
    request.headers.append(qMakePair(QByteArray("X-User-Agent"), userAgent()));
    askParsed<Identity>(context, request, [](const HttpResponse &reply) {
        return ok(reply) ? identityOfHash(reply.body) : Identity();
    }, [done](const HttpResponse &, const Identity &identity) { done(identity); });
}

namespace {

// ---------------------------------------------------------------------------
// Wizdom: Hebrew subtitles, by the IMDb id (of the series, with the season
// and episode), scored against the name of the video file

void searchWizdom(QObject *context, const Query &query, Done done)
{
    if (!query.imdb.startsWith(QLatin1String("tt")))
    {
        nothing(done);
        return;
    }
    QUrl url(QStringLiteral("https://wizdom.xyz/api/search"));
    QUrlQuery params;
    params.addQueryItem(QStringLiteral("action"), QStringLiteral("by_id"));
    params.addQueryItem(QStringLiteral("imdb"), query.imdb);
    params.addQueryItem(QStringLiteral("season"), QString::number(query.series ? std::max(query.season, 0) : 0));
    params.addQueryItem(QStringLiteral("episode"), QString::number(query.series ? std::max(query.episode, 0) : 0));
    params.addQueryItem(QStringLiteral("version"), QFileInfo(query.fileName).completeBaseName());
    url.setQuery(params);
    askParsed<QList<Result>>(context, makeRequest(url), [](const HttpResponse &reply) {
        QList<Result> results;
        if (!ok(reply))
            return results;
        for (const QJsonValue item : QJsonDocument::fromJson(reply.body).array())
        {
            const QJsonObject o = item.toObject();
            const QString id = o.value(QLatin1String("id")).toVariant().toString();
            if (id.isEmpty())
                continue;
            Result r;
            r.id = QStringLiteral("wizdom:") + id;
            r.provider = QStringLiteral("wizdom");
            r.language = QStringLiteral("he");
            r.release = cleanRelease(o.value(QLatin1String("versioname")).toString());
            r.url = QUrl(QStringLiteral("https://wizdom.xyz/api/files/sub/") + id);
            const double score = o.value(QLatin1String("score")).toDouble();
            r.rank = std::isfinite(score) ? score * 5. : 0.;
            results.append(r);
        }
        return results;
    }, [done](const HttpResponse &reply, const QList<Result> &results) {
        // (it answers 500 when it has nothing)
        if (reply.error.isEmpty() && reply.status == 500)
        {
            nothing(done);
            return;
        }
        if (!ok(reply))
        {
            failed(done, QStringLiteral("Wizdom"), reply);
            return;
        }
        done(results, {}, Problem::None);
    });
}

// ---------------------------------------------------------------------------
// Addic7ed (through Gestdown's open interface): series only, by the show's
// name, then its episode in each of the user's languages

void searchAddic7ed(QObject *context, const Query &query, Done done)
{
    if (!query.series || query.season < 0 || query.episode < 0 || query.title.isEmpty())
    {
        nothing(done);
        return;
    }
    QPointer<QObject> guard(context);
    QByteArray address("https://api.gestdown.info/shows/search/");
    address += QUrl::toPercentEncoding(query.title);
    const QUrl url = QUrl::fromEncoded(address);
    // the show of that name (read on the worker thread)
    const QString wanted = simplified(query.title);
    askParsed<QString>(context, makeRequest(url), [wanted](const HttpResponse &reply) {
        QString showId;
        if (!ok(reply))
            return showId;
        double best = 0;
        for (const QJsonValue item : QJsonDocument::fromJson(reply.body).object()
                                         .value(QLatin1String("shows")).toArray())
        {
            const QJsonObject o = item.toObject();
            const QString name = simplified(o.value(QLatin1String("name")).toString());
            const double score = name == wanted ? 2. : name.startsWith(wanted) ? 1. : 0.;
            if (score > best)
            {
                best = score;
                showId = o.value(QLatin1String("id")).toString();
            }
        }
        return showId;
    }, [guard, query, done](const HttpResponse &reply, const QString &showId) {
        if (!guard)
            return;
        if (reply.error.isEmpty() && reply.status == 404)
        {
            nothing(done);
            return;
        }
        if (!ok(reply))
        {
            failed(done, QStringLiteral("Addic7ed"), reply);
            return;
        }
        if (showId.isEmpty())
        {
            nothing(done);
            return;
        }
        // the episode in each language, one after the other
        auto results = std::make_shared<QList<Result>>();
        auto languages = std::make_shared<QStringList>(query.languages.mid(0, 2));
        // (each request keeps the chain while it runs: no cycle)
        auto next = std::make_shared<std::function<void()>>();
        const std::weak_ptr<std::function<void()>> weak = next;
        *next = [guard, query, showId, results, languages, weak, done]() {
            if (!guard)
                return;
            if (languages->isEmpty())
            {
                done(*results, {}, Problem::None);
                return;
            }
            const auto self = weak.lock();
            const QString language = languages->takeFirst();
            const QUrl url(QStringLiteral("https://api.gestdown.info/subtitles/get/%1/%2/%3/%4")
                               .arg(showId).arg(query.season).arg(query.episode)
                               .arg(language.section(QLatin1Char('-'), 0, 0)));
            askParsed<QList<Result>>(guard, makeRequest(url), [language](const HttpResponse &reply) {
                QList<Result> found;
                if (!ok(reply))
                    return found;
                for (const QJsonValue item : QJsonDocument::fromJson(reply.body).object()
                                                 .value(QLatin1String("matchingSubtitles")).toArray())
                {
                    const QJsonObject o = item.toObject();
                    const QString id = o.value(QLatin1String("subtitleId")).toString();
                    const QString uri = o.value(QLatin1String("downloadUri")).toString();
                    if (id.isEmpty() || !uri.startsWith(QLatin1Char('/'))
                        || !o.value(QLatin1String("completed")).toBool(true))
                        continue;
                    Result r;
                    r.id = QStringLiteral("addic7ed:") + id;
                    r.provider = QStringLiteral("addic7ed");
                    r.language = languageFromName(o.value(QLatin1String("language")).toString());
                    if (r.language.isEmpty())
                        r.language = language;
                    r.release = o.value(QLatin1String("version")).toString();
                    r.url = QUrl(QStringLiteral("https://api.gestdown.info") + uri);
                    r.hearingImpaired = o.value(QLatin1String("hearingImpaired")).toBool();
                    r.downloads = o.value(QLatin1String("downloadCount")).toInt();
                    r.rank = std::log10(double(std::max(r.downloads, 0)) + 1.) * 3.
                           + (o.value(QLatin1String("corrected")).toBool() ? 3. : 0.);
                    found.append(r);
                }
                return found;
            }, [guard, results, self](const HttpResponse &, const QList<Result> &found) {
                if (!guard)
                    return;
                results->append(found);
                QTimer::singleShot(requestGapMs, guard, [self]() { if (self && *self) (*self)(); });
            });
        };
        (*next)();
    });
}

// ---------------------------------------------------------------------------
// YIFY Subtitles: films only, by the IMDb id, every language (its page)

void searchYify(QObject *context, const Query &query, Done done)
{
    if (query.series || !query.imdb.startsWith(QLatin1String("tt")))
    {
        nothing(done);
        return;
    }
    const QUrl url(QStringLiteral("https://yifysubtitles.ch/movie-imdb/") + query.imdb);
    // (in one pass, on the worker thread: its time follows the size of the
    // page, whatever it is)
    askParsed<QList<Result>>(context, makeRequest(url, maxPageBytes), [](const HttpResponse &reply) {
        return ok(reply, maxPageBytes) ? parseYify(reply.body, languageFromName) : QList<Result>();
    }, [done](const HttpResponse &reply, const QList<Result> &results) {
        if (reply.error.isEmpty() && reply.status == 404)
        {
            nothing(done);
            return;
        }
        if (!ok(reply, maxPageBytes))
        {
            failed(done, QStringLiteral("YIFY"), reply);
            return;
        }
        done(results, {}, Problem::None);
    });
}

// ---------------------------------------------------------------------------
// The services asked with the user's own key or account: what they are asked
// and what they answer in subtitle_services.cpp (tested by themselves)

QString languageOf(const QString &name)
{
    return languageFromName(name);
}

HttpRequest jsonRequest(const QUrl &url)
{
    HttpRequest request = makeRequest(url);
    request.headers.append(qMakePair(QByteArray("Accept"), QByteArray("application/json")));
    return request;
}

// SubSource: the title (by its IMDb id, else its name), then its subtitles
void subSourceSubtitles(QPointer<QObject> guard, const Query &query, const QString &movieId, Done done)
{
    HttpRequest request = jsonRequest(subSourceSubtitlesUrl(movieId, query));
    request.headers.append(qMakePair(QByteArray("X-API-Key"), query.credentials.key.toUtf8()));
    askParsed<Parsed>(guard, request, [query](const HttpResponse &reply) {
        Parsed parsed;
        if (ok(reply))
            parsed.results = parseSubSource(reply.body, query, languageOf, &parsed.error);
        return parsed;
    }, [guard, done](const HttpResponse &reply, const Parsed &parsed) {
        if (!guard)
            return;
        if (!ok(reply))
        {
            failed(done, QStringLiteral("SubSource"), reply, true);
            return;
        }
        if (!parsed.error.isEmpty())
            done({}, QStringLiteral("SubSource: ") + parsed.error, Problem::Failed);
        else
            done(parsed.results, {}, Problem::None);
    });
}

void subSourceFind(QPointer<QObject> guard, const Query &query, bool byText, Done done)
{
    const QUrl url = subSourceSearchUrl(query, byText);
    if (url.isEmpty())
    {
        nothing(done);
        return;
    }
    HttpRequest request = jsonRequest(url);
    request.headers.append(qMakePair(QByteArray("X-API-Key"), query.credentials.key.toUtf8()));
    askParsed<Parsed>(guard, request, [query](const HttpResponse &reply) {
        Parsed parsed;
        if (ok(reply))
            parsed.id = subSourceMovieId(reply.body, query, &parsed.error);
        return parsed;
    }, [guard, query, byText, done](const HttpResponse &reply, const Parsed &parsed) {
        if (!guard)
            return;
        if (!ok(reply))
        {
            failed(done, QStringLiteral("SubSource"), reply, true);
            return;
        }
        const QString movieId = parsed.id;
        if (!parsed.error.isEmpty())
        {
            done({}, QStringLiteral("SubSource: ") + parsed.error, Problem::Failed);
            return;
        }
        // (one request a second at most: it asks for no more)
        if (movieId.isEmpty())
        {
            // not known by its IMDb id: by its name
            if (!byText && !query.title.isEmpty())
                QTimer::singleShot(requestGapMs, guard, [guard, query, done]() {
                    if (guard)
                        subSourceFind(guard, query, true, done);
                });
            else
                nothing(done);
            return;
        }
        QTimer::singleShot(requestGapMs, guard, [guard, query, movieId, done]() {
            if (guard)
                subSourceSubtitles(guard, query, movieId, done);
        });
    });
}

void searchSubSource(QObject *context, const Query &query, Done done)
{
    if (query.credentials.key.isEmpty())
    {
        nothing(done);
        return;
    }
    subSourceFind(QPointer<QObject>(context), query, false, done);
}

// SubDL: its second version, else the first
void searchSubDl(QObject *context, const Query &query, Done done, bool v2 = true)
{
    const QUrl url = subDlSearchUrl(query, v2);
    if (query.credentials.key.isEmpty() || url.isEmpty())
    {
        nothing(done);
        return;
    }
    HttpRequest request = jsonRequest(url);
    if (v2)
        request.headers.append(qMakePair(QByteArray("Authorization"), "Bearer " + query.credentials.key.toUtf8()));
    QPointer<QObject> guard(context);
    askParsed<Parsed>(context, request, [query](const HttpResponse &reply) {
        Parsed parsed;
        if (ok(reply))
            parsed.results = parseSubDl(reply.body, query, languageOf, &parsed.error);
        return parsed;
    }, [guard, query, done, v2](const HttpResponse &reply, const Parsed &parsed) {
        if (!guard)
            return;
        if (v2 && reply.error.isEmpty() && reply.status == 404)
        {
            searchSubDl(guard, query, done, false);
            return;
        }
        if (!ok(reply))
        {
            failed(done, QStringLiteral("SubDL"), reply, true);
            return;
        }
        if (!parsed.error.isEmpty())
            done({}, QStringLiteral("SubDL: ") + parsed.error, Problem::Failed);
        else
            done(parsed.results, {}, Problem::None);
    });
}

// OpenSubtitles.com: the user's free key (its own requests), signed in for
// the downloads when the user gave the account
QByteArray openSubtitlesComAgent()
{
    // (the name and version it asks for: "App v1.0")
    return QByteArray("VLC-Redesign v" VLC_REDESIGN_VERSION);
}

HttpRequest openSubtitlesComRequest(const QUrl &url, const QString &key, const QString &token = {})
{
    HttpRequest request;
    request.url = url;
    request.timeoutMs = 15000;
    request.deadlineMs = requestDeadlineMs;
    request.maxBytes = maxListBytes;
    request.headers = {
        { "Api-Key", key.toUtf8() },
        { "User-Agent", openSubtitlesComAgent() },
        { "Accept", "application/json" },
        { "Content-Type", "application/json" },
    };
    if (!token.isEmpty())
        request.headers.append(qMakePair(QByteArray("Authorization"), "Bearer " + token.toUtf8()));
    return request;
}

void searchOpenSubtitlesCom(QObject *context, const Query &query, Done done)
{
    const QUrl url = openSubtitlesComSearchUrl(query);
    if (query.credentials.key.isEmpty() || url.isEmpty())
    {
        nothing(done);
        return;
    }
    askParsed<QList<Result>>(context, openSubtitlesComRequest(url, query.credentials.key), [](const HttpResponse &reply) {
        return ok(reply) ? parseOpenSubtitlesCom(reply.body, languageOf) : QList<Result>();
    }, [done](const HttpResponse &reply, const QList<Result> &results) {
        if (!ok(reply))
        {
            failed(done, QStringLiteral("OpenSubtitles.com"), reply, true);
            return;
        }
        done(results, {}, Problem::None);
    });
}

// The sessions of the services signed in (in memory, for this run only):
// their tokens, by service, with the user they are of
struct SignedIn
{
    QString username;
    Session session;
};
QHash<QString, SignedIn> &sessions()
{
    static QHash<QString, SignedIn> map;
    return map;
}

// Signs in to OpenSubtitles.com: done(session) or done({}, problem). A
// check (Test, Sign in: credentials typed, maybe never saved) signs in
// afresh and keeps nothing: the searches only ever use a session of the
// credentials kept
void openSubtitlesComSignIn(QObject *context, const Credentials &credentials,
                            std::function<void(const Session &, Problem)> done, bool check = false)
{
    const auto known = sessions().constFind(QStringLiteral("opensubtitlescom"));
    if (!check && known != sessions().constEnd() && known->username == credentials.username)
    {
        done(known->session, Problem::None);
        return;
    }
    HttpRequest login = openSubtitlesComRequest(QUrl(QStringLiteral("https://api.opensubtitles.com/api/v1/login")),
                                                credentials.key);
    login.method = "POST";
    login.body = openSubtitlesComLoginBody(credentials.username, credentials.password);
    const QString username = credentials.username;
    httpRequestAsync(context, login, [done, username, check](const HttpResponse &reply) {
        if (!ok(reply))
        {
            // (401: the account; 403: the key)
            const Problem problem = reply.status == 401 ? Problem::LoginRefused : problemOfReply(reply);
            done({}, problem);
            return;
        }
        const Session session = openSubtitlesComSession(reply.body);
        if (session.token.isEmpty())
        {
            done({}, Problem::LoginRefused);
            return;
        }
        if (!check)
            sessions().insert(QStringLiteral("opensubtitlescom"), { username, session });
        done(session, Problem::None);
    });
}

// ---------------------------------------------------------------------------
// The services of wave 2 (with an account or a key), as Bazarr's providers
// ask them

// Who is signed in: the user name, or the email (Legendas.net)
QString whoOf(const QString &providerId, const Credentials &credentials)
{
    return providerId == QLatin1String("legendasnet") ? credentials.email.trimmed() : credentials.username.trimmed();
}

// Signs in to a service that gives a session (a token): done(session) or
// done({}, problem); a session kept for this run is given again. A check
// signs in afresh and keeps nothing (openSubtitlesComSignIn)
void signIn(QObject *context, const QString &providerId, const Credentials &credentials,
            std::function<void(const Session &, Problem)> done, bool check = false)
{
    if (providerId == QLatin1String("opensubtitlescom"))
    {
        openSubtitlesComSignIn(context, credentials, done, check);
        return;
    }
    const QString who = whoOf(providerId, credentials);
    const auto known = sessions().constFind(providerId);
    if (!check && known != sessions().constEnd() && known->username == who)
    {
        done(known->session, Problem::None);
        return;
    }
    HttpRequest request;
    if (providerId == QLatin1String("titlovi"))
    {
        // (its parameters in the address, as its add-ons send them; a
        // body, for the length the server asks of a POST)
        request = jsonRequest(titloviLoginUrl(who, credentials.password));
        request.body = request.url.query(QUrl::FullyEncoded).toUtf8();
        request.headers.append(qMakePair(QByteArray("Content-Type"), QByteArray("application/x-www-form-urlencoded")));
    }
    else if (providerId == QLatin1String("legendasnet"))
    {
        request = jsonRequest(QUrl(QStringLiteral("https://legendas.net/api/v1/login")));
        request.body = legendasLoginBody(who, credentials.password);
        request.headers.append(qMakePair(QByteArray("Content-Type"), QByteArray("application/json")));
    }
    else
    {
        done({}, Problem::Failed);
        return;
    }
    request.method = "POST";
    httpRequestAsync(context, request, [providerId, who, done, check](const HttpResponse &reply) {
        if (!ok(reply))
        {
            const Problem problem = reply.status == 401 || reply.status == 403 ? Problem::LoginRefused
                                                                               : problemOfReply(reply);
            done({}, problem);
            return;
        }
        const Session session = providerId == QLatin1String("titlovi") ? titloviSession(reply.body)
                                                                       : legendasSession(reply.body);
        if (session.token.isEmpty())
        {
            done({}, Problem::LoginRefused);
            return;
        }
        if (!check)
            sessions().insert(providerId, { who, session });
        done(session, Problem::None);
    });
}

// The search of a service signed in: ask(session, again) asks it; a 401 then
// signs in again, once (its token ran out)
void withSession(QObject *context, const QString &providerId, const QString &name, const Query &query,
                 const Done &done, std::function<void(const Session &, std::function<void()>)> ask)
{
    QPointer<QObject> guard(context);
    signIn(context, providerId, query.credentials, [guard, providerId, name, query, done, ask](const Session &session,
                                                                                              Problem problem) {
        if (!guard)
            return;
        if (problem != Problem::None)
        {
            done({}, failure(name, {}, problem), problem);
            return;
        }
        ask(session, [guard, providerId, name, query, done, ask]() {
            forgetSession(providerId);
            if (guard)
                signIn(guard, providerId, query.credentials, [guard, name, done, ask](const Session &fresh, Problem again) {
                    if (!guard)
                        return;
                    if (again != Problem::None)
                        done({}, failure(name, {}, again), again);
                    else
                        ask(fresh, {});
                });
        });
    });
}

void searchTitlovi(QObject *context, const Query &query, Done done)
{
    if (titloviSearchUrl(query, { QStringLiteral("t"), {}, QStringLiteral("u") }).isEmpty())
    {
        nothing(done);
        return;
    }
    QPointer<QObject> guard(context);
    const QString name = QStringLiteral("Titlovi");
    withSession(context, QStringLiteral("titlovi"), name, query, done,
                [guard, query, name, done](const Session &session, std::function<void()> again) {
        if (!guard)
            return;
        askParsed<QList<Result>>(guard, jsonRequest(titloviSearchUrl(query, session)), [query](const HttpResponse &reply) {
            return ok(reply) ? parseTitlovi(reply.body, query) : QList<Result>();
        }, [name, done, again](const HttpResponse &reply, const QList<Result> &results) {
            if (reply.error.isEmpty() && reply.status == 401 && again)
            {
                again();
                return;
            }
            if (!ok(reply))
            {
                const Problem problem = reply.status == 401 ? Problem::LoginRefused : problemOfReply(reply);
                done({}, failure(name, reply, problem), problem);
                return;
            }
            done(results, {}, Problem::None);
        });
    });
}

void searchAssrt(QObject *context, const Query &query, Done done)
{
    const QUrl url = assrtSearchUrl(query);
    if (query.credentials.key.isEmpty() || url.isEmpty())
    {
        nothing(done);
        return;
    }
    // (its errors are answers too: 400 with their status)
    const auto answered = [](const HttpResponse &reply) {
        return reply.error.isEmpty() && !reply.body.isEmpty() && (reply.status == 200 || reply.status == 400);
    };
    askParsed<Parsed>(context, jsonRequest(url), [query, answered](const HttpResponse &reply) {
        Parsed parsed;
        if (answered(reply))
            parsed.results = parseAssrt(reply.body, query, &parsed.error, &parsed.refused);
        return parsed;
    }, [done, answered](const HttpResponse &reply, const Parsed &parsed) {
        const QString name = QStringLiteral("Assrt");
        if (!answered(reply))
        {
            failed(done, name, reply, true);
            return;
        }
        if (parsed.refused)
            done({}, failure(name, reply, Problem::KeyRefused), Problem::KeyRefused);
        else if (!parsed.error.isEmpty())
            done({}, name + QStringLiteral(": ") + parsed.error, Problem::Failed);
        else
            done(parsed.results, {}, Problem::None);
    });
}

HttpRequest jimakuRequest(const QUrl &url, const QString &key)
{
    HttpRequest request = jsonRequest(url);
    request.headers.append(qMakePair(QByteArray("Authorization"), key.toUtf8()));
    return request;
}

void searchJimaku(QObject *context, const Query &query, Done done)
{
    const QUrl url = jimakuSearchUrl(query);
    if (query.credentials.key.isEmpty() || url.isEmpty())
    {
        nothing(done);
        return;
    }
    QPointer<QObject> guard(context);
    const QString name = QStringLiteral("Jimaku");
    askParsed<QString>(context, jimakuRequest(url, query.credentials.key), [query](const HttpResponse &reply) {
        return ok(reply) ? jimakuEntryId(reply.body, query) : QString();
    }, [guard, query, name, done](const HttpResponse &reply, const QString &entryId) {
        if (!guard)
            return;
        if (!ok(reply))
        {
            failed(done, name, reply, true);
            return;
        }
        const QUrl files = jimakuFilesUrl(entryId, query);
        if (files.isEmpty())
        {
            nothing(done);
            return;
        }
        QTimer::singleShot(requestGapMs, guard, [guard, query, name, files, done]() {
            if (!guard)
                return;
            askParsed<QList<Result>>(guard, jimakuRequest(files, query.credentials.key), [query](const HttpResponse &reply) {
                return ok(reply) ? parseJimaku(reply.body, query) : QList<Result>();
            }, [name, done](const HttpResponse &reply, const QList<Result> &results) {
                if (!ok(reply))
                    failed(done, name, reply, true);
                else
                    done(results, {}, Problem::None);
            });
        });
    });
}

// BetaSeries' answers (read on the worker thread): its errors come as 400
// with their code
bool betaSeriesAnswered(const HttpResponse &reply)
{
    return reply.error.isEmpty() && (reply.status == 200 || reply.status == 400);
}

Parsed parseBetaSeriesAnswer(const HttpResponse &reply, bool show)
{
    Parsed parsed;
    if (!betaSeriesAnswered(reply))
        return parsed;
    parsed.results = parseBetaSeries(reply.body, &parsed.error, &parsed.refused);
    if (show)
        parsed.id = betaSeriesShowId(reply.body);
    return parsed;
}

bool betaSeriesFailed(const HttpResponse &reply, const Parsed &parsed, const Done &done)
{
    const QString name = QStringLiteral("BetaSeries");
    if (!betaSeriesAnswered(reply))
    {
        failed(done, name, reply, true);
        return true;
    }
    if (parsed.refused)
        done({}, failure(name, reply, Problem::KeyRefused), Problem::KeyRefused);
    else if (!parsed.error.isEmpty())
        done({}, name + QStringLiteral(": ") + parsed.error, Problem::Failed);
    else if (reply.status == 400)
        nothing(done); // (no such show or episode)
    else
        return false;
    return true;
}

void searchBetaSeries(QObject *context, const Query &query, Done done)
{
    const QUrl url = betaSeriesShowUrl(query);
    if (query.credentials.key.isEmpty() || url.isEmpty())
    {
        nothing(done);
        return;
    }
    QPointer<QObject> guard(context);
    askParsed<Parsed>(context, jsonRequest(url), [](const HttpResponse &reply) {
        return parseBetaSeriesAnswer(reply, true);
    }, [guard, query, done](const HttpResponse &reply, const Parsed &parsed) {
        if (!guard || betaSeriesFailed(reply, parsed, done))
            return;
        const QUrl episode = betaSeriesEpisodeUrl(parsed.id, query);
        if (episode.isEmpty())
        {
            nothing(done);
            return;
        }
        QTimer::singleShot(requestGapMs, guard, [guard, episode, done]() {
            if (!guard)
                return;
            askParsed<Parsed>(guard, jsonRequest(episode), [](const HttpResponse &reply) {
                return parseBetaSeriesAnswer(reply, false);
            }, [done](const HttpResponse &reply, const Parsed &parsed) {
                if (betaSeriesFailed(reply, parsed, done))
                    return;
                done(parsed.results, {}, Problem::None);
            });
        });
    });
}

HttpRequest subsRoRequest(const QUrl &url, const QString &key)
{
    HttpRequest request = jsonRequest(url);
    request.headers.append(qMakePair(QByteArray("X-Subs-Api-Key"), key.toUtf8()));
    return request;
}

void searchSubsRo(QObject *context, const Query &query, Done done)
{
    const QUrl url = subsRoSearchUrl(query);
    if (query.credentials.key.isEmpty() || url.isEmpty())
    {
        nothing(done);
        return;
    }
    askParsed<Parsed>(context, subsRoRequest(url, query.credentials.key), [query](const HttpResponse &reply) {
        Parsed parsed;
        if (ok(reply))
            parsed.results = parseSubsRo(reply.body, query, &parsed.error);
        return parsed;
    }, [done](const HttpResponse &reply, const Parsed &parsed) {
        const QString name = QStringLiteral("Subs.ro");
        if (reply.error.isEmpty() && reply.status == 404)
        {
            nothing(done);
            return;
        }
        if (!ok(reply))
        {
            failed(done, name, reply, true);
            return;
        }
        if (!parsed.error.isEmpty())
            done({}, name + QStringLiteral(": ") + parsed.error, Problem::Failed);
        else
            done(parsed.results, {}, Problem::None);
    });
}

const QUrl &napisy24Url()
{
    static const QUrl url(QStringLiteral("https://napisy24.pl/run/CheckSubAgent.php"));
    return url;
}

HttpRequest napisy24Request(const Query &query)
{
    HttpRequest request = makeRequest(napisy24Url());
    request.method = "POST";
    request.body = napisy24Body(query, query.credentials.username.trimmed(), query.credentials.password);
    request.headers.append(qMakePair(QByteArray("Content-Type"), QByteArray("application/x-www-form-urlencoded")));
    return request;
}

// Napisy24: the subtitle made for this very file (its hash), Polish
void searchNapisy24(QObject *context, const Query &query, Done done)
{
    if (query.hash.isEmpty() || query.size <= 0 || query.credentials.username.trimmed().isEmpty()
        || query.credentials.password.isEmpty())
    {
        nothing(done);
        return;
    }
    httpRequestAsync(context, napisy24Request(query), [query, done](const HttpResponse &reply) {
        const QString name = QStringLiteral("Napisy24");
        if (!ok(reply))
        {
            failed(done, name, reply);
            return;
        }
        const Napisy24Answer answer = parseNapisy24(reply.body);
        if (answer.loginRefused)
        {
            done({}, failure(name, reply, Problem::LoginRefused), Problem::LoginRefused);
            return;
        }
        if (!answer.found)
        {
            nothing(done);
            return;
        }
        Result r;
        r.id = QStringLiteral("napisy24:%1-%2").arg(query.hash).arg(query.size);
        r.provider = QStringLiteral("napisy24");
        r.language = QStringLiteral("pl");
        r.release = answer.title.isEmpty() ? QFileInfo(query.fileName).completeBaseName() : answer.title;
        r.fileName = query.fileName;
        r.url = napisy24Url();
        r.exact = true;
        done({ r }, {}, Problem::None);
    });
}

void searchLegendas(QObject *context, const Query &query, Done done)
{
    const QByteArray body = legendasSearchBody(query);
    if (body.isEmpty())
    {
        nothing(done);
        return;
    }
    QPointer<QObject> guard(context);
    const QString name = QStringLiteral("Legendas.net");
    withSession(context, QStringLiteral("legendasnet"), name, query, done,
                [guard, query, body, name, done](const Session &session, std::function<void()> again) {
        if (!guard)
            return;
        // (a GET with a JSON body, as its API asks)
        HttpRequest request = jsonRequest(legendasSearchUrl(query));
        request.body = body;
        request.headers.append(qMakePair(QByteArray("Content-Type"), QByteArray("application/json")));
        request.headers.append(qMakePair(QByteArray("Authorization"), "Bearer " + session.token.toUtf8()));
        askParsed<QList<Result>>(guard, request, [query](const HttpResponse &reply) {
            return ok(reply) ? parseLegendas(reply.body, query) : QList<Result>();
        }, [name, done, again](const HttpResponse &reply, const QList<Result> &results) {
            if (reply.error.isEmpty() && reply.status == 401 && again)
            {
                again();
                return;
            }
            if (reply.error.isEmpty() && reply.status == 404)
            {
                nothing(done);
                return;
            }
            if (!ok(reply))
            {
                const Problem problem = reply.status == 401 ? Problem::LoginRefused : problemOfReply(reply);
                done({}, failure(name, reply, problem), problem);
                return;
            }
            done(results, {}, Problem::None);
        });
    });
}

// (gzip and zip: gunzip() and unzip(), in subtitle_services.cpp, tested by
// themselves)

bool validUtf8(const QByteArray &data)
{
    QStringDecoder utf8(QStringDecoder::Utf8);
    // decode() is lazy: the text must be made for hasError() to be set
    const QString text = utf8.decode(data);
    Q_UNUSED(text);
    return !utf8.hasError();
}

// The Windows code page of a language's subtitles that aren't in UTF-8
unsigned codePageOf(const QString &language)
{
    static const QHash<QString, unsigned> pages = {
        { "he", 1255 }, { "yi", 1255 },
        { "ar", 1256 }, { "fa", 1256 }, { "ur", 1256 }, { "prs", 1256 }, { "ckb", 1256 },
        { "ru", 1251 }, { "uk", 1251 }, { "bg", 1251 }, { "be", 1251 }, { "mk", 1251 },
        { "sr", 1251 }, { "kk", 1251 }, { "ky", 1251 }, { "tg", 1251 }, { "mn", 1251 },
        { "el", 1253 }, { "tr", 1254 }, { "az", 1254 },
        { "cs", 1250 }, { "sk", 1250 }, { "pl", 1250 }, { "hu", 1250 }, { "ro", 1250 },
        { "hr", 1250 }, { "sl", 1250 }, { "bs", 1250 }, { "sq", 1250 }, { "cnr", 1250 },
        { "lt", 1257 }, { "lv", 1257 }, { "et", 1257 }, { "vi", 1258 }, { "th", 874 },
        { "ja", 932 }, { "zh", 936 }, { "zh-TW", 950 }, { "yue", 950 }, { "ko", 949 },
    };
    return pages.value(language, 1252);
}

// The code page an encoding is named by ("CP1255", "windows-1256",
// "ISO-8859-8", "UTF-8": 65001), 0 when unknown
unsigned codePageNamed(const QString &encoding)
{
    const QString e = encoding.trimmed().toLower().remove(QLatin1Char('_'));
    if (e.isEmpty())
        return 0;
    if (e == QLatin1String("utf-8") || e == QLatin1String("utf8"))
        return 65001;
    static const QRegularExpression cpRe(QStringLiteral(R"(^(?:cp|windows-?|ms)(\d{3,4})$)"));
    const QRegularExpressionMatch cp = cpRe.match(e);
    if (cp.hasMatch())
        return cp.captured(1).toUInt();
    static const QRegularExpression isoRe(QStringLiteral(R"(^iso-?8859-(\d{1,2})$)"));
    const QRegularExpressionMatch iso = isoRe.match(e);
    if (iso.hasMatch())
        return 28590 + iso.captured(1).toUInt();
    static const QHash<QString, unsigned> named = {
        { "koi8-r", 20866 }, { "koi8-u", 21866 }, { "gb2312", 936 }, { "gbk", 936 }, { "gb18030", 54936 },
        { "big5", 950 }, { "shift-jis", 932 }, { "shiftjis", 932 }, { "sjis", 932 }, { "euc-kr", 949 },
        { "euc-jp", 20932 }, { "tis-620", 874 }, { "ascii", 1252 }, { "us-ascii", 1252 },
    };
    return named.value(e, 0);
}

#ifndef _WIN32
// The iconv name of a Windows code page number
QByteArray iconvNameOf(unsigned page)
{
    if (page >= 28591 && page <= 28606)
        return "ISO-8859-" + QByteArray::number(page - 28590);
    switch (page)
    {
        case 20866: return "KOI8-R";
        case 21866: return "KOI8-U";
        case 20932: return "EUC-JP";
        case 54936: return "GB18030";
        default:    return "CP" + QByteArray::number(page);
    }
}

// Converts with iconv; a sequence it cannot convert becomes U+FFFD when
// decoding (as MultiByteToWideChar), and fails when encoding
bool convertIconv(const char *to, const char *from, const QByteArray &in, QByteArray &out,
                  bool replace)
{
    vlc_iconv_t cd = vlc_iconv_open(to, from);
    if (cd == (vlc_iconv_t)(-1))
        return false;
    out.resize(in.size() * 4 + 16);
    const char *src = in.constData();
    size_t srcLeft = size_t(in.size());
    char *dst = out.data();
    size_t dstLeft = size_t(out.size());
    bool ok = true;
    while (srcLeft > 0)
    {
        if (vlc_iconv(cd, &src, &srcLeft, &dst, &dstLeft) != (size_t)(-1))
            continue;
        if ((errno == EILSEQ || errno == EINVAL) && replace && dstLeft >= 3)
        {
            ++src;
            --srcLeft;
            memcpy(dst, "\xEF\xBF\xBD", 3);
            dst += 3;
            dstLeft -= 3;
            continue;
        }
        ok = false;
        break;
    }
    // the last character: some converters hold it back for the combining
    // marks that may follow (glibc's CP1255 and CP1258)
    if (ok && vlc_iconv(cd, nullptr, nullptr, &dst, &dstLeft) == (size_t)(-1))
        ok = false;
    vlc_iconv_close(cd);
    out.resize(out.size() - qsizetype(dstLeft));
    return ok;
}
#endif

QString decodePage(const QByteArray &data, unsigned page)
{
    if (page == 65001)
        return QString::fromUtf8(data);
#ifndef _WIN32
    QByteArray utf8;
    if (convertIconv("UTF-8", iconvNameOf(page).constData(), data, utf8, true))
        return QString::fromUtf8(utf8);
#endif
#ifdef _WIN32
    const int length = MultiByteToWideChar(page, 0, data.constData(), int(data.size()), nullptr, 0);
    if (length > 0)
    {
        std::wstring wide(size_t(length), L'\0');
        MultiByteToWideChar(page, 0, data.constData(), int(data.size()), wide.data(), length);
        return QString::fromStdWString(wide);
    }
#else
    Q_UNUSED(page);
#endif
    return QString::fromLatin1(data);
}

// Whether a text has letters of the script of a language's code page
bool hasScriptOf(const QString &text, unsigned page)
{
    char16_t lo = 0, hi = 0;
    switch (page)
    {
        case 1255: lo = 0x0590; hi = 0x05FF; break;   // Hebrew
        case 1256: lo = 0x0600; hi = 0x06FF; break;   // Arabic
        case 1251: lo = 0x0400; hi = 0x04FF; break;   // Cyrillic
        case 1253: lo = 0x0370; hi = 0x03FF; break;   // Greek
        case 874:  lo = 0x0E00; hi = 0x0E7F; break;   // Thai
        default: return true;
    }
    int found = 0;
    for (const QChar c : text)
        if (c.unicode() >= lo && c.unicode() <= hi && ++found > 20)
            return true;
    return found > 0;
}

// Text of another code page saved as UTF-8 by way of Windows-1252 (letters
// as "àáâ...": Addic7ed's Hebrew): back to its own letters
QString repairDoubleEncoding(const QString &text, unsigned page)
{
#ifdef _WIN32
    if (hasScriptOf(text, page))
        return text;
    int latin = 0;
    for (const QChar c : text)
        if (c.unicode() >= 0xC0 && c.unicode() <= 0xFF)
            ++latin;
    if (latin < 20)
        return text;
    const std::wstring wide = text.toStdWString();
    const int length = WideCharToMultiByte(1252, 0, wide.c_str(), int(wide.size()), nullptr, 0, nullptr, nullptr);
    if (length <= 0)
        return text;
    QByteArray bytes(length, '\0');
    WideCharToMultiByte(1252, 0, wide.c_str(), int(wide.size()), bytes.data(), length, nullptr, nullptr);
    const QString repaired = decodePage(bytes, page);
    return hasScriptOf(repaired, page) ? repaired : text;
#else
    if (hasScriptOf(text, page))
        return text;
    int latin = 0;
    for (const QChar c : text)
        if (c.unicode() >= 0xC0 && c.unicode() <= 0xFF)
            ++latin;
    if (latin < 20)
        return text;
    QByteArray bytes;
    if (!convertIconv("CP1252", "UTF-8", text.toUtf8(), bytes, false))
        return text;
    const QString repaired = decodePage(bytes, page);
    return hasScriptOf(repaired, page) ? repaired : text;
#endif
}

// The text: its BOM, else UTF-8 when it is, else the encoding the service
// said, else the code page of its language
QString decodeText(const QByteArray &data, const QString &encoding, const QString &language)
{
    if (data.startsWith("\xEF\xBB\xBF"))
        return QString::fromUtf8(data.mid(3));
    if (data.startsWith("\xFF\xFE"))
        return QString::fromUtf16(reinterpret_cast<const char16_t *>(data.constData() + 2), (data.size() - 2) / 2);
    if (data.startsWith("\xFE\xFF"))
    {
        QStringDecoder utf16be(QStringDecoder::Utf16BE);
        return utf16be.decode(data.mid(2));
    }
    if (validUtf8(data))
        return QString::fromUtf8(data);
    unsigned page = codePageNamed(encoding);
    if (page == 0 || page == 65001)
        page = codePageOf(language);
    return decodePage(data, page);
}

// The ads a service adds as subtitles of their own (OpenSubtitles.org puts
// one at the start), and the placeholder of the subtitles it keeps for its
// paying members
const QRegularExpression &adRe()
{
    static const QRegularExpression re(QStringLiteral(
        R"(osdb\.link|(?:get|try)ray\.app|opensubtitles\.(?:org|com)|Advertise your product|become VIP member|api\.opensubtitles)"),
        QRegularExpression::CaseInsensitiveOption);
    return re;
}

QString withoutAds(const QString &text)
{
    // SubRip: blocks between empty lines
    static const QRegularExpression blankRe(QStringLiteral(R"(\r?\n[ \t]*\r?\n)"));
    const QStringList blocks = text.split(blankRe);
    if (blocks.size() < 3 || !text.contains(QLatin1String("-->")))
        return text;
    QStringList kept;
    for (const QString &block : blocks)
        if (!adRe().match(block).hasMatch())
            kept.append(block);
    return kept.join(QStringLiteral("\n\n"));
}

QString extensionOf(const QString &text, const QString &name)
{
    const QString start = text.left(512).trimmed();
    if (start.startsWith(QLatin1String("WEBVTT")))
        return QStringLiteral("vtt");
    if (start.contains(QLatin1String("[Script Info]")))
        return QStringLiteral("ass");
    const QString suffix = QFileInfo(name).suffix().toLower();
    if (suffix == QLatin1String("sub") || suffix == QLatin1String("smi") || suffix == QLatin1String("ssa"))
        return suffix;
    return QStringLiteral("srt");
}

} // namespace

QByteArray userAgent()
{
    return QByteArray("VLC-Redesign/" VLC_REDESIGN_VERSION);
}

const QList<Provider> &providers()
{
    // (the order of the services page: the four free ones without an
    // account keep theirs, each keyed one after the one it is most like)
    static const QList<Provider> list = {
        { QStringLiteral("opensubtitles"), QStringLiteral("OpenSubtitles"),
          N_("All languages · movies and series · finds those made for your file"), true, true, {}, 0, 0, {}, false },
        { QStringLiteral("opensubtitlescom"), QStringLiteral("OpenSubtitles.com"),
          N_("With your own free key · all languages"), true, true, {},
          KeyField | UsernameField | PasswordField, KeyField,
          QStringLiteral("https://www.opensubtitles.com/en/consumers"), false },
        { QStringLiteral("subsource"), QStringLiteral("SubSource"),
          N_("With your own free key · strong in Arabic, many languages"), true, true, {},
          KeyField, KeyField, QStringLiteral("https://subsource.net"), false },
        { QStringLiteral("subdl"), QStringLiteral("SubDL"),
          N_("With your own free key · the Subscene archive, many languages"), true, true, {},
          KeyField, KeyField, QStringLiteral("https://subdl.com/panel/api"), false },
        { QStringLiteral("wizdom"), QStringLiteral("Wizdom"),
          N_("Hebrew · movies and series"), true, true, { QStringLiteral("he") }, 0, 0, {}, false },
        { QStringLiteral("addic7ed"), QStringLiteral("Addic7ed"),
          N_("Series · many languages"), false, true, {}, 0, 0, {}, false },
        { QStringLiteral("yify"), QStringLiteral("YIFY Subtitles"),
          N_("Movies · many languages"), true, false, {}, 0, 0, {}, false },
        // with an account or a key, each for its languages (asked only when
        // one of them is to be searched)
        { QStringLiteral("titlovi"), QStringLiteral("Titlovi"),
          N_("With your free account · Croatian, Serbian, Bosnian, Slovenian, Macedonian"), true, true,
          { QStringLiteral("hr"), QStringLiteral("sr"), QStringLiteral("bs"), QStringLiteral("sl"), QStringLiteral("mk") },
          UsernameField | PasswordField, UsernameField | PasswordField, QStringLiteral("https://titlovi.com"), true },
        { QStringLiteral("assrt"), QStringLiteral("Assrt"),
          N_("With your free token · Chinese"), true, true, { QStringLiteral("zh"), QStringLiteral("zh-TW") },
          KeyField, KeyField, QStringLiteral("https://assrt.net/usercp.php"), false },
        { QStringLiteral("jimaku"), QStringLiteral("Jimaku"),
          N_("With your free key · anime, Japanese"), true, true, { QStringLiteral("ja") },
          KeyField, KeyField, QStringLiteral("https://jimaku.cc/account"), false },
        { QStringLiteral("betaseries"), QStringLiteral("BetaSeries"),
          N_("With your free key · series, French"), false, true, { QStringLiteral("fr") },
          KeyField, KeyField, QStringLiteral("https://www.betaseries.com/api/"), false },
        { QStringLiteral("subsro"), QStringLiteral("Subs.ro"),
          N_("With your free key · Romanian"), true, true, { QStringLiteral("ro") },
          KeyField, KeyField, QStringLiteral("https://subs.ro/api"), false },
        { QStringLiteral("napisy24"), QStringLiteral("Napisy24"),
          N_("With your free account · Polish · finds those made for your file"), true, true, { QStringLiteral("pl") },
          UsernameField | PasswordField, UsernameField | PasswordField, QStringLiteral("https://napisy24.pl"), true },
        { QStringLiteral("legendasnet"), QStringLiteral("Legendas.net"),
          N_("With your free account · Portuguese (Brazil)"), true, true, { QStringLiteral("pt-BR") },
          EmailField | PasswordField, EmailField | PasswordField, QStringLiteral("https://legendas.net"), true },
    };
    return list;
}

const Provider *provider(const QString &id)
{
    for (const Provider &p : providers())
        if (p.id == id)
            return &p;
    return nullptr;
}

bool hasRequired(const Provider &provider, const Credentials &credentials)
{
    return (!(provider.required & KeyField) || !credentials.key.trimmed().isEmpty())
        && (!(provider.required & UsernameField) || !credentials.username.trimmed().isEmpty())
        && (!(provider.required & PasswordField) || !credentials.password.isEmpty())
        && (!(provider.required & EmailField) || !credentials.email.trimmed().isEmpty());
}

void search(QObject *context, const QString &providerId, const Query &query, Done done)
{
    if (providerId == QLatin1String("opensubtitles"))
        searchOpenSubtitles(context, query, done);
    else if (providerId == QLatin1String("opensubtitlescom"))
        searchOpenSubtitlesCom(context, query, done);
    else if (providerId == QLatin1String("subsource"))
        searchSubSource(context, query, done);
    else if (providerId == QLatin1String("subdl"))
        searchSubDl(context, query, done);
    else if (providerId == QLatin1String("wizdom"))
        searchWizdom(context, query, done);
    else if (providerId == QLatin1String("addic7ed"))
        searchAddic7ed(context, query, done);
    else if (providerId == QLatin1String("yify"))
        searchYify(context, query, done);
    else if (providerId == QLatin1String("titlovi"))
        searchTitlovi(context, query, done);
    else if (providerId == QLatin1String("assrt"))
        searchAssrt(context, query, done);
    else if (providerId == QLatin1String("jimaku"))
        searchJimaku(context, query, done);
    else if (providerId == QLatin1String("betaseries"))
        searchBetaSeries(context, query, done);
    else if (providerId == QLatin1String("subsro"))
        searchSubsRo(context, query, done);
    else if (providerId == QLatin1String("napisy24"))
        searchNapisy24(context, query, done);
    else if (providerId == QLatin1String("legendasnet"))
        searchLegendas(context, query, done);
    else
        nothing(done);
}

void forgetSession(const QString &providerId)
{
    sessions().remove(providerId);
}

void check(QObject *context, const QString &providerId, const Credentials &credentials,
           std::function<void(Check check, const QString &who)> done)
{
    const QString key = credentials.key.trimmed();
    if (providerId == QLatin1String("opensubtitlescom"))
    {
        if (!credentials.username.trimmed().isEmpty() && !credentials.password.isEmpty())
        {
            // signed in afresh with these, the session not kept (they may
            // never be saved; the session of those kept stays)
            Credentials typed = credentials;
            typed.key = key;
            typed.username = credentials.username.trimmed();
            const QString who = typed.username;
            openSubtitlesComSignIn(context, typed, [done, who](const Session &, Problem problem) {
                switch (problem)
                {
                    case Problem::None: done(Check::SignedIn, who); break;
                    case Problem::LoginRefused: done(Check::LoginRefused, {}); break;
                    case Problem::KeyRefused: done(Check::KeyRefused, {}); break;
                    case Problem::Busy: done(Check::Busy, {}); break;
                    default: done(Check::NoAnswer, {}); break;
                }
            }, true);
            return;
        }
        httpRequestAsync(context, openSubtitlesComRequest(openSubtitlesComCheckUrl(), key),
                         [done](const HttpResponse &reply) { done(checkOf(reply.status), {}); });
        return;
    }
    if (providerId == QLatin1String("subsource"))
    {
        HttpRequest request = jsonRequest(subSourceCheckUrl());
        request.headers.append(qMakePair(QByteArray("X-API-Key"), key.toUtf8()));
        httpRequestAsync(context, request, [done](const HttpResponse &reply) { done(checkOf(reply.status), {}); });
        return;
    }
    if (providerId == QLatin1String("subdl"))
    {
        HttpRequest request = jsonRequest(subDlCheckUrl(key, true));
        request.headers.append(qMakePair(QByteArray("Authorization"), "Bearer " + key.toUtf8()));
        QPointer<QObject> guard(context);
        httpRequestAsync(context, request, [guard, key, done](const HttpResponse &reply) {
            if (!guard)
                return;
            if (reply.error.isEmpty() && reply.status == 404)
            {
                httpRequestAsync(guard, jsonRequest(subDlCheckUrl(key, false)),
                                 [done](const HttpResponse &v1) { done(checkOf(v1.status), {}); });
                return;
            }
            done(checkOf(reply.status), {});
        });
        return;
    }
    const auto checkOfSignIn = [](Problem problem) {
        switch (problem)
        {
            case Problem::None: return Check::SignedIn;
            case Problem::LoginRefused: return Check::LoginRefused;
            case Problem::KeyRefused: return Check::KeyRefused;
            case Problem::Busy: return Check::Busy;
            default: return Check::NoAnswer;
        }
    };
    if (providerId == QLatin1String("titlovi") || providerId == QLatin1String("legendasnet"))
    {
        // (afresh, the session not kept: as OpenSubtitles.com's above)
        const QString who = whoOf(providerId, credentials);
        signIn(context, providerId, credentials, [done, who, checkOfSignIn](const Session &, Problem problem) {
            const Check result = checkOfSignIn(problem);
            done(result, result == Check::SignedIn ? who : QString());
        }, true);
        return;
    }
    if (providerId == QLatin1String("napisy24"))
    {
        // (a file it doesn't know: what it says of the account)
        Query probe;
        probe.hash = QStringLiteral("0000000000000000");
        probe.size = 1;
        probe.fileName = QStringLiteral("check.mkv");
        probe.credentials = credentials;
        const QString who = credentials.username.trimmed();
        httpRequestAsync(context, napisy24Request(probe), [done, who](const HttpResponse &reply) {
            if (!ok(reply))
                done(checkOf(reply.status) == Check::Works ? Check::NoAnswer : checkOf(reply.status), {});
            else if (parseNapisy24(reply.body).loginRefused)
                done(Check::LoginRefused, {});
            else if (reply.body.startsWith("OK"))
                done(Check::SignedIn, who);
            else
                done(Check::NoAnswer, {});
        });
        return;
    }
    if (providerId == QLatin1String("assrt"))
    {
        httpRequestAsync(context, jsonRequest(assrtCheckUrl(key)), [done](const HttpResponse &reply) {
            if (!reply.error.isEmpty() || (reply.status != 200 && reply.status != 400))
            {
                done(checkOf(reply.status), {});
                return;
            }
            const QJsonObject answer = QJsonDocument::fromJson(reply.body).object();
            const int status = answer.value(QLatin1String("status")).toVariant().toInt();
            done(answer.isEmpty() ? Check::NoAnswer : status == 0 ? Check::Works
                 : status == 20001 ? Check::KeyRefused : Check::NoAnswer, {});
        });
        return;
    }
    if (providerId == QLatin1String("jimaku"))
    {
        Query probe;
        probe.title = QStringLiteral("Frieren");
        httpRequestAsync(context, jimakuRequest(jimakuSearchUrl(probe), key),
                         [done](const HttpResponse &reply) { done(checkOf(reply.status), {}); });
        return;
    }
    if (providerId == QLatin1String("betaseries"))
    {
        Query probe;
        probe.series = true;
        probe.imdb = QStringLiteral("tt0903747");
        probe.credentials.key = key;
        httpRequestAsync(context, jsonRequest(betaSeriesShowUrl(probe)), [done](const HttpResponse &reply) {
            if (!reply.error.isEmpty() || (reply.status != 200 && reply.status != 400))
            {
                done(checkOf(reply.status), {});
                return;
            }
            QString error;
            bool refused = false;
            parseBetaSeries(reply.body, &error, &refused);
            done(refused ? Check::KeyRefused : reply.status == 200 ? Check::Works : Check::NoAnswer, {});
        });
        return;
    }
    if (providerId == QLatin1String("subsro"))
    {
        Query probe;
        probe.imdb = QStringLiteral("tt0133093");
        httpRequestAsync(context, subsRoRequest(subsRoSearchUrl(probe), key),
                         [done](const HttpResponse &reply) { done(checkOf(reply.status), {}); });
        return;
    }
    done(Check::Works, {});
}

namespace {

// OpenSubtitles.com: the link of a file, signed in when the user gave the
// account (a higher quota), else with the key alone
void openSubtitlesComDownload(QObject *context, const Result &result, const Credentials &credentials,
                              std::function<void(const Download &)> done, bool again = false)
{
    const QString fileId = result.id.section(QLatin1Char(':'), 1);
    const QString name = QStringLiteral("OpenSubtitles.com");
    QPointer<QObject> guard(context);
    const auto ask = [guard, fileId, name, result, credentials, done, again](const Session &session) {
        if (!guard)
            return;
        const QUrl base = session.base.isEmpty() ? QUrl(QStringLiteral("https://api.opensubtitles.com/api/v1/"))
                                                 : session.base;
        HttpRequest request = openSubtitlesComRequest(base.resolved(QUrl(QStringLiteral("download"))),
                                                      credentials.key.trimmed(), session.token);
        request.method = "POST";
        request.body = openSubtitlesComDownloadBody(fileId);
        httpRequestAsync(guard, request, [guard, name, result, credentials, done, again, session](const HttpResponse &reply) {
            if (!guard)
                return;
            Download download;
            if (!ok(reply))
            {
                // (its token ran out: signed in again, once)
                if (reply.status == 401 && !session.token.isEmpty() && !again)
                {
                    forgetSession(QStringLiteral("opensubtitlescom"));
                    openSubtitlesComDownload(guard, result, credentials, done, true);
                    return;
                }
                download.problem = problemOfReply(reply);
                if (reply.status == 406 || reply.status == 429)
                {
                    // the downloads of the day used up
                    download.problem = Problem::Busy;
                    download.error = qtr("OpenSubtitles download failed. Check your quota.");
                }
                else
                    download.error = failure(name, reply, download.problem);
                done(download);
                return;
            }
            download.url = openSubtitlesComLink(reply.body);
            if (download.url.isEmpty())
            {
                download.problem = Problem::Failed;
                download.error = qtr("OpenSubtitles did not return a valid subtitle download link.");
            }
            else
                download.headers.append(qMakePair(QByteArray("User-Agent"), openSubtitlesComAgent()));
            done(download);
        });
    };
    if (credentials.username.trimmed().isEmpty() || credentials.password.isEmpty())
    {
        ask({});
        return;
    }
    Credentials typed = credentials;
    typed.username = credentials.username.trimmed();
    typed.key = credentials.key.trimmed();
    openSubtitlesComSignIn(context, typed, [ask, done, name](const Session &session, Problem problem) {
        if (problem == Problem::LoginRefused || problem == Problem::KeyRefused)
        {
            Download download;
            download.problem = problem;
            download.error = failure(name, {}, problem);
            done(download);
            return;
        }
        // (no answer to the sign-in: with the key alone)
        ask(session);
    });
}

} // namespace

void prepareDownload(QObject *context, const Result &result, const Credentials &credentials,
                     std::function<void(const Download &download)> done)
{
    if (result.provider == QLatin1String("opensubtitlescom"))
    {
        openSubtitlesComDownload(context, result, credentials, done);
        return;
    }
    Download download;
    // the user's key, account or session go with the download of these:
    // over HTTPS only, never in clear text (their parsers take no other
    // link; this keeps it so whatever a result says)
    const bool keyed = result.provider == QLatin1String("subsource") || result.provider == QLatin1String("jimaku")
                    || result.provider == QLatin1String("subsro") || result.provider == QLatin1String("napisy24")
                    || result.provider == QLatin1String("legendasnet");
    if (keyed && result.url.scheme() != QLatin1String("https"))
    {
        download.problem = Problem::Failed;
        done(download); // (no address: "couldn't be downloaded")
        return;
    }
    download.url = result.url;
    download.headers.append(qMakePair(QByteArray("User-Agent"), userAgent()));
    if (result.provider == QLatin1String("opensubtitles"))
        download.headers.append(qMakePair(QByteArray("X-User-Agent"), userAgent()));
    // (YIFY's downloads are from its pages)
    if (result.provider == QLatin1String("yify"))
        download.headers.append(qMakePair(QByteArray("Referer"), QByteArray("https://yifysubtitles.ch/")));
    if (result.provider == QLatin1String("subsource"))
        download.headers.append(qMakePair(QByteArray("X-API-Key"), credentials.key.trimmed().toUtf8()));
    if (result.provider == QLatin1String("jimaku"))
        download.headers.append(qMakePair(QByteArray("Authorization"), credentials.key.trimmed().toUtf8()));
    if (result.provider == QLatin1String("subsro"))
        download.headers.append(qMakePair(QByteArray("X-Subs-Api-Key"), credentials.key.trimmed().toUtf8()));
    if (result.provider == QLatin1String("napisy24"))
    {
        // the subtitle comes with the answer of the file's search, asked again
        const QString hashAndSize = result.id.section(QLatin1Char(':'), 1);
        Query file;
        file.hash = hashAndSize.section(QLatin1Char('-'), 0, 0);
        file.size = hashAndSize.section(QLatin1Char('-'), 1).toLongLong();
        file.fileName = result.fileName;
        file.credentials = credentials;
        const HttpRequest request = napisy24Request(file);
        download.method = request.method;
        download.body = request.body;
        download.headers = request.headers;
        download.unwrap = [](const QByteArray &answer) { return parseNapisy24(answer).archive; };
    }
    if (result.provider == QLatin1String("assrt"))
    {
        // the file of its language, from the subtitle's details
        const QString id = result.id.section(QLatin1Char(':'), 1);
        const QString language = result.language;
        httpRequestAsync(context, jsonRequest(assrtDetailUrl(credentials.key.trimmed(), id)),
                         [download, language, done](const HttpResponse &reply) {
            Download prepared = download;
            prepared.url = ok(reply) ? assrtFileOf(reply.body, language) : QUrl();
            if (prepared.url.isEmpty())
            {
                prepared.problem = ok(reply) ? Problem::Failed : problemOfReply(reply);
                prepared.error = failure(QStringLiteral("Assrt"), reply,
                                         reply.status == 400 ? Problem::KeyRefused : Problem::None);
            }
            done(prepared);
        });
        return;
    }
    if (result.provider == QLatin1String("legendasnet"))
    {
        signIn(context, result.provider, credentials, [download, done](const Session &session, Problem problem) {
            Download prepared = download;
            if (problem != Problem::None)
            {
                prepared.url.clear();
                prepared.problem = problem;
                prepared.error = failure(QStringLiteral("Legendas.net"), {}, problem);
            }
            else
                prepared.headers.append(qMakePair(QByteArray("Authorization"), "Bearer " + session.token.toUtf8()));
            done(prepared);
        });
        return;
    }
    done(download);
}

QByteArray subtitleOf(const QByteArray &download, const Result &result, const QString &videoName,
                      QString *extension, QString *error)
{
    QByteArray data = download;
    QString name = result.fileName;
    if (data.startsWith("Rar!"))
    {
        *error = qtr("This subtitle comes as a RAR archive, which can't be opened here.");
        return {};
    }
    if (data.startsWith("\x1F\x8B"))
    {
        // (whole, or nothing: a file cut short isn't saved as the subtitle)
        data = gunzip(data, maxUnpackedBytes);
        if (data.isEmpty())
        {
            *error = qtr("The subtitle couldn't be unpacked.");
            return {};
        }
    }
    else if (data.startsWith("PK\x03\x04"))
    {
        // the subtitle of the archive: the one named most like the video,
        // in UTF-8 rather than not when there are two (Wizdom gives both)
        const QList<QPair<QString, QByteArray>> files = unzip(data);
        const QString video = simplified(QFileInfo(videoName).completeBaseName());
        double best = -1e9;
        data.clear();
        for (const auto &file : files)
        {
            if (!isSubtitleName(file.first) || file.second.isEmpty())
                continue;
            const QString entry = simplified(QFileInfo(file.first).completeBaseName());
            double score = validUtf8(file.second) ? 5. : 0.;
            if (!video.isEmpty() && entry == video)
                score += 20.;
            for (const QString &word : video.split(QLatin1Char(' '), Qt::SkipEmptyParts))
                if (word.size() > 1 && entry.contains(word))
                    score += 1.;
            score += std::log10(double(file.second.size()) + 1.) * 0.1;
            if (score > best)
            {
                best = score;
                data = file.second;
                name = file.first;
            }
        }
        if (data.isEmpty())
        {
            *error = files.isEmpty() ? qtr("The subtitle couldn't be unpacked.")
                                     : qtr("The archive has no subtitle in it.");
            return {};
        }
    }

    QString text = decodeText(data, result.encoding, result.language);
    const unsigned page = codePageOf(result.language);
    if (page != 1252)
        text = repairDoubleEncoding(text, page);
    if (name.contains(QLatin1String("only-for-vip"), Qt::CaseInsensitive)
        || text.contains(QLatin1String("osdb.link/vip"), Qt::CaseInsensitive))
    {
        *error = qtr("OpenSubtitles keeps this subtitle for its paying members. Choose another one.");
        return {};
    }
    text = withoutAds(text);
    if (text.trimmed().isEmpty())
    {
        *error = qtr("The subtitle is empty.");
        return {};
    }
    *extension = extensionOf(text, name);
    return text.toUtf8();
}

} // namespace SubtitleProviders
