/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or later.
 *****************************************************************************/
#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "local_folder_model.hpp"

#include "medialibrary/medialib.hpp"
#include "medialibrary/mlhelper.hpp"

#include <QCollator>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <QThreadPool>

#include <algorithm>

#include <vlc_common.h>
#include <vlc_interface.h>
#include <vlc_media_library.h>
#include <vlc_url.h>

#include <vector>

namespace {
// Suffixes of the files VLC opens, from "*.mkv;*.mp4;..."
const QSet<QString>& mediaSuffixes()
{
    static const QSet<QString> suffixes = [] {
        QSet<QString> set;
        const QString patterns = QStringLiteral(EXTENSIONS_VIDEO ";" EXTENSIONS_AUDIO ";" EXTENSIONS_PLAYLIST);
        for (const QString& pattern : patterns.split(QLatin1Char(';'), Qt::SkipEmptyParts))
            set.insert(pattern.mid(2).toLower()); // drop "*."
        return set;
    }();
    return suffixes;
}

// The MRL of a local path as the core writes it (vlc_path2uri), the one the
// media library stores: QUrl::fromLocalFile leaves ( ) ' ! & + , ; = @ $
// as they are, the media would not be found (resume, artwork, subtitles)
// and played under another MRL, a second media of the library
QString coreMrl(const QString& path)
{
    return MediaLib::mrlFromUrl(QUrl::fromLocalFile(path));
}

// Its url: the encoding of the MRL is kept (QUrl leaves the encoded
// delimiters as they are, QUrl::FullyEncoded gives the MRL back)
QUrl coreUrl(const QString& path, const QString& mrl)
{
    return mrl.isEmpty() ? QUrl::fromLocalFile(path) : QUrl::fromEncoded(mrl.toUtf8());
}

QUrl coreUrl(const QString& path)
{
    return coreUrl(path, coreMrl(path));
}
}

LocalFolderModel::LocalFolderModel(QObject* parent)
    : QAbstractListModel(parent)
    , m_guard(std::make_shared<Guard>())
{
    m_guard->model = this;
}

LocalFolderModel::~LocalFolderModel()
{
    // a listing ending later finds no model
    std::lock_guard<std::mutex> lock(m_guard->lock);
    m_guard->model = nullptr;
}

void LocalFolderModel::setFolder(const QUrl& folder)
{
    if (folder == m_folder)
        return;
    m_folder = folder;
    emit folderChanged();
    // the entries of the previous folder go now, not when the listing ends
    m_entries.clear();
    ++m_generation;
    updateRows();
    startListing();
}

QUrl LocalFolderModel::parentFolder() const
{
    if (!m_folder.isLocalFile())
        return {};
    QDir dir(m_folder.toLocalFile());
    if (!dir.cdUp())
        return {};
    return coreUrl(dir.absolutePath());
}

QString LocalFolderModel::folderName() const
{
    if (!m_folder.isLocalFile())
        return {};
    const QFileInfo info(m_folder.toLocalFile());
    // A drive root has no name: show "C:" rather than nothing
    return info.fileName().isEmpty() ? QDir::toNativeSeparators(info.absoluteFilePath()).remove(QLatin1Char('\\'))
                                     : info.fileName();
}

void LocalFolderModel::setSearchPattern(const QString& pattern)
{
    if (pattern == m_searchPattern)
        return;
    m_searchPattern = pattern;
    emit searchPatternChanged();
    updateRows();
}

void LocalFolderModel::setSortCriteria(const QString& criteria)
{
    if (criteria == m_sortCriteria)
        return;
    m_sortCriteria = criteria;
    emit sortChanged();
    updateRows();
}

void LocalFolderModel::setSortOrder(Qt::SortOrder order)
{
    if (order == m_sortOrder)
        return;
    m_sortOrder = order;
    emit sortChanged();
    updateRows();
}

QUrl LocalFolderModel::folderOf(const QUrl& media)
{
    if (!media.isLocalFile())
        return {};
    return coreUrl(QFileInfo(media.toLocalFile()).absolutePath());
}

void LocalFolderModel::reload()
{
    startListing();
}

