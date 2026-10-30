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

#include "subtitle_finder.hpp"
#include "util/language_change.hpp"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QSet>
#include <QSettings>
#include <QThread>
#include <QUrl>

#include <memory>

#include <vlc_common.h>
#include <vlc_configuration.h>
#include <vlc_es.h>
#include <vlc_player.h>
#include <vlc_url.h>

#include "maininterface/mainctx.hpp"
#include "player/input_models.hpp"
#include "player/player_controller.hpp"
#include "player/subtitle_providers.hpp"
#include "util/http_request.hpp"
#include "iptv/iptv_addon.hpp"
#include "util/subtitle_preferences.hpp"

namespace {

// the player has its tracks (and VLC its subtitles next to the file) then
constexpr int startDelayMs = 1500;

// Where the subtitle files added to the media come from, by the MD5 of
// their MRL (the start of the ids of their tracks)
struct Source
{
    QString kind;     // file, added, online
    QString name;     // the release, or the name of the file
    QString provider; // the service it was found on
    QString folder;
    QString path;     // the local file, for whoever reads it again
};

QHash<QString, Source> &sources()
{
    static QHash<QString, Source> table;
    return table;
}

SubtitleFinder *&finderInstance()
{
    static SubtitleFinder *instance = nullptr;
    return instance;
}

QString mrlHash(const QString &mrl)
{
    return QString::fromLatin1(QCryptographicHash::hash(mrl.toUtf8(), QCryptographicHash::Md5).toHex());
}

// The MRL VLC makes of a file it finds next to a video (vlc_path2uri)
QString fileMrl(const QString &path)
{
    char *uri = vlc_path2uri(qtu(QDir::toNativeSeparators(path)), nullptr);
    const QString mrl = uri ? qfu(uri) : QUrl::fromLocalFile(path).toString(QUrl::FullyEncoded);
    free(uri);
    return mrl;
}

// the extensions of the subtitle files VLC finds next to a video
bool isSubtitleFile(const QString &name)
{
    static const QSet<QString> extensions = {
        "idx", "sub", "srt", "ssa", "ass", "smi", "utf", "utf8", "utf-8", "rt", "aqt", "txt",
        "usf", "jss", "cdg", "psb", "mpsub", "mpl2", "pjs", "dks", "stl", "vtt", "sbv", "ttml",
        "scc" };
    return extensions.contains(QFileInfo(name).suffix().toLower());
}

} // namespace

SubtitleFinder::SubtitleFinder(qt_intf_t *intf, PlayerController *player,
                               SubtitlePreferences *preferences, QObject *parent)
    : QObject(parent)
    , m_intf(intf)
    , m_player(player)
    , m_preferences(preferences)
    , m_search(new SubtitleSearch(intf, preferences, this))
{
    static_assert(int(Failed) == int(SubtitleSearch::Failed) && int(Ready) == int(SubtitleSearch::Ready),
                  "SubtitleFinder::State is SubtitleSearch::State");
    connect(m_search, &SubtitleSearch::stateChanged, this, &SubtitleFinder::stateChanged);
    connect(m_search, &SubtitleSearch::resultsChanged, this, &SubtitleFinder::resultsChanged);
    connect(m_search, &SubtitleSearch::runsChanged, this, &SubtitleFinder::providersChanged);
    connect(m_search, &SubtitleSearch::finished, this, &SubtitleFinder::onSearchFinished);
    m_startTimer.setSingleShot(true);
    m_startTimer.setInterval(startDelayMs);
    connect(&m_startTimer, &QTimer::timeout, this, &SubtitleFinder::onStarted);
    connect(m_player, &PlayerController::inputChanged, this, &SubtitleFinder::onInputChanged);
    finderInstance() = this;
    TrackListModel *tracks = m_player->getSubtitleTracks();
    connect(tracks, &QAbstractItemModel::rowsInserted, this, &SubtitleFinder::selectAdded);
    connect(tracks, &QAbstractItemModel::modelReset, this, &SubtitleFinder::selectAdded);
}

SubtitleFinder::~SubtitleFinder()
{
    // (noteFile() may still be called: it tells nobody then)
    if (finderInstance() == this)
        finderInstance() = nullptr;
}

