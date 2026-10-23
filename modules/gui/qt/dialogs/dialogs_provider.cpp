/*****************************************************************************
 * dialogs_provider.cpp : Dialog Provider
 *****************************************************************************
 * Copyright (C) 2006-2009 the VideoLAN team
 *
 * Authors: Clément Stenac <zorglub@videolan.org>
 *          Jean-Baptiste Kempf <jb@videolan.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_interface.h>
#include <vlc_intf_strings.h>
#include <vlc_preparser.h>

#include "qt.hpp"
#include "dialogs_provider.hpp"
#include "player/player_controller.hpp" /* Load Subtitles */
#include "playlist/playlist_controller.hpp"
#include "menus/menus.hpp"
#include "util/qt_dirs.hpp"
#include "util/vlchotkeyconverter.hpp"
#include "maininterface/mainctx.hpp"

/* The dialogs */
#include "dialogs/bookmarks/bookmarks.hpp"
#include "dialogs/preferences/preferences.hpp"
#include "dialogs/mediainfo/mediainfo.hpp"
#include "dialogs/messages/messages.hpp"
#include "dialogs/extended/extended.hpp"
#include "dialogs/vlm/vlm.hpp"
#include "dialogs/sout/sout.hpp"
#include "dialogs/sout/convert.hpp"
#include "dialogs/open/open.hpp"
#include "dialogs/open/openurl.hpp"
#include "dialogs/help/help.hpp"
#include "dialogs/gototime/gototime.hpp"
#include "dialogs/podcast/podcast_configuration.hpp"
#include "dialogs/plugins/plugins.hpp"
#include "dialogs/epg/epg.hpp"
#include "dialogs/playlists/playlists.hpp"
#include "dialogs/firstrun/firstrunwizard.hpp"

#include <vlc_url.h>
#include <vlc_fs.h>
#include <vlc_charset.h>
#include <vlc_strings.h>

#include <cerrno>
#include <vector>

#include <QEvent>
#include <QApplication>
#include <QSignalMapper>
#include <QFileDialog>
#include <QUrl>
#include <QInputDialog>
#include <QPointer>
#include <QMessageBox>
#include <QRegularExpression>
#include <QThreadPool>

#define I_OP_DIR_WINTITLE I_DIR_OR_FOLDER( N_("Open Directory"), \
                                           N_("Open Folder") )

#define BASE_DIALOG(X) \
    X(prefsDialog) \
    X(openDialog) \
    X(firstRunDialog) \
    X(extendedDialog) \
    X(messagesDialog) \
    X(gotoTimeDialog) \
    X(vlmDialog) \
    X(helpDialog) \
    X(aboutDialog) \
    X(mediaInfoDialog) \
    X(bookmarkDialog) \
    X(podcastDialog) \
    X(pluginDialog)\
    X(egpDialog)


#ifdef UPDATE_CHECK
#define ALL_DIALOG(X) \
    BASE_DIALOG(X) \
    X(updateDialog)
#else
#define ALL_DIALOG(X) BASE_DIALOG(X)
#endif

DialogsProvider::DialogsProvider( qt_intf_t *_p_intf )
    : QObject( NULL ), p_intf( _p_intf ), m_parser( nullptr, &input_item_parser_id_Release )

{
    b_isDying = false;
}

DialogsProvider::~DialogsProvider()
{
    /* free parentless menus  */
    VLCMenuBar::freeRendererMenu();
}

void DialogsProvider::quit()
{
    if (b_isDying)
        return;
    b_isDying = true;
    assert(qApp);
    qApp->setProperty("isDying", true);
    libvlc_Quit( vlc_object_instance(p_intf) );
}

