/*****************************************************************************
 * menus.cpp : Qt menus
 *****************************************************************************
 * Copyright © 2006-2011 the VideoLAN team
 *
 * Authors: Clément Stenac <zorglub@videolan.org>
 *          Jean-Baptiste Kempf <jb@videolan.org>
 *          Jean-Philippe André <jpeg@videolan.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * ( at your option ) any later version.
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

/** \todo
 * - Remove static currentGroup
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_intf_strings.h>
#include <vlc_aout.h>                             /* audio_output_t */
#include <vlc_interface.h>

#include "menus.hpp"
#include "redesign_menu_style.hpp"

#include "maininterface/mainctx.hpp"                     /* View modifications */
#include "maininterface/mainctx_submodels.hpp"
#include "dialogs/dialogs_provider.hpp"                   /* Dialogs display */
#include "dialogs/help/help.hpp"
#include "player/player_controller.hpp"                      /* Input Management */
#include "playlist/playlist_controller.hpp"
#include "dialogs/extensions/extensions_manager.hpp"                 /* Extensions menu */
#include "dialogs/extended/extended_panels.hpp"
#include "dialogs/systray/systray.hpp"
#include "util/varchoicemodel.hpp"
#include "util/color_scheme_model.hpp"
#include "util/colorizedsvgicon.hpp"
#include "medialibrary/medialib.hpp"
#include "medialibrary/mlrecentmediamodel.hpp"
#include "medialibrary/mlbookmarkmodel.hpp"


#include <QMenu>
#include <QMenuBar>
#include <QPainter>
#include <QPixmap>
#include <QAction>
#include <QActionGroup>
#include <QSignalMapper>

using namespace vlc::playlist;

/*
  This file defines the main menus and the pop-up menu (right-click menu)
  and the systray menu (in that order in the file)

  There are 4 menus that have to be rebuilt every time there are called:
  Audio, Video, Navigation, view
  4 functions are building those menus: AudioMenu, VideoMenu, NavigMenu, View

  */

RendererMenu *VLCMenuBar::rendererMenu = NULL;

/**
 * @brief Add static entries to DP in menus
 **/
template<typename Functor>
QAction *addDPStaticEntry( QMenu *menu,
                       const QString& text,
                       const char *icon,
                       const Functor member,
                       const char *shortcut = NULL,
                       QAction::MenuRole role = QAction::NoRole
                       )
{
    QAction *action = NULL;
#ifndef __APPLE__ /* We don't set icons in menus in MacOS X */
    if( !EMPTY_STR( icon ) )
    {
        action = menu->addAction( RedesignMenuStyle::menuIcon( icon, menu ), text, THEDP, member );
    }
    else
#endif
    {
        action = menu->addAction( text, THEDP, member );
    }
    if( !EMPTY_STR( shortcut ) )
        action->setShortcut( qfut( shortcut ) );
#ifdef __APPLE__
    action->setMenuRole( role );
#else
    Q_UNUSED( role );
#endif
    return action;
}

/**
 * @brief Add static entries to MIM in menus
 **/
template<typename Fun>
static QAction* addMIMStaticEntry( qt_intf_t *p_intf,
                            QMenu *menu,
                            const QString& text,
                            const char *icon,
                            Fun member )
{
    QAction *action;
#ifndef __APPLE__ /* We don't set icons in menus in MacOS X */
    if( !EMPTY_STR( icon ) )
    {
        action = menu->addAction( text, THEMIM,  member );
        action->setIcon( RedesignMenuStyle::menuIcon( icon, menu ) );
    }
    else
#endif
    {
        action = menu->addAction( text, THEMIM, member );
    }
    return action;
}

template<typename Fun>
static QAction* addMPLStaticEntry( qt_intf_t *p_intf,
                            QMenu *menu,
                            const QString& text,
                            const char *icon,
                            Fun member )
{
    QAction *action;
#ifndef __APPLE__ /* We don't set icons in menus in MacOS X */
    if( !EMPTY_STR( icon ) )
    {
        action = menu->addAction( text, THEMPL,  member );
        action->setIcon( RedesignMenuStyle::menuIcon( icon, menu ) );
    }
    else
#endif
    {
        action = menu->addAction( text, THEMPL, member );
    }
    return action;
}

/*****************************************************************************
 * All normal menus
 * Simple Code
 *****************************************************************************/

/**
 * Main Menu Bar Creation
 **/
VLCMenuBar::VLCMenuBar(QObject* parent)
    : QObject(parent)
{}


/**
 * Media ( File ) Menu
 * Opening, streaming and quit
 **/