QString SubtitleFinder::externalTitle(const char *trackId)
{
    const char *slash = trackId ? strchr(trackId, '/') : nullptr;
    if (!slash || slash - trackId != 32)
        return {};
    // (the files of the user keep VLC's names: known a moment after them)
    const Source source = sources().value(QString::fromLatin1(trackId, 32));
    return source.kind == QLatin1String("online") ? source.name : QString();
}

QVariantMap SubtitleFinder::externalSource(const char *trackId)
{
    const char *slash = trackId ? strchr(trackId, '/') : nullptr;
    if (!slash || slash - trackId != 32)
        return {};
    const auto it = sources().constFind(QString::fromLatin1(trackId, 32));
    if (it == sources().constEnd())
        return {};
    return { { QStringLiteral("kind"), it->kind }, { QStringLiteral("name"), it->name },
             { QStringLiteral("provider"), it->provider }, { QStringLiteral("folder"), it->folder } };
}

QString SubtitleFinder::externalPath(const char *trackId)
{
    const char *slash = trackId ? strchr(trackId, '/') : nullptr;
    if (!slash)
        return {};
    const auto it = sources().constFind(QString::fromLatin1(trackId, slash - trackId));
    return it != sources().constEnd() ? it->path : QString();
}

void SubtitleFinder::noteFile(const QString &mrl, const QString &kind, const QString &name,
                              const QString &provider)
{
    const QString key = mrlHash(mrl);
    // one found online stays so when it is added again
    const auto it = sources().constFind(key);
    if (it != sources().constEnd() && it->kind == QLatin1String("online") && kind != QLatin1String("online"))
        return;
    const QUrl url(mrl);
    const QString local = url.isLocalFile() ? url.toLocalFile() : QString();
    const QString folder = local.isEmpty() ? QString() : QFileInfo(local).absolutePath();
    sources().insert(key, { kind, name, provider, folder, local });
    if (finderInstance())
        emit finderInstance()->sourcesChanged();
}

// The subtitle files VLC finds by itself next to a local video (and in its
// sub-autodetect-path folders): their names, for the tracks it makes of
// them (on a worker thread: the folder can be on a slow drive)
void SubtitleFinder::scanFolder()
{
    const QUrl url = m_player->getUrl();
    if (!url.isLocalFile())
        return;
    const QString video = url.toLocalFile();
    QStringList folders = { QFileInfo(video).absolutePath() };
    char *paths = config_GetPsz("sub-autodetect-path");
    for (const QString &relative : qfu(paths ? paths : "").split(QLatin1Char(','), Qt::SkipEmptyParts))
    {
        const QString path = relative.trimmed();
        if (!path.isEmpty())
            folders.append(QDir::isAbsolutePath(path) ? path : QFileInfo(video).absoluteDir().filePath(path));
    }
    free(paths);

    auto found = std::make_shared<QList<QPair<QString, QString>>>(); // MRL, name
    QThread *worker = QThread::create([found, folders]() {
        for (const QString &folder : folders)
        {
            const QFileInfoList files = QDir(folder).entryInfoList(QDir::Files | QDir::Readable);
            for (const QFileInfo &file : files)
                if (isSubtitleFile(file.fileName()))
                    found->append({ fileMrl(file.absoluteFilePath()), file.fileName() });
            if (found->size() > 500)
                break;
        }
    });
    // (by the finished signal, as HTTP answers: dropped if the finder is
    // gone, which the worker can't tell)
    connect(worker, &QThread::finished, this, [found]() {
        for (const auto &file : *found)
            noteFile(file.first, QStringLiteral("file"), file.second);
    });
    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    worker->start(QThread::LowPriority);
}

