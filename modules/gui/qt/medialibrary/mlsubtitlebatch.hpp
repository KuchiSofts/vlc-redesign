/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * ( at your option ) any later version.
 *****************************************************************************/
#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QPointer>
#include <QUrl>
#include <QVariantList>
#include <QVector>

Q_MOC_INCLUDE("medialibrary/medialib.hpp")
Q_MOC_INCLUDE("player/subtitle_finder.hpp")

class MediaLib;
class SubtitleFinder;
class SubtitleSearch;

// Subtitles from the internet for every video of a folder, a season or a
// playlist (the Add Subtitles dialog, "From the Internet"): each video is
// searched as the player searches the one playing (SubtitleSearch: its
// name, its hash, the user's services), two at a time, and its subtitles
// in the user's language (else the second one) listed: made for this very
// file (its hash), or the best guess for its title, season and episode.
// The ones chosen are downloaded with the linked subtitles and linked to
// their video in the media library, as a file added by hand: VLC loads
// them when the video plays, and the player knows them as found online.
class MLSubtitleBatch : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(MediaLib* ml READ ml WRITE setMl FINAL)
    Q_PROPERTY(SubtitleFinder* finder READ finder WRITE setFinder FINAL)
    // still searching, or downloading the ones chosen
    Q_PROPERTY(bool searching READ searching NOTIFY progressChanged FINAL)
    Q_PROPERTY(bool downloading READ downloading NOTIFY progressChanged FINAL)
    Q_PROPERTY(int total READ total NOTIFY progressChanged FINAL)
    // searched (found or not)
    Q_PROPERTY(int searched READ searched NOTIFY progressChanged FINAL)
    Q_PROPERTY(int exactCount READ exactCount NOTIFY progressChanged FINAL)
    Q_PROPERTY(int guessCount READ guessCount NOTIFY progressChanged FINAL)
    Q_PROPERTY(int notFoundCount READ notFoundCount NOTIFY progressChanged FINAL)
    Q_PROPERTY(int failedCount READ failedCount NOTIFY progressChanged FINAL)
    Q_PROPERTY(int linkedCount READ linkedCount NOTIFY progressChanged FINAL)
    // the ones to download (a subtitle chosen)
    Q_PROPERTY(int chosenCount READ chosenCount NOTIFY progressChanged FINAL)
    // the language looked for, the second one too ("he", "en")
    Q_PROPERTY(QString language READ language NOTIFY progressChanged FINAL)
    Q_PROPERTY(QString secondLanguage READ secondLanguage NOTIFY progressChanged FINAL)

public:
    enum RowState {
        Waiting,     // for a search to end (two at a time)
        Searching,
        Exact,       // made for this very file (its hash)
        Guess,       // of its title (season and episode): chosen or not
        NotFound,
        Failed,      // no answer (the error)
        Downloading,
        Linked
    };
    Q_ENUM(RowState)

    enum Role {
        LabelRole = Qt::UserRole + 1, // the video's file name
        StateRole,
        // the video as identified ("Night Harbor · S01E02", "Sintel (2010)")
        TitleRole,
        // { id, label: the release, detail: service (and language when it
        // isn't the first), exact }, the best first
        CandidatesRole,
        // the one to download, -1: none
        ChoiceRole,
        ErrorRole,
    };

    explicit MLSubtitleBatch(QObject* parent = nullptr);
    ~MLSubtitleBatch() override;

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    MediaLib* ml() const { return m_ml; }
    void setMl(MediaLib* ml) { m_ml = ml; }
    SubtitleFinder* finder() const { return m_finder; }
    void setFinder(SubtitleFinder* finder) { m_finder = finder; }

    bool searching() const;
    bool downloading() const;
    int total() const { return int(m_rows.size()); }
    int searched() const;
    int exactCount() const { return count(Exact); }
    int guessCount() const { return count(Guess); }
    int notFoundCount() const { return count(NotFound); }
    int failedCount() const { return count(Failed); }
    int linkedCount() const { return count(Linked); }
    int chosenCount() const;
    QString language() const;
    QString secondLanguage() const;

    // the videos ({ label, url }: MLSubtitleAssociations::destinations),
    // searched; the ones before are forgotten
    Q_INVOKABLE void start(const QVariantList& media);
    // the subtitle of a video to download (-1: none)
    Q_INVOKABLE void choose(int row, int candidate);
    // the ones chosen downloaded and linked (finished() at the end)
    Q_INVOKABLE void download();
    // every search and download stopped, the list emptied
    Q_INVOKABLE void cancel();

signals:
    void progressChanged();
    // the downloads ended: linked to their video, or not
    void finished(int linked, int failed);

private:
    struct Row
    {
        QUrl url;
        QString label;
        RowState state = Waiting;
        // its search, kept for its download (deleted: its answers dropped)
        QPointer<SubtitleSearch> search;
        QString title;
        QVariantList candidates;
        int choice = -1;
        QString error;
    };

    int count(RowState state) const;
    void setState(int row, RowState state, const QString& error = {});
    // the next videos waiting searched, two at a time
    void pumpSearches();
    void startSearch(int row);
    void onSearched(int row);
    void pumpDownloads();
    void startDownload(int row);
    void linkFile(int row, const QString& file);
    void downloadDone(int row, RowState state, const QString& error = {});

    QPointer<MediaLib> m_ml;
    QPointer<SubtitleFinder> m_finder;
    QVector<Row> m_rows;
    QList<int> m_downloadQueue;
    int m_activeSearches = 0;
    int m_activeDownloads = 0;
    bool m_downloadsStarted = false;
    // the list (start, cancel): the answers for the one before are dropped
    quint64 m_generation = 0;
};
