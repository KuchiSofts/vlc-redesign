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
#include <QRegularExpression>
#include <QSet>
#include <QSettings>
#include <QThread>
#include <QUrl>

#include <memory>

#include <vlc_common.h>
#include <vlc_configuration.h>
#include <vlc_es.h>
#include <vlc_interface.h>
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
    // what tells a file beside the video that is not the film's subtitles
    // (isPartialSubtitle): "forced" in its name, its number of cues (-1: not
    // counted)
    bool forced = false;
    int cues = -1;
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

bool recordSource(const QString &mrl, const QString &kind, const QString &name,
                  const QString &provider = {}, bool forced = false, int cues = -1)
{
    const QString key = mrlHash(mrl);
    // Keep the provider of an online subtitle when it is found locally.
    const auto it = sources().constFind(key);
    if (it != sources().constEnd() && it->kind == QLatin1String("online") && kind != QLatin1String("online"))
        return false;
    const QUrl url(mrl);
    const QString local = url.isLocalFile() ? url.toLocalFile() : QString();
    const QString folder = local.isEmpty() ? QString() : QFileInfo(local).absolutePath();
    sources().insert(key, { kind, name, provider, folder, local, forced, cues });
    return true;
}

bool isForcedName(const QString &name)
{
    return name.contains(QLatin1String("forced"), Qt::CaseInsensitive);
}

// The cues of a subtitle file of text (SubRip, WebVTT: "-->"; SSA/ASS:
// "Dialogue:"), its first 4 MB read; -1 for the others (VobSub, binary)
int countCues(const QString &path)
{
    const QString suffix = QFileInfo(path).suffix().toLower();
    const bool timed = suffix == QLatin1String("srt") || suffix == QLatin1String("vtt");
    const bool ass = suffix == QLatin1String("ass") || suffix == QLatin1String("ssa");
    if (!timed && !ass)
        return -1;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return -1;
    QByteArray data = file.read(4 << 20);
    // (UTF-16: its ASCII with a zero byte each)
    if (data.count('\0') > data.size() / 4)
        data.replace(QByteArray(1, '\0'), QByteArray());
    return int(timed ? data.count("-->") : data.count("Dialogue:"));
}

// A subtitle file that is not the film's subtitles: the forced lines only
// (signs, a foreign language spoken), or an advert of the site it came from
// (YTS's .srt named as the video: "Downloaded from YTS.MX", 3 cues for 103
// minutes). Under one cue in 5 minutes of a film of 20 minutes or more: a
// film has hundreds (a quiet one, "All Is Lost", 0.5 a minute); a short one
// ("Sintel", 15 minutes, 20 cues) is never judged so.
bool isPartialSubtitle(bool forced, int cues, double minutes)
{
    return forced || (cues >= 0 && minutes >= 20 && cues < minutes / 5);
}

// For the hearing impaired: "English (SDH).eng.srt", "Film.en.hi.srt", "(CC)"
bool isHearingImpairedName(const QString &name)
{
    static const QRegularExpression words(QStringLiteral("(^|[ ._()\\[\\]-])(sdh|hi|cc)([ ._()\\[\\]-]|$)"),
                                          QRegularExpression::CaseInsensitiveOption);
    return words.match(QFileInfo(name).completeBaseName()).hasMatch();
}