// VLC shows a subtitle added to a media only when none is (b_forced), so
// the track of the file is selected once it is listed
void SubtitleFinder::selectAdded()
{
    if (m_selectHash.isEmpty())
        return;
    const QByteArray prefix = m_selectHash.toLatin1() + '/';
    vlc_player_t *player = m_intf->p_player;
    vlc_player_Lock(player);
    const size_t count = vlc_player_GetSubtitleTrackCount(player);
    for (size_t i = 0; i < count; ++i)
    {
        const struct vlc_player_track *track = vlc_player_GetSubtitleTrackAt(player, i);
        const char *id = track ? vlc_es_id_GetStrId(track->es_id) : nullptr;
        if (id && !strncmp(id, prefix.constData(), size_t(prefix.size())))
        {
            if (!track->selected)
                vlc_player_SelectEsId(player, track->es_id, m_selectAlongside ? VLC_PLAYER_SELECT_SIMULTANEOUS
                                                                              : VLC_PLAYER_SELECT_EXCLUSIVE);
            m_selectHash.clear();
            // the default subtitle shown (alone): the secondary one with it
            if (!m_selectAlongside)
                QMetaObject::invokeMethod(this, &SubtitleFinder::ensureSecondary, Qt::QueuedConnection);
            break;
        }
    }
    vlc_player_Unlock(player);
}

bool SubtitleFinder::hasTrackOf(const QString &file) const
{
    // added by the finder (its URL), or by the media library (a file linked
    // to the media: VLC's MRL, which escapes more)
    const QByteArray prefix = mrlHash(QUrl::fromLocalFile(file).toString(QUrl::FullyEncoded)).toLatin1() + '/';
    const QByteArray libraryPrefix = mrlHash(fileMrl(file)).toLatin1() + '/';
    bool found = false;
    vlc_player_t *player = m_intf->p_player;
    vlc_player_Lock(player);
    const size_t count = vlc_player_GetSubtitleTrackCount(player);
    for (size_t i = 0; i < count && !found; ++i)
    {
        const struct vlc_player_track *track = vlc_player_GetSubtitleTrackAt(player, i);
        const char *id = track ? vlc_es_id_GetStrId(track->es_id) : nullptr;
        found = id && (!strncmp(id, prefix.constData(), size_t(prefix.size()))
                       || !strncmp(id, libraryPrefix.constData(), size_t(libraryPrefix.size())));
    }
    vlc_player_Unlock(player);
    return found;
}

bool SubtitleFinder::supported() const
{
    return httpRequestSupported();
}

bool SubtitleFinder::searchesByItself() const
{
    return supported() && m_preferences->autoSearch()
        && !(m_intf->p_mi && m_intf->p_mi->onlineChoicesPending());
}

SubtitleSearch *SubtitleFinder::newSearch(QObject *parent) const
{
    return new SubtitleSearch(m_intf, m_preferences, parent);
}

QSettings *SubtitleFinder::settings() const
{
    return m_intf->mainSettings;
}

void SubtitleFinder::onInputChanged(bool hasInput)
{
    ++m_revision;
    m_startTimer.stop();
    m_autoPending = false;
    m_queue.clear();
    m_loadingId.clear();
    m_loadedIds.clear();
    m_titleResults.clear();
    // (its answers on their way, downloads too, dropped)
    m_search->reset();
    emit titleResultsChanged();
    if (hasInput)
    {
        scanFolder();
        m_startTimer.start();
    }
}

// The media started: its subtitle remembered, or the best one online when
// it has none in the user's language
void SubtitleFinder::onStarted()
{
    if (!m_player->hasVideoOutput())
        return; // music: no subtitles to look for
    // a TV channel, or a programme of its archive (the subtitles stay as
    // the channel has them), or another live stream: no film
    if (IptvAddon::isTvUrl(m_player->getUrl().toString()))
        return;
    if (!m_player->getUrl().isLocalFile()
        && (!m_player->getLength().valid() || m_player->getLength().toSecf() <= 0))
        return;
    prepareMedia([this]() {
        const QStringList local = localLanguages();
        const QString primary = m_preferences->primary();
        const QString secondary = m_preferences->secondary();
        const bool showBoth = m_preferences->showBoth() && !secondary.isEmpty();
        // the subtitle chosen for this file last time, in one of the user's
        // languages: shown again, from the cache
        const QStringList remembered = rememberedChoice();
        const QString rememberedLanguage = remembered.isEmpty() ? QString() : remembered[2];
        if (remembered.size() >= 5)
        {
            // one the Add Subtitles dialog linked to the media (its track
            // listed already): found online all the same, on its service
            const QString file = remembered[0];
            for (const QString &mrl : { QUrl::fromLocalFile(file).toString(QUrl::FullyEncoded), fileMrl(file) })
                noteFile(mrl, QStringLiteral("online"), remembered[3], remembered[4]);
        }
        if (!remembered.isEmpty() && !hasTrackOf(remembered[0])
            && (rememberedLanguage == primary || (showBoth && rememberedLanguage == secondary)))
            attach(remembered[0], remembered[1], rememberedLanguage, true, rememberedLanguage != primary);
        // the default language: remembered, or in the media (VLC shows it, its
        // sub-language); the secondary one too when both are shown
        const bool needPrimary = rememberedLanguage != primary && !local.contains(primary);
        const bool needSecondary = showBoth && rememberedLanguage != secondary && !local.contains(secondary);
        if ((!needPrimary && !needSecondary) || !searchesByItself())
        {
            if (rememberedLanguage != primary)
                ensureSecondary();
            return;
        }
        m_autoPending = true;
        search();
    });
}

