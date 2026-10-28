/*****************************************************************************
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

#include "iptv_addon.hpp"

#include <QDateTime>
#include <QImage>
#include <QImageReader>
#include <QJSEngine>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLocale>
#include <QQmlEngine>
#include <QQuickTextureFactory>
#include <QRegularExpression>
#include <QSettings>
#include <QUrl>
#include <QUrlQuery>

#include <vlc_common.h>
#include <vlc_configuration.h>
#include <vlc_iptv.h>
#include <vlc_modules.h>
#include <vlc_objects.h>

#include "maininterface/mainctx.hpp"
#include "playlist/media.hpp"
#include "playlist/playlist_controller.hpp"
#include "playlist/playlist_item.hpp"

#include <algorithm>

namespace {

// Settings > TV (the interface's settings, Settings writes them)
const char* const kRefresh = "redesign/iptv-refresh";
const char* const kGuideDays = "redesign/iptv-guide-days";
const char* const kGuideHours = "redesign/iptv-guide-hours";
const char* const kBanner = "redesign/iptv-banner";
const char* const kDescriptions = "redesign/iptv-descriptions";
const char* const kProgress = "redesign/iptv-progress";
const char* const kNext = "redesign/iptv-next";
const char* const kLanguages = "redesign/iptv-languages";
const char* const kStartLast = "redesign/iptv-start-last";
const char* const kAdult = "redesign/iptv-adult";
// what the interface keeps of the TV
const char* const kLastChannel = "iptv/last-channel";
const char* const kFavorites = "iptv/favorites";
const char* const kRecent = "iptv/recent";
const char* const kReminders = "iptv/reminders";

// "Never|Every 6 hours|Every 12 hours|Every day|Every week" of Settings
int refreshHoursOf(int choice)
{
    static const int hours[] = { 0, 6, 12, 24, 168 };
    return hours[qBound(0, choice, 4)];
}

const vlc_iptv_owner_cbs ownerCallbacks = {
    [](vlc_iptv_t*, const char* json, void* data) {
        auto* addon = static_cast<IptvAddon*>(data);
        const QJsonObject event = QJsonDocument::fromJson(QByteArray(json)).object();
        // to the interface's thread (the object goes away after the module)
        QMetaObject::invokeMethod(addon, [addon, event]() {
            addon->onEvent(event);
        }, Qt::QueuedConnection);
    }
};

}

QJsonObject IptvBridge::call(const QJsonObject& request)
{
    QReadLocker guard(&lock);
    if (!object || !object->request)
        return QJsonObject();
    char* answer = object->request(object, QJsonDocument(request).toJson(QJsonDocument::Compact).constData());
    if (!answer)
        return QJsonObject();
    const QJsonObject out = QJsonDocument::fromJson(QByteArray(answer)).object();
    free(answer);
    return out;
}

/* The channels */

IptvChannelModel::IptvChannelModel(IptvAddon* addon, bool)
    : QAbstractListModel(addon)
    , m_addon(addon)
{
}

int IptvChannelModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : int(m_rows.size());
}

QHash<int, QByteArray> IptvChannelModel::roleNames() const
{
    return {
        { KeyRole, "key" }, { NameRole, "name" }, { NumberRole, "number" }, { GroupRole, "group" },
        { LogoRole, "logo" }, { CatchupDaysRole, "catchupDays" }, { FavoriteRole, "favorite" },
        { CurrentRole, "current" }, { HasGuideRole, "hasGuide" }, { NowTitleRole, "nowTitle" },
        { NowStartRole, "nowStart" }, { NowStopRole, "nowStop" }, { NowDescriptionRole, "nowDescription" },
        { NowArchivedRole, "nowArchived" }, { NextTitleRole, "nextTitle" }, { NextStartRole, "nextStart" },
        { SourceRole, "source" },
    };
}

QVariant IptvChannelModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_rows.size())
        return QVariant();
    const IptvChannelModel::Item& item = m_addon->channels().at(m_rows[index.row()]);
    switch (role)
    {
    case KeyRole: return item.key;
    case NameRole: return item.name;
    case NumberRole: return item.number;
    case GroupRole: return item.group;
    case LogoRole:
        return item.logo.isEmpty() ? QString()
                                   : QStringLiteral("image://iptvlogo/")
                                         + QString::fromLatin1(QUrl::toPercentEncoding(item.logo));
    case CatchupDaysRole: return item.catchupDays;
    case FavoriteRole: return m_addon->isFavorite(item.key);
    case CurrentRole: return item.key == m_addon->currentKey();
    case HasGuideRole: return item.hasGuide;
    case SourceRole: return item.source;
    default:
        break;
    }
    // what is on: asked for the rows the view shows only
    if (!item.hasGuide)
        return role == NowStartRole || role == NowStopRole || role == NextStartRole ? QVariant(0)
                                                                                     : QVariant(QString());
    const QVariantMap nn = m_addon->nowNextOf(item.key);
    const QVariantMap now = nn.value(QStringLiteral("now")).toMap();
    const QVariantMap next = nn.value(QStringLiteral("next")).toMap();
    switch (role)
    {
    case NowTitleRole: return now.value(QStringLiteral("title")).toString();
    case NowStartRole: return now.value(QStringLiteral("start")).toLongLong();
    case NowStopRole: return now.value(QStringLiteral("stop")).toLongLong();
    case NowDescriptionRole: return now.value(QStringLiteral("description")).toString();
    case NowArchivedRole: return item.catchupDays > 0 && !now.isEmpty();
    case NextTitleRole: return next.value(QStringLiteral("title")).toString();
    case NextStartRole: return next.value(QStringLiteral("start")).toLongLong();
    default: return QVariant();
    }
}