void DialogsProvider::customEvent( QEvent *event )
{
    if( event->type() == DialogEvent::DialogEvent_Type )
    {
        DialogEvent *de = static_cast<DialogEvent*>(event);
        switch( de->i_dialog )
        {
        case INTF_DIALOG_FILE_SIMPLE:
        case INTF_DIALOG_FILE:
            openDialog(); break;
        case INTF_DIALOG_FILE_GENERIC:
            openFileGenericDialog( de->p_arg ); break;
        case INTF_DIALOG_DISC:
            openDiscDialog(); break;
        case INTF_DIALOG_NET:
            openNetDialog(); break;
        case INTF_DIALOG_SAT:
        case INTF_DIALOG_CAPTURE:
            openCaptureDialog(); break;
        case INTF_DIALOG_DIRECTORY:
            PLAppendDir(); break;
        case INTF_DIALOG_PLAYLIST:
            //FIXME
            //playlistDialog(); break;
            break;
        // case INTF_DIALOG_PLAYLISTS:
        // FIXME: `intf_dialog_args_t` is not supported by `PlaylistsDialog`.
        case INTF_DIALOG_MESSAGES:
            messagesDialog(); break;
        case INTF_DIALOG_FILEINFO:
           mediaInfoDialog(); break;
        case INTF_DIALOG_PREFS:
           prefsDialog(); break;
        case INTF_DIALOG_BOOKMARKS:
           bookmarksDialog(); break;
        case INTF_DIALOG_EXTENDED:
           extendedDialog(); break;
        case INTF_DIALOG_SENDKEY:
           sendKey( de->i_arg ); break;
#ifdef ENABLE_VLM
        case INTF_DIALOG_VLM:
           vlmDialog(); break;
#endif
        case INTF_DIALOG_POPUPMENU:
        {
           popupMenu.reset();
           bool show = (de->i_arg != 0);
           if( show )
           {
              //popping a QMenu prevents mouse release events to be received,
              //this ensures the coherency of the vout mouse state.
              emit releaseMouseEvents();
              popupMenu.reset(VLCMenuBar::PopupMenu( p_intf, true ));
           }
           break;
        }
        case INTF_DIALOG_AUDIOPOPUPMENU:
        {
           audioPopupMenu.reset();
           bool show = (de->i_arg != 0);
           if( show )
               audioPopupMenu.reset(VLCMenuBar::AudioPopupMenu( p_intf, show ));
           break;
        }
        case INTF_DIALOG_VIDEOPOPUPMENU:
        {
           videoPopupMenu.reset();
           bool show = (de->i_arg != 0);
           if( show )
               videoPopupMenu.reset(VLCMenuBar::VideoPopupMenu( p_intf, show ));
           break;
        }
        case INTF_DIALOG_MISCPOPUPMENU:
        {
           miscPopupMenu.reset();
           bool show = (de->i_arg != 0);
           if( show )
               miscPopupMenu.reset(VLCMenuBar::MiscPopupMenu( p_intf, show ));
           break;
        }
        case INTF_DIALOG_WIZARD:
        case INTF_DIALOG_STREAMWIZARD:
            openAndStreamingDialogs(); break;
#ifdef UPDATE_CHECK
        case INTF_DIALOG_UPDATEVLC:
            updateDialog(); break;
#endif
        case INTF_DIALOG_EXIT:
            quit(); break;
        default:
           msg_Warn( p_intf, "unimplemented dialog" );
        }
    }
}

bool DialogsProvider::eventFilter(QObject *watched, QEvent *event) {
    assert(event);
    assert(watched);

#define CASE_DIALOG(dialog) \
    if (watched == m_ ## dialog .get()) { \
        emit dialog ## VisibleChanged(); \
        break;\
    }

    switch (event->type()) {
    case QEvent::Hide:
    case QEvent::Show:
    case QEvent::Close:
    case QEvent::Destroy:
    {
        ALL_DIALOG(CASE_DIALOG)
        break;
    }
    default:
        break;
    }
    return false;
}


/****************************************************************************
 * Individual simple dialogs
 ****************************************************************************/
const QEvent::Type DialogEvent::DialogEvent_Type =
        (QEvent::Type)QEvent::registerEventType();

void DialogsProvider::extendedDialog(int page)
{
    // as the preferences: the native effects without the main interface
    if( p_intf->p_mi )
        emit effectsRequested(page == 1 ? 1 : 0);
    else
        nativeEffectsDialog(page == 1 ? 1 : 0);
}

void DialogsProvider::synchroDialog()
{
    ensureDialog(m_extendedDialog);
    if( !m_extendedDialog->isVisible() || /* Hidden */
        m_extendedDialog->currentTab() != 2 )  /* wrong tab */
        m_extendedDialog->showTab( 2 );
    else
        m_extendedDialog->hide();
}

void DialogsProvider::messagesDialog(int page)
{
    ensureDialog(m_messagesDialog);
    if(!m_messagesDialog->isVisible() || page)
        m_messagesDialog->showTab(page);
    else
        m_messagesDialog->toggleVisible();
}

void DialogsProvider::gotoTimeDialog()
{
    toggleDialogVisible(m_gotoTimeDialog);
}

#ifdef ENABLE_VLM
void DialogsProvider::vlmDialog()
{
    toggleDialogVisible(m_vlmDialog);
}
#endif

void DialogsProvider::helpDialog()
{
    toggleDialogVisible(m_helpDialog);
}

#ifdef UPDATE_CHECK
void DialogsProvider::updateDialog(Mode mode)
{
    ensureDialog(m_updateDialog);
    assert(m_updateDialog);

    switch (mode)
    {
    case Mode::Show:
        m_updateDialog->show();
        break;
    case Mode::Hide:
        m_updateDialog->hide();
        break;
    case Mode::Toggle:
    default:
        toggleDialogVisible(m_updateDialog);
    };
}
#endif

void DialogsProvider::aboutDialog()
{
    // The redesign's About lives in the main interface; a skin keeps the
    // classic one
    if( p_intf->p_mi )
        emit aboutRequested();
    else
        nativeAboutDialog();
}

void DialogsProvider::nativeAboutDialog()
{
    toggleDialogVisible(m_aboutDialog);
}

void DialogsProvider::mediaInfoDialog( void )
{
    ensureDialog(m_mediaInfoDialog);
    if( !m_mediaInfoDialog->isVisible() || m_mediaInfoDialog->currentTab() != MediaInfoDialog::META_PANEL )
        m_mediaInfoDialog->showTab( MediaInfoDialog::META_PANEL );
    else
        m_mediaInfoDialog->hide();
}

void DialogsProvider::mediaInfoDialog( const SharedInputItem& inputItem )
{
    assert(inputItem.get());

    MediaInfoDialog * const mid = new MediaInfoDialog( p_intf, inputItem );
    mid->setWindowFlag( Qt::Dialog );
    mid->setAttribute( Qt::WA_DeleteOnClose );
    mid->showTab( MediaInfoDialog::META_PANEL );
}

