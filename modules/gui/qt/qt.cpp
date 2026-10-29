/*****************************************************************************
 * qt.cpp : Qt interface
 ****************************************************************************
 * Copyright © 2006-2009 the VideoLAN team
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

#include <qconfig.h>
#include <QtPlugin>
#include <QQuickStyle>

QT_BEGIN_NAMESPACE
#include "plugins.hpp"
QT_END_NAMESPACE

#define VLC_MODULE_LICENSE VLC_LICENSE_GPL_2_PLUS

#include <stdlib.h>
#include <unistd.h>
#ifndef _POSIX_SPAWN
# define _POSIX_SPAWN (-1)
#endif
#if (_POSIX_SPAWN >= 0)
# include <spawn.h>
# include <sys/wait.h>

extern "C" char **environ;
#endif

#ifdef __APPLE__
# include <TargetConditionals.h>
#endif

#include <QApplication>
#include <QFontDatabase>
#include <QLocale>
#include <QThreadPool>
#include <QDate>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QtQuickControls2/QQuickStyle>
#include <QLoggingCategory>
#include <QQmlError>
#include <QList>
#include <QTranslator>

#if __has_include(<rhi/qrhi.h>) // Qt 6.6
#define RHI_HEADER_AVAILABLE
#include <rhi/qrhi.h>
#elif __has_include(<QtGui/private/qrhi_p.h>) && \
      QT_VERSION >= QT_VERSION_CHECK(6, 4, 0) // `QRhi::probe()` is available since Qt 6.4
#define RHI_HEADER_AVAILABLE
#include <QtGui/private/qrhi_p.h>
#include <QtGui/private/qrhigles2_p.h> // for `QRhiGles2InitParams`
#endif
#ifdef RHI_HEADER_AVAILABLE
#include <QOffscreenSurface>
#include <QThreadPool>
#include <QThread>
#include "util/asynctask.hpp"
#endif

#ifdef _WIN32
#include <QOperatingSystemVersion>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "qt.hpp"

#include "player/player_controller.hpp"    /* THEMIM destruction */
#include "playlist/playlist_controller.hpp" /* THEMPL creation */
#include "dialogs/dialogs_provider.hpp" /* THEDP creation */
#include "dialogs/dialogs/dialogmodel.hpp"
#ifdef _WIN32
# include "maininterface/mainctx_win32.hpp"
#include "maininterface/win32windoweffects_module.hpp"
#include "util/csdmenu_win32.h"
#else
# include "maininterface/mainctx.hpp"   /* MainCtx creation */
#endif
#include "style/defaultthemeproviders.hpp"
#include "dialogs/extensions/extensions_manager.hpp" /* Extensions manager */
#include "dialogs/help/help.hpp"     /* Launch Update */
#include "util/dismiss_popup_event_filter.hpp"
#include "maininterface/compositor.hpp"
#include "util/vlctick.hpp"
#include "util/shared_input_item.hpp"
#include "util/vlcqtmessagehandler.hpp"
#include "dialogs/preferences/interface_language.hpp"
#include "network/networkmediamodel.hpp"
#include "network/devicesourceprovider.hpp"
#include "playlist/playlist_common.hpp"
#include "playlist/playlist_item.hpp"
#include "dialogs/dialogs/dialogmodel.hpp"
#include "medialibrary/mlqmltypes.hpp"

#include <QVector>
#include "playlist/playlist_item.hpp"
#include "util/language_change.hpp"

#include <vlc_interface.h>
#include <vlc_plugin.h>
#include <vlc_window.h>
#include <vlc_player.h>
#include <vlc_threads.h>
#include <vlc_messages.h>

#include <QQuickWindow>
#include <QOpenGLContext> // Available in Qt GUI, no need for Qt OpenGL module
#ifdef Q_OS_MACOS
# include <QMenuBar>
# include <vlc_intf_strings.h>
#endif

#ifndef X_DISPLAY_MISSING
# include <vlc_xlib.h>
#endif

#if defined(_WIN32) && defined(__GNUC__) && !defined(__clang__)
#include <cxxabi.h>

/* GCC on MinGW keeps thread_local objects in emulated TLS. When a thread
 * exits, winpthreads runs the pthread key destructors in key creation order,
 * and the emutls key is normally created first: it frees the storage of the
 * thread_local objects before libstdc++ runs their destructors, from its own
 * key. Qt's thread_local QUnifiedTimer then crashes the interface thread on
 * exit. Creating the libstdc++ key before this module uses any thread_local
 * makes the destructors run while their storage is still there. */
static void NoThreadExitCleanup(void *) {}

__attribute__((constructor(101)))
static void CreateThreadLocalDestructorKeyFirst()
{
    __cxxabiv1::__cxa_thread_atexit(NoThreadExitCleanup, nullptr, nullptr);
}
#endif

/*****************************************************************************
 * Local prototypes.
 *****************************************************************************/
static int  OpenInternal ( qt_intf_t * );
static void CloseInternal( qt_intf_t * );
static int  OpenIntf     ( vlc_object_t * );
static int  OpenDialogs  ( vlc_object_t * );
static void Close        ( vlc_object_t * );
static int  WindowOpen   ( vlc_window_t * );
static void ShowDialog   ( intf_thread_t *, int, int, intf_dialog_args_t * );


/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
#define INITIAL_PREFS_VIEW_TEXT N_( "Select the initial preferences view" )
#define INITIAL_PREFS_VIEW_LONGTEXT N_( "Select which preferences view to show upon "\
                                        "opening the preferences dialog." )

#define SYSTRAY_TEXT N_( "Systray icon" )
#define SYSTRAY_LONGTEXT N_( "Show an icon in the systray " \
                             "allowing you to control VLC media player " \
                             "for basic actions." )

#define MINIMIZED_TEXT N_( "Start VLC with only a systray icon" )
#define MINIMIZED_LONGTEXT N_( "VLC will start with just an icon in " \
                               "your taskbar." )

#define KEEPSIZE_TEXT N_( "Resize interface to the native video size" )
#define KEEPSIZE_LONGTEXT N_( "You have two choices:\n" \
            " - The interface will resize to the native video size\n" \
            " - The video will fit to the interface size\n " \
            "By default, interface resize to the native video size." )

#define TITLE_TEXT N_( "Show playing item name in window title" )
#define TITLE_LONGTEXT N_( "Show the name of the song or video in the " \
                           "controller window title." )

#define NOTIFICATION_TEXT N_( "Show notification popup on track change" )
#define NOTIFICATION_LONGTEXT N_( \
    "Show a notification popup with the artist and track name when " \
    "the current play queue item changes, when VLC is minimized or hidden." )

#define OPACITY_TEXT N_( "Windows opacity between 0.1 and 1" )
#define OPACITY_LONGTEXT N_( "Sets the windows opacity between 0.1 and 1 " \
                             "for main interface, play queue and extended panel."\
                             " This option only works with Windows and " \
                             "X11 with composite extensions." )

#define OPACITY_FS_TEXT N_( "Fullscreen controller opacity between 0.1 and 1" )
#define OPACITY_FS_LONGTEXT N_( "Sets the fullscreen controller opacity between 0.1 and 1 " \
                             "for main interface, play queue and extended panel."\
                             " This option only works with Windows and " \
                             "X11 with composite extensions." )

#define INTERFACE_SCALE_TEXT N_( "Initial user scale factor for the interface, between 0.3 and 3.0" )

#define ERROR_TEXT N_( "Show unimportant error and warnings dialogs" )
#define ERROR_LONGTEXT N_( "Show the errors of VLC at the bottom of the window, and ask " \
    "before removing media from a playlist or from the play queue, or deleting " \
    "playlists. Off, the errors are only kept in the messages of VLC and these " \
    "removals are done at once; deleting a file from the disk is always confirmed." )

#define UPDATER_TEXT N_( "Activate the updates availability notification" )
#define UPDATER_LONGTEXT N_( "Activate the automatic notification of new " \
                            "versions of the software. It runs once every " \
                            "two weeks." )

#define QT_QML_DEBUG_TEXT N_( "set the options for qml debugger" )
#define QT_QML_DEBUG_LONGTEXT N_( "set the options for qml debugger (see http://doc.qt.io/qt-5/qtquick-debugging.html#starting-applications)" )

#define UPDATER_DAYS_TEXT N_("Number of days between two update checks")

#define PRIVACY_TEXT N_( "Ask for network policy at start" )

#define RECENTPLAY_TEXT N_( "Save the recently played items in the menu" )

#define RECENTPLAY_FILTER_TEXT N_( "List of words separated by | to filter" )
#define RECENTPLAY_FILTER_LONGTEXT N_( "Regular expression used to filter " \
        "the recent items played in the player." )

#define QT_MODE_TEXT N_( "Selection of the starting mode and look" )
#define QT_MODE_LONGTEXT N_( "Start VLC with:\n" \
                             " - normal mode\n"  \
                             " - a zone always present to show information " \
                                  "as lyrics, album arts...\n" \
                             " - minimal mode with limited controls" )

#define QT_FULLSCREEN_TEXT N_( "Show a controller in fullscreen mode" )
#define QT_FULLSCREEN_LONGTEXT N_( "Show the playback controls in fullscreen when the " \
    "mouse moves. Off, only the keyboard shows them (the I key). The skins read it " \
    "for their fullscreen controller too." )
#define QT_NATIVEOPEN_TEXT N_( "Embed the file browser in open dialog" )

#define FULLSCREEN_NUMBER_TEXT N_( "Define which screen fullscreen goes" )
#define FULLSCREEN_NUMBER_LONGTEXT N_( "Screennumber of fullscreen, instead of " \
                                       "same screen where interface is." )

