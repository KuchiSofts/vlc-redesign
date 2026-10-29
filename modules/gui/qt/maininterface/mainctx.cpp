/*****************************************************************************
 * main_interface.cpp : Main interface
 ****************************************************************************
 * Copyright (C) 2006-2011 VideoLAN and AUTHORS
 *
 * Authors: Clément Stenac <zorglub@videolan.org>
 *          Jean-Baptiste Kempf <jb@videolan.org>
 *          Ilkka Ollakka <ileoo@videolan.org>
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
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "qt.hpp"

#include <QQmlEngine>
#include <QJSEngine>

#include "mainctx.hpp"

#include <algorithm>
#ifdef _WIN32
# include <windows.h>
#endif
#include "playlist/media.hpp"
#include "mainctx_submodels.hpp"
#include "medialibrary/mlhelper.hpp"

#include "compositor.hpp"
#include "live_settings.hpp"
#include "util/renderer_manager.hpp"
#include "util/csdbuttonmodel.hpp"
#include "util/subtitle_preferences.hpp"

#include "util/vlchotkeyconverter.hpp"
#include "util/qt_dirs.hpp"                     // toNativeSeparators

#include "util/color_scheme_model.hpp"
#include "util/redesign_accent.hpp"
#include "util/model_recovery_agent.hpp"

#include "widgets/native/interface_widgets.hpp"     // bgWidget, videoWidget

#include "playlist/playlist_controller.hpp"
#include "player/player_controller.hpp"

#include "dialogs/dialogs_provider.hpp"
#include "dialogs/systray/systray.hpp"

#include "videosurface.hpp"

#include "menus/menus.hpp"                            // Menu creation
#include "menus/redesign_menu_style.hpp"

#include "dialogs/toolbar/controlbar_profile_model.hpp"
#include "dialogs/toolbar/profile_settings.hpp"
#include "dialogs/help/help.hpp"
#include "dialogs/preferences/interface_language.hpp"
#include "util/language_change.hpp"

#include <QKeyEvent>

#include <QUrl>
#include <QImage>
#include <cmath>
#include <QDate>
#include <QLocale>
#include <QGuiApplication>
#include <QMimeData>
#include <QClipboard>

#include <QQmlProperty>
#include <QQuickWindow>

#include <QWindow>
#include <QScreen>

#include <QOperatingSystemVersion>

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
#include <QStyleHints>
#endif

#ifdef _WIN32
#include <QFileInfo>
#include <QProcess>
#endif

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>

#include <QThread>

#include <vlc_actions.h>
#include <vlc_interface.h>
#include <vlc_playlist.h>
#include <vlc_preparser.h>

#define VLC_REFERENCE_SCALE_FACTOR 96.

using  namespace vlc::playlist;

// #define DEBUG_INTF

/* Callback prototypes */
static int PopupMenuCB( vlc_object_t *p_this, const char *psz_variable,
                        vlc_value_t old_val, vlc_value_t new_val, void *param );
static int IntfShowCB( vlc_object_t *p_this, const char *psz_variable,
                       vlc_value_t old_val, vlc_value_t new_val, void *param );
static int IntfBossCB( vlc_object_t *p_this, const char *psz_variable,
                       vlc_value_t old_val, vlc_value_t new_val, void *param );
static int IntfRaiseMainCB( vlc_object_t *p_this, const char *psz_variable,
                           vlc_value_t old_val, vlc_value_t new_val,
                           void *param );

const QEvent::Type MainCtx::ToolbarsNeedRebuild =
        (QEvent::Type)QEvent::registerEventType();

namespace
{

template <typename T>
T loadVLCOption(vlc_object_t *obj, const char *name);

template <>
int loadVLCOption<int>(vlc_object_t *obj, const char *name)
{
    return var_InheritInteger(obj, name);
}

template <>
float loadVLCOption<float>(vlc_object_t *obj, const char *name)
{
    return var_InheritFloat(obj, name);
}

template <>
bool loadVLCOption<bool>(vlc_object_t *obj, const char *name)
{
    return var_InheritBool(obj, name);
}

}

MainCtx::MainCtx(qt_intf_t *_p_intf)
    : p_intf(_p_intf)
    , m_csdButtonModel {std::make_unique<CSDButtonModel>(this, this)}
{
    // If `playlist-autostart` is set, and there are initial files
    // we don't bother loading main display just to unload it
    // immediately after. Instead, we directly load the player
    // page:
    if (var_InheritBool(p_intf, "playlist-autostart"))
    {
        // We can not use `vlc_playlist_GetCurrentIndex()`, because
        // playlist starts playing after the interface is added.
        // We also can not use the playlist controller because it
        // is not updated yet.
        vlc_playlist_locker lock(p_intf->p_playlist);

        if (vlc_playlist_Count(p_intf->p_playlist) > 0)
        {
            m_mainInterfaceModes |= MAININTERFACE_MODE_PLAYER;
        }
    }

    /**
     *  Configuration and settings
     *  Pre-building of interface
     **/

    settings = getSettings();
    m_colorScheme = new ColorSchemeModel(this);
    // the accent of the system, for "Follow the Windows accent color"
    m_redesignAccent = new RedesignAccent(this);
    connect(m_redesignAccent, &RedesignAccent::systemAccentChanged, this, &MainCtx::systemAccentChanged);
    // Widget menus (right-click, tracks...) get the redesign look; the style
    // lives with the application, as the menus using it may outlive MainCtx
    auto *menuStyle = new RedesignMenuStyle(this);
    menuStyle->setParent(qApp);
    qApp->installEventFilter(menuStyle);

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    connect(m_colorScheme, &ColorSchemeModel::currentChanged, qGuiApp, [colorScheme = m_colorScheme]() {
        QStyleHints *const styleHints = qGuiApp->styleHints();
        if (unlikely(!styleHints))
            return;

        Qt::ColorScheme scheme;
        switch (colorScheme->currentScheme())
        {
        case ColorSchemeModel::ColorScheme::Day:
            scheme = Qt::ColorScheme::Light;
            break;
        case ColorSchemeModel::ColorScheme::Night:
            scheme = Qt::ColorScheme::Dark;
            break;
        case ColorSchemeModel::ColorScheme::System:
        default:
            styleHints->unsetColorScheme();
            return;
        }

        styleHints->setColorScheme(scheme);
    });
#endif

    m_sort = new SortCtx(this);
    m_search = new SearchCtx(this);
    m_playqueuePanel = new SidePanelCtx(this);
    m_navigationPanel = new SidePanelCtx(this);

    // getOSInfo();
    QOperatingSystemVersion currentOS = QOperatingSystemVersion::current();
    switch (currentOS.type()) {
    case QOperatingSystemVersion::OSType::Windows:
        m_osName = Windows;
        break;

    default:
        m_osName = Unknown;
        break;
    }
    m_osVersion = (currentOS.majorVersion());

    // before the updater and the subtitle finder read them
    applySetupChoices();

    // as chosen, else the direction of the language (a right-to-left one
    // chosen before the direction followed it)
    const QVariant rightToLeft = getSettings()->value(QStringLiteral("redesign/right-to-left"));
    setRightToLeft(rightToLeft.isValid() ? rightToLeft.toBool() : languageRightToLeft());

    loadPrefs(false);
    loadFromSettingsImpl(false);

    /* Get the available interfaces */
    m_extraInterfaces = new VLCVarChoiceModel(VLC_OBJECT(p_intf->intf), "intf-add", this);

    vlc_medialibrary_t* ml = vlc_ml_instance_get( p_intf );
    b_hasMedialibrary = (ml != NULL);
    if (b_hasMedialibrary) {
        m_medialib = new MediaLib(p_intf, p_intf->p_mainPlaylistController);
    }

    /* Controlbar Profile Model Creation */
    m_controlbarProfileModel = new ControlbarProfileModel(p_intf->mainSettings, this);

    const QUrl homeUrl = QUrl::fromLocalFile(QVLCUserDir( VLC_HOME_DIR ));
    m_dialogFilepath = getSettings()->value( "filedialog-path", homeUrl ).toUrl();

    QString platformName = QGuiApplication::platformName();

    b_hasWayland = platformName.startsWith(QLatin1String("wayland"), Qt::CaseInsensitive);

    /*********************************
     * Create the Systray Management *
     *********************************/
    //postpone systray initialisation to speedup starting time
    QMetaObject::invokeMethod(this, &MainCtx::initSystray, Qt::QueuedConnection);

    /*************************************************************
     * Connect the input manager to the GUI elements it manages  *
     * Beware initSystray did some connects on input manager too *
     *************************************************************/
    /**
     * Connects on nameChanged()
     * Those connects are different because options can impeach them to trigger.
     **/
    /* Main Interface statusbar */
    /* and title of the Main Interface*/
    connect( THEMIM, &PlayerController::inputChanged, this, &MainCtx::onInputChanged );
    connect( THEMIM, &PlayerController::hasVideoOutputChanged, this, &MainCtx::onVOutChanged );

    /* END CONNECTS ON IM */

    /* VideoWidget connects for asynchronous calls */
    connect( this, &MainCtx::askToQuit, THEDP, &DialogsProvider::quit, Qt::QueuedConnection  );

    /** END of CONNECTS**/


    /************
     * Callbacks
     ************/
    libvlc_int_t* libvlc = vlc_object_instance(p_intf);
    var_AddCallback( libvlc, "intf-toggle-fscontrol", IntfShowCB, p_intf );
    var_AddCallback( libvlc, "intf-boss", IntfBossCB, p_intf );
    var_AddCallback( libvlc, "intf-show", IntfRaiseMainCB, p_intf );

    /* Register callback for the intf-popupmenu variable */
    var_AddCallback( libvlc, "intf-popupmenu", PopupMenuCB, p_intf );

    if( var_InheritBool( p_intf, "qt-privacy-ask") )
    {
        /* Not over a file VLC was opened with (from Explorer): the film
         * first, the welcome the next time VLC opens without one */
        vlc_playlist_t *playlist = vlc_intf_GetMainPlaylist(p_intf->intf);
        vlc_playlist_Lock(playlist);
        const bool has_media = vlc_playlist_Count(playlist) > 0;
        vlc_playlist_Unlock(playlist);

        //postpone dialog call, as composition might not be ready yet
        if (!has_media)
            QMetaObject::invokeMethod(this, [](){
                THEDP->firstRunDialog();
            }, Qt::QueuedConnection);
        else if (m_medialib)
            /* the library is read as usual meanwhile (VLC always opened with
             * a file never read it at start) */
            QMetaObject::invokeMethod(m_medialib, &MediaLib::reload, Qt::QueuedConnection);
    }
    else if (m_medialib)
    {
        QMetaObject::invokeMethod(m_medialib, &MediaLib::reload, Qt::QueuedConnection);
    }

    const struct vlc_preparser_cfg cfg = []{
        struct vlc_preparser_cfg cfg{};
        cfg.types = VLC_PREPARSER_TYPE_PARSE;
        cfg.max_parser_threads = 1;
        cfg.timeout = 0;
        cfg.external_process = false;
        return cfg;
    }();
    m_network_preparser = vlc_preparser_New(VLC_OBJECT(libvlc), &cfg);

#ifdef UPDATE_CHECK
    /* Checking for VLC updates */
    if( var_InheritBool( p_intf, "qt-updates-notif" ) &&
        !var_InheritBool( p_intf, "qt-privacy-ask" ) )
    {
        int interval = var_InheritInteger( p_intf, "qt-updates-days" );
        if( QDate::currentDate() >
            getSettings()->value( "updatedate" ).toDate().addDays( interval ) )
        {
            /* check for update at startup */
            getUpdateModel()->checkUpdate();
            getSettings()->setValue( "updatedate", QDate::currentDate() );
        }
    }
#endif

    m_threadRunner = new ThreadRunner();

    m_hotkeyThread = new QThread(this);
    m_hotkeyThread->setObjectName("vlc-qt-hotkeys");
    m_hotkeyContext = new QObject;
    m_hotkeyContext->moveToThread(m_hotkeyThread);
    connect(m_hotkeyThread, &QThread::finished, m_hotkeyContext, &QObject::deleteLater);
    m_hotkeyThread->start();

    m_liveSettings = std::make_unique<LiveSettings>(p_intf);

    try {
        m_playlistRecoveryAgent = std::make_unique<ModelRecoveryAgent>(settings,
                                                                       QStringLiteral("Playlist"),
                                                                       p_intf->p_mainPlaylistController);
        QMetaObject::invokeMethod(this, [this]() {
            m_playlistRecoveryAgent->prompt(p_intf);
        }, Qt::QueuedConnection);
    } catch (...) { }

    m_initialEffectiveMainInterfaceMode = getEffectiveMainInterfaceMode();
}