void DialogsProvider::mediaInfoDialog( const PlaylistItem& pItem )
{
    input_item_t *p_input = nullptr;

    vlc_playlist_item_t * const playlistItem = pItem.raw();

    if( playlistItem )
    {
        p_input = vlc_playlist_item_GetMedia(playlistItem);
    }

    if( p_input )
    {
        mediaInfoDialog( SharedInputItem{ p_input } );
    }
}

void DialogsProvider::mediaInfoDialog( const MLItemId& itemId )
{
    assert( p_intf );

    vlc_medialibrary_t* const ml = vlc_ml_instance_get( p_intf );
    assert( ml );

    input_item_t * const inputItem = vlc_ml_get_input_item( ml, itemId.id );
    assert( inputItem );

    mediaInfoDialogOf( inputItem );
}

void DialogsProvider::mediaInfoDialogForMrl( const QString& mrl )
{
    input_item_t * const inputItem = input_item_New( qtu( mrl ), NULL );
    if ( inputItem )
        mediaInfoDialogOf( inputItem );
}

void DialogsProvider::mediaInfoDialogOf( input_item_t* const inputItem )
{
    const SharedInputItem sharedInputItem { inputItem, false };
    if ( input_item_IsPreparsed( inputItem ) )
    {
        mediaInfoDialog( sharedInputItem );
    }
    else
    {
        static const input_item_parser_cbs_t cbs = {
            // on_preparse_ended
            []( input_item_t * const item, int status, void * const userData ) {
                const auto dp = static_cast<DialogsProvider *>( userData );

                if ( status != VLC_SUCCESS )
                    qWarning( "Could not preparse input item %p. Status %i", item, status );

                const SharedInputItem sharedInputItem{ item };

                QMetaObject::invokeMethod( dp, [dp, sharedInputItem]() {
                        dp->m_parser.reset();
                        dp->mediaInfoDialog( sharedInputItem );
                    }, Qt::QueuedConnection );
            },
            // on_subtree_added
            NULL,
            // on_attachments_added
            NULL,
        };

        const struct input_item_parser_cfg cfg = {
            .cbs = &cbs,
            .cbs_data = this,
            .subitems = true,
            .interact = false,
        };

        m_parser = vlc::wrap_cptr( input_item_Parse( VLC_OBJECT( p_intf ),
                                   inputItem, &cfg ),
                                   &input_item_parser_id_Release );
    }
}

bool DialogsProvider::questionDialog(const QString& text, const QString &title, bool always) const
{
    if (!always && !var_InheritBool(p_intf, "qt-error-dialogs"))
        return true;

    QMessageBox messageBox;
    messageBox.setText(text);
    if (!title.isEmpty())
        messageBox.setWindowTitle(title);
    messageBox.setIcon(QMessageBox::Question);
    messageBox.setStandardButtons(QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No));
    messageBox.setDefaultButton(QMessageBox::No);

    QVLCDialog::setWindowTransientParent(&messageBox, nullptr, p_intf);

    int result = messageBox.exec();
    return result == QMessageBox::Yes;
}

void DialogsProvider::mediaCodecDialog()
{
    ensureDialog(m_mediaInfoDialog);

    if( !m_mediaInfoDialog->isVisible() || m_mediaInfoDialog->currentTab() != MediaInfoDialog::INFO_PANEL )
        m_mediaInfoDialog->showTab( MediaInfoDialog::INFO_PANEL );
    else
        m_mediaInfoDialog->hide();
}

QMessageBox::StandardButton DialogsProvider::messageDialog(const QString &text,
                                                           const QString &title,
                                                           const QMessageBox::StandardButtons buttons,
                                                           const QMessageBox::StandardButton defaultButton,
                                                           const QMessageBox::Icon icon)
{
    QMessageBox messageBox;

    messageBox.setText(text);

    if (!title.isEmpty())
        messageBox.setWindowTitle(title);

    if (icon != QMessageBox::NoIcon)
        messageBox.setIcon(icon);

    if (buttons != QMessageBox::NoButton)
        messageBox.setStandardButtons(buttons);

    if (defaultButton != QMessageBox::NoButton)
        messageBox.setDefaultButton(defaultButton);

    QVLCDialog::setWindowTransientParent(&messageBox, nullptr, p_intf);

    return static_cast<QMessageBox::StandardButton>(messageBox.exec());
}

void DialogsProvider::playlistsDialog( const QVariantList & medias, MLPlaylistListModel::PlaylistType type )
{
    const auto dialog = new PlaylistsDialog(p_intf, medias, type);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show(); // "Modeless dialogs are displayed using show()"
}

void DialogsProvider::bookmarksDialog()
{
    toggleDialogVisible(m_bookmarkDialog);
}

void DialogsProvider::podcastConfigureDialog()
{
    toggleDialogVisible(m_podcastDialog);
}

void DialogsProvider::pluginDialog()
{
    toggleDialogVisible(m_pluginDialog);
}

void DialogsProvider::epgDialog()
{
    toggleDialogVisible(m_egpDialog);
}

void DialogsProvider::setPopupMenu()
{
    popupMenu.reset(VLCMenuBar::PopupMenu( p_intf, true ));
}

