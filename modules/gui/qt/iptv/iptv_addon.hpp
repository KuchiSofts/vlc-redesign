/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
#pragma once

#include <QAbstractListModel>
#include <QElapsedTimer>
#include <QHash>
#include <QJSValue>
#include <QJsonObject>
#include <QPointer>
#include <QQuickAsyncImageProvider>
#include <QReadWriteLock>
#include <QSet>
#include <QStringList>
#include <QThreadPool>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

#include <functional>
#include <memory>

#include "qt.hpp"
#include "util/shared_input_item.hpp"

struct vlc_iptv;
class IptvAddon;

// The module loaded, shared with the image provider of the logos (its
// threads may outlive the interface's objects): lock taken by every call,
// for writing to load and unload
struct IptvBridge
{
    QReadWriteLock lock;
    vlc_iptv* object = nullptr;
    // blocking, from any thread; {} when the module is not loaded
    QJsonObject call(const QJsonObject& request);
};

// The channels of the TV page, the mini list over the video and the guide:
// every channel of the sources, filtered by a group, the favourites or the
// ones watched last, and a search. What is on now and next is asked of the
// add-on for the rows shown only, a few at a time.
class IptvChannelModel : public QAbstractListModel
{
    Q_OBJECT
    // "" every channel, "favorites", "recent", or a group's name
    Q_PROPERTY(QString group READ group WRITE setGroup NOTIFY filterChanged FINAL)
    Q_PROPERTY(QString search READ search WRITE setSearch NOTIFY filterChanged FINAL)
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)
    // the channel playing, as an index of the list (-1: none)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY currentIndexChanged FINAL)

public:
    enum Roles {
        KeyRole = Qt::UserRole + 1,
        NameRole,
        NumberRole,
        GroupRole,
        LogoRole,
        CatchupDaysRole,
        FavoriteRole,
        CurrentRole,
        HasGuideRole,
        NowTitleRole,
        NowStartRole,
        NowStopRole,
        NowDescriptionRole,
        NowArchivedRole,
        NextTitleRole,
        NextStartRole,
        SourceRole,
    };

    struct Item
    {
        QString key;
        QString source;
        QString name;
        QString nameFolded;
        int number = 0;
        QString group;
        QString logo;
        int catchupDays = 0;
        bool adult = false;
        bool radio = false;
        bool hasGuide = false;
    };

    explicit IptvChannelModel(IptvAddon* addon, bool followsAddonFilter = false);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString group() const { return m_group; }
    void setGroup(const QString& group);
    QString search() const { return m_search; }
    void setSearch(const QString& search);
    int count() const { return int(m_rows.size()); }
    int currentIndex() const;

    Q_INVOKABLE QString keyAt(int row) const;
    Q_INVOKABLE int indexOfKey(const QString& key) const;
    Q_INVOKABLE QVariantMap get(int row) const;
    // the keys in the order shown (zapping goes through them)
    QStringList keys() const;

    void refilter();
    void nowNextChanged(const QSet<QString>& keys);

signals:
    void filterChanged();
    void countChanged();
    void currentIndexChanged();

private:
    IptvAddon* m_addon;
    QString m_group;
    QString m_search;
    QVector<int> m_rows;   // indexes of IptvAddon::channels()
};