void VLCMenuBar::FileMenu(qt_intf_t *p_intf, QMenu *menu)
{
    auto mi = p_intf->p_mi;
    QAction *action;

    //use a lambda here as the Triggrered signal is emiting and it will pass false (checked) as a first argument
    addDPStaticEntry( menu, qtr( "Open &File..." ),
        ":/menu/file.svg", []() { THEDP->simpleOpenDialog(); } , "Ctrl+O" );
    addDPStaticEntry( menu, qtr( "&Open Multiple Files..." ),
        ":/menu/file.svg", &DialogsProvider::openFileDialog, "Ctrl+Shift+O" );
    addDPStaticEntry( menu, qfut( I_OP_OPDIR ),
        ":/menu/folder.svg", &DialogsProvider::PLOpenDir, "Ctrl+F" );
    addDPStaticEntry( menu, qtr( "Open &Disc..." ),
        ":/menu/disc.svg", &DialogsProvider::openDiscDialog, "Ctrl+D" );
    addDPStaticEntry( menu, qtr( "Open &Network Stream..." ),
        ":/menu/network.svg", &DialogsProvider::openNetDialog, "Ctrl+N" );
    addDPStaticEntry( menu, qtr( "Open &Capture Device..." ),
        ":/menu/capture-card.svg", &DialogsProvider::openCaptureDialog, "Ctrl+C" );

    addDPStaticEntry( menu, qtr( "Open &Location from clipboard" ),
                      NULL, &DialogsProvider::openUrlDialog, "Ctrl+V" );

    if( mi && var_InheritBool( p_intf, "save-recentplay" ) && mi->hasMediaLibrary() )
    {
        MLRecentMediaModel* recentModel = new MLRecentMediaModel(nullptr);
        recentModel->setMl(mi->getMediaLibrary());
        recentModel->setLimit(10);
        QMenu* recentsMenu = new RecentMenu(recentModel, mi->getMediaLibrary(), menu);
        recentsMenu->setTitle(qtr( "Open &Recent Media" ) );
        recentModel->setParent(recentsMenu);
        menu->addMenu( recentsMenu );
    }

    menu->addSeparator();

    addDPStaticEntry( menu, qfut( I_PL_SAVE ), "", &DialogsProvider::savePlayingToPlaylist,
        "Ctrl+Y" );

    addDPStaticEntry( menu, qtr( "Conve&rt / Save..." ), "",
        &DialogsProvider::openAndTranscodingDialogs, "Ctrl+R" );
    addDPStaticEntry( menu, qtr( "&Stream..." ),
        ":/menu/stream.svg", &DialogsProvider::openAndStreamingDialogs, "Ctrl+S" );
    menu->addSeparator();

    action = menu->addAction( qtr( "Quit at the end of play queue" ), [playlist = THEMPL](bool checked){
        if (checked)
            playlist->setMediaStopAction(PlaylistController::MEDIA_STOPPED_EXIT);
        else
            playlist->setMediaStopAction(PlaylistController::MEDIA_STOPPED_CONTINUE);
    });
    action->setCheckable( true );
    action->setChecked( THEMPL->getMediaStopAction() ==  PlaylistController::MEDIA_STOPPED_EXIT);

    VLCSystray* systray = mi ? mi->getSysTray() : nullptr;
    if( systray )
    {
        action = menu->addAction( qtr( "Close to systray"), systray,
                                 &VLCSystray::toggleUpdateMenu );
    }

    addDPStaticEntry( menu, qtr( "&Quit" ) ,
        ":/menu/exit.svg", &DialogsProvider::quit, "Ctrl+Q" );
}

/**
 * Tools, like Media Information, Preferences or Messages
 **/
void VLCMenuBar::ToolsMenu( qt_intf_t *p_intf, QMenu *menu )
{
    addDPStaticEntry( menu, qtr( "&Effects and Filters"), ":/menu/ic_fluent_options.svg",
            &DialogsProvider::extendedDialog, "Ctrl+E" );

    addDPStaticEntry( menu, qtr( "&Track Synchronization"), ":/menu/ic_fluent_options.svg",
            &DialogsProvider::synchroDialog, "" );

    addDPStaticEntry( menu, qfut( I_MENU_INFO ) , ":/menu/info.svg",
        QOverload<>::of(&DialogsProvider::mediaInfoDialog), "Ctrl+I" );

    addDPStaticEntry( menu, qfut( I_MENU_CODECINFO ) ,
        ":/menu/info.svg", &DialogsProvider::mediaCodecDialog, "Ctrl+J" );

    addDPStaticEntry( menu, qtr( "Media &Statistics" ),
        ":/menu/info.svg", &DialogsProvider::statisticsDialog, "" );

#ifdef ENABLE_VLM
    addDPStaticEntry( menu, qfut( I_MENU_VLM ), "", &DialogsProvider::vlmDialog,
        "Ctrl+Shift+W" );
#endif

    addDPStaticEntry( menu, qtr( "Program Guide" ), "", &DialogsProvider::epgDialog,
        "" );

    addDPStaticEntry( menu, qfut( I_MENU_MSG ),
        ":/menu/messages.svg", &DialogsProvider::messagesDialog, "Ctrl+M" );

    addDPStaticEntry( menu, qtr( "Plu&gins and extensions" ),
        "", &DialogsProvider::pluginDialog );
    menu->addSeparator();

    if( !p_intf->b_isDialogProvider )
        addDPStaticEntry( menu, qtr( "Customi&ze Interface..." ),
            ":/menu/preferences.svg", &DialogsProvider::showToolbarEditorDialog);

    addDPStaticEntry( menu, qtr( "&Preferences" ),
        ":/menu/preferences.svg", &DialogsProvider::prefsDialog, "Ctrl+P", QAction::PreferencesRole );
}

/**
 * View Menu
 * Interface modification, load other interfaces, activate Extensions
 * \param current, set to NULL for menu creation, else for menu update
 **/
void VLCMenuBar::ViewMenu(qt_intf_t *p_intf, QMenu *menu)
{
    QAction *action;

    MainCtx *mi = p_intf->p_mi;
    assert( mi );
    assert(menu);

    //menu->clear();
    //HACK menu->clear() does not delete submenus
    QList<QAction*> actions = menu->actions();
    foreach( QAction *a, actions )
    {
        QMenu *m = a->menu();
        if( a->parent() == menu ) delete a;
        else menu->removeAction( a );
        if( m && m->parent() == menu ) delete m;
    }

    action = menu->addAction(
#ifndef __APPLE__
            RedesignMenuStyle::menuIcon( ":/menu/ic_playlist.svg", menu ),
#endif
            qtr( "Play &Queue" ));
    action->setShortcut(QString( "Ctrl+L" ));
    action->setCheckable( true );
    connect( action, &QAction::triggered, mi->getPlayqueuePanel(), &SidePanelCtx::setVisible );
    action->setChecked( mi->getPlayqueuePanel()->isVisible() );

    /* Docked Play Queue */
    action = menu->addAction( qtr( "Docked Play Queue" ) );
    action->setCheckable( true );
    connect( action, &QAction::triggered, mi->getPlayqueuePanel(), &SidePanelCtx::setDocked );
    action->setChecked( mi->getPlayqueuePanel()->isDocked() );

    menu->addSeparator();

    action = menu->addAction( qtr( "Always on &top" ) );
    action->setCheckable( true );
    action->setChecked( mi->isInterfaceAlwaysOnTop() );
    connect( action, &QAction::triggered, mi, &MainCtx::setInterfaceAlwaysOnTop );

    menu->addSeparator();

    {
        bool isPlayerVisible = mi->getMainInterfaceModes() & MainCtx::MAININTERFACE_MODE_PLAYER;
        action = menu->addAction(qtr("&Big Player View"));
        action->setCheckable( true );
        action->setChecked( isPlayerVisible );
        connect( action, &QAction::triggered, mi, &MainCtx::setPlayerView);
    }

    /* FullScreen View */
    action = menu->addAction( qtr( "&Fullscreen Interface" ), mi,
            &MainCtx::toggleInterfaceFullScreen );
    action->setShortcut( QString( "F11" ) );
    action->setCheckable( true );
    action->setChecked( mi->interfaceVisibility() == QWindow::FullScreen );

    /*Minimal View*/
    action = menu->addAction( qtr( "&Minimal View" ) );
    action->setShortcut(QString( "Ctrl+H" ));
    action->setCheckable( true );
    connect( action, &QAction::triggered, mi, &MainCtx::setMinimalView );
    action->setChecked( mi->isMinimalView() );

    action = menu->addAction( qtr( "&View Items as Grid" ), mi,
            &MainCtx::setGridView );
    action->setCheckable( true );
    action->setChecked( mi->hasGridView() );

    menu->addMenu( new CheckableListMenu(qtr( "&Color Scheme" ), mi->getColorScheme(), QActionGroup::ExclusionPolicy::Exclusive, menu) );

    menu->addSeparator();

    InterfacesMenu( p_intf, menu );
    menu->addSeparator();

    /* Extensions */
    ExtensionsMenu( p_intf, menu );
}