// The language a subtitle file's name states: its code last ("Hebrew.heb.srt",
// "Film.en.srt"), else a language's English name ("Hebrew.srt")
QString languageOfName(const QString &name)
{
    static const QRegularExpression separators(QStringLiteral("[ ._()\\[\\]-]+"));
    static const QSet<QString> notLanguages = { "sdh", "hi", "cc", "forced", "full", "sub", "subs", "srt",
                                                "eng-sdh", "default", "complete", "part" };
    const QStringList words = QFileInfo(name).completeBaseName().split(separators, Qt::SkipEmptyParts);
    if (words.isEmpty())
        return {};
    for (int i = words.size() - 1; i >= 0 && i >= words.size() - 3; --i)
    {
        const QString word = words[i].toLower();
        if (word.size() >= 2 && word.size() <= 3 && !notLanguages.contains(word))
        {
            const QString code = SubtitlePreferences::normalize(word);
            if (!code.isEmpty())
                return code;
        }
    }
    for (const QString &word : words)
        if (word.size() >= 4 && !notLanguages.contains(word.toLower()))
        {
            const QString code = SubtitlePreferences::normalize(word);
            if (!code.isEmpty())
                return code;
        }
    return {};
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
    // (the services of a language follow the languages to search)
    connect(m_preferences, &SubtitlePreferences::changed, this, &SubtitleFinder::providersChanged);
    connect(m_search, &SubtitleSearch::finished, this, &SubtitleFinder::onSearchFinished);
    m_startTimer.setSingleShot(true);
    m_startTimer.setInterval(startDelayMs);
    connect(&m_startTimer, &QTimer::timeout, this, &SubtitleFinder::onStarted);
    connect(m_player, &PlayerController::inputChanged, this, &SubtitleFinder::onInputChanged);
    // the picture (or a stream's length) came after the first look: looked
    // at again a moment later (VLC lists the subtitles beside the file then)
    auto lookAgain = [this]() {
        if (m_waitingForPicture && !m_startTimer.isActive())
            m_startTimer.start();
    };
    connect(m_player, &PlayerController::hasVideoOutputChanged, this, [lookAgain](bool has) {
        if (has)
            lookAgain();
    });
    connect(m_player, &PlayerController::lengthChanged, this, lookAgain);
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
    if (recordSource(mrl, kind, name, provider) && finderInstance())
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

    struct Found
    {
        QString mrl;
        QString name;
        bool forced;
        int cues;
    };
    auto found = std::make_shared<QList<Found>>();
    // the language files of the video's own subtitle folder (YTS's "Subs":
    // "Hebrew.heb.srt", "English (SDH).eng.srt"), which VLC does not load (no
    // video's name in theirs): only when the video is alone in its folder,
    // so the folder is surely its own
    auto ownFolder = std::make_shared<QList<FolderSubtitle>>();
    // (waited for at the exit, a few seconds at most: runInBackground; its
    // answer dropped if the finder is gone, which the worker can't tell)
    const quint64 revision = m_revision;
    const QString videoStem = QFileInfo(video).completeBaseName().toLower();
    runInBackground(this, [found, ownFolder, folders, videoStem]() {
        QThread::currentThread()->setPriority(QThread::LowPriority);
        static const QSet<QString> videoSuffixes = { EXTENSIONS_VIDEO_CSV };
        const QString videoFolder = QFileInfo(folders.first()).canonicalFilePath();
        bool alone = false;
        QSet<QString> seen; // Subs and subs are one folder on Windows and macOS
        for (const QString &folder : folders)
        {
            const QString canonical = QFileInfo(folder).canonicalFilePath();
            if (canonical.isEmpty() || seen.contains(canonical))
                continue;
            seen.insert(canonical);
            const QFileInfoList files = QDir(folder).entryInfoList(QDir::Files | QDir::Readable);
            if (canonical == videoFolder)
            {
                int videos = 0;
                for (const QFileInfo &file : files)
                    videos += videoSuffixes.contains(file.suffix().toLower()) ? 1 : 0;
                alone = videos == 1;
            }
            for (const QFileInfo &file : files)
            {
                if (!isSubtitleFile(file.fileName()))
                    continue;
                const bool forced = isForcedName(file.fileName());
                const bool own = canonical != videoFolder && alone;
                // its cues counted for the files VLC shows with the video
                // (named after it) and those of its own folder; not every
                // subtitle of a big folder of downloads
                const int cues = own || file.fileName().toLower().startsWith(videoStem)
                               ? countCues(file.absoluteFilePath()) : -1;
                found->append({ fileMrl(file.absoluteFilePath()), file.fileName(), forced, cues });
                if (own && !forced)
                {
                    const QString language = languageOfName(file.fileName());
                    if (!language.isEmpty())
                        ownFolder->append({ file.absoluteFilePath(), language,
                                            isHearingImpairedName(file.fileName()), cues });
                }
            }
            if (found->size() > 500)
                break;
        }
    }, [this, found, ownFolder, revision]() {
        if (revision != m_revision)
            return;
        // The files found next to the videos played before go: the table
        // keeps those of this one (at most ~500), not of every folder seen
        // in the session. (Only this scan records them; the subtitles added
        // or found online stay)
        bool changed = false;
        for (auto it = sources().begin(); it != sources().end();)
        {
            if (it->kind == QLatin1String("file"))
            {
                it = sources().erase(it);
                changed = true;
            }
            else
                ++it;
        }
        for (const Found &file : *found)
            changed |= recordSource(file.mrl, QStringLiteral("file"), file.name, {}, file.forced, file.cues);
        m_folderSubtitles = *ownFolder;
        // Each notification refreshes the subtitle-track source labels.
        if (changed)
            emit sourcesChanged();
    });
}