void IptvChannelModel::setGroup(const QString& group)
{
    if (group == m_group)
        return;
    m_group = group;
    refilter();
    emit filterChanged();
}

void IptvChannelModel::setSearch(const QString& search)
{
    if (search == m_search)
        return;
    m_search = search;
    refilter();
    emit filterChanged();
}

void IptvChannelModel::refilter()
{
    beginResetModel();
    m_rows.clear();
    const auto& channels = m_addon->channels();
    const QString needle = m_search.trimmed().toCaseFolded();
    bool numeric = false;
    const int number = needle.toInt(&numeric);
    const bool adult = m_addon->showAdult();
    auto matches = [&](const IptvChannelModel::Item& c) {
        if (c.adult && !adult)
            return false;
        if (needle.isEmpty())
            return true;
        return c.nameFolded.contains(needle) || (numeric && c.number == number);
    };
    if (m_group == QLatin1String("recent") || m_group == QLatin1String("favorites"))
    {
        // in the order of the list kept (the last watched first)
        const QStringList keys = m_group == QLatin1String("recent") ? m_addon->recent() : m_addon->favorites();
        for (const QString& key : keys)
        {
            const int i = m_addon->channelIndex(key);
            if (i >= 0 && matches(channels[i]))
                m_rows.append(i);
        }
    }
    else
    {
        for (int i = 0; i < channels.size(); ++i)
            if ((m_group.isEmpty() || channels[i].group == m_group) && matches(channels[i]))
                m_rows.append(i);
    }
    endResetModel();
    emit countChanged();
    emit currentIndexChanged();
}

void IptvChannelModel::nowNextChanged(const QSet<QString>& keys)
{
    if (m_rows.isEmpty())
        return;
    if (keys.isEmpty())
    {
        emit dataChanged(index(0), index(int(m_rows.size()) - 1));
        return;
    }
    const auto& channels = m_addon->channels();
    for (int row = 0; row < m_rows.size(); ++row)
        if (keys.contains(channels[m_rows[row]].key))
            emit dataChanged(index(row), index(row));
}

int IptvChannelModel::currentIndex() const
{
    return indexOfKey(m_addon->currentKey());
}

QString IptvChannelModel::keyAt(int row) const
{
    if (row < 0 || row >= m_rows.size())
        return QString();
    return m_addon->channels().at(m_rows[row]).key;
}

int IptvChannelModel::indexOfKey(const QString& key) const
{
    const int i = m_addon->channelIndex(key);
    return i < 0 ? -1 : int(m_rows.indexOf(i));
}

QVariantMap IptvChannelModel::get(int row) const
{
    QVariantMap out;
    if (row < 0 || row >= m_rows.size())
        return out;
    const QModelIndex i = index(row);
    const auto names = roleNames();
    for (auto it = names.cbegin(); it != names.cend(); ++it)
        out.insert(QString::fromLatin1(it.value()), data(i, it.key()));
    return out;
}

QStringList IptvChannelModel::keys() const
{
    QStringList out;
    out.reserve(m_rows.size());
    for (int i : m_rows)
        out << m_addon->channels().at(i).key;
    return out;
}

/* The add-on */

IptvAddon::IptvAddon(qt_intf_t* intf, QObject* parent)
    : QObject(parent)
    , m_intf(intf)
    , m_bridge(std::make_shared<IptvBridge>())
{
    // the plugin file is there (looked up by name: nothing is loaded, no
    // message when it is missing)
    m_available = module_exists("iptv");
    m_pool.setMaxThreadCount(2);
    m_channelModel = new IptvChannelModel(this);
    m_playerModel = new IptvChannelModel(this);
    m_now = QDateTime::currentSecsSinceEpoch();

    QSettings* settings = m_intf->mainSettings;
    m_favorites = settings->value(kFavorites).toStringList();
    m_favoriteSet = QSet<QString>(m_favorites.cbegin(), m_favorites.cend());
    m_recent = settings->value(kRecent).toStringList();
    m_currentKey = settings->value(kLastChannel).toString();
    m_reminders = QJsonDocument::fromJson(settings->value(kReminders).toByteArray()).array().toVariantList();

    // what is on, for the rows shown: gathered for a moment, asked at once
    m_nowNextTimer.setSingleShot(true);
    m_nowNextTimer.setInterval(40);
    connect(&m_nowNextTimer, &QTimer::timeout, this, &IptvAddon::fetchNowNext);
    // the progress of the programmes, their ends, the reminders
    m_clock.setInterval(30000);
    connect(&m_clock, &QTimer::timeout, this, [this]() {
        m_now = QDateTime::currentSecsSinceEpoch();
        emit nowChanged();
        // the programmes over: asked again
        QSet<QString> ended;
        for (auto it = m_nowNext.begin(); it != m_nowNext.end();)
        {
            const qint64 stop = it->value(QStringLiteral("now")).toMap().value(QStringLiteral("stop")).toLongLong();
            const qint64 nextStart = it->value(QStringLiteral("next")).toMap().value(QStringLiteral("start")).toLongLong();
            if ((stop > 0 && stop <= m_now) || (stop == 0 && nextStart > 0 && nextStart <= m_now))
            {
                ended.insert(it.key());
                it = m_nowNext.erase(it);
            }
            else
                ++it;
        }
        if (!ended.isEmpty())
        {
            m_channelModel->nowNextChanged(ended);
            m_playerModel->nowNextChanged(ended);
        }
        checkReminders();
    });

    if (m_intf->p_mainPlaylistController)
        connect(m_intf->p_mainPlaylistController, &vlc::playlist::PlaylistController::currentItemChanged,
                this, &IptvAddon::checkPlaying);
    if (m_intf->p_mi)
        connect(m_intf->p_mi, &MainCtx::settingValueChanged, this, [this](const QString& key) {
            if (key.startsWith(QLatin1String("redesign/iptv-")))
                reloadSettings();
        });
    reloadSettings();
}

