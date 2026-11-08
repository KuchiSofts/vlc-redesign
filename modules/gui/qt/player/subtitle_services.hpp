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
    QString userId; // (Titlovi)
};
Session openSubtitlesComSession(const QByteArray &loginReply);
QByteArray openSubtitlesComLoginBody(const QString &username, const QString &password);
QByteArray openSubtitlesComDownloadBody(const QString &fileId);
// The link of a download's answer (https only), empty when none
QUrl openSubtitlesComLink(const QByteArray &downloadReply);
QUrl openSubtitlesComCheckUrl();

// ---------------------------------------------------------------------------
// The services with an account or a key of wave 2, each as Bazarr's provider
// of it asks it (custom_libs/subliminal_patch/providers/<name>.py)

// Titlovi (kodi.titlovi.com/api/subtitles): the Balkan languages, signed in
// (a token and the user's id, given at sign-in)
QString titloviLanguage(const QString &code);        // "hr" → "Hrvatski"
QString languageOfTitlovi(const QString &name);      // "Cirilica" → "sr"
QUrl titloviLoginUrl(const QString &username, const QString &password);
Session titloviSession(const QByteArray &loginReply);
QUrl titloviSearchUrl(const Query &query, const Session &session);
QList<Result> parseTitlovi(const QByteArray &body, const Query &query);

// Assrt (api.assrt.net/v1): Chinese, with the user's token
QUrl assrtSearchUrl(const Query &query);
// error: its errmsg; refused: the token is ("invalid token", 20001)
QList<Result> parseAssrt(const QByteArray &body, const Query &query, QString *error, bool *refused);
QUrl assrtDetailUrl(const QString &key, const QString &id);
// the file of a detail's answer for this language (else its archive)
QUrl assrtFileOf(const QByteArray &detailReply, const QString &language);
QUrl assrtCheckUrl(const QString &key);

// Jimaku (jimaku.cc/api): anime, Japanese, the key in Authorization
QUrl jimakuSearchUrl(const Query &query);
QString jimakuEntryId(const QByteArray &body, const Query &query);
QUrl jimakuFilesUrl(const QString &entryId, const Query &query);
QList<Result> parseJimaku(const QByteArray &body, const Query &query);

// BetaSeries (api.betaseries.com): series, French; the show by its IMDb id,
// then its episode's subtitles
QUrl betaSeriesShowUrl(const Query &query);
QString betaSeriesShowId(const QByteArray &body);
QUrl betaSeriesEpisodeUrl(const QString &showId, const Query &query);
// refused: its error 1001 (the key)
QList<Result> parseBetaSeries(const QByteArray &body, QString *error, bool *refused);

// Subs.ro (api.subs.ro/v1.0): Romanian, the key in X-Subs-Api-Key
QUrl subsRoSearchUrl(const Query &query);
QList<Result> parseSubsRo(const QByteArray &body, const Query &query, QString *error);

// Napisy24 (napisy24.pl): Polish, those made for this very file (its hash),
// signed in; the subtitle comes with the answer (a zip after "||")
QByteArray napisy24Body(const Query &query, const QString &username, const QString &password);
struct Napisy24Answer
{
    bool loginRefused = false;
    bool found = false;
    QString imdb;
    QString title;
    QByteArray archive;
};
Napisy24Answer parseNapisy24(const QByteArray &body);

// Legendas.net (legendas.net/api/v1): Portuguese (Brazil), signed in with an
// email (a token given at sign-in); its searches are GET with a JSON body
QByteArray legendasLoginBody(const QString &email, const QString &password);
Session legendasSession(const QByteArray &loginReply);
QUrl legendasSearchUrl(const Query &query);
QByteArray legendasSearchBody(const Query &query);
QList<Result> parseLegendas(const QByteArray &body, const Query &query);

} // namespace SubtitleProviders
