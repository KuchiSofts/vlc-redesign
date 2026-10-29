/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
/*****************************************************************************
 * subtitle_cues.cpp: The lines of the subtitle shown, drawn on the seek bar
 *****************************************************************************/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "subtitle_cues.hpp"
#include "subtitle_finder.hpp"
#include "../../../codec/subs_charset.h"

#include <vlc_configuration.h>
#include <vlc_input_item.h>
#include <vlc_player.h>
#include <vlc_url.h>

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QPainter>
#include <QPointer>
#include <QStringDecoder>
#include <QThread>
#include <algorithm>
#include <cstring>
#include <utility>

namespace {
// the films of an evening, both bars (the player's, the mini player's)
constexpr int maxCacheEntries = 8;
// a file added is in the media a moment after its track: asked again
constexpr int retryMs = 300;
constexpr int maxRetries = 30; // 9 s: the folder of a video on a slow drive is read on a worker thread

struct CacheEntry
{
    QString key;
    std::shared_ptr<const std::vector<subtitlecues::Cue>> cues;
};
// all in the GUI thread
QList<CacheEntry> s_cache;
QStringList s_reading;
QList<QPointer<QThread>> s_threads;
bool s_waitAllAtEnd = false;
QList<SubtitleCues *> s_instances;

QString decodeText(const std::string &text)
{
    if (text.empty())
        return {};
    QStringDecoder utf8(QStringDecoder::Utf8);
    const QString decoded = utf8(QByteArrayView(text.data(), qsizetype(text.size())));
    // (a file of an old code page: the one of the system)
    if (utf8.hasError())
        return QString::fromLocal8Bit(text.data(), qsizetype(text.size()));
    return decoded;
}
}

SubtitleCues::SubtitleCues(QQuickItem *parent) : QQuickPaintedItem(parent)
{
    s_instances.append(this);
    setAntialiasing(true);
    setOpaquePainting(false);
    m_resolveTimer.setSingleShot(true);
    connect(&m_resolveTimer, &QTimer::timeout, this, &SubtitleCues::resolve);
}

SubtitleCues::~SubtitleCues()
{
    s_instances.removeAll(this);
}

void SubtitleCues::setPlayer(PlayerController *player)
{
    if (m_player == player)
        return;
    if (m_player) {
        disconnect(m_player, nullptr, this, nullptr);
        disconnect(m_player->getSubtitleTracks(), nullptr, this, nullptr);
    }
    m_player = player;
    if (m_player) {
        TrackListModel *tracks = m_player->getSubtitleTracks();
        // the selection is the check state of the rows
        connect(tracks, &QAbstractItemModel::dataChanged, this, &SubtitleCues::scheduleResolve);
        connect(tracks, &QAbstractItemModel::modelReset, this, &SubtitleCues::scheduleResolve);
        connect(tracks, &QAbstractItemModel::rowsInserted, this, &SubtitleCues::scheduleResolve);
        connect(tracks, &QAbstractItemModel::rowsRemoved, this, &SubtitleCues::scheduleResolve);
        connect(m_player, &PlayerController::inputChanged, this, &SubtitleCues::scheduleResolve);
        connect(m_player, &PlayerController::lengthChanged, this, &SubtitleCues::readTimes);
        connect(m_player, &PlayerController::subtitleDelayChanged, this, &SubtitleCues::readTimes);
    }
    emit playerChanged();
    readTimes();
    scheduleResolve();
}

void SubtitleCues::setActive(bool active)
{
    if (m_active == active)
        return;
    m_active = active;
    emit activeChanged();
    scheduleResolve();
}

void SubtitleCues::setColor(const QColor &color)
{
    if (m_color == color)
        return;
    m_color = color;
    emit colorChanged();
    update();
}

void SubtitleCues::setMinimumWidth(qreal width)
{
    if (qFuzzyCompare(m_minimumWidth, width))
        return;
    m_minimumWidth = width;
    emit layoutChanged();
    update();
}

void SubtitleCues::setMergeDistance(qreal distance)
{
    if (qFuzzyCompare(m_mergeDistance, distance))
        return;
    m_mergeDistance = distance;
    emit layoutChanged();
    update();
}

void SubtitleCues::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);
    if (newGeometry.size() != oldGeometry.size())
        update();
}

void SubtitleCues::readTimes()
{
    const qint64 length = m_player ? qint64(m_player->getLength().toMilliseconds()) : 0;
    const qint64 delay = m_player ? qint64(m_player->getSubtitleDelay().toMilliseconds()) : 0;
    if (length == m_lengthMs && delay == m_delayMs)
        return;
    m_lengthMs = length;
    m_delayMs = delay;
    update();
}

void SubtitleCues::scheduleResolve()
{
    m_retries = 0;
    m_resolveTimer.start(0);
}

