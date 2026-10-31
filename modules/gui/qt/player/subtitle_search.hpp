/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
#pragma once

#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>

#include <functional>

#include "qt.hpp"
#include "player/subtitle_providers.hpp"

class QSettings;
class SubtitlePreferences;

// The subtitles of one video on the internet, played or not: its name read
// (the title, the year, the season and episode), its file hashed, the video
// identified (a title chosen for the file before, else IMDb's suggestions,
// Wikidata, then Stremio's Cinemeta, for its IMDb id), the services the
// user left on asked together (SubtitleProviders), their subtitles ranked
// (made for this very file first) and one downloaded, unpacked and
// converted to UTF-8. SubtitleFinder runs one for the media playing, the
// Add Subtitles dialog one for each video of a folder (MLSubtitleBatch).
class SubtitleSearch : public QObject
{
    Q_OBJECT

public:
    // (SubtitleFinder::State in the same order)
    enum State {
        Idle,
        Identifying, // which video this is
        Searching,   // its subtitles
        Ready,
        NotFound,    // no subtitles, or the video wasn't identified
        Failed       // no connection, or an answer that couldn't be read
    };

    struct Match
    {
        QString imdb;     // tt…
        QString title;
        int year = 0;
        bool series = false;
        // the user's choice, or the same title with its year (or episode,
        // or two words or more): subtitles of it are loaded by themselves.
        // A guess from a name alone (silent.mp4 → "Silent Hill") is only
        // offered in the list
        bool strong = false;
    };
    // what a service is doing for this search
    struct Run
    {
        bool busy = false;
        int count = 0;
        QString error;
    };

    SubtitleSearch(qt_intf_t *intf, SubtitlePreferences *preferences, QObject *parent = nullptr);

    // The video: its address (a local file is hashed), its name (of the
    // file, else of the media) and its frame rate when known. Forgets the
    // one before (its answers are dropped)
    void setMedia(const QUrl &url, const QString &name, double fps = 0);
    // nothing searched any more: the answers on their way are dropped
    void reset();
    // its name read and its file hashed (on a worker thread), then next();
    // at once when done already
    void prepare(std::function<void()> next);
    // identified, then its subtitles asked for (finished() at the end)
    void start();
    // the subtitles of a title the user chose, remembered for this file
    void useMatch(const Match &match);
    // titles for a name the user typed: { imdb, label, detail, year, series }
    void findTitles(const QString &query, std::function<void(const QVariantList &titles)> done);
    // One of the results saved into folder ("<release>-<id>.<language>.<ext>",
    // VLC reads the language from the name; found there already: not
    // downloaded again), then done(file) or done({}, why). prune: the
    // other media's folders next to it not used for long are deleted
    void download(const QString &id, const QString &folder, bool prune,
                  std::function<void(const QString &file, const QString &error)> done);
    // the state kept, with this error (a download that failed)
    void setError(const QString &error);

    QUrl url() const { return m_url; }
    // the file (hash and size), or its URL: what is remembered of it
    QString mediaKey() const { return m_media.key; }
    QString mediaName() const { return m_media.name; }
    QString mediaTitle() const { return m_media.title; }
    int season() const { return m_media.season; }
    int episode() const { return m_media.episode; }
    bool hashed() const { return !m_media.hash.isEmpty(); }

    State state() const { return m_state; }
    QString errorText() const { return m_error; }
    const Match &match() const { return m_match; }
    // "Home Sweet Home Alone (2021)", "Breaking Bad · S01E02"; empty while
    // the video isn't identified
    QString matchTitle() const;
    // { id, provider, providerName, language, languageName, nativeName,
    // release, exact, hearingImpaired, foreignOnly, downloads }: by
    // language, the user's first, the best first in each
    QVariantList results() const { return m_results; }
    const QList<SubtitleProviders::Result> &found() const { return m_found; }
    const SubtitleProviders::Result *result(const QString &id) const;
    // the shown map of a result (empty when unknown)
    QVariantMap resultMap(const QString &id) const;
    const QHash<QString, Run> &runs() const { return m_runs; }
    // the best subtitle of a language that can be loaded by itself: made
    // for this file (its hash), or of a title identified for sure
    QVariantMap bestOf(const QString &language) const;

    // a service asked or not (the user's setting, kept)
    static bool serviceEnabled(QSettings *settings, const QString &id);
    static void setServiceEnabled(QSettings *settings, const QString &id, bool enabled);
    // The subtitle chosen for a file ({ file, id, language, release,
    // service }; the last two may be missing), when its file is still there
    static QStringList rememberedChoice(QSettings *settings, const QString &mediaKey);
    static void rememberChoice(QSettings *settings, const QString &mediaKey, const QStringList &choice);
    // the name of the folder of a media's subtitles (in the cache, or with
    // the linked ones)
    static QString folderName(const QString &mediaKey);
    // where the subtitles found online are cached (VLC's cache), empty
    // without one
    static QString cacheRoot();

signals:
    void stateChanged();
    void resultsChanged();
    // what the services are doing (runs())
    void runsChanged();
    // the search ended: Ready, NotFound or Failed
    void finished();

private:
    struct Media
    {
        QString path;     // local file, empty otherwise
        QString name;     // file name, or the media's name
        QString key;      // the file (hash and size), or its URL
        QString hash;     // OpenSubtitles hash, empty if not a local file
        qint64 size = 0;
        QString title;    // from the name
        int year = 0;
        int season = -1;
        int episode = -1;
        QStringList tokens; // of the release (2160p, web, yts…)
        double fps = 0;
    };

    void setState(State state, const QString &error = {});
    void identify();
    void searchImdb(const QString &query, bool forMedia, std::function<void(const QVariantList &)> done = {});
    void searchTitles(const QString &query, bool forMedia, std::function<void(const QVariantList &)> done = {});
    // the best of them for the media; otherwise() when none fits it
    void onTitles(const QVariantList &candidates, bool forMedia, std::function<void(const QVariantList &)> done,
                  std::function<void()> otherwise = {});
    void searchCinemeta();
    void querySubtitles();
    void onProviderDone(const QString &id, const QList<SubtitleProviders::Result> &results,
                        const QString &error);
    void rebuildResults();
    void finishSearch();
    bool strongTitle(double similarity, int year, bool series) const;

    qt_intf_t *m_intf;
    SubtitlePreferences *m_preferences;
    QUrl m_url;
    bool m_prepared = false;
    // the media (setMedia, reset) and the title (useMatch): the answers of
    // before are dropped
    quint64 m_revision = 0;
    // the search going on (its answers after another starts are dropped)
    quint64 m_searchRevision = 0;
    // the downloads of this media (not m_revision: choosing another title
    // doesn't stop them)
    quint64 m_downloadRevision = 0;
    State m_state = Idle;
    QString m_error;
    Media m_media;
    Match m_match;
    QVariantList m_results;
    QList<SubtitleProviders::Result> m_found;
    QHash<QString, Run> m_runs;
    int m_pending = 0;
};