// Live TV: the add-on module (modules/misc/iptv) seen from the interface.
// Nothing is loaded while the module is missing or switched off (the
// iptv-enabled option): the TV entry, its settings and the keys of the
// player are not shown. The module answers in JSON on a worker thread
// (vlc_iptv.h); the answers come back here on the thread of the interface.
class IptvAddon : public QObject
{
    Q_OBJECT
    // the plugin file is there
    Q_PROPERTY(bool available READ available CONSTANT FINAL)
    // and switched on: the interface shows Live TV
    Q_PROPERTY(bool enabled READ enabled NOTIFY enabledChanged FINAL)
    Q_PROPERTY(QVariantList sources READ sources NOTIFY sourcesChanged FINAL)
    Q_PROPERTY(bool busy READ busy NOTIFY sourcesChanged FINAL)
    // the languages the guides give their programmes in
    Q_PROPERTY(QStringList guideLanguages READ guideLanguages NOTIFY sourcesChanged FINAL)
    Q_PROPERTY(QVariantList groups READ groups NOTIFY channelsChanged FINAL)
    Q_PROPERTY(int channelCount READ channelCount NOTIFY channelsChanged FINAL)
    Q_PROPERTY(bool channelsLoaded READ channelsLoaded NOTIFY channelsChanged FINAL)
    Q_PROPERTY(IptvChannelModel* channelModel READ channelModel CONSTANT FINAL)
    // the list over the video (its own group and search)
    Q_PROPERTY(IptvChannelModel* playerModel READ playerModel CONSTANT FINAL)
    // the channel playing, or the last one ({} before any)
    Q_PROPERTY(QVariantMap currentChannel READ currentChannel NOTIFY currentChannelChanged FINAL)
    // a channel of the TV is what plays (not another media)
    Q_PROPERTY(bool playing READ playing NOTIFY playingChanged FINAL)
    // a programme of the archive plays (catch-up), {} when live
    Q_PROPERTY(QVariantMap catchupProgramme READ catchupProgramme NOTIFY playingChanged FINAL)
    // seconds since the epoch, moved on every 30 s (the progress bars)
    Q_PROPERTY(qint64 now READ now NOTIFY nowChanged FINAL)
    Q_PROPERTY(int guideRevision READ guideRevision NOTIFY guideChanged FINAL)
    Q_PROPERTY(QStringList favorites READ favorites NOTIFY favoritesChanged FINAL)
    Q_PROPERTY(QVariantList reminders READ reminders NOTIFY remindersChanged FINAL)
    // options of Settings > TV
    Q_PROPERTY(int bannerMode READ bannerMode NOTIFY optionsChanged FINAL)
    Q_PROPERTY(bool showDescriptions READ showDescriptions NOTIFY optionsChanged FINAL)
    Q_PROPERTY(bool showProgress READ showProgress NOTIFY optionsChanged FINAL)
    Q_PROPERTY(bool showNext READ showNext NOTIFY optionsChanged FINAL)
    Q_PROPERTY(int guideHours READ guideHours NOTIFY optionsChanged FINAL)
    Q_PROPERTY(bool showAdult READ showAdult NOTIFY optionsChanged FINAL)

public:
    explicit IptvAddon(qt_intf_t* intf, QObject* parent = nullptr);
    ~IptvAddon() override;

    bool available() const { return m_available; }
    bool enabled() const { return m_enabled; }
    QVariantList sources() const { return m_sources; }
    bool busy() const { return m_busy; }
    QStringList guideLanguages() const { return m_guideLanguages; }
    QVariantList groups() const { return m_groups; }
    int channelCount() const { return int(m_channels.size()); }
    bool channelsLoaded() const { return m_channelsLoaded; }
    IptvChannelModel* channelModel() const { return m_channelModel; }
    IptvChannelModel* playerModel() const { return m_playerModel; }
    std::shared_ptr<IptvBridge> bridge() const { return m_bridge; }
    QVariantMap currentChannel() const;
    bool playing() const { return m_playing; }
    QVariantMap catchupProgramme() const { return m_playing ? m_catchup : QVariantMap(); }
    qint64 now() const { return m_now; }
    int guideRevision() const { return m_guideRevision; }
    QStringList favorites() const { return m_favorites; }
    QVariantList reminders() const { return m_reminders; }
    int bannerMode() const { return m_bannerMode; }
    bool showDescriptions() const { return m_showDescriptions; }
    bool showProgress() const { return m_showProgress; }
    bool showNext() const { return m_showNext; }
    int guideHours() const { return m_guideHours; }
    bool showAdult() const { return m_showAdult; }

    const QVector<IptvChannelModel::Item>& channels() const { return m_channels; }
    int channelIndex(const QString& key) const { return m_channelIndex.value(key, -1); }
    bool isFavorite(const QString& key) const { return m_favoriteSet.contains(key); }
    QStringList recent() const { return m_recent; }
    QString currentKey() const { return m_currentKey; }
    // what is on now and next on a channel ({} until asked and known)
    QVariantMap nowNextOf(const QString& key);

    // A request to the module, answered on this thread (callback) or
    // nothing when it is not loaded
    void call(const QJsonObject& request, std::function<void(const QJsonObject&)> done);

    // A change the module tells (on the interface's thread)
    void onEvent(const QJsonObject& event);

    // Settings > TV changed (the dialog keeps them)
    Q_INVOKABLE void reloadSettings();

    Q_INVOKABLE void addSource(const QVariantMap& source, const QJSValue& done = QJSValue());
    Q_INVOKABLE void updateSource(const QString& id, const QVariantMap& source, const QJSValue& done = QJSValue());
    Q_INVOKABLE void getSource(const QString& id, const QJSValue& done);
    Q_INVOKABLE void removeSource(const QString& id);
    Q_INVOKABLE void moveSource(const QString& id, int index);
    Q_INVOKABLE void setSourceEnabled(const QString& id, bool enabled);
    Q_INVOKABLE void refresh(const QString& id = QString());

    Q_INVOKABLE void catalog(const QJSValue& done);
    // the page of the project's discussions to suggest a source
    Q_INVOKABLE QString suggestionUrl() const;

