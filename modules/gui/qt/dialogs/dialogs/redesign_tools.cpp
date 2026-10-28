/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
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

#include "redesign_tools.hpp"

#include "qt.hpp"
#include "maininterface/mainctx.hpp"
#include "player/player_controller.hpp"
#include "dialogs/extensions/extensions_manager.hpp"

#include <vlc_common.h>
#include <vlc_configuration.h>
#include <vlc_extensions.h>
#include <vlc_input_item.h>
#include <vlc_interface.h>
#include <vlc_modules.h>

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QLocale>
#include <QSet>
#include <QThreadPool>

#include <algorithm>
#include <cmath>

/*****************************************************************************
 * MediaStatistics
 *****************************************************************************/

namespace {
// two minutes of the bitrate (one value a second)
constexpr int bitrateHistory = 120;
}

MediaStatistics::MediaStatistics(QObject *parent)
    : QObject(parent)
{
    m_collecting = config_GetInt("stats") != 0;
}

void MediaStatistics::setCtx(MainCtx *ctx)
{
    if (ctx == m_ctx)
        return;
    QObject::disconnect(m_connection);
    m_ctx = ctx;
    if (ctx && ctx->getIntf() && ctx->getIntf()->p_mainPlayerController)
    {
        PlayerController *player = ctx->getIntf()->p_mainPlayerController;
        m_connection = connect(player, &PlayerController::statisticsUpdated, this, &MediaStatistics::update);
        // another file: its own statistics
        connect(player, &PlayerController::inputChanged, this, [this](bool) { clear(); });
    }
    emit ctxChanged();
}

QVariantList MediaStatistics::bitrates() const
{
    QVariantList list;
    list.reserve(m_bitrates.size());
    for (const double value : m_bitrates)
        list.append(value);
    return list;
}

double MediaStatistics::bitrateMax() const
{
    double max = 0;
    for (const double value : m_bitrates)
        max = std::max(max, value);
    return max;
}

void MediaStatistics::startCollecting()
{
    if (m_collecting)
        return;
    config_PutInt("stats", 1);
    m_collecting = true;
    emit collectingChanged();
}

void MediaStatistics::clear()
{
    m_values.clear();
    m_bitrates.clear();
    emit valuesChanged();
}

void MediaStatistics::update(const input_stats_t &stats)
{
    const auto number = [](uint64_t value) { return static_cast<double>(value); };
    // as the Statistics tab: kB read, kb/s (the bitrates are in bytes/µs
    // * 1000)
    m_values[QStringLiteral("read")] = number(stats.i_read_bytes / 1024);
    m_values[QStringLiteral("inputBitrate")] = std::round(stats.f_input_bitrate * 8000);
    m_values[QStringLiteral("demuxed")] = number(stats.i_demux_read_bytes / 1024);
    m_values[QStringLiteral("streamBitrate")] = std::round(stats.f_demux_bitrate * 8000);
    m_values[QStringLiteral("corrupted")] = number(stats.i_demux_corrupted);
    m_values[QStringLiteral("discontinuities")] = number(stats.i_demux_discontinuity);
    m_values[QStringLiteral("videoDecoded")] = number(stats.i_decoded_video);
    m_values[QStringLiteral("videoDisplayed")] = number(stats.i_displayed_pictures);
    m_values[QStringLiteral("videoLate")] = number(stats.i_late_pictures);
    m_values[QStringLiteral("videoLost")] = number(stats.i_lost_pictures);
    m_values[QStringLiteral("audioDecoded")] = number(stats.i_decoded_audio);
    m_values[QStringLiteral("audioPlayed")] = number(stats.i_played_abuffers);
    m_values[QStringLiteral("audioLost")] = number(stats.i_lost_abuffers);

    m_bitrates.append(stats.f_input_bitrate * 8000);
    if (m_bitrates.size() > bitrateHistory)
        m_bitrates.remove(0, m_bitrates.size() - bitrateHistory);
    emit valuesChanged();
}

/*****************************************************************************
 * PluginListModel
 *****************************************************************************/

PluginListModel::PluginListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    size_t count = 0;
    module_t **list = module_list_get(&count);
    m_all.reserve(static_cast<int>(count));
    for (size_t i = 0; i < count; i++)
    {
        const module_t *module = list[i];
        Plugin plugin;
        plugin.name = qfu(module_GetLongName(module));
        plugin.capability = qfu(module_get_capability(module));
        plugin.shortName = qfu(module_GetShortName(module));
        plugin.help = qfu(module_get_help(module));
        plugin.score = module_get_score(module);
        // as the Plugins tab: the modules that are only parts of others
        // (no capability) out
        if (plugin.capability.isEmpty())
            continue;
        m_all.append(plugin);
    }
    module_list_free(list);
    refilter();
}

int PluginListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_shown.size());
}