MainCtx::~MainCtx()
{
    /* The hotkeys left are dropped; an action still running ends first (as
     * the callbacks it may call go below) */
    m_hotkeysClosed->store(true);
    m_hotkeyThread->quit();
    if (!m_hotkeyThread->wait(QDeadlineTimer(3000)))
    {
        /* it waits for something that waits for this thread: not destroyed
         * running, but when it ends (its context goes with it); no hotkey
         * runs after this one, the interface is gone by then */
        msg_Warn(p_intf, "hotkey action still running at exit");
        m_hotkeyThread->setParent(nullptr);
        connect(m_hotkeyThread, &QThread::finished, m_hotkeyThread, &QObject::deleteLater);
    }

    /* an interface still starting or stopping ends first */
    m_liveSettings.reset();

    /* Save states */

    m_threadRunner->destroy();

    settings->beginGroup("MainWindow");
    settings->setValue( "ShowRemainingTime", m_showRemainingTime );
    settings->setValue( "interface-scale", QString::number( m_intfUserScaleFactor ) );

    /* Save playlist state */
    settings->setValue( "playqueue-panel-docked", m_playqueuePanel->m_docked );
    settings->setValue( "playqueue-panel-visible", m_playqueuePanel->m_visible );
    settings->setValue( "playqueue-panel-width", QString::number( m_playqueuePanel->m_width ) );

    settings->setValue( "navigation-panel-visible", m_navigationPanel->m_visible );
    settings->setValue( "navigation-panel-width", QString::number( m_navigationPanel->m_width ) );

    settings->setValue( "player-playlist-width", QString::number( m_playerPlaylistWidth ) );
    settings->setValue( "artist-albums-width", QString::number( m_artistAlbumsWidth ) );

    settings->setValue( "grid-view", m_gridView );
    settings->setValue( "grouping", m_grouping );

    settings->setValue( "color-scheme-index", m_colorScheme->currentIndex() );

    settings->setValue( "album-sections", m_albumSections );

    settings->setValue( "lyrics-mode", m_lyricsMode );

    /* Save the stackCentralW sizes */
    settings->endGroup();

    if( var_InheritBool( p_intf, "save-recentplay" ) )
        getSettings()->setValue( "filedialog-path", m_dialogFilepath );
    else
        getSettings()->remove( "filedialog-path" );

    /* Unregister callbacks */
    libvlc_int_t* libvlc = vlc_object_instance(p_intf);
    var_DelCallback( libvlc, "intf-boss", IntfBossCB, p_intf );
    var_DelCallback( libvlc, "intf-show", IntfRaiseMainCB, p_intf );
    var_DelCallback( libvlc, "intf-toggle-fscontrol", IntfShowCB, p_intf );
    var_DelCallback( libvlc, "intf-popupmenu", PopupMenuCB, p_intf );

    if (m_medialib)
        delete m_medialib;

    if (m_network_preparser)
        vlc_preparser_Delete(m_network_preparser);

    p_intf->p_mi = NULL;
}

bool MainCtx::hasVLM() const {
#ifdef ENABLE_VLM
    return true;
#else
    return false;
#endif
}

bool MainCtx::useClientSideDecoration() const
{
    //don't show CSD when interface is fullscreen
    return !m_windowTitlebar;
}

bool MainCtx::hasFirstrun() const {
    return var_InheritBool(p_intf,  "qt-privacy-ask" );
}

namespace {
const char keyPrivacyAsked[] = "redesign/privacy-asked";
}

bool MainCtx::onlineChoicesPending() const
{
    return var_InheritBool(p_intf, "qt-privacy-ask")
        && !getSettings()->value(keyPrivacyAsked, false).toBool();
}

void MainCtx::setOnlineChoicesMade()
{
    getSettings()->setValue(keyPrivacyAsked, true);
    getSettings()->sync();
}