IptvAddon::~IptvAddon()
{
    unload();
}

void IptvAddon::reloadSettings()
{
    QSettings* s = m_intf->mainSettings;
    const int banner = s->value(kBanner, 0).toInt();
    const bool descriptions = s->value(kDescriptions, true).toBool();
    const bool progress = s->value(kProgress, true).toBool();
    const bool next = s->value(kNext, true).toBool();
    const int hours = qBound(1, s->value(kGuideHours, 3).toInt(), 12);
    const bool adult = s->value(kAdult, false).toBool();
    m_startOnLast = s->value(kStartLast, false).toBool();
    const bool optionsDiffer = banner != m_bannerMode || descriptions != m_showDescriptions
                               || progress != m_showProgress || next != m_showNext
                               || hours != m_guideHours || adult != m_showAdult;
    const bool refilter = adult != m_showAdult;
    const bool refetch = descriptions != m_showDescriptions;
    m_bannerMode = banner;
    m_showDescriptions = descriptions;
    m_showProgress = progress;
    m_showNext = next;
    m_guideHours = hours;
    m_showAdult = adult;
    if (optionsDiffer)
        emit optionsChanged();
    if (refilter)
    {
        m_channelModel->refilter();
        m_playerModel->refilter();
    }
    if (refetch)
    {
        m_nowNext.clear();
        m_channelModel->nowNextChanged({});
        m_playerModel->nowNextChanged({});
    }

    const bool enabled = m_available && var_InheritBool(m_intf, "iptv-enabled");
    if (enabled != m_enabled)
    {
        m_enabled = enabled;
        if (enabled)
            load();
        else
            unload();
        emit enabledChanged();
    }
    else if (enabled)
        sendOptions();
}

void IptvAddon::load()
{
    {
        QWriteLocker guard(&m_bridge->lock);
        if (m_bridge->object)
            return;
        auto* obj = static_cast<vlc_iptv_t*>(vlc_object_create(m_intf, sizeof(vlc_iptv_t)));
        if (!obj)
            return;
        obj->owner_cbs = &ownerCallbacks;
        obj->owner_data = this;
        obj->request = nullptr;
        module_t* module = module_need(obj, "iptv", "iptv", true);
        if (!module)
        {
            vlc_object_delete(obj);
            return;
        }
        m_bridge->object = obj;
        m_module = module;
    }
    sendOptions();
    call(QJsonObject { { QStringLiteral("op"), QStringLiteral("start") } }, nullptr);
    loadStatus();
    loadChannels();
    m_clock.start();
}

void IptvAddon::unload()
{
    m_clock.stop();
    // the requests on their way end first
    m_pool.waitForDone();
    QWriteLocker guard(&m_bridge->lock);
    if (!m_bridge->object)
        return;
    module_unneed(m_bridge->object, static_cast<module_t*>(m_module));
    vlc_object_delete(m_bridge->object);
    m_bridge->object = nullptr;
    m_module = nullptr;
}

void IptvAddon::sendOptions()
{
    QSettings* s = m_intf->mainSettings;
    QJsonObject r;
    r.insert(QLatin1String("op"), QStringLiteral("setOptions"));
    r.insert(QLatin1String("guideDays"), qBound(1, s->value(kGuideDays, 7).toInt(), 14));
    r.insert(QLatin1String("refreshHours"), refreshHoursOf(s->value(kRefresh, 3).toInt()));
    const QStringList languages = s->value(kLanguages).toString().split(QRegularExpression(QStringLiteral("[,; ]+")),
                                                                         Qt::SkipEmptyParts);
    r.insert(QLatin1String("languages"), QJsonArray::fromStringList(languages));
    call(r, nullptr);
}

void IptvAddon::call(const QJsonObject& request, std::function<void(const QJsonObject&)> done)
{
    if (!m_enabled)
        return;
    QPointer<IptvAddon> self(this);
    std::shared_ptr<IptvBridge> bridge = m_bridge;
    m_pool.start([self, bridge, request, done = std::move(done)]() {
        const QJsonObject answer = bridge->call(request);
        if (!done || !self)
            return;
        QMetaObject::invokeMethod(self.data(), [self, done, answer]() {
            if (self)
                done(answer);
        }, Qt::QueuedConnection);
    });
}