/**
 * Interface Sub-Menu, to list extras interface and skins
 **/
void VLCMenuBar::InterfacesMenu( qt_intf_t *p_intf, QMenu *current )
{
    assert(current);
    VLCVarChoiceModel* model = new VLCVarChoiceModel(VLC_OBJECT(p_intf->intf), "intf-add", current);
    CheckableListMenu* submenu = new CheckableListMenu(qtr("Interfaces"), model, QActionGroup::ExclusionPolicy::None, current);
    current->addMenu(submenu);
}

/**
 * Extensions menu: populate the current menu with extensions
 **/
void VLCMenuBar::ExtensionsMenu( qt_intf_t *p_intf, QMenu *extMenu )
{
    /* Get ExtensionsManager and load extensions if needed */
    ExtensionsManager *extMgr = ExtensionsManager::getInstance( p_intf );

    if( !var_InheritBool( p_intf, "qt-autoload-extensions")
        && !extMgr->isLoaded() )
    {
        return;
    }

    if( !extMgr->isLoaded() && !extMgr->cannotLoad() )
    {
        extMgr->loadExtensions();
    }

    /* Let the ExtensionsManager build itself the menu */
    extMenu->addSeparator();
    extMgr->menu( extMenu );
}

void VLCMenuBar::VolumeEntries( qt_intf_t *p_intf, QMenu *current )
{
    current->addSeparator();

    current->addAction( RedesignMenuStyle::menuIcon( ":/menu/volume-high.svg" ), qtr( "&Increase Volume" ), THEMIM, &PlayerController::setVolumeUp );
    current->addAction( RedesignMenuStyle::menuIcon( ":/menu/volume-low.svg" ), qtr( "&Decrease Volume" ), THEMIM, &PlayerController::setVolumeDown );
    current->addAction( RedesignMenuStyle::menuIcon( ":/menu/volume-muted.svg" ), qtr( "&Mute" ), THEMIM, &PlayerController::toggleMuted );
}

/**
 * Main Audio Menu
 **/
void VLCMenuBar::AudioMenu( qt_intf_t *p_intf, QMenu * current )
{
    if( current->isEmpty() )
    {
        current->addMenu(new CheckableListMenu(qtr( "Audio &Track" ), THEMIM->getAudioTracks(), QActionGroup::ExclusionPolicy::ExclusiveOptional, current));

        current->addMenu(new CheckableListMenu(qtr( "&Audio Device" ), THEMIM->getAudioDevices(), QActionGroup::ExclusionPolicy::Exclusive, current));

        VLCVarChoiceModel *mix_mode = THEMIM->getAudioMixMode();
        if (mix_mode->rowCount() == 0)
            current->addMenu( new CheckableListMenu(qtr( "&Stereo Mode" ), THEMIM->getAudioStereoMode(), QActionGroup::ExclusionPolicy::Exclusive, current) );
        else
            current->addMenu( new CheckableListMenu(qtr( "&Mix Mode" ), mix_mode, QActionGroup::ExclusionPolicy::Exclusive, current) );
        current->addSeparator();

        current->addMenu( new CheckableListMenu(qtr( "&Visualizations" ), THEMIM->getAudioVisualizations(), QActionGroup::ExclusionPolicy::Exclusive, current) );
        VolumeEntries( p_intf, current );
    }
}

/* Subtitles */
void VLCMenuBar::SubtitleMenu( qt_intf_t *p_intf, QMenu *current, bool b_popup )
{
    if( current->isEmpty() || b_popup )
    {
        addDPStaticEntry( current, qtr( "Add &Subtitle File..." ), "",
                &DialogsProvider::loadSubtitlesFile);
        current->addMenu(new CheckableListMenu(qtr( "Sub &Track" ), THEMIM->getSubtitleTracks(), QActionGroup::ExclusionPolicy::ExclusiveOptional, current));
        current->addSeparator();
    }
}

/**
 * Main Video Menu
 * Subtitles are part of Video.
 **/
