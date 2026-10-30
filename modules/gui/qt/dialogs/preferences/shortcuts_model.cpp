/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
/****************************************************************************
 * shortcuts_model.cpp : Staged VLC keyboard shortcuts for QML
 ****************************************************************************/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "shortcuts_model.hpp"
#include "maininterface/mainctx.hpp"
#include "qt.hpp"
#include "util/vlchotkeyconverter.hpp"

#include <vlc_actions.h>
#include <vlc_configuration.h>
#include <vlc_modules.h>
#include <vlc_plugin.h>

#include <QKeyEvent>
#include <QWheelEvent>
#include <QStringList>
#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace {
// VLC keeps the keys of an action in one string, a tab between them
QStringList splitKeys(const QString &keys)
{
    return keys.split(QLatin1Char('\t'), Qt::SkipEmptyParts);
}
}

ShortcutsModel::ShortcutsModel(QObject *parent) : QAbstractListModel(parent) {}

MainCtx *ShortcutsModel::ctx() const { return m_ctx.data(); }

void ShortcutsModel::setCtx(MainCtx *ctx)
{
    if (m_ctx == ctx)
        return;
    if (m_ctx)
        disconnect(m_ctx, &MainCtx::interfaceLanguageChanged, this, &ShortcutsModel::retranslate);
    m_ctx = ctx;
    if (m_ctx)
        connect(m_ctx, &MainCtx::interfaceLanguageChanged, this, &ShortcutsModel::retranslate);
    emit ctxChanged();
    reload();
}

// Another language of the interface: the names of the actions and of their
// groups in it, the keys staged kept
void ShortcutsModel::retranslate()
{
    for (Entry &entry : m_entries) {
        const QByteArray name = entry.key.toUtf8();
        const module_config_t *item = config_FindConfig(name.constData());
        if (item && item->psz_text)
            entry.action = qfut(item->psz_text);
        entry.group = groupFor(entry.key);
    }
    // a search may find other rows in the new names
    if (!m_search.isEmpty())
        rebuildRows();
    else if (!m_rows.isEmpty())
        emit dataChanged(index(0), index(m_rows.size() - 1), {ActionRole, GroupRole});
}

void ShortcutsModel::setSearch(const QString &search)
{
    if (m_search == search)
        return;
    m_search = search;
    emit searchChanged();
    rebuildRows();
}

void ShortcutsModel::setSearchField(const QString &field)
{
    if (m_searchField == field)
        return;
    m_searchField = field;
    emit searchFieldChanged();
    rebuildRows();
}

void ShortcutsModel::setLiveTv(bool liveTv)
{
    if (m_liveTv == liveTv)
        return;
    m_liveTv = liveTv;
    emit liveTvChanged();
    rebuildRows();
}

bool ShortcutsModel::dirty() const
{
    for (const Entry &entry : m_entries)
        if (entry.staged != entry.current || entry.globalStaged != entry.globalCurrent)
            return true;
    return false;
}

int ShortcutsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_rows.size();
}

QVariant ShortcutsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return {};
    const Entry &entry = m_entries[m_rows[index.row()]];
    switch (role) {
    case KeyRole: return entry.key;
    case ActionRole: return entry.action;
    case GroupRole: return entry.group;
    case HotkeyRole: return QString(entry.staged).replace(QLatin1Char('\t'), QStringLiteral(", "));
    case GlobalHotkeyRole: return QString(entry.globalStaged).replace(QLatin1Char('\t'), QStringLiteral(", "));
    case DirtyRole: return entry.staged != entry.current || entry.globalStaged != entry.globalCurrent;
    case HotkeysRole: return splitKeys(entry.staged);
    case GlobalHotkeysRole: return splitKeys(entry.globalStaged);
    case HasGlobalRole: return !entry.globalKey.isEmpty();
    default: return {};
    }
}

QHash<int, QByteArray> ShortcutsModel::roleNames() const
{
    return {{KeyRole, "key"}, {ActionRole, "action"}, {GroupRole, "group"},
            {HotkeyRole, "hotkey"}, {GlobalHotkeyRole, "globalHotkey"},
            {DirtyRole, "dirty"}, {HotkeysRole, "hotkeys"},
            {GlobalHotkeysRole, "globalHotkeys"}, {HasGlobalRole, "hasGlobal"}};
}

