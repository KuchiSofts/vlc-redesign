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

#ifndef ML_FOLDERS_MODEL_HPP
#define ML_FOLDERS_MODEL_HPP

#ifdef HAVE_CONFIG_H

# include "config.h"

#endif

#include "qt.hpp"
#include <QAbstractListModel>
#include <QTimer>
#include <QUrl>
#include "mlhelper.hpp"

#include <maininterface/mainctx.hpp>

extern "C" {
    struct vlc_medialibrary_t;
    typedef struct vlc_ml_folder_t vlc_ml_folder_t;
    typedef struct vlc_ml_event_t vlc_ml_event_t;
};

class MLFoldersBaseModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(MainCtx* ctx READ getCtx WRITE setCtx NOTIFY ctxChanged FINAL)
    // the entry points listed: all of them, or those holding videos (or
    // nothing yet), or songs (or nothing yet)
    Q_PROPERTY(Kind kind READ getKind WRITE setKind NOTIFY kindChanged FINAL)

public:
    enum Roles
    {
        Banned = Qt::UserRole + 1,
        DisplayUrl,
        MRL,
        HasVideo,
        HasAudio
    };

    enum Kind
    {
        AllFolders,
        VideoFolders,
        AudioFolders
    };
    Q_ENUM(Kind)

    enum Operation
    {
        Add,
        Remove,
        Ban,
        Unban
    };

    MLFoldersBaseModel( QObject *parent = nullptr );
    virtual ~MLFoldersBaseModel() = default;

    void setCtx(MainCtx* ctx);
    inline MainCtx* getCtx() { return m_ctx; }
    void setKind(Kind kind);
    inline Kind getKind() const { return m_kind; }

    int rowCount( QModelIndex const &parent = {} ) const  override;
    QVariant data( QModelIndex const &index , const int role = Qt::DisplayRole ) const  override;
    QHash<int, QByteArray> roleNames() const override;

public slots:
    virtual void remove( const QUrl &mrl ) = 0;
    virtual void add( const QUrl &mrl ) = 0;
    virtual void reload( const int row );

    void removeAt( int index );

signals:
    void ctxChanged();
    void kindChanged();
    void operationFailed( int op, QUrl url ) const;
    void onMLEntryPointModified(QPrivateSignal);
    void onMLFolderModified(QPrivateSignal);

protected:
    struct EntryPoint
    {
        EntryPoint(const vlc_ml_folder_t &entryPoint );
        QString mrl;
        bool banned;
        // it holds videos, songs (in any of its folders)
        bool hasVideo = false;
        bool hasAudio = false;
    };

    virtual bool failed( const vlc_ml_event_t* event ) const = 0; // will be called outside the main thread
    // what the entry points hold is read (has_video, has_audio, kind), and
    // read again as the library changes; called outside the main thread too
    virtual bool readsContent() const { return false; }

protected:
    static void onMlEvent( void* data , const vlc_ml_event_t* event );
    virtual void update() = 0;
    void updateImpl(vlc_ml_folder_list_t* (*folderListFunc)( vlc_medialibrary_t* p_ml, const vlc_ml_query_params_t* params),
                    bool withContent = false);

    using EventCallbackPtr = std::unique_ptr<vlc_ml_event_callback_t, std::function<void( vlc_ml_event_callback_t* )>>;

    std::vector<EntryPoint> m_mrls;
    MediaLib *m_mediaLib = nullptr;
    MainCtx* m_ctx = nullptr;
    EventCallbackPtr m_ml_event_handle;
    bool m_updatePending = false;
    // asked again while loading: loaded again after
    bool m_updateRequested = false;
    Kind m_kind = AllFolders;
    // what the entry points hold changes while the library indexes: read
    // again once it settles
    QTimer m_contentTimer;
};

class MLFoldersModel : public MLFoldersBaseModel
{
public:
    using MLFoldersBaseModel::MLFoldersBaseModel;
    ~MLFoldersModel() override;

    void remove( const QUrl &mrl ) override;
    void add( const QUrl &mrl ) override;

private:
    void update() override final;
    bool failed( const vlc_ml_event_t* event ) const override;
    bool readsContent() const override { return true; }
};

class MLBannedFoldersModel : public MLFoldersBaseModel
{
public:
    using MLFoldersBaseModel::MLFoldersBaseModel;
    ~MLBannedFoldersModel() override;

    void remove( const QUrl &mrl ) override;
    void add( const QUrl &mrl ) override;

private:
    void update() override final;
    bool failed( const vlc_ml_event_t* event ) const override;
};

#endif // ML_FOLDERS_MODEL_HPP
