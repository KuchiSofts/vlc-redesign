/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariantList>

#include <functional>

#include "qt.hpp"
#include "player/subtitle_search.hpp"

class PlayerController;
class QSettings;
class SubtitlePreferences;

// Subtitles from the internet for the media playing: SubtitleSearch
// identifies the video (its file name read for the title, the year, the
// season and episode; IMDb's suggestions, else Wikidata, then Stremio's
// Cinemeta, give its IMDb id), asks the free services together
// (SubtitleProviders: OpenSubtitles, Wizdom for Hebrew, Addic7ed for series,
// YIFY for movies) and ranks their subtitles of every language, those made
// for this very file first (OpenSubtitles hash), each with the service it
// comes from. One loads in a click, unpacked and converted to UTF-8, kept
// in VLC's cache and remembered for the file. When a video starts without
// subtitles in the user's language (SubtitlePreferences), the best one
// loads by itself: of the default language, else of the second one.
class SubtitleFinder : public QObject
{
    Q_OBJECT
    Q_PROPERTY(State state READ state NOTIFY stateChanged FINAL)
    // "Home Sweet Home Alone (2021)", "Breaking Bad · S01E02"
    Q_PROPERTY(QString matchTitle READ matchTitle NOTIFY stateChanged FINAL)
    Q_PROPERTY(QString errorText READ errorText NOTIFY stateChanged FINAL)
    // { id, language, languageName, nativeName, release, exact, hearingImpaired }
    // by language, the best first in each
    Q_PROPERTY(QVariantList results READ results NOTIFY resultsChanged FINAL)
    // the result downloading, and the ones loaded for this media
    Q_PROPERTY(QString loadingId READ loadingId NOTIFY resultsChanged FINAL)
    Q_PROPERTY(QStringList loadedIds READ loadedIds NOTIFY resultsChanged FINAL)
    // titles found for findTitle(): { label, detail }
    Q_PROPERTY(QVariantList titleResults READ titleResults NOTIFY titleResultsChanged FINAL)
    Q_PROPERTY(bool titleBusy READ titleBusy NOTIFY titleResultsChanged FINAL)
    Q_PROPERTY(bool supported READ supported CONSTANT FINAL)
    // the services: { id, name, detail, enabled, busy, count (-1: not
    // asked), error } (setServiceEnabled)
    Q_PROPERTY(QVariantList services READ services NOTIFY providersChanged FINAL)

public:
    enum State {
        Idle,
        Identifying, // which video this is
        Searching,   // its subtitles
        Ready,
        NotFound,    // no subtitles, or the video wasn't identified
        Failed       // no connection, or an answer that couldn't be read
    };
    Q_ENUM(State)

    SubtitleFinder(qt_intf_t *intf, PlayerController *player, SubtitlePreferences *preferences,
                   QObject *parent = nullptr);
    ~SubtitleFinder() override;

    State state() const { return State(m_search->state()); }
    QString matchTitle() const { return m_search->matchTitle(); }
    QString errorText() const { return m_search->errorText(); }
    QVariantList results() const { return m_search->results(); }
    QString loadingId() const { return m_loadingId; }
    QStringList loadedIds() const { return m_loadedIds; }
    QVariantList titleResults() const { return m_titleResults; }
    bool titleBusy() const { return m_titleBusy; }
    bool supported() const;
    QVariantList services() const;
    bool serviceEnabled(const QString &id) const;
    // A search of the subtitles of another video, played or not (the Add
    // Subtitles dialog's, for each video of a folder): the user's languages
    // and services
    SubtitleSearch *newSearch(QObject *parent) const;
    SubtitlePreferences *preferences() const { return m_preferences; }
    QSettings *settings() const;
    // Searches without being asked (a video starts, the subtitles menu
    // opens): the user's setting, once the user was asked about it (the
    // first-run setup, the installer)
    Q_INVOKABLE bool searchesByItself() const;

    // The name of a subtitle loaded from the internet (its release), for the
    // track VLC made of it ("<md5 of its MRL>/spu/0"); empty for the others
    static QString externalTitle(const char *trackId);
    // Where it comes from: { kind: "file" (found next to the video), "added"
    // (opened by the user) or "online"; name: of the file, or the release;
    // provider: the service it was found on; folder: where the file is };
    // empty for the tracks of the media itself and unknown files
    static QVariantMap externalSource(const char *trackId);
    // the local file of an external track (empty when it isn't known)
    static QString externalPath(const char *trackId);
    // a subtitle file added to the media (its MRL as VLC has it)
    static void noteFile(const QString &mrl, const QString &kind, const QString &name,
                         const QString &provider = {});

public slots:
    // the subtitles of the media playing (again)
    void search();
    // titles for a name the user typed, when the video was taken for another
    void findTitle(const QString &query);
    // the subtitles of one of them, remembered for this file
    void useTitle(int index);
    // downloads one, adds it to the media and shows it
    void load(const QString &id);
    // the same, shown with the subtitle already shown (the secondary one)
    void loadAlongside(const QString &id);
    // a service asked or not (kept)
    void setServiceEnabled(const QString &id, bool enabled);

signals:
    void stateChanged();
    void resultsChanged();
    void titleResultsChanged();
    // a subtitle loaded by itself (the player tells it): its language
    void autoLoaded(const QString &languageName);
    // what is known of where the tracks come from changed (externalSource)
    void sourcesChanged();
    void providersChanged();
    // a subtitle chosen could not be loaded: why
    void downloadFailed(const QString &error);

private:
    void onInputChanged(bool hasInput);
    // the subtitle files next to the video (and in its subtitle folders)
    void scanFolder();
    void onStarted();
    // the media playing given to m_search (its name read, its file hashed),
    // then next()
    void prepareMedia(std::function<void()> next);
    void onSearchFinished();
    void autoPick();
    void attach(const QString &file, const QString &id, const QString &language, bool select,
                bool alongside = false);
    void download(const QString &id, bool alongside);
    // the first of m_queue, if any
    void downloadNext();
    // the secondary language shown too (SubtitlePreferences::showBoth)
    void ensureSecondary();
    // the best one of the language the user picks most (learned), unless
    // the video has it (local) or one is loaded
    QVariantMap bestOfLearned(const QStringList &local) const;
    // the subtitle chosen for this file last time ({ file, id, language,
    // release, service }), if its file is still there
    QStringList rememberedChoice() const;
    bool hasTrackOf(const QString &file) const;
    // the track of the file just added, once VLC lists it: shown
    void selectAdded();
    QStringList localLanguages() const;
    bool loadedLanguage(const QString &language) const;

    qt_intf_t *m_intf;
    PlayerController *m_player;
    SubtitlePreferences *m_preferences;
    // the media playing's: identified, its subtitles found and downloaded
    SubtitleSearch *m_search;
    QTimer m_startTimer;
    // the media playing (an answer for the one before is dropped)
    quint64 m_revision = 0;
    bool m_autoPending = false;
    QString m_loadingId;
    QStringList m_loadedIds;
    QVariantList m_titleResults;
    bool m_titleBusy = false;
    QString m_selectHash; // the file whose track is to be shown
    bool m_selectAlongside = false;
    // downloads waiting for the one going on: { id, alongside }
    QList<QPair<QString, bool>> m_queue;
};
