/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or later.
 *****************************************************************************/
#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "local_folder_model.hpp"
#include "local_folder_order.hpp"

#include "medialibrary/medialib.hpp"
#include "medialibrary/mlhelper.hpp"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QLocale>
#include <QSet>
#include <QThreadPool>

#include <algorithm>
#include <utility>

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
    // A worker ending later finds no model
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
    clearEntries();
    startListing();
}

void LocalFolderModel::setRoots(const QStringList& roots)
{
    if (roots == m_roots)
        return;
    m_roots = roots;
    emit rootsChanged();
    // (shown only without a folder)
    if (m_folder.isLocalFile())
        return;
    clearEntries();
    startListing();
}

void LocalFolderModel::clearEntries()
{
    ++m_generation;
    m_refilterGeneration = ++m_rowGeneration;
    m_refilterEntries.reset();
    m_pendingEntries.reset();
    m_libraryDataChanged = false;
    beginResetModel();
    m_entries.clear();
    m_rows.clear();
    endResetModel();
    emit countChanged();
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
    // (a folder of the library ends with a slash: its name is before it)
    const QFileInfo info(QDir::cleanPath(m_folder.toLocalFile()));
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
    // without a folder, the roots (the local ones: a folder of the network
    // has no page of this kind)
    const QString path = m_folder.isLocalFile() ? m_folder.toLocalFile() : QString();
    QStringList roots;
    if (path.isEmpty())
        for (const QString& root : m_roots)
        {
            const QUrl url(root);
            if (url.isLocalFile())
                roots.push_back(url.toLocalFile());
        }
    if (path.isEmpty() && roots.isEmpty())
    {
        onListed({});
        return;
    }
    m_listing = true;
    const std::shared_ptr<Guard> guard = m_guard;
    // (a folder of a network drive that does not answer would freeze the
    // interface: even the roots are looked at on the pool)
    QThreadPool::globalInstance()->start([guard, path, roots]() {
        const QVector<Entry> entries = path.isEmpty() ? listRoots(roots) : listFolder(path);
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
        Entry entry;
        entry.name = info.fileName();
        entry.mrl = mrl;
        entry.url = coreUrl(filePath, mrl);
        entry.isFolder = isFolder;
        entry.modified = info.lastModified().toMSecsSinceEpoch();
        entries.push_back(std::move(entry));
    }
    return entries;
}

QVector<LocalFolderModel::Entry> LocalFolderModel::listRoots(const QStringList& paths)
{
    QVector<Entry> entries;
    QSet<QString> seen;
    for (const QString& path : paths)
    {
        // (a folder of the library ends with a slash)
        const QFileInfo info(QDir::cleanPath(path));
        const QString filePath = info.absoluteFilePath();
        if (!info.isDir() || seen.contains(filePath))
            continue;
        seen.insert(filePath);
        const QString mrl = coreMrl(filePath);
        Entry entry;
        // A drive root has no name: "C:" rather than nothing (folderName)
        entry.name = info.fileName().isEmpty() ? QDir::toNativeSeparators(filePath).remove(QLatin1Char('\\'))
                                               : info.fileName();
        entry.mrl = mrl;
        entry.url = coreUrl(filePath, mrl);
        entry.isFolder = true;
        entry.modified = info.lastModified().toMSecsSinceEpoch();
        entries.push_back(std::move(entry));
    }
    return entries;
}

void LocalFolderModel::onListed(QVector<Entry> entries)
{
    ++m_generation;
    m_pendingEntries = std::move(entries);
    m_libraryDataChanged = false;
    updateRows();
}

void LocalFolderModel::updateRows(bool refilter)
{
    ++m_rowGeneration;
    if (refilter)
    {
        m_refilterGeneration = m_rowGeneration;
        m_refilterEntries = m_pendingEntries ? *m_pendingEntries : m_entries;
    }
    if (m_updatingRows)
        return;
    if (refilter)
        startRowUpdate();
    else
        startRowUpdate(m_rows);
}

void LocalFolderModel::startRowUpdate(std::optional<QVector<int>> seed)
{
    // Keep the rebuild's original input if enrichment arrived while it was queued
    const quint64 generation = m_refilterEntries ? m_refilterGeneration : m_rowGeneration;
    auto refilterEntries = std::exchange(m_refilterEntries, std::nullopt);
    const QVector<Entry>& entries = refilterEntries ? *refilterEntries : m_entries;
    if (entries.isEmpty())
    {
        applyRows({});
        return;
    }

    // Keep one job in flight so typing does not queue a sort for each keystroke
    m_updatingRows = true;
    QThreadPool::globalInstance()->start([guard = m_guard, generation,
                                          entries, pattern = m_searchPattern, criteria = m_sortCriteria,
                                          order = m_sortOrder, locale = QLocale().collation(),
                                          seed = std::move(seed)]() mutable {
        QVector<int> rows = seed ? std::move(*seed) : LocalFolderOrder::matchingRows(entries, pattern);
        LocalFolderOrder::sortRows(rows, entries, criteria, order, locale);
        std::lock_guard<std::mutex> lock(guard->lock);
        if (LocalFolderModel* model = guard->model)
            QMetaObject::invokeMethod(model, [model, generation, rows = std::move(rows)]() mutable {
                model->m_updatingRows = false;
                if (generation == model->m_rowGeneration)
                    model->applyRows(std::move(rows));
                else if (generation >= model->m_refilterGeneration)
                    // Enrichment sorts the filtered order, including entries tied by name
                    model->startRowUpdate(std::move(rows));
                else
                    model->startRowUpdate();
            }, Qt::QueuedConnection);
    });
}

void LocalFolderModel::applyRows(QVector<int> rows)
{
    const bool newListing = m_pendingEntries.has_value();
    if (!newListing && rows == m_rows)
    {
        notifyLibraryDataChanged();
        return;
    }

    if (!newListing && rows.size() == m_rows.size())
    {
        QVector<int> positions(m_entries.size(), -1);
        for (int i = 0; i < rows.size(); ++i)
            positions[rows[i]] = i;
        const auto stillShown = [&positions](int entry) { return positions.at(entry) >= 0; };
        if (std::all_of(m_rows.cbegin(), m_rows.cend(), stillShown))
        {
            emit layoutAboutToBeChanged({}, VerticalSortHint);
            const QModelIndexList oldIndexes = persistentIndexList();
            QModelIndexList newIndexes;
            newIndexes.reserve(oldIndexes.size());
            const QVector<int> oldRows = std::exchange(m_rows, std::move(rows));
            for (const QModelIndex& oldIndex : oldIndexes)
                newIndexes.push_back(index(positions.at(oldRows.at(oldIndex.row())), oldIndex.column()));
            changePersistentIndexList(oldIndexes, newIndexes);
            emit layoutChanged({}, VerticalSortHint);
            notifyLibraryDataChanged();
            return;
        }
    }

    beginResetModel();
    if (newListing)
    {
        m_entries = std::move(*m_pendingEntries);
        m_pendingEntries.reset();
    }
    m_rows = std::move(rows);
    m_libraryDataChanged = false;
    endResetModel();
    // Some bindings also use this signal when the entries change at the same count
    emit countChanged();
    if (newListing)
        loadLibraryData();
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
    if (!m_ml || m_pendingEntries || m_entries.isEmpty())
        return;

    // The MRLs the library stores (vlc_path2uri) of the media of the folder,
    // all of them: the search only hides rows
    std::vector<std::pair<int, QString>> mrls;
    const auto& entries = std::as_const(m_entries);
    for (int i = 0; i < entries.size(); ++i)
    {
        if (entries[i].isFolder || entries[i].mrl.isEmpty())
            continue;
        mrls.emplace_back(i, entries[i].mrl);
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
        QString title;
        QString artist;
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
            if (!media)
                continue;
            // a song: its title and its artist
            QString title, artist;
            if (media->i_type == VLC_ML_MEDIA_TYPE_AUDIO
             && media->i_subtype == VLC_ML_MEDIA_SUBTYPE_ALBUMTRACK)
            {
                title = qfu(media->psz_title);
                if (media->album_track.i_artist_id > 0)
                {
                    ml_unique_ptr<vlc_ml_artist_t> found{ vlc_ml_get_artist(ml, media->album_track.i_artist_id) };
                    if (found)
                        artist = qfu(found->psz_name);
                }
            }
            ctx.results.push_back({ entry.first, entry.second, media->f_progress,
                                    media->i_playcount, media->i_id, media->b_is_favorite,
                                    title, artist });
        }
    },
    // UI thread
    [this, generation](quint64, Ctx& ctx)
    {
        if (generation != m_generation || ctx.results.empty())
            return;
        for (const auto& result : ctx.results)
        {
            if (result.entry >= m_entries.size() || m_entries.at(result.entry).mrl != result.mrl)
                continue;
            Entry& entry = m_entries[result.entry];
            entry.progress = result.progress > 0. ? result.progress : 0.;
            entry.playCount = result.playCount;
            entry.mlId = result.id;
            entry.favorite = result.favorite;
            entry.title = result.title;
            entry.artist = result.artist;
        }
        m_libraryDataChanged = true;
        if (m_sortCriteria == QLatin1String("unwatched"))
            updateRows(false);
        else
            notifyLibraryDataChanged();
    });
}

void LocalFolderModel::notifyLibraryDataChanged()
{
    if (std::exchange(m_libraryDataChanged, false) && !m_rows.isEmpty())
        emit dataChanged(index(0), index(m_rows.size() - 1),
                         { ProgressRole, PlayCountRole, IsFavoriteRole, IsIndexedRole,
                           TitleRole, ArtistRole });
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
    case TitleRole:
        return entry.title;
    case ArtistRole:
        return entry.artist;
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
        { TitleRole, "title" },
        { ArtistRole, "main_artist" },
    };
}
