/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or later.
 *****************************************************************************/
#pragma once

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <QAbstractListModel>
#include <QStringList>
#include <QUrl>
#include <QVector>

#include <memory>
#include <mutex>
#include <optional>

#include "medialibrary/medialib.hpp"
#include "medialibrary/mlqmltypes.hpp"

// Media files and subfolders of a local folder, for the library panel of the
// player: the folder of the playing media is a plain path, not a node of the
// media source trees that NetworkMediaModel browses.
class LocalFolderModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QUrl folder READ folder WRITE setFolder NOTIFY folderChanged FINAL)
    // Without a folder: these folders as the rows (the library's folders,
    // the page of its own of FolderDisplay, #112)
    Q_PROPERTY(QStringList roots READ roots WRITE setRoots NOTIFY rootsChanged FINAL)
    Q_PROPERTY(QUrl parentFolder READ parentFolder NOTIFY folderChanged FINAL)
    Q_PROPERTY(QString folderName READ folderName NOTIFY folderChanged FINAL)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged FINAL)
    // Same names as the media library models (BaseModel)
    Q_PROPERTY(QString searchPattern READ searchPattern WRITE setSearchPattern NOTIFY searchPatternChanged FINAL)
    Q_PROPERTY(QString sortCriteria READ sortCriteria WRITE setSortCriteria NOTIFY sortChanged FINAL)
    Q_PROPERTY(Qt::SortOrder sortOrder READ sortOrder WRITE setSortOrder NOTIFY sortChanged FINAL)
    // The media library, for the progress and the play count of the media
    // it knows (optional)
    Q_PROPERTY(MediaLib* ml READ ml WRITE setMl NOTIFY mlChanged FINAL)

public:
    enum Roles
    {
        NameRole = Qt::UserRole + 1,
        UrlRole,
        IsFolderRole,
        ProgressRole,
        PlayCountRole,
        // of the media the library knows
        IsFavoriteRole,
        IsIndexedRole,
        // the MRL as the core and the library write it (vlc_path2uri), as
        // text: the url (UrlRole) shown as text decodes some characters
        MrlRole,
        // of a song the library knows: its title and its artist ("title -
        // artist", as the design names songs)
        TitleRole,
        ArtistRole,
        // a song, by its suffix (the last role: the numbers of the others stay)
        IsAudioRole,
    };
    Q_ENUM(Roles)

    explicit LocalFolderModel(QObject* parent = nullptr);
    ~LocalFolderModel() override;

    QUrl folder() const { return m_folder; }
    void setFolder(const QUrl& folder);
    QStringList roots() const { return m_roots; }
    void setRoots(const QStringList& roots);
    QUrl parentFolder() const;
    QString folderName() const;
    QString searchPattern() const { return m_searchPattern; }
    void setSearchPattern(const QString& pattern);
    // "name", "date" (last modification) or "unwatched" (the media not
    // played first, then the names; "watched" with the descending order)
    QString sortCriteria() const { return m_sortCriteria; }
    void setSortCriteria(const QString& criteria);
    Qt::SortOrder sortOrder() const { return m_sortOrder; }
    void setSortOrder(Qt::SortOrder order);
    MediaLib* ml() const { return m_ml; }
    void setMl(MediaLib* ml);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Folder holding a local media, e.g. the one playing
    Q_INVOKABLE static QUrl folderOf(const QUrl& media);

    // The media of a row in the library (an invalid id when it does not know
    // it), and what the file menu changes there
    Q_INVOKABLE MLItemId mediaId(int row) const;
    Q_INVOKABLE void setPlayed(int row, bool played);
    Q_INVOKABLE void setFavorite(int row, bool favorite);

public slots:
    void reload();
    void loadLibraryData();

signals:
    void folderChanged();
    void rootsChanged();
    void countChanged();
    void searchPatternChanged();
    void sortChanged();
    void mlChanged();

private:
    struct Entry
    {
        QString name;
        // the MRL of the core (vlc_path2uri) and its url
        QString mrl;
        QUrl url;
        bool isFolder;
        // a song, by its suffix: played, it is not a media watched
        bool isAudio = false;
        qint64 modified;
        double progress = 0.;
        unsigned playCount = 0;
        int64_t mlId = 0;
        bool favorite = false;
        // of a song (empty for other media)
        QString title;
        QString artist;
    };
    // The model for the workers still running, null once it is gone
    struct Guard
    {
        std::mutex lock;
        LocalFolderModel* model = nullptr;
    };

    // (loadLibraryData is a public slot: Resync calls it when the library is idle again)
    void notifyLibraryDataChanged();
    // The rows of the entries matching the search, sorted
    void updateRows(bool refilter = true);
    void startRowUpdate(std::optional<QVector<int>> seed = std::nullopt);
    void applyRows(QVector<int> rows);
    void clearEntries();
    // Lists the folder on a thread of the pool: a network drive that does
    // not answer would freeze the interface
    void startListing();
    void onListed(QVector<Entry> entries);
    static QVector<Entry> listFolder(const QString& path);
    // the folders of these paths that are there
    static QVector<Entry> listRoots(const QStringList& paths);

    // The folder as last listed, and the rows shown (indexes in it): the
    // search and the sort work on it, without listing the folder again
    QVector<Entry> m_entries;
    QVector<int> m_rows;
    // A reload keeps showing the previous listing until its replacement is ready
    std::optional<QVector<Entry>> m_pendingEntries;
    MediaLib* m_ml = nullptr;
    std::shared_ptr<Guard> m_guard;
    // one listing at a time; another one asked meanwhile runs after it
    bool m_listing = false;
    bool m_relist = false;
    // Changes with each listing or library query so older answers are dropped
    quint64 m_generation = 0;
    // Separate from the listing so a search or sort does not invalidate enrichment
    quint64 m_rowGeneration = 0;
    quint64 m_refilterGeneration = 0;
    std::optional<QVector<Entry>> m_refilterEntries;
    bool m_updatingRows = false;
    bool m_libraryDataChanged = false;
    QUrl m_folder;
    QStringList m_roots;
    QString m_searchPattern;
    QString m_sortCriteria = QStringLiteral("name");
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;
};
