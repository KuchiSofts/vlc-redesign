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

#include "mlfoldersmodel.hpp"
#include "medialibrary/medialib.hpp"
#include <algorithm>
#include <cassert>

static const char* const ML_FOLDER_ADD_QUEUE = "ML_FOLDER_ADD_QUEUE";
static const char* const ML_FOLDER_BAN_QUEUE = "ML_FOLDER_BAN_QUEUE";

MLFoldersBaseModel::MLFoldersBaseModel( QObject *parent )
    : QAbstractListModel( parent )
    , m_ml_event_handle( nullptr , [this](vlc_ml_event_callback_t* cb ) {
        if (!cb)
            return;
        if ( m_mediaLib )
            m_mediaLib->unregisterEventListener(cb);
    })
{
    connect( this , &MLFoldersBaseModel::onMLEntryPointModified , this , &MLFoldersBaseModel::update );
    m_contentTimer.setSingleShot( true );
    m_contentTimer.setInterval( 1000 );
    connect( &m_contentTimer, &QTimer::timeout, this, &MLFoldersBaseModel::update );
    connect( this, &MLFoldersBaseModel::onMLFolderModified,
             &m_contentTimer, qOverload<>(&QTimer::start) );
}

MLFoldersBaseModel::EntryPoint::EntryPoint( const vlc_ml_folder_t& entryPoint)
    : mrl(entryPoint.psz_mrl)
    , banned(entryPoint.b_banned)
{
}

void MLFoldersBaseModel::setCtx(MainCtx* ctx)
{
    if (ctx)
    {
        m_ctx = ctx;
        m_mediaLib = m_ctx->getMediaLibrary();
        m_ml_event_handle.reset(m_mediaLib->registerEventListener(onMlEvent, this));
    }
    else
    {
        m_ctx = nullptr;
        //reset *before* setting m_mediaLib to nullptr
        m_ml_event_handle.reset( nullptr );
        m_mediaLib = nullptr;
        beginResetModel();
        m_mrls.clear();
        endResetModel();
    }
    update();
    emit ctxChanged();
}

void MLFoldersBaseModel::setKind(Kind kind)
{
    if (m_kind == kind)
        return;
    m_kind = kind;
    update();
    emit kindChanged();
}

int MLFoldersBaseModel::rowCount( QModelIndex const & ) const
{
    return static_cast<int>(m_mrls.size());
}

QVariant MLFoldersBaseModel::data( const QModelIndex &index ,
                              int role) const {
    if ( !index.isValid() )
        return {};

    switch ( role )
    {
        case Banned:
            return m_mrls[index.row()].banned;
        case MRL:
            return m_mrls[index.row()].mrl;
        case HasVideo:
            return m_mrls[index.row()].hasVideo;
        case HasAudio:
            return m_mrls[index.row()].hasAudio;
        case Qt::DisplayRole:
        case DisplayUrl:
        {
            QUrl url = QUrl::fromUserInput(m_mrls[index.row()].mrl);
            if (!url.isValid())
                return {};
            return QVariant::fromValue(urlToDisplayString(url));
        }
        default :
            return {};
    }
}

QHash<int, QByteArray> MLFoldersBaseModel::roleNames() const
{
    return {
        {DisplayUrl, QByteArrayLiteral("display_url")},
        {Banned, QByteArrayLiteral("banned")},
        {MRL, QByteArrayLiteral("url")},
        {HasVideo, QByteArrayLiteral("has_video")},
        {HasAudio, QByteArrayLiteral("has_audio")}
    };
}

void MLFoldersBaseModel::reload(const int row)
{
    const QString mrl = m_mrls.at(row).mrl;
    m_mediaLib->runOnMLThread(this, [mrl](vlc_medialibrary_t *ml)
    {
        vlc_ml_reload_folder( ml, qtu(mrl) );
    });
}