void DialogsProvider::destroyPopupMenu()
{
    popupMenu.reset();
}

/* Generic open file */
void DialogsProvider::openFileGenericDialog( intf_dialog_args_t *p_arg )
{
    if( p_arg == NULL )
    {
        msg_Warn( p_intf, "openFileGenericDialog() called with NULL arg" );
        return;
    }

    /* Replace the extensions to a Qt format */
    int i = 0;
    QString extensions = qfu( p_arg->psz_extensions );
    while ( ( i = extensions.indexOf( "|", i ) ) != -1 )
    {
        if( ( extensions.count( "|" ) % 2 ) == 0 )
            extensions.replace( i, 1, ");;" );
        else
            extensions.replace( i, 1, "(" );
    }
    extensions.replace( ";*", " *" );
    extensions.append( ")" );

    /* Save */
    if( p_arg->b_save )
    {
        QString file = getSaveFileName( NULL,
                                        qfu( p_arg->psz_title ),
                                        p_intf->p_mi->getDialogFilePath(), extensions );
        if( !file.isEmpty() )
        {
            p_arg->i_results = 1;
            p_arg->psz_results = (char **)vlc_alloc( p_arg->i_results, sizeof( char * ) );
            p_arg->psz_results[0] = strdup( qtu( toNativeSepNoSlash( std::move(file) ) ) );
        }
        else
            p_arg->i_results = 0;
    }
    else /* non-save mode */
    {
        QList<QUrl> urls = QFileDialog::getOpenFileUrls( NULL, qfu( p_arg->psz_title ),
                                       p_intf->p_mi->getDialogFilePath(), extensions );
        p_arg->i_results = urls.count();
        p_arg->psz_results = (char **)vlc_alloc( p_arg->i_results, sizeof( char * ) );
        i = 0;
        foreach( const QUrl &uri, urls )
            p_arg->psz_results[i++] = strdup( uri.toEncoded().constData() );
        if( !urls.isEmpty() )
            p_intf->p_mi->setDialogFilePath(urls.last());
    }

    /* Callback */
    if( p_arg->pf_callback )
        p_arg->pf_callback( p_arg );

    /* Clean afterwards */
    if( p_arg->psz_results )
    {
        for( i = 0; i < p_arg->i_results; i++ )
            free( p_arg->psz_results[i] );
        free( p_arg->psz_results );
    }
    free( p_arg->psz_title );
    free( p_arg->psz_extensions );
    free( p_arg );
}
/****************************************************************************
 * All the open/add stuff
 * Open Dialog first - Simple Open then
 ****************************************************************************/

void DialogsProvider::openDialog( OpenDialog::OpenTab i_tab )
{
    ensureDialog(m_openDialog);
    m_openDialog->showTab( i_tab, OpenDialog::OPEN_AND_PLAY );
}
void DialogsProvider::openDialog()
{
    openDialog( OpenDialog::OPEN_FILE_TAB );
}
void DialogsProvider::openFileDialog()
{
    openDialog( OpenDialog::OPEN_FILE_TAB );
}
void DialogsProvider::openDiscDialog()
{
    openDialog( OpenDialog::OPEN_DISC_TAB );
}
void DialogsProvider::openNetDialog()
{
    // The redesign opens streams in its own dialog (Figma "Open network
    // stream"); without the main interface (skins), the Open dialog does
    if( p_intf->p_mi )
        networkStreamDialog();
    else
        openDialog( OpenDialog::OPEN_NETWORK_TAB );
}
void DialogsProvider::openCaptureDialog()
{
    openDialog( OpenDialog::OPEN_CAPTURE_TAB );
}

/* Same as the open one, but force the enqueue */
void DialogsProvider::PLAppendDialog( OpenDialog::OpenTab tab )
{
    ensureDialog(m_openDialog);
    m_openDialog->showTab( tab, OpenDialog::OPEN_AND_ENQUEUE );
}

/**
 * Simple open
 ***/
void DialogsProvider::simpleOpenDialog(bool start)
{
    QStringList urls = DialogsProvider::showSimpleOpen();

    urls.sort();
    QVector<vlc::playlist::Media> medias;
    for( const QString& mrl : urls)
        medias.push_back( vlc::playlist::Media{mrl, QString {}} );
    if (!medias.empty())
        THEMPL->append(medias, start);
}

/* A link typed without its scheme ("example.com/live.m3u8") names a host,
 * not a file of the computer: it gets https, or http for an address,
 * localhost or a port (local servers, IPTV boxes); as NetworkStreamDialog.
 * Returns an empty string for anything else (paths stay paths). */
static QString hostLinkWithScheme( const QString& text )
{
    static const QRegularExpression path( QStringLiteral( "^([/\\\\~.]|[A-Za-z]:([/\\\\]|$))" ) );
    static const QRegularExpression host( QStringLiteral(
        "^(\\[[0-9A-Fa-f:.]+\\]|localhost|[A-Za-z0-9-]+(\\.[A-Za-z0-9-]+)+)(:(\\d+))?([/?#].*)?$" ) );
    static const QRegularExpression ipv4( QStringLiteral( "^\\d{1,3}(\\.\\d{1,3}){3}$" ) );

    if( path.match( text ).hasMatch() )
        return {};
    const QRegularExpressionMatch match = host.match( text );
    if( !match.hasMatch() )
        return {};
    const QString name = match.captured( 1 );
    const bool address = name.startsWith( QLatin1Char( '[' ) )
                      || name == QLatin1String( "localhost" )
                      || ipv4.match( name ).hasMatch();
    const QString portNumber = match.captured( 4 );
    const bool port = !portNumber.isEmpty() && portNumber != QLatin1String( "443" );
    return ( ( address || port ) ? QStringLiteral( "http://" ) : QStringLiteral( "https://" ) ) + text;
}

