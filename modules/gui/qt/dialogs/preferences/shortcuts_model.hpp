/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
/****************************************************************************
 * shortcuts_model.hpp : Staged VLC keyboard shortcuts for QML
 ****************************************************************************/
#ifndef VLC_QT_PREFERENCES_SHORTCUTS_MODEL_HPP
#define VLC_QT_PREFERENCES_SHORTCUTS_MODEL_HPP

#include <QAbstractListModel>
#include <QElapsedTimer>
#include <QPointer>
#include <QVariantMap>
#include <QVector>

class MainCtx;
Q_MOC_INCLUDE("maininterface/mainctx.hpp")

class ShortcutsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(MainCtx *ctx READ ctx WRITE setCtx NOTIFY ctxChanged)
    Q_PROPERTY(QString search READ search WRITE setSearch NOTIFY searchChanged)
    Q_PROPERTY(QString searchField READ searchField WRITE setSearchField NOTIFY searchFieldChanged)
    Q_PROPERTY(bool dirty READ dirty NOTIFY dirtyChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)

public:
    // Hotkey(s)Role: the keys of the action as one text ("f, Ctrl+e") and as
    // a list, for a chip per key
    enum Role { KeyRole = Qt::UserRole + 1, ActionRole, GroupRole,
                HotkeyRole, GlobalHotkeyRole, DirtyRole,
                HotkeysRole, GlobalHotkeysRole };
    Q_ENUM(Role)

    explicit ShortcutsModel(QObject *parent = nullptr);
    MainCtx *ctx() const;
    void setCtx(MainCtx *ctx);
    QString search() const { return m_search; }
    void setSearch(const QString &search);
    QString searchField() const { return m_searchField; }
    void setSearchField(const QString &field);
    bool dirty() const;
    QString error() const { return m_error; }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void reload();
    // reload(), unless the values were read a moment ago (the dialog made
    // just before it opens: read twice, every row told again)
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void resetAll();
    Q_INVOKABLE bool apply();
    // Adds the key pressed to the keys of the action (an action has several)
    Q_INVOKABLE QVariantMap stageKey(const QString &option, bool global,
                                    int qtKey, int modifiers, bool replaceConflict = false);
    // The same with a turn of the wheel (up or down) and the modifiers held
    // ("Ctrl+Mouse Wheel Up"): not for a global hotkey
    Q_INVOKABLE QVariantMap stageWheel(const QString &option, bool global,
                                      bool up, int modifiers, bool replaceConflict = false);
    // Every key of the action, or one of them ("Ctrl+e" as VLC writes it)
    Q_INVOKABLE bool clearKey(const QString &option, bool global);
    Q_INVOKABLE bool removeKey(const QString &option, bool global, const QString &key);

signals:
    void ctxChanged();
    void searchChanged();
    void searchFieldChanged();
    void dirtyChanged();
    void errorChanged();

private:
    struct Entry {
        QString key, globalKey, action, group;
        QString current, staged, defaults, globalCurrent, globalStaged, globalDefaults;
        // the place of the row: its group, then the first actions of the
        // design (worked out once, not at each comparison of the sort)
        int groupRank = 5, firstRank = 100;
        // a key of the interface's zoom: anywhere in the window, but over the
        // video the subtitles' size takes its keys first (no conflict with
        // them: Ctrl+wheel is both)
        bool zoom = false;
    };
    struct Change { QString key, action, before, after; bool global; };

    bool visible(const Entry &entry) const;
    void rebuildRows();
    // The names of the actions in the language of the interface
    void retranslate();
    // The rows again after a change of keys, while a search may hide them
    void refreshSearch();
    void notifyEntry(int entryIndex);
    void setError(const QString &error);
    int findEntry(const QString &key) const;
    // Two actions their keys may not share (their keys work at once)
    static bool canConflict(const Entry &a, const Entry &b);
    QVariantMap stageVlcKey(int target, bool global, int vlcKey, bool replaceConflict);
    static QString readKey(const QString &key);
    static void putKey(const QString &key, const QString &value);
    static QString groupFor(const QString &key);
    static int groupOrder(const QString &group);

    QPointer<MainCtx> m_ctx;
    QString m_search, m_searchField = QStringLiteral("all"), m_error;
    QElapsedTimer m_loaded;
    QVector<Entry> m_entries;
    QVector<int> m_rows;
};

#endif