void LocalFolderModel::startListing()
{
    if (m_listing)
    {
        m_relist = true;
        return;
    }
    if (!m_folder.isLocalFile())
    {
        onListed({});
        return;
    }
    m_listing = true;
    const QString path = m_folder.toLocalFile();
    const std::shared_ptr<Guard> guard = m_guard;
    QThreadPool::globalInstance()->start([guard, path]() {
        const QVector<Entry> entries = listFolder(path);
        std::lock_guard<std::mutex> lock(guard->lock);
        if (LocalFolderModel* model = guard->model)
            QMetaObject::invokeMethod(model, [model, entries]() {
                model->m_listing = false;
                // the folder changed or was reloaded meanwhile
                if (model->m_relist)
                {
                    model->m_relist = false;
                    model->startListing();
                    return;
                }
                model->onListed(entries);
            }, Qt::QueuedConnection);
    });
}

QVector<LocalFolderModel::Entry> LocalFolderModel::listFolder(const QString& path)
{
    QVector<Entry> entries;
    const QDir dir(path);
    const QFileInfoList infos = dir.entryInfoList(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot
                                                  | QDir::Readable);
    for (const QFileInfo& info : infos)
    {
        const bool isFolder = info.isDir();
        if (!isFolder && !mediaSuffixes().contains(info.suffix().toLower()))
            continue;
        const QString filePath = info.absoluteFilePath();
        const QString mrl = coreMrl(filePath);
        entries.push_back({ info.fileName(), mrl, coreUrl(filePath, mrl), isFolder,
                            info.lastModified().toMSecsSinceEpoch() });
    }
    return entries;
}

void LocalFolderModel::onListed(const QVector<Entry>& entries)
{
    m_entries = entries;
    ++m_generation;
    updateRows();
    loadLibraryData();
}

void LocalFolderModel::updateRows()
{
    beginResetModel();
    m_rows.clear();
    for (int i = 0; i < m_entries.size(); ++i)
        if (m_searchPattern.isEmpty() || m_entries[i].name.contains(m_searchPattern, Qt::CaseInsensitive))
            m_rows.push_back(i);
    sortRows();
    endResetModel();
    emit countChanged();
}

void LocalFolderModel::sortRows()
{
    // Folders first, then names the way people read them (2 before 10)
    QCollator collator;
    collator.setNumericMode(true);
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    const bool byDate = m_sortCriteria == QLatin1String("date");
    const bool byWatched = m_sortCriteria == QLatin1String("unwatched");
    const bool descending = m_sortOrder == Qt::DescendingOrder;
    const auto played = [](const Entry& e) { return e.playCount > 0 || e.progress > 0.; };
    std::sort(m_rows.begin(), m_rows.end(), [&](int rowA, int rowB) {
        const Entry& a = m_entries[rowA];
        const Entry& b = m_entries[rowB];
        if (a.isFolder != b.isFolder)
            return a.isFolder;
        if (byWatched && played(a) != played(b))
            return descending ? played(a) : played(b);
        const int order = byDate ? (a.modified < b.modified ? -1 : a.modified > b.modified)
                                 : collator.compare(a.name, b.name);
        return (descending && !byWatched) ? order > 0 : order < 0;
    });
}

void LocalFolderModel::setMl(MediaLib* ml)
{
    if (m_ml == ml)
        return;
    m_ml = ml;
    emit mlChanged();
    loadLibraryData();
}