void SubtitleCues::setCues(std::shared_ptr<const CueList> cues)
{
    if (m_cues == cues)
        return;
    m_cues = std::move(cues);
    emit cuesChanged();
    update();
}

// The file of the subtitle track shown: VLC names the tracks of a file
// added to a media by the MD5 of its MRL ("<md5>/spu/<id>"). The subtitle
// finder knows the files VLC found next to the video and those added or
// downloaded (the media gives its slaves to the input when it starts, so
// its own list is empty while it plays; it is still looked at)
void SubtitleCues::resolve()
{
    QString path;
    int streamIndex = -1;
    bool pending = false;
    if (m_active && m_player) {
        vlc_player_t *player = m_player->getPlayer();
        vlc_player_locker lock{ player };
        const vlc_player_track *track = vlc_player_GetSelectedTrack(player, SPU_ES);
        const char *id = track ? vlc_es_id_GetStrId(track->es_id) : nullptr;
        const char *slash = id ? strchr(id, '/') : nullptr;
        if (slash && slash - id == 32) {
            const QByteArray hash(id, 32);
            const char *last = strrchr(id, '/');
            streamIndex = last && last[1] ? atoi(last + 1) : -1;
            pending = true;
            path = SubtitleFinder::externalPath(id);
            if (!path.isEmpty())
                pending = false;
            input_item_t *media = path.isEmpty() ? vlc_player_GetCurrentMedia(player) : nullptr;
            if (media) {
                vlc_mutex_lock(&media->lock);
                for (int i = 0; i < media->i_slaves; ++i) {
                    const input_item_slave_t *slave = media->pp_slaves[i];
                    if (!slave || slave->i_type != SLAVE_TYPE_SPU)
                        continue;
                    const QByteArray mrl(slave->psz_uri);
                    if (QCryptographicHash::hash(mrl, QCryptographicHash::Md5).toHex() != hash)
                        continue;
                    pending = false;
                    // (only local files: a subtitle of a server is not read again)
                    if (char *local = vlc_uri2path(slave->psz_uri)) {
                        path = qfu(local);
                        free(local);
                    }
                    break;
                }
                vlc_mutex_unlock(&media->lock);
            }
        }
    }
    if (pending && m_retries < maxRetries) {
        ++m_retries;
        m_resolveTimer.start(retryMs);
    }
    if (path.isEmpty()) {
        m_key.clear();
        setCues({});
        return;
    }
    load(path, streamIndex);
}

void SubtitleCues::load(const QString &path, int streamIndex)
{
    const QFileInfo file(path);
    if (!file.isFile() || !file.isReadable() || file.size() > qint64(subtitlecues::maxFileBytes)) {
        m_key.clear();
        setCues({});
        return;
    }
    // a file saved again (an editor, a new download) is read again
    const QString key = file.absoluteFilePath() + '|' + QString::number(file.size()) + '|'
                        + QString::number(file.lastModified().toMSecsSinceEpoch()) + '|'
                        + QString::number(streamIndex);
    if (key == m_key && (m_cues || s_reading.contains(key)))
        return;
    m_key = key;
    for (int i = 0; i < s_cache.size(); ++i) {
        if (s_cache[i].key != key)
            continue;
        const CacheEntry hit = s_cache.takeAt(i);
        s_cache.prepend(hit);
        setCues(hit.cues);
        return;
    }
    setCues({});
    if (s_reading.contains(key))
        return;
    // MicroDVD counts frames: at the rate set for it, else VLC's 25
    const float fps = config_FindConfig("sub-fps") ? config_GetFloat("sub-fps") : 0.f;
    startReading(key, path, streamIndex, fps > 0 ? fps : 25.0);
}