#define QT_AUTOLOAD_EXTENSIONS_TEXT N_( "Load extensions on startup" )
#define QT_AUTOLOAD_EXTENSIONS_LONGTEXT N_( "Automatically load the "\
                                            "extensions module on startup." )

#define QT_MINIMAL_MODE_TEXT N_("Start in minimal view (without menus)" )

#define QT_BGCONE_TEXT N_( "Display background cone or art" )
#define QT_BGCONE_LONGTEXT N_( "Display the cover art of the audio playing in the " \
                            "player (the cone in the compact view). Off, a plain " \
                            "picture takes its place, which also prevents burning the screen." )
#define QT_BGCONE_EXPANDS_TEXT N_( "Expanding background cone or art" )
#define QT_BGCONE_EXPANDS_LONGTEXT N_( "Background art fits window's size." )

#define QT_DISABLE_VOLUME_KEYS_TEXT N_( "Ignore keyboard volume buttons." )
#define QT_DISABLE_VOLUME_KEYS_LONGTEXT N_(                                             \
    "With this option checked, the volume up, volume down and mute buttons on your "    \
    "keyboard will always change your system volume. With this option unchecked, the "  \
    "volume buttons will change VLC's volume when VLC is selected and change the "      \
    "system volume when VLC is not selected." )

#define QT_PAUSE_MINIMIZED_TEXT N_( "Pause the video playback when minimized" )
#define QT_PAUSE_MINIMIZED_LONGTEXT N_( \
    "With this option enabled, the playback will be automatically paused when minimizing the window." )

#define ICONCHANGE_TEXT N_( "Allow automatic icon changes")
#define ICONCHANGE_LONGTEXT N_( \
    "This option allows the interface to change its icon on various occasions.")

#define VOLUME_MAX_TEXT N_( "Maximum Volume displayed" )

#define AUTORAISE_ON_PLAYBACK_TEXT N_( "When to raise the interface" )
#define AUTORAISE_ON_PLAYBACK_LONGTEXT N_( "This option allows the interface to be shown and raised automatically " \
    "when a video/audio playback starts, or never." )

#define QT_CLIENT_SIDE_DECORATION_TEXT N_( "Enable window titlebar" )
#define QT_CLIENT_SIDE_DECORATION_LONGTEXT N_( "This option enables the title bar. Disabling it will remove " \
    "the titlebar and move window buttons within the interface (Client Side Decoration)" )


#define QT_MENUBAR_TEXT N_( "Show the menu bar" )
#define QT_MENUBAR_LONGTEXT N_( "This option displays the classic menu bar" )

#define QT_PIN_CONTROLS_TEXT N_("Pin video controls")
#define QT_PIN_CONTROLS_LONGTEXT N_("Place video controls above and below the video instead of above")

#define QT_SEEK_THUMBNAIL_TEXT N_("Show thumbnail while scrubbing")
#define QT_SEEK_THUMBNAIL_LONGTEXT N_("Show the frame at the hovered position above the seek bar")
#define QT_SEEK_SUBTITLE_CUES_TEXT N_("Show subtitle lines on the seek bar")
#define QT_SEEK_SUBTITLE_CUES_LONGTEXT N_("Mark on the seek bar where the lines of the subtitle shown are, " \
                                         "when it comes from a subtitle file")

#define FULLSCREEN_CONTROL_PIXELS N_( "Fullscreen controller mouse sensitivity" )

#define QT_COMPOSITOR_TEXT N_("Select Qt video integration backend")
#define QT_COMPOSITOR_LONGTEXT N_("Select Qt video integration backend. Use with care, the interface may not start if an incompatible compositor is selected")

#define SMOOTH_SCROLLING_TEXT N_( "Use smooth scrolling in Flickable based views" )
#define SMOOTH_SCROLLING_LONGTEXT N_( "Deactivating this option will disable smooth scrolling in Flickable based views (such as the Play Queue). " \
                                      "This option is only respected with a scroll handler, see option `qt-use-scroll-handler`." )

#define SCROLL_HANDLER_TEXT N_( "Use Kirigami scroll handler for Flickable based views" )
#define SCROLL_HANDLER_LONGTEXT N_( "Using Kirigami scroll handler may improve scrolling behavior." )

#define SAFE_AREA_TEXT N_( "Safe area for the user interface" )
#define SAFE_AREA_LONGTEXT N_( "Sets the safe area percentage between 0.0 and 100 when you want " \
                               "to ensure the visibility of the user interface on a constrained " \
                               "viewport" )

#define VERBOSE_TEXT N_( "Print Qt's important internal messages" )
#define VERBOSE_LONGTEXT N_( "Enable Qt's all own messaging categories except the debug category." )

#define HIDE_WINDOW_ON_CLOSE_TEXT N_( "Hide the window on close" )
#define HIDE_WINDOW_ON_CLOSE_LONGTEXT N_( "Instead of closing the application, hide the window. This setting is only applicable when system tray icon is enabled." )

#define BACKDROP_BLUR_FILTER_TEXT N_( "Enable backdrop blur filter for the main window if possible" )
#define BACKDROP_BLUR_FILTER_LONGTEXT N_( "If backdrop blur filter is available, use translucency in the backgrounds of certain parts of the user interface " \
    "instead of a solid color. This setting impairs readibility of text." )

#define PIP_MODE_TEXT N_( "Enable picture-in-picture (PiP) mode" )
#define PIP_MODE_LONGTEXT N_( "Picture-in-picture mode allows playing video contained in a smaller area so that the interface remains usable." )

#define ACCENT_COLOR_TEXT N_( "Accent color" )
#define ACCENT_COLOR_LONGTEXT N_( "Adjusts the accent color, special values are 'default' and 'system' (requires Qt 6.6) ." )

/* The keys of live TV over the video (Settings > Shortcuts, "Live TV"): they
 * win over the hotkeys only while a channel plays */
#define TV_CHANNEL_UP_TEXT N_( "Next channel" )
#define TV_CHANNEL_DOWN_TEXT N_( "Previous channel" )
#define TV_PREVIOUS_TEXT N_( "Last channel watched" )
#define TV_LIST_TEXT N_( "Channel list" )
#define TV_INFO_TEXT N_( "Channel banner" )

/* The interface's zoom (Settings > Shortcuts, "Interface"): Ctrl+wheel out of
 * the video, where it sizes the subtitles (key-subtitle-text-scale-up/down) */
#define INTF_ZOOM_IN_TEXT N_( "Zoom the interface in" )
#define INTF_ZOOM_OUT_TEXT N_( "Zoom the interface out" )

static const int initial_prefs_view_list[] = { 0, 1, 2 };
static const char *const initial_prefs_view_list_texts[] =
    { N_("Simple"), N_("Advanced"), N_("Expert") };

static const int i_notification_list[] =
    { NOTIFICATION_NEVER, NOTIFICATION_MINIMIZED, NOTIFICATION_ALWAYS };

static const char *const psz_notification_list_text[] =
    { N_("Never"), N_("When minimized"), N_("Always") };

static const int i_raise_list[] =
    { MainCtx::RAISE_NEVER, MainCtx::RAISE_VIDEO, \
      MainCtx::RAISE_AUDIO, MainCtx::RAISE_AUDIOVIDEO,  };

static const char *const psz_raise_list_text[] =
    { N_( "Never" ), N_( "Video" ), N_( "Audio" ), _( "Audio/Video" ) };

static const char *const compositor_vlc[] = {
    "auto",
#ifdef _WIN32
    "dcomp",
    "platform",
    "win7",
#endif
#ifdef QT_HAS_WAYLAND_COMPOSITOR
    "wayland",
#endif
#ifdef QT_HAS_X11_COMPOSITOR
    "x11",
#endif
    "dummy"
};
static const char *const compositor_user[] = {
    N_("Automatic"),
#ifdef _WIN32
    "Direct Composition",
    "Platform Composition",
    "Windows 7",
#endif
#ifdef QT_HAS_WAYLAND_COMPOSITOR
    "Wayland",
#endif
#ifdef QT_HAS_X11_COMPOSITOR
    N_("X11"),
#endif
    N_("Dummy"),
};