// The media playing given to the search (its name read, and the hash of
// the file on a worker thread: it can be on a slow drive), then next()
void SubtitleFinder::prepareMedia(std::function<void()> next)
{
    const QUrl url = m_player->getUrl();
    if (!m_search->mediaKey().isEmpty() && m_search->url() == url)
    {
        next();
        return;
    }
    double fps = 0;
    {
        vlc_player_t *player = m_intf->p_player;
        vlc_player_Lock(player);
        const struct vlc_player_track *video = vlc_player_GetSelectedTrack(player, VIDEO_ES);
        if (video && video->fmt.video.i_frame_rate && video->fmt.video.i_frame_rate_base)
            fps = double(video->fmt.video.i_frame_rate) / video->fmt.video.i_frame_rate_base;
        vlc_player_Unlock(player);
    }
    m_search->setMedia(url, m_player->getName(), fps);
    const quint64 revision = m_revision;
    m_search->prepare([this, revision, next]() {
        if (revision == m_revision)
            next();
    });
}

QStringList SubtitleFinder::localLanguages() const
{
    QStringList languages;
    vlc_player_t *player = m_intf->p_player;
    vlc_player_Lock(player);
    const size_t count = vlc_player_GetSubtitleTrackCount(player);
    for (size_t i = 0; i < count; ++i)
    {
        const struct vlc_player_track *track = vlc_player_GetSubtitleTrackAt(player, i);
        if (track && track->fmt.psz_language)
        {
            const QString language = SubtitlePreferences::normalize(qfu(track->fmt.psz_language));
            if (!language.isEmpty())
                languages.append(language);
        }
    }
    vlc_player_Unlock(player);
    return languages;
}

void SubtitleFinder::search()
{
    if (!supported())
    {
        m_search->start(); // (it says why)
        return;
    }
    const State current = state();
    if (current == Identifying || current == Searching)
        return;
    msg_Dbg(m_intf, "subtitles: searching online (%s)", m_autoPending ? "by itself" : "asked");
    // the media playing given first: the search starts from it
    const quint64 revision = m_revision;
    prepareMedia([this, revision]() {
        if (revision == m_revision)
            m_search->start();
    });
}

void SubtitleFinder::onSearchFinished()
{
    if (m_autoPending)
        autoPick();
}

QVariantList SubtitleFinder::services() const
{
    // in another language of the interface, the bindings asking are made again
    markTranslationBinding(this);
    QVariantList list;
    const QHash<QString, SubtitleSearch::Run> &runs = m_search->runs();
    for (const SubtitleProviders::Provider &provider : SubtitleProviders::providers())
    {
        const auto run = runs.constFind(provider.id);
        list.append(QVariantMap{
            { QStringLiteral("id"), provider.id },
            { QStringLiteral("name"), provider.name },
            { QStringLiteral("detail"), qfut(provider.detail) },
            { QStringLiteral("enabled"), serviceEnabled(provider.id) },
            { QStringLiteral("busy"), run != runs.constEnd() && run->busy },
            { QStringLiteral("count"), run != runs.constEnd() ? run->count : -1 },
            { QStringLiteral("error"), run != runs.constEnd() ? run->error : QString() },
        });
    }
    return list;
}

bool SubtitleFinder::serviceEnabled(const QString &id) const
{
    return SubtitleSearch::serviceEnabled(m_intf->mainSettings, id);
}