bool ShortcutsModel::visible(const Entry &entry) const
{
    if (entry.tv && !m_liveTv)
        return false;
    if (m_search.isEmpty())
        return true;
    const auto match = [this](const QString &text) {
        return text.contains(m_search, Qt::CaseInsensitive);
    };
    if (m_searchField == QLatin1String("action"))
        return match(entry.action);
    if (m_searchField == QLatin1String("hotkey"))
        return match(entry.staged);
    if (m_searchField == QLatin1String("global"))
        return match(entry.globalStaged);
    return match(entry.action) || match(entry.staged) || match(entry.globalStaged);
}

void ShortcutsModel::rebuildRows()
{
    beginResetModel();
    m_rows.clear();
    for (int i = 0; i < m_entries.size(); ++i)
        if (visible(m_entries[i]))
            m_rows.append(i);
    endResetModel();
}

void ShortcutsModel::refreshSearch()
{
    // without a search every row stays: no reset (the list keeps its place)
    if (!m_search.isEmpty())
        rebuildRows();
}

void ShortcutsModel::notifyEntry(int entryIndex)
{
    const int row = m_rows.indexOf(entryIndex);
    if (row >= 0)
        emit dataChanged(index(row), index(row), {HotkeyRole, GlobalHotkeyRole, DirtyRole,
                                                  HotkeysRole, GlobalHotkeysRole});
}

void ShortcutsModel::setError(const QString &error)
{
    if (m_error == error)
        return;
    m_error = error;
    emit errorChanged();
}

int ShortcutsModel::findEntry(const QString &key) const
{
    for (int i = 0; i < m_entries.size(); ++i)
        if (m_entries[i].key == key)
            return i;
    return -1;
}

QString ShortcutsModel::readKey(const QString &key)
{
    const QByteArray name = key.toUtf8();
    char *value = config_GetPsz(name.constData());
    const QString result = QString::fromUtf8(value ? value : "");
    free(value);
    return result;
}

void ShortcutsModel::putKey(const QString &key, const QString &value)
{
    const QByteArray name = key.toUtf8();
    const QByteArray utf8 = value.toUtf8();
    config_PutPsz(name.constData(), utf8.constData());
}

QString ShortcutsModel::groupFor(const QString &key)
{
    if (key.startsWith(QLatin1String("qt-key-tv-")))
        return qtr("Live TV");
    if (key.startsWith(QLatin1String("qt-key-intf-")))
        return qtr("Interface");
    if (key == QLatin1String("key-toggle-fullscreen")
        || key == QLatin1String("key-leave-fullscreen")
        || key == QLatin1String("key-position")
        || key == QLatin1String("key-intf-show"))
        return qtr("Basic");
    if (key.contains(QLatin1String("vol-")) || key.contains(QLatin1String("audio")))
        return qtr("Audio");
    if (key.contains(QLatin1String("sub")) || key.contains(QLatin1String("caption")))
        return qtr("Subtitles");
    if (key.contains(QLatin1String("nav-")) || key.contains(QLatin1String("disc-")))
        return qtr("Navigation");
    if (key.contains(QLatin1String("crop")) || key.contains(QLatin1String("zoom"))
        || key.contains(QLatin1String("aspect")) || key.contains(QLatin1String("deinterlace"))
        || key.contains(QLatin1String("snapshot")))
        return qtr("Video");
    return qtr("Playback");
}

int ShortcutsModel::groupOrder(const QString &group)
{
    if (group == qtr("Basic")) return 0;
    if (group == qtr("Playback")) return 1;
    if (group == qtr("Audio")) return 2;
    if (group == qtr("Video")) return 3;
    if (group == qtr("Subtitles")) return 4;
    if (group == qtr("Interface")) return 6;
    if (group == qtr("Live TV")) return 7;
    return 5;
}

void ShortcutsModel::refresh()
{
    if (!m_loaded.isValid() || m_loaded.elapsed() > 1000)
        reload();
}