/**********************************************************************/
vlc_module_begin ()
    set_shortname( "Qt" )
    set_description( N_("Qt interface") )
    set_subcategory( SUBCAT_INTERFACE_MAIN )
    set_capability( "interface", 151 )
    set_callbacks( OpenIntf, Close )

    add_shortcut("qt")

    add_bool( "qt-minimal-view", false, QT_MINIMAL_MODE_TEXT,
              nullptr )

    add_bool( "qt-system-tray", true, SYSTRAY_TEXT, SYSTRAY_LONGTEXT)

    add_integer( "qt-notification", NOTIFICATION_MINIMIZED,
                 NOTIFICATION_TEXT,
                 NOTIFICATION_LONGTEXT )
            change_integer_list( i_notification_list, psz_notification_list_text )

    add_bool( "qt-start-minimized", false, MINIMIZED_TEXT,
              MINIMIZED_LONGTEXT)
    add_bool( "qt-pause-minimized", false, QT_PAUSE_MINIMIZED_TEXT,
              QT_PAUSE_MINIMIZED_LONGTEXT )

    add_float_with_range( "qt-opacity", 1., 0.1, 1., OPACITY_TEXT,
                          OPACITY_LONGTEXT )
    add_float_with_range( "qt-fs-opacity", 0.8, 0.1, 1., OPACITY_FS_TEXT,
                          OPACITY_FS_LONGTEXT )

    //qt-interface-scale is stored in Qt config file
    //this option is here to force an initial scale factor at startup
    add_float_with_range( "qt-interface-scale", -1.0, 0.3, 3.0, INTERFACE_SCALE_TEXT,
                          nullptr )
        change_volatile()

    /* Off: the window keeps the size and position the user gave it */
    add_bool( "qt-video-autoresize", false, KEEPSIZE_TEXT,
              KEEPSIZE_LONGTEXT )
    add_bool( "qt-name-in-title", true, TITLE_TEXT,
              TITLE_LONGTEXT )
    add_bool( "qt-fs-controller", true, QT_FULLSCREEN_TEXT,
              QT_FULLSCREEN_LONGTEXT )

    add_string("qt-compositor", "auto", QT_COMPOSITOR_TEXT, QT_COMPOSITOR_LONGTEXT)
            change_string_list(compositor_vlc, compositor_user)

    add_obsolete_bool( "qt-recentplay" )
    add_obsolete_string( "qt-recentplay-filter" )
    add_obsolete_integer( "qt-continue" )

    add_bool( "redesign-update-check", true, N_("Check for VLC Redesign updates"),
              N_("Look for a new release of VLC Redesign on GitHub at startup, at most once a day.") )
    /* Given by vlc-redesign-update.exe and the installer when they open VLC
     * again after an update. Older updaters pass it to newer versions, so
     * it must stay. */
    add_integer( "redesign-resume-time", 0, N_("Resume at"),
                 N_("Seconds into the media of the command line where playback continues, after an update.") )
        change_volatile()
#ifdef UPDATE_CHECK
    /* off: VideoLAN's update feed offers the official VLC, not this build */
    add_bool( "qt-updates-notif", false, UPDATER_TEXT,
              UPDATER_LONGTEXT )
    add_integer_with_range( "qt-updates-days", 3, 0, 180,
              UPDATER_DAYS_TEXT, nullptr )
#endif

#ifdef QT_QML_DEBUG
    add_string( "qt-qmljsdebugger", NULL,
                QT_QML_DEBUG_TEXT, QT_QML_DEBUG_LONGTEXT )
#endif

#ifdef _WIN32
    add_bool( "qt-disable-volume-keys"             /* name */,
              true                                 /* default value */,
              QT_DISABLE_VOLUME_KEYS_TEXT          /* text */,
              QT_DISABLE_VOLUME_KEYS_LONGTEXT      /* longtext */)
#endif

    /* Off: the redesign's own title bar; on macOS a unified one, which
     * keeps the system's traffic lights, full screen in its own Space and
     * edge resizing (maininterface/macos_titlebar.hpp) */
    add_bool( "qt-titlebar", false,
              QT_CLIENT_SIDE_DECORATION_TEXT, QT_CLIENT_SIDE_DECORATION_LONGTEXT )

    add_bool( "qt-menubar", false, QT_MENUBAR_TEXT, QT_MENUBAR_LONGTEXT )

    add_bool( "qt-embedded-open", false, QT_NATIVEOPEN_TEXT,
               nullptr )

    /* On: the video sits between the bars, which only hide in fullscreen */
    add_bool( "qt-pin-controls", true, QT_PIN_CONTROLS_TEXT, QT_PIN_CONTROLS_LONGTEXT )
    add_bool( "qt-seek-thumbnail", true, QT_SEEK_THUMBNAIL_TEXT, QT_SEEK_THUMBNAIL_LONGTEXT )
    add_bool( "qt-seek-subtitle-cues", true, QT_SEEK_SUBTITLE_CUES_TEXT, QT_SEEK_SUBTITLE_CUES_LONGTEXT )


    add_obsolete_bool( "qt-advanced-pref" ) /* since 4.0.0 */
    add_integer( "qt-initial-prefs-view", 0, INITIAL_PREFS_VIEW_TEXT, INITIAL_PREFS_VIEW_LONGTEXT )
        change_integer_range( 0, 2 )
        change_integer_list( initial_prefs_view_list, initial_prefs_view_list_texts )
    /* the error popup (Dialogs.qml) and DialogsProvider::questionDialog */
    add_bool( "qt-error-dialogs", true, ERROR_TEXT,
              ERROR_LONGTEXT )

    add_obsolete_string( "qt-slider-colours")

    add_bool( "qt-privacy-ask", true, PRIVACY_TEXT, nullptr )
        change_private ()

    add_integer( "qt-fullscreen-screennumber", -1, FULLSCREEN_NUMBER_TEXT,
               FULLSCREEN_NUMBER_LONGTEXT )

    add_bool( "qt-autoload-extensions", true,
              QT_AUTOLOAD_EXTENSIONS_TEXT, QT_AUTOLOAD_EXTENSIONS_LONGTEXT )

    add_bool( "qt-bgcone", true, QT_BGCONE_TEXT, QT_BGCONE_LONGTEXT )
    add_bool( "qt-bgcone-expands", false, QT_BGCONE_EXPANDS_TEXT,
              QT_BGCONE_EXPANDS_LONGTEXT )

    add_bool( "qt-icon-change", true, ICONCHANGE_TEXT, ICONCHANGE_LONGTEXT )

    add_integer_with_range( "qt-max-volume", 200, 60, 300, VOLUME_MAX_TEXT, nullptr)

    add_integer_with_range( "qt-fs-sensitivity", 3, 0, 4000, FULLSCREEN_CONTROL_PIXELS,
            nullptr)

    add_integer( "qt-auto-raise", MainCtx::RAISE_VIDEO, AUTORAISE_ON_PLAYBACK_TEXT,
                 AUTORAISE_ON_PLAYBACK_LONGTEXT )
            change_integer_list( i_raise_list, psz_raise_list_text )

    add_bool( "qt-smooth-scrolling", true, SMOOTH_SCROLLING_TEXT, SMOOTH_SCROLLING_LONGTEXT )

    add_bool( "qt-use-scroll-handler", true, SCROLL_HANDLER_TEXT, SCROLL_HANDLER_LONGTEXT )

    add_bool( "qt-verbose", false, VERBOSE_TEXT, VERBOSE_LONGTEXT )

    add_bool( "qt-close-to-system-tray", false, HIDE_WINDOW_ON_CLOSE_TEXT, HIDE_WINDOW_ON_CLOSE_LONGTEXT )

    add_bool( "qt-backdrop-blur", true, BACKDROP_BLUR_FILTER_TEXT, BACKDROP_BLUR_FILTER_LONGTEXT )

    add_bool( "qt-pip-mode", true, PIP_MODE_TEXT, PIP_MODE_LONGTEXT )

    add_float_with_range( "qt-safe-area", 0, 0, 100.0, SAFE_AREA_TEXT, SAFE_AREA_LONGTEXT )

    add_string( "qt-accent-color", "default", ACCENT_COLOR_TEXT, ACCENT_COLOR_LONGTEXT )

    /* edited in the shortcuts only (no global hotkey: the interface's) */
    set_subcategory( SUBCAT_HIDDEN )
    add_string_inner( CONFIG_ITEM_KEY, "qt-key-tv-channel-up", TV_CHANNEL_UP_TEXT, nullptr, "Page Up" )
    add_string_inner( CONFIG_ITEM_KEY, "qt-key-tv-channel-down", TV_CHANNEL_DOWN_TEXT, nullptr, "Page Down" )
    add_string_inner( CONFIG_ITEM_KEY, "qt-key-tv-previous", TV_PREVIOUS_TEXT, nullptr, "Backspace" )
    add_string_inner( CONFIG_ITEM_KEY, "qt-key-tv-list", TV_LIST_TEXT, nullptr, "Enter" )
    add_string_inner( CONFIG_ITEM_KEY, "qt-key-tv-info", TV_INFO_TEXT, nullptr, "i" )
#ifdef __APPLE__
    add_string_inner( CONFIG_ITEM_KEY, "qt-key-intf-zoom-in", INTF_ZOOM_IN_TEXT, nullptr, "Command+Mouse Wheel Up" )
    add_string_inner( CONFIG_ITEM_KEY, "qt-key-intf-zoom-out", INTF_ZOOM_OUT_TEXT, nullptr, "Command+Mouse Wheel Down" )
#else
    add_string_inner( CONFIG_ITEM_KEY, "qt-key-intf-zoom-in", INTF_ZOOM_IN_TEXT, nullptr, "Ctrl+Mouse Wheel Up" )
    add_string_inner( CONFIG_ITEM_KEY, "qt-key-intf-zoom-out", INTF_ZOOM_OUT_TEXT, nullptr, "Ctrl+Mouse Wheel Down" )
#endif

    cannot_unload_broken_library()

    add_submodule ()
        set_description( "Dialogs provider" )
        set_capability( "dialogs provider", 51 )

        set_callbacks( OpenDialogs, Close )

    add_submodule ()
        set_capability( "vout window", 0 )
        set_callback( WindowOpen )

#ifdef _WIN32
    add_submodule ()
        set_capability( "qt theme provider", 10 )
        set_callback( WindowsThemeProviderOpen )
        set_description( "Qt Windows theme" )
        add_shortcut("qt-themeprovider-windows")

    add_submodule ()
        add_shortcut( "QtWin32WindowEffects" )
        set_description( "Provides window effects on Windows." )
        set_capability( "qtwindoweffects", 10 )
        set_callback( QtWin32WindowEffectsOpen )
    add_submodule ()
        add_shortcut( "QtWin32CSDMenu" )
        set_description( "Provides csd menu on Windows." )
        set_capability( "qtcsdmenu", 10 )
        set_callback( QtWin32CSDMenuOpen )