// The answers of the installer's Privacy page (extras/juxtopposed/installer/
// vlc-redesign.iss): a file next to vlcrc that the installer writes rather
// than the settings themselves, which a new install doesn't have yet (vlc.exe
// fills an empty profile with the settings of the copy started last). Put
// into the settings once, then gone: the settings change in Settings after.
//   [privacy]
//   update-check=true|false      (redesign-update-check, absent: as it is)
//   subtitle-search=true|false   (SubtitlePreferences' auto-search)
//   [interface]
//   right-to-left=true|false     (redesign/right-to-left: the layout)
void MainCtx::applySetupChoices()
{
    char *dir = config_GetUserDir(VLC_CONFIG_DIR);
    if (dir == nullptr)
        return;
    const QString path = QDir(QString::fromUtf8(dir)).filePath(QStringLiteral("setup-choices.ini"));
    free(dir);
    if (!QFileInfo::exists(path))
        return;
    {
        QSettings choices(path, QSettings::IniFormat);
        const QVariant updateCheck = choices.value(QStringLiteral("privacy/update-check"));
        const QVariant subtitleSearch = choices.value(QStringLiteral("privacy/subtitle-search"));
        if (updateCheck.isValid())
        {
            config_PutInt("redesign-update-check", updateCheck.toBool() ? 1 : 0);
            config_SaveConfigFile(p_intf);
        }
        if (subtitleSearch.isValid())
            getSettings()->setValue(QStringLiteral("redesign/subtitle-auto-search"), subtitleSearch.toBool());
        const QVariant rightToLeft = choices.value(QStringLiteral("interface/right-to-left"));
        if (rightToLeft.isValid())
            getSettings()->setValue(QStringLiteral("redesign/right-to-left"), rightToLeft.toBool());
        // the subtitle language of Setup's Languages page: the default one,
        // the second one and the rest as they were
        const QString subtitles = choices.value(QStringLiteral("subtitles/language")).toString();
        if (!SubtitlePreferences::normalize(subtitles).isEmpty())
        {
            SubtitlePreferences preferences(p_intf);
            preferences.set(subtitles, preferences.secondary(), preferences.autoSearch(), preferences.showBoth());
            msg_Dbg(p_intf, "setup choices: subtitles in %s", qtu(subtitles));
        }
        msg_Dbg(p_intf, "setup choices: update check %s, online subtitles %s",
                updateCheck.isValid() ? (updateCheck.toBool() ? "on" : "off") : "as it was",
                subtitleSearch.isValid() ? (subtitleSearch.toBool() ? "on" : "off") : "as it was");
    }
    getSettings()->setValue(keyPrivacyAsked, true);
    // (setOnlineChoicesMade, judged by this write): the file stays until
    // the settings it gave are written
    if (syncSettings(getSettings()) && !QFile::remove(path))
        msg_Warn(p_intf, "setup choices: %s could not be removed", qtu(QDir::toNativeSeparators(path)));
}

void MainCtx::setUseGlobalShortcuts( bool useShortcuts )
{
    if (m_useGlobalShortcuts == useShortcuts)
        return;
    m_useGlobalShortcuts = useShortcuts;
    emit useGlobalShortcutsChanged(m_useGlobalShortcuts);
}

void MainCtx::setWindowSuportExtendedFrame(bool support) {
    if (m_windowSuportExtendedFrame == support)
        return;
    m_windowSuportExtendedFrame = support;
    emit windowSuportExtendedFrameChanged();
}

void MainCtx::setWindowExtendedMargin(unsigned int margin) {
    if (margin == m_windowExtendedMargin)
        return;
    m_windowExtendedMargin = margin;
    emit windowExtendedMarginChanged(margin);
}

/*****************************
 *   Main UI handling        *
 *****************************/

void MainCtx::loadPrefs(const bool callSignals)
{
    const auto loadFromVLCOption = [this, callSignals](auto &variable, const char *name
            , const std::function<void(MainCtx *)> signal)
    {
        using variableType = std::remove_reference_t<decltype(variable)>;

        const auto value =  loadVLCOption<variableType>(VLC_OBJECT(p_intf), name);
        if (value == variable)
            return;

        variable = value;
        if (callSignals && signal)
            signal(this);
    };

    /* Are we in the enhanced always-video mode or not ? At the start and
     * when the option changes only: reloading the preferences (every save
     * of the settings) must not show a UI hidden with Ctrl+H again */
    const bool minimalOption = m_minimalViewOption;
    loadFromVLCOption(m_minimalViewOption, "qt-minimal-view", nullptr);
    if (!callSignals || m_minimalViewOption != minimalOption)
        setMinimalView(m_minimalViewOption);

    loadFromVLCOption(m_bgCone, "qt-bgcone", &MainCtx::bgConeToggled);

    /* Should the UI stays on top of other windows */
    loadFromVLCOption(b_interfaceOnTop, "video-on-top", [this](MainCtx *)
    {
        emit interfaceAlwaysOnTopChanged(b_interfaceOnTop);
    });

    loadFromVLCOption(m_hasToolbarMenu, "qt-menubar", &MainCtx::hasToolbarMenuChanged);

    loadFromVLCOption(m_windowTitlebar, "qt-titlebar" , &MainCtx::useClientSideDecorationChanged);

    loadFromVLCOption(m_smoothScroll, "qt-smooth-scrolling", &MainCtx::smoothScrollChanged);

    loadFromVLCOption(m_maxVolume, "qt-max-volume", &MainCtx::maxVolumeChanged);

    loadFromVLCOption(m_pinVideoControls, "qt-pin-controls", &MainCtx::pinVideoControlsChanged);

    loadFromVLCOption(m_seekThumbnail, "qt-seek-thumbnail", &MainCtx::seekThumbnailChanged);
    loadFromVLCOption(m_seekSubtitleCues, "qt-seek-subtitle-cues", &MainCtx::seekSubtitleCuesChanged);

    loadFromVLCOption(m_pinOpacity, "qt-fs-opacity", &MainCtx::pinOpacityChanged);

    loadFromVLCOption(m_safeArea, "qt-safe-area", &MainCtx::safeAreaChanged);

    loadFromVLCOption(m_mouseHideTimeout, "mouse-hide-timeout", &MainCtx::mouseHideTimeoutChanged);
    loadFromVLCOption(m_fsSensitivity, "qt-fs-sensitivity", &MainCtx::fsSensitivityChanged);
    loadFromVLCOption(m_fullscreenControls, "qt-fs-controller", &MainCtx::fullscreenControlsChanged);
    loadFromVLCOption(m_playlistTree, "playlist-tree", &MainCtx::playlistTreeChanged);
    loadFromVLCOption(m_showErrorDialogs, "qt-error-dialogs", &MainCtx::showErrorDialogsChanged);

    /* The window of the interface follows these at once (the file name in
     * its title, its opacity); the tray icon shows or goes when its option
     * changes (at the start: initSystray) */
    loadFromVLCOption(m_nameInTitle, "qt-name-in-title", &MainCtx::nameInTitleChanged);
    loadFromVLCOption(m_windowOpacity, "qt-opacity", &MainCtx::windowOpacityChanged);
    loadFromVLCOption(m_systrayOption, "qt-system-tray", &MainCtx::updateSystray);

#ifdef _WIN32
    /* "Process priority" of the settings (-2 idle ... 2 high); while normal,
     * "Give VLC real-time priority" (high-priority) raises it above normal,
     * as VLC does when it is given on its command line (VLC reads it before
     * its settings: never from them) */
    {
        static const DWORD classes[] = { IDLE_PRIORITY_CLASS, BELOW_NORMAL_PRIORITY_CLASS,
                                         NORMAL_PRIORITY_CLASS, ABOVE_NORMAL_PRIORITY_CLASS,
                                         HIGH_PRIORITY_CLASS };
        static int applied = 0;
        int priority = std::clamp( getSettings()->value( "redesign/process-priority", 0 ).toInt(), -2, 2 );
        if( priority == 0 && var_InheritBool( p_intf, "high-priority" ) )
            priority = 1;
        if( priority != applied )
        {
            SetPriorityClass( GetCurrentProcess(), classes[priority + 2] );
            applied = priority;
        }
    }
#endif

    /* "Always show the player bar" of the settings */
    const bool alwaysShowPlayerBar = getSettings()->value( "redesign/always-show-player-bar", false ).toBool();
    if( alwaysShowPlayerBar != m_alwaysShowPlayerBar )
    {
        m_alwaysShowPlayerBar = alwaysShowPlayerBar;
        if( callSignals )
            emit alwaysShowPlayerBarChanged();
    }

    /* "Increase text contrast" of the settings */
    const bool increaseContrast = getSettings()->value( "redesign/increase-contrast", false ).toBool();
    if( increaseContrast != m_increaseContrast )
    {
        m_increaseContrast = increaseContrast;
        if( callSignals )
            emit increaseContrastChanged();
    }

    /* "Double click on the video" of the settings */
    const int doubleClick = getSettings()->value( "redesign/double-click-action", 1 ).toInt();
    if( doubleClick != m_doubleClickAction )
    {
        m_doubleClickAction = doubleClick;
        if( callSignals )
            emit doubleClickActionChanged();
    }
    {
        char *policy = config_GetPsz( "mouse-pause-policy" );
        const bool clickPauses = !policy || strcmp( policy, "disabled" ) != 0;
        free( policy );
        if( clickPauses != m_clickPauses )
        {
            m_clickPauses = clickPauses;
            if( callSignals )
                emit clickPausesChanged();
        }
    }

    /* "Never remember items matching" of the settings */
    vlc::playlist::Media::setHistoryFilter( QRegularExpression(
        getSettings()->value( "redesign/recentplay-filter" ).toString(),
        QRegularExpression::CaseInsensitiveOption ) );
}