void VLCMenuBar::VideoMenu( qt_intf_t *p_intf, QMenu *current )
{
    if( current->isEmpty() )
    {
        current->addMenu(new CheckableListMenu(qtr( "Video &Track" ), THEMIM->getVideoTracks(), QActionGroup::ExclusionPolicy::None, current));

        current->addSeparator();
        current->addMenu( new CheckableListMenu(qtr( "&3D Output" ), THEMIM->getVideoStereoMode(), QActionGroup::ExclusionPolicy::Exclusive, current) );

        current->addSeparator();
        /* Surface modifiers */
        current->addAction(new BooleanPropertyAction(qtr( "&Fullscreen"), THEMIM, "fullscreen", current));
        QAction* action = new BooleanPropertyAction(qtr( "Always Fit &Window"), THEMIM, "autoscale", current);
        action->setEnabled( THEMIM->hasVideoOutput() );
        connect(THEMIM, &PlayerController::hasVideoOutputChanged, action, &QAction::setEnabled);
        current->addAction(action);
        current->addAction(new BooleanPropertyAction(qtr( "Set as Wall&paper"), THEMIM, "wallpaperMode", current));

        current->addSeparator();
        /* Size modifiers */
        current->addMenu( new CheckableListMenu(qtr( "&Zoom" ), THEMIM->getZoom(), QActionGroup::ExclusionPolicy::Exclusive, current) );
        current->addMenu( new CheckableListMenu(qtr( "&Aspect Ratio" ), THEMIM->getAspectRatio(), QActionGroup::ExclusionPolicy::Exclusive, current) );
        current->addMenu( new CheckableListMenu(qtr( "&Crop" ), THEMIM->getCrop(), QActionGroup::ExclusionPolicy::Exclusive, current) );
        current->addMenu( new CheckableListMenu(qtr( "&Fit" ), THEMIM->getFit(), QActionGroup::ExclusionPolicy::Exclusive, current) );

        current->addSeparator();
        /* Rendering modifiers */
        current->addMenu( new CheckableListMenu(qtr( "&Deinterlace" ), THEMIM->getDeinterlace(), QActionGroup::ExclusionPolicy::Exclusive, current) );
        current->addMenu( new CheckableListMenu(qtr( "&Deinterlace mode" ), THEMIM->getDeinterlaceMode(), QActionGroup::ExclusionPolicy::Exclusive, current) );

        current->addSeparator();
        /* Other actions */
        QAction* snapshotAction = new QAction(qtr( "Take &Snapshot" ), current);
        connect(snapshotAction, &QAction::triggered, THEMIM, &PlayerController::snapshot);
        current->addAction(snapshotAction);
    }
}

/**
 * Navigation Menu
 * For DVD, MP4, MOV and other chapter based format
 **/
void VLCMenuBar::NavigMenu( qt_intf_t *p_intf, QMenu *menu, bool playlistEntries )
{
    QAction *action;
    QMenu *submenu;

    menu->addMenu( new CheckableListMenu( qtr( "T&itle" ), THEMIM->getTitles(), QActionGroup::ExclusionPolicy::Exclusive, menu) );
    submenu = new CheckableListMenu( qtr( "&Chapter" ), THEMIM->getChapters(), QActionGroup::ExclusionPolicy::Exclusive, menu );
    submenu->setTearOffEnabled( true );
    menu->addMenu( submenu );
    menu->addMenu( new CheckableListMenu( qtr("&Program") , THEMIM->getPrograms(), QActionGroup::ExclusionPolicy::Exclusive, menu) );

    MainCtx * mi = p_intf->p_mi;

    if (mi && p_intf->p_mi->hasMediaLibrary() )
    {
        MediaLib * mediaLib = mi->getMediaLibrary();

        BookmarkMenu * bookmarks = new BookmarkMenu(mediaLib, p_intf->p_player, menu);

        bookmarks->setTitle(qfut(I_MENU_BOOKMARK));

        action = menu->addMenu(bookmarks);

        action->setData("bookmark");
    }

    menu->addSeparator();

    if ( !VLCMenuBar::rendererMenu )
        VLCMenuBar::rendererMenu = new RendererMenu( NULL, p_intf, p_intf->p_mainPlayerController );

    menu->addMenu( VLCMenuBar::rendererMenu );
    menu->addSeparator();


    PopupMenuControlEntries( menu, p_intf );

    RebuildNavigMenu( p_intf, menu, playlistEntries );
}

void VLCMenuBar::RebuildNavigMenu( qt_intf_t *p_intf, QMenu *menu, bool playlistEntries )
{
    QAction* action;

#define ADD_ACTION( title, slot, visibleSignal, visible ) \
    action = menu->addAction( title, THEMIM,  slot ); \
    if (! visible)\
        action->setVisible(false); \
    connect( THEMIM, visibleSignal, action, &QAction::setVisible );

    ADD_ACTION( qtr("Previous Title"), &PlayerController::titlePrev, &PlayerController::hasTitlesChanged, THEMIM->hasTitles() );
    ADD_ACTION( qtr("Next Title"), &PlayerController::titleNext, &PlayerController::hasTitlesChanged, THEMIM->hasTitles() );
    ADD_ACTION( qtr("Previous Chapter"), &PlayerController::chapterPrev, &PlayerController::hasChaptersChanged, THEMIM->hasChapters() );
    ADD_ACTION( qtr("Next Chapter"), &PlayerController::chapterNext, &PlayerController::hasChaptersChanged, THEMIM->hasChapters() );

#undef ADD_ACTION

    if( playlistEntries )
        PopupMenuPlaylistEntries( menu, p_intf );
}

/**
 * Help/About Menu
**/
void VLCMenuBar::HelpMenu(qt_intf_t *p_intf, QMenu *menu )
{
    addDPStaticEntry( menu, qtr( "&Help" ) ,
        ":/menu/help.svg", &DialogsProvider::helpDialog, "F1" );
    // VideoLAN's update check offers the official VLC: the About of the
    // redesign checks this build's GitHub releases
    addDPStaticEntry( menu, qtr( "Check for &Updates..." ) , "",
                      &DialogsProvider::aboutDialog );
    menu->addSeparator();
    addDPStaticEntry( menu, qfut( I_MENU_ABOUT ), ":/menu/info.svg",
            &DialogsProvider::aboutDialog, "Shift+F1", QAction::AboutRole );
}

/*****************************************************************************
 * Popup menus - Right Click menus                                           *
 *****************************************************************************/