void IptvAddon::onEvent(const QJsonObject& event)
{
    const QString kind = event.value(QLatin1String("event")).toString();
    if (kind == QLatin1String("progress"))
    {
        // the status of the source (its stage and how much came)
        const QString id = event.value(QLatin1String("id")).toString();
        for (QVariant& v : m_sources)
        {
            QVariantMap s = v.toMap();
            if (s.value(QStringLiteral("id")).toString() != id)
                continue;
            s.insert(QStringLiteral("stage"), event.value(QLatin1String("stage")).toString());
            s.insert(QStringLiteral("received"), event.value(QLatin1String("received")).toDouble());
            s.insert(QStringLiteral("total"), event.value(QLatin1String("total")).toDouble());
            s.insert(QStringLiteral("busy"), !event.value(QLatin1String("stage")).toString().isEmpty());
            v = s;
        }
        m_busy = std::any_of(m_sources.cbegin(), m_sources.cend(),
                             [](const QVariant& v) { return v.toMap().value(QStringLiteral("busy")).toBool(); });
        emit sourcesChanged();
        return;
    }
    if (kind != QLatin1String("changed"))
        return;
    const QString what = event.value(QLatin1String("what")).toString();
    loadStatus();
    if (what == QLatin1String("channels") || what == QLatin1String("guide"))
        loadChannels();
    if (what == QLatin1String("guide"))
    {
        m_nowNext.clear();
        ++m_guideRevision;
        emit guideChanged();
        m_channelModel->nowNextChanged({});
        m_playerModel->nowNextChanged({});
    }
}

void IptvAddon::loadStatus()
{
    call(QJsonObject { { QStringLiteral("op"), QStringLiteral("status") } }, [this](const QJsonObject& answer) {
        m_sources = answer.value(QLatin1String("sources")).toArray().toVariantList();
        m_busy = answer.value(QLatin1String("busy")).toBool();
        m_guideLanguages.clear();
        for (const QJsonValue& v : answer.value(QLatin1String("languages")).toArray())
            m_guideLanguages << v.toString();
        emit sourcesChanged();
    });
}

void IptvAddon::loadChannels()
{
    // one reading at a time; one more after it when asked meanwhile
    if (m_channelsLoading)
    {
        m_channelsAgain = true;
        return;
    }
    if (!m_enabled)
        return;
    m_channelsLoading = true;
    QPointer<IptvAddon> self(this);
    std::shared_ptr<IptvBridge> bridge = m_bridge;
    // read and sorted out on the worker: 20 000 channels take a moment
    m_pool.start([self, bridge]() {
        const QJsonObject answer = bridge->call(QJsonObject { { QStringLiteral("op"), QStringLiteral("channels") } });
        auto items = std::make_shared<QVector<IptvChannelModel::Item>>();
        const QJsonArray list = answer.value(QLatin1String("channels")).toArray();
        items->reserve(list.size());
        for (const QJsonValue& v : list)
        {
            const QJsonObject o = v.toObject();
            IptvChannelModel::Item item;
            item.key = o.value(QLatin1String("key")).toString();
            item.source = o.value(QLatin1String("source")).toString();
            item.name = o.value(QLatin1String("name")).toString();
            item.nameFolded = item.name.toCaseFolded();
            item.number = o.value(QLatin1String("number")).toInt();
            item.group = o.value(QLatin1String("group")).toString();
            item.logo = o.value(QLatin1String("logo")).toString();
            item.catchupDays = o.value(QLatin1String("catchupDays")).toInt();
            item.radio = o.value(QLatin1String("radio")).toBool();
            item.adult = o.value(QLatin1String("adult")).toBool();
            item.hasGuide = o.value(QLatin1String("guide")).toBool();
            items->append(std::move(item));
        }
        const int revision = answer.value(QLatin1String("revision")).toInt(-1);
        const bool ok = answer.contains(QLatin1String("channels"));
        if (!self)
            return;
        QMetaObject::invokeMethod(self.data(), [self, items, revision, ok]() {
            if (!self)
                return;
            IptvAddon* a = self.data();
            a->m_channelsLoading = false;
            if (ok && revision != a->m_channelsRevision)
            {
                a->m_channelsRevision = revision;
                a->m_channels = std::move(*items);
                a->m_channelIndex.clear();
                QVariantList groups;
                QHash<QString, int> counts;
                QStringList order;
                for (int i = 0; i < a->m_channels.size(); ++i)
                {
                    const auto& c = a->m_channels[i];
                    a->m_channelIndex.insert(c.key, i);
                    if (c.adult && !a->m_showAdult)
                        continue;
                    if (!c.group.isEmpty() && !counts.contains(c.group))
                        order << c.group;
                    ++counts[c.group];
                }
                for (const QString& g : std::as_const(order))
                    groups.append(QVariantMap { { QStringLiteral("name"), g }, { QStringLiteral("count"), counts.value(g) } });
                a->m_groups = groups;
                a->m_channelsLoaded = true;
                a->m_channelModel->refilter();
                a->m_playerModel->refilter();
                emit a->channelsChanged();
                emit a->currentChannelChanged();
                // the last channel when VLC starts (Settings > TV)
                if (a->m_startOnLast && !a->m_startedLast && !a->m_currentKey.isEmpty()
                    && a->m_channelIndex.contains(a->m_currentKey) && a->m_intf->p_mainPlaylistController
                    && a->m_intf->p_mainPlaylistController->isEmpty())
                {
                    a->m_startedLast = true;
                    a->play(a->m_currentKey);
                }
            }
            if (a->m_channelsAgain)
            {
                a->m_channelsAgain = false;
                a->loadChannels();
            }
        }, Qt::QueuedConnection);
    });
}