void SubtitleFinder::setServiceEnabled(const QString &id, bool enabled)
{
    if (serviceEnabled(id) == enabled)
        return;
    SubtitleSearch::setServiceEnabled(m_intf->mainSettings, id, enabled);
    emit providersChanged();
}

bool SubtitleFinder::loadedLanguage(const QString &language) const
{
    for (const QVariant &r : m_search->results())
    {
        const QVariantMap map = r.toMap();
        if (map.value(QStringLiteral("language")).toString() == language
            && m_loadedIds.contains(map.value(QStringLiteral("id")).toString()))
            return true;
    }
    return false;
}

QVariantMap SubtitleFinder::bestOfLearned(const QStringList &local) const
{
    const QString learned = m_preferences->learned();
    if (learned.isEmpty() || local.contains(learned) || loadedLanguage(learned))
        return {};
    return m_search->bestOf(learned);
}

// The best subtitle of the default language, else of the secondary one
// unless the video has it already; then the secondary one with it, when
// both are to be shown
void SubtitleFinder::autoPick()
{
    m_autoPending = false;
    const QStringList local = localLanguages();
    const QString primary = m_preferences->primary();
    const QString secondary = m_preferences->secondary();
    if (!local.contains(primary) && !loadedLanguage(primary))
    {
        QVariantMap best = m_search->bestOf(primary);
        if (best.isEmpty() && !secondary.isEmpty() && !local.contains(secondary) && !loadedLanguage(secondary))
        {
            best = m_search->bestOf(secondary);
            // none in either: the language the user picks most
            if (best.isEmpty())
                best = bestOfLearned(local);
            if (!best.isEmpty())
            {
                load(best.value(QStringLiteral("id")).toString());
                emit autoLoaded(best.value(QStringLiteral("languageName")).toString());
            }
            return;
        }
        if (best.isEmpty() && secondary.isEmpty())
            best = bestOfLearned(local);
        if (!best.isEmpty())
        {
            // the secondary one follows, once this one is shown (selectAdded)
            load(best.value(QStringLiteral("id")).toString());
            emit autoLoaded(best.value(QStringLiteral("languageName")).toString());
            return;
        }
    }
    ensureSecondary();
}

// The secondary language shown too: a track of the media, else the best
// subtitle found online
void SubtitleFinder::ensureSecondary()
{
    const QString secondary = m_preferences->secondary();
    if (!m_preferences->showBoth() || secondary.isEmpty())
        return;
    vlc_player_t *player = m_intf->p_player;
    bool found = false;
    vlc_player_Lock(player);
    const size_t count = vlc_player_GetSubtitleTrackCount(player);
    for (size_t i = 0; i < count && !found; ++i)
    {
        const struct vlc_player_track *track = vlc_player_GetSubtitleTrackAt(player, i);
        if (track && track->fmt.psz_language
            && SubtitlePreferences::normalize(qfu(track->fmt.psz_language)) == secondary)
        {
            found = true;
            if (!track->selected)
                vlc_player_SelectEsId(player, track->es_id, VLC_PLAYER_SELECT_SIMULTANEOUS);
        }
    }
    vlc_player_Unlock(player);
    if (found)
        return;
    const QVariantMap best = m_search->bestOf(secondary);
    if (!best.isEmpty())
        loadAlongside(best.value(QStringLiteral("id")).toString());
}

void SubtitleFinder::findTitle(const QString &query)
{
    if (query.trimmed().isEmpty() || !supported())
        return;
    m_titleBusy = true;
    m_titleResults.clear();
    emit titleResultsChanged();
    m_search->findTitles(query, [this](const QVariantList &titles) {
        m_titleResults = titles;
        m_titleBusy = false;
        emit titleResultsChanged();
    });
}

void SubtitleFinder::useTitle(int index)
{
    if (index < 0 || index >= m_titleResults.size() || m_search->mediaKey().isEmpty())
        return;
    const QVariantMap entry = m_titleResults[index].toMap();
    SubtitleSearch::Match match;
    match.imdb = entry.value(QStringLiteral("imdb")).toString();
    match.title = entry.value(QStringLiteral("label")).toString();
    match.year = entry.value(QStringLiteral("year")).toInt();
    match.series = entry.value(QStringLiteral("series")).toBool();
    // (a download going on goes on: it is of this file all the same)
    m_titleResults.clear();
    emit titleResultsChanged();
    m_search->useMatch(match);
}