void MainCtx::loadFromSettingsImpl(const bool callSignals)
{
    const auto loadFromSettings = [this, callSignals](auto &variable, const char *name
            , const auto defaultValue, auto obj,  auto signal)
    {
        using variableType = std::remove_reference_t<decltype(variable)>;

        const auto value = getSettings()->value(name, defaultValue).template value<variableType>();
        if (value == variable)
            return;

        variable = value;
        if (obj && callSignals && signal)
            (obj->*signal)(variable);
    };

    loadFromSettings(m_playqueuePanel->m_docked, "MainWindow/playqueue-panel-docked", true, m_playqueuePanel, &SidePanelCtx::dockedChanged);
    loadFromSettings(m_playqueuePanel->m_visible, "MainWindow/playqueue-panel-visible", false, m_playqueuePanel, &SidePanelCtx::visibleChanged);
    loadFromSettings(m_playqueuePanel->m_width, "MainWindow/playqueue-panel-width", 120, m_playqueuePanel, &SidePanelCtx::widthChanged);

    loadFromSettings(m_navigationPanel->m_visible, "MainWindow/navigation-panel-visible", true, m_navigationPanel, &SidePanelCtx::visibleChanged);
    loadFromSettings(m_navigationPanel->m_width, "MainWindow/navigation-panel-width", 120, m_navigationPanel, &SidePanelCtx::widthChanged);

    loadFromSettings(m_playerPlaylistWidth, "MainWindow/player-playlist-width", 120, this, &MainCtx::playerPlaylistWidthChanged);
    loadFromSettings(m_artistAlbumsWidth, "MainWindow/artist-albums-width", 120, this,  &MainCtx::artistAlbumsWidthChanged);

    loadFromSettings(m_gridView, "MainWindow/grid-view", true, this, &MainCtx::gridViewChanged);

    loadFromSettings(m_grouping, "MainWindow/grouping", GROUPING_NONE, this, &MainCtx::groupingChanged);

    loadFromSettings(m_showRemainingTime, "MainWindow/ShowRemainingTime", false, this, &MainCtx::showRemainingTimeChanged);

    loadFromSettings(m_albumSections, "MainWindow/album-sections", true, this, &MainCtx::albumSectionsChanged);

    loadFromSettings(m_lyricsMode, "MainWindow/lyrics-mode", true, this, &MainCtx::lyricsModeChanged);

    const auto colorSchemeIndex = getSettings()->value( "MainWindow/color-scheme-index", 0 ).toInt();
    m_colorScheme->setCurrentIndex(colorSchemeIndex);

    /* user interface scale factor */
    auto userIntfScaleFactor = var_InheritFloat(p_intf, "qt-interface-scale");
    if (userIntfScaleFactor == -1)
        userIntfScaleFactor = getSettings()->value( "MainWindow/interface-scale", 1.0).toDouble();
    if (m_intfUserScaleFactor != userIntfScaleFactor)
    {
        m_intfUserScaleFactor = userIntfScaleFactor;
        updateIntfScaleFactor();
    }
}

void MainCtx::reloadPrefs()
{
    loadPrefs(true);
    /* the log, the interfaces started with VLC, the hotkeys, the speed */
    if (m_liveSettings)
        m_liveSettings->reload();
}

void MainCtx::applyPlayOrder()
{
    using vlc::playlist::PlaylistController;
    if (!p_intf || !p_intf->p_mainPlaylistController)
        return;
    PlaylistController *playlist = p_intf->p_mainPlaylistController;
    // as the core reads the options at the start (libvlc.c)
    playlist->setRandom(config_GetInt("random") != 0);
    playlist->setRepeatMode(config_GetInt("repeat") ? PlaylistController::PLAYBACK_REPEAT_CURRENT
                            : config_GetInt("loop") ? PlaylistController::PLAYBACK_REPEAT_ALL
                                                    : PlaylistController::PLAYBACK_REPEAT_NONE);
    playlist->setMediaStopAction(config_GetInt("play-and-exit") ? PlaylistController::MEDIA_STOPPED_EXIT
                                 : config_GetInt("play-and-stop") ? PlaylistController::MEDIA_STOPPED_STOP
                                                                  : PlaylistController::MEDIA_STOPPED_CONTINUE);
}

void MainCtx::onInputChanged( bool hasInput )
{
    if( hasInput == false )
        return;
    int autoRaise = var_InheritInteger( p_intf, "qt-auto-raise" );
    if ( autoRaise == MainCtx::RAISE_NEVER )
        return;
    if( THEMIM->hasVideoOutput() == true )
    {
        if( ( autoRaise & MainCtx::RAISE_VIDEO ) == 0 )
            return;
    }
    else if ( ( autoRaise & MainCtx::RAISE_AUDIO ) == 0 )
        return;
    emit askRaise();
}

void MainCtx::onVOutChanged( bool video )
{
    int autoRaise = var_InheritInteger( p_intf, "qt-auto-raise" );
    if ( video && ( autoRaise & MainCtx::RAISE_VIDEO ) )
        emit askRaise();
}

#ifdef KeyPress
#undef KeyPress
#endif
void MainCtx::sendHotkey(Qt::Key key , Qt::KeyboardModifiers modifiers)
{
    QKeyEvent event(QEvent::KeyPress, key, modifiers );
    int vlckey = qtEventToVLCKey(&event);
    postHotkey(vlckey);
}

double MainCtx::windowLuminance(QQuickWindow *window, const QRectF &area) const
{
    if (window == nullptr || !area.isValid())
        return -1;
    // the frame as shown (rendered again for the grab: once per media)
    const QImage frame = window->grabWindow();
    if (frame.isNull())
        return -1;
    const qreal dpr = frame.devicePixelRatio() > 0 ? frame.devicePixelRatio() : 1;
    const QRect pixels = QRectF(area.x() * dpr, area.y() * dpr, area.width() * dpr, area.height() * dpr)
                             .toAlignedRect().intersected(frame.rect());
    if (pixels.isEmpty())
        return -1;
    const QImage small = frame.copy(pixels).scaled(16, 4, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
                                           .convertToFormat(QImage::Format_RGB32);
    // sRGB to linear light, as WCAG's relative luminance
    const auto linear = [](int channel) {
        const double c = channel / 255.0;
        return c <= 0.04045 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4);
    };
    double sum = 0;
    for (int y = 0; y < small.height(); ++y)
    {
        const QRgb *line = reinterpret_cast<const QRgb *>(small.constScanLine(y));
        for (int x = 0; x < small.width(); ++x)
            sum += 0.2126 * linear(qRed(line[x])) + 0.7152 * linear(qGreen(line[x]))
                 + 0.0722 * linear(qBlue(line[x]));
    }
    const double luminance = sum / (small.width() * small.height());
    msg_Dbg(p_intf, "luminance behind a text: %.3f", luminance);
    return luminance;
}

bool MainCtx::isHotkeyFree(Qt::Key key, Qt::KeyboardModifiers modifiers) const
{
    QKeyEvent event(QEvent::KeyPress, key, modifiers );
    return !isVLCKeyBound(VLC_OBJECT(p_intf), qtEventToVLCKey(&event));
}

void MainCtx::sendVLCHotkey(int vlcHotkey)
{
    postHotkey(vlcHotkey);
}

void MainCtx::postHotkey(int vlcKey)
{
    libvlc_int_t *libvlc = vlc_object_instance(p_intf);
    const std::shared_ptr<std::atomic<bool>> closed = m_hotkeysClosed;
    QMetaObject::invokeMethod(m_hotkeyContext, [libvlc, vlcKey, closed]() {
        // the actions reach the interface (boss key, fullscreen): none once
        // it closes
        if (!closed->load())
            var_SetInteger(libvlc, "key-pressed", vlcKey);
    }, Qt::QueuedConnection);
}