void VLCMenuBar::PopupMenuPlaylistEntries( QMenu *menu, qt_intf_t *p_intf )
{
    QAction *action;
    bool hasInput = THEMIM->hasInput();

    /* Play or Pause action and icon */
    if( !hasInput || THEMIM->getPlayingState() != PlayerController::PLAYING_STATE_PLAYING )
    {
        action = menu->addAction( qtr( "&Play" ), [p_intf]() {
            if ( THEMPL->isEmpty() )
                THEDP->openFileDialog();
            else
                THEMPL->togglePlayPause();
        });
#ifndef __APPLE__ /* No icons in menus in Mac */
        action->setIcon( RedesignMenuStyle::menuIcon( QStringLiteral(":/menu/ic_fluent_play_filled.svg"), menu ) );
#endif
    }
    else
    {
        action = addMPLStaticEntry( p_intf, menu, qtr( "Pause" ),
                ":/menu/ic_pause_filled.svg", &PlaylistController::togglePlayPause );
    }

    /* Stop */
    action = addMPLStaticEntry( p_intf, menu, qtr( "&Stop" ),
            ":/menu/ic_fluent_stop.svg", &PlaylistController::stop );
    if( !hasInput )
        action->setEnabled( false );

    /* Next / Previous */
    bool bPlaylistEmpty = THEMPL->isEmpty();
    QAction* previousAction = addMPLStaticEntry( p_intf, menu, qtr( "Pre&vious" ),
            ":/menu/ic_fluent_previous.svg", &PlaylistController::prev );
    previousAction->setEnabled( !bPlaylistEmpty );
    connect( THEMPL, &PlaylistController::isEmptyChanged, previousAction, &QAction::setDisabled);

    QAction* nextAction = addMPLStaticEntry( p_intf, menu, qtr( "Ne&xt" ),
            ":/menu/ic_fluent_next.svg", &PlaylistController::next );
    nextAction->setEnabled( !bPlaylistEmpty );
    connect( THEMPL, &PlaylistController::isEmptyChanged, nextAction, &QAction::setDisabled);

    action = menu->addAction( qtr( "Record" ), THEMIM, &PlayerController::toggleRecord );
    action->setIcon( RedesignMenuStyle::menuIcon( ":/menu/ic_fluent_record.svg" ) );
    if( !hasInput )
        action->setEnabled( false );
    menu->addSeparator();
}

void VLCMenuBar::PopupMenuControlEntries( QMenu *menu, qt_intf_t *p_intf,
                                        bool b_normal )
{
    QAction *action;
    QMenu *rateMenu = new QMenu( qtr( "Sp&eed" ), menu );
    rateMenu->setTearOffEnabled( true );

    if( b_normal )
    {
        /* Faster/Slower */
        action = rateMenu->addAction( qtr( "&Faster" ), THEMIM,
                                  &PlayerController::faster );
#ifndef __APPLE__ /* No icons in menus in Mac */
        action->setIcon( RedesignMenuStyle::menuIcon( ":/menu/ic_fluent_fast_forward.svg", rateMenu ) );
#endif
    }

    action = rateMenu->addAction( RedesignMenuStyle::menuIcon( ":/menu/ic_fluent_fast_forward.svg", rateMenu ), qtr( "Faster (fine)" ), THEMIM,
                              &PlayerController::littlefaster );

    action = rateMenu->addAction( qtr( "N&ormal Speed" ), THEMIM,
                              &PlayerController::normalRate );

    action = rateMenu->addAction( RedesignMenuStyle::menuIcon( ":/menu/ic_fluent_rewind.svg", rateMenu ), qtr( "Slower (fine)" ), THEMIM,
                              &PlayerController::littleslower );

    if( b_normal )
    {
        action = rateMenu->addAction( qtr( "Slo&wer" ), THEMIM,
                                  &PlayerController::slower );
#ifndef __APPLE__ /* No icons in menus in Mac */
        action->setIcon( RedesignMenuStyle::menuIcon( ":/menu/ic_fluent_rewind.svg", rateMenu ) );
#endif
    }

    action = menu->addMenu( rateMenu );

    menu->addSeparator();

    if( !b_normal ) return;

    action = menu->addAction( qtr( "&Jump Forward" ), THEMIM,
             &PlayerController::jumpFwd );
#ifndef __APPLE__ /* No icons in menus in Mac */
    action->setIcon( RedesignMenuStyle::menuIcon( ":/menu/ic_fluent_skip_forward_10.svg", menu ) );
#endif

    action = menu->addAction( qtr( "Jump Bac&kward" ), THEMIM,
             &PlayerController::jumpBwd );
#ifndef __APPLE__ /* No icons in menus in Mac */
    action->setIcon( RedesignMenuStyle::menuIcon( ":/menu/ic_fluent_skip_back_10.svg", menu ) );
#endif

    action = menu->addAction( qfut( I_MENU_GOTOTIME ), THEDP, &DialogsProvider::gotoTimeDialog );
    action->setShortcut( QKeySequence( "Ctrl+T" ) );

    menu->addSeparator();
}

void VLCMenuBar::PopupMenuStaticEntries( QMenu *menu )
{
    QMenu *openmenu = new QMenu( qtr( "Open Media" ), menu );
    addDPStaticEntry( openmenu, qfut( I_OP_OPF ),
        ":/menu/file.svg", &DialogsProvider::openFileDialog);
    addDPStaticEntry( openmenu, qfut( I_OP_OPDIR ),
        ":/menu/folder.svg", &DialogsProvider::PLOpenDir);
    addDPStaticEntry( openmenu, qtr( "Open &Disc..." ),
        ":/menu/disc.svg", &DialogsProvider::openDiscDialog);
    addDPStaticEntry( openmenu, qtr( "Open &Network..." ),
        ":/menu/network.svg", &DialogsProvider::openNetDialog);
    addDPStaticEntry( openmenu, qtr( "Open &Capture Device..." ),
        ":/menu/capture-card.svg", &DialogsProvider::openCaptureDialog);
    menu->addMenu( openmenu );

    menu->addSeparator();

    addDPStaticEntry( menu, qtr( "Quit" ), ":/menu/exit.svg",
                      &DialogsProvider::quit, "Ctrl+Q", QAction::QuitRole );
}