/* Url & Clipboard */
/**
 * Open a MRL.
 * If the clipboard contains URLs, the first is automatically 'preselected'.
 **/
void DialogsProvider::openUrlDialog()
{
    OpenUrlDialog oud( p_intf );
    if( oud.exec() != QDialog::Accepted )
        return;

    QString url = oud.url().trimmed();
    if( url.isEmpty() )
        return;

    if( !url.contains( qfu( "://" ) ) )
    {
        const QString link = hostLinkWithScheme( url );
        if( !link.isEmpty() )
            url = link;
    }

    char *uri;
    if( !url.contains( qfu( "://" ) ) )
    {
        uri = vlc_path2uri( qtu( url ), NULL );
    } else {
        uri = vlc_uri_fixup( qtu( url ) );
    }

    if( uri == NULL )
        return;
    url = qfu(uri);
    free( uri );

    QVector<vlc::playlist::Media> medias = { {url, QString {}} };
    THEMPL->append(medias, !oud.shouldEnqueue());
}

/* Directory */
/**
 * Open a directory,
 * pl helps you to choose from playlist or media library,
 * go to start or enqueue
 **/
static void openDirectory( qt_intf_t *p_intf, bool go )
{
    QString uri = DialogsProvider::getDirectoryDialog( p_intf );
    if( !uri.isEmpty() )
    {
        QVector<vlc::playlist::Media> medias = { {uri, QString {}} };
        THEMPL->append(medias, go);
    }
}

QString DialogsProvider::getDirectoryDialog( qt_intf_t *p_intf )
{
    const QStringList schemes = QStringList(QStringLiteral("file"));
    QUrl dirurl = QFileDialog::getExistingDirectoryUrl( NULL,
            qfut( I_OP_DIR_WINTITLE ), p_intf->p_mi->getDialogFilePath(),
            QFileDialog::ShowDirsOnly, schemes );

    if( dirurl.isEmpty() ) return QString();

    p_intf->p_mi->setDialogFilePath(dirurl);

    QString dir = dirurl.toLocalFile();
    const char *scheme = "directory";
    if( dir.endsWith( DIR_SEP "VIDEO_TS", Qt::CaseInsensitive ) )
        scheme = "dvd";
    else if( dir.endsWith( DIR_SEP "BDMV", Qt::CaseInsensitive ) )
    {
        scheme = "bluray";
        dir.remove( "BDMV" );
    }

    char *uri = vlc_path2uri( qtu( toNativeSeparators( dir ) ), scheme );
    if( unlikely(uri == NULL) )
        return QString();

    dir = qfu( uri );
    free( uri );

    return dir;
}

void DialogsProvider::PLOpenDir()
{
    openDirectory( p_intf, true );
}

void DialogsProvider::PLAppendDir()
{
    openDirectory( p_intf, false );
}

/****************
 * Playlist     *
 ****************/
QString DialogsProvider::askPlaylistFile( const QString& title, const char **module )
{
    static const struct
    {
        char filter_name[14];
        char filter_patterns[5];
        char module[12];
    } types[] = {
        { N_("XSPF playlist"), "xspf", "export-xspf", },
        { N_("M3U playlist"),  "m3u",  "export-m3u", },
        { N_("M3U8 playlist"), "m3u8", "export-m3u8", },
        { N_("HTML playlist"), "html", "export-html", },
    };

    QStringList filters;
    QString ext = getSettings()->value( "last-playlist-ext" ).toString();

    for( size_t i = 0; i < ARRAY_SIZE(types); i++ )
    {
        QString tmp = qfut( types[i].filter_name ) + " (*." + types[i].filter_patterns + ")";
        if( ext == qfu( types[i].filter_patterns ) )
            filters.insert( 0, tmp );
        else
            filters.append( tmp );
    }

    QString selected;
    QString file = getSaveFileName( NULL, title,
                                    p_intf->p_mi->getDialogFilePath(), filters.join( ";;" ),
                                    &selected );
    const char *psz_selected_module = NULL;
    const char *psz_last_playlist_ext = NULL;

    if( file.isEmpty() )
        return {};

    /* First test if the file extension is set, and different to selected filter */
    for( size_t i = 0; i < ARRAY_SIZE(types); i++)
    {
        if ( file.endsWith( QString( "." ) + qfu( types[i].filter_patterns ) ) )
        {
            psz_selected_module = types[i].module;
            psz_last_playlist_ext = types[i].filter_patterns;
            break;
        }
    }

    /* otherwise apply the selected extension */
    if ( !psz_last_playlist_ext )
    {
        for( size_t i = 0; i < ARRAY_SIZE(types); i++)
        {
            if ( selected.startsWith( qfut( types[i].filter_name ) ) )
            {
                psz_selected_module = types[i].module;
                psz_last_playlist_ext = types[i].filter_patterns;
                /* Fix file extension */
                file = file.append( QString( "." ) + qfu( psz_last_playlist_ext ) );
                break;
            }
        }
    }

    if ( !psz_selected_module )
        return {};
    getSettings()->setValue( "last-playlist-ext", psz_last_playlist_ext );
    *module = psz_selected_module;
    return file;
}