void SubtitleFinder::load(const QString &id)
{
    download(id, false);
}

void SubtitleFinder::loadAlongside(const QString &id)
{
    download(id, true);
}

void SubtitleFinder::download(const QString &id, bool alongside)
{
    const SubtitleProviders::Result *choice = m_search->result(id);
    if (!choice)
    {
        // (one of a search since replaced) the next one waiting
        if (m_loadingId.isEmpty())
            downloadNext();
        return;
    }
    if (!m_loadingId.isEmpty())
    {
        // after the one going on
        if (m_loadingId != id && !m_queue.contains(qMakePair(id, alongside)))
            m_queue.append(qMakePair(id, alongside));
        return;
    }
    const QString language = choice->language;
    // in VLC's cache, a folder a media; the others not played for long
    // cleaned up (no cache folder: nothing to clean up)
    const QString root = SubtitleSearch::cacheRoot();
    const QString folder = root + QLatin1Char('/') + SubtitleSearch::folderName(m_search->mediaKey());
    m_loadingId = id;
    emit resultsChanged();
    // (its answer dropped by the search once the media changed)
    m_search->download(id, folder, !root.isEmpty(),
                       [this, id, language, alongside](const QString &file, const QString &error) {
        m_loadingId.clear();
        if (file.isEmpty())
        {
            emit resultsChanged();
            m_search->setError(error);
            emit downloadFailed(error);
            downloadNext();
            return;
        }
        attach(file, id, language, true, alongside);
    });
}

void SubtitleFinder::downloadNext()
{
    if (m_queue.isEmpty())
        return;
    const QPair<QString, bool> next = m_queue.takeFirst();
    download(next.first, next.second);
}

void SubtitleFinder::attach(const QString &file, const QString &id, const QString &language, bool select,
                            bool alongside)
{
    const QString uri = QUrl::fromLocalFile(file).toString(QUrl::FullyEncoded);
    // its track named after the release, and shown once listed
    const QVariantMap shown = m_search->resultMap(id);
    QString release = shown.value(QStringLiteral("release")).toString();
    QString provider = shown.value(QStringLiteral("providerName")).toString();
    const QStringList remembered = rememberedChoice();
    if (remembered.size() >= 5 && remembered[1] == id)
    {
        // (from the cache: kept with its release and service)
        if (release.isEmpty())
            release = remembered[3];
        if (provider.isEmpty())
            provider = remembered[4];
    }
    if (release.isEmpty())
        release = QFileInfo(file).completeBaseName().section(QLatin1Char('-'), 0, -2);
    if (provider.isEmpty())
    {
        // its service is in its id ("<service>:<its id>")
        const QString service = id.section(QLatin1Char(':'), 0, 0);
        const SubtitleProviders::Provider *p = SubtitleProviders::provider(service);
        provider = p ? p->name : QString();
    }
    noteFile(uri, QStringLiteral("online"), release, provider);
    if (select)
    {
        m_selectHash = mrlHash(uri);
        m_selectAlongside = alongside;
    }
    // used: it stays in the cache (pruneCache goes by the time of the files)
    {
        QFile used(file);
        if (used.open(QIODevice::ReadWrite))
            used.setFileTime(QDateTime::currentDateTime(), QFileDevice::FileModificationTime);
    }
    // b_forced would take the place of the subtitle shown
    m_player->AddAssociatedMedia(SPU_ES, uri, select && !alongside, false, false);
    if (!m_loadedIds.contains(id))
        m_loadedIds.append(id);
    // shown again for this file next time, without looking online
    if (!alongside)
        SubtitleSearch::rememberChoice(m_intf->mainSettings, m_search->mediaKey(),
                                       { file, id, language, release, provider });
    emit resultsChanged();
    downloadNext();
}

QStringList SubtitleFinder::rememberedChoice() const
{
    return SubtitleSearch::rememberedChoice(m_intf->mainSettings, m_search->mediaKey());
}