double SubtitleFinder::lengthMinutes() const
{
    const VLCDuration length = m_player->getLength();
    return length.valid() ? length.toSecf() / 60 : 0;
}

QString SubtitleFinder::folderSubtitle(const QString &language) const
{
    // the plain one first, the one for the hearing impaired else
    QString hearingImpaired;
    for (const FolderSubtitle &file : m_folderSubtitles)
        if (file.language == language && !isPartialSubtitle(false, file.cues, lengthMinutes()))
        {
            if (!file.hearingImpaired)
                return file.path;
            if (hearingImpaired.isEmpty())
                hearingImpaired = file.path;
        }
    return hearingImpaired;
}

void SubtitleFinder::attachFolderFile(const QString &path, bool alongside)
{
    const QString uri = QUrl::fromLocalFile(path).toString(QUrl::FullyEncoded);
    if (recordSource(uri, QStringLiteral("file"), QFileInfo(path).fileName()))
        emit sourcesChanged();
    m_selectHash = mrlHash(uri);
    m_selectAlongside = alongside;
    m_player->AddAssociatedMedia(SPU_ES, uri, !alongside, false, false);
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
    m_waitingForPicture = false;
    m_folderSubtitles.clear();
    m_autoPending = false;
    m_queue.clear();
    m_loadingId.clear();
    m_loadedIds.clear();
    m_titleResults.clear();
    // (a title search for the media before: its answer is dropped)
    ++m_titleRevision;
    m_titleBusy = false;
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
    m_waitingForPicture = false;
    // a TV channel, or a programme of its archive (the subtitles stay as
    // the channel has them): no film
    if (IptvAddon::isTvUrl(m_player->getUrl().toString()))
        return;
    // No picture yet: the app or the drive starting cold (a film opened
    // from the file manager), a slow server. Looked at again when it comes
    // (hasVideoOutputChanged); music never gets one
    if (!m_player->hasVideoOutput())
    {
        m_waitingForPicture = true;
        msg_Dbg(m_intf, "subtitles: no picture yet, waiting for it");
        return;
    }
    // a stream: its length known first (none: a live stream, no film; it
    // can come late from a slow server: lengthChanged)
    if (!m_player->getUrl().isLocalFile()
        && (!m_player->getLength().valid() || m_player->getLength().toSecf() <= 0))
    {
        m_waitingForPicture = true;
        msg_Dbg(m_intf, "subtitles: a stream of no length (yet)");
        return;
    }
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
        bool needPrimary = rememberedLanguage != primary && !local.contains(primary);
        bool needSecondary = showBoth && rememberedLanguage != secondary && !local.contains(secondary);
        // the video's own subtitle folder ("Subs") has it: shown from there,
        // nothing searched for it (local files: whatever the online setting)
        if (needPrimary)
        {
            const QString file = folderSubtitle(primary);
            if (!file.isEmpty())
            {
                msg_Dbg(m_intf, "subtitles: %s from the video's folder: %s", qtu(primary),
                        qtu(QFileInfo(file).fileName()));
                attachFolderFile(file, false);
                needPrimary = false;
            }
        }
        if (needSecondary)
        {
            const QString file = folderSubtitle(secondary);
            if (!file.isEmpty())
            {
                msg_Dbg(m_intf, "subtitles: %s from the video's folder: %s", qtu(secondary),
                        qtu(QFileInfo(file).fileName()));
                attachFolderFile(file, true);
                needSecondary = false;
            }
        }
        if (!needPrimary && !needSecondary)
            msg_Dbg(m_intf, "subtitles: none to search (%s)",
                    rememberedLanguage == primary ? "the one chosen before" : "the video has them");
        else if (!searchesByItself())
            msg_Dbg(m_intf, "subtitles: not searched by itself (the setting is off)");
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
    // a stream's own title (and its year), for an address that names no
    // film ("500.mp4"): the player's name joins the artist to it
    QString title;
    {
        vlc_player_t *player = m_intf->p_player;
        vlc_player_Lock(player);
        const struct vlc_player_track *video = vlc_player_GetSelectedTrack(player, VIDEO_ES);
        if (video && video->fmt.video.i_frame_rate && video->fmt.video.i_frame_rate_base)
            fps = double(video->fmt.video.i_frame_rate) / video->fmt.video.i_frame_rate_base;
        if (input_item_t *item = vlc_player_GetCurrentMedia(player))
        {
            char *meta = input_item_GetTitle(item);
            title = qfu(meta).trimmed();
            free(meta);
            char *date = input_item_GetDate(item);
            const QString year = qfu(date).left(4);
            free(date);
            if (!title.isEmpty() && year.size() == 4 && year.toInt() >= 1900 && year.toInt() <= 2100
                && !title.contains(year))
                title += QStringLiteral(" (%1)").arg(year);
        }
        vlc_player_Unlock(player);
    }
    m_search->setMedia(url, !title.isEmpty() ? title : m_player->getName(), fps);
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
        if (!track)
            continue;
        // A file's track: its id starts with the MD5 of its MRL, the media's
        // own tracks' don't
        const char *id = vlc_es_id_GetStrId(track->es_id);
        const bool fromFile = id && strlen(id) > 33 && id[32] == '/';
        const Source source = fromFile ? sources().value(QString::fromLatin1(id, 32)) : Source();
        // the forced lines only, or an advert: no subtitles of the film
        if (source.kind == QLatin1String("file") && isPartialSubtitle(source.forced, source.cues, lengthMinutes()))
        {
            msg_Dbg(m_intf, "subtitles: %s is not the film's subtitles (%s)", qtu(source.name),
                    source.forced ? "forced lines" : qtu(QStringLiteral("%1 cues").arg(source.cues)));
            continue;
        }
        const QString language = track->fmt.psz_language
                               ? SubtitlePreferences::normalize(qfu(track->fmt.psz_language)) : QString();
        if (!language.isEmpty())
        {
            languages.append(language);
            continue;
        }
        // A file of the user's beside the video (or added to it) whose name
        // says no language ("Film.srt"): taken for the preferred language,
        // so nothing is searched over it (every app's rule); one found
        // online is no file of the user's
        if (fromFile && source.kind != QLatin1String("online"))
            languages.append(m_preferences->primary());
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

namespace {

QStringList fieldNames(int fields)
{
    QStringList names;
    if (fields & SubtitleProviders::KeyField)
        names.append(QStringLiteral("key"));
    // (who first: the user name or the email, then the password)
    if (fields & SubtitleProviders::UsernameField)
        names.append(QStringLiteral("username"));
    if (fields & SubtitleProviders::EmailField)
        names.append(QStringLiteral("email"));
    if (fields & SubtitleProviders::PasswordField)
        names.append(QStringLiteral("password"));
    return names;
}

SubtitleProviders::Credentials credentialsOf(const QVariantMap &map)
{
    SubtitleProviders::Credentials c;
    c.key = map.value(QStringLiteral("key")).toString().trimmed();
    c.username = map.value(QStringLiteral("username")).toString().trimmed();
    c.password = map.value(QStringLiteral("password")).toString();
    c.email = map.value(QStringLiteral("email")).toString().trimmed();
    return c;
}

} // namespace

QVariantList SubtitleFinder::services() const
{
    // in another language of the interface, the bindings asking are made again
    markTranslationBinding(this);
    QSettings *settings = m_intf->mainSettings;
    const QStringList searched = m_preferences->searched();
    QVariantList list;
    const QHash<QString, SubtitleSearch::Run> &runs = m_search->runs();
    for (const SubtitleProviders::Provider &provider : SubtitleProviders::providers())
    {
        const auto run = runs.constFind(provider.id);
        const SubtitleProviders::Credentials own = SubtitleSearch::credentials(settings, provider.id);
        const bool complete = SubtitleProviders::hasRequired(provider, own);
        QString status;
        switch (SubtitleSearch::serviceStatus(settings, provider.id))
        {
            case SubtitleSearch::StatusKeyRefused: status = QStringLiteral("keyRefused"); break;
            case SubtitleSearch::StatusLoginRefused: status = QStringLiteral("loginRefused"); break;
            case SubtitleSearch::StatusBusy: status = QStringLiteral("busy"); break;
            default: break;
        }
        QStringList languageNames;
        bool languageOn = provider.languages.isEmpty();
        for (const QString &code : provider.languages)
        {
            languageNames.append(m_preferences->name(code));
            languageOn = languageOn || searched.contains(code);
        }
        list.append(QVariantMap{
            { QStringLiteral("id"), provider.id },
            { QStringLiteral("name"), provider.name },
            { QStringLiteral("detail"), qfut(provider.detail) },
            { QStringLiteral("enabled"), serviceEnabled(provider.id) && (!provider.required || complete) },
            { QStringLiteral("switched"), SubtitleSearch::serviceSwitched(settings, provider.id) },
            { QStringLiteral("removed"), SubtitleSearch::serviceRemoved(settings, provider.id) },
            { QStringLiteral("asked"), SubtitleSearch::serviceAsked(settings, provider, searched) },
            { QStringLiteral("busy"), run != runs.constEnd() && run->busy },
            { QStringLiteral("count"), run != runs.constEnd() ? run->count : -1 },
            { QStringLiteral("error"), run != runs.constEnd() ? run->error : QString() },
            { QStringLiteral("fields"), fieldNames(provider.fields) },
            { QStringLiteral("required"), fieldNames(provider.required) },
            { QStringLiteral("page"), provider.page },
            { QStringLiteral("account"), provider.account },
            { QStringLiteral("hasCredentials"), provider.fields != 0 && own != SubtitleProviders::Credentials() },
            { QStringLiteral("status"), status },
            { QStringLiteral("languages"), provider.languages },
            { QStringLiteral("languageNames"), languageNames.join(QLatin1String(" / ")) },
            { QStringLiteral("languageOn"), languageOn },
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

void SubtitleFinder::setServiceRemoved(const QString &id, bool removed)
{
    if (!SubtitleProviders::provider(id) || SubtitleSearch::serviceRemoved(m_intf->mainSettings, id) == removed)
        return;
    SubtitleSearch::setServiceRemoved(m_intf->mainSettings, id, removed);
    emit providersChanged();
}

QVariantMap SubtitleFinder::serviceCredentials(const QString &id) const
{
    const SubtitleProviders::Credentials c = SubtitleSearch::credentials(m_intf->mainSettings, id);
    return { { QStringLiteral("key"), c.key }, { QStringLiteral("username"), c.username },
             { QStringLiteral("password"), c.password }, { QStringLiteral("email"), c.email } };
}

void SubtitleFinder::setServiceCredentials(const QString &id, const QVariantMap &credentials)
{
    if (!SubtitleProviders::provider(id))
        return;
    if (SubtitleSearch::setCredentials(m_intf->mainSettings, id, credentialsOf(credentials)))
        emit providersChanged();
}

void SubtitleFinder::checkService(const QString &id, const QVariantMap &credentials)
{
    const SubtitleProviders::Credentials typed = credentialsOf(credentials);
    if (!SubtitleProviders::provider(id) || !httpRequestSupported())
    {
        emit serviceChecked(id, QStringLiteral("noAnswer"), {});
        return;
    }
    SubtitleProviders::check(this, id, typed, [this, id, typed](SubtitleProviders::Check check, const QString &who) {
        using SubtitleProviders::Check;
        static const char *const names[] = { "works", "signedIn", "keyRefused", "loginRefused", "noAnswer", "busy" };
        msg_Dbg(m_intf, "online subtitles: %s: the credentials checked: %s", qtu(id), names[int(check)]);
        // working the first time: the service turns on by itself
        QSettings *settings = m_intf->mainSettings;
        if ((check == Check::Works || check == Check::SignedIn) && !SubtitleSearch::serviceSwitched(settings, id))
        {
            SubtitleSearch::setServiceEnabled(settings, id, true);
            emit providersChanged();
        }
        // (those kept: what is known of them follows)
        if (typed == SubtitleSearch::credentials(settings, id))
        {
            using SubtitleProviders::Problem;
            const Problem problem = check == Check::Works || check == Check::SignedIn ? Problem::None
                                  : check == Check::KeyRefused ? Problem::KeyRefused
                                  : check == Check::LoginRefused ? Problem::LoginRefused
                                  : check == Check::Busy ? Problem::Busy : Problem::Failed;
            SubtitleSearch::noteProblem(m_intf->mainSettings, id, problem);
            emit providersChanged();
        }
        emit serviceChecked(id, QString::fromLatin1(names[int(check)]), who);
    });
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
    // (only the answer of the last one asked is shown)
    const quint64 revision = ++m_titleRevision;
    m_search->findTitles(query, [this, revision](const QVariantList &titles) {
        if (revision != m_titleRevision)
            return;
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
    // (a download going on goes on: it is of this file all the same; a
    // title search still on its way is dropped)
    ++m_titleRevision;
    m_titleBusy = false;
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
    // cleaned up. Without VLC's cache folder, nowhere (never "/<folder>",
    // at the root of the drive)
    const QString root = SubtitleSearch::cacheRoot();
    if (root.isEmpty())
    {
        const QString error = qtr("The subtitle couldn't be saved.");
        m_search->setError(error);
        emit downloadFailed(error);
        downloadNext();
        return;
    }
    const QString folder = root + QLatin1Char('/') + SubtitleSearch::folderName(m_search->mediaKey());
    m_loadingId = id;
    emit resultsChanged();
    // (its answer dropped by the search once the media changed)
    m_search->download(id, folder, true,
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