#endif
    add_submodule()
        set_capability("qt theme provider", 1)
        set_description( "Qt basic system theme" )
        set_callback( SystemPaletteThemeProviderOpen )
        add_shortcut("qt-themeprovider-systempalette")
vlc_module_end ()

/*****************************************/

Q_DECLARE_METATYPE(QQmlError)

/* Ugly, but the Qt interface assumes single instance anyway */
static qt_intf_t* g_qtIntf = nullptr;
static vlc::threads::condition_variable wait_ready;
static vlc::threads::mutex lock;
static bool busy = false;
static enum {
    OPEN_STATE_INIT,
    OPEN_STATE_OPENED,
    OPEN_STATE_ERROR,
} open_state = OPEN_STATE_INIT;


enum CleanupReason {
    CLEANUP_ERROR,
    CLEANUP_APP_TERMINATED,
    CLEANUP_INTF_CLOSED
};

static inline void triggerQuit()
{
    QMetaObject::invokeMethod(qApp, []() {
            qApp->closeAllWindows();
            //at this point both UI and Vout have ended, stop Qt mainloop
            qApp->exit();
        }, Qt::QueuedConnection);
}

class Translator : public QTranslator
{
public:
    explicit Translator(QObject* parent) : QTranslator(parent) { }

    bool isEmpty() const override { return false; };

    QString translate(const char *context,
                      const char *sourceText,
                      const char *disambiguation = nullptr,
                      int n = -1) const override
    {
        Q_UNUSED(context);

        const char* text;
        if (n >= 0 && disambiguation != nullptr)
            text = vlc_ngettext(sourceText, disambiguation, n);
        else if (disambiguation != nullptr && disambiguation[0] != '\0')
        {
            // qsTr("Tracks", "media streams"): the msgctxt of a text that
            // means something else in another place
            const QByteArray msgid = QByteArray(disambiguation) + '\004' + sourceText;
            text = vlc_gettext(msgid.constData());
            if (text == msgid.constData())
                text = sourceText;
        }
        else
            text = vlc_gettext(sourceText);

        assert(text);
        const bool untranslated = text == sourceText || (n >= 0 && text == disambiguation);
        if (untranslated && QGuiApplication::layoutDirection() == Qt::RightToLeft)
            return leftToRight(QString::fromUtf8(text));
        return QString::fromUtf8(text);
    }

private:
    // A text left in English in a right-to-left interface (Hebrew, Arabic)
    // is still read from left to right: its punctuation at an end ("Advanced
    // options...", "Name:", "(default)") would take the direction of the
    // paragraph and show on the other side, the dots first. A left-to-right
    // mark at each end keeps it with the words.
    static QString leftToRight(const QString &text)
    {
        bool latin = false;
        for (const QChar c : text)
        {
            switch (c.direction())
            {
            case QChar::DirR: case QChar::DirAL:
                return text;
            case QChar::DirL:
                latin = true;
                break;
            default:
                break;
            }
        }
        if (!latin)
            return text;
        const auto neutral = [](QChar c) {
            const QChar::Direction d = c.direction();
            return d != QChar::DirL && d != QChar::DirEN && d != QChar::DirAN;
        };
        if (!neutral(text.front()) && !neutral(text.back()))
            return text;
        static const QChar mark(0x200E);
        return mark + text + mark;
    }
};

/*****************************************************************************
 * Module callbacks
 *****************************************************************************/

static void *Thread( void * );
static void *ThreadCleanup( qt_intf_t *p_intf, CleanupReason cleanupReason );

#ifdef Q_OS_MAC
/* Used to abort the app.exec() on OSX after libvlc_Quit is called */
#include <CoreFoundation/CFRunLoop.h>
#endif

/* Open Interface */
static int OpenInternal( qt_intf_t *p_intf )
{
#ifndef X_DISPLAY_MISSING
    if (!vlc_xlib_init(&p_intf->obj))
        return VLC_EGENERIC;
#endif

#if (_POSIX_SPAWN >= 0) && (!defined(__APPLE__) || TARGET_OS_OSX)
    /* Check if QApplication works */
    /* Note: Disabled on iOS/tvOS/visionOS as process spawning is not allowed */
    char *path = config_GetSysPath(VLC_PKG_LIBEXEC_DIR, "vlc-qt-check");
    if (unlikely(path == NULL))
        return VLC_ENOMEM;

    char *argv[] = { path, NULL };
    pid_t pid;

    int val = posix_spawn(&pid, path, NULL, NULL, argv, environ);
    free(path);
    if (val)
        return VLC_ENOMEM;

    int status;
    while (waitpid(pid, &status, 0) == -1);

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
    {
        msg_Dbg(p_intf, "Qt check failed (%d). Skipping.", status);
        return VLC_EGENERIC;
    }
#endif

    /* Get the playlist before the lock to avoid a lock-order-inversion */
    vlc_playlist_t *playlist = vlc_intf_GetMainPlaylist(p_intf->intf);

    vlc::threads::mutex_locker locker (lock);
    if (busy || open_state == OPEN_STATE_ERROR)
    {
        if (busy)
            msg_Err (p_intf, "cannot start Qt multiple times");
        return VLC_EGENERIC;
    }

    p_intf->p_mi = NULL;

    /* set up the playlist to work on */
    p_intf->p_playlist = playlist;
    p_intf->p_player = vlc_playlist_GetPlayer( p_intf->p_playlist );

    /* */
#ifdef Q_OS_MAC
    /* Run mainloop on the main thread as Cocoa requires */
    CFRunLoopPerformBlock(CFRunLoopGetMain(), kCFRunLoopDefaultMode, ^{
        Thread(static_cast<void*>(p_intf));
        vlc_sem_post(&p_intf->wait_quit);
    });
    CFRunLoopWakeUp(CFRunLoopGetMain());
#else
    if( vlc_clone( &p_intf->thread, Thread, p_intf ) )
    {
        return VLC_ENOMEM;
    }
#endif

    /* Wait for the interface to be ready. This prevents the main
     * LibVLC thread from starting video playback before we can create
     * an embedded video window. */
    while (open_state == OPEN_STATE_INIT)
        wait_ready.wait(lock);

    if (open_state == OPEN_STATE_ERROR)
    {
#ifndef Q_OS_MAC
        vlc_join (p_intf->thread, NULL);
#endif
        return VLC_EGENERIC;
    }

    busy = true;
    return VLC_SUCCESS;
}


static void CloseInternal( qt_intf_t *p_intf )
{
    /* And quit */
    msg_Dbg( p_intf, "requesting exit..." );

    triggerQuit();

    msg_Dbg( p_intf, "waiting for UI thread..." );
#ifndef Q_OS_MAC
    vlc_join (p_intf->thread, NULL);
#else
    vlc_sem_wait(&p_intf->wait_quit);
#endif


    //mutex scope
    {
        vlc::threads::mutex_locker locker (lock);
        assert (busy);
        assert (open_state == OPEN_STATE_INIT);
        busy = false;
    }
    vlc_LogDestroy(p_intf->obj.logger);
    vlc_object_delete(p_intf);
}


/* Open Qt interface */
static int OpenIntfCommon( vlc_object_t *p_this, bool dialogProvider )
{
    auto intfThread = reinterpret_cast<intf_thread_t*>(p_this);
    libvlc_int_t *libvlc = vlc_object_instance( p_this );

    /* Ensure initialization of objects in qt_intf_t. */
    auto p_intf = vlc_object_create<qt_intf_t>( libvlc );
    if (!p_intf)
        return VLC_ENOMEM;
    p_intf->obj.logger = vlc_LogHeaderCreate(libvlc->obj.logger, "qt");
    if (!p_intf->obj.logger)
    {
        vlc_object_delete(p_intf);
        return VLC_EGENERIC;
    }
    vlc_sem_init(&p_intf->wait_quit, 0);
    p_intf->intf = intfThread;
    p_intf->b_isDialogProvider = dialogProvider;
    p_intf->isShuttingDown = false;
    p_intf->refCount = 1;

    int ret = OpenInternal(p_intf);
    if (ret != VLC_SUCCESS)
    {
        vlc_LogDestroy(p_intf->obj.logger);
        vlc_object_delete(p_intf);
        return VLC_EGENERIC;
    }
    intfThread->pf_show_dialog = p_intf->pf_show_dialog;
    intfThread->p_sys = reinterpret_cast<intf_sys_t*>(p_intf);

    //mutex scope
    {
        vlc::threads::mutex_locker locker (lock);
        g_qtIntf = p_intf;
    }
    return VLC_SUCCESS;
}

static int OpenIntf( vlc_object_t *p_this )
{
    return OpenIntfCommon(p_this, false);
}

/* Open Dialog Provider */
static int OpenDialogs( vlc_object_t *p_this )
{
    return OpenIntfCommon(p_this, false);
}

/* close interface */
static void Close( vlc_object_t *p_this )
{
    intf_thread_t* intfThread = (intf_thread_t*)(p_this);
    auto p_intf = reinterpret_cast<qt_intf_t*>(intfThread->p_sys);
    if (p_intf)
    {
        //cleanup the interface
        QMetaObject::invokeMethod( p_intf->p_mi, [p_intf] () {
            ThreadCleanup(p_intf, CLEANUP_INTF_CLOSED);
        }, Qt::BlockingQueuedConnection);

        bool shutdown = false;
        //mutex scope
        {
            vlc::threads::mutex_locker locker(lock);
            assert(g_qtIntf == p_intf);
            p_intf->refCount -= 1;
            if (p_intf->refCount == 0)
            {
                shutdown = true;
                g_qtIntf = nullptr;
            }
            else
                p_intf->isShuttingDown = true;
        }

        if (shutdown)
            CloseInternal(p_intf);
    }
}

