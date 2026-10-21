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
#endif
#ifdef HAVE_ZLIB
# include <zlib.h>
#endif

#include <vlc_common.h>
#include <vlc_iso_lang.h>

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

using Done = std::function<void(const QList<Result> &, const QString &)>;

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

QString failure(const QString &name, const HttpResponse &reply)
{
    if (reply.status == 0)
        return qtr("%1 didn't answer.").arg(name);
    if (reply.status == 429)
        return qtr("%1 asks to wait a moment (too many searches).").arg(name);
    return qtr("%1 didn't answer (%2).").arg(name).arg(reply.status);
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
    int answered = 0;
};

void runOpenSubtitles(QPointer<QObject> context, std::shared_ptr<Chain> chain, Done done)
{
    if (!context)
        return;
    if (chain->urls.isEmpty())
    {
        done(chain->results, chain->answered == 0 ? chain->error : QString());
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
            chain->error = failure(QStringLiteral("OpenSubtitles"), reply);
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
        done({}, {});
        return;
    }
    runOpenSubtitles(QPointer<QObject>(context), chain, done);
}

// ---------------------------------------------------------------------------
// Wizdom: Hebrew subtitles, by the IMDb id (of the series, with the season
// and episode), scored against the name of the video file

void searchWizdom(QObject *context, const Query &query, Done done)
{
    if (!query.imdb.startsWith(QLatin1String("tt")))
    {
        done({}, {});
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
            done({}, {});
            return;
        }
        if (!ok(reply))
        {
            done({}, failure(QStringLiteral("Wizdom"), reply));
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
        done(results, {});
    });
}

// ---------------------------------------------------------------------------
// Addic7ed (through Gestdown's open interface): series only, by the show's
// name, then its episode in each of the user's languages

void searchAddic7ed(QObject *context, const Query &query, Done done)
{
    if (!query.series || query.season < 0 || query.episode < 0 || query.title.isEmpty())
    {
        done({}, {});
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
            done({}, {});
            return;
        }
        if (!ok(reply))
        {
            done({}, failure(QStringLiteral("Addic7ed"), reply));
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
            done({}, {});
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
                done(*results, {});
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
        done({}, {});
        return;
    }
    const QUrl url(QStringLiteral("https://yifysubtitles.ch/movie-imdb/") + query.imdb);
    httpRequestAsync(context, makeRequest(url), [done](const HttpResponse &reply) {
        if (reply.error.isEmpty() && reply.status == 404)
        {
            done({}, {});
            return;
        }
        if (!ok(reply))
        {
            done({}, failure(QStringLiteral("YIFY"), reply));
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
        done(results, {});
    });
}

// ---------------------------------------------------------------------------
// What was downloaded

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
    (void)utf8.decode(data);
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

QString decodePage(const QByteArray &data, unsigned page)
{
    if (page == 65001)
        return QString::fromUtf8(data);
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
    Q_UNUSED(page);
    return text;
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
    static const QList<Provider> list = {
        { QStringLiteral("opensubtitles"), QStringLiteral("OpenSubtitles"),
          qtr("All languages · movies and series · finds those made for your file"), true, true, {} },
        { QStringLiteral("wizdom"), QStringLiteral("Wizdom"),
          qtr("Hebrew · movies and series"), true, true, QStringLiteral("he") },
        { QStringLiteral("addic7ed"), QStringLiteral("Addic7ed"),
          qtr("Series · many languages"), false, true, {} },
        { QStringLiteral("yify"), QStringLiteral("YIFY Subtitles"),
          qtr("Movies · many languages"), true, false, {} },
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

void search(QObject *context, const QString &providerId, const Query &query, Done done)
{
    if (providerId == QLatin1String("opensubtitles"))
        searchOpenSubtitles(context, query, done);
    else if (providerId == QLatin1String("wizdom"))
        searchWizdom(context, query, done);
    else if (providerId == QLatin1String("addic7ed"))
        searchAddic7ed(context, query, done);
    else if (providerId == QLatin1String("yify"))
        searchYify(context, query, done);
    else
        done({}, {});
}

QList<QPair<QByteArray, QByteArray>> downloadHeaders(const Result &result)
{
    QList<QPair<QByteArray, QByteArray>> headers;
    headers.append(qMakePair(QByteArray("User-Agent"), userAgent()));
    if (result.provider == QLatin1String("opensubtitles"))
        headers.append(qMakePair(QByteArray("X-User-Agent"), userAgent()));
    // (YIFY's downloads are from its pages)
    if (result.provider == QLatin1String("yify"))
        headers.append(qMakePair(QByteArray("Referer"), QByteArray("https://yifysubtitles.ch/")));
    return headers;
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