void LocalFolderModel::loadLibraryData()
{
    if (!m_ml || m_entries.isEmpty())
        return;

    // The MRLs the library stores (vlc_path2uri) of the media of the folder,
    // all of them: the search only hides rows
    std::vector<std::pair<int, QString>> mrls;
    for (int i = 0; i < m_entries.size(); ++i)
    {
        if (m_entries[i].isFolder || m_entries[i].mrl.isEmpty())
            continue;
        mrls.emplace_back(i, m_entries[i].mrl);
    }
    if (mrls.empty())
        return;

    // An answer goes to the entry of its MRL, wherever the sort or the
    // search put its row meanwhile
    struct Result
    {
        int entry;
        QString mrl;
        double progress;
        unsigned playCount;
        int64_t id;
        bool favorite;
    };
    struct Ctx
    {
        std::vector<Result> results;
    };
    const quint64 generation = m_generation;
    m_ml->runOnMLThread<Ctx>(this,
    // ML thread
    [mrls](vlc_medialibrary_t* ml, Ctx& ctx)
    {
        for (const auto& entry : mrls)
        {
            ml_unique_ptr<vlc_ml_media_t> media{ vlc_ml_get_media_by_mrl(ml, qtu(entry.second)) };
            if (media)
                ctx.results.push_back({ entry.first, entry.second, media->f_progress,
                                        media->i_playcount, media->i_id, media->b_is_favorite });
        }
    },
    // UI thread
    [this, generation](quint64, Ctx& ctx)
    {
        if (generation != m_generation || ctx.results.empty())
            return;
        for (const auto& result : ctx.results)
        {
            if (result.entry >= m_entries.size() || m_entries[result.entry].mrl != result.mrl)
                continue;
            Entry& entry = m_entries[result.entry];
            entry.progress = result.progress > 0. ? result.progress : 0.;
            entry.playCount = result.playCount;
            entry.mlId = result.id;
            entry.favorite = result.favorite;
        }
        if (m_sortCriteria == QLatin1String("unwatched"))
        {
            // the order depends on what was played
            beginResetModel();
            sortRows();
            endResetModel();
            return;
        }
        if (!m_rows.isEmpty())
            emit dataChanged(index(0), index(m_rows.size() - 1),
                             { ProgressRole, PlayCountRole, IsFavoriteRole, IsIndexedRole });
    });
}

int LocalFolderModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_rows.size();
}

QVariant LocalFolderModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_rows.size())
        return {};
    const Entry& entry = m_entries[m_rows[index.row()]];
    switch (role)
    {
    case Qt::DisplayRole:
    case NameRole:
        return entry.name;
    case UrlRole:
        return entry.url;
    case MrlRole:
        return entry.mrl.isEmpty() ? entry.url.toString(QUrl::FullyEncoded) : entry.mrl;
    case IsFolderRole:
        return entry.isFolder;
    case ProgressRole:
        return entry.progress;
    case PlayCountRole:
        return entry.playCount;
    case IsFavoriteRole:
        return entry.favorite;
    case IsIndexedRole:
        return entry.mlId > 0;
    default:
        return {};
    }
}

MLItemId LocalFolderModel::mediaId(int row) const
{
    if (row < 0 || row >= m_rows.size() || m_entries[m_rows[row]].mlId <= 0)
        return {};
    return MLItemId(m_entries[m_rows[row]].mlId, VLC_ML_PARENT_UNKNOWN);
}

void LocalFolderModel::setPlayed(int row, bool played)
{
    if (!m_ml || row < 0 || row >= m_rows.size() || m_entries[m_rows[row]].mlId <= 0)
        return;
    const int64_t id = m_entries[m_rows[row]].mlId;
    m_ml->runOnMLThread(this,
    [id, played](vlc_medialibrary_t* ml)
    {
        vlc_ml_media_set_played(ml, id, played);
    },
    // the progress and the play count again (and the order of "unwatched")
    [this]() { loadLibraryData(); });
}

void LocalFolderModel::setFavorite(int row, bool favorite)
{
    if (!m_ml || row < 0 || row >= m_rows.size() || m_entries[m_rows[row]].mlId <= 0)
        return;
    const int64_t id = m_entries[m_rows[row]].mlId;
    m_entries[m_rows[row]].favorite = favorite;
    emit dataChanged(index(row), index(row), { IsFavoriteRole });
    m_ml->runOnMLThread(this,
    [id, favorite](vlc_medialibrary_t* ml)
    {
        vlc_ml_media_set_favorite(ml, id, favorite);
    });
}

QHash<int, QByteArray> LocalFolderModel::roleNames() const
{
    return {
        { NameRole, "name" },
        { UrlRole, "url" },
        { IsFolderRole, "isFolder" },
        { ProgressRole, "progress" },
        { PlayCountRole, "playcount" },
        { IsFavoriteRole, "isFavorite" },
        { IsIndexedRole, "isIndexed" },
        { MrlRole, "mrl" },
    };
}