static inline void registerMetaTypes()
{
    qRegisterMetaType<size_t>();
    qRegisterMetaType<ssize_t>();
    qRegisterMetaType<vlc_tick_t>();

    qRegisterMetaType<VLCTime>();
    qRegisterMetaType<VLCDuration>();
    qRegisterMetaType<QList<TimedText>>("QList<TimedText>");
    qRegisterMetaType<SharedInputItem>();
    qRegisterMetaType<NetworkTreeItem>();
    qRegisterMetaType<Playlist>();
    qRegisterMetaType<PlaylistItem>();
    qRegisterMetaType<DialogId>();
    qRegisterMetaType<MLItemId>();
    qRegisterMetaType<QVector<MLItemId>>();
    qRegisterMetaType<QList<QQmlError>>();
    qRegisterMetaType<VLCDialog::QuestionType>();
}

static void *Thread( void *obj )
{
    qt_intf_t *p_intf = (qt_intf_t *)obj;

    auto vlcQtMessageHandler = VlcQtMessageHandlerRegisterer{VLC_OBJECT(p_intf)};

    char vlc_name[] = "vlc"; /* for WM_CLASS */
    char *argv[3] = { nullptr };
    int argc = 0;

#if !defined(__APPLE__)
    vlc_thread_set_name("vlc-qt");
#endif

    auto argvReleaser = vlc::wrap_carray<char*>(argv, [](char* ptr[]) {
        for ( int i = 0; ptr[i] != nullptr; ++i )
            free(ptr[i]);
    });
    argv[argc++] = strdup(vlc_name);

#ifdef QT_QML_DEBUG
    char* qmlJsDebugOpt = var_InheritString(p_intf, "qt-qmljsdebugger");
    if (qmlJsDebugOpt)
    {
        msg_Dbg(p_intf, "option qt-qmljsdebugger is %s", qmlJsDebugOpt);
        char* psz_debug_opt;
        if (asprintf(&psz_debug_opt, "-qmljsdebugger=%s", qmlJsDebugOpt) < 0)
        {
            free(qmlJsDebugOpt);
            return NULL;
        }
        argv[argc++] = psz_debug_opt;
        free(qmlJsDebugOpt);
    }
#endif
    argv[argc] = NULL;

    //qml modules are always statically linked
    Q_INIT_RESOURCE( dialogs_assets );
    Q_INIT_RESOURCE( maininterface_assets );
    Q_INIT_RESOURCE( menus_assets );
    Q_INIT_RESOURCE( maininterface_assets );
    Q_INIT_RESOURCE( medialibrary_assets );
    Q_INIT_RESOURCE( network_assets );
    Q_INIT_RESOURCE( player_assets );
    Q_INIT_RESOURCE( playercontrols_assets );
    Q_INIT_RESOURCE( playlist_assets );
    Q_INIT_RESOURCE( style_assets );
    Q_INIT_RESOURCE( util_assets );
    Q_INIT_RESOURCE( widgets_assets );

#ifdef QT_USE_QMLCACHEGEN
    Q_INIT_RESOURCE( dialogs_cachegen );
    Q_INIT_RESOURCE( maininterface_cachegen );
    Q_INIT_RESOURCE( menus_cachegen );
    Q_INIT_RESOURCE( maininterface_cachegen );
    Q_INIT_RESOURCE( medialibrary_cachegen );
    Q_INIT_RESOURCE( network_cachegen );
    Q_INIT_RESOURCE( player_cachegen );
    Q_INIT_RESOURCE( playercontrols_cachegen );
    Q_INIT_RESOURCE( playlist_cachegen );
    Q_INIT_RESOURCE( style_cachegen );
    Q_INIT_RESOURCE( util_cachegen );
    Q_INIT_RESOURCE( widgets_cachegen );
#endif

#ifdef QT_STATIC
    Q_INIT_RESOURCE( assets );
#ifdef _WIN32
    Q_INIT_RESOURCE( windows );
#endif
    Q_INIT_RESOURCE( shaders );

    // Qt Widgets uses shaders with RHI, which is
    // enabled with `QT_WIDGETS_RHI`. Currently the
    // shaders are pre-baked, so we don't need to
    // require Qt shader tools when building qtbase.
    Q_INIT_RESOURCE( gui_shaders );

    // NOTE:  Qt declarative 6.8.0 initializes scenegraph_shaders,
    //        but not scenegraph_curve_shaders. Curve shaders
    //        are used in particular cases even when curve
    //        rendering is not explicitly enabled, this was
    //        observed with underlined text on Windows which lead
    //        access violation in the scene graph thread.
    Q_INIT_RESOURCE( scenegraph_curve_shaders );

    Q_INIT_RESOURCE( qmake_QtQml );
    Q_INIT_RESOURCE( qmake_QtQml_Models );
    Q_INIT_RESOURCE( qmake_QtQml_WorkerScript );
    Q_INIT_RESOURCE( qmake_QtQuick );
    Q_INIT_RESOURCE( qmake_QtQuick_Window );
    Q_INIT_RESOURCE( qmake_QtQuick_Controls );
    Q_INIT_RESOURCE( qmake_QtQuick_Controls_impl );
    Q_INIT_RESOURCE( qmake_QtQuick_Controls_Basic );
    Q_INIT_RESOURCE( qmake_QtQuick_Controls_Basic_impl );
    Q_INIT_RESOURCE( qmake_QtQuick_Layouts );
    Q_INIT_RESOURCE( qmake_QtQuick_Templates );

    Q_INIT_RESOURCE( QuickControls2Basic_raw_qml_0 );
    Q_INIT_RESOURCE( qtquickcontrols2basicstyle );

    // Q_INIT_RESOURCE( qtquickshapes_shaders );
#endif

    auto compositor = var_InheritString(p_intf, "qt-compositor");
    vlc::CompositorFactory compositorFactory(p_intf, compositor);
    free(compositor);

    // Qt's caches (QML compiled at run time, graphics pipelines) in the
    // profile of VLC Redesign, not in %LOCALAPPDATA%\vlc where an installed
    // VLC keeps its own (Qt names the folder after the program)
    if( char *cache = config_GetUserDir( VLC_CACHE_DIR ) )
    {
        const QString dir = QString::fromUtf8( cache ) + QStringLiteral( "/qtcache" );
        free( cache );
        if( QDir().mkpath( dir ) )
        {
            const QByteArray pipelines = QFile::encodeName( QDir::toNativeSeparators( dir + QStringLiteral( "/pipelines.bin" ) ) );
            qputenv( "QML_DISK_CACHE_PATH", QFile::encodeName( QDir::toNativeSeparators( dir + QStringLiteral( "/qml" ) ) ) );
            qputenv( "QSG_RHI_PIPELINE_CACHE_SAVE", pipelines );
            qputenv( "QSG_RHI_PIPELINE_CACHE_LOAD", pipelines );
        }
    }

    // at the moment, the vout is created in another thread than the rendering thread
    QApplication::setAttribute( Qt::AA_DontCheckOpenGLContextThreadAffinity );
    QQuickWindow::setDefaultAlphaBuffer(true);

    /* Start the QApplication here */
    QApplication app( argc, argv );
    app.setProperty("initialStyle", app.style()->name());

    {
        // Install custom translator:
        const auto translator = new Translator(&app);
        const auto ret = QCoreApplication::installTranslator(translator);
        assert(ret);
    }
    {
        // The language VLC started in (bin/winvlc.c), not the one of Windows:
        // its numbers and dates
        const QString iso = interfaceLanguage();
        QLocale::setDefault( QLocale( iso == QLatin1String( "auto" ) ? QLocale::system().name() : iso ) );
#ifndef _WIN32
        // its texts: elsewhere than on Windows, VLC started in the system's
        if( iso != QLatin1String( "auto" ) )
            vlc_settextlanguage( qtu( iso ) );
#endif
        // its fonts for the characters Inter does not have
        applyLanguageFonts( QLocale() );
    }

    /* Ctrl+Q, or more specifically Command+Q on MacOS is going
     * through the NSApplication/NSApplicationDelegate selector
     * applicationShouldTerminate:, which is catched by Qt and
     * trigger termination of the app.exec() runloop.
     * We don't want to quit right now and instead trigger
     * libvlc_Quit() to unload the interface from Close and avoid
     * racing the window by removing the eventloop before it's
     * destroyed. */
    class QuitSpy : public QObject
    {
        qt_intf_t *m_intf;
    public:
        QuitSpy(QObject *parent, qt_intf_t *intf)
             : QObject(parent), m_intf(intf)
        {
            qGuiApp->installEventFilter(this);
        }
        bool eventFilter(QObject *o, QEvent *e) override
        {
            (void)o;
            if (e->type() == QEvent::Quit)
            {
             DialogsProvider::getInstance(m_intf)->quit();
                return true;
            }
            return false;
        }
    };
    QuitSpy quitSpy(&app, p_intf);

    registerMetaTypes();

    //app.setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);

    /* The direction is a setting of the interface, whatever the language:
     * left to right unless right to left is chosen (MainCtx::rightToLeft) */
    app.setLayoutDirection(Qt::LeftToRight);

    /* The redesign uses Inter everywhere: as the application font, it also
     * reaches the widget dialogs and the QML text without a font family */
    {
        const int fontId = QFontDatabase::addApplicationFont( QStringLiteral(":/fonts/InterVariable.ttf") );
        const QStringList families = QFontDatabase::applicationFontFamilies( fontId );
        if( !families.isEmpty() )
        {
            QFont font = app.font();
            font.setFamilies( { families.first() } );
            // Grey scale text without hinting, as the design draws it: the
            // distance field glyphs of Qt Quick come closest to Figma's
            // rendering (measured against the frames of the design; the
            // native glyphs of Windows add ClearType colour fringes and move
            // the glyphs to the pixel grid)
            QQuickWindow::setTextRenderType( QQuickWindow::QtTextRendering );
            // emboldened less at small sizes than Qt does by default
            // (0.065), as Figma draws the glyphs
            if( !qEnvironmentVariableIsSet( "QT_DF_BASEDEVIATION" ) )
                qputenv( "QT_DF_BASEDEVIATION", "0.02" );
            // and grey scale: on an opaque window the distance field text
            // otherwise takes sub-pixel colour fringes, which Figma never draws
            if( !qEnvironmentVariableIsSet( "QSG_DISTANCEFIELD_ANTIALIASING" ) )
                qputenv( "QSG_DISTANCEFIELD_ANTIALIASING", "gray" );
            // and the advances of the font itself (not rounded to pixels)
            font.setHintingPreference( QFont::PreferNoHinting );
            app.setFont( font );
        }
    }

    /* The first text in a script Inter does not draw (Hebrew, Arabic,
     * Chinese...) has Qt read every font of the system for the scripts it
     * covers: about 0.2 s of the start in such a language of the interface.
     * The fonts are read on a thread of the pool meanwhile (QFontDatabase
     * is thread-safe), while the interface is created. Not in the other
     * languages: the fonts read take memory (~15 MB) that only a name in
     * such a script needs (the first one holds the interface that long) */
    {
        const QLocale::Script script = QLocale().script();
        if( script != QLocale::LatinScript && script != QLocale::CyrillicScript
            && script != QLocale::GreekScript && script != QLocale::AnyScript )
        {
            QThreadPool::globalInstance()->start([]() {
                const QStringList families = QFontDatabase::families();
                for( const QString &family : families )
                {
                    if( QCoreApplication::closingDown() )
                        return;
                    QFontDatabase::writingSystems( family );
                }
            });
        }
    }

    /* All the settings are in the .conf/.ini style */
#ifdef _WIN32
    char *cConfigDir = config_GetUserDir( VLC_CONFIG_DIR );
    if (likely(cConfigDir != nullptr))
    {
        QString configDir = cConfigDir;
        free( cConfigDir );
        if( configDir.endsWith( "\\vlc" ) )
            configDir.chop( 4 ); /* the "\vlc" dir is added again by QSettings */
        QSettings::setPath( QSettings::IniFormat, QSettings::UserScope, configDir );
    }
#endif

    p_intf->mainSettings = new QSettings(
#ifdef _WIN32
            QSettings::IniFormat,
#else
            QSettings::NativeFormat,
#endif
            QSettings::UserScope, "vlc", "vlc-qt-interface" );

    // It is guaranteed that the returned format is supported, if a format is returned.
    static const auto createCompatibleOpenGLFormat = []() -> std::optional<QSurfaceFormat> {
        QOpenGLContext defaultCtx;

        QSurfaceFormat defaultFormat = defaultCtx.format();

        /// <QSGDefaultContext::defaultSurfaceFormat()>
        /// This is copied from `QSGDefaultContext::defaultSurfaceFormat()`, since
        /// it is not public. The default `QSurfaceFormat::defaultFormat()` that is
        /// provided by Qt Gui naturally does not consider the default format of
        /// Qt Quick:
        // These depend solely on the env.vars., not QQuickGraphicsConfiguration
        // since that does not have a flag that maps 100% to QSG_NO_xx_BUFFER.
        static bool useDepth = qEnvironmentVariableIsEmpty("QSG_NO_DEPTH_BUFFER");
        static bool useStencil = qEnvironmentVariableIsEmpty("QSG_NO_STENCIL_BUFFER");
        static bool enableDebug = qEnvironmentVariableIsSet("QSG_OPENGL_DEBUG");
        static bool disableVSync = qEnvironmentVariableIsSet("QSG_NO_VSYNC");
        if (useDepth && defaultFormat.depthBufferSize() == -1)
            defaultFormat.setDepthBufferSize(24);
        else if (!useDepth)
            defaultFormat.setDepthBufferSize(0);
        if (useStencil && defaultFormat.stencilBufferSize() == -1)
            defaultFormat.setStencilBufferSize(8);
        else if (!useStencil)
            defaultFormat.setStencilBufferSize(0);
        if (enableDebug)
            defaultFormat.setOption(QSurfaceFormat::DebugContext);
        if (QQuickWindow::hasDefaultAlphaBuffer())
            defaultFormat.setAlphaBufferSize(8);
        defaultFormat.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
        if (disableVSync) // swapInterval defaults to 1, it has no -1 special value
            defaultFormat.setSwapInterval(0);
        /// </QSGDefaultContext::defaultSurfaceFormat()>

        defaultCtx.setFormat(defaultFormat);

        // This is really unnecessary to check, since Qt should not be using unavailable contexts, but nevertheless.
        if (Q_UNLIKELY(!defaultCtx.create()))
            return std::nullopt;

        const QSurfaceFormat format = defaultCtx.format();

        if (format.majorVersion() >= 3) // We need OpenGL (ES) 3.0 since our shaders use `textureSize()`.
            return format;

        QSurfaceFormat compatibleFormat = format;
        compatibleFormat.setMajorVersion(3);
        compatibleFormat.setMinorVersion(0);

        // OpenGL ES does not have core/compatibility profiles:
        if (compatibleFormat.renderableType() == QSurfaceFormat::OpenGL)
            compatibleFormat.setProfile(QSurfaceFormat::CoreProfile);

        QOpenGLContext ctx;
        ctx.setFormat(compatibleFormat);

        if (!ctx.create() || ctx.format().majorVersion() < 3)
        {
            qCritical() << "OpenGL (ES) 3.0 context creation failed.";
            return std::nullopt;
        }
        else
        {
            return compatibleFormat;
        }
    };

#ifdef RHI_HEADER_AVAILABLE
    // NOTE: Qt Quick does not have a cross-API RHI fallback procedure (as of Qt 6.7.1).
    //       We have to manually pick a graphics api here, since the default graphics
    //       api (Direct3D 11.2) may not be supported.
    static const auto probeRhi = []() -> QPair<QSGRendererInterface::GraphicsApi,
                                               bool /* software through rhi, such as d3d warp */> {
#if defined(_WIN32)
        // TODO: Investigate if we should use D3D12. Currently it is not the default by
        //       Qt (as of Qt 6.8), and is not as battle tested as the default D3D11.
#ifndef NDEBUG
        const bool d3d11SdkLayersAvailable = []() {
            std::unique_ptr<std::remove_pointer_t<HMODULE>, BOOL WINAPI (*)(HMODULE)>
                d3d11sdklayerslib(::LoadLibraryEx(TEXT("d3d11_1sdklayers.dll"), nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32), ::FreeLibrary);

            if (d3d11sdklayerslib)
                return true;
            else
                return false;
        }();
#endif

        {
            QRhiD3D11InitParams params;
#ifndef NDEBUG
            if (d3d11SdkLayersAvailable)
                params.enableDebugLayer = true;
#endif
            if (QRhi::probe(QRhi::D3D11, &params))
            {
                return {QSGRendererInterface::Direct3D11, false};
            }
        }
#endif

        if (const std::optional<QSurfaceFormat> format = createCompatibleOpenGLFormat())
        {
            QRhiGles2InitParams params;
            QOffscreenSurface *surface = nullptr; // This is needed to not use explicit cast.

            params.format = *format;

            // Caller takes the ownership of the created fallback surface:
            const auto initializeFallbackSurface = [&params, &surface, &format]() {
                // Due to offscreen surface involvement, this has to be done in the
                // gui thread only:
                surface = new QOffscreenSurface;
                params.fallbackSurface = surface;
                surface->setFormat(*format);
                surface->create();
                assert(surface->isValid()); // The provided format is guaranteed to be valid (see `createCompatibleOpenGLFormat()`).
            };

            const auto isGuiThread = QThread::currentThread() == qApp->thread();

            if (isGuiThread)
                initializeFallbackSurface();
            else
                QMetaObject::invokeMethod(qApp, initializeFallbackSurface, Qt::BlockingQueuedConnection);

            const bool probe = QRhi::probe(QRhi::OpenGLES2, &params);

            if (isGuiThread)
                delete surface;
            else
                surface->deleteLater(); // No need to be blocking.

            if (probe)
            {
                QSurfaceFormat::setDefaultFormat(*format);
                return {QSGRendererInterface::OpenGL, false};
            }
        }

        // TODO: Investigate if using Vulkan makes sense on Windows.
        // TODO: Investigate if it makes sense to try D3D12 when probing D3D11 failed.

#if defined(_WIN32)
        {
            // D3D11 Warp:

            // `QRhi::probe()` does not accept rhi flags, but we can use `QRhi::create()`
            // to see if DirectX Warp would be functional. This is essentially the same
            // as what `::probe()` does, at least for DirectX:
            QRhiD3D11InitParams params;
#ifndef NDEBUG
            if (d3d11SdkLayersAvailable)
                params.enableDebugLayer = true;
#endif
            QRhi *rhi = QRhi::create(QRhi::D3D11, &params, QRhi::PreferSoftwareRenderer);
            if (rhi)
            {
                delete rhi;
                return {QSGRendererInterface::Direct3D11, true};
            }
        }
#endif

        // Qt's own software renderer, it can not display shader effects and is very
        // primitive. Used as last resort:
        return {QSGRendererInterface::Software, false};
    };

    static const char* const asyncRhiProbeCompletedProperty = "asyncRhiProbeCompleted";
    // NOTE: `QSettings` accepts `QAnyStringView` starting from Qt 6.4, use `QLatin1String(View)`:
    static constexpr QLatin1String graphicsApiKey {"graphics-api"};
    static constexpr QLatin1String graphicsApiRhiSoftwareKey {"graphics-api-rhi-software"};
    if (qEnvironmentVariableIsEmpty("QSG_RHI_BACKEND") &&
        qEnvironmentVariableIsEmpty("QT_QUICK_BACKEND") &&
        !uint(qEnvironmentVariableIntValue("QSG_RHI_PREFER_SOFTWARE_RENDERER")))
    {
        const auto enableRhiSoftwareRenderer = []() {
            // We could use `QQuickGraphicsConfiguration::setPreferSoftwareDevice()`, but this
            // is more convenient. We do not need to check if it is already explicitly set by
            // the user, because that is already done above.
            qWarning() << "Qt RHI can not use hardware acceleration, software renderer (such "
                          "as D3D Warp) through rhi is going to be used.";
            qputenv("QSG_RHI_PREFER_SOFTWARE_RENDERER", QByteArrayLiteral("1"));
        };

        const QVariant graphicsApiValue = p_intf->mainSettings->value(graphicsApiKey);
        // settings value can be string (ini file), do not use `typeId()`:
        if (graphicsApiValue.isValid() && Q_LIKELY(graphicsApiValue.canConvert<int>()))
        {

            //some part of the probing needs to be executed on the GUI thread and some part will be ran in a background thread,
            //so we defer the execution of this once the app is started. We can't start the task directly as qApp isn't set yet
            //and there are scenarios where the main loop will never be run.
            QMetaObject::invokeMethod(&app, [&](){
                // A cached (by then) valid graphics api is found, use it:
                QQuickWindow::setGraphicsApi(static_cast<QSGRendererInterface::GraphicsApi>(graphicsApiValue.value<int>()));
                const bool rhiSoftware = p_intf->mainSettings->value(graphicsApiRhiSoftwareKey).value<bool>();
                if (rhiSoftware)
                    enableRhiSoftwareRenderer();

#ifdef _WIN32
                // Direct3D 11 on the hardware is the first choice of the probe:
                // the window's own device checks it, and if that fails the scene
                // graph error removes it for the next start (gracefulExitHandler).
                // Probing again would make one more device, whose memory the
                // driver keeps (about 14 MB).
                if (graphicsApiValue.toInt() == QSGRendererInterface::Direct3D11 && !rhiSoftware)
                    return;
#endif

                // Asynchronous re-probe to see if the cached graphics api is still applicable.
                // If not, QQuickWindow is going to emit scene graph error, and the application is
                // likely going to terminate. However, when the user starts the application again
                // there will not be an error thanks to this. We can not prevent the error, as
                // it is decided to not make compositor initializaation wait to not reduce the startup
                // speed. Startup time is defined as the time it takes to start playing the initial
                // item. Currently the player waits for the interface, and QQuickWindow initialization
                // is therefore not enforced to be synchronous (`QWindow::setVisible(true)` which
                // initializes the scene graph thus rhi is asynchronous).

                class RhiProbeTask : public AsyncTask<QPair<QSGRendererInterface::GraphicsApi, bool>>
                {
                public:
                    RhiProbeTask() = default;

                    QPair<QSGRendererInterface::GraphicsApi, bool> execute() override
                    {
                        return probeRhi();
                    }

                    //prevent the main loop (qApp) from exiting until the task is complete
                    //the lock is taken on RhiProbeTask creation which occurs in the main loop
                    QEventLoopLocker m_applocker;
                };

                const auto rhiProbeTask = QPointer(new RhiProbeTask);
                QObject::connect(rhiProbeTask, &BaseAsyncTask::result, qApp, [rhiProbeTask, settings = QPointer(p_intf->mainSettings)]() {
                    // We can not use `QQuickWindow::setGraphicsApi()` here, as QQuickWindow
                    // may have already tried to initialize the scene graph hence rhi. If the
                    // cached graphics api is not optimal or not available anymore, the next
                    // startup will use the refreshed value. That's the best we can do here
                    // without forcing QQuickWindow to wait (hence delaying startup).
                    assert(settings);
                    const QPair<QSGRendererInterface::GraphicsApi, bool> rhiResult = rhiProbeTask->takeResult();
                    settings->setValue(graphicsApiKey, static_cast<int>(rhiResult.first));
                    settings->setValue(graphicsApiRhiSoftwareKey, rhiResult.second);
                    settings->sync();
                    qApp->setProperty(asyncRhiProbeCompletedProperty, true);
                    qDebug() << "Asynchronous rhi re-probe completed. Graphics api (QSGRendererInterface::GraphicsApi):"
                             << rhiResult.first
                             << "Prefer software renderer:"
                             << rhiResult.second;
                    rhiProbeTask->abandon();
                }, Qt::QueuedConnection);

                const auto globalThreadPool = QThreadPool::globalInstance();
                assert(globalThreadPool);

                if (graphicsApiValue.toInt() == QSGRendererInterface::OpenGL)
                {
                    // Due to asynchronous probing, we need to set the default
                    // format before the main interface window's (`::create()`)
                    // is called. Since previous probe result succeeded with
                    // OpenGL and required version, we can assume that the there
                    // will be again a compatible OpenGL format, rather than
                    // storing and re-using the version (considering that it
                    // takes negligible time to re-create the compatible format):
                    if (const auto format = createCompatibleOpenGLFormat()) /* [[likely]] */
                        QSurfaceFormat::setDefaultFormat(*format);
                }

                rhiProbeTask->start(*globalThreadPool, -1);
            }, Qt::QueuedConnection);
        }
        else
        {
            const auto rhiResult = probeRhi();
            if (rhiResult.second)
                enableRhiSoftwareRenderer();

            const QSGRendererInterface::GraphicsApi graphicsApi = rhiResult.first;
            QQuickWindow::setGraphicsApi(graphicsApi);
            p_intf->mainSettings->setValue(graphicsApiKey, static_cast<int>(graphicsApi));
            p_intf->mainSettings->sync();
        }
    }
#else
    if (qEnvironmentVariable("QT_QUICK_BACKEND", QStringLiteral("rhi")) == QLatin1String("rhi"))
    {
        if (QQuickWindow::graphicsApi() == QSGRendererInterface::OpenGL)
        {
            // At least set the default format without RHI probing if RHI headers are not available:
            if (std::optional<QSurfaceFormat> format = createCompatibleOpenGLFormat())
            {
                QSurfaceFormat::setDefaultFormat(*format);
            }
            else
            {
                qCritical() << "Falling back to software mode...";
                QQuickWindow::setGraphicsApi(QSGRendererInterface::Software);
            }
        }
    }
#endif

    app.setApplicationDisplayName( qtr("VLC media player") );
    app.setApplicationVersion( QString::fromUtf8(VERSION_MESSAGE) );

    if( QDate::currentDate().dayOfYear() >= QT_XMAS_JOKE_DAY && var_InheritBool( p_intf, "qt-icon-change" ) )
        app.setWindowIcon( QIcon::fromTheme( "vlc-xmas", QIcon( ":/logo/vlc128-xmas.png" ) ) );
    else
        // the vector (the cone of vlc.exe's icon): the title bar and the
        // taskbar draw it at their size and the screen's scale
        app.setWindowIcon( QIcon::fromTheme( "vlc", QIcon( ":/logo/vlc.svg" ) ) );

    app.setDesktopFileName( PACKAGE );

    DialogErrorModel::getInstance( p_intf );
    VLCDialogModel::getInstance( p_intf );

    /* Initialize the Dialog Provider and the Main Input Manager */
    DialogsProvider::getInstance( p_intf );
    p_intf->p_mainPlayerController = new PlayerController(p_intf);
    p_intf->p_mainPlaylistController = new vlc::playlist::PlaylistController(p_intf->p_playlist);

    /* Create the normal interface in non-DP mode */
#ifdef _WIN32
    p_intf->p_mi = new MainCtxWin32(p_intf);
#else
    p_intf->p_mi = new MainCtx(p_intf);
#endif

    if( !p_intf->b_isDialogProvider )
    {
        class TouchObserver : public QObject
        {
            const QPointer<MainCtx> m_mainCtx;
            const bool m_persistent;

        public:
            TouchObserver(MainCtx *mainCtx, bool persistent = true)
                : QObject(mainCtx)
                , m_mainCtx(mainCtx)
                , m_persistent(persistent)
            {
                // Touch events can not be filtered through QQuickWindow,
                // that is why the filter is installed on the application
                // instead:
                assert(qApp);
                qApp->installEventFilter(this);
            }

        protected:
            bool eventFilter(QObject*, QEvent* event) override
            {
                if (event->type() == QEvent::TouchBegin)
                {
                    assert(m_mainCtx);

                    m_mainCtx->setUsingTouch(true);

                    if (m_persistent)
                        deleteLater(); // There will be no more use, we can delete it.
                }
                else if (!m_persistent)
                {
                    if (event->type() == QEvent::TouchEnd)
                    {
                        assert(m_mainCtx);

                        m_mainCtx->setUsingTouch(false);
                    }
                }

                return false;
            }
        };

        new TouchObserver(p_intf->p_mi);

        static const auto gracefulExitHandler = [&app, mainCtx = QPointer(p_intf->p_mi), settings = QPointer(p_intf->mainSettings)](QQuickWindow *window) {
            // Graceful exit on error, as if this signal is not connected
            // Qt uses `qFatal()`. With `MainCtx::askToQuit()`, the event loop
            // would be cleared first:
            assert(window);
            QObject::connect(window,
                             &QQuickWindow::sceneGraphError,
                             &app,
                             [&app, mainCtx, settings](QQuickWindow::SceneGraphError error, const QString &message) {
                                 qCritical() << "Compositor: Scene Graph Error: " << error << ", Message: " << message;
                                 assert(mainCtx);
#ifdef _WIN32
                                // This is not really important, as with graceful exit the events in the queue should
                                // be cleared first. This means that in the worst case the application should wait
                                // until asynchronous probing is completed before exiting.
                                 if (!app.property(asyncRhiProbeCompletedProperty).toBool())
                                 {
                                     assert(settings);
                                     settings->remove(graphicsApiKey);
                                     settings->sync();
                                 }
#endif
                                 mainCtx->askToQuit();
                             });
        };

        bool ret = false;
        do {
            p_intf->p_compositor.reset(compositorFactory.createCompositor());
            if (! p_intf->p_compositor)
                break;
            ret = p_intf->p_compositor->makeMainInterface(p_intf->p_mi, gracefulExitHandler);
            if (!ret)
            {
                p_intf->p_compositor->destroyMainInterface();
                p_intf->p_compositor.reset();
            }
        } while(!ret);

        if (!ret)
        {
            msg_Err(p_intf, "unable to create main interface");
            delete p_intf->p_mi;
            p_intf->p_mi = nullptr;
            //process deleteLater events as the main loop will never run
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            return ThreadCleanup( p_intf, CLEANUP_ERROR );
        }

        /* Check window type from the Qt platform back-end */
        bool known_type = true;

        const QString& platform = app.platformName();
        if( platform == QLatin1String("xcb") ) { }
        else if( platform.startsWith( QLatin1String("wayland") ) ) {
            // Workaround for popup widgets not closing on mouse press on wayland:
            app.installEventFilter(new DismissPopupEventFilter(&app));
        }
        else if( platform == QLatin1String("windows") || platform == QLatin1String("direct2d") ) { }
        else if( platform == QLatin1String("cocoa") || platform == QLatin1String("ios") ) { }
        else
        {
            msg_Err( p_intf, "unknown Qt platform: %s", qtu(platform) );
            known_type = false;
        }

        /* FIXME: Temporary, while waiting for a proper window provider API */
        libvlc_int_t *libvlc = vlc_object_instance( p_intf );
        if( known_type )
            var_SetString( libvlc, "window", "qt,any" );
    }

    /* Explain how to show a dialog :D */
    p_intf->pf_show_dialog = ShowDialog;

    /* Tell the main LibVLC thread we are ready */
    {
        vlc::threads::mutex_locker locker (lock);
        assert(g_qtIntf == nullptr);
        g_qtIntf = p_intf;
        open_state = OPEN_STATE_OPENED;
        wait_ready.signal();
    }

    /* Last settings */
    app.setQuitOnLastWindowClosed( false );
    app.setQuitLockEnabled( false );

    /* Loads and tries to apply the preferred QStyle */
#define QTSTYLE_KEY "MainWindow/QtStyle"
    QString s_style = getSettings()->value( QTSTYLE_KEY, "" ).toString();
    if( s_style.compare("") != 0 )
    {
        if ( !QApplication::setStyle( s_style ) )
        {
            // With VLC 3.0 we provide the Fusion style, but not with VLC 4.0.
            // Since we use the same setting key, if the style is no longer
            // available, we clear the setting so that we don't unnecessarily
            // try the style at every start:
            getSettings()->remove( QTSTYLE_KEY );
        }
    }
#undef QTSTYLE_KEY

#ifdef Q_OS_MACOS
    /* The menu bar at the top of the screen: the window keeps the
     * redesign's own menus. Its actions have no shortcut (the QML ones
     * would become ambiguous): About, Settings and Quit go to the
     * application menu by their roles, where Cmd+, and Cmd+Q are the
     * system's. Destroyed after ThreadCleanup, before the QApplication. */
    std::unique_ptr<QMenuBar> macMenuBar;
    if( !p_intf->b_isDialogProvider )
    {
        macMenuBar = std::make_unique<QMenuBar>( nullptr ); // parentless: the global one
        QMenu *media = macMenuBar->addMenu( qtr( "&Media" ) );
        media->addAction( qtr( "Open &File..." ), []() { THEDP->simpleOpenDialog(); } );
        media->addAction( qtr( "Open &Network Stream..." ), []() { THEDP->openNetDialog(); } );
        QAction *about = media->addAction( qfut( I_MENU_ABOUT ), []() { THEDP->aboutDialog(); } );
        about->setMenuRole( QAction::AboutRole );
        QAction *prefs = media->addAction( qtr( "&Preferences" ), []() { THEDP->prefsDialog(); } );
        prefs->setMenuRole( QAction::PreferencesRole );
        prefs->setShortcut( QKeySequence::Preferences );
        QAction *quit = media->addAction( qtr( "Quit" ), []() { THEDP->quit(); } );
        quit->setMenuRole( QAction::QuitRole );
        QMenu *help = macMenuBar->addMenu( qtr( "&Help" ) );
        help->addAction( qtr( "&Help" ), []() { THEDP->helpDialog(); } );
    }
#endif

    /* Launch */
    app.exec();

    msg_Dbg( p_intf, "QApp exec() finished" );

    return ThreadCleanup( p_intf, CLEANUP_APP_TERMINATED );
}

