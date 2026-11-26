/*****************************************************************************
 * Copyright (C) 2019 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * ( at your option ) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#include "medialib.hpp"
#include "mlhelper.hpp"
#include "playlist/playlist_controller.hpp"
#include "util/shared_input_item.hpp"
#include "dialogs/dialogs_provider.hpp"
// (the kinds of a song's file, as the library module's parser reads them)
#include "../../../misc/medialibrary/cover_track.h"

#include <algorithm>
#include <random>
#include <utility>

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QQmlEngine>
#include <QSet>
#include <QSettings>
#include <QTimer>
#include <QUrl>

#include <vlc_url.h>
#include <vlc_interface.h>
#include <vlc_playlist.h>

#ifdef Q_OS_LINUX
#include <cerrno>
#include <cstring>
#include <optional>
#include <sys/stat.h>
#include <sys/vfs.h>

#include <QFile>
#include <QHash>
#include <QStorageInfo>

#include <vlc_fs.h>

namespace {

constexpr char missingFilesQueue[] = "missingFiles";

struct FileVolume
{
    QStorageInfo storage;
    QByteArray filesystemId;

    bool matches(const FileVolume& other) const
    {
        if (filesystemId != other.filesystemId)
            return false;
        return storage == other.storage && storage.subvolume() == other.storage.subvolume();
    }
};

std::optional<FileVolume> fileVolume(const QString& path)
{
    FileVolume volume;
    volume.storage = QStorageInfo(path);
    if (!volume.storage.isValid() || !volume.storage.isReady()
        || volume.storage.rootPath().isEmpty() || volume.storage.device().isEmpty())
        return std::nullopt;

    // Device paths can be reused when a different disk is plugged in.
    struct statfs fs;
    const decltype(fs.f_fsid) emptyId{};
    if (statfs(QFile::encodeName(path).constData(), &fs) != 0
        || std::memcmp(&fs.f_fsid, &emptyId, sizeof(emptyId)) == 0)
        return std::nullopt;
    volume.filesystemId = QByteArray(reinterpret_cast<const char*>(&fs.f_fsid), sizeof(fs.f_fsid));
    return volume;
}

} // namespace

struct MediaLib::MissingFiles
{
    bool localFileGone(const QString& mrl, bool network);
    QHash<QString, FileVolume> volumes;
};
#endif

MediaLib::MediaLib(qt_intf_t *_intf, vlc::playlist::PlaylistController* playlistController, QObject *_parent)
    : QObject( _parent )
#ifdef Q_OS_LINUX
    , m_missingFiles(std::make_shared<MissingFiles>())
#endif
    , m_intf( _intf )
    , m_playlistController(playlistController)
    , m_ml( vlc_ml_instance_get( _intf ) )
    , m_event_cb( nullptr, [this](vlc_ml_event_callback_t* cb ) {
        vlc_ml_event_unregister_callback( m_ml, cb );
      })
{
    m_event_cb.reset( vlc_ml_event_register_callback( m_ml, MediaLib::onMediaLibraryEvent,
                                                      this ) );
    m_runner = new ThreadRunner();
    reparseSongsListedAsVideos();

    // deleted files out of the history and the queue: once the library has
    // started, then each time VLC comes to the front (at most once a minute)
    QTimer::singleShot(8000, this, &MediaLib::checkMissing);
    connect(qGuiApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState state) {
        if (state == Qt::ApplicationActive)
            checkMissing();
    });
}

// A missing local file is considered deleted or moved if its parent still
// exists. On Windows, an available drive also suffices if the folder is gone.
// On Linux, a mount-point directory can survive an unmount, so this session
// must first see the file present and the parent must still be on that volume.
// Network shares are only checked after playback fails.
#ifdef Q_OS_LINUX
bool MediaLib::MissingFiles::localFileGone(const QString& mrl, bool network)
#else
static bool localFileGone(const QString& mrl, bool network)
#endif
{
    const QUrl url(mrl);
    if (!url.isLocalFile())
        return false;
    const QString path = url.toLocalFile();
    if (path.isEmpty())
        return false;
    if (!network && (path.startsWith(QLatin1String("//")) || path.startsWith(QLatin1String("\\\\"))))
        return false;
#ifdef Q_OS_LINUX
    const QByteArray nativePath = QFile::encodeName(path);
    struct stat st;
    if (vlc_stat(nativePath.constData(), &st) == 0)
    {
        if (!volumes.contains(mrl))
        {
            const auto volume = fileVolume(path);
            if (volume && vlc_stat(nativePath.constData(), &st) == 0)
                volumes.insert(mrl, *volume);
        }
        return false;
    }
    if (errno != ENOENT)
        return false;

    const auto original = volumes.constFind(mrl);
    if (original == volumes.cend())
        return false;
    const QFileInfo info(path);
    const QDir parent = info.dir();
    if (info.isSymLink() || !parent.exists())
        return false;
    const auto current = fileVolume(parent.absolutePath());
    return current && original->matches(*current)
        && vlc_stat(nativePath.constData(), &st) != 0 && errno == ENOENT;
#else
    const QFileInfo info(path);
    if (info.exists())
        return false;
    if (info.dir().exists())
        return true;
#ifdef _WIN32
    const QString root = path.left(3);
    return root.size() == 3 && root.at(1) == QLatin1Char(':') && QDir(root).exists();
#else
    return false;
#endif
#endif
}

namespace {

// The library forgets a media whose file is gone: out of the history
// (Continue Watching, Recent), and the library's folder that held it read
// again (the library takes out the files it no longer finds there)
void forgetInLibrary(vlc_medialibrary_t* ml, const QString& mrl, QSet<QString>& reload)
{
    ml_unique_ptr<vlc_ml_media_t> media{ vlc_ml_get_media_by_mrl(ml, qtu(mrl)) };
    if (!media)
        return;
    vlc_ml_media_set_played(ml, media->i_id, false);
    bool inLibrary = false;
    if (media->p_files)
        for (size_t i = 0; i < media->p_files->i_nb_items; ++i)
        {
            const vlc_ml_file_t& file = media->p_files->p_items[i];
            if (file.i_type == VLC_ML_FILE_TYPE_MAIN && !file.b_external)
                inLibrary = true;
        }
    if (!inLibrary)
        return;
    vlc_ml_query_params_t params = vlc_ml_query_params_create();
    ml_unique_ptr<vlc_ml_folder_list_t> points{ vlc_ml_list_entry_points(ml, &params) };
    if (!points)
        return;
    for (size_t i = 0; i < points->i_nb_items; ++i)
    {
        const char* point = points->p_items[i].psz_mrl;
        if (point && mrl.startsWith(QString::fromUtf8(point)))
            reload.insert(QString::fromUtf8(point));
    }
}

} // namespace

void MediaLib::forgetMissing(const QString& mrl)
{
    struct Ctx { bool gone = false; };
    runOnMLThread<Ctx>(this,
#ifdef Q_OS_LINUX
    [mrl, missing = m_missingFiles](vlc_medialibrary_t* ml, Ctx& ctx) {
        ctx.gone = missing->localFileGone(mrl, true);
#else
    [mrl](vlc_medialibrary_t* ml, Ctx& ctx) {
        ctx.gone = localFileGone(mrl, true);
#endif
        if (!ctx.gone)
            return;
        QSet<QString> reload;
        forgetInLibrary(ml, mrl, reload);
        for (const QString& point : std::as_const(reload))
            vlc_ml_reload_folder(ml, qtu(point));
    },
    [this, mrl](quint64, Ctx& ctx) {
        if (ctx.gone)
            removeFromQueue({ mrl });
#ifdef Q_OS_LINUX
    }, missingFilesQueue);
#else
    });
#endif
}

void MediaLib::checkMissing()
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (m_lastMissingCheck != 0 && now - m_lastMissingCheck < 60000)
        return;
    m_lastMissingCheck = now;
    // the queue's local files, read here (the queue is this thread's); looked
    // at on the library's thread with the history
    QStringList queue;
    vlc_playlist_t* playlist = vlc_intf_GetMainPlaylist(m_intf->intf);
    vlc_playlist_Lock(playlist);
    const size_t count = vlc_playlist_Count(playlist);
    for (size_t i = 0; i < count && i < 5000; ++i)
    {
        input_item_t* item = vlc_playlist_item_GetMedia(vlc_playlist_Get(playlist, i));
        char* uri = item ? input_item_GetURI(item) : nullptr;
        if (uri && strncmp(uri, "file://", 7) == 0)
            queue << QString::fromUtf8(uri);
        free(uri);
    }
    vlc_playlist_Unlock(playlist);

    struct Ctx { QStringList gone; };
    runOnMLThread<Ctx>(this,
#ifdef Q_OS_LINUX
    [queue, missing = m_missingFiles](vlc_medialibrary_t* ml, Ctx& ctx) {
        QSet<QString> seen(queue.cbegin(), queue.cend());
#else
    [queue](vlc_medialibrary_t* ml, Ctx& ctx) {
#endif
        QSet<QString> reload;
        vlc_ml_query_params_t params = vlc_ml_query_params_create();
        params.i_nbResults = 500;
        ml_unique_ptr<vlc_ml_media_list_t> history{ vlc_ml_list_history(ml, &params, VLC_ML_HISTORY_TYPE_LOCAL) };
        if (history)
            for (size_t i = 0; i < history->i_nb_items; ++i)
            {
                const vlc_ml_media_t& media = history->p_items[i];
                if (!media.p_files)
                    continue;
                for (size_t f = 0; f < media.p_files->i_nb_items; ++f)
                {
                    const vlc_ml_file_t& file = media.p_files->p_items[f];
                    // (a drive that is not plugged in: kept)
                    if (file.i_type != VLC_ML_FILE_TYPE_MAIN || !file.psz_mrl || (file.b_removable && !file.b_present))
                        continue;
                    const QString mrl = QString::fromUtf8(file.psz_mrl);
#ifdef Q_OS_LINUX
                    seen.insert(mrl);
                    if (missing->localFileGone(mrl, false))
#else
                    if (localFileGone(mrl, false))
#endif
                        forgetInLibrary(ml, mrl, reload);
                }
            }
        for (const QString& point : std::as_const(reload))
            vlc_ml_reload_folder(ml, qtu(point));
        for (const QString& mrl : queue)
#ifdef Q_OS_LINUX
            if (missing->localFileGone(mrl, false))
#else
            if (localFileGone(mrl, false))
#endif
                ctx.gone << mrl;
#ifdef Q_OS_LINUX
        missing->volumes.removeIf([&seen](auto it) { return !seen.contains(it.key()); });
#endif
    },
    [this](quint64, Ctx& ctx) {
        removeFromQueue(ctx.gone);
#ifdef Q_OS_LINUX
    }, missingFilesQueue);
#else
    });
#endif
}

void MediaLib::removeFromQueue(const QStringList& mrls)
{
    if (mrls.isEmpty())
        return;
    const QSet<QString> gone(mrls.cbegin(), mrls.cend());
    vlc_playlist_t* playlist = vlc_intf_GetMainPlaylist(m_intf->intf);
    vlc_playlist_Lock(playlist);
    for (size_t i = vlc_playlist_Count(playlist); i-- > 0;)
    {
        input_item_t* item = vlc_playlist_item_GetMedia(vlc_playlist_Get(playlist, i));
        char* uri = item ? input_item_GetURI(item) : nullptr;
        const bool remove = uri && gone.contains(QString::fromUtf8(uri));
        free(uri);
        if (remove)
            vlc_playlist_RemoveOne(playlist, i);
    }
    vlc_playlist_Unlock(playlist);
}

// VLC Redesign: songs whose picture is a video track were videos for the
// library before 0.6.2 (#81; its parser skips that track since,
// cover_track.h). The library reads a file again only when it changes: the
// songs it had then stayed videos, under Videos and Home's New Videos
// (#111). Once, each video whose file is a song's kind is read again: the
// library reads a media again when its type goes from Unknown to another
// (Media::setType), and then gives it the type of its tracks (a song: read
// in full, its album and artist). A real video stays one.
void MediaLib::reparseSongsListedAsVideos()
{
    static const char key[] = "redesign/ml-songs-as-videos-reread";
    QSettings* settings = m_intf->mainSettings;
    if (settings == nullptr || settings->value(key, false).toBool())
        return;
    settings->setValue(key, true);
    qt_intf_t* intf = m_intf;
    runOnMLThread(this,
    //ML thread
    [intf](vlc_medialibrary_t* ml) {
        vlc_ml_query_params_t query = vlc_ml_query_params_create();
        ml_unique_ptr<vlc_ml_media_list_t> list(vlc_ml_list_video_media(ml, &query));
        if (list == nullptr)
        {
            msg_Warn(intf, "songs listed as videos: the videos could not be listed");
            return;
        }
        std::vector<int64_t> ids;
        for (const vlc_ml_media_t& media : ml_range_iterate<vlc_ml_media_t>(list))
        {
            if (media.p_files == nullptr)
                continue;
            for (const vlc_ml_file_t& file : ml_range_iterate<vlc_ml_file_t>(media.p_files))
                if (file.i_type == VLC_ML_FILE_TYPE_MAIN && redesign_cover::isAudioName(file.psz_mrl))
                {
                    ids.push_back(media.i_id);
                    break;
                }
        }
        size_t failed = 0;
        for (const int64_t id : ids)
            if (vlc_ml_media_set_type(ml, id, VLC_ML_MEDIA_TYPE_UNKNOWN) != VLC_SUCCESS
                || vlc_ml_media_set_type(ml, id, VLC_ML_MEDIA_TYPE_VIDEO) != VLC_SUCCESS)
                ++failed;
        msg_Dbg(intf, "songs listed as videos: %zu of %zu videos read again (%zu failed)",
                ids.size(), list->i_nb_items, failed);
    });
}

MediaLib::~MediaLib()
{
    m_runner->destroy();
}

// The videos of a group (the episodes of a show), played or queued in their
// natural order: "Episode 2" before "Episode 10", as the group shows them (#108)
static void sortGroupNaturally(const MLItemId &itemId, std::vector<const vlc_ml_media_t *> &media)
{
    if (itemId.type != VLC_ML_PARENT_GROUP)
        return;
    std::stable_sort(media.begin(), media.end(), [](const vlc_ml_media_t *a, const vlc_ml_media_t *b) {
        return naturalCompare(qfu(a->psz_title), qfu(b->psz_title)) < 0;
    });
}

static bool convertMLItemToPlaylistMedias(vlc_medialibrary_t* ml, const MLItemId & itemId, const QStringList &options, QVector<vlc::playlist::Media>& medias)
{
    //invalid item
    if (itemId.id == 0)
        return false;

    if (itemId.type == VLC_ML_PARENT_UNKNOWN)
    {
        SharedInputItem item( vlc_ml_get_input_item( ml, itemId.id ), false );
        if (!item)
            return false;
        medias.push_back(vlc::playlist::Media(item.get(), options));
    }
    else
    {
        vlc_ml_query_params_t query = vlc_ml_query_params_create();
        ml_unique_ptr<vlc_ml_media_list_t> media_list(vlc_ml_list_media_of( ml, &query, itemId.type, itemId.id));
        if (media_list == nullptr)
            return false;
        if (media_list->i_nb_items == 0)
            return true;

        auto mediaRange = ml_range_iterate<vlc_ml_media_t>( media_list );
        std::vector<const vlc_ml_media_t *> ordered;
        for (const vlc_ml_media_t &m : mediaRange)
            ordered.push_back(&m);
        sortGroupNaturally(itemId, ordered);
        std::transform(ordered.begin(), ordered.end(), std::back_inserter(medias), [&](const vlc_ml_media_t *m) {
            SharedInputItem item(vlc_ml_get_input_item( ml, m->i_id ), false);
            return vlc::playlist::Media(item.get(), options);
        });
    }
    return true;
}

static void convertQUrlToPlaylistMedias(QUrl mrl, const QStringList& options, QVector<vlc::playlist::Media>& medias)
{
    vlc::playlist::Media media{ mrl.toString(QUrl::FullyEncoded), mrl.fileName(), options };
    medias.push_back(media);
}

static void convertQStringToPlaylistMedias(const QString &mrl, const QStringList& options, QVector<vlc::playlist::Media>& medias)
{
    vlc::playlist::Media media{ mrl, mrl, options };
    medias.push_back(media);
}


static bool convertQVariantListToPlaylistMedias(vlc_medialibrary_t* ml, QVariantList itemIdList, const QStringList& options, QVector<vlc::playlist::Media>& medias)
{
    bool resolved = !itemIdList.isEmpty();
    for (const QVariant& varValue: itemIdList)
    {
        if (varValue.canConvert<QUrl>())
        {
            auto mrl = varValue.value<QUrl>();
            convertQUrlToPlaylistMedias(mrl, options, medias);
        }
        else if (varValue.canConvert<QString>())
        {
            auto mrl = varValue.value<QString>();
            convertQStringToPlaylistMedias(mrl, options, medias);
        }
        else if (varValue.canConvert<MLItemId>())
        {
            MLItemId itemId = varValue.value<MLItemId>();
            if (!convertMLItemToPlaylistMedias(ml, itemId, options, medias))
                resolved = false;
        }
        else
            resolved = false;
    }
    return resolved;
}

MLItemId MediaLib::deserializeMlItemIdFromString(const QString& serialized_id) {
    return MLItemId::fromString(serialized_id);
}

QString MediaLib::mrlFromUrl(const QUrl& url)
{
    if (url.isLocalFile())
    {
        const QByteArray path = QDir::toNativeSeparators(url.toLocalFile()).toUtf8();
        if (char* mrl = vlc_path2uri(path.constData(), "file"))
        {
            const QString result = QString::fromUtf8(mrl);
            free(mrl);
            return result;
        }
    }
    return url.toString(QUrl::FullyEncoded);
}

void MediaLib::addToPlaylist(const QString& mrl, const QStringList &options)
{
    QVector<vlc::playlist::Media> medias;
    convertQStringToPlaylistMedias(mrl, options, medias);
    m_playlistController->append(medias, false);
}

void MediaLib::addToPlaylist(const QUrl& mrl, const QStringList &options)
{
    QVector<vlc::playlist::Media> medias;
    convertQUrlToPlaylistMedias(mrl, options, medias);
    m_playlistController->append(medias, false);
}

// A specific item has been asked to be added to the playlist
void MediaLib::addToPlaylist(const MLItemId & itemId, const QStringList &options)
{
    struct Context {
        QVector<vlc::playlist::Media> medias;
    };
    runOnMLThread<Context>(this,
    //ML thread
    [itemId, options]
    (vlc_medialibrary_t* ml, Context& ctx){
        convertMLItemToPlaylistMedias(ml, itemId, options, ctx.medias);
    },
    //UI thread
    [this](quint64, Context& ctx){
        if (!ctx.medias.empty())
            m_playlistController->append(ctx.medias, false);
    });
}

void MediaLib::addToPlaylist(const QVariantList& itemIdList, const QStringList &options)
{
    struct Context {
        QVector<vlc::playlist::Media> medias;
    };
    runOnMLThread<Context>(this,
    //ML thread
    [itemIdList, options]
    (vlc_medialibrary_t* ml, Context& ctx)
    {
        convertQVariantListToPlaylistMedias(ml, itemIdList, options, ctx.medias);
    },
    //UI thread
    [this](quint64, Context& ctx){
        if (!ctx.medias.empty())
            m_playlistController->append(ctx.medias, false);
    });
}

// A specific item has been asked to be played,
// so it's added to the playlist and played
void MediaLib::addAndPlay(const MLItemId & itemId, const QStringList &options )
{
    struct Context {
        QVector<vlc::playlist::Media> medias;
    };
    runOnMLThread<Context>(this,
    //ML thread
    [itemId, options]
    (vlc_medialibrary_t* ml, Context& ctx)
    {
        convertMLItemToPlaylistMedias(ml, itemId, options, ctx.medias);
    },
    //UI thread
    [this](quint64, Context& ctx){
        if (!ctx.medias.empty())
            m_playlistController->append(ctx.medias, true);
    });


}

void MediaLib::addAndPlay(const QString& mrl, const QStringList &options)
{
    vlc::playlist::Media media{ mrl, mrl, options };
    m_playlistController->append( QVector<vlc::playlist::Media>{media}, true );
}

void MediaLib::addAndPlay(const QUrl& mrl, const QStringList &options)
{
    vlc::playlist::Media media{ mrl.toString(QUrl::FullyEncoded), mrl.fileName(), options };
    m_playlistController->append( QVector<vlc::playlist::Media>{media}, true );
}


void MediaLib::addAndPlay(const QVariantList& itemIdList, const QStringList &options)
{
    struct Context {
        QVector<vlc::playlist::Media> medias;
    };
    runOnMLThread<Context>(this,
    //ML thread
    [itemIdList, options]
    (vlc_medialibrary_t* ml, Context& ctx)
    {
        convertQVariantListToPlaylistMedias(ml, itemIdList, options, ctx.medias);
    },
    //UI thread
    [this](quint64, Context& ctx){
        if (!ctx.medias.empty())
            m_playlistController->append(ctx.medias, true);
    });
}

void MediaLib::addAndPlayShuffled(const QVariantList& itemIdList, const QStringList &options)
{
    struct Context {
        QVector<vlc::playlist::Media> medias;
    };
    runOnMLThread<Context>(this,
    //ML thread
    [itemIdList, options]
    (vlc_medialibrary_t* ml, Context& ctx)
    {
        convertQVariantListToPlaylistMedias(ml, itemIdList, options, ctx.medias);
    },
    //UI thread
    [this](quint64, Context& ctx){
        if (ctx.medias.empty())
            return;
        std::shuffle(ctx.medias.begin(), ctx.medias.end(), std::mt19937(std::random_device{}()));
        m_playlistController->append(ctx.medias, true);
    });
}

void MediaLib::exportToFile(const QVariantList& itemIdList)
{
    struct Context {
        QVector<vlc::playlist::Media> medias;
        bool resolved = false;
    };
    runOnMLThread<Context>(this,
    //ML thread
    [itemIdList]
    (vlc_medialibrary_t* ml, Context& ctx)
    {
        ctx.resolved = convertQVariantListToPlaylistMedias(ml, itemIdList, {}, ctx.medias)
            && std::none_of(ctx.medias.cbegin(), ctx.medias.cend(),
                            [](const vlc::playlist::Media& media) { return !media.raw(); });
    },
    //UI thread
    [](quint64, Context& ctx){
        auto* dialogs = DialogsProvider::getInstance();
        if (!ctx.resolved)
        {
            dialogs->messageDialog(qtr("Could not load the playlist media for export."),
                                   qtr("Export Playlist"), QMessageBox::Ok, QMessageBox::Ok,
                                   QMessageBox::Warning);
            return;
        }
        dialogs->saveMediaToPlaylistFile(ctx.medias);
    });
}

void MediaLib::insertIntoPlaylist(const size_t index, const QVariantList &itemIdList, const QStringList &options)
{
    struct Context {
        QVector<vlc::playlist::Media> medias;
    };
    runOnMLThread<Context>(this,
    //ML thread
    [itemIdList, options]
    (vlc_medialibrary_t* ml, Context& ctx )
    {
        convertQVariantListToPlaylistMedias(ml, itemIdList, options, ctx.medias);
    },
    //UI thread
    [this, index]
    (quint64, Context& ctx) {
        if (!ctx.medias.isEmpty())
            m_playlistController->insert( index, ctx.medias );
    });
}

void MediaLib::reload()
{
    runOnMLThread(this,
    //ML thread
    [](vlc_medialibrary_t* ml){
        vlc_ml_reload_folder(ml, nullptr);
    });
}

void MediaLib::mlInputItem(const QVector<MLItemId>& itemIdVector, QJSValue callback)
{
    if (!callback.isCallable()) // invalid argument
    {
        msg_Warn(m_intf, "callback is not callbable");
        return;
    }

    struct Ctx {
        std::vector<SharedInputItem> items;
    };

    if (itemIdVector.empty())
    {
        //call the callback with and empty list
        auto jsEngine = qjsEngine(this);
        if (!jsEngine)
            return;
        auto jsArray = jsEngine->newArray(0);
        callback.call({jsArray});
        return;
    }

    auto it = m_inputItemQuery.find(itemIdVector);

    if (it == m_inputItemQuery.end())
    {
        it = m_inputItemQuery.insert(itemIdVector, {callback});
    }
    else
    {
        // be patient

        for (const auto& it2 : it.value())
        {
            if (callback.strictlyEquals(it2))
                return;
        }

        it.value().push_back(callback); // FIXME: Use an ordered set
        return;
    }

    runOnMLThread<Ctx>(this,
    //ML thread
    [itemIdVector](vlc_medialibrary_t* ml, Ctx& ctx){
        for (auto mlId : itemIdVector)
        {
            // NOTE: When we have a parent it's a collection of media(s).
            if (mlId.type == VLC_ML_PARENT_UNKNOWN)
            {
                ctx.items.emplace_back(vlc_ml_get_input_item(ml, mlId.id), false);
            }
            else
            {
                ml_unique_ptr<vlc_ml_media_list_t> list;

                vlc_ml_query_params_t query = vlc_ml_query_params_create();

                list.reset(vlc_ml_list_media_of(ml, &query, mlId.type, mlId.id));

                if (list == nullptr)
                    continue;

                std::vector<const vlc_ml_media_t *> ordered;
                for (const vlc_ml_media_t & media : ml_range_iterate<vlc_ml_media_t>(list))
                    ordered.push_back(&media);
                sortGroupNaturally(mlId, ordered);
                for (const vlc_ml_media_t *media : ordered)
                    ctx.items.emplace_back(vlc_ml_get_input_item(ml, media->i_id), false);
            }
        }
    },
    //UI thread
    [this, it](quint64, Ctx& ctx) mutable
    {
        auto jsEngine = qjsEngine(this);
        if (!jsEngine)
            return;

        auto jsArray = jsEngine->newArray(ctx.items.size());

        int i  = 0;
        for (const auto& inputItem : ctx.items)
        {
            jsArray.setProperty(i, jsEngine->toScriptValue(inputItem));
            i++;
        }

        for (const auto& cb : std::as_const(it.value()))
        {
            cb.call({jsArray});
        }

        m_inputItemQuery.erase(it);
    });
}

vlc_ml_event_callback_t* MediaLib::registerEventListener( void (*callback)(void*, const vlc_ml_event_t*), void* data)
{
    return vlc_ml_event_register_callback( m_ml , callback , data ) ;
}

void MediaLib::unregisterEventListener(vlc_ml_event_callback_t* cb)
{
    vlc_ml_event_unregister_callback( m_ml , cb );
}

void MediaLib::onMediaLibraryEvent( void* data, const vlc_ml_event_t* event )
{
    MediaLib* self = static_cast<MediaLib*>( data );
    switch ( event->i_type )
    {
        case VLC_ML_EVENT_PARSING_PROGRESS_UPDATED:
        {
            int percent =  event->parsing_progress.i_percent;
            QMetaObject::invokeMethod(self, [self, percent]() {
                self->m_parsingProgress = percent;
                self->emit parsingProgressChanged(percent);
            });
            break;
        }
        case VLC_ML_EVENT_DISCOVERY_STARTED:
        {
            QMetaObject::invokeMethod(self, [self]() {
                self->m_discoveryPending = true;
                self->emit discoveryPendingChanged(self->m_discoveryPending);
                self->emit discoveryStarted();
            });
            break;
        }
        case VLC_ML_EVENT_DISCOVERY_PROGRESS:
        {
            const QUrl entryPoint{ event->discovery_progress.psz_entry_point };
            const QString entryPointStr = entryPoint.isLocalFile()
                                              ? entryPoint.toLocalFile()
                                              : entryPoint.toDisplayString();

            QMetaObject::invokeMethod(self, [self, entryPointStr]() {
                self->m_discoveryEntryPoint = entryPointStr;
                self->emit discoveryEntryPointChanged(entryPointStr);
            });
            break;
        }
        case VLC_ML_EVENT_DISCOVERY_COMPLETED:
        {
            QMetaObject::invokeMethod(self, [self]() {
                self->m_discoveryPending = false;
                self->emit discoveryPendingChanged(self->m_discoveryPending);
                self->emit discoveryCompleted();
            });
            break;
        }
        case VLC_ML_EVENT_BACKGROUND_IDLE_CHANGED:
        {
            bool idle = event->background_idle_changed.b_idle;
            QMetaObject::invokeMethod(self, [self, idle]() {
                self->m_idle = idle;
                self->emit idleChanged();
            });
            break;
        }
        default:
            break;
    }
}

quint64 MediaLib::runOnMLThread(const QObject* obj,
                std::function< void(vlc_medialibrary_t* ml)> mlCb,
                std::function< void()> uiCb,
                const char* queue)
{
    struct NoCtx{};
    return m_runner->runOnThread<NoCtx>(obj,
    [mlCb, ml=m_ml](NoCtx&){
        mlCb(ml);
    },
    [uiCb](quint64, NoCtx&){
        uiCb();
    },
    queue);
}

quint64 MediaLib::runOnMLThread(const QObject* obj,
                std::function< void(vlc_medialibrary_t* ml)> mlCb,
                std::function< void(quint64)> uiCb, const char* queue)
{
    struct NoCtx{};
    return m_runner->runOnThread<NoCtx>(obj,
    [mlCb, ml=m_ml](NoCtx&){
        mlCb(ml);
    },
    [uiCb](quint64 requestId, NoCtx&){
        uiCb(requestId);
    },
    queue);
}

quint64 MediaLib::runOnMLThread(const QObject* obj,
                std::function< void(vlc_medialibrary_t* ml)> mlCb,
                const char* queue)
{
    struct NoCtx{};
    return m_runner->runOnThread<NoCtx>(obj,
    [mlCb, ml=m_ml](NoCtx&){
        mlCb(ml);
    },
    [](quint64, NoCtx&){
    },
    queue);
}

void MediaLib::cancelMLTask(const QObject* object, quint64 taskId)
{
    m_runner->cancelTask(object, taskId);
}