/* Video Tracks and Subtitles tracks */
QMenu* VLCMenuBar::VideoPopupMenu( qt_intf_t *p_intf, bool show )
{
    QMenu* menu = new VLCMenu(p_intf);
    VideoMenu(p_intf, menu);
    if( show )
        menu->popup( QCursor::pos() );
    return menu;
}

/* Audio Tracks */
QMenu* VLCMenuBar::AudioPopupMenu( qt_intf_t *p_intf, bool show )
{
    QMenu* menu = new VLCMenu(p_intf);
    AudioMenu(p_intf, menu);
    if( show )
        menu->popup( QCursor::pos() );
    return menu;
}

/* Navigation stuff, and general menus ( open ), used only for skins */
QMenu* VLCMenuBar::MiscPopupMenu( qt_intf_t *p_intf, bool show )
{
    QMenu* menu = new VLCMenu(p_intf);

    menu->addSeparator();
    PopupMenuPlaylistEntries( menu, p_intf );

    menu->addSeparator();
    PopupMenuControlEntries( menu, p_intf );

    menu->addSeparator();
    PopupMenuStaticEntries( menu );

    if( show )
        menu->popup( QCursor::pos() );
    return menu;
}

namespace {

// Shortcuts of the redesign menus read "Ctrl + Q"
QString shortcutLabel( const QString &keys )
{
    QStringList parts = keys.split( '+', Qt::SkipEmptyParts );
    if( keys.endsWith( QLatin1String( "++" ) ) )
        parts << QStringLiteral( "+" );
    for( QString &part : parts )
        if( part.size() == 1 )
            part = part.toUpper();
    return parts.join( QLatin1String( " + " ) );
}

// First key bound to a hotkey of the core ("key-stop" -> "S")
QString hotkeyLabel( qt_intf_t *p_intf, const char *hotkey )
{
    char *keys = var_InheritString( p_intf, hotkey );
    if( !keys )
        return {};
    const QString first = QString::fromUtf8( keys ).section( '\t', 0, 0 );
    free( keys );
    return shortcutLabel( first );
}

// QMenu draws the text after a tab in the shortcut column, without binding
// the keys: the hotkeys keep handling them
QString withShortcut( const QString &text, const QString &shortcut )
{
    return shortcut.isEmpty() ? text : text + '\t' + shortcut;
}

// Icon of the redesign set in the accent color (see RedesignMenuStyle)
QIcon redesignIcon( qt_intf_t *p_intf, const char *name )
{
    return RedesignMenuStyle::accentIcon( p_intf->p_mi, QLatin1String( name ) );
}

template<typename Receiver, typename Slot>
QAction *addEntry( QMenu *menu, qt_intf_t *p_intf, const char *icon, const QString &text,
                   const QString &shortcut, Receiver *receiver, Slot slot )
{
    QAction *action = menu->addAction( withShortcut( text, shortcut ), receiver, slot );
    action->setIcon( redesignIcon( p_intf, icon ) );
    return action;
}

QMenu *addSubmenu( QMenu *menu, qt_intf_t *p_intf, const char *icon, const QString &title )
{
    QMenu *submenu = new QMenu( title, menu );
    menu->addMenu( submenu )->setIcon( redesignIcon( p_intf, icon ) );
    return submenu;
}

/**
 * Right-click menu of the redesign (Figma "right-click menu"): playback,
 * Hide UI, the media menus (Playback, Video, Audio, Subtitle), then More
 * Settings, Playlist, Open Media and Quit VLC.
 */
QMenu *RedesignPopupMenu( qt_intf_t *p_intf )
{
    QMenu *menu = new VLCMenu( p_intf );
    /* Figma "right-click menu": 292 wide (the other menus 265) */
    menu->setMinimumWidth( 292 );
    MainCtx *mi = p_intf->p_mi;
    const bool hasInput = THEMIM->hasInput();
    const bool playing = hasInput
        && THEMIM->getPlayingState() == PlayerController::PLAYING_STATE_PLAYING;
    QAction *action;

    /* Playback */
    action = menu->addAction( withShortcut( playing ? qtr( "Pause" ) : qtr( "Play" ),
                                            hotkeyLabel( p_intf, "key-play-pause" ) ),
                              [p_intf]() {
        if( THEMPL->isEmpty() )
            THEDP->openFileDialog();
        else
            THEMPL->togglePlayPause();
    });
    action->setIcon( redesignIcon( p_intf, playing ? "pause" : "play" ) );

    action = addEntry( menu, p_intf, "stop", qtr( "Stop" ), hotkeyLabel( p_intf, "key-stop" ),
                       THEMPL, &PlaylistController::stop );
    action->setEnabled( hasInput );

    const bool emptyQueue = THEMPL->isEmpty();
    action = addEntry( menu, p_intf, "previous", qtr( "Previous Item" ),
                       hotkeyLabel( p_intf, "key-prev" ), THEMPL, &PlaylistController::prev );
    action->setEnabled( !emptyQueue );
    action = addEntry( menu, p_intf, "next", qtr( "Next Item" ),
                       hotkeyLabel( p_intf, "key-next" ), THEMPL, &PlaylistController::next );
    action->setEnabled( !emptyQueue );

    action = addEntry( menu, p_intf, "record", qtr( "Record" ), hotkeyLabel( p_intf, "key-record" ),
                       THEMIM, &PlayerController::toggleRecord );
    action->setEnabled( hasInput );
    menu->addSeparator();

    /* Hide UI: the minimal view, Ctrl + \ (GlobalShortcuts.qml) */
    if( mi )
    {
        action = addEntry( menu, p_intf, "ui", qtr( "Hide UI" ), QStringLiteral( "Ctrl + \\" ),
                           mi, &MainCtx::setMinimalView );
        action->setCheckable( true );
        action->setChecked( mi->isMinimalView() );
        menu->addSeparator();
    }

    /* Media menus */
    if( THEMIM->getInput() )
    {
        QMenu *submenu = addSubmenu( menu, p_intf, "playback", qtr( "Playback" ) );
        VLCMenuBar::NavigMenu( p_intf, submenu, false );

        submenu = addSubmenu( menu, p_intf, "video", qtr( "Video" ) );
        VLCMenuBar::VideoMenu( p_intf, submenu );
        submenu->menuAction()->setEnabled( !submenu->isEmpty() );

        submenu = addSubmenu( menu, p_intf, "audio", qtr( "Audio" ) );
        VLCMenuBar::AudioMenu( p_intf, submenu );
        submenu->menuAction()->setEnabled( !submenu->isEmpty() );

        submenu = addSubmenu( menu, p_intf, "subtitle", qtr( "Subtitle" ) );
        VLCMenuBar::SubtitleMenu( p_intf, submenu, true );
        menu->addSeparator();
    }

    /* More Settings: tools, view and help */
    QMenu *settings = addSubmenu( menu, p_intf, "settings", qtr( "More Settings" ) );
    VLCMenuBar::ToolsMenu( p_intf, settings );
    if( mi )
    {
        QMenu *view = new QMenu( qtr( "View" ), settings );
        VLCMenuBar::ViewMenu( p_intf, view );
        settings->addMenu( view );
    }
    QMenu *help = new QMenu( qtr( "Help" ), settings );
    VLCMenuBar::HelpMenu( p_intf, help );
    settings->addMenu( help );
    {
        // The entries of the Tools menu, by their (translated) labels
        const std::pair<QString, const char *> icons[] = {
            { qtr( "&Effects and Filters" ), "spectrum" },
            { qtr( "&Track Synchronization" ), "playback-settings" },
            { qfut( I_MENU_INFO ), "info" },
            { qfut( I_MENU_CODECINFO ), "info" },
            { qfut( I_MENU_VLM ), "streaming" },
            { qtr( "Program Guide" ), "tv" },
            { qfut( I_MENU_MSG ), "lists" },
            { qtr( "Plu&gins and extensions" ), "plugins" },
            { qtr( "Customi&ze Interface..." ), "paint" },
            { qtr( "&Preferences" ), "settings" },
            { qtr( "View" ), "view" },
            { qtr( "Help" ), "info" },
        };
        for( QAction *entry : settings->actions() )
            for( const auto &icon : icons )
                if( entry->text() == icon.first )
                    entry->setIcon( redesignIcon( p_intf, icon.second ) );
    }

    /* Playlist: the play queue */
    QMenu *playlist = addSubmenu( menu, p_intf, "playlist", qtr( "Playlist" ) );
    if( mi )
    {
        action = playlist->addAction( withShortcut( qtr( "Play Queue" ), QStringLiteral( "Ctrl + L" ) ) );
        action->setCheckable( true );
        action->setChecked( mi->getPlayqueuePanel()->isVisible() );
        QObject::connect( action, &QAction::triggered, mi->getPlayqueuePanel(), &SidePanelCtx::setVisible );
    }
    action = playlist->addAction( qtr( "Shuffle" ), THEMPL, &PlaylistController::toggleRandom );
    action->setCheckable( true );
    action->setChecked( THEMPL->isRandom() );
    {
        /* Repeat has three states: one radio item per mode */
        QMenu *repeat = playlist->addMenu( qtr( "Repeat" ) );
        QActionGroup *repeatGroup = new QActionGroup( repeat );
        const std::pair<QString, PlaylistController::PlaybackRepeat> modes[] = {
            { qtr( "Off" ), PlaylistController::PLAYBACK_REPEAT_NONE },
            { qtr( "Repeat All" ), PlaylistController::PLAYBACK_REPEAT_ALL },
            { qtr( "Repeat One" ), PlaylistController::PLAYBACK_REPEAT_CURRENT },
        };
        for( const auto &mode : modes )
        {
            const PlaylistController::PlaybackRepeat repeatMode = mode.second;
            action = repeat->addAction( mode.first, [playlistController = THEMPL, repeatMode]() {
                playlistController->setRepeatMode( repeatMode );
            });
            action->setCheckable( true );
            action->setChecked( THEMPL->getRepeatMode() == repeatMode );
            repeatGroup->addAction( action );
        }
    }
    playlist->addSeparator();
    playlist->addAction( withShortcut( qfut( I_PL_SAVE ), QStringLiteral( "Ctrl + Y" ) ),
                         THEDP, &DialogsProvider::savePlayingToPlaylist );
    action = playlist->addAction( qtr( "Quit at the end of play queue" ),
                                  [playlistController = THEMPL](bool checked) {
        playlistController->setMediaStopAction( checked ? PlaylistController::MEDIA_STOPPED_EXIT
                                                        : PlaylistController::MEDIA_STOPPED_CONTINUE );
    });
    action->setCheckable( true );
    action->setChecked( THEMPL->getMediaStopAction() == PlaylistController::MEDIA_STOPPED_EXIT );

    /* Open Media */
    QMenu *open = addSubmenu( menu, p_intf, "open", qtr( "Open Media" ) );
    // triggered(bool) would pass false to simpleOpenDialog(bool start)
    action = open->addAction( withShortcut( qtr( "Open File(s)" ), QStringLiteral( "Ctrl + O" ) ),
                              []() { THEDP->simpleOpenDialog(); } );
    /* the icons of the main menu (Figma "menu") */
    action->setIcon( redesignIcon( p_intf, "open" ) );
    addEntry( open, p_intf, "folder-open", qtr( "Open Folder" ), QStringLiteral( "Ctrl + F" ),
              THEDP, &DialogsProvider::PLOpenDir );
    addEntry( open, p_intf, "streaming", qtr( "Open Network Stream" ), QStringLiteral( "Ctrl + N" ),
              THEDP, &DialogsProvider::openNetDialog );
    addEntry( open, p_intf, "disc", qtr( "Open Disc" ), QStringLiteral( "Ctrl + D" ),
              THEDP, &DialogsProvider::openDiscDialog );
    addEntry( open, p_intf, "capture-device", qtr( "Open Capture Device" ), QStringLiteral( "Ctrl + C" ),
              THEDP, &DialogsProvider::openCaptureDialog );
    /* as Ctrl+V (GlobalShortcuts.qml): one location opens the URL dialog,
     * several ask to enqueue them */
    if( mi )
        open->addAction( withShortcut( qtr( "Open Location from Clipboard" ), QStringLiteral( "Ctrl + V" ) ),
                         mi, &MainCtx::pasteFromClipboard );
    else
        open->addAction( withShortcut( qtr( "Open Location from Clipboard" ), QStringLiteral( "Ctrl + V" ) ),
                         THEDP, &DialogsProvider::openUrlDialog );
    open->addSeparator();
    addEntry( open, p_intf, "swap", qtr( "Convert" ), QStringLiteral( "Ctrl + R" ),
              THEDP, &DialogsProvider::openAndTranscodingDialogs );
    addEntry( open, p_intf, "streaming", qtr( "Stream" ), QStringLiteral( "Ctrl + S" ),
              THEDP, &DialogsProvider::openAndStreamingDialogs );

    /* Quit, right under Open Media (no separator in the design) */
    action = addEntry( menu, p_intf, "quit", qtr( "Quit VLC" ), hotkeyLabel( p_intf, "key-quit" ),
                       THEDP, &DialogsProvider::quit );
    action->setMenuRole( QAction::QuitRole );
    return menu;
}

} // anonymous namespace

