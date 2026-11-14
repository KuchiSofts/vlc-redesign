/*****************************************************************************
 * mac_integration.mm: the interface in macOS (Now Playing, media keys, Dock)
 *****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * Now Playing and the remote commands follow VLC's own macOS interface
 * (modules/gui/macosx/os-integration/VLCRemoteControlService.m, by
 * Carola Nitz and Felix Paul Kühne).
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

/* (the check(), verify() and require() of <AssertMacros.h> would clash with
 * C++ names) */
#ifndef __ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES
# define __ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES 0
#endif
#import <AppKit/AppKit.h>
#import <MediaPlayer/MediaPlayer.h>
#import <Metal/Metal.h>

#include <sys/resource.h>
#include <algorithm>
#include <cmath>

#include "mac_integration.hpp"

#include <QAction>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QMenu>
#include <QUrl>

#include "player/player_controller.hpp"
#include "playlist/playlist_controller.hpp"
#include "util/vlctick.hpp"

#if !__has_feature(objc_arc)
# error "mac_integration.mm is built with ARC (-fobjc-arc)"
#endif

/* (the background state of Darwin, <sys/resource.h>: hidden by some
 * feature macros) */
#ifndef PRIO_DARWIN_PROCESS
# define PRIO_DARWIN_PROCESS 4
#endif
#ifndef PRIO_DARWIN_BG
# define PRIO_DARWIN_BG 0x1000
#endif

using vlc::playlist::PlaylistController;

struct MacIntegration::Private
{
    qt_intf_t *intf = nullptr;
    PlayerController *player = nullptr;
    PlaylistController *playlist = nullptr;

    // the targets of the remote commands, removed with this object
    NSMutableArray *targets = nil;
    NSMutableArray *commands = nil;

    // Now Playing: the time told last, to tell it again only on a jump (the
    // system counts it on by the rate in between)
    double toldTime = -1.;
    double toldRate = 0.;
    QElapsedTimer toldAt;
    // the artwork of the item shown (loaded once per file)
    QString artworkPath;
    MPMediaItemArtwork *artwork = nil;

    std::unique_ptr<QMenu> dockMenu;
    QAction *playPause = nullptr;
    QAction *stop = nullptr;
    QAction *previous = nullptr;
    QAction *next = nullptr;
    QAction *mute = nullptr;

    bool playing() const
    {
        return player->getPlayingState() == PlayerController::PLAYING_STATE_PLAYING;
    }

    double timeSeconds() const
    {
        const VLCTime time = player->getTime();
        return time.valid() ? time.asMilliseconds() / 1000. : 0.;
    }

    void addCommand(MPRemoteCommand *command, MPRemoteCommandHandlerStatus (^handler)(MPRemoteCommandEvent *))
    {
        command.enabled = YES;
        id target = [command addTargetWithHandler:handler];
        if (target) {
            [targets addObject:target];
            [commands addObject:command];
        }
    }