void ShortcutsModel::reload()
{
    m_loaded.start();
    const bool wasDirty = dirty();
    // The entries made again aside
    QVector<Entry> previous;
    previous.swap(m_entries);
    if (m_ctx) {
        module_t *main = module_get_main();
        if (main) {
            unsigned count = 0;
            module_config_t *configs = module_config_get(main, &count);
            if (configs) {
                for (unsigned i = 0; i < count; ++i) {
                    const module_config_t &item = configs[i];
                    if (item.i_type != CONFIG_ITEM_KEY || !item.psz_name
                        || !item.psz_text || !*item.psz_text
                        || strncmp(item.psz_name, "global-", 7) == 0)
                        continue;
                    Entry entry;
                    entry.key = QString::fromLatin1(item.psz_name);
                    entry.globalKey = QStringLiteral("global-") + entry.key;
                    if (!config_FindConfig(entry.globalKey.toUtf8().constData()))
                        entry.globalKey.clear();
                    entry.action = qfut(item.psz_text);
                    entry.group = groupFor(entry.key);
                    entry.groupRank = groupOrder(entry.group);
                    entry.current = readKey(entry.key);
                    entry.staged = entry.current;
                    entry.defaults = QString::fromUtf8(item.orig.psz ? item.orig.psz : "");
                    if (!entry.globalKey.isEmpty()) {
                        entry.globalCurrent = readKey(entry.globalKey);
                        entry.globalStaged = entry.globalCurrent;
                        const QByteArray globalName = entry.globalKey.toUtf8();
                        const module_config_t *globalItem = config_FindConfig(globalName.constData());
                        entry.globalDefaults = QString::fromUtf8(
                            globalItem && globalItem->orig.psz ? globalItem->orig.psz : "");
                    }
                    m_entries.append(entry);
                }
                module_config_free(configs);
            }
        }
        // the keys of live TV: the interface's options (qt.cpp)
        for (const char *name : { "qt-key-tv-channel-up", "qt-key-tv-channel-down",
                                  "qt-key-tv-previous", "qt-key-tv-list", "qt-key-tv-info" }) {
            const module_config_t *item = config_FindConfig(name);
            if (!item || !item->psz_text)
                continue;
            Entry entry;
            entry.key = QString::fromLatin1(name);
            entry.action = qfut(item->psz_text);
            entry.group = groupFor(entry.key);
            entry.groupRank = groupOrder(entry.group);
            entry.current = readKey(entry.key);
            entry.staged = entry.current;
            entry.defaults = QString::fromUtf8(item->orig.psz ? item->orig.psz : "");
            entry.tv = true;
            m_entries.append(entry);
        }
        // the interface's zoom (qt.cpp): Ctrl+wheel out of the video
        for (const char *name : { "qt-key-intf-zoom-in", "qt-key-intf-zoom-out" }) {
            const module_config_t *item = config_FindConfig(name);
            if (!item || !item->psz_text)
                continue;
            Entry entry;
            entry.key = QString::fromLatin1(name);
            entry.action = qfut(item->psz_text);
            entry.group = groupFor(entry.key);
            entry.groupRank = groupOrder(entry.group);
            entry.current = readKey(entry.key);
            entry.staged = entry.current;
            entry.defaults = QString::fromUtf8(item->orig.psz ? item->orig.psz : "");
            entry.zoom = true;
            m_entries.append(entry);
        }
        const QStringList firstActions = {
            QStringLiteral("key-toggle-fullscreen"), QStringLiteral("key-leave-fullscreen"),
            QStringLiteral("key-position"), QStringLiteral("key-intf-show"),
            QStringLiteral("key-play-pause"), QStringLiteral("key-stop"),
            QStringLiteral("key-next"), QStringLiteral("key-prev"),
            QStringLiteral("key-program-sid-next")
        };
        for (Entry &entry : m_entries) {
            const int first = firstActions.indexOf(entry.key);
            entry.firstRank = first < 0 ? 100 : first;
        }
        // (groupOrder translates the names of the groups: once per row,
        // not at each of the thousands of comparisons)
        std::stable_sort(m_entries.begin(), m_entries.end(), [](const Entry &a, const Entry &b) {
            if (a.groupRank != b.groupRank)
                return a.groupRank < b.groupRank;
            return a.firstRank < b.firstRank;
        });
    }
    QVector<Entry> fresh;
    fresh.swap(m_entries);
    QVector<int> freshRows;
    for (int i = 0; i < fresh.size(); ++i)
        if (visible(fresh[i]))
            freshRows.append(i);
    // The same actions in the same rows (the dialog opened, its list made
    // with it a moment before): their keys are told, the rows of the list
    // are not made again
    bool sameRows = !m_rows.isEmpty() && m_rows == freshRows && previous.size() == fresh.size();
    for (int i = 0; sameRows && i < fresh.size(); ++i)
        sameRows = previous[i].key == fresh[i].key;
    if (sameRows) {
        m_entries.swap(fresh);
        emit dataChanged(index(0), index(m_rows.size() - 1));
    } else {
        m_entries.swap(previous);
        beginResetModel();
        m_entries.swap(fresh);
        m_rows = freshRows;
        endResetModel();
    }
    if (wasDirty)
        emit dirtyChanged();
    setError({});
}