void MainCtx::updateIntfScaleFactor()
{
    auto newValue = m_intfUserScaleFactor;
    if (QWindow* window = p_intf->p_compositor ? p_intf->p_compositor->interfaceMainWindow() : nullptr)
    {
        QScreen* screen = window->screen();
        if (screen)
        {
            qreal dpi = screen->logicalDotsPerInch();
            newValue = m_intfUserScaleFactor * dpi / VLC_REFERENCE_SCALE_FACTOR;
        }
    }

    const int FACTOR_RESOLUTION = 100;
    if ( static_cast<int>(m_intfScaleFactor * FACTOR_RESOLUTION)
         == static_cast<int>(newValue * FACTOR_RESOLUTION) ) return;

    m_intfScaleFactor = newValue;
    emit intfScaleFactorChanged();
}

void MainCtx::onWindowVisibilityChanged(QWindow::Visibility visibility)
{
    m_windowVisibility = visibility;
}

void MainCtx::setHasAcrylicSurface(const bool v)
{
    if (m_hasAcrylicSurface == v)
        return;

    m_hasAcrylicSurface = v;
    emit hasAcrylicSurfaceChanged(v);
}

void MainCtx::incrementIntfUserScaleFactor(bool increment)
{
    if (increment)
        setIntfUserScaleFactor(m_intfUserScaleFactor + 0.1);
    else
        setIntfUserScaleFactor(m_intfUserScaleFactor - 0.1);
}

void MainCtx::setIntfUserScaleFactor(double newValue)
{
    const auto value = qBound(getMinIntfUserScaleFactor(), newValue, getMaxIntfUserScaleFactor());
    if (qFuzzyCompare(value, m_intfUserScaleFactor))
        return;
    m_intfUserScaleFactor = value;
    updateIntfScaleFactor();
}

void MainCtx::setHasToolbarMenu( bool hasToolbarMenu )
{
    if (m_hasToolbarMenu == hasToolbarMenu)
        return;

    m_hasToolbarMenu = hasToolbarMenu;

    config_PutInt("qt-menubar", (int) hasToolbarMenu);

    emit hasToolbarMenuChanged();
}

void MainCtx::setRightToLeft(bool rightToLeft)
{
    QGuiApplication::setLayoutDirection(rightToLeft ? Qt::RightToLeft : Qt::LeftToRight);
    if (m_rightToLeft == rightToLeft)
        return;
    m_rightToLeft = rightToLeft;
    emit rightToLeftChanged();
}

void MainCtx::setPinVideoControls(bool pinVideoControls)
{
    if (m_pinVideoControls == pinVideoControls)
        return;

    m_pinVideoControls = pinVideoControls;
    emit pinVideoControlsChanged();
}

void MainCtx::setPinOpacity(float pinOpacity)
{
    if (m_pinOpacity == pinOpacity)
        return;

    m_pinOpacity = pinOpacity;

    emit pinOpacityChanged();
}

inline void MainCtx::initSystray()
{
    bool b_systrayAvailable = QSystemTrayIcon::isSystemTrayAvailable();
    bool b_systrayWanted = var_InheritBool( p_intf, "qt-system-tray" );

    if( var_InheritBool( p_intf, "qt-start-minimized") )
    {
        if( b_systrayAvailable )
        {
            b_systrayWanted = true;
            b_hideAfterCreation = true;
        }
        else
            msg_Err( p_intf, "cannot start minimized without system tray bar" );
    }

    if( b_systrayAvailable && b_systrayWanted && !m_systray )
        m_systray = std::make_unique<VLCSystray>(this);
}

void MainCtx::updateSystray()
{
    const bool wanted = m_systrayOption && QSystemTrayIcon::isSystemTrayAvailable();
    if( wanted == ( m_systray != nullptr ) )
        return;
    if( wanted )
    {
        m_systray = std::make_unique<VLCSystray>(this);
        return;
    }
    /* the window hidden in the tray comes back: nothing would show it */
    if( m_windowVisibility == QWindow::Hidden )
        emit setInterfaceVisibible( true );
    m_systray.reset();
}

ThreadRunner* MainCtx::threadRunner() const
{
    return m_threadRunner;
}

QUrl MainCtx::folderMRL(const QString &fileMRL) const
{
    return folderMRL(QUrl::fromUserInput(fileMRL));
}

QUrl MainCtx::folderMRL(const QUrl &fileMRL) const
{
    if (fileMRL.isLocalFile())
    {
        const QString f = fileMRL.toLocalFile();
        return QUrl::fromLocalFile(QFileInfo(f).absoluteDir().absolutePath());
    }

    return {};
}

void MainCtx::showInFileManager(const QUrl &fileMRL) const
{
    if (!fileMRL.isLocalFile())
        return;

#ifdef _WIN32
    const QString path = fileMRL.toLocalFile();
    if (QFileInfo::exists(path))
    {
        // Explorer opens the folder with the file selected; the path quoted
        // as Explorer wants it (a comma in a name)
        QProcess explorer;
        explorer.setProgram(QStringLiteral("explorer.exe"));
        explorer.setNativeArguments(QStringLiteral("/select,\"%1\"").arg(QDir::toNativeSeparators(path)));
        if (explorer.startDetached())
            return;
    }
#endif
    QDesktopServices::openUrl(folderMRL(fileMRL));
}

QString MainCtx::displayMRL(const QUrl &mrl) const
{
    return urlToDisplayString(mrl);
}

void MainCtx::setPlayerPlaylistWidth( double factor )
{
    if (factor > 0.0)
    {
        m_playerPlaylistWidth = factor;
        emit playerPlaylistWidthChanged(factor);
    }
}

void MainCtx::setbgCone(bool bgCone)
{
    if (m_bgCone == bgCone)
        return;

    m_bgCone = bgCone;

    emit bgConeToggled();
}

void MainCtx::setMinimalView(bool enable)
{
    if (m_mainInterfaceModes.testFlag(MAININTERFACE_MODE_MINIMAL) == enable)
        return;

    m_mainInterfaceModes.setFlag(MAININTERFACE_MODE_MINIMAL, enable);
    emit mainInterfaceModesChanged(m_mainInterfaceModes);
}

void MainCtx::setPlayerView(bool enable)
{
    if (m_mainInterfaceModes.testFlag(MAININTERFACE_MODE_PLAYER) == enable)
        return;

    m_mainInterfaceModes.setFlag(MAININTERFACE_MODE_PLAYER, enable);
    emit mainInterfaceModesChanged(m_mainInterfaceModes);
}

void MainCtx::setShowRemainingTime( bool show )
{
    m_showRemainingTime = show;
    emit showRemainingTimeChanged(show);
}

void MainCtx::setGridView(bool asGrid)
{
    m_gridView = asGrid;
    emit gridViewChanged( asGrid );
}

void MainCtx::setGrouping(Grouping grouping)
{
    m_grouping = grouping;

    emit groupingChanged(grouping);
}

void MainCtx::setAlbumSections(bool enabled)
{
    if (m_albumSections == enabled)
        return;

    m_albumSections = enabled;
    emit albumSectionsChanged(enabled);
}

void MainCtx::setLyricsMode(bool enabled)
{
    if (m_lyricsMode == enabled)
        return;

    m_lyricsMode = enabled;
    emit lyricsModeChanged(enabled);
}

void MainCtx::setInterfaceAlwaysOnTop( bool on_top )
{
    if (b_interfaceOnTop == on_top)
        return;

    b_interfaceOnTop = on_top;
    emit interfaceAlwaysOnTopChanged(on_top);
}

bool MainCtx::hasEmbededVideo() const
{
    return m_videoSurfaceProvider && m_videoSurfaceProvider->hasVideoEmbed();
}

void MainCtx::setVideoSurfaceProvider(VideoSurfaceProvider* videoSurfaceProvider)
{
    if (m_videoSurfaceProvider)
        disconnect(m_videoSurfaceProvider, &VideoSurfaceProvider::hasVideoEmbedChanged, this, &MainCtx::hasEmbededVideoChanged);
    m_videoSurfaceProvider = videoSurfaceProvider;
    if (m_videoSurfaceProvider)
        connect(m_videoSurfaceProvider, &VideoSurfaceProvider::hasVideoEmbedChanged,
                this, &MainCtx::hasEmbededVideoChanged,
                Qt::QueuedConnection);
    emit hasEmbededVideoChanged(m_videoSurfaceProvider && m_videoSurfaceProvider->hasVideoEmbed());
}