void DialogsProvider::savePlayingToPlaylist()
{
    const char *module = nullptr;
    const QString file = askPlaylistFile( qtr( "Save play queue as..." ), &module );
    if ( file.isEmpty() )
        return;
    vlc_playlist_Lock( p_intf->p_playlist  );
    vlc_playlist_Export( p_intf->p_playlist, qtu( toNativeSeparators( file ) ), module );
    vlc_playlist_Unlock( p_intf->p_playlist  );
}

namespace {

/* The formats of askPlaylistFile written from a list of media, as the
 * "playlist export" modules (modules/misc/playlist) write them: these need a
 * vlc_playlist_t, that is a player and its audio output */

char *xmlEncoded( char *text )
{
    if( !text )
        return nullptr;
    char *encoded = vlc_xml_encode( text );
    free( text );
    return encoded;
}

void writeM3U( FILE *file, const char *baseUrl, const QVector<vlc::playlist::Media>& medias,
               int (*printText)( FILE *, const char *, ... ) )
{
    /* relative addresses for the media next to the playlist or below it */
    size_t prefixLength = 0;
    if( baseUrl )
        if( const char *slash = strrchr( baseUrl, '/' ) )
            prefixLength = ( slash + 1 ) - baseUrl;

    fputs( "#EXTM3U\n", file );
    for( const auto& media : medias )
    {
        input_item_t *item = media.raw();
        char *uri = input_item_GetURI( item );
        if( !uri )
            continue;

        char *name = input_item_GetName( item );
        if( name && strcmp( uri, name ) )
        {
            char *artist = input_item_GetArtist( item );
            const vlc_tick_t duration = input_item_GetDuration( item );
            const long long seconds = duration > 0 ? SEC_FROM_VLC_TICK( duration ) : -1;
            if( artist && *artist )
                printText( file, "#EXTINF:%lld,%s - %s\n", seconds, artist, name );
            else
                printText( file, "#EXTINF:%lld,%s\n", seconds, name );
            free( artist );
        }
        free( name );

        vlc_mutex_lock( &item->lock );
        for( int i = 0; i < item->i_options; i++ )
        {
            const char *option = item->ppsz_options[i];
            printText( file, "#EXTVLCOPT:%s\n", option[0] == ':' ? option + 1 : option );
        }
        vlc_mutex_unlock( &item->lock );

        const size_t skip = prefixLength > 0 && !strncmp( baseUrl, uri, prefixLength )
                          ? prefixLength : 0;
        fprintf( file, "%s\n", uri + skip );
        free( uri );
    }
}

void writeXSPF( FILE *file, const QVector<vlc::playlist::Media>& medias )
{
    fputs( "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
           "<playlist xmlns=\"http://xspf.org/ns/0/\" "
           "xmlns:vlc=\"http://www.videolan.org/vlc/playlist/ns/0/\" version=\"1\">\n"
           "\t<trackList>\n", file );

    unsigned long long id = 0;
    for( const auto& media : medias )
    {
        input_item_t *item = media.raw();
        fputs( "\t\t<track>\n", file );

        char *uri = xmlEncoded( input_item_GetURI( item ) );
        if( uri && *uri )
            fprintf( file, "\t\t\t<location>%s</location>\n", uri );
        char *title = xmlEncoded( input_item_GetTitle( item ) );
        if( title && ( !uri || strcmp( uri, title ) ) )
            fprintf( file, "\t\t\t<title>%s</title>\n", title );
        free( title );
        free( uri );

        const struct
        {
            char *(*get)( input_item_t * );
            const char *tag;
        } metas[] = {
            { input_item_GetArtist, "creator" },
            { input_item_GetAlbum, "album" },
            { input_item_GetDescription, "annotation" },
            { input_item_GetURL, "info" },
            { input_item_GetArtURL, "image" },
        };
        for( const auto& meta : metas )
        {
            char *value = xmlEncoded( meta.get( item ) );
            if( value && *value )
                fprintf( file, "\t\t\t<%s>%s</%s>\n", meta.tag, value, meta.tag );
            free( value );
        }
        if( char *trackNumber = input_item_GetTrackNum( item ) )
        {
            const int number = atoi( trackNumber );
            free( trackNumber );
            if( number > 0 )
                fprintf( file, "\t\t\t<trackNum>%d</trackNum>\n", number );
        }

        const vlc_tick_t duration = input_item_GetDuration( item );
        if( duration > 0 )
            fprintf( file, "\t\t\t<duration>%lld</duration>\n",
                     static_cast<long long>( MS_FROM_VLC_TICK( duration ) ) );

        /* the id and the options of the media (bookmarks...) */
        fputs( "\t\t\t<extension application=\"http://www.videolan.org/vlc/playlist/0\">\n", file );
        fprintf( file, "\t\t\t\t<vlc:id>%llu</vlc:id>\n", id++ );
        vlc_mutex_lock( &item->lock );
        for( int i = 0; i < item->i_options; i++ )
        {
            const char *option = item->ppsz_options[i];
            char *encoded = vlc_xml_encode( option[0] == ':' ? option + 1 : option );
            if( encoded )
                fprintf( file, "\t\t\t\t<vlc:option>%s</vlc:option>\n", encoded );
            free( encoded );
        }
        vlc_mutex_unlock( &item->lock );
        fputs( "\t\t\t</extension>\n"
               "\t\t</track>\n", file );
    }

    fputs( "\t</trackList>\n"
           "</playlist>\n", file );
}

void writeHTML( FILE *file, const QVector<vlc::playlist::Media>& medias )
{
    fputs( "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n"
           "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\n"
           "<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\">\n"
           "<head>\n"
           "  <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\n"
           "  <meta name=\"Generator\" content=\"VLC media player\" />\n"
           "  <meta name=\"Author\" content=\"VLC, http://www.videolan.org/vlc/\" />\n"
           "  <title>VLC generated playlist</title>\n"
           "  <style type=\"text/css\">\n"
           "    body {\n"
           "      background-color: #E4F3FF;\n"
           "      font-family: sans-serif, Helvetica, Arial;\n"
           "      font-size: 13px;\n"
           "    }\n"
           "    h1 {\n"
           "      color: #2D58AE;\n"
           "      font-size: 25px;\n"
           "    }\n"
           "    hr {\n"
           "      color: #555555;\n"
           "    }\n"
           "  </style>\n"
           "</head>\n\n"
           "<body>\n"
           "  <h1>Playlist</h1>\n"
           "  <hr />\n"
           "  <ol>\n", file );

    for( const auto& media : medias )
    {
        input_item_t *item = media.raw();
        char *name = xmlEncoded( input_item_GetName( item ) );
        if( !name )
            continue;
        char *artist = xmlEncoded( input_item_GetArtist( item ) );
        const vlc_tick_t duration = input_item_GetDuration( item );
        const long long seconds = duration > 0 ? SEC_FROM_VLC_TICK( duration ) : 0;
        const int minutes = int( seconds / 60 ), rest = int( seconds % 60 );
        if( artist && *artist )
            fprintf( file, "    <li>%s - %s (%02d:%02d)</li>\n", artist, name, minutes, rest );
        else
            fprintf( file, "    <li>%s (%02d:%02d)</li>\n", name, minutes, rest );
        free( artist );
        free( name );
    }

    fputs( "  </ol>\n"
           "  <hr />\n"
           "</body>\n"
           "</html>", file );
}

/* Returns an error message, empty on success */
QString writePlaylistFile( const QString& path, const QByteArray& module,
                           const QVector<vlc::playlist::Media>& medias )
{
    const QByteArray filename = path.toUtf8();
    FILE *file = vlc_fopen( filename.constData(), "wt" );
    if( !file )
        return QString::fromUtf8( vlc_strerror_c( errno ) );

    if( module == "export-m3u" || module == "export-m3u8" )
    {
        char *baseUrl = vlc_path2uri( filename.constData(), nullptr );
        writeM3U( file, baseUrl, medias, module == "export-m3u" ? utf8_fprintf : fprintf );
        free( baseUrl );
    }
    else if( module == "export-xspf" )
        writeXSPF( file, medias );
    else
        writeHTML( file, medias );

    QString error;
    if( ferror( file ) )
        error = QString::fromUtf8( vlc_strerror_c( errno ) );
    if( fclose( file ) != 0 && error.isEmpty() )
        error = QString::fromUtf8( vlc_strerror_c( errno ) );
    return error;
}

} // anonymous namespace