    Q_INVOKABLE void play(const QString& key);
    Q_INVOKABLE void playCatchup(const QString& key, qint64 start, qint64 stop, const QString& title);
    // the programme on now from its start, when the channel has an archive
    Q_INVOKABLE void playFromStart();
    Q_INVOKABLE void backToLive();
    Q_INVOKABLE void channelUp();
    Q_INVOKABLE void channelDown();
    Q_INVOKABLE void previousChannel();
    Q_INVOKABLE bool playNumber(int number);
    Q_INVOKABLE void toggleFavorite(const QString& key);
    Q_INVOKABLE bool favorite(const QString& key) const { return isFavorite(key); }

    // the guide: the programmes of these channels between from and to
    Q_INVOKABLE void programmes(const QStringList& keys, qint64 from, qint64 to, const QJSValue& done);
    Q_INVOKABLE void description(const QString& key, qint64 start, const QJSValue& done);
    Q_INVOKABLE QVariantMap nowNext(const QString& key) { return nowNextOf(key); }

    // Xtream Codes: films and series of an account
    Q_INVOKABLE void library(const QString& id, const QString& kind, const QJSValue& done);
    Q_INVOKABLE void seriesInfo(const QString& id, const QString& series, const QJSValue& done);
    Q_INVOKABLE void playUrl(const QString& url, const QString& title);

    Q_INVOKABLE void addReminder(const QString& key, qint64 start, const QString& title);
    Q_INVOKABLE void removeReminder(const QString& key, qint64 start);
    Q_INVOKABLE bool hasReminder(const QString& key, qint64 start) const;

    // "1 h 20 min left", "in 5 min", times as the locale writes them
    Q_INVOKABLE QString timeText(qint64 secs) const;
    Q_INVOKABLE QString durationText(qint64 secs) const;
    Q_INVOKABLE QString dayText(qint64 secs) const;

signals:
    void enabledChanged();
    void sourcesChanged();
    void channelsChanged();
    void currentChannelChanged();
    void playingChanged();
    void nowChanged();
    void guideChanged();
    void favoritesChanged();
    void remindersChanged();
    void optionsChanged();
    // a channel was switched to (the banner shows)
    void channelSwitched();
    // a short message for the player (no archive, the source failed...)
    void message(const QString& text);
    void reminderDue(const QVariantMap& reminder);

private:
    void load();
    void unload();
    void sendOptions();
    void loadStatus();
    void loadChannels();
    void fetchNowNext();
    void startMedia(const QString& url, const QString& name, const QStringList& options);
    void checkPlaying();
    void checkReminders();
    void saveFavorites();
    void callJs(const QJSValue& callback, const QVariant& value);
    void zap(int step);

    qt_intf_t* m_intf;
    bool m_available = false;
    bool m_enabled = false;
    // the module
    std::shared_ptr<IptvBridge> m_bridge;
    void* m_module = nullptr;
    QThreadPool m_pool;

    QVariantList m_sources;
    bool m_busy = false;
    QStringList m_guideLanguages;
    QVector<IptvChannelModel::Item> m_channels;
    QHash<QString, int> m_channelIndex;
    QVariantList m_groups;
    bool m_channelsLoaded = false;
    int m_channelsRevision = -1;
    bool m_channelsLoading = false;
    bool m_channelsAgain = false;
    IptvChannelModel* m_channelModel = nullptr;
    IptvChannelModel* m_playerModel = nullptr;

    QHash<QString, QVariantMap> m_nowNext;
    QSet<QString> m_nowNextWanted;
    QTimer m_nowNextTimer;
    QTimer m_clock;
    qint64 m_now = 0;
    int m_guideRevision = 0;

    QString m_currentKey;
    QString m_previousKey;
    QString m_currentUrl;
    SharedInputItem m_currentMedia;
    QElapsedTimer m_switchTime;
    QVariantMap m_catchup;
    bool m_playing = false;
    bool m_startedLast = false;

    QStringList m_favorites;
    QSet<QString> m_favoriteSet;
    QStringList m_recent;
    QVariantList m_reminders;

    int m_bannerMode = 0;
    bool m_showDescriptions = true;
    bool m_showProgress = true;
    bool m_showNext = true;
    int m_guideHours = 3;
    bool m_showAdult = false;
    bool m_startOnLast = false;
};

// The logos of the channels, through the add-on's cache on disk:
// image://iptvlogo/<the logo's address, encoded>
class IptvLogoProvider : public QQuickAsyncImageProvider
{
public:
    explicit IptvLogoProvider(std::shared_ptr<IptvBridge> bridge) : m_bridge(std::move(bridge)) {}
    ~IptvLogoProvider() override { m_pool.waitForDone(); }
    QQuickImageResponse* requestImageResponse(const QString& id, const QSize& requestedSize) override;

private:
    std::shared_ptr<IptvBridge> m_bridge;
    QThreadPool m_pool;
};