    void subscribe()
    {
        targets = [NSMutableArray array];
        commands = [NSMutableArray array];
        MPRemoteCommandCenter *center = MPRemoteCommandCenter.sharedCommandCenter;
        PlayerController *const p = player;
        PlaylistController *const l = playlist;

        // (the ones VLC has no action for: off, as in VLC's macOS interface)
        center.ratingCommand.enabled = NO;
        center.likeCommand.enabled = NO;
        center.dislikeCommand.enabled = NO;
        center.bookmarkCommand.enabled = NO;
        center.enableLanguageOptionCommand.enabled = NO;
        center.disableLanguageOptionCommand.enabled = NO;
        center.seekForwardCommand.enabled = NO;
        center.seekBackwardCommand.enabled = NO;

        addCommand(center.playCommand, ^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *) {
            if (l->isEmpty())
                return MPRemoteCommandHandlerStatusNoActionableNowPlayingItem;
            l->play();
            return MPRemoteCommandHandlerStatusSuccess;
        });
        addCommand(center.pauseCommand, ^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *) {
            l->pause();
            return MPRemoteCommandHandlerStatusSuccess;
        });
        addCommand(center.togglePlayPauseCommand, ^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *) {
            if (l->isEmpty())
                return MPRemoteCommandHandlerStatusNoActionableNowPlayingItem;
            l->togglePlayPause();
            return MPRemoteCommandHandlerStatusSuccess;
        });
        addCommand(center.stopCommand, ^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *) {
            l->stop();
            return MPRemoteCommandHandlerStatusSuccess;
        });
        addCommand(center.nextTrackCommand, ^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *) {
            if (!l->hasNext())
                return MPRemoteCommandHandlerStatusNoSuchContent;
            l->next();
            return MPRemoteCommandHandlerStatusSuccess;
        });
        addCommand(center.previousTrackCommand, ^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *) {
            if (l->isEmpty())
                return MPRemoteCommandHandlerStatusNoSuchContent;
            l->prev();
            return MPRemoteCommandHandlerStatusSuccess;
        });

        // the jumps of the settings ("Short jump"), as the keys of VLC
        const int jump = std::max<int64_t>(1, var_InheritInteger(intf, "short-jump-size"));
        center.skipForwardCommand.preferredIntervals = @[@(jump)];
        center.skipBackwardCommand.preferredIntervals = @[@(jump)];
        addCommand(center.skipForwardCommand, ^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *) {
            p->jumpFwd();
            return MPRemoteCommandHandlerStatusSuccess;
        });
        addCommand(center.skipBackwardCommand, ^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *) {
            p->jumpBwd();
            return MPRemoteCommandHandlerStatusSuccess;
        });
        addCommand(center.changePlaybackPositionCommand, ^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *event) {
            if (!p->isSeekable())
                return MPRemoteCommandHandlerStatusCommandFailed;
            const double seconds = ((MPChangePlaybackPositionCommandEvent *)event).positionTime;
            p->setTime(VLC_TICK_FROM_MS(std::llround(std::max(0., seconds) * 1000.)));
            return MPRemoteCommandHandlerStatusSuccess;
        });
        addCommand(center.changeRepeatModeCommand, ^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *event) {
            switch (((MPChangeRepeatModeCommandEvent *)event).repeatType) {
            case MPRepeatTypeAll:
                l->setRepeatMode(PlaylistController::PLAYBACK_REPEAT_ALL);
                break;
            case MPRepeatTypeOne:
                l->setRepeatMode(PlaylistController::PLAYBACK_REPEAT_CURRENT);
                break;
            default:
                l->setRepeatMode(PlaylistController::PLAYBACK_REPEAT_NONE);
                break;
            }
            return MPRemoteCommandHandlerStatusSuccess;
        });
        addCommand(center.changeShuffleModeCommand, ^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *event) {
            l->setRandom(((MPChangeShuffleModeCommandEvent *)event).shuffleType != MPShuffleTypeOff);
            return MPRemoteCommandHandlerStatusSuccess;
        });
    }

    void unsubscribe()
    {
        for (NSUInteger i = 0; i < targets.count; ++i) {
            MPRemoteCommand *command = commands[i];
            [command removeTarget:targets[i]];
        }
        targets = nil;
        commands = nil;
        MPNowPlayingInfoCenter *info = MPNowPlayingInfoCenter.defaultCenter;
        info.nowPlayingInfo = nil;
        info.playbackState = MPNowPlayingPlaybackStateStopped;
    }

    void updateState()
    {
        MPNowPlayingInfoCenter *info = MPNowPlayingInfoCenter.defaultCenter;
        MPNowPlayingPlaybackState state;
        const char *name;
        switch (player->getPlayingState()) {
        case PlayerController::PLAYING_STATE_PLAYING:
            state = MPNowPlayingPlaybackStatePlaying;
            name = "playing";
            break;
        case PlayerController::PLAYING_STATE_PAUSED:
            state = MPNowPlayingPlaybackStatePaused;
            name = "paused";
            break;
        case PlayerController::PLAYING_STATE_STOPPING:
        case PlayerController::PLAYING_STATE_STOPPED:
            state = MPNowPlayingPlaybackStateStopped;
            name = "stopped";
            break;
        default:
            state = MPNowPlayingPlaybackStateUnknown;
            name = "unknown";
            break;
        }
        if (info.playbackState == state)
            return;
        info.playbackState = state;
        // (the launch test looks for it, ci/macos-smoke.sh)
        NSString *title = info.nowPlayingInfo[MPMediaItemPropertyTitle];
        msg_Dbg(intf, "macOS: Now Playing: %s%s%s", name, title ? ", " : "", title ? title.UTF8String : "");
    }

    // the time and rate in a copy of what Now Playing shows
    void putTime(NSMutableDictionary *dictionary)
    {
        toldTime = timeSeconds();
        toldRate = playing() ? player->getRate() : 0.;
        toldAt.start();
        dictionary[MPNowPlayingInfoPropertyElapsedPlaybackTime] = @(toldTime);
        dictionary[MPNowPlayingInfoPropertyPlaybackRate] = @(toldRate);
        dictionary[MPNowPlayingInfoPropertyDefaultPlaybackRate] = @(1.0);
    }

    void updateTime(bool always)
    {
        MPNowPlayingInfoCenter *info = MPNowPlayingInfoCenter.defaultCenter;
        if (!info.nowPlayingInfo)
            return;
        if (!always && toldAt.isValid()) {
            // where the system has it by now: told again past a second off
            const double expected = toldTime + toldRate * (toldAt.elapsed() / 1000.);
            if (std::fabs(timeSeconds() - expected) < 1.)
                return;
        }
        NSMutableDictionary *dictionary = [info.nowPlayingInfo mutableCopy];
        putTime(dictionary);
        info.nowPlayingInfo = dictionary;
    }

    MPMediaItemArtwork *artworkFor(const QUrl &url)
    {
        const QString path = url.isLocalFile() ? url.toLocalFile() : QString();
        if (path == artworkPath)
            return artwork;
        artworkPath = path;
        artwork = nil;
        if (path.isEmpty() || !QFileInfo::exists(path))
            return nil;
        NSImage *image = [[NSImage alloc] initWithContentsOfFile:path.toNSString()];
        if (!image || image.size.width <= 0 || image.size.height <= 0)
            return nil;
        // (the deployment target of the build is older than the app's
        // macOS 12)
        if (@available(macOS 10.13.2, *))
            artwork = [[MPMediaItemArtwork alloc] initWithBoundsSize:image.size
                                                      requestHandler:^NSImage *(CGSize) { return image; }];
        return artwork;
    }

    void updateMeta()
    {
        MPNowPlayingInfoCenter *info = MPNowPlayingInfoCenter.defaultCenter;
        if (!player->isStarted()) {
            info.nowPlayingInfo = nil;
            updateState();
            return;
        }
        NSMutableDictionary *dictionary = [NSMutableDictionary dictionary];
        QString title = player->getTitle();
        if (title.isEmpty())
            title = player->getName();
        dictionary[MPMediaItemPropertyTitle] = title.toNSString();
        const QString artist = player->getArtist();
        if (!artist.isEmpty())
            dictionary[MPMediaItemPropertyArtist] = artist.toNSString();
        const QString album = player->getAlbum();
        if (!album.isEmpty())
            dictionary[MPMediaItemPropertyAlbumTitle] = album.toNSString();
        const VLCDuration length = player->getLength();
        const bool live = !length.valid() || length.asMilliseconds() <= 0;
        if (!live)
            dictionary[MPMediaItemPropertyPlaybackDuration] = @(length.asMilliseconds() / 1000.);
        dictionary[MPNowPlayingInfoPropertyIsLiveStream] = @(live);
        dictionary[MPNowPlayingInfoPropertyMediaType] = @(player->hasVideoOutput() ? MPNowPlayingInfoMediaTypeVideo
                                                                                 : MPNowPlayingInfoMediaTypeAudio);
        if (MPMediaItemArtwork *art = artworkFor(player->getArtwork()))
            dictionary[MPMediaItemPropertyArtwork] = art;
        putTime(dictionary);
        info.nowPlayingInfo = dictionary;
        updateState();
    }

    void updateDockMenu()
    {
        if (!dockMenu)
            return;
        playPause->setText(playing() ? qtr("Pause") : qtr("Play"));
        playPause->setEnabled(!playlist->isEmpty());
        stop->setEnabled(player->isStarted());
        previous->setEnabled(!playlist->isEmpty());
        next->setEnabled(playlist->hasNext());
        mute->setChecked(player->isMuted());
    }

    void makeDockMenu()
    {
        dockMenu = std::make_unique<QMenu>();
        PlayerController *const p = player;
        PlaylistController *const l = playlist;
        playPause = dockMenu->addAction(qtr("Play"));
        QObject::connect(playPause, &QAction::triggered, l, [l]() { l->togglePlayPause(); });
        stop = dockMenu->addAction(qtr("Stop"));
        QObject::connect(stop, &QAction::triggered, l, [l]() { l->stop(); });
        previous = dockMenu->addAction(qtr("Previous"));
        QObject::connect(previous, &QAction::triggered, l, [l]() { l->prev(); });
        next = dockMenu->addAction(qtr("Next"));
        QObject::connect(next, &QAction::triggered, l, [l]() { l->next(); });
        dockMenu->addSeparator();
        mute = dockMenu->addAction(qtr("Mute"));
        mute->setCheckable(true);
        QObject::connect(mute, &QAction::triggered, p, [p]() { p->toggleMuted(); });
        updateDockMenu();
        dockMenu->setAsDockMenu();
    }
};