static void *ThreadCleanup( qt_intf_t *p_intf, CleanupReason cleanupReason )
{
    {
        vlc::threads::mutex_locker locker (lock);
        if( cleanupReason == CLEANUP_ERROR )
        {
            open_state = OPEN_STATE_ERROR;
            wait_ready.signal();
        }
        else
            open_state = OPEN_STATE_INIT;
    }

    if ( p_intf->p_mi )
        emit p_intf->p_mi->interfaceUnloading();

    if ( p_intf->p_compositor )
    {
        if (cleanupReason == CLEANUP_INTF_CLOSED)
            p_intf->p_compositor->unloadGUI();
        else // CLEANUP_APP_TERMINATED
            p_intf->p_compositor->destroyMainInterface();
    }

    /* */
    ExtensionsManager::killInstance();

    /* Destroy all remaining windows,
       because some are connected to some slots
       in the MainInputManager
       Settings must be destroyed after that.
     */
    DialogsProvider::killInstance();
    VLCDialogModel::killInstance();
    DialogErrorModel::killInstance();
    MediaSourceCache::killInstance();

    //destroy MainCtx, Compositor shouldn't not use MainCtx after `unloadGUI`
    if (p_intf->p_mi) {
        delete p_intf->p_mi;
        p_intf->p_mi = nullptr;
    }

    if ( p_intf->p_compositor &&  cleanupReason == CLEANUP_APP_TERMINATED)
    {
        p_intf->p_compositor.reset();

        delete p_intf->mainSettings;
        p_intf->mainSettings = nullptr;
    }

    /* Destroy the main playlist controller */
    if (p_intf->p_mainPlaylistController)
    {
        delete p_intf->p_mainPlaylistController;
        p_intf->p_mainPlaylistController = nullptr;
    }

    /* Destroy the main InputManager */
    if (p_intf->p_mainPlayerController)
    {
        delete p_intf->p_mainPlayerController;
        p_intf->p_mainPlayerController = nullptr;
    }

    /* Delete the application automatically */
    return NULL;
}