QVariantMap IptvAddon::nowNextOf(const QString& key)
{
    auto it = m_nowNext.constFind(key);
    if (it != m_nowNext.cend())
        return *it;
    if (m_enabled && !m_nowNextWanted.contains(key))
    {
        m_nowNextWanted.insert(key);
        if (!m_nowNextTimer.isActive())
            m_nowNextTimer.start();
    }
    return QVariantMap();
}

void IptvAddon::fetchNowNext()
{
    if (m_nowNextWanted.isEmpty())
        return;
    const QStringList keys(m_nowNextWanted.cbegin(), m_nowNextWanted.cend());
    m_nowNextWanted.clear();
    QJsonObject r;
    r.insert(QLatin1String("op"), QStringLiteral("nowNext"));
    r.insert(QLatin1String("keys"), QJsonArray::fromStringList(keys));
    r.insert(QLatin1String("descriptions"), m_showDescriptions);
    call(r, [this, keys](const QJsonObject& answer) {
        const QJsonObject channels = answer.value(QLatin1String("channels")).toObject();
        QSet<QString> changed;
        for (const QString& key : keys)
        {
            // known now, even when nothing is on (no question again)
            m_nowNext.insert(key, channels.value(key).toObject().toVariantMap());
            changed.insert(key);
        }
        m_channelModel->nowNextChanged(changed);
        m_playerModel->nowNextChanged(changed);
        if (changed.contains(m_currentKey))
            emit currentChannelChanged();
    });
}

QVariantMap IptvAddon::currentChannel() const
{
    const int i = m_channelIndex.value(m_currentKey, -1);
    if (i < 0)
        return QVariantMap();
    const auto& c = m_channels[i];
    QVariantMap out;
    out.insert(QStringLiteral("key"), c.key);
    out.insert(QStringLiteral("name"), c.name);
    out.insert(QStringLiteral("number"), c.number);
    out.insert(QStringLiteral("group"), c.group);
    out.insert(QStringLiteral("logo"), c.logo.isEmpty() ? QString()
                                                         : QStringLiteral("image://iptvlogo/")
                                                               + QString::fromLatin1(QUrl::toPercentEncoding(c.logo)));
    out.insert(QStringLiteral("catchupDays"), c.catchupDays);
    out.insert(QStringLiteral("favorite"), isFavorite(c.key));
    out.insert(QStringLiteral("hasGuide"), c.hasGuide);
    const QVariantMap nn = const_cast<IptvAddon*>(this)->nowNextOf(c.key);
    out.insert(QStringLiteral("now"), nn.value(QStringLiteral("now")));
    out.insert(QStringLiteral("next"), nn.value(QStringLiteral("next")));
    return out;
}

void IptvAddon::callJs(const QJSValue& callback, const QVariant& value)
{
    if (!callback.isCallable())
        return;
    QJSEngine* engine = qjsEngine(this);
    QJSValue f = callback;
    f.call({ engine ? engine->toScriptValue(value) : QJSValue() });
}

/* Sources */

void IptvAddon::addSource(const QVariantMap& source, const QJSValue& done)
{
    QJsonObject r;
    r.insert(QLatin1String("op"), QStringLiteral("addSource"));
    r.insert(QLatin1String("source"), QJsonObject::fromVariantMap(source));
    call(r, [this, done](const QJsonObject& answer) {
        callJs(done, answer.toVariantMap());
        loadStatus();
    });
}

void IptvAddon::updateSource(const QString& id, const QVariantMap& source, const QJSValue& done)
{
    QJsonObject r;
    r.insert(QLatin1String("op"), QStringLiteral("updateSource"));
    r.insert(QLatin1String("id"), id);
    r.insert(QLatin1String("source"), QJsonObject::fromVariantMap(source));
    call(r, [this, done](const QJsonObject& answer) {
        callJs(done, answer.toVariantMap());
        loadStatus();
    });
}

void IptvAddon::getSource(const QString& id, const QJSValue& done)
{
    call(QJsonObject { { QStringLiteral("op"), QStringLiteral("getSource") }, { QStringLiteral("id"), id } },
         [this, done](const QJsonObject& answer) {
             callJs(done, answer.value(QLatin1String("source")).toObject().toVariantMap());
         });
}

void IptvAddon::removeSource(const QString& id)
{
    call(QJsonObject { { QStringLiteral("op"), QStringLiteral("removeSource") }, { QStringLiteral("id"), id } },
         [this](const QJsonObject&) { loadStatus(); });
}

void IptvAddon::moveSource(const QString& id, int index)
{
    call(QJsonObject { { QStringLiteral("op"), QStringLiteral("moveSource") }, { QStringLiteral("id"), id },
                       { QStringLiteral("index"), index } },
         [this](const QJsonObject&) { loadStatus(); });
}

