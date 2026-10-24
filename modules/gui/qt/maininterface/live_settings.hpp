/*****************************************************************************
 * live_settings.hpp : the settings VLC reads once, applied when they change
 *****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
#ifndef VLC_QT_LIVE_SETTINGS_HPP
#define VLC_QT_LIVE_SETTINGS_HPP

#include "qt.hpp"

#include <QByteArray>
#include <QHash>
#include <QString>
#include <QStringList>

#include <atomic>
#include <functional>
#include <memory>

class QObject;
class QThread;

// The settings VLC reads once, when it starts, applied again when they
// change, so that none waits for VLC to start again:
//  - the log (Show minor warnings, Save a log, its file, format, details):
//    opened again;
//  - the interfaces started with VLC (extraintf, control: the web remote,
//    Last.fm...): started or stopped, and started again when an option
//    they read when they start changes (the port, the password...);
//  - the hotkeys and the mouse wheels (the key map of VLC), the global
//    hotkeys (registered with the system when their interface starts);
//  - the options of the player set when VLC starts (libvlc.c): the default
//    playback speed (the speed of now and of the next media), start paused,
//    pause when another app uses audio, pause on the last frame.
// reload() (MainCtx::reloadPrefs, after each save of the settings) compares
// them with the values applied last. On the thread of the interface.
class LiveSettings
{
public:
    explicit LiveSettings(qt_intf_t *intf);
    ~LiveSettings();

    LiveSettings(const LiveSettings &) = delete;
    LiveSettings &operator=(const LiveSettings &) = delete;

    void reload();

private:
    void reloadLog();
    void reloadInterfaces();
    void reloadHotkeys();
    void reloadPlayer();
    // Runs on the thread of the interfaces, in the order asked; nothing once
    // the interface closes (closed is set then)
    void post(std::function<void(libvlc_int_t *, const std::atomic<bool> &closed)> work);

    qt_intf_t *p_intf;

    QString m_log;
    // the interfaces of extraintf and control, and the options each read
    // when it started
    QStringList m_interfaces;
    QHash<QString, QString> m_interfaceOptions;
    QByteArray m_hotkeys;
    QByteArray m_globalHotkeys;
    float m_rate = 1.f;
    bool m_startPaused = false;
    bool m_pauseOnCork = false;
    bool m_playAndPause = false;

    // An interface waits for its thread to end when it stops: never on the
    // thread of the interface, which a thread of an interface may wait for
    // (an action that needs the video window), but on one of their own, in
    // the order asked
    QThread *m_thread = nullptr;
    QObject *m_context = nullptr;
    // Set when the interface closes, read by the work still queued (shared:
    // the thread may outlive it)
    std::shared_ptr<std::atomic<bool>> m_closed = std::make_shared<std::atomic<bool>>(false);
};

#endif