/* The media of a library playlist written to a playlist file (Figma "Export
 * Playlist"). The file is written from the media themselves on a worker
 * thread: the export modules would need a playlist, with its player and
 * audio output, created on the UI thread. */
void DialogsProvider::saveMediaToPlaylistFile( const QVector<vlc::playlist::Media>& medias )
{
    if ( medias.isEmpty() )
        return;
    const char *module = nullptr;
    const QString file = askPlaylistFile( qtr( "Export playlist as..." ), &module );
    if ( file.isEmpty() )
        return;

    const QString path = toNativeSeparators( file );
    const QByteArray moduleName( module );
    QPointer<DialogsProvider> provider( this );
    QThreadPool::globalInstance()->start( [provider, path, moduleName, medias]() {
        const QString error = writePlaylistFile( path, moduleName, medias );
        if( error.isEmpty() )
            return;
        QMetaObject::invokeMethod( qApp, [provider, path, error]() {
            if( provider )
                msg_Err( provider->p_intf, "Could not export the playlist to %s: %s",
                         qtu( path ), qtu( error ) );
        }, Qt::QueuedConnection );
    });
}

/****************************************************************************
 * Sout emulation
 ****************************************************************************/

void DialogsProvider::streamingDialog( QWindow *parent,
                                       const QStringList& mrls,
                                       bool b_transcode_only,
                                       QStringList options )
{
    QStringList outputMRLs;

    /* Stream */
    // Does streaming multiple files make sense?  I suppose so, just stream one
    // after the other, but not at the moment.
    if( !b_transcode_only )
    {
        SoutDialog *s = new SoutDialog( parent, p_intf, mrls[0] );
        s->setAttribute( Qt::WA_QuitOnClose, false ); // See #4883
        if( s->exec() == QDialog::Accepted )
        {
            outputMRLs.append(s->getChain());
            delete s;
        }
        else
        {
            delete s; return;
        }
    } else {
    /* Convert */
        ConvertDialog *s = new ConvertDialog( parent, p_intf, mrls );
        s->setAttribute( Qt::WA_QuitOnClose, false ); // See #4883
        if( s->exec() == QDialog::Accepted )
        {
            /* Clear the playlist.  This is because we're going to be populating
               it */
            THEMPL->clear();
            outputMRLs = s->getMrls();
            delete s;
        }
        else
        {
            delete s; return;
        }
    }

    /* Get SoutChain(s) */
    QVector<vlc::playlist::Media> outputMedias;

    for( auto it = outputMRLs.cbegin(); it != outputMRLs.cend(); ++it )
    {
        const QString &mrl = mrls[std::distance(outputMRLs.cbegin(), it)];
        QString title = "Converting " + mrl;
        outputMedias.append(vlc::playlist::Media(mrl, title, options + (*it).split(" :")));
    }

    if( !outputMedias.empty() )
        THEMPL->append(outputMedias, true);
}