QJSValue MainCtx::urlListToMimeData(const QJSValue &array) {
    // NOTE: Due to a Qt regression since 17318c4
    //       (Nov 11, 2022), it is not possible to
    //       use RFC-2483 compliant string here.
    //       This regression was later corrected by
    //       c25f53b (Jul 31, 2024).
    // NOTE: Qt starts supporting string list since
    //       17318c4, so starting from 6.5.0 a string
    //       list can be used which is not affected
    //       by the said issue. For Qt versions below
    //       6.5.0, use byte array which is used as is
    //       by Qt.
    assert(array.property("length").toInt() > 0);

    QJSEngine* const engine = qjsEngine(this);
    assert(engine);

    QJSValue data;
#if QT_VERSION < QT_VERSION_CHECK(6, 5, 0)
    QString string;
    for (int i = 0; i < array.property(QStringLiteral("length")).toInt(); ++i)
    {
        QString decodedUrl;
        const QJSValue element = array.property(i);
        if (element.isUrl())
            // QJSValue does not have `toUrl()`
            decodedUrl = QJSManagedValue(element, engine).toUrl().toString(QUrl::FullyEncoded);
        else if (element.isString())
            // If the element is string, we assume it is already encoded
            decodedUrl = element.toString();
        else if (element.isVariant())
        {
            const QVariant variant = element.toVariant();
            if (variant.typeId() == QMetaType::QUrl)
                decodedUrl = variant.toUrl().toString(QUrl::FullyEncoded);
            else if (variant.typeId() == QMetaType::QString)
                decodedUrl = variant.toString();
            else
                Q_UNREACHABLE();
        }
        else
            Q_UNREACHABLE(); // Assertion failure in debug builds
        string += decodedUrl + QStringLiteral("\r\n");
    }
    string.chop(2);
    data = engine->toScriptValue(string);
#else
    data = array;
#endif
    QJSValue ret = engine->newObject();
    ret.setProperty(QStringLiteral("text/uri-list"), data);
    return ret;
}

VideoSurfaceProvider* MainCtx::getVideoSurfaceProvider() const
{
    return m_videoSurfaceProvider;
}

/************************************************************************
 * Events stuff
 ************************************************************************/

void MainCtx::onWindowClose( QWindow* )
{
    PlaylistController* playlistController = p_intf->p_mainPlaylistController;
    PlayerController* playerController = p_intf->p_mainPlayerController;

    if (m_videoSurfaceProvider)
        m_videoSurfaceProvider->onWindowClosed();
    //We need to make sure that noting is playing anymore otherwise the vout will be closed
    //after the main interface, and it requires (at least with OpenGL) that the OpenGL context
    //from the main window is still valid.
    //vlc_window_ReportClose is currently stubbed
    if (playerController && playerController->hasVideoOutput()) {
        connect(playerController, &PlayerController::playingStateChanged, [this](PlayerController::PlayingState state){
            if (state == PlayerController::PLAYING_STATE_STOPPED) {
                emit askToQuit();
            }
        });
        playlistController->stop();
    }
    else
    {
        emit askToQuit(); /* ask THEDP to quit, so we have a unique method */
    }
}

void MainCtx::toggleToolbarMenu()
{
    setHasToolbarMenu(!m_hasToolbarMenu);
}

void MainCtx::toggleInterfaceFullScreen()
{
    emit setInterfaceFullScreen( m_windowVisibility != QWindow::FullScreen );
}

void MainCtx::emitBoss()
{
    emit askBoss();
}

void MainCtx::emitShow()
{
    emit askShow();
}

void MainCtx::emitRaise()
{
    emit askRaise();
}

VLCVarChoiceModel* MainCtx::getExtraInterfaces()
{
    return m_extraInterfaces;
}

bool MainCtx::pasteFromClipboard()
{
    assert(qApp);
    const QClipboard *const clipboard = qApp->clipboard();
    if (Q_UNLIKELY(!clipboard))
        return false;
    const QMimeData *mimeData = clipboard->mimeData(QClipboard::Selection);
    if (!mimeData || !mimeData->hasUrls())
        mimeData = clipboard->mimeData(QClipboard::Clipboard);

    if (Q_UNLIKELY(!mimeData))
        return false;

    QList<QUrl> urlList = mimeData->urls();
    QString text = mimeData->text();

    if (urlList.count() > 1 || text.contains('\n'))
    {
        // NOTE: The reason that mime data for `text/uri-list` is not used
        //       directly as the placeholder of the input dialog instead of
        //       re-constructing is to decode the urls in the list.
        QString placeholder;

        if (urlList.isEmpty())
        {
            placeholder = std::move(text);
        }
        else
        {
            for (const auto& i : urlList)
                placeholder += i.toString(QUrl::PrettyDecoded) + '\n';
            placeholder.chop(1);
        }

        // the URLs in the field of the redesign's dialogs, to edit
        RedesignMessage message;
        message.title = qtr("Paste from clipboard");
        message.heading = qtr("Do you want to enqueue the following URLs into the play queue?");
        message.acceptText = qtr("Enqueue");
        message.input = RedesignMessage::TextInput;
        message.inputText = placeholder;
        QString ret;
        if (THEDP->execRedesignMessage(message, &ret) != RedesignMessage::Accepted)
            return false;

        for (const auto& i : QStringView(ret).split('\n'))
        {
            if (i.length() > 0)
                THEMPL->append(i.trimmed().toString(), false);
        }

        return true;
    }
    else if ((urlList.count() == 1) || mimeData->hasText())
    {
        THEDP->openUrlDialog();
        return true;
    }

    return false;
}

/*****************************************************************************
 * PopupMenuCB: callback triggered by the intf-popupmenu playlist variable.
 *  We don't show the menu directly here because we don't want the
 *  caller to block for a too long time.
 *****************************************************************************/
static int PopupMenuCB( vlc_object_t *, const char *,
                        vlc_value_t, vlc_value_t new_val, void *param )
{
    qt_intf_t *p_intf = (qt_intf_t *)param;

    if( p_intf->pf_show_dialog )
    {
        p_intf->pf_show_dialog( p_intf->intf, INTF_DIALOG_POPUPMENU,
                                new_val.b_bool, NULL );
    }

    return VLC_SUCCESS;
}

/*****************************************************************************
 * IntfShowCB: callback triggered by the intf-toggle-fscontrol libvlc variable.
 *****************************************************************************/
static int IntfShowCB( vlc_object_t *, const char *,
                       vlc_value_t, vlc_value_t, void *param )
{
    qt_intf_t *p_intf = (qt_intf_t *)param;
    p_intf->p_mi->emitShow();

    return VLC_SUCCESS;
}

/*****************************************************************************
 * IntfRaiseMainCB: callback triggered by the intf-show-main libvlc variable.
 *****************************************************************************/
static int IntfRaiseMainCB( vlc_object_t *, const char *,
                            vlc_value_t, vlc_value_t, void *param )
{
    qt_intf_t *p_intf = (qt_intf_t *)param;
    p_intf->p_mi->emitRaise();

    return VLC_SUCCESS;
}

/*****************************************************************************
 * IntfBossCB: callback triggered by the intf-boss libvlc variable.
 *****************************************************************************/
static int IntfBossCB( vlc_object_t *, const char *,
                       vlc_value_t, vlc_value_t, void *param )
{
    qt_intf_t *p_intf = (qt_intf_t *)param;
    p_intf->p_mi->emitBoss();

    return VLC_SUCCESS;
}

bool MainCtx::acrylicActive() const
{
    return m_acrylicActive;
}

void MainCtx::setAcrylicActive(bool newAcrylicActive)
{
    if (m_acrylicActive == newAcrylicActive)
        return;

    m_acrylicActive = newAcrylicActive;
    emit acrylicActiveChanged();
}

bool MainCtx::preferHotkeys() const
{
    return m_preferHotkeys;
}

void MainCtx::setPreferHotkeys(bool enable)
{
    if (m_preferHotkeys == enable)
        return;

    m_preferHotkeys = enable;

    emit preferHotkeysChanged();
}

void MainCtx::setKeysBeforeHotkeys(QObject* item)
{
    if (m_keysBeforeHotkeys == item)
        return;
    m_keysBeforeHotkeys = item;
    emit keysBeforeHotkeysChanged();
}

static bool askKeyItem(QObject *item, const char *function, int key, Qt::KeyboardModifiers modifiers)
{
    if (!item)
        return false;
    QVariant answer;
    QMetaObject::invokeMethod(item, function, Q_RETURN_ARG(QVariant, answer),
                              Q_ARG(QVariant, key), Q_ARG(QVariant, static_cast<int>(modifiers)));
    return answer.toBool();
}

bool MainCtx::keyTakenBeforeHotkeys(int key, Qt::KeyboardModifiers modifiers) const
{
    return askKeyItem(m_keysBeforeHotkeys.data(), "takeKey", key, modifiers);
}

