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

// The free services subtitles are found on, without an account: what each
// is asked, and what it answers, for SubtitleFinder. Each search is a few
// requests, one after the other (the services ask for no more), at most a
// few seconds; a service that does not answer is left out of the results.
namespace SubtitleProviders
{

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
    QStringList languages; // the user's, the default first (SubtitlePreferences)
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
    QString detail;        // what it has ("All languages · movies and series")
    bool movies;
    bool series;
    QString onlyLanguage;  // its only language ("he"), empty: all
};

// The services, in the order they are asked and shown
const QList<Provider> &providers();
const Provider *provider(const QString &id);

// Searches one service; done is called on the thread of context with its
// results, or an error (and no results) when it could not be asked
void search(QObject *context, const QString &providerId, const Query &query,
            std::function<void(const QList<Result> &results, const QString &error)> done);

// The request of a download (its headers: some services want them)
QList<QPair<QByteArray, QByteArray>> downloadHeaders(const Result &result);

// The subtitle of what was downloaded: unpacked (gzip, zip), in UTF-8,
// without the ads a service adds; its extension (srt, ass, vtt...). Empty,
// with the reason, when it is not one. Runs on any thread (SubtitleFinder
// calls it on a worker one)
QByteArray subtitleOf(const QByteArray &download, const Result &result, const QString &videoName,
                      QString *extension, QString *error);

// The User-Agent the services are told
QByteArray userAgent();

} // namespace SubtitleProviders
