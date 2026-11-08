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

class QObject;

// The services subtitles are found on: the free ones without an account,
// and those asked with the user's own free key or account (SubSource, SubDL,
// OpenSubtitles.com...: their requests and answers in subtitle_services.cpp,
// tested by themselves); what each is asked, and what it answers, for
// SubtitleFinder. Each search is a few requests, one after the other (the
// services ask for no more), at most a few seconds; a service that does not
// answer is left out of the results.
namespace SubtitleProviders
{

// What went wrong asking a service
enum class Problem
{
    None,
    NoAnswer,     // no answer at all
    Busy,         // too many searches (429): later
    KeyRefused,   // the user's key isn't taken (401, 403)
    LoginRefused, // the user name or password isn't
    Failed        // another answer
};

// The credentials a service takes (Provider::fields)
enum Field
{
    KeyField = 1,      // an API key or token
    UsernameField = 2,
    PasswordField = 4,
    EmailField = 8
};

// The user's credentials for a service (secrets: never logged)
struct Credentials
{
    QString key;
    QString username;
    QString password;
    QString email;
    bool operator==(const Credentials &other) const
    {
        return key == other.key && username == other.username && password == other.password
            && email == other.email;
    }
    bool operator!=(const Credentials &other) const { return !(*this == other); }
};

// What is known of the video
struct Query
{
    QString imdb;          // "tt0133093" (of the series for an episode), may be empty
    QString title;         // of the video, as identified, else from its name
    int year = 0;
    bool series = false;
    int season = -1;
    int episode = -1;
    QString fileName;      // of the video ("The.Matrix.1999.1080p.mkv")
    QString hash;          // OpenSubtitles hash of a local file, else empty
    qint64 size = 0;
    QStringList languages; // the user's to search, in their order (SubtitlePreferences)
    bool everyLanguage = true; // the others asked too (else only those)
    Credentials credentials; // the user's for the service asked (its own)
};

// A subtitle one of them has
struct Result
{
    QString id;            // "<service>:<its id>"
    QString provider;      // the service's id
    QString language;      // as SubtitlePreferences keeps it ("he", "pt-BR")
    QString release;       // what it was made for, as shown
    QString fileName;      // of the subtitle, when said
    QUrl url;              // its download
    QString encoding;      // of the text, when said ("CP1255")
    bool exact = false;    // made for this very file (its hash)
    bool hearingImpaired = false;
    bool foreignOnly = false; // only the parts in another language
    int downloads = 0;
    double rank = 0;       // the service's own score, higher is better
    double fps = 0;
};

struct Provider
{
    QString id;
    QString name;          // "OpenSubtitles"
    const char *detail;    // what it has ("All languages · movies and series"),
                           // untranslated (N_()): shown in the interface language
    bool movies;
    bool series;
    QStringList languages; // its only languages ("he"), empty: all; asked
                           // only when one of them is one of the user's
    int fields = 0;        // the credentials it takes (Field)
    int required = 0;      // of them, those it needs: asked only with them
    QString page;          // where its free key or account is made
    bool account = false;  // that page makes an account (else a key)
};

// The services, in the order they are asked and shown
const QList<Provider> &providers();
const Provider *provider(const QString &id);
// Whether the credentials have what the service needs
bool hasRequired(const Provider &provider, const Credentials &credentials);

// Searches one service; done is called on the thread of context with its
// results, or an error and what went wrong (and no results) when it could
// not be asked
void search(QObject *context, const QString &providerId, const Query &query,
            std::function<void(const QList<Result> &results, const QString &error, Problem problem)> done);

// What checking credentials found: they work (who, when signed in), or not
enum class Check
{
    Works,
    SignedIn,
    KeyRefused,
    LoginRefused,
    NoAnswer,
    Busy
};
// One cheap request with the credentials typed (Test, Sign in)
void check(QObject *context, const QString &providerId, const Credentials &credentials,
           std::function<void(Check check, const QString &who)> done);
// The signed-in session of a service forgotten (Sign out, new credentials)
void forgetSession(const QString &providerId);

// A download: its request (some services want headers; OpenSubtitles.com
// gives the link of each when asked), or why it can't be
struct Download
{
    QUrl url;
    QList<QPair<QByteArray, QByteArray>> headers;
    QString error;
    Problem problem = Problem::None;
};
void prepareDownload(QObject *context, const Result &result, const Credentials &credentials,
                     std::function<void(const Download &download)> done);

// The subtitle of what was downloaded: unpacked (gzip, zip), in UTF-8,
// without the ads a service adds; its extension (srt, ass, vtt...). Empty,
// with the reason, when it is not one. Runs on any thread (SubtitleFinder
// calls it on a worker one)
QByteArray subtitleOf(const QByteArray &download, const Result &result, const QString &videoName,
                      QString *extension, QString *error);

// The User-Agent the services are told
QByteArray userAgent();

// What OpenSubtitles knows of this very file (its hash): the title most of
// the subtitles made for it are filed under, with its IMDb id (of the series
// for an episode), year, season and episode. imdb is empty when it doesn't
// know the file; sure when those subtitles agree on it
struct Identity
{
    QString imdb;
    QString title;
    int year = 0;
    bool series = false;
    int season = -1;
    int episode = -1;
    bool sure = false;
};
Identity identityOfHash(const QByteArray &openSubtitlesReply);
void identifyByHash(QObject *context, const QString &hash, qint64 size, std::function<void(const Identity &)> done);

} // namespace SubtitleProviders