bool MainCtx::keyGoesBeforeHotkeys(int key, Qt::KeyboardModifiers modifiers) const
{
    return askKeyItem(m_keysBeforeHotkeys.data(), "wantsKey", key, modifiers);
}

QString MainCtx::tvKeyAction(Qt::Key key, Qt::KeyboardModifiers modifiers) const
{
    // the keypad's digits and Enter are the keyboard's
    QKeyEvent event(QEvent::KeyPress, key, modifiers & ~Qt::KeypadModifier);
    const uint_fast32_t vlcKey = qtEventToVLCKey(&event);
    if (vlcKey == 0)
        return {};
    static const char *const actions[][2] = {
        { "qt-key-tv-channel-up", "channel-up" },
        { "qt-key-tv-channel-down", "channel-down" },
        { "qt-key-tv-previous", "previous" },
        { "qt-key-tv-list", "list" },
        { "qt-key-tv-info", "info" },
    };
    for (const auto &action : actions)
    {
        char *keys = var_InheritString(p_intf, action[0]);
        bool found = false;
        for (char *buf, *name = keys ? strtok_r(keys, "\t", &buf) : nullptr;
             name && !found; name = strtok_r(nullptr, "\t", &buf))
            found = vlc_str2keycode(name) == vlcKey;
        free(keys);
        if (found)
            return QString::fromLatin1(action[1]);
    }
    return {};
}

void MainCtx::setSubtitleWheelArea(QObject *item)
{
    if (m_subtitleWheelArea == item)
        return;
    m_subtitleWheelArea = item;
    emit subtitleWheelAreaChanged();
}

void MainCtx::setSubtitleWheelExclusion(QObject *item)
{
    if (m_subtitleWheelExclusion == item)
        return;
    m_subtitleWheelExclusion = item;
    emit subtitleWheelExclusionChanged();
}

// One of the keys of an option of keys (tab separated)
static bool optionHasKey(qt_intf_t *intf, const char *option, uint_fast32_t vlcKey)
{
    char *keys = var_InheritString(intf, option);
    bool found = false;
    for (char *buf, *name = keys ? strtok_r(keys, "	", &buf) : nullptr;
         name && !found; name = strtok_r(nullptr, "	", &buf))
        found = vlc_str2keycode(name) == vlcKey;
    free(keys);
    return found;
}

// The step of the subtitles' size a key makes: 1 larger, -1 smaller, 0 back
// to 100 %, 2 none
static int subtitleScaleStep(qt_intf_t *intf, uint_fast32_t vlcKey)
{
    if (optionHasKey(intf, "key-subtitle-text-scale-up", vlcKey))
        return 1;
    if (optionHasKey(intf, "key-subtitle-text-scale-down", vlcKey))
        return -1;
    if (optionHasKey(intf, "key-subtitle-text-scale-normal", vlcKey))
        return 0;
    return 2;
}

// The interface's zoom a key makes: 1 in, -1 out, 0 none
static int interfaceZoomStep(qt_intf_t *intf, uint_fast32_t vlcKey)
{
    if (optionHasKey(intf, "qt-key-intf-zoom-in", vlcKey))
        return 1;
    if (optionHasKey(intf, "qt-key-intf-zoom-out", vlcKey))
        return -1;
    return 0;
}

bool MainCtx::wheelKeyAction(int vlcKey, const QPointF &scenePos, bool apply)
{
    if (vlcKey == 0 || m_wheelCapture)
        return false;
    // over the video: no popup (a dialog, a panel, a menu, its dimmed
    // background) under the pointer
    bool overVideo = false;
    PlayerController *player = p_intf->p_mainPlayerController;
    auto *area = qobject_cast<QQuickItem *>(m_subtitleWheelArea.data());
    if (player && player->hasVideoOutput() && area && area->isVisible() && area->window())
    {
        overVideo = area->contains(area->mapFromScene(scenePos));
        // (not over the library beside it, drawn over the video)
        auto *exclusion = qobject_cast<QQuickItem *>(m_subtitleWheelExclusion.data());
        if (overVideo && exclusion && exclusion->isVisible() && exclusion->window() == area->window()
            && exclusion->contains(exclusion->mapFromScene(scenePos)))
            overVideo = false;
        // (the overlay of the popups: a child of the content item or of the
        // window's root item)
        QQuickItem *content = area->window()->contentItem();
        QList<QQuickItem *> layers = content ? content->childItems() : QList<QQuickItem *>();
        if (content && content->parentItem())
            layers += content->parentItem()->childItems();
        for (QQuickItem *layer : layers)
        {
            if (!overVideo)
                break;
            if (!layer->inherits("QQuickOverlay"))
                continue;
            const QList<QQuickItem *> popups = layer->childItems();
            for (QQuickItem *popup : popups)
                if (popup->isVisible() && popup->opacity() > 0
                    && popup->contains(popup->mapFromScene(scenePos)))
                {
                    overVideo = false;
                    break;
                }
        }
    }
    if (overVideo)
    {
        const int step = subtitleScaleStep(p_intf, vlcKey);
        if (step != 2)
        {
            if (apply)
                player->stepSubtitleTextScale(step);
            return true;
        }
    }
    const int zoom = interfaceZoomStep(p_intf, vlcKey);
    if (zoom == 0)
        return false;
    if (apply)
        incrementIntfUserScaleFactor(zoom > 0);
    return true;
}

bool MainCtx::takeSizeKey(int vlcKey, bool playerKeys)
{
    if (vlcKey == 0)
        return false;
    PlayerController *player = p_intf->p_mainPlayerController;
    if (playerKeys && player && player->hasVideoOutput())
    {
        // (VLC's hotkeys would change it unseen by the interface)
        const int step = subtitleScaleStep(p_intf, vlcKey);
        if (step != 2)
        {
            player->stepSubtitleTextScale(step);
            return true;
        }
    }
    const int zoom = interfaceZoomStep(p_intf, vlcKey);
    if (zoom == 0)
        return false;
    incrementIntfUserScaleFactor(zoom > 0);
    return true;
}

QWindow *MainCtx::intfMainWindow() const
{
    if (p_intf->p_compositor)
        return p_intf->p_compositor->interfaceMainWindow();
    else
        return nullptr;
}

bool MainCtx::videoSurfaceCanHaveFractionalPosition() const
{
    assert(p_intf->p_compositor);
    if (p_intf->p_compositor->type() == vlc::Compositor::DummyCompositor)
        return true; // Does not matter, there is no video embedding in this case.

    // We can avoid dynamic cast here, since we early return if the type is not `CompositorVideo`:
    const auto videoCompositor = static_cast<vlc::CompositorVideo*>(p_intf->p_compositor.get());
    return videoCompositor->supportsFractionalPositioning();
}

QVariant MainCtx::settingValue(const QString &key, const QVariant &defaultValue) const
{
    return settings->value(key, defaultValue);
}

void MainCtx::applyInterfaceLanguage(const QString &iso)
{
    const QString code = (iso.isEmpty() || iso == QLatin1String("auto")) ? QLocale::system().name() : iso;
    if (code == m_interfaceLanguage)
        return;
    m_interfaceLanguage = code;
    vlc_settextlanguage(qtu(code));
    // (Qt asks for setDefault() before other threads use the default locale:
    // here only the UI thread does, the numbers and dates of QML; the
    // interface's workers - folder listing, playlist export, subtitle
    // hashing - must keep from QLocale() and QCollator)
    QLocale::setDefault(QLocale(code));
    applyLanguageFonts(QLocale(code));
    // (the direction is a setting of its own: rightToLeft, chosen with the
    // language from languageRightToLeft)
    // the widgets get it from the application (QEvent::LanguageChange)
    QCoreApplication::postEvent(QCoreApplication::instance(), new QEvent(QEvent::LanguageChange));
    emit interfaceLanguageChanged();
}

bool MainCtx::languageRightToLeft() const
{
    /* xgettext: The direction your language is written in: "RTL" if from
     * right to left (Arabic, Hebrew, Persian, Urdu...), else "LTR". Choosing
     * the language lays the interface out in that direction (the user can
     * change it under the language). */
    const QString direction = QString::fromUtf8(vlc_pgettext("layout direction", "LTR")).trimmed();
    if (direction.compare(QLatin1String("RTL"), Qt::CaseInsensitive) == 0)
        return true;
    if (direction.compare(QLatin1String("LTR"), Qt::CaseInsensitive) != 0)
        return false;
    // not translated (or left to right): as Qt knows the language
    QString code = m_interfaceLanguage;
    if (code.isEmpty())
    {
        code = interfaceLanguage();
        if (code == QLatin1String("auto"))
            code = QLocale::system().name();
    }
    return QLocale(code).textDirection() == Qt::RightToLeft;
}