MacIntegration::MacIntegration(qt_intf_t *intf, QWindow *window)
    : QObject(nullptr)
    , d(std::make_unique<Private>())
{
    Q_UNUSED(window);
    d->intf = intf;
    d->player = intf->p_mainPlayerController;
    d->playlist = intf->p_mainPlaylistController;
    if (!d->player || !d->playlist)
        return;

    d->subscribe();
    d->makeDockMenu();

    Private *const p = d.get();
    PlayerController *const player = d->player;
    PlaylistController *const playlist = d->playlist;
    connect(player, &PlayerController::playingStateChanged, this, [p]() {
        p->updateTime(true);
        p->updateState();
        p->updateDockMenu();
    });
    connect(player, &PlayerController::inputChanged, this, [p]() {
        p->updateMeta();
        p->updateDockMenu();
    });
    connect(player, &PlayerController::currentMetaChanged, this, [p]() { p->updateMeta(); });
    connect(player, &PlayerController::nameChanged, this, [p]() { p->updateMeta(); });
    connect(player, &PlayerController::lengthChanged, this, [p]() { p->updateMeta(); });
    connect(player, &PlayerController::hasVideoOutputChanged, this, [p]() { p->updateMeta(); });
    connect(player, &PlayerController::rateChanged, this, [p]() { p->updateTime(true); });
    connect(player, &PlayerController::timeChanged, this, [p]() { p->updateTime(false); });
    connect(player, &PlayerController::soundMuteChanged, this, [p]() { p->updateDockMenu(); });
    connect(playlist, &PlaylistController::countChanged, this, [p]() { p->updateDockMenu(); });
    connect(playlist, &PlaylistController::hasNextChanged, this, [p]() { p->updateDockMenu(); });
    connect(playlist, &PlaylistController::hasPrevChanged, this, [p]() { p->updateDockMenu(); });

    d->updateMeta();
    QStringList items;
    for (const QAction *action : d->dockMenu->actions())
        if (!action->isSeparator())
            items.append(action->text());
    msg_Dbg(intf, "macOS: Now Playing and %lld remote commands ready; the Dock menu: %s",
            (long long)[d->commands count], qtu(items.join(QStringLiteral(", "))));
}