void IptvAddon::setSourceEnabled(const QString& id, bool enabled)
{
    call(QJsonObject { { QStringLiteral("op"), QStringLiteral("enableSource") }, { QStringLiteral("id"), id },
                       { QStringLiteral("enabled"), enabled } },
         [this](const QJsonObject&) { loadStatus(); });
}

void IptvAddon::refresh(const QString& id)
{
    call(QJsonObject { { QStringLiteral("op"), QStringLiteral("refresh") }, { QStringLiteral("id"), id } },
         [this](const QJsonObject&) { loadStatus(); });
}

void IptvAddon::catalog(const QJSValue& done)
{
    call(QJsonObject { { QStringLiteral("op"), QStringLiteral("catalog") } }, [this, done](const QJsonObject& answer) {
        callJs(done, answer.value(QLatin1String("catalog")).toObject().toVariantMap());
    });
}

QString IptvAddon::suggestionUrl() const
{
    // as "Suggest an idea" (RedesignFeedback): a new discussion of the
    // project, its fields filled in; nothing is sent by VLC itself
    QUrl url(QStringLiteral("https://github.com/KuchiSofts/vlc-redesign/discussions/new"));
    const QString body = QStringLiteral(
        "**Name:** \n\n**Website:** \n\n**M3U playlist link:** \n\n**EPG (XMLTV) link:** \n\n"
        "**Languages / countries:** \n\n**Catch-up (how many days back):** \n\n**Films / series (VOD):** \n\n"
        "**Free or paid:** \n\n**Why it belongs in the catalog (and that it is legal):** \n");
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("category"), QStringLiteral("ideas"));
    query.addQueryItem(QStringLiteral("title"), QStringLiteral("IPTV source: "));
    query.addQueryItem(QStringLiteral("body"), body);
    url.setQuery(query);
    return url.toString(QUrl::FullyEncoded);
}

/* Watching */

void IptvAddon::startMedia(const QString& url, const QString& name, const QStringList& options)
{
    if (!m_intf->p_mainPlaylistController)
        return;
    // the channel alone in the queue, as a TV switches channels; not kept
    // in the history nor in the library (no entry for every channel zapped)
    QStringList all = options;
    all << QStringLiteral(":no-save-recentplay");
    vlc::playlist::Media media(url, name, all);
    m_currentUrl = url;
    // the item itself tells the channel playing from another media
    m_currentMedia.reset(media.raw());
    m_switchTime.start();
    if (!m_playing)
    {
        m_playing = true;
        emit playingChanged();
    }
    m_intf->p_mainPlaylistController->clear();
    m_intf->p_mainPlaylistController->append(QVector<vlc::playlist::Media> { media }, true);
}

void IptvAddon::play(const QString& key)
{
    if (key.isEmpty())
        return;
    call(QJsonObject { { QStringLiteral("op"), QStringLiteral("play") }, { QStringLiteral("key"), key } },
         [this, key](const QJsonObject& answer) {
             const QString url = answer.value(QLatin1String("url")).toString();
             if (url.isEmpty())
             {
                 emit message(qtr("This channel cannot be played"));
                 return;
             }
             QStringList options;
             for (const QJsonValue& o : answer.value(QLatin1String("options")).toArray())
                 options << o.toString();
             if (key != m_currentKey)
                 m_previousKey = m_currentKey;
             m_currentKey = key;
             m_catchup.clear();
             m_intf->mainSettings->setValue(kLastChannel, key);
             m_recent.removeAll(key);
             m_recent.prepend(key);
             while (m_recent.size() > 30)
                 m_recent.removeLast();
             m_intf->mainSettings->setValue(kRecent, m_recent);
             if (m_channelModel->group() == QLatin1String("recent"))
                 m_channelModel->refilter();
             startMedia(url, answer.value(QLatin1String("name")).toString(), options);
             emit currentChannelChanged();
             emit m_channelModel->currentIndexChanged();
             emit m_playerModel->currentIndexChanged();
             m_channelModel->nowNextChanged({});
             m_playerModel->nowNextChanged({});
             emit channelSwitched();
         });
}

void IptvAddon::playCatchup(const QString& key, qint64 start, qint64 stop, const QString& title)
{
    QJsonObject r;
    r.insert(QLatin1String("op"), QStringLiteral("catchup"));
    r.insert(QLatin1String("key"), key);
    r.insert(QLatin1String("start"), double(start));
    r.insert(QLatin1String("stop"), double(stop));
    call(r, [this, key, start, stop, title](const QJsonObject& answer) {
        const QString url = answer.value(QLatin1String("url")).toString();
        if (url.isEmpty())
        {
            const QString error = answer.value(QLatin1String("error")).toString();
            emit message(error == QLatin1String("too old") ? qtr("This programme is older than the channel's archive")
                         : error == QLatin1String("not yet") ? qtr("This programme has not started yet")
                                                             : qtr("This channel has no archive: past programmes cannot be played"));
            return;
        }
        QStringList options;
        for (const QJsonValue& o : answer.value(QLatin1String("options")).toArray())
            options << o.toString();
        if (key != m_currentKey)
            m_previousKey = m_currentKey;
        m_currentKey = key;
        m_catchup = QVariantMap { { QStringLiteral("key"), key }, { QStringLiteral("start"), start },
                                  { QStringLiteral("stop"), stop }, { QStringLiteral("title"), title } };
        startMedia(url, title.isEmpty() ? answer.value(QLatin1String("name")).toString()
                                        : answer.value(QLatin1String("name")).toString() + QStringLiteral(" – ") + title,
                   options);
        emit currentChannelChanged();
        emit channelSwitched();
    });
}

