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
#include <QUrl>
#include <QVector>

#include <memory>
#include <mutex>

#include "medialibrary/medialib.hpp"
#include "medialibrary/mlqmltypes.hpp"

// Media files and subfolders of a local folder, for the library panel of the
// player: the folder of the playing media is a plain path, not a node of the
// media source trees that NetworkMediaModel browses.
class LocalFolderModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QUrl folder READ folder WRITE setFolder NOTIFY folderChanged FINAL)
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
    };
    Q_ENUM(Roles)

    explicit LocalFolderModel(QObject* parent = nullptr);
    ~LocalFolderModel() override;

    QUrl folder() const { return m_folder; }
    void setFolder(const QUrl& folder);
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

signals:
    void folderChanged();
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
        qint64 modified;
        double progress = 0.;
        unsigned playCount = 0;
        int64_t mlId = 0;
        bool favorite = false;
    };
    // The model for the listings still running, null once it is gone
    struct Guard
    {
        std::mutex lock;
        LocalFolderModel* model = nullptr;
    };

    // Reads the progress and the play count of the media from the library
    void loadLibraryData();
    // The rows of the entries matching the search, sorted
    void updateRows();
    void sortRows();
    // Lists the folder on a thread of the pool: a network drive that does
    // not answer would freeze the interface
    void startListing();
    void onListed(const QVector<Entry>& entries);
    static QVector<Entry> listFolder(const QString& path);

    // The folder as last listed, and the rows shown (indexes in it): the
    // search and the sort work on it, without listing the folder again
    QVector<Entry> m_entries;
    QVector<int> m_rows;
    MediaLib* m_ml = nullptr;
    std::shared_ptr<Guard> m_guard;
    // one listing at a time; another one asked meanwhile runs after it
    bool m_listing = false;
    bool m_relist = false;
    // Changes with each listing: older answers of the library are dropped
    quint64 m_generation = 0;
    QUrl m_folder;
    QString m_searchPattern;
    QString m_sortCriteria = QStringLiteral("name");
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;
};