MacIntegration::~MacIntegration()
{
    // (the connections to this object go before what they reach)
    if (d->player)
        QObject::disconnect(d->player, nullptr, this, nullptr);
    if (d->playlist)
        QObject::disconnect(d->playlist, nullptr, this, nullptr);
    if (d->targets)
        d->unsubscribe();
}

void mac_platform::setProcessPriority(int priority)
{
    static int applied = 0;
    static id<NSObject> activity = nil;
    priority = std::clamp(priority, -2, 2);
    // (no lower priority that macOS lets a program take back: normal)
    if (priority == -1)
        priority = 0;
    if (priority == applied)
        return;
    setpriority(PRIO_DARWIN_PROCESS, 0, priority < 0 ? PRIO_DARWIN_BG : 0);
    if (activity) {
        [NSProcessInfo.processInfo endActivity:activity];
        activity = nil;
    }
    if (priority > 0) {
        // kept running at full speed (no App Nap, no timer coalescing),
        // the system still free to sleep when idle
        NSActivityOptions options = NSActivityUserInitiatedAllowingIdleSystemSleep;
        if (priority > 1)
            options |= NSActivityLatencyCritical;
        activity = [NSProcessInfo.processInfo beginActivityWithOptions:options
                                                                reason:@"Process priority of the settings"];
    }
    applied = priority;
}

QStringList mac_platform::graphicsAdapters()
{
    QStringList names;
    for (id<MTLDevice> device in MTLCopyAllDevices()) {
        QString name = QString::fromNSString(device.name).trimmed();
        if (device.removable)
            name += QStringLiteral(" (external)");
        else if (device.lowPower)
            name += QStringLiteral(" (integrated)");
        if (!name.isEmpty() && !names.contains(name))
            names.append(name);
    }
    return names;
}

QList<QPair<QPoint, bool>> mac_platform::screensHdr()
{
    QList<QPair<QPoint, bool>> screens;
    NSArray<NSScreen *> *all = NSScreen.screens;
    if (all.count == 0)
        return screens;
    // AppKit counts from the bottom left of the main screen, Qt from its top
    // left
    const CGFloat height = all[0].frame.size.height;
    for (NSScreen *screen in all) {
        const NSRect frame = screen.frame;
        const QPoint topLeft(qRound(frame.origin.x), qRound(height - frame.origin.y - frame.size.height));
        bool hdr = false;
        if (@available(macOS 10.15, *))
            hdr = screen.maximumPotentialExtendedDynamicRangeColorComponentValue > 1.0;
        screens.append({ topLeft, hdr });
    }
    return screens;
}