void IptvAddon::playFromStart()
{
    const QVariantMap now = nowNextOf(m_currentKey).value(QStringLiteral("now")).toMap();
    if (now.isEmpty())
    {
        emit message(qtr("The guide does not say what is on"));
        return;
    }
    playCatchup(m_currentKey, now.value(QStringLiteral("start")).toLongLong(),
                now.value(QStringLiteral("stop")).toLongLong(), now.value(QStringLiteral("title")).toString());
}

void IptvAddon::backToLive()
{
    play(m_currentKey);
}

void IptvAddon::zap(int step)
{
    // through the list the channel was chosen in (a group, the favourites),
    // all the channels when it is not in it
    QStringList keys = m_playerModel->keys();
    if (!keys.contains(m_currentKey))
        keys = m_channelModel->keys();
    if (!keys.contains(m_currentKey))
    {
        keys.clear();
        for (const auto& c : std::as_const(m_channels))
            if (!c.adult || m_showAdult)
                keys << c.key;
    }
    if (keys.isEmpty())
        return;
    const int i = int(keys.indexOf(m_currentKey));
    const int next = i < 0 ? 0 : int((i + step + keys.size()) % keys.size());
    play(keys[next]);
}

void IptvAddon::channelUp() { zap(1); }
void IptvAddon::channelDown() { zap(-1); }

void IptvAddon::previousChannel()
{
    if (!m_previousKey.isEmpty() && m_channelIndex.contains(m_previousKey))
        play(m_previousKey);
}

bool IptvAddon::playNumber(int number)
{
    for (const auto& c : std::as_const(m_channels))
        if (c.number == number && (!c.adult || m_showAdult))
        {
            play(c.key);
            return true;
        }
    emit message(qtr("No channel %1").arg(number));
    return false;
}

void IptvAddon::playUrl(const QString& url, const QString& title)
{
    m_catchup.clear();
    startMedia(url, title, {});
    // a film or an episode: no channel, no keys of the TV
    m_currentMedia.reset();
    if (m_playing)
    {
        m_playing = false;
        emit playingChanged();
    }
}

void IptvAddon::checkPlaying()
{
    const PlaylistItem item = m_intf->p_mainPlaylistController->getCurrentItem();
    // (none for a moment while a channel is switched to: nothing changes;
    // none later, the queue was emptied or ended)
    if (!item && m_switchTime.isValid() && m_switchTime.elapsed() < 3000)
        return;
    const bool playing = item && m_currentMedia && item.inputItem() == m_currentMedia.get();
    if (playing != m_playing)
    {
        m_playing = playing;
        emit playingChanged();
    }
}

void IptvAddon::toggleFavorite(const QString& key)
{
    if (m_favoriteSet.contains(key))
    {
        m_favoriteSet.remove(key);
        m_favorites.removeAll(key);
    }
    else
    {
        m_favoriteSet.insert(key);
        m_favorites.append(key);
    }
    saveFavorites();
}

void IptvAddon::saveFavorites()
{
    m_intf->mainSettings->setValue(kFavorites, m_favorites);
    emit favoritesChanged();
    if (m_channelModel->group() == QLatin1String("favorites"))
        m_channelModel->refilter();
    else
        m_channelModel->nowNextChanged({});
    if (m_playerModel->group() == QLatin1String("favorites"))
        m_playerModel->refilter();
    emit currentChannelChanged();
}

/* The guide */

void IptvAddon::programmes(const QStringList& keys, qint64 from, qint64 to, const QJSValue& done)
{
    QJsonObject r;
    r.insert(QLatin1String("op"), QStringLiteral("programmes"));
    r.insert(QLatin1String("keys"), QJsonArray::fromStringList(keys));
    r.insert(QLatin1String("from"), double(from));
    r.insert(QLatin1String("to"), double(to));
    call(r, [this, done](const QJsonObject& answer) {
        callJs(done, answer.value(QLatin1String("programmes")).toObject().toVariantMap());
    });
}

void IptvAddon::description(const QString& key, qint64 start, const QJSValue& done)
{
    call(QJsonObject { { QStringLiteral("op"), QStringLiteral("description") }, { QStringLiteral("key"), key },
                       { QStringLiteral("start"), double(start) } },
         [this, done](const QJsonObject& answer) {
             callJs(done, answer.value(QLatin1String("description")).toString());
         });
}

void IptvAddon::library(const QString& id, const QString& kind, const QJSValue& done)
{
    call(QJsonObject { { QStringLiteral("op"), QStringLiteral("vod") }, { QStringLiteral("id"), id },
                       { QStringLiteral("kind"), kind } },
         [this, done](const QJsonObject& answer) {
             callJs(done, answer.value(QLatin1String("items")).toArray().toVariantList());
         });
}

void IptvAddon::seriesInfo(const QString& id, const QString& series, const QJSValue& done)
{
    call(QJsonObject { { QStringLiteral("op"), QStringLiteral("seriesInfo") }, { QStringLiteral("id"), id },
                       { QStringLiteral("series"), series } },
         [this, done](const QJsonObject& answer) { callJs(done, answer.toVariantMap()); });
}