QVariant PluginListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_shown.size())
        return {};
    const Plugin &plugin = m_all.at(m_shown.at(index.row()));
    switch (role)
    {
    case Qt::DisplayRole:
    case NameRole: return plugin.name;
    case CapabilityRole: return plugin.capability;
    case ScoreRole: return plugin.score;
    case ShortNameRole: return plugin.shortName;
    case HelpRole: return plugin.help;
    default: return {};
    }
}

QHash<int, QByteArray> PluginListModel::roleNames() const
{
    return {
        { NameRole, "name" },
        { CapabilityRole, "capability" },
        { ScoreRole, "score" },
        { ShortNameRole, "shortName" },
        { HelpRole, "help" },
    };
}

void PluginListModel::setSearchPattern(const QString &pattern)
{
    if (pattern == m_pattern)
        return;
    m_pattern = pattern;
    refilter();
    emit searchPatternChanged();
}

void PluginListModel::setSortCriteria(const QString &criteria)
{
    if (criteria == m_sort)
        return;
    m_sort = criteria;
    refilter();
    emit sortCriteriaChanged();
}

void PluginListModel::refilter()
{
    beginResetModel();
    m_shown.clear();
    const QString pattern = m_pattern.trimmed();
    for (int i = 0; i < m_all.size(); i++)
    {
        const Plugin &plugin = m_all.at(i);
        if (pattern.isEmpty() || plugin.name.contains(pattern, Qt::CaseInsensitive)
            || plugin.capability.contains(pattern, Qt::CaseInsensitive)
            || plugin.shortName.contains(pattern, Qt::CaseInsensitive))
            m_shown.append(i);
    }
    const auto byName = [this](int a, int b) {
        return QString::localeAwareCompare(m_all.at(a).name, m_all.at(b).name) < 0;
    };
    if (m_sort == QLatin1String("capability"))
        std::stable_sort(m_shown.begin(), m_shown.end(), [this, byName](int a, int b) {
            const int order = QString::compare(m_all.at(a).capability, m_all.at(b).capability, Qt::CaseInsensitive);
            if (order != 0)
                return order < 0;
            // the module chosen first for a capability at the top
            if (m_all.at(a).score != m_all.at(b).score)
                return m_all.at(a).score > m_all.at(b).score;
            return byName(a, b);
        });
    else if (m_sort == QLatin1String("score"))
        std::stable_sort(m_shown.begin(), m_shown.end(), [this, byName](int a, int b) {
            if (m_all.at(a).score != m_all.at(b).score)
                return m_all.at(a).score > m_all.at(b).score;
            return byName(a, b);
        });
    else
        std::stable_sort(m_shown.begin(), m_shown.end(), byName);
    endResetModel();
    emit countChanged();
}

/*****************************************************************************
 * ExtensionInfoModel
 *****************************************************************************/

ExtensionInfoModel::ExtensionInfoModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

void ExtensionInfoModel::setCtx(MainCtx *ctx)
{
    if (ctx == m_ctx)
        return;
    m_ctx = ctx;
    if (ctx && ctx->getIntf())
    {
        ExtensionsManager *manager = ExtensionsManager::getInstance(ctx->getIntf());
        connect(manager, &ExtensionsManager::extensionsUpdated, this, &ExtensionInfoModel::updateList);
        // loaded now if not yet (as the Extensions tab)
        if (!manager->isLoaded() && !manager->cannotLoad())
            manager->loadExtensions();
        updateList();
    }
    emit ctxChanged();
}

void ExtensionInfoModel::reload()
{
    if (!m_ctx || !m_ctx->getIntf())
        return;
    ExtensionsManager::getInstance(m_ctx->getIntf())->reloadExtensions();
}

void ExtensionInfoModel::updateList()
{
    beginResetModel();
    m_extensions.clear();
    if (m_ctx && m_ctx->getIntf())
    {
        ExtensionsManager *manager = ExtensionsManager::getInstance(m_ctx->getIntf());
        if (extensions_manager_t *p_mgr = manager->getManager())
        {
            vlc_mutex_lock(&p_mgr->lock);
            extension_t *p_ext;
            ARRAY_FOREACH(p_ext, p_mgr->extensions)
            {
                Extension extension;
                extension.title = qfu(p_ext->psz_title);
                extension.description = qfu(p_ext->psz_description);
                extension.summary = qfu(p_ext->psz_shortdescription);
                if (extension.summary.isEmpty())
                    extension.summary = extension.description;
                extension.version = qfu(p_ext->psz_version);
                extension.author = qfu(p_ext->psz_author);
                extension.url = qfu(p_ext->psz_url);
                extension.fileName = qfu(p_ext->psz_name);
                if (extension.title.isEmpty())
                    extension.title = QFileInfo(extension.fileName).completeBaseName();
                m_extensions.append(extension);
            }
            vlc_mutex_unlock(&p_mgr->lock);
        }
    }
    std::stable_sort(m_extensions.begin(), m_extensions.end(), [](const Extension &a, const Extension &b) {
        return QString::localeAwareCompare(a.title, b.title) < 0;
    });
    endResetModel();
    emit countChanged();
}

int ExtensionInfoModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_extensions.size());
}

QVariant ExtensionInfoModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_extensions.size())
        return {};
    const Extension &extension = m_extensions.at(index.row());
    switch (role)
    {
    case Qt::DisplayRole:
    case TitleRole: return extension.title;
    case SummaryRole: return extension.summary;
    case DescriptionRole: return extension.description;
    case VersionRole: return extension.version;
    case AuthorRole: return extension.author;
    case UrlRole: return extension.url;
    case FileNameRole: return QFileInfo(extension.fileName).fileName();
    default: return {};
    }
}

QHash<int, QByteArray> ExtensionInfoModel::roleNames() const
{
    return {
        { TitleRole, "title" },
        { SummaryRole, "summary" },
        { DescriptionRole, "description" },
        { VersionRole, "version" },
        { AuthorRole, "author" },
        { UrlRole, "url" },
        { FileNameRole, "fileName" },
    };
}

/*****************************************************************************
 * FolderInformation
 *****************************************************************************/

namespace {

// at most so many entries looked at (a whole drive asked for)
constexpr int entryLimit = 200000;

QSet<QString> suffixes(const char *list)
{
    // "*.avi;*.mkv;..."
    QSet<QString> set;
    const QStringList parts = QString::fromLatin1(list).split(QLatin1Char(';'), Qt::SkipEmptyParts);
    for (const QString &part : parts)
    {
        QString suffix = part.trimmed();
        if (suffix.startsWith(QLatin1String("*.")))
            suffix = suffix.mid(2);
        if (!suffix.isEmpty() && suffix != QLatin1String("*"))
            set.insert(suffix.toLower());
    }
    return set;
}

FolderInformation::Counts count(const QString &path, const std::shared_ptr<std::atomic_bool> &cancel)
{
    static const QSet<QString> video = suffixes(EXTENSIONS_VIDEO);
    static const QSet<QString> audio = suffixes(EXTENSIONS_AUDIO);
    static const QSet<QString> subtitle = suffixes(EXTENSIONS_SUBTITLE);

    FolderInformation::Counts counts;
    QDirIterator it(path, QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System,
                    QDirIterator::Subdirectories);
    int seen = 0;
    while (it.hasNext())
    {
        if (cancel->load())
            break;
        if (++seen > entryLimit)
        {
            counts.partial = true;
            break;
        }
        it.next();
        const QFileInfo info = it.fileInfo();
        if (info.isSymLink())
            continue;
        if (info.isDir())
        {
            counts.folders++;
            continue;
        }
        counts.files++;
        counts.bytes += info.size();
        const QString suffix = info.suffix().toLower();
        if (video.contains(suffix))
            counts.videos++;
        else if (audio.contains(suffix))
            counts.audios++;
        else if (subtitle.contains(suffix))
            counts.subtitles++;
    }
    return counts;
}

} // anonymous namespace

FolderInformation::FolderInformation(QObject *parent)
    : QObject(parent)
{
}

FolderInformation::~FolderInformation()
{
    if (m_cancel)
        m_cancel->store(true);
}

void FolderInformation::setUrl(const QUrl &url)
{
    if (url == m_url)
        return;
    if (m_cancel)
        m_cancel->store(true);
    m_url = url;
    m_counts = Counts();

    const QFileInfo info(path());
    m_exists = !path().isEmpty() && info.isDir();
    m_modified = m_exists ? QLocale().toString(info.lastModified(), QLocale::LongFormat) : QString();
    m_busy = m_exists;
    emit urlChanged();
    emit countsChanged();
    if (!m_exists)
        return;

    auto cancel = std::make_shared<std::atomic_bool>(false);
    m_cancel = cancel;
    const QString folder = path();
    const QPointer<FolderInformation> guard(this);
    QThreadPool::globalInstance()->start([guard, cancel, folder]() {
        const Counts counts = count(folder, cancel);
        if (cancel->load())
            return;
        QMetaObject::invokeMethod(qApp, [guard, cancel, counts]() {
            if (!guard || cancel->load())
                return;
            guard->m_counts = counts;
            guard->m_busy = false;
            emit guard->countsChanged();
        }, Qt::QueuedConnection);
    });
}

QString FolderInformation::name() const
{
    const QString folder = path();
    const QString name = QFileInfo(folder).fileName();
    // a drive: "C:"
    return name.isEmpty() ? QDir::toNativeSeparators(folder) : name;
}

QString FolderInformation::path() const
{
    if (!m_url.isLocalFile())
        return {};
    QString folder = m_url.toLocalFile();
    if (folder.size() > 3 && (folder.endsWith(QLatin1Char('/')) || folder.endsWith(QLatin1Char('\\'))))
        folder.chop(1);
    return folder;
}

QString FolderInformation::size() const
{
    return QLocale().formattedDataSize(m_counts.bytes);
}