void SubtitleCues::startReading(const QString &key, const QString &path, int streamIndex, double fps)
{
    if (!std::exchange(s_waitAllAtEnd, true))
        qAddPostRoutine(waitAll);
    s_threads.removeIf([](const QPointer<QThread> &thread) { return thread.isNull(); });
    s_reading.append(key);

    // The encoding of the text, as the player finds it (subtitle demux):
    // the language in the file name ("movie.he.srt"), the preferred
    // subtitle languages, the code page of the interface language
    QList<QByteArray> langs;
    const QString name = QFileInfo(path).fileName();
    const qsizetype ext = name.lastIndexOf(QLatin1Char('.'));
    const qsizetype dot = ext > 0 ? name.lastIndexOf(QLatin1Char('.'), ext - 1) : -1;
    if (dot >= 0)
        langs << name.mid(dot + 1, ext - dot - 1).toUtf8();
    if (char *pref = config_GetPsz("sub-language")) {
        for (const QByteArray &lang : QByteArray(pref).split(','))
            if (!lang.trimmed().isEmpty() && langs.size() < 4)
                langs << lang.trimmed();
        free(pref);
    }
    const QByteArray uiCharset = vlc_pgettext("GetACP", "CP1252");

    QThread *thread = QThread::create([key, path, streamIndex, fps, langs, uiCharset] {
        const auto read = [&langs, &uiCharset](const QString &name) {
            QFile file(name);
            if (!file.open(QIODevice::ReadOnly) || file.size() > qint64(subtitlecues::maxFileBytes))
                return std::string();
            const QByteArray bytes = file.readAll();
            const char *hints[4];
            size_t count = 0;
            for (const QByteArray &lang : langs)
                if (count < 4)
                    hints[count++] = lang.constData();
            const subs_charset_t cs = subs_charset_Detect(bytes.constData(), size_t(bytes.size()),
                                                          hints, count, uiCharset.constData());
            size_t size = 0;
            char *utf8 = subs_charset_ToUTF8(bytes.constData() + cs.bom_size,
                                             size_t(bytes.size()) - cs.bom_size, cs.charset, &size);
            if (!utf8)
                return std::string(bytes.constData(), size_t(bytes.size()));
            std::string text(utf8, size);
            free(utf8);
            return text;
        };
        auto cues = std::make_shared<CueList>(subtitlecues::parse(read(path), fps, streamIndex));
        // a VobSub .sub (pictures) has its times in the .idx beside it
        if (cues->empty() && path.endsWith(QLatin1String(".sub"), Qt::CaseInsensitive)) {
            const QString index = path.left(path.size() - 4) + QLatin1String(".idx");
            if (QFileInfo::exists(index))
                *cues = subtitlecues::parse(read(index), fps, streamIndex);
        }
        std::shared_ptr<const CueList> result = std::move(cues);
        QMetaObject::invokeMethod(QCoreApplication::instance(), [key, result] {
            finishReading(key, result);
        }, Qt::QueuedConnection);
    });
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    s_threads.append(thread);
    thread->start(QThread::LowPriority);
}

void SubtitleCues::finishReading(const QString &key, const std::shared_ptr<const CueList> &cues)
{
    s_reading.removeAll(key);
    s_cache.prepend({ key, cues });
    while (s_cache.size() > maxCacheEntries)
        s_cache.removeLast();
    // (setCues() emits: a bar may go meanwhile)
    QList<QPointer<SubtitleCues>> waiting;
    for (SubtitleCues *item : std::as_const(s_instances))
        if (item->m_key == key)
            waiting << item;
    for (const QPointer<SubtitleCues> &item : std::as_const(waiting))
        if (item && item->m_key == key)
            item->setCues(cues);
}

// the readings end before the Qt module goes (each reads one file)
void SubtitleCues::waitAll()
{
    s_waitAllAtEnd = false;
    for (const QPointer<QThread> &thread : std::exchange(s_threads, {}))
        if (thread) {
            thread->wait();
            delete thread;
        }
}

QString SubtitleCues::textAt(qreal position) const
{
    if (!m_cues || m_cues->empty() || m_lengthMs <= 0)
        return {};
    const qint64 time = qint64(position * m_lengthMs) - m_delayMs;
    // the last line started by then that still shows
    auto it = std::upper_bound(m_cues->begin(), m_cues->end(), time,
                               [](qint64 t, const subtitlecues::Cue &cue) { return t < cue.startMs; });
    for (int checked = 0; it != m_cues->begin() && checked < 64; ++checked) {
        --it;
        if (it->endMs >= time)
            return decodeText(it->text);
    }
    return {};
}

void SubtitleCues::paint(QPainter *painter)
{
    const qreal w = width();
    const qreal h = height();
    if (!m_active || !m_cues || m_cues->empty() || m_lengthMs <= 0 || w <= 0 || h <= 0)
        return;
    const qreal scale = w / qreal(m_lengthMs);
    const qreal minimum = std::min(std::max<qreal>(m_minimumWidth, 0.5), w);
    QList<QRectF> marks;
    qreal runStart = -1;
    qreal runEnd = -1;
    for (const subtitlecues::Cue &cue : *m_cues) {
        qreal start = (cue.startMs + m_delayMs) * scale;
        qreal end = (cue.endMs + m_delayMs) * scale;
        if (end < 0 || start > w)
            continue;
        start = std::max<qreal>(0, start);
        end = std::max(end, start + minimum);
        if (end > w) {
            end = w;
            start = std::min(start, w - minimum);
        }
        if (runEnd >= 0 && start - runEnd <= m_mergeDistance) {
            runEnd = std::max(runEnd, end);
            continue;
        }
        if (runEnd >= 0)
            marks.append(QRectF(runStart, 0, runEnd - runStart, h));
        runStart = start;
        runEnd = end;
    }
    if (runEnd >= 0)
        marks.append(QRectF(runStart, 0, runEnd - runStart, h));
    painter->setPen(Qt::NoPen);
    painter->setBrush(m_color);
    painter->drawRects(marks.constData(), int(marks.size()));
}