/* Reminders */

void IptvAddon::addReminder(const QString& key, qint64 start, const QString& title)
{
    if (hasReminder(key, start))
        return;
    const int i = m_channelIndex.value(key, -1);
    m_reminders.append(QVariantMap { { QStringLiteral("key"), key }, { QStringLiteral("start"), start },
                                     { QStringLiteral("title"), title },
                                     { QStringLiteral("channel"), i >= 0 ? m_channels[i].name : QString() } });
    m_intf->mainSettings->setValue(kReminders, QJsonDocument(QJsonArray::fromVariantList(m_reminders)).toJson(QJsonDocument::Compact));
    emit remindersChanged();
}

void IptvAddon::removeReminder(const QString& key, qint64 start)
{
    for (int i = 0; i < m_reminders.size(); ++i)
    {
        const QVariantMap r = m_reminders[i].toMap();
        if (r.value(QStringLiteral("key")).toString() == key && r.value(QStringLiteral("start")).toLongLong() == start)
        {
            m_reminders.removeAt(i);
            break;
        }
    }
    m_intf->mainSettings->setValue(kReminders, QJsonDocument(QJsonArray::fromVariantList(m_reminders)).toJson(QJsonDocument::Compact));
    emit remindersChanged();
}

bool IptvAddon::hasReminder(const QString& key, qint64 start) const
{
    return std::any_of(m_reminders.cbegin(), m_reminders.cend(), [&](const QVariant& v) {
        const QVariantMap r = v.toMap();
        return r.value(QStringLiteral("key")).toString() == key && r.value(QStringLiteral("start")).toLongLong() == start;
    });
}

void IptvAddon::checkReminders()
{
    // a minute before the start; the ones missed (VLC was closed) go quietly
    QVariantList due;
    for (int i = int(m_reminders.size()) - 1; i >= 0; --i)
    {
        const QVariantMap r = m_reminders[i].toMap();
        const qint64 start = r.value(QStringLiteral("start")).toLongLong();
        if (start - 60 <= m_now)
        {
            if (m_now - start < 600)
                due.prepend(r);
            m_reminders.removeAt(i);
        }
    }
    if (due.isEmpty() && m_reminders.size() == m_intf->mainSettings->value(kReminders).toByteArray().count('{'))
        return;
    m_intf->mainSettings->setValue(kReminders, QJsonDocument(QJsonArray::fromVariantList(m_reminders)).toJson(QJsonDocument::Compact));
    emit remindersChanged();
    for (const QVariant& r : std::as_const(due))
        emit reminderDue(r.toMap());
}

/* Texts */

QString IptvAddon::timeText(qint64 secs) const
{
    return QLocale().toString(QDateTime::fromSecsSinceEpoch(secs).time(), QLocale::ShortFormat);
}

QString IptvAddon::durationText(qint64 secs) const
{
    const qint64 minutes = qMax<qint64>(0, (secs + 59) / 60);
    if (minutes < 60)
        return qtr("%1 min", "%1 min", int(minutes)).arg(minutes);
    const qint64 hours = minutes / 60;
    const qint64 rest = minutes % 60;
    return rest == 0 ? qtr("%1 h", "%1 h", int(hours)).arg(hours) : qtr("%1 h %2 min").arg(hours).arg(rest);
}

QString IptvAddon::dayText(qint64 secs) const
{
    const QDate day = QDateTime::fromSecsSinceEpoch(secs).date();
    const QDate today = QDate::currentDate();
    if (day == today)
        return qtr("Today");
    if (day == today.addDays(1))
        return qtr("Tomorrow");
    if (day == today.addDays(-1))
        return qtr("Yesterday");
    return QLocale().toString(day, QStringLiteral("ddd d MMM"));
}

/* Logos */

namespace {

class LogoResponse : public QQuickImageResponse
{
public:
    void finish(const QImage& image, const QString& error)
    {
        m_image = image;
        m_error = error;
        emit finished();
    }
    QQuickTextureFactory* textureFactory() const override
    {
        return m_image.isNull() ? nullptr : QQuickTextureFactory::textureFactoryForImage(m_image);
    }
    QString errorString() const override { return m_error; }

private:
    QImage m_image;
    QString m_error;
};

}

QQuickImageResponse* IptvLogoProvider::requestImageResponse(const QString& id, const QSize& requestedSize)
{
    auto* response = new LogoResponse;
    const QString url = QUrl::fromPercentEncoding(id.toLatin1());
    std::shared_ptr<IptvBridge> bridge = m_bridge;
    m_pool.start([response, url, requestedSize, bridge]() {
        const QJsonObject answer = bridge->call(QJsonObject { { QStringLiteral("op"), QStringLiteral("logo") },
                                                              { QStringLiteral("url"), url } });
        const QString path = answer.value(QLatin1String("path")).toString();
        QImage image;
        if (!path.isEmpty())
        {
            QImageReader reader(path);
            reader.setAutoTransform(true);
            const QSize size = reader.size();
            // the size shown, the logo's proportions kept
            if (requestedSize.isValid() && size.isValid())
                reader.setScaledSize(size.scaled(requestedSize, Qt::KeepAspectRatio));
            image = reader.read();
        }
        response->finish(image, image.isNull() ? QStringLiteral("no logo") : QString());
    });
    return response;
}