void ShortcutsModel::cancel()
{
    const bool wasDirty = dirty();
    for (int i = 0; i < m_entries.size(); ++i) {
        Entry &entry = m_entries[i];
        entry.staged = entry.current;
        entry.globalStaged = entry.globalCurrent;
        notifyEntry(i);
    }
    if (wasDirty)
        emit dirtyChanged();
    setError({});
    rebuildRows();
}

void ShortcutsModel::resetAll()
{
    if (!m_ctx)
        return;
    const bool wasDirty = dirty();
    for (int i = 0; i < m_entries.size(); ++i) {
        Entry &entry = m_entries[i];
        entry.staged = entry.defaults;
        if (!entry.globalKey.isEmpty())
            entry.globalStaged = entry.globalDefaults;
        notifyEntry(i);
    }
    if (wasDirty != dirty())
        emit dirtyChanged();
    setError({});
    rebuildRows();
}

QVariantMap ShortcutsModel::stageKey(const QString &option, bool global,
                                     int qtKey, int modifiers, bool replaceConflict)
{
    const int target = findEntry(option);
    if (target < 0 || (global && m_entries[target].globalKey.isEmpty())) {
        setError(qtr("This shortcut is unavailable."));
        return {{QStringLiteral("ok"), false}};
    }
    if (qtKey == Qt::Key_Tab || qtKey == Qt::Key_Shift || qtKey == Qt::Key_Control
        || qtKey == Qt::Key_Meta || qtKey == Qt::Key_Alt || qtKey == Qt::Key_AltGr)
        return {{QStringLiteral("ok"), false}, {QStringLiteral("modifierOnly"), true}};

    QKeyEvent event(QEvent::KeyPress, qtKey, Qt::KeyboardModifiers::fromInt(modifiers));
    return stageVlcKey(target, global, qtEventToVLCKey(&event), replaceConflict);
}

QVariantMap ShortcutsModel::stageWheel(const QString &option, bool global,
                                       bool up, int modifiers, bool replaceConflict)
{
    const int target = findEntry(option);
    // (a global hotkey is the system's: no wheel there)
    if (target < 0 || global) {
        setError(global ? qtr("This key combination cannot be used by VLC.")
                        : qtr("This shortcut is unavailable."));
        return {{QStringLiteral("ok"), false}};
    }
    QWheelEvent event(QPointF(), QPointF(), QPoint(), QPoint(0, up ? 120 : -120),
                      Qt::NoButton, Qt::KeyboardModifiers::fromInt(modifiers),
                      Qt::NoScrollPhase, false);
    const int vlcKey = qtKeyModifiersToVLC(event) | (up ? KEY_MOUSEWHEELUP : KEY_MOUSEWHEELDOWN);
    return stageVlcKey(target, false, vlcKey, replaceConflict);
}

bool ShortcutsModel::canConflict(const Entry &a, const Entry &b)
{
    if (a.tv != b.tv)
        return false;
    // the subtitles' size over the video, the interface's zoom elsewhere
    const auto subtitleSize = [](const Entry &entry) {
        return entry.key.startsWith(QLatin1String("key-subtitle-text-scale-"));
    };
    if ((a.zoom && subtitleSize(b)) || (b.zoom && subtitleSize(a)))
        return false;
    return true;
}

QVariantMap ShortcutsModel::stageVlcKey(int target, bool global, int vlcKey, bool replaceConflict)
{
    const QString canonical = VLCKeyToString(vlcKey, false);
    if (vlcKey == 0 || canonical.isEmpty()) {
        setError(qtr("This key combination cannot be used by VLC."));
        return {{QStringLiteral("ok"), false}};
    }
    setError({});
    // by the key code, not the text: the modifiers of a default key or of a
    // vlcrc edited by hand come in any order ("Command+Shift+Alt+Left"),
    // and in any case ("shift+e")
    const auto sameKey = [vlcKey](const QString &key) {
        return vlc_str2keycode(qtu(key)) == static_cast<uint_fast32_t>(vlcKey);
    };
    QVector<int> conflicts;
    for (int i = 0; i < m_entries.size(); ++i) {
        if (i == target || !canConflict(m_entries[i], m_entries[target]))
            continue;
        const QString &keys = global ? m_entries[i].globalStaged : m_entries[i].staged;
        const QStringList parts = splitKeys(keys);
        if (std::any_of(parts.cbegin(), parts.cend(), sameKey))
            conflicts.append(i);
    }
    if (!conflicts.isEmpty() && !replaceConflict)
        return {{QStringLiteral("ok"), false},
                {QStringLiteral("needsConfirmation"), true},
                {QStringLiteral("conflictAction"), m_entries[conflicts.first()].action},
                {QStringLiteral("key"), canonical}};

    const bool wasDirty = dirty();
    for (int i : conflicts) {
        QString &keys = global ? m_entries[i].globalStaged : m_entries[i].staged;
        QStringList parts = splitKeys(keys);
        parts.erase(std::remove_if(parts.begin(), parts.end(), sameKey), parts.end());
        keys = parts.join(QLatin1Char('\t'));
        notifyEntry(i);
    }
    // added after the keys the action has (pressed again: nothing changes)
    QString &targetKeys = global ? m_entries[target].globalStaged : m_entries[target].staged;
    QStringList targetParts = splitKeys(targetKeys);
    if (std::none_of(targetParts.cbegin(), targetParts.cend(), sameKey))
        targetParts.append(canonical);
    targetKeys = targetParts.join(QLatin1Char('\t'));
    notifyEntry(target);
    if (wasDirty != dirty())
        emit dirtyChanged();
    setError({});
    refreshSearch();
    return {{QStringLiteral("ok"), true}, {QStringLiteral("key"), canonical}};
}