void DialogsProvider::streamingDialog(const QList<QUrl> &urls, bool b_transcode_only )
{
    if(urls.isEmpty())
        return;

    QStringList _urls;
    std::transform(urls.begin(),
                   urls.end(),
                   std::back_inserter(_urls),
                   [](const QUrl& url){ return url.toString(); });

    streamingDialog(nullptr, _urls, b_transcode_only);
}

void DialogsProvider::openAndStreamingDialogs()
{
    ensureDialog(m_openDialog);
    m_openDialog->showTab( OpenDialog::OPEN_FILE_TAB, OpenDialog::OPEN_AND_STREAM );
}

void DialogsProvider::openAndTranscodingDialogs()
{
    ensureDialog(m_openDialog);
    m_openDialog->showTab( OpenDialog::OPEN_FILE_TAB, OpenDialog::OPEN_AND_SAVE );
}

void  DialogsProvider::loadMediaFile( const es_format_category_e category, const int filter , const QString &dialogTitle)
{
    input_item_t *p_item = THEMIM->getInput();
    if( !p_item ) return;

    char *path = input_item_GetURI( p_item );
    QUrl url;
    if( path )
    {
        url.setUrl( qfu(path) );
        url = url.adjusted(QUrl::RemoveFilename);
        if (url.scheme() != "file")
            url.clear();
        free(path);
    }

    QStringList qsl = showSimpleOpen( dialogTitle,
                                      filter,
                                      url );

    foreach( const QString &qsUrl, qsl )
    {

        if ( THEMIM->AddAssociatedMedia( category, qsUrl, true, true, false ) )
            msg_Warn( p_intf, "unable to load media from '%s', category(%d)", qtu( qsUrl ), category );
    }
}

void DialogsProvider::loadSubtitlesFile()
{
    loadMediaFile( SPU_ES, EXT_FILTER_SUBTITLE, qtr( "Open subtitles..." ) );
}

void DialogsProvider::loadAudioFile()
{
    loadMediaFile( AUDIO_ES, EXT_FILTER_AUDIO, qtr( "Open audio..." ) );
}

void DialogsProvider::loadVideoFile()
{
    loadMediaFile( VIDEO_ES, EXT_FILTER_VIDEO, qtr( "Open video..." ) );
}


/****************************************************************************
 * Menus
 ****************************************************************************/

void DialogsProvider::sendKey( int key )
{
     // translate from a vlc keycode into a Qt sequence
     QKeySequence kseq0( VLCKeyToString( key, true ) );

     if( !popupMenu )
     {
         // make sure at least a non visible popupmenu is available
         popupMenu.reset(VLCMenuBar::PopupMenu( p_intf, false ));
         if( unlikely( !popupMenu ) )
             return;
     }

     // test against key accelerators from the popupmenu
     QList<QAction*> actions = popupMenu->findChildren<QAction*>();
     for( int i = 0; i < actions.size(); i++ )
     {
         QAction* action = actions.at(i);
         QKeySequence kseq = action->shortcut();
         if( kseq == kseq0 )
         {
             action->trigger();
             return;
         }
     }

     // forward key to vlc core when not a key accelerator
     var_SetInteger( vlc_object_instance(p_intf), "key-pressed", key );
}


template<typename T>
void DialogsProvider::ensureDialog(std::unique_ptr<T>& dialog)
{
    if (!dialog) {
        dialog = std::make_unique<T>(p_intf);
        //install event filter to track visibility
        dialog->installEventFilter(this);
    }
}

template<typename T>
void DialogsProvider::toggleDialogVisible(std::unique_ptr<T>& dialog) {
    ensureDialog(dialog);
    dialog->toggleVisible();
}

#define DIALOG_VISIBLE_GETTER(dialog) \
    bool DialogsProvider:: dialog ## Visible() const { \
        if (m_redesignDialogs.contains(QStringLiteral(#dialog))) \
            return true; \
        if (!m_ ## dialog) \
            return false; \
        return m_ ## dialog->isVisible(); \
    }

ALL_DIALOG(DIALOG_VISIBLE_GETTER)