QColor MainCtx::systemAccent() const
{
    return m_redesignAccent ? m_redesignAccent->systemAccent() : RedesignAccent::defaultAccent();
}

void MainCtx::setSettingValue(const QString &key, const QVariant &value)
{
    settings->setValue(key, value);
    emit settingValueChanged(key);
}

void MainCtx::removeSettingValue(const QString &key)
{
    settings->remove(key);
    emit settingValueChanged(key);
}

void MainCtx::setAttachedToolTip(QObject *toolTip)
{
    // See QQuickToolTipAttachedPrivate::instance(bool create)
    assert(toolTip);

    // Prevent possible invalid down-casting:
    assert(toolTip->inherits("QQuickToolTip"));

    QQmlEngine* const engine = qmlEngine(toolTip);
    assert(engine);
    assert(engine->objectOwnership(toolTip) == QQmlEngine::ObjectOwnership::JavaScriptOwnership);

    // Dynamic internal property:
#if QT_VERSION < QT_VERSION_CHECK(6, 12, 0)
    static const char* const name = "_q_QQuickToolTip";
#else
    static const char* const name = "_q_shared_QQuickToolTip";
#endif

    if (const auto obj = engine->property(name).value<QObject *>())
    {
        if (engine->objectOwnership(obj) == QQmlEngine::ObjectOwnership::CppOwnership)
            obj->deleteLater();
    }

    // setProperty() will return false, so there is no
    // need to check the return value:
    engine->setProperty(name, QVariant::fromValue(toolTip));

    // Check if the attached tooltip is actually the
    // one that is set
#ifndef NDEBUG
    QMetaObject::invokeMethod(toolTip, [toolTip]() {
        const auto engine = qmlEngine(toolTip);
        if (Q_UNLIKELY(!engine)) // Very unlikely, if not impossible
            return;
        QQmlComponent component(engine);
        component.setData(QByteArrayLiteral("import QtQuick; import QtQuick.Templates as T; Item { }"), {});
        QObject* const obj = component.create();
        assert(obj);
        // Consider disabling setting of custom attached
        // tooltip if the following assertion fails:
        if (QQmlProperty::read(obj, QStringLiteral("T.ToolTip.toolTip"), qmlContext(obj)).value<QObject*>() != toolTip)
            qmlWarning(obj) << "Could not set self as custom ToolTip!";
        obj->deleteLater();
    }, Qt::QueuedConnection);
#endif
}

MainCtx::MainInterfaceMode MainCtx::getEffectiveMainInterfaceMode() const {
    //priority applies across modes
    if (m_mainInterfaceModes & MainCtx::MAININTERFACE_MODE_MINIMAL)
        return MAININTERFACE_MODE_MINIMAL;
    else if (m_mainInterfaceModes & MAININTERFACE_MODE_PLAYER)
        return MAININTERFACE_MODE_PLAYER;
    else if (m_mainInterfaceModes & MAININTERFACE_MODE_MAINDISPLAY)
        return MAININTERFACE_MODE_MAINDISPLAY;
    else
        return MAININTERFACE_MODE_INVALID;
}

double MainCtx::dp(const double px, const double scale)
{
    return std::round(px * scale);
}

double MainCtx::dp(const double px) const
{
    return dp(px, m_intfScaleFactor);
}

bool MainCtx::useXmasCone() const
{
    return (QDate::currentDate().dayOfYear() >= QT_XMAS_JOKE_DAY)
            && var_InheritBool( p_intf, "qt-icon-change" );
}

bool WindowStateHolder::holdFullscreen(QWindow *window, Source source, bool hold)
{
    QVariant prop = window->property("__windowFullScreen");
    bool ok = false;
    unsigned fullscreenCounter = prop.toUInt(&ok);
    if (!ok)
        fullscreenCounter = 0;

    if (hold)
        fullscreenCounter |= source;
    else
        fullscreenCounter &= ~source;

    Qt::WindowStates oldflags = window->windowStates();
    Qt::WindowStates newflags;

    if( fullscreenCounter != 0 )
        newflags = oldflags | Qt::WindowFullScreen;
    else
        newflags = oldflags & ~Qt::WindowFullScreen;

    window->setProperty("__windowFullScreen", QVariant::fromValue(fullscreenCounter));

    if( newflags != oldflags )
    {
        window->setWindowStates( newflags );
    }

    return fullscreenCounter != 0;
}

bool WindowStateHolder::holdOnTop(QWindow *window, Source source, bool hold)
{
    QVariant prop = window->property("__windowOnTop");
    bool ok = false;
    unsigned onTopCounter = prop.toUInt(&ok);
    if (!ok)
        onTopCounter = 0;

    if (hold)
        onTopCounter |= source;
    else
        onTopCounter &= ~source;

#ifdef _WIN32
    if (!window->handle()) // do not call hold on top if there is no platform window
        return false;
#endif

    if( onTopCounter != 0 )
    {
#ifdef _WIN32
        // Qt::WindowStaysOnTopHint is broken on Windows: QTBUG-133034, QTBUG-132522.
        // Reported to be fixed by Qt 6.9, but needs additional checking if the solution does not interfere with `CSDWin32EventHandler`.
        SetWindowPos(reinterpret_cast<HWND>(window->winId()), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
#else
        window->setFlag(Qt::WindowStaysOnTopHint, true);
#endif
    }
    else
    {
#ifdef _WIN32
        // Qt::WindowStaysOnTopHint is broken on Windows: QTBUG-133034, QTBUG-132522.
        // Reported to be fixed by Qt 6.9, but needs additional checking if the solution does not interfere with `CSDWin32EventHandler`.
        SetWindowPos(reinterpret_cast<HWND>(window->winId()), HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
#else
        window->setFlag(Qt::WindowStaysOnTopHint, false);
#endif
    }

    window->setProperty("__windowOnTop", QVariant::fromValue(onTopCounter));

    return onTopCounter != 0;
}

double MainCtx::artistAlbumsWidth() const
{
    return dp(m_artistAlbumsWidth);
}

void MainCtx::setArtistAlbumsWidth(double newArtistAlbumsWidth)
{
    double unscaledValue = newArtistAlbumsWidth / getIntfScaleFactor();
    if (qFuzzyCompare(m_artistAlbumsWidth, unscaledValue))
        return;

    m_artistAlbumsWidth = unscaledValue;
    emit artistAlbumsWidthChanged( dp(m_artistAlbumsWidth) );
}

#ifdef UPDATE_CHECK
UpdateModel* MainCtx::getUpdateModel() const
{
    if (!m_updateModel)
    {
        m_updateModel = std::make_unique<UpdateModel>(p_intf);

        connect(m_updateModel.get(), &UpdateModel::updateStatusChanged, this, [this](){
            // No-op, modern update pane opens itself automatically when status changes.
            // However, the old update dialog is used as fallback in case for any reason
            // the modern pane is not available.

            const auto openLegacyDialogIfNecessary = [&]() {
                // Fallback:

                if (Q_LIKELY(p_intf &&
                             p_intf->p_compositor &&
                             p_intf->p_compositor->quickWindow() &&
                             p_intf->p_compositor->quickWindow()->contentItem()))
                {
                    const auto target = p_intf->p_compositor->quickWindow()->contentItem();
                    QMetaObject::invokeMethod(target, [target]() {
                        const auto updatePaneLoader = target->findChild<QQuickItem*>(QStringLiteral("updatePaneLoader"));
                        if (!updatePaneLoader || (updatePaneLoader->property("status").toInt() != 1 /* Loader.Ready */))
                        {
                            qDebug("No acknowledgement from the modern update loader, falling back to the old update dialog.");
                            THEDP->updateDialog(DialogsProvider::Show);
                        }
                    }, Qt::QueuedConnection);
                }
                else
                {
                    qDebug("Compositor is not available, falling back to the old update dialog.");
                    THEDP->updateDialog();
                }
            };

            if (m_updateModel->explicitCheck())
            {
                if (m_updateModel->updateStatus() != UpdateModel::Unchecked)
                    openLegacyDialogIfNecessary();
            }
            else
            {
                switch (m_updateModel->updateStatus())
                {
                case UpdateModel::UpToDate:
                case UpdateModel::Unchecked:
                case UpdateModel::Checking:
                    break;
                case UpdateModel::NeedUpdate:
                    qWarning() << "Need update";
                    [[fallthrough]];
                default:
                    openLegacyDialogIfNecessary();
                }
            }
        });
    }

    return m_updateModel.get();
}
#endif