void MLFoldersBaseModel::removeAt(int index)
{
    assert(index < rowCount());
    const QModelIndex idx = this->index( index, 0 );
    if (idx.isValid())
        remove( data( idx,  MRL ).toUrl() );

}

void MLFoldersBaseModel::onMlEvent( void* data , const vlc_ml_event_t* event )
{
    auto self = static_cast<MLFoldersBaseModel *>( data );
    if ( event->i_type == VLC_ML_EVENT_ENTRY_POINT_ADDED || event->i_type == VLC_ML_EVENT_ENTRY_POINT_REMOVED ||
         event->i_type == VLC_ML_EVENT_ENTRY_POINT_UNBANNED || event->i_type == VLC_ML_EVENT_ENTRY_POINT_BANNED  )
    {
        if (!self->failed( event ))
            emit self->onMLEntryPointModified( QPrivateSignal() );
    }
    else if ( event->i_type == VLC_ML_EVENT_FOLDER_ADDED || event->i_type == VLC_ML_EVENT_FOLDER_UPDATED )
        emit self->onMLFolderModified( QPrivateSignal() );
}

void MLFoldersBaseModel::updateImpl(vlc_ml_folder_list_t* (*folderListFunc)( vlc_medialibrary_t* p_ml, const vlc_ml_query_params_t* params ),
                                     bool withContent)
{
    if (!m_mediaLib)
        return;
    if (m_updatePending)
    {
        m_updateRequested = true;
        return;
    }

    m_updatePending = true;
    struct Ctx {
        std::vector<MLFoldersBaseModel::EntryPoint> r;
    };

    m_mediaLib->runOnMLThread<Ctx>(this,
    //ML thread
    [folderListFunc, withContent](vlc_medialibrary_t* ml,  Ctx& ctx){
        vlc_ml_folder_list_t* entrypoints = folderListFunc( ml, nullptr );
        if ( entrypoints != nullptr )
        {
            for ( unsigned int i = 0; entrypoints && i < entrypoints->i_nb_items; i++ )
                ctx.r.emplace_back( entrypoints->p_items[i] );
            vlc_ml_release( entrypoints );
        }
        if ( !withContent || ctx.r.empty() )
            return;
        // the folders holding videos, songs (at any depth): the entry
        // points they are in hold them
        const auto mark = [&ctx]( vlc_ml_folder_list_t* folders, bool EntryPoint::*holds ) {
            if ( folders == nullptr )
                return;
            for ( unsigned int i = 0; i < folders->i_nb_items; i++ )
            {
                if ( folders->p_items[i].psz_mrl == nullptr )
                    continue;
                const QString folder = QString::fromUtf8( folders->p_items[i].psz_mrl );
                for ( EntryPoint& entryPoint : ctx.r )
                {
                    const QString& root = entryPoint.mrl;
                    if ( !root.isEmpty() && folder.startsWith( root )
                         && ( root.endsWith( '/' ) || folder.size() == root.size() || folder.at( root.size() ) == '/' ) )
                        entryPoint.*holds = true;
                }
            }
            vlc_ml_release( folders );
        };
        mark( vlc_ml_list_folders_by_type( ml, nullptr, VLC_ML_MEDIA_TYPE_VIDEO ), &EntryPoint::hasVideo );
        mark( vlc_ml_list_folders_by_type( ml, nullptr, VLC_ML_MEDIA_TYPE_AUDIO ), &EntryPoint::hasAudio );
    },
    //UI thread
    [this](quint64, Ctx& ctx)
    {
        m_updatePending = false;
        if ( m_kind != AllFolders )
        {
            // an entry point holding nothing yet is in both lists
            const bool videos = m_kind == VideoFolders;
            ctx.r.erase( std::remove_if( ctx.r.begin(), ctx.r.end(), [videos]( const EntryPoint& e ) {
                return videos ? ( !e.hasVideo && e.hasAudio ) : ( !e.hasAudio && e.hasVideo );
            } ), ctx.r.end() );
        }
        // the same entry points: only what they hold changed (the views
        // are not reset while the library indexes)
        const bool same = std::equal( ctx.r.cbegin(), ctx.r.cend(), m_mrls.cbegin(), m_mrls.cend(),
                                      []( const EntryPoint& a, const EntryPoint& b ) {
            return a.mrl == b.mrl && a.banned == b.banned;
        } );
        if ( same )
        {
            m_mrls = std::move(ctx.r);
            if ( !m_mrls.empty() )
                emit dataChanged( index( 0 ), index( rowCount() - 1 ), { HasVideo, HasAudio } );
        }
        else
        {
            beginResetModel();
            m_mrls = std::move(ctx.r);
            endResetModel();
        }
        if ( m_updateRequested )
        {
            m_updateRequested = false;
            update();
        }
    });
}