bool ShortcutsModel::clearKey(const QString &option, bool global)
{
    const int target = findEntry(option);
    if (target < 0 || (global && m_entries[target].globalKey.isEmpty())) {
        setError(qtr("This shortcut is unavailable."));
        return false;
    }
    const bool wasDirty = dirty();
    QString &keys = global ? m_entries[target].globalStaged : m_entries[target].staged;
    keys.clear();
    notifyEntry(target);
    if (wasDirty != dirty())
        emit dirtyChanged();
    setError({});
    refreshSearch();
    return true;
}

bool ShortcutsModel::removeKey(const QString &option, bool global, const QString &key)
{
    const int target = findEntry(option);
    if (target < 0 || (global && m_entries[target].globalKey.isEmpty())) {
        setError(qtr("This shortcut is unavailable."));
        return false;
    }
    QString &keys = global ? m_entries[target].globalStaged : m_entries[target].staged;
    QStringList parts = splitKeys(keys);
    if (parts.removeAll(key) == 0)
        return false;
    const bool wasDirty = dirty();
    keys = parts.join(QLatin1Char('\t'));
    notifyEntry(target);
    if (wasDirty != dirty())
        emit dirtyChanged();
    setError({});
    refreshSearch();
    return true;
}

bool ShortcutsModel::apply()
{
    if (!m_ctx || !m_ctx->getIntf()) {
        setError(qtr("VLC preferences are unavailable."));
        return false;
    }
    QVector<Change> changes;
    for (const Entry &entry : m_entries) {
        if (entry.staged != entry.current)
            changes.append({entry.key, entry.action, entry.current, entry.staged, false});
        if (!entry.globalKey.isEmpty() && entry.globalStaged != entry.globalCurrent)
            changes.append({entry.globalKey, entry.action, entry.globalCurrent, entry.globalStaged, true});
    }
    for (const Change &change : changes) {
        if (readKey(change.key) != change.before) {
            // the action by its name, not by the option of VLC
            setError((change.global
                      ? qtr("The global hotkeys of \"%1\" changed outside this dialog. Cancel and reopen the shortcuts.")
                      : qtr("The hotkeys of \"%1\" changed outside this dialog. Cancel and reopen the shortcuts."))
                         .arg(change.action));
            return false;
        }
    }
    if (changes.isEmpty()) {
        setError({});
        return true;
    }
    for (const Change &change : changes) {
        putKey(change.key, change.after);
        if (readKey(change.key) != change.after) {
            for (const Change &rollback : changes)
                putKey(rollback.key, rollback.before);
            setError(qtr("A shortcut could not be changed. Changes were reverted in memory."));
            return false;
        }
    }
    const int result = config_SaveConfigFile(m_ctx->getIntf());
    if (result != 0) {
        for (const Change &change : changes)
            putKey(change.key, change.before);
        setError(qtr("Could not save shortcuts (error %1). Changes were reverted in memory; verify the configuration file.")
                     .arg(result));
        return false;
    }
    for (int i = 0; i < m_entries.size(); ++i) {
        Entry &entry = m_entries[i];
        entry.current = entry.staged;
        entry.globalCurrent = entry.globalStaged;
        notifyEntry(i);
    }
    // the next keys pressed use them; the global hotkeys register again
    m_ctx->reloadPrefs();
    emit dirtyChanged();
    setError({});
    return true;
}