/* Main Menu that sticks everything together  */
QMenu* VLCMenuBar::PopupMenu( qt_intf_t *p_intf, bool show )
{
    if( !p_intf->b_isDialogProvider )
    {
        QMenu *menu = RedesignPopupMenu( p_intf );
        if( show )
            menu->popup( QCursor::pos() );
        return menu;
    }

    /* Dialog provider (skins): the classic menu */
    QMenu* menu = new VLCMenu(p_intf);
    input_item_t* p_input = THEMIM->getInput();
    QAction *action;
    bool b_isFullscreen = false;

    PopupMenuPlaylistEntries( menu, p_intf );
    menu->addSeparator();

    if( p_input )
    {
        QMenu *submenu;
        SharedVOutThread p_vout = THEMIM->getVout();

        /* Add a fullscreen switch button, since it is the most used function */
        if( p_vout )
        {
            menu->addAction(new BooleanPropertyAction(qtr( "&Fullscreen" ), THEMIM, "fullscreen", menu) );
            menu->addSeparator();
        }

        /* Input menu */

        /* Audio menu */
        submenu = new QMenu( menu );
        AudioMenu( p_intf, submenu );
        action = menu->addMenu( submenu );
        action->setText( qtr( "&Audio" ) );
        if( action->menu()->isEmpty() )
            action->setEnabled( false );

        /* Video menu */
        submenu = new QMenu( menu );
        VideoMenu( p_intf, submenu );
        action = menu->addMenu( submenu );
        action->setText( qtr( "&Video" ) );
        if( action->menu()->isEmpty() )
            action->setEnabled( false );

        /* Subtitles menu */
        submenu = new QMenu( menu );
        SubtitleMenu( p_intf, submenu, true );
        action = menu->addMenu( submenu );
        action->setText( qtr( "Subti&tle") );

        /* Playback menu for chapters */
        submenu = new QMenu( menu );
        NavigMenu( p_intf, submenu );
        action = menu->addMenu( submenu );
        action->setText( qtr( "&Playback" ) );
        if( action->menu()->isEmpty() )
            action->setEnabled( false );
    }

    menu->addSeparator();

    /* Add some special entries for windowed mode: Interface Menu */
    if( !b_isFullscreen )
    {
        if( p_intf->b_isDialogProvider )
        {
            // same as Tool menu but with extra entries
            QMenu* submenu = new QMenu( qtr( "Interface" ), menu );
            ToolsMenu( p_intf, submenu );
            submenu->addSeparator();

            vlc_object_t* p_object = vlc_object_parent(p_intf->intf);

            /* Open skin dialog box */
            if (var_Type(p_object, "intf-skins-interactive") & VLC_VAR_ISCOMMAND)
            {
                QAction* openSkinAction = new QAction(qtr("Open skin..."), menu);
                openSkinAction->setShortcut( QKeySequence( "Ctrl+Shift+S" ));
                connect(openSkinAction, &QAction::triggered, [=]() {
                    var_TriggerCallback(p_object, "intf-skins-interactive");
                });
                submenu->addAction(openSkinAction);
            }

            VLCVarChoiceModel* skinmodel = new VLCVarChoiceModel(p_object, "intf-skins", submenu);
            CheckableListMenu* skinsubmenu = new CheckableListMenu(qtr("Interface"), skinmodel, QActionGroup::ExclusionPolicy::ExclusiveOptional, submenu);
            submenu->addMenu(skinsubmenu);

            submenu->addSeparator();

            /* list of extensions */
            ExtensionsMenu( p_intf, submenu );

            menu->addMenu( submenu );
        }
        else
        {
            QMenu* toolsMenu = new QMenu( qtr( "Tool&s" ), menu );
            ToolsMenu( p_intf, toolsMenu );
            menu->addMenu( toolsMenu );

            QMenu* viewMenu = new QMenu( qtr( "V&iew" ), menu );
            ViewMenu( p_intf, viewMenu );
            menu->addMenu( viewMenu );
        }
    }

    /* Static entries for ending, like open */
    if( p_intf->b_isDialogProvider )
    {

        QMenu* openmenu = new QMenu( qtr( "Open Media" ), menu );
        FileMenu( p_intf, openmenu );
        menu->addMenu( openmenu );

        menu->addSeparator();

        QMenu* helpmenu = new QMenu( qtr( "Help" ), menu );
        HelpMenu( p_intf, helpmenu );
        menu->addMenu( helpmenu );

        addDPStaticEntry( menu, qtr( "Quit" ), ":/menu/exit.svg",
                          &DialogsProvider::quit, "Ctrl+Q", QAction::QuitRole );
    }
    else
        PopupMenuStaticEntries( menu );

    if( show )
        menu->popup( QCursor::pos() );
    return menu;
}