void MLFoldersModel::update()
{
    updateImpl(vlc_ml_list_entry_points, true);
}


bool MLFoldersModel::failed(const vlc_ml_event_t *event) const
{
    if ( event->i_type == VLC_ML_EVENT_ENTRY_POINT_ADDED && !event->entry_point_added.b_success )
    {
        emit operationFailed( Add, QUrl::fromEncoded( event->entry_point_added.psz_entry_point ) );
        return true;
    }
    else if ( event->i_type == VLC_ML_EVENT_ENTRY_POINT_REMOVED && !event->entry_point_removed.b_success )
    {
        emit operationFailed( Remove, QUrl::fromEncoded( event->entry_point_removed.psz_entry_point ) );
        return true;
    }

    return false;
}

void MLFoldersModel::remove( const QUrl &mrl )
{
    //UI data is updated on onMlEvent
    m_mediaLib->runOnMLThread(this,
    //ML thread
    [mrl](vlc_medialibrary_t* ml){
        vlc_ml_remove_folder( ml , qtu( mrl.toString( QUrl::FullyEncoded ) ) );
    },
    ML_FOLDER_ADD_QUEUE);
}

void MLFoldersModel::add(const QUrl &mrl )
{
    //UI data is updated on onMlEvent
    m_mediaLib->runOnMLThread(this,
    //ML thread
    [mrl](vlc_medialibrary_t* ml){
        vlc_ml_add_folder( ml, qtu( mrl.toString( QUrl::FullyEncoded ) ) );
    },
    ML_FOLDER_ADD_QUEUE);
}

void MLBannedFoldersModel::remove(const QUrl &mrl)
{
    //UI data is updated on onMlEvent
    m_mediaLib->runOnMLThread(this,
    //ML thread
    [mrl](vlc_medialibrary_t* ml){
        vlc_ml_unban_folder( ml , qtu( mrl.toString( QUrl::FullyEncoded ) ) );
    },
    ML_FOLDER_BAN_QUEUE);
}

void MLBannedFoldersModel::add(const QUrl &mrl)
{
    //UI data is updated on onMlEvent
    m_mediaLib->runOnMLThread(this,
    //ML thread
    [mrl](vlc_medialibrary_t* ml){
        vlc_ml_ban_folder( ml , qtu( mrl.toString( QUrl::FullyEncoded ) ) );
    },
    ML_FOLDER_BAN_QUEUE);
}

void MLBannedFoldersModel::update()
{
    updateImpl(vlc_ml_list_banned_entry_points);
}

bool MLBannedFoldersModel::failed(const vlc_ml_event_t *event) const
{
    if ( event->i_type == VLC_ML_EVENT_ENTRY_POINT_BANNED && !event->entry_point_banned.b_success )
    {
        emit operationFailed( Ban, QUrl::fromEncoded( event->entry_point_banned.psz_entry_point ) );
        return true;
    }
    else if ( event->i_type == VLC_ML_EVENT_ENTRY_POINT_UNBANNED && !event->entry_point_unbanned.b_success )
    {
        emit operationFailed( Unban, QUrl::fromEncoded( event->entry_point_unbanned.psz_entry_point ) );
        return true;
    }

    return false;
}