/*****************************************************************************
 * Callback to show a dialog
 *****************************************************************************/
static void ShowDialog( intf_thread_t *p_intf, int i_dialog_event, int i_arg,
                        intf_dialog_args_t *p_arg )
{
    VLC_UNUSED( p_intf );
    DialogEvent *event = new DialogEvent( i_dialog_event, i_arg, p_arg );
    QApplication::postEvent( THEDP, event );
}

static void WindowCloseCb( vlc_window_t * )
{
    qt_intf_t *p_intf = nullptr;
    bool shutdown = false;
    //mutex scope
    {
        vlc::threads::mutex_locker locker(lock);
        assert(g_qtIntf != nullptr);
        p_intf = g_qtIntf;

        p_intf->refCount -= 1;
        if (p_intf->refCount == 0)
            shutdown = true;
    }
    if (shutdown)
        CloseInternal(p_intf);
}

/**
 * Video output window provider
 */
static int WindowOpen( vlc_window_t *p_wnd ) try
{
    if( !var_InheritBool( p_wnd, "embedded-video" ) )
        return VLC_EGENERIC;

    qt_intf_t *p_intf = nullptr;
    //mutex scope
    {
        vlc::threads::mutex_locker locker(lock);

        p_intf = g_qtIntf;
        if( !p_intf )
        {   /* If another interface is used, this plugin cannot work */
            msg_Dbg( p_wnd, "Qt interface not found" );
            return VLC_EGENERIC;
        }

        if (unlikely(open_state != OPEN_STATE_OPENED))
            return VLC_EGENERIC;

        if (p_intf->isShuttingDown)
            return VLC_EGENERIC;

        {
            const QString& platform = qApp->platformName();
            if (platform == QLatin1String("windows") || platform == QLatin1String("direct2d"))
            {
                if( var_InheritBool( p_wnd, "video-wallpaper" ) )
                    return VLC_EGENERIC;
            }
        }

        bool ret = p_intf->p_compositor->setupVoutWindow( p_wnd, &WindowCloseCb );
        if (ret)
            p_intf->refCount += 1;
        return ret ? VLC_SUCCESS : VLC_EGENERIC;
    }
}
catch (std::bad_alloc&)
{
    return VLC_ENOMEM;
}
