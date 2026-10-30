/*****************************************************************************
 * live_settings.cpp : the settings VLC reads once, applied when they change
 *****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "live_settings.hpp"

#include <vlc_actions.h>
#include <vlc_interface.h>
#include <vlc_player.h>
#include <vlc_plugin.h>

#include <QDeadlineTimer>
#include <QObject>
#include <QThread>

#include <cstdlib>
#include <initializer_list>

namespace {

// An option as VLC reads it (the command line first), as a text
QString optionText(vlc_object_t *obj, const char *name)
{
    const module_config_t *option = config_FindConfig(name);
    if (!option)
        return {};
    switch (CONFIG_CLASS(option->i_type))
    {
    case CONFIG_ITEM_STRING:
    {
        char *value = var_InheritString(obj, name);
        const QString text = QString::fromUtf8(value ? value : "");
        free(value);
        return text;
    }
    case CONFIG_ITEM_FLOAT:
        return QString::number(var_InheritFloat(obj, name));
    case CONFIG_ITEM_BOOL:
        return var_InheritBool(obj, name) ? QStringLiteral("1") : QStringLiteral("0");
    case CONFIG_ITEM_INTEGER:
        return QString::number(var_InheritInteger(obj, name));
    default:
        return {};
    }
}

// Options, a line each
QString optionsText(vlc_object_t *obj, std::initializer_list<const char *> names)
{
    QString text;
    for (const char *name : names)
        text += QLatin1String(name) + QLatin1Char('=') + optionText(obj, name) + QLatin1Char('\n');
    return text;
}

// The options of the loggers (src/misc/messages.c, modules/logger)
QString logOptions(vlc_object_t *obj)
{
    return optionsText(obj, { "verbose", "quiet", "file-logging", "logfile", "logmode", "log-verbose",
                              "syslog", "syslog-debug", "syslog-ident", "syslog-facility" });
}

// The module of an interface ("http" of "http{...}")
QString moduleOf(const QString &chain)
{
    return chain.section(QLatin1Char('{'), 0, 0).section(QLatin1Char(','), 0, 0).trimmed();
}

// The interfaces VLC starts with it, as it reads them (libvlc.c), each once
QStringList backgroundInterfaces(vlc_object_t *obj)
{
    QStringList interfaces;
    for (const char *list : { "extraintf", "control" })
    {
        char *value = var_InheritString(obj, list);
        const QStringList chains = QString::fromUtf8(value ? value : "").split(QLatin1Char(':'), Qt::SkipEmptyParts);
        free(value);
        for (const QString &chain : chains)
            // (never this interface: stopping it would wait for itself)
            if (!interfaces.contains(chain) && moduleOf(chain).compare(QLatin1String("qt"), Qt::CaseInsensitive) != 0)
                interfaces.append(chain);
    }
    return interfaces;
}

// The options an interface reads when it starts: it starts again when one
// of them changes (the web remote's port, Last.fm's account...)
QString startOptions(vlc_object_t *obj, const QString &module)
{
    const auto is = [&module](const char *name) {
        return module.compare(QLatin1String(name), Qt::CaseInsensitive) == 0;
    };
    if (is("http") || is("luahttp"))
        return optionsText(obj, { "http-host", "http-port", "http-password" });
    if (is("telnet") || is("luatelnet"))
        return optionsText(obj, { "telnet-host", "telnet-port", "telnet-password" });
    if (is("audioscrobbler"))
        return optionsText(obj, { "lastfm-username", "lastfm-password", "scrobbler-url" });
    return {};
}

// The bindings of the hotkeys (or of the global hotkeys); the mouse wheels
// with the hotkeys (the key map of VLC has them)
QByteArray hotkeyBindings(vlc_object_t *obj, bool global)
{
    QByteArray bindings;
    for (const char *const *name = vlc_actions_get_key_names(obj); name && *name; ++name)
    {
        QByteArray option(global ? "global-key-" : "key-");
        option.append(*name);
        char *keys = var_InheritString(obj, option.constData());
        bindings.append(option).append('=').append(keys ? keys : "").append('\n');
        free(keys);
    }
    if (!global)
        for (const char *wheel : { "hotkeys-x-wheel-mode", "hotkeys-y-wheel-mode" })
            bindings.append(wheel).append('=')
                    .append(QByteArray::number(var_InheritInteger(obj, wheel))).append('\n');
    return bindings;
}

} // namespace

LiveSettings::LiveSettings(qt_intf_t *intf)
    : p_intf(intf)
{
    // what VLC started with
    vlc_object_t *obj = VLC_OBJECT(p_intf);
    m_log = logOptions(obj);
    m_interfaces = backgroundInterfaces(obj);
    for (const QString &chain : m_interfaces)
        m_interfaceOptions.insert(chain, startOptions(obj, moduleOf(chain)));
    m_hotkeys = hotkeyBindings(obj, false);
    m_globalHotkeys = hotkeyBindings(obj, true);
    m_rate = var_InheritFloat(obj, "rate");
    m_startPaused = var_InheritBool(obj, "start-paused");
    m_pauseOnCork = var_InheritBool(obj, "playlist-cork");
    m_playAndPause = var_InheritBool(obj, "play-and-pause");
}

LiveSettings::~LiveSettings()
{
    m_closed->store(true);
    if (!m_thread)
        return;
    m_thread->quit();
    // Waited for to the end: the work in progress stops after its current
    // step (closed), one intf_Create() or intf_DestroyByName(). Left running,
    // it could start an interface after VLC destroyed them all, with no
    // event loop left to delete the thread (VLC waits for an interface that
    // doesn't end anyway, in intf_DestroyAll)
    if (!m_thread->wait(QDeadlineTimer(3000)))
    {
        msg_Warn(p_intf, "an interface is still starting or stopping at exit");
        m_thread->wait();
    }
    delete m_thread;
}

void LiveSettings::reload()
{
    reloadLog();
    reloadInterfaces();
    reloadHotkeys();
    reloadPlayer();
}

// Show minor warnings, Save a log, its file, its format, its details: the
// log opens again with them
void LiveSettings::reloadLog()
{
    const QString log = logOptions(VLC_OBJECT(p_intf));
    if (log == m_log)
        return;
    m_log = log;
    // while the Messages window has the messages, it opens the log of the
    // settings again when it lets them go
    if (!p_intf->logInMessages)
        vlc_LogReload(vlc_object_instance(p_intf));
}

// The web remote, Last.fm...: what was added starts, what was removed stops,
// what an option read at its start changed for starts again
void LiveSettings::reloadInterfaces()
{
    vlc_object_t *obj = VLC_OBJECT(p_intf);
    const QStringList interfaces = backgroundInterfaces(obj);
    QHash<QString, QString> options;
    for (const QString &chain : interfaces)
        options.insert(chain, startOptions(obj, moduleOf(chain)));

    QStringList stop, start;
    for (const QString &chain : m_interfaces)
        if (!interfaces.contains(chain) && !stop.contains(moduleOf(chain)))
            stop.append(moduleOf(chain));
    for (const QString &chain : interfaces)
        if (!m_interfaces.contains(chain) || m_interfaceOptions.value(chain) != options.value(chain))
            start.append(chain);
    m_interfaces = interfaces;
    m_interfaceOptions = options;
    if (stop.isEmpty() && start.isEmpty())
        return;

    post([stop, start](libvlc_int_t *libvlc, const std::atomic<bool> &closed) {
        for (const QString &module : stop)
        {
            if (closed.load())
                return;
            const int count = intf_DestroyByName(libvlc, qtu(module));
            msg_Dbg(libvlc, "interface \"%s\" stopped (%d)", qtu(module), count);
        }
        for (const QString &chain : start)
        {
            if (closed.load())
                return;
            // once: it may run already (an option changed, the Interfaces
            // menu started it)
            intf_DestroyByName(libvlc, qtu(moduleOf(chain)));
            if (intf_Create(libvlc, qtu(chain)) != VLC_SUCCESS)
                msg_Warn(libvlc, "interface \"%s\" could not start", qtu(chain));
            // (VLC began to close meanwhile: not left running past it)
            else if (closed.load())
                intf_DestroyByName(libvlc, qtu(moduleOf(chain)));
        }
    });
}

// The keyboard shortcuts, the mouse wheels: the key map of VLC is made
// again; the global hotkeys, registered with the system when their
// interface starts, start again with it
void LiveSettings::reloadHotkeys()
{
    vlc_object_t *obj = VLC_OBJECT(p_intf);
    const QByteArray hotkeys = hotkeyBindings(obj, false);
    const QByteArray globalHotkeys = hotkeyBindings(obj, true);
    if (hotkeys == m_hotkeys && globalHotkeys == m_globalHotkeys)
        return;
    m_hotkeys = hotkeys;
    vlc_actions_reload(obj);
    if (globalHotkeys == m_globalHotkeys)
        return;
    m_globalHotkeys = globalHotkeys;
    post([](libvlc_int_t *libvlc, const std::atomic<bool> &closed) {
        // (only where it runs: VLC starts it where the system has them)
        if (intf_DestroyByName(libvlc, "globalhotkeys") > 0 && !closed.load()
            && intf_Create(libvlc, "globalhotkeys") != VLC_SUCCESS)
            msg_Warn(libvlc, "the global hotkeys could not start again");
    });
}

// The options of the player VLC sets when it starts (libvlc.c), the ones
// that changed: the default playback speed (the speed of now, which the
// player keeps for the next media), Start paused, Pause when another app
// uses audio, Pause on the last frame of each item
void LiveSettings::reloadPlayer()
{
    vlc_object_t *obj = VLC_OBJECT(p_intf);
    const float rate = var_InheritFloat(obj, "rate");
    const bool startPaused = var_InheritBool(obj, "start-paused");
    const bool pauseOnCork = var_InheritBool(obj, "playlist-cork");
    const bool playAndPause = var_InheritBool(obj, "play-and-pause");
    if (rate == m_rate && startPaused == m_startPaused && pauseOnCork == m_pauseOnCork
        && playAndPause == m_playAndPause)
        return;
    vlc_player_t *player = p_intf->p_player;
    if (player)
    {
        vlc_player_Lock(player);
        if (rate != m_rate && rate != 0.f)
            vlc_player_ChangeRate(player, rate);
        if (startPaused != m_startPaused)
            vlc_player_SetStartPaused(player, startPaused);
        if (pauseOnCork != m_pauseOnCork)
            vlc_player_SetPauseOnCork(player, pauseOnCork);
        if (playAndPause != m_playAndPause)
            vlc_player_SetPlayAndPause(player, playAndPause);
        vlc_player_Unlock(player);
    }
    m_rate = rate;
    m_startPaused = startPaused;
    m_pauseOnCork = pauseOnCork;
    m_playAndPause = playAndPause;
}

void LiveSettings::post(std::function<void(libvlc_int_t *, const std::atomic<bool> &)> work)
{
    if (!m_thread)
    {
        m_thread = new QThread;
        m_thread->setObjectName(QStringLiteral("vlc-qt-interfaces"));
        m_context = new QObject;
        m_context->moveToThread(m_thread);
        QObject::connect(m_thread, &QThread::finished, m_context, &QObject::deleteLater);
        m_thread->start();
    }
    libvlc_int_t *libvlc = vlc_object_instance(p_intf);
    const std::shared_ptr<std::atomic<bool>> closed = m_closed;
    QMetaObject::invokeMethod(m_context, [libvlc, closed, work]() {
        if (!closed->load())
            work(libvlc, *closed);
    }, Qt::QueuedConnection);
}
