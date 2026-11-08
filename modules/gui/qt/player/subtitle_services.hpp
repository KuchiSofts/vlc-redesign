/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
#pragma once

#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>
#include <QUrl>

#include <functional>

#include "player/subtitle_providers.hpp"

// The services asked with the user's own free key or account: the addresses
// they are asked (built from the video) and what they answer, read here
// without the network nor VLC (Qt Core only), so that their tests run by
// themselves (extras/juxtopposed/subtitle-services.cpp, run by
// extras/juxtopposed/ci/unit-tests.sh). subtitle_providers.cpp sends the
// requests. The keys go in headers but for SubDL's first version (in the
// address): nothing here is logged.
namespace SubtitleProviders
{

// A language a service names ("Arabic", "Portuguese (Brazil)") in the app's
// form ("ar", "pt-BR"), empty when unknown (the app: SubtitlePreferences)
using LanguageOf = std::function<QString(const QString &name)>;

// What a status means: 0 no answer, 401 and 403 the key refused, 429 too
// many searches, another one not answered (200: None)
Problem problemOf(int status);
// The same for a check (Test): 200 works
Check checkOf(int status);

// The release cleaned of the uploaders' decorations ("* x *", "- x -")
QString cleanRelease(const QString &release);
// Whether a release is of this episode: it names it ("S01E02", "1x02",
// "S01E01-E03"), or names no episode (the whole season); false when it names
// another one
bool releaseOfEpisode(const QString &release, int season, int episode);

// ---------------------------------------------------------------------------
// SubSource (api.subsource.net/api/v1; the key in X-API-Key)

// Its name of a language ("pt-BR" → "brazilian_portuguese"): one or two
QStringList subSourceNames(const QString &code);
// The language of its name ("farsi_persian" → "fa"), empty when unknown
QString languageOfSubSource(const QString &name);
// The title: by the IMDb id (of the series for an episode), else its name
QUrl subSourceSearchUrl(const Query &query, bool byText);
// The title of the answer: the film of the IMDb id, the season of the
// episode; empty when none (error: what it said, when it failed)
QString subSourceMovieId(const QByteArray &body, const Query &query, QString *error);
// Its subtitles in the user's languages
QUrl subSourceSubtitlesUrl(const QString &movieId, const Query &query);
QList<Result> parseSubSource(const QByteArray &body, const Query &query, const LanguageOf &languageOf,
                             QString *error);
QUrl subSourceCheckUrl();

// ---------------------------------------------------------------------------
// SubDL (api.subdl.com: v2 with the key in Authorization, v1 with it in the
// address when v2 isn't there)

QString subDlCode(const QString &code); // "pt-BR" → "BR_PT"
QString languageOfSubDl(const QString &codeOrName, const LanguageOf &languageOf);
QUrl subDlSearchUrl(const Query &query, bool v2);
// error: empty when it found nothing ("can't find")
QList<Result> parseSubDl(const QByteArray &body, const Query &query, const LanguageOf &languageOf,
                         QString *error);
QUrl subDlCheckUrl(const QString &key, bool v2);

// ---------------------------------------------------------------------------
// OpenSubtitles.com (api.opensubtitles.com/api/v1; the key in Api-Key, a
// download's link asked with the file's id, signed in for a higher quota)

QString openSubtitlesComCode(const QString &code); // "pt-BR" → "pt-br", "zh" → "zh-cn"
QString languageOfOpenSubtitlesCom(const QString &code, const LanguageOf &languageOf);
QUrl openSubtitlesComSearchUrl(const Query &query);
QList<Result> parseOpenSubtitlesCom(const QByteArray &body, const LanguageOf &languageOf);
// The sign-in's token and the address to ask then (vip-api for its members);
// empty when refused
struct Session
{
    QString token;
    QUrl base;
};
Session openSubtitlesComSession(const QByteArray &loginReply);
QByteArray openSubtitlesComLoginBody(const QString &username, const QString &password);
QByteArray openSubtitlesComDownloadBody(const QString &fileId);
// The link of a download's answer (https only), empty when none
QUrl openSubtitlesComLink(const QByteArray &downloadReply);
QUrl openSubtitlesComCheckUrl();

} // namespace SubtitleProviders
