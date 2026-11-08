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
#ifdef HAVE_ZLIB
# include <zlib.h>
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
constexpr qsizetype maxDownloadBytes = 8 * 1024 * 1024;
// what a download may unpack to in all (a small archive can unpack to
// gigabytes), and how many files of an archive are looked at, and unpacked
constexpr qsizetype maxUnpackedBytes = 16 * 1024 * 1024;
constexpr int maxArchiveEntries = 1000;
constexpr int maxArchiveSubtitles = 64;
// between two requests to the same service (they ask for one at a time)
constexpr int requestGapMs = 600;

using Done = std::function<void(const QList<Result> &, const QString &, Problem)>;

HttpRequest makeRequest(const QUrl &url, qsizetype maxBytes = maxListBytes, int timeoutMs = 15000)
{
    HttpRequest request;
    request.url = url;
    request.timeoutMs = timeoutMs;
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
    httpRequestAsync(context, request, [context, chain, done](const HttpResponse &reply) {
        if (ok(reply))
        {
            ++chain->answered;
            parseOpenSubtitles(reply.body, chain->results, chain->seen);
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
    httpRequestAsync(context, request, [done](const HttpResponse &reply) {
        done(ok(reply) ? identityOfHash(reply.body) : Identity());
    });
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
    httpRequestAsync(context, makeRequest(url), [done](const HttpResponse &reply) {
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
        QList<Result> results;
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
            r.release = o.value(QLatin1String("versioname")).toString();
            r.release.remove(QRegularExpression(QStringLiteral(R"(^[\s*\-:.|]+|[\s*\-:.|]+$)")));
            r.url = QUrl(QStringLiteral("https://wizdom.xyz/api/files/sub/") + id);
            r.rank = o.value(QLatin1String("score")).toDouble() * 5.;
            results.append(r);
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
    httpRequestAsync(context, makeRequest(url), [guard, query, done](const HttpResponse &reply) {
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
        // the show of that name
        QString showId;
        double best = 0;
        const QString wanted = simplified(query.title);
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
            httpRequestAsync(guard, makeRequest(url), [guard, language, results, self](const HttpResponse &reply) {
                if (!guard)
                    return;
                if (ok(reply))
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
                        r.rank = std::log10(double(r.downloads) + 1.) * 3.
                               + (o.value(QLatin1String("corrected")).toBool() ? 3. : 0.);
                        results->append(r);
                    }
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
    httpRequestAsync(context, makeRequest(url), [done](const HttpResponse &reply) {
        if (reply.error.isEmpty() && reply.status == 404)
        {
            nothing(done);
            return;
        }
        if (!ok(reply))
        {
            failed(done, QStringLiteral("YIFY"), reply);
            return;
        }
        static const QRegularExpression rowRe(QStringLiteral(R"re(<tr data-id="(\d+)">(.*?)</tr>)re"),
                                              QRegularExpression::DotMatchesEverythingOption);
        static const QRegularExpression ratingRe(QStringLiteral(R"re(<span class="label[^"]*">\s*(-?\d+)\s*</span>)re"));
        static const QRegularExpression languageRe(QStringLiteral(R"re(<span class="sub-lang">([^<]+)</span>)re"));
        static const QRegularExpression linkRe(QStringLiteral(
            R"re(<a href="/subtitles/([A-Za-z0-9._\-]+)">\s*(?:<span class="text-muted">[^<]*</span>)?\s*([^<]*)</a>)re"));
        const QString page = QString::fromUtf8(reply.body);
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
            r.language = languageFromName(language.captured(1));
            if (r.language.isEmpty())
                continue;
            r.release = link.captured(2).trimmed();
            r.url = QUrl(QStringLiteral("https://yifysubtitles.ch/subtitle/") + link.captured(1)
                         + QStringLiteral(".zip"));
            r.hearingImpaired = cells.contains(QLatin1String("hi-subtitle"));
            const QRegularExpressionMatch rating = ratingRe.match(cells);
            r.rank = rating.hasMatch() ? std::min(rating.captured(1).toDouble(), 30.) * 0.5 : 0.;
            results.append(r);
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
    httpRequestAsync(guard, request, [guard, query, done](const HttpResponse &reply) {
        if (!guard)
            return;
        if (!ok(reply))
        {
            failed(done, QStringLiteral("SubSource"), reply, true);
            return;
        }
        QString error;
        const QList<Result> results = parseSubSource(reply.body, query, languageOf, &error);
        if (!error.isEmpty())
            done({}, QStringLiteral("SubSource: ") + error, Problem::Failed);
        else
            done(results, {}, Problem::None);
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
    httpRequestAsync(guard, request, [guard, query, byText, done](const HttpResponse &reply) {
        if (!guard)
            return;
        if (!ok(reply))
        {
            failed(done, QStringLiteral("SubSource"), reply, true);
            return;
        }
        QString error;
        const QString movieId = subSourceMovieId(reply.body, query, &error);
        if (!error.isEmpty())
        {
            done({}, QStringLiteral("SubSource: ") + error, Problem::Failed);
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
    httpRequestAsync(context, request, [guard, query, done, v2](const HttpResponse &reply) {
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
        QString error;
        const QList<Result> results = parseSubDl(reply.body, query, languageOf, &error);
        if (!error.isEmpty())
            done({}, QStringLiteral("SubDL: ") + error, Problem::Failed);
        else
            done(results, {}, Problem::None);
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
    httpRequestAsync(context, openSubtitlesComRequest(url, query.credentials.key), [done](const HttpResponse &reply) {
        if (!ok(reply))
        {
            failed(done, QStringLiteral("OpenSubtitles.com"), reply, true);
            return;
        }
        done(parseOpenSubtitlesCom(reply.body, languageOf), {}, Problem::None);
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

// Signs in to OpenSubtitles.com: done(session) or done({}, problem)
void openSubtitlesComSignIn(QObject *context, const Credentials &credentials,
                            std::function<void(const Session &, Problem)> done)
{
    const auto known = sessions().constFind(QStringLiteral("opensubtitlescom"));
    if (known != sessions().constEnd() && known->username == credentials.username)
    {
        done(known->session, Problem::None);
        return;
    }
    HttpRequest login = openSubtitlesComRequest(QUrl(QStringLiteral("https://api.opensubtitles.com/api/v1/login")),
                                                credentials.key);
    login.method = "POST";
    login.body = openSubtitlesComLoginBody(credentials.username, credentials.password);
    const QString username = credentials.username;
    httpRequestAsync(context, login, [done, username](const HttpResponse &reply) {
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
        sessions().insert(QStringLiteral("opensubtitlescom"), { username, session });
        done(session, Problem::None);
    });
}

#ifdef HAVE_ZLIB
// inflates a gzip file (windowBits 16 + 15) or a raw deflate stream (-15);
// empty when it unpacks to more than limit (it stops there)
QByteArray inflateData(const QByteArray &data, int windowBits, qsizetype limit)
{
    z_stream stream = {};
    if (inflateInit2(&stream, windowBits) != Z_OK)
        return {};
    QByteArray out;
    out.reserve(std::min(limit, data.size() * 3));
    stream.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(data.constData()));
    stream.avail_in = uInt(data.size());
    char buffer[65536];
    int status = Z_OK;
    do
    {
        stream.next_out = reinterpret_cast<Bytef *>(buffer);
        stream.avail_out = sizeof(buffer);
        status = ::inflate(&stream, Z_NO_FLUSH);
        if (status != Z_OK && status != Z_STREAM_END)
            break;
        out.append(buffer, int(sizeof(buffer) - stream.avail_out));
        if (out.size() > limit)
            break;
    } while (status != Z_STREAM_END && (stream.avail_in > 0 || stream.avail_out == 0));
    inflateEnd(&stream);
    if (out.size() > limit)
        return {};
    return status == Z_STREAM_END || !out.isEmpty() ? out : QByteArray();
}
#endif

bool isSubtitleName(const QString &name)
{
    static const QSet<QString> extensions = { "srt", "ass", "ssa", "vtt", "sub", "smi", "txt" };
    return extensions.contains(QFileInfo(name).suffix().toLower());
}

bool validUtf8(const QByteArray &data)
{
    QStringDecoder utf8(QStringDecoder::Utf8);
    // decode() is lazy: the text must be made for hasError() to be set
    const QString text = utf8.decode(data);
    Q_UNUSED(text);
    return !utf8.hasError();
}

// The files of a zip archive: { name, data }, through its central directory.
// Only the subtitles are unpacked (the others have no data), each up to the
// size it declares, and at most maxUnpackedBytes in all.
QList<QPair<QString, QByteArray>> unzip(const QByteArray &zip)
{
    QList<QPair<QString, QByteArray>> files;
    qsizetype total = 0;
    int unpackedFiles = 0;
    const auto u16 = [&zip](qsizetype at) { return qFromLittleEndian<quint16>(zip.constData() + at); };
    const auto u32 = [&zip](qsizetype at) { return qFromLittleEndian<quint32>(zip.constData() + at); };
    // the end of the central directory, in the last 64 KiB
    qsizetype end = -1;
    for (qsizetype i = zip.size() - 22; i >= std::max<qsizetype>(0, zip.size() - 65557); --i)
        if (u32(i) == 0x06054b50)
        {
            end = i;
            break;
        }
    if (end < 0)
        return files;
    const quint16 count = u16(end + 10);
    qsizetype at = u32(end + 16);
    for (quint16 n = 0; n < count && n < maxArchiveEntries && at + 46 <= zip.size()
                        && u32(at) == 0x02014b50; ++n)
    {
        const quint16 method = u16(at + 10);
        const quint32 packed = u32(at + 20);
        const quint32 unpacked = u32(at + 24);
        const quint16 nameLength = u16(at + 28), extraLength = u16(at + 30), commentLength = u16(at + 32);
        const quint32 local = u32(at + 42);
        const QString name = QString::fromUtf8(zip.mid(at + 46, nameLength));
        at += 46 + nameLength + extraLength + commentLength;
        if (name.endsWith(QLatin1Char('/')) || name.startsWith(QLatin1String("__MACOSX")))
            continue;
        if (!isSubtitleName(name) || unpackedFiles >= maxArchiveSubtitles
            || unpacked > quint32(maxDownloadBytes) || qsizetype(unpacked) > maxUnpackedBytes - total
            || qsizetype(local) + 30 > zip.size() || u32(local) != 0x04034b50)
        {
            files.append({ name, QByteArray() });
            continue;
        }
        const qsizetype data = local + 30 + u16(local + 26) + u16(local + 28);
        if (data + qsizetype(packed) > zip.size())
            continue;
        const QByteArray raw = zip.mid(data, packed);
        QByteArray content;
        if (method == 0)
            content = raw.left(unpacked);
#ifdef HAVE_ZLIB
        else if (method == 8)
            content = inflateData(raw, -MAX_WBITS, qsizetype(unpacked));
#endif
        ++unpackedFiles;
        total += content.size();
        files.append({ name, content });
    }
    return files;
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
            // signed in again with these
            forgetSession(providerId);
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
            });
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
    download.url = result.url;
    download.headers.append(qMakePair(QByteArray("User-Agent"), userAgent()));
    if (result.provider == QLatin1String("opensubtitles"))
        download.headers.append(qMakePair(QByteArray("X-User-Agent"), userAgent()));
    // (YIFY's downloads are from its pages)
    if (result.provider == QLatin1String("yify"))
        download.headers.append(qMakePair(QByteArray("Referer"), QByteArray("https://yifysubtitles.ch/")));
    if (result.provider == QLatin1String("subsource"))
        download.headers.append(qMakePair(QByteArray("X-API-Key"), credentials.key.trimmed().toUtf8()));
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
#ifdef HAVE_ZLIB
        data = inflateData(data, 16 + MAX_WBITS, maxUnpackedBytes);
#else
        data.clear();
#endif
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
