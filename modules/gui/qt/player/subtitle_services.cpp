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

#include "subtitle_services.hpp"

#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <QUrlQuery>

#include <algorithm>
#include <cmath>

namespace SubtitleProviders
{

namespace {

QString text(const QJsonObject &o, const char *key)
{
    return o.value(QLatin1String(key)).toVariant().toString().trimmed();
}

// The query of an address, its items in this order (some services want
// them sorted), each value encoded once
QUrl withQuery(const QString &address, const QList<QPair<QString, QString>> &items)
{
    QUrl url(address);
    QUrlQuery query;
    for (const auto &item : items)
        query.addQueryItem(item.first, QString::fromUtf8(QUrl::toPercentEncoding(item.second)));
    url.setQuery(query.query(QUrl::FullyEncoded), QUrl::StrictMode);
    return url;
}

// The answer's message ("Invalid API key"), else its error
QString messageOf(const QJsonObject &o)
{
    const QJsonValue error = o.value(QLatin1String("error"));
    if (error.isObject())
    {
        const QString message = text(error.toObject(), "message");
        if (!message.isEmpty())
            return message;
    }
    const QString message = text(o, "message");
    if (!message.isEmpty())
        return message;
    return error.isString() ? error.toString() : QString();
}

} // namespace

Problem problemOf(int status)
{
    if (status == 200)
        return Problem::None;
    if (status == 0)
        return Problem::NoAnswer;
    if (status == 401 || status == 403)
        return Problem::KeyRefused;
    if (status == 429)
        return Problem::Busy;
    return Problem::Failed;
}

Check checkOf(int status)
{
    switch (problemOf(status))
    {
        case Problem::None: return Check::Works;
        case Problem::KeyRefused: return Check::KeyRefused;
        case Problem::LoginRefused: return Check::LoginRefused;
        case Problem::Busy: return Check::Busy;
        default: return Check::NoAnswer;
    }
}

QString cleanRelease(const QString &release)
{
    static const QRegularExpression decorations(QStringLiteral(R"(^[\s*\-:.|]+|[\s*\-:.|]+$)"));
    QString cleaned = release;
    cleaned.remove(decorations);
    return cleaned.trimmed();
}

bool releaseOfEpisode(const QString &release, int season, int episode)
{
    if (season < 0 || episode < 0)
        return true;
    using RE = QRegularExpression;
    // S01E02, S01E02E03, S01E02-E03, S01E02-03; 1x02
    static const RE sxe(QStringLiteral(R"((?:^|[^a-z0-9])s(\d{1,2})[\s._\-]*e(\d{1,3})(?:[\s._\-]*(?:e|-e?)(\d{1,3}))?(?=[^0-9]|$))"),
                        RE::CaseInsensitiveOption);
    static const RE nxn(QStringLiteral(R"((?:^|[^0-9a-z])(\d{1,2})x(\d{1,3})(?=[^0-9]|$))"), RE::CaseInsensitiveOption);
    bool named = false;
    for (const RE *re : { &sxe, &nxn })
        for (QRegularExpressionMatchIterator it = re->globalMatch(release); it.hasNext();)
        {
            const QRegularExpressionMatch m = it.next();
            named = true;
            const int s = m.captured(1).toInt();
            const int first = m.captured(2).toInt();
            int last = re == &sxe && !m.captured(3).isEmpty() ? m.captured(3).toInt() : first;
            // (a range of a season, not "S02E03-720p")
            if (last < first || last - first > 30)
                last = first;
            if (s == season && episode >= first && episode <= std::max(first, last))
                return true;
        }
    return !named;
}

// ---------------------------------------------------------------------------
// SubSource

namespace {

// The app's languages and SubSource's names of them (a4kSubtitles'
// services/subsource.py), the name it is asked by first
struct SubSourceLanguage
{
    const char *code;
    const char *names;
};
const SubSourceLanguage subSourceLanguages[] = {
    { "ar", "arabic" }, { "en", "english" }, { "bn", "bengali" }, { "id", "indonesian" },
    { "ab", "abkhazian" }, { "af", "afrikaans" }, { "sq", "albanian" }, { "am", "amharic" },
    { "an", "aragonese" }, { "hy", "armenian" }, { "as", "assamese" }, { "fa", "farsi_persian" },
    { "ast", "asturian" }, { "az", "azerbaijani" }, { "eu", "basque" }, { "be", "belarusian" },
    { "zh", "chinese_simplified|chinese|chinese_bg_code|chinese_bilingual" },
    { "zh-TW", "chinese_traditional|big_5_code" }, { "yue", "chinese_cantonese" },
    { "bs", "bosnian" }, { "pt-BR", "brazilian_portuguese" }, { "pt", "portuguese" },
    { "br", "breton" }, { "bg", "bulgarian" }, { "my", "burmese" }, { "ca", "catalan" },
    { "hr", "croatian" }, { "cs", "czech" }, { "da", "danish" }, { "prs", "dari" }, { "nl", "dutch" },
    { "eo", "espranto" }, { "et", "estonian" }, { "ext", "extremaduran" }, { "fil", "filipino" },
    { "fi", "finnish" }, { "fr", "french|french_france|french_canada" }, { "gd", "gaelic" },
    { "gl", "gaelician" }, { "ka", "georgian" }, { "de", "german" }, { "el", "greek" },
    { "kl", "greenlandic" }, { "he", "hebrew" }, { "hi", "hindi" }, { "hu", "hungarian" },
    { "is", "icelandic" }, { "ig", "igbo" }, { "ia", "interlingua" }, { "ga", "irish" },
    { "it", "italian" }, { "ja", "japanese" }, { "kn", "kannada" }, { "kk", "kazakh" }, { "km", "khmer" },
    { "ko", "korean" }, { "ku", "kurdish" }, { "ky", "kyrgyz" }, { "lv", "latvian" }, { "lt", "lithuanian" },
    { "lb", "luxembourgish" }, { "mk", "macedonian" }, { "ml", "malayalam" }, { "mni", "manipuri" },
    { "mr", "marathi" }, { "ms", "malay" }, { "mn", "mongolian" }, { "cnr", "montenegrin" },
    { "nv", "navajo" }, { "ne", "nepali" }, { "se", "northen_sami" }, { "no", "norwegian" },
    { "oc", "occitan" }, { "ps", "pashto|pushto" }, { "or", "odia" }, { "pl", "polish" },
    { "ro", "romanian" }, { "ru", "russian" }, { "sat", "santli" }, { "sr", "serbian" }, { "sd", "sindhi" },
    { "si", "sinhala|sinhalese" }, { "sk", "slovak" }, { "sl", "slovenian" }, { "so", "somali" },
    { "wen", "sorbian" }, { "es", "spanish|spanish_spain" }, { "es-419", "spanish_latin_america" },
    { "sw", "swahili" }, { "sv", "swedish" }, { "syl", "sylheti" }, { "syr", "syriac" },
    { "tl", "tagalog" }, { "ta", "tamil" }, { "tt", "tatar" }, { "te", "telugu" }, { "tet", "tetum" },
    { "th", "thai" }, { "tok", "toki_pona" }, { "tr", "turkish" }, { "tk", "turkmen" },
    { "uk", "ukrainian" }, { "ur", "urdu" }, { "uz", "uzbek" }, { "vi", "vietnamese" }, { "cy", "welsh" },
};

const QString subSourceApi = QStringLiteral("https://api.subsource.net/api/v1");

} // namespace

QStringList subSourceNames(const QString &code)
{
    for (const SubSourceLanguage &l : subSourceLanguages)
        if (code == QLatin1String(l.code))
            return QString::fromLatin1(l.names).split(QLatin1Char('|'));
    return {};
}

QString languageOfSubSource(const QString &name)
{
    static const QHash<QString, QString> codes = []() {
        QHash<QString, QString> map;
        for (const SubSourceLanguage &l : subSourceLanguages)
            for (const QString &n : QString::fromLatin1(l.names).split(QLatin1Char('|')))
                map.insert(n, QString::fromLatin1(l.code));
        return map;
    }();
    QString key = name.trimmed().toLower();
    key.replace(QLatin1Char(' '), QLatin1Char('_'));
    return codes.value(key);
}

QUrl subSourceSearchUrl(const Query &query, bool byText)
{
    if (!byText && query.imdb.startsWith(QLatin1String("tt")))
        return withQuery(subSourceApi + QStringLiteral("/movies/search"),
                         { { QStringLiteral("searchType"), QStringLiteral("imdb") },
                           { QStringLiteral("imdb"), query.imdb } });
    if (query.title.isEmpty())
        return {};
    QList<QPair<QString, QString>> items = { { QStringLiteral("searchType"), QStringLiteral("text") },
                                             { QStringLiteral("q"), query.title } };
    if (query.year > 0 && !query.series)
        items.append({ QStringLiteral("year"), QString::number(query.year) });
    items.append({ QStringLiteral("type"), query.series ? QStringLiteral("tvseries") : QStringLiteral("movie") });
    return withQuery(subSourceApi + QStringLiteral("/movies/search"), items);
}

QString subSourceMovieId(const QByteArray &body, const Query &query, QString *error)
{
    const QJsonObject answer = QJsonDocument::fromJson(body).object();
    if (answer.contains(QLatin1String("error")) || !answer.value(QLatin1String("success")).toBool(true))
    {
        *error = messageOf(answer);
        return {};
    }
    QString first, firstOfSeason;
    for (const QJsonValue item : answer.value(QLatin1String("data")).toArray())
    {
        const QJsonObject o = item.toObject();
        const QString id = text(o, "movieId");
        if (id.isEmpty() || id == QLatin1String("0"))
            continue;
        if (query.series)
        {
            // a title a season ("season": 1)
            const int season = o.value(QLatin1String("season")).toVariant().toInt();
            if (query.season >= 0 ? season == query.season : season <= 1)
                return id;
            if (season == 1 && firstOfSeason.isEmpty())
                firstOfSeason = id;
            continue;
        }
        const QString imdb = text(o, "imdbId");
        if (!query.imdb.isEmpty() && imdb == query.imdb)
            return id;
        if (first.isEmpty() && (query.imdb.isEmpty() || imdb.isEmpty()))
            first = id;
    }
    // (an episode of an unknown season: its first)
    return query.series ? (query.season < 0 ? firstOfSeason : QString()) : first;
}

QUrl subSourceSubtitlesUrl(const QString &movieId, const Query &query)
{
    QStringList names;
    for (const QString &code : query.languages)
        for (const QString &name : subSourceNames(code))
            if (!names.contains(name))
                names.append(name);
    QList<QPair<QString, QString>> items = { { QStringLiteral("movieId"), movieId } };
    if (!names.isEmpty())
        items.append({ QStringLiteral("language"), names.join(QLatin1Char(',')) });
    return withQuery(subSourceApi + QStringLiteral("/subtitles"), items);
}

QList<Result> parseSubSource(const QByteArray &body, const Query &query, const LanguageOf &languageOf,
                             QString *error)
{
    QList<Result> results;
    const QJsonObject answer = QJsonDocument::fromJson(body).object();
    if (answer.contains(QLatin1String("error")) || !answer.value(QLatin1String("success")).toBool(false))
    {
        *error = messageOf(answer);
        if (error->isEmpty())
            *error = QStringLiteral("?");
        return results;
    }
    const bool episode = query.series && query.season >= 0 && query.episode >= 0;
    for (const QJsonValue item : answer.value(QLatin1String("data")).toArray())
    {
        const QJsonObject o = item.toObject();
        const QString id = text(o, "subtitleId");
        static const QRegularExpression digits(QStringLiteral("^[0-9]+$"));
        if (!digits.match(id).hasMatch())
            continue;
        const QString name = text(o, "language");
        QString language = languageOfSubSource(name);
        if (language.isEmpty() && languageOf)
            language = languageOf(QString(name).replace(QLatin1Char('_'), QLatin1Char(' ')));
        if (language.isEmpty())
            continue;
        // the releases it was made for: the first naming this episode
        QStringList releases;
        const QJsonValue info = o.value(QLatin1String("releaseInfo"));
        if (info.isArray())
            for (const QJsonValue r : info.toArray())
                releases.append(cleanRelease(r.toString()));
        else if (info.isString())
            releases.append(cleanRelease(info.toString()));
        releases.removeAll(QString());
        QString release = releases.value(0);
        if (episode && !releases.isEmpty())
        {
            const auto it = std::find_if(releases.cbegin(), releases.cend(), [&query](const QString &r) {
                return releaseOfEpisode(r, query.season, query.episode);
            });
            if (it == releases.cend())
                continue; // of other episodes
            release = *it;
        }
        Result r;
        r.id = QStringLiteral("subsource:") + id;
        r.provider = QStringLiteral("subsource");
        r.language = language;
        r.release = release;
        r.url = QUrl(subSourceApi + QStringLiteral("/subtitles/") + id + QStringLiteral("/download"));
        r.hearingImpaired = o.value(QLatin1String("hearingImpaired")).toBool();
        r.foreignOnly = o.value(QLatin1String("foreignParts")).toBool();
        r.downloads = o.value(QLatin1String("downloads")).toVariant().toInt();
        const QJsonObject rating = o.value(QLatin1String("rating")).toObject();
        const int good = rating.value(QLatin1String("good")).toVariant().toInt();
        const int bad = rating.value(QLatin1String("bad")).toVariant().toInt();
        const int total = rating.value(QLatin1String("total")).toVariant().toInt();
        r.rank = std::log10(double(std::max(rating.contains(QLatin1String("good")) ? good : total, 0)) + 1.) * 3.
               + std::log10(double(std::max(r.downloads, 0)) + 1.)
               - (bad > good && bad >= 3 ? 10. : 0.);
        results.append(r);
    }
    return results;
}

QUrl subSourceCheckUrl()
{
    Query query;
    query.imdb = QStringLiteral("tt0133093");
    return subSourceSearchUrl(query, false);
}

// ---------------------------------------------------------------------------
// SubDL

namespace {

// Its codes (Bazarr's converters/subdl.py)
const QSet<QString> &subDlCodes()
{
    static const QSet<QString> codes = {
        "AR", "DA", "NL", "EN", "FA", "FI", "FR", "ID", "IT", "NO", "RO", "ES", "SV", "VI", "SQ", "AZ",
        "BE", "BN", "BS", "BG", "MY", "CA", "ZH", "HR", "CS", "EO", "ET", "KA", "DE", "EL", "KL", "HE",
        "HI", "HU", "IS", "JA", "KO", "KU", "LV", "LT", "MK", "MS", "ML", "PL", "PT", "RU", "SR", "SI",
        "SK", "SL", "TL", "TA", "TE", "TH", "TR", "UK", "UR", "HY", "KK", "KY", "KM", "KN", "MN", "EU",
        "GL", "GA", "JV", "SU", "BR_PT", "ZH_BG",
    };
    return codes;
}

} // namespace

QString subDlCode(const QString &code)
{
    if (code == QLatin1String("pt-BR"))
        return QStringLiteral("BR_PT");
    if (code == QLatin1String("zh-TW"))
        return QStringLiteral("ZH_BG");
    const QString base = code.section(QLatin1Char('-'), 0, 0).toUpper();
    return subDlCodes().contains(base) ? base : QString();
}

QString languageOfSubDl(const QString &codeOrName, const LanguageOf &languageOf)
{
    const QString upper = codeOrName.trimmed().toUpper();
    if (upper == QLatin1String("BR_PT"))
        return QStringLiteral("pt-BR");
    if (upper == QLatin1String("ZH_BG"))
        return QStringLiteral("zh-TW");
    if (subDlCodes().contains(upper))
        return upper.toLower();
    if (upper.contains(QLatin1Char('_')))
        return {}; // (two languages: "BG_EN")
    return languageOf ? languageOf(codeOrName.trimmed()) : QString();
}

QUrl subDlSearchUrl(const Query &query, bool v2)
{
    QStringList codes;
    for (const QString &language : query.languages)
    {
        const QString code = subDlCode(language);
        if (!code.isEmpty() && !codes.contains(code))
            codes.append(code);
    }
    const bool episode = query.series && query.season >= 0 && query.episode >= 0;
    QList<QPair<QString, QString>> items;
    if (v2)
    {
        items.append({ QStringLiteral("type"), query.series ? QStringLiteral("tv") : QStringLiteral("movie") });
        if (!query.series && query.imdb.startsWith(QLatin1String("tt")))
        {
            items.append({ QStringLiteral("imdb_id"), query.imdb });
            if (query.year > 0)
                items.append({ QStringLiteral("year"), QString::number(query.year) });
        }
        else if (!query.title.isEmpty())
        {
            items.append({ QStringLiteral("film_name"), query.title });
            if (!query.series && query.year > 0)
                items.append({ QStringLiteral("year"), QString::number(query.year) });
        }
        else
            return {};
        if (episode)
        {
            items.append({ QStringLiteral("season"), QString::number(query.season) });
            items.append({ QStringLiteral("episode"), QString::number(query.episode) });
        }
        if (query.series && !query.fileName.isEmpty())
            items.append({ QStringLiteral("file_name"), QFileInfo(query.fileName).completeBaseName() });
        if (!codes.isEmpty())
            items.append({ QStringLiteral("languages"), codes.join(QLatin1Char(',')).toLower() });
        return withQuery(QStringLiteral("https://api.subdl.com/api/v2/subtitles/search"), items);
    }
    items.append({ QStringLiteral("api_key"), query.credentials.key });
    if (query.imdb.startsWith(QLatin1String("tt")))
        items.append({ QStringLiteral("imdb_id"), query.imdb });
    else if (!query.title.isEmpty())
        items.append({ QStringLiteral("film_name"), query.title });
    else
        return {};
    items.append({ QStringLiteral("type"), query.series ? QStringLiteral("tv") : QStringLiteral("movie") });
    if (episode)
    {
        items.append({ QStringLiteral("season_number"), QString::number(query.season) });
        items.append({ QStringLiteral("episode_number"), QString::number(query.episode) });
    }
    if (!codes.isEmpty())
        items.append({ QStringLiteral("languages"), codes.join(QLatin1Char(',')) });
    items.append({ QStringLiteral("subs_per_page"), QStringLiteral("30") });
    return withQuery(QStringLiteral("https://api.subdl.com/api/v1/subtitles"), items);
}

QList<Result> parseSubDl(const QByteArray &body, const Query &query, const LanguageOf &languageOf,
                         QString *error)
{
    QList<Result> results;
    const QJsonObject answer = QJsonDocument::fromJson(body).object();
    if (answer.isEmpty() || answer.contains(QLatin1String("error"))
        || (answer.contains(QLatin1String("status")) && !answer.value(QLatin1String("status")).toBool()))
    {
        const QString message = messageOf(answer);
        // (nothing of that title: no error)
        if (!message.contains(QLatin1String("can't find"), Qt::CaseInsensitive)
            && !message.contains(QLatin1String("not found"), Qt::CaseInsensitive))
            *error = message.isEmpty() ? QStringLiteral("?") : message;
        return results;
    }
    const bool episode = query.series && query.season >= 0 && query.episode >= 0;
    QSet<QString> seen;
    for (const QJsonValue item : answer.value(QLatin1String("subtitles")).toArray())
    {
        const QJsonObject o = item.toObject();
        const QString path = text(o, "url");
        // "/subtitle/3197651-3213944.zip": on dl.subdl.com, nothing else
        static const QRegularExpression pathRe(QStringLiteral(R"(^/subtitle/[A-Za-z0-9._\-]+$)"));
        if (!pathRe.match(path).hasMatch() || seen.contains(path))
            continue;
        QString language = languageOfSubDl(text(o, "language"), languageOf);
        if (language.isEmpty())
            language = languageOfSubDl(text(o, "lang"), languageOf);
        if (language.isEmpty())
            continue;
        const QString release = cleanRelease(text(o, "release_name").isEmpty() ? text(o, "name")
                                                                                : text(o, "release_name"));
        if (episode)
        {
            const QJsonValue seasonValue = o.value(QLatin1String("season"));
            const int season = seasonValue.toVariant().toInt();
            if (!seasonValue.isNull() && !seasonValue.isUndefined() && season > 0 && season != query.season)
                continue;
            const bool fullSeason = o.value(QLatin1String("full_season")).toBool();
            const QJsonValue from = o.value(QLatin1String("episode_from"));
            const QJsonValue to = o.value(QLatin1String("episode_end"));
            const int number = o.value(QLatin1String("episode")).toVariant().toInt();
            if (from.isDouble() && to.isDouble() && to.toInt() >= from.toInt() && from.toInt() > 0)
            {
                if (query.episode < from.toInt() || query.episode > to.toInt())
                    continue;
            }
            else if (!fullSeason && number > 0)
            {
                if (number != query.episode)
                    continue;
            }
            else if (!fullSeason && !releaseOfEpisode(release, query.season, query.episode))
                continue;
        }
        seen.insert(path);
        Result r;
        r.id = QStringLiteral("subdl:") + QFileInfo(path).completeBaseName();
        r.provider = QStringLiteral("subdl");
        r.language = language;
        r.release = release;
        r.url = QUrl(QStringLiteral("https://dl.subdl.com") + path);
        r.hearingImpaired = o.value(QLatin1String("hi")).toBool();
        r.downloads = o.value(QLatin1String("downloads")).toVariant().toInt();
        r.rank = std::log10(double(std::max(r.downloads, 0)) + 1.) * 3.
               + std::min(o.value(QLatin1String("rating")).toVariant().toDouble(), 10.) * 0.5;
        results.append(r);
    }
    return results;
}

QUrl subDlCheckUrl(const QString &key, bool v2)
{
    Query query;
    query.imdb = QStringLiteral("tt0133093");
    query.languages = { QStringLiteral("en") };
    query.credentials.key = key;
    return subDlSearchUrl(query, v2);
}

// ---------------------------------------------------------------------------
// OpenSubtitles.com

namespace {
const QString openSubtitlesComApi = QStringLiteral("https://api.opensubtitles.com/api/v1/");
}

QString openSubtitlesComCode(const QString &code)
{
    if (code == QLatin1String("pt-BR"))
        return QStringLiteral("pt-br");
    if (code == QLatin1String("zh-TW"))
        return QStringLiteral("zh-tw");
    if (code == QLatin1String("zh"))
        return QStringLiteral("zh-cn");
    if (code == QLatin1String("pt"))
        return QStringLiteral("pt-pt");
    if (code == QLatin1String("es-419"))
        return QStringLiteral("ea");
    return code.section(QLatin1Char('-'), 0, 0).toLower();
}

QString languageOfOpenSubtitlesCom(const QString &code, const LanguageOf &languageOf)
{
    const QString c = code.trimmed().toLower();
    if (c == QLatin1String("pt-br") || c == QLatin1String("pb"))
        return QStringLiteral("pt-BR");
    if (c == QLatin1String("zh-tw"))
        return QStringLiteral("zh-TW");
    if (c == QLatin1String("zh-cn"))
        return QStringLiteral("zh");
    if (c == QLatin1String("pt-pt"))
        return QStringLiteral("pt");
    if (c == QLatin1String("ea"))
        return QStringLiteral("es-419");
    if (c.isEmpty())
        return {};
    return languageOf ? languageOf(c) : c;
}

QUrl openSubtitlesComSearchUrl(const Query &query)
{
    QList<QPair<QString, QString>> items;
    if (query.imdb.startsWith(QLatin1String("tt")))
        items.append({ query.series ? QStringLiteral("parent_imdb_id") : QStringLiteral("imdb_id"),
                       QString::number(query.imdb.mid(2).toLongLong()) });
    else if (!query.title.isEmpty())
        items.append({ QStringLiteral("query"), query.title.toLower() });
    if (!query.hash.isEmpty())
        items.append({ QStringLiteral("moviehash"), query.hash });
    if (items.isEmpty())
        return {};
    if (query.series && query.season >= 0 && query.episode >= 0)
    {
        items.append({ QStringLiteral("season_number"), QString::number(query.season) });
        items.append({ QStringLiteral("episode_number"), QString::number(query.episode) });
    }
    QStringList codes;
    for (const QString &language : query.languages)
    {
        const QString code = openSubtitlesComCode(language);
        if (!code.isEmpty() && !codes.contains(code))
            codes.append(code);
    }
    std::sort(codes.begin(), codes.end());
    if (!codes.isEmpty())
        items.append({ QStringLiteral("languages"), codes.join(QLatin1Char(',')) });
    // (it asks for the parameters in alphabetical order)
    std::sort(items.begin(), items.end(), [](const auto &a, const auto &b) { return a.first < b.first; });
    return withQuery(openSubtitlesComApi + QStringLiteral("subtitles"), items);
}

QList<Result> parseOpenSubtitlesCom(const QByteArray &body, const LanguageOf &languageOf)
{
    QList<Result> results;
    for (const QJsonValue item : QJsonDocument::fromJson(body).object().value(QLatin1String("data")).toArray())
    {
        const QJsonObject a = item.toObject().value(QLatin1String("attributes")).toObject();
        // made by a machine: not offered
        if (a.value(QLatin1String("ai_translated")).toBool() || a.value(QLatin1String("machine_translated")).toBool())
            continue;
        const QJsonObject file = a.value(QLatin1String("files")).toArray().at(0).toObject();
        const qint64 fileId = file.value(QLatin1String("file_id")).toVariant().toLongLong();
        if (fileId <= 0)
            continue;
        const QString language = languageOfOpenSubtitlesCom(text(a, "language"), languageOf);
        if (language.isEmpty())
            continue;
        Result r;
        r.id = QStringLiteral("opensubtitlescom:%1").arg(fileId);
        r.provider = QStringLiteral("opensubtitlescom");
        r.language = language;
        r.fileName = text(file, "file_name");
        r.release = cleanRelease(text(a, "release"));
        if (r.release.isEmpty())
            r.release = QFileInfo(r.fileName).completeBaseName();
        r.url = QUrl(openSubtitlesComApi + QStringLiteral("download"));
        r.exact = a.value(QLatin1String("moviehash_match")).toBool();
        r.hearingImpaired = a.value(QLatin1String("hearing_impaired")).toBool();
        r.foreignOnly = a.value(QLatin1String("foreign_parts_only")).toBool();
        r.downloads = a.value(QLatin1String("download_count")).toVariant().toInt();
        r.fps = a.value(QLatin1String("fps")).toDouble();
        r.rank = std::log10(double(std::max(r.downloads, 0)) + 1.) * 3.
               + (a.value(QLatin1String("from_trusted")).toBool() ? 5. : 0.);
        results.append(r);
    }
    return results;
}

Session openSubtitlesComSession(const QByteArray &loginReply)
{
    const QJsonObject o = QJsonDocument::fromJson(loginReply).object();
    Session session;
    session.token = text(o, "token");
    // a host or an address; only theirs
    QString host = text(o, "base_url");
    host.remove(QRegularExpression(QStringLiteral("^https://")));
    host = host.section(QLatin1Char('/'), 0, 0);
    if (host.isEmpty())
        host = QStringLiteral("api.opensubtitles.com");
    if (host != QLatin1String("api.opensubtitles.com") && host != QLatin1String("vip-api.opensubtitles.com"))
        return {};
    if (session.token.isEmpty())
        return {};
    session.base = QUrl(QStringLiteral("https://%1/api/v1/").arg(host));
    return session;
}

QByteArray openSubtitlesComLoginBody(const QString &username, const QString &password)
{
    return QJsonDocument(QJsonObject{ { QStringLiteral("username"), username },
                                      { QStringLiteral("password"), password } })
        .toJson(QJsonDocument::Compact);
}

QByteArray openSubtitlesComDownloadBody(const QString &fileId)
{
    return QJsonDocument(QJsonObject{ { QStringLiteral("file_id"), fileId.toLongLong() } })
        .toJson(QJsonDocument::Compact);
}

QUrl openSubtitlesComLink(const QByteArray &downloadReply)
{
    const QUrl link(text(QJsonDocument::fromJson(downloadReply).object(), "link"));
    if (link.scheme() != QLatin1String("https") || link.host().isEmpty())
        return {};
    return link;
}

QUrl openSubtitlesComCheckUrl()
{
    Query query;
    query.imdb = QStringLiteral("tt0133093");
    query.languages = { QStringLiteral("en") };
    return openSubtitlesComSearchUrl(query);
}

} // namespace SubtitleProviders
