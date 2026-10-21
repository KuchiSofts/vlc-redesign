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

#include "thumbnail_image_provider.hpp"

#include <vlc_common.h>
#include <vlc_image.h>
#include <vlc_input_item.h>
#include <vlc_media_library.h>
#include <vlc_picture.h>
#include <vlc_preparser.h>

#include <QElapsedTimer>
#include <QHash>
#include <QImage>
#include <QImageReader>
#include <QUrl>
#include <QUrlQuery>

#include <algorithm>
#include <vector>

namespace {

#define URL_KEY "url"
#define POSITION_KEY "pos"
#define ARTWORK_KEY "art"
#define SEEK_KEY "seek"

// The MRL is base64 encoded: kept as is, its percent encoding included
constexpr auto mrlEncoding = QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals;

QString encodeMrl(const QString &mrl)
{
    return QString::fromLatin1(mrl.toUtf8().toBase64(mrlEncoding));
}

QString decodeMrl(const QString &value)
{
    const auto result = QByteArray::fromBase64Encoding(value.toLatin1(), mrlEncoding);
    return result ? QString::fromUtf8(*result) : QString();
}

// Frames are small: decoding them in software keeps the GPU for the video
constexpr int defaultWidth = 320;

QImage toImage(vlc_object_t *obj, picture_t *picture, const QSize &requestedSize)
{
    video_format_t fmtIn = picture->format;
    if (fmtIn.i_sar_num == 0 || fmtIn.i_sar_den == 0)
        fmtIn.i_sar_num = fmtIn.i_sar_den = 1;
    if (fmtIn.i_visible_width == 0 || fmtIn.i_visible_height == 0)
        return {};

    // Display size, then fit in the requested size
    QSize size(fmtIn.i_visible_width * fmtIn.i_sar_num / fmtIn.i_sar_den,
               fmtIn.i_visible_height);
    QSize bound(requestedSize.width() > 0 ? requestedSize.width() : defaultWidth,
                requestedSize.height() > 0 ? requestedSize.height() : INT_MAX);
    size.scale(bound, Qt::KeepAspectRatio);
    if (size.isEmpty())
        return {};

    video_format_t fmtOut;
    video_format_Init(&fmtOut, VLC_CODEC_RGBA);
    fmtOut.i_width = fmtOut.i_visible_width = size.width();
    fmtOut.i_height = fmtOut.i_visible_height = size.height();
    fmtOut.i_sar_num = fmtOut.i_sar_den = 1;

    image_handler_t *handler = image_HandlerCreate(obj);
    if (!handler)
        return {};
    picture_t *converted = image_Convert(handler, picture, &fmtIn, &fmtOut);
    image_HandlerDelete(handler);
    if (!converted)
        return {};

    QImage image = QImage(converted->p[0].p_pixels,
                          converted->format.i_visible_width,
                          converted->format.i_visible_height,
                          converted->p[0].i_pitch,
                          QImage::Format_RGBA8888).copy();
    picture_Release(converted);
    return image;
}

} // anonymous namespace

// The frame a response waits for: filled on any thread, read by Qt
struct ThumbnailResponseState
{
    std::recursive_mutex lock;
    QQuickImageResponse *response = nullptr;  // null once the response is gone
    bool delivered = false;
    QImage image;
    QString error;

    void deliver(const QImage &frame)
    {
        std::lock_guard<std::recursive_mutex> guard(lock);
        if (!response || delivered)
            return;
        delivered = true;
        image = frame;
        if (frame.isNull())
            error = QStringLiteral("No frame decoded");
        // queued: the response may still be in its constructor
        QMetaObject::invokeMethod(response, &QQuickImageResponse::finished, Qt::QueuedConnection);
    }
};

// Frames decoded by the thumbnailer, kept for the next requests. A request
// is never cancelled: a frame the pointer has left behind still goes to the
// cache, where a sweep back finds it. The frames of the seek bar are on a
// grid, and while it is asked for, the frames around the pointer are
// decoded ahead, one at a time. A media whose frames take long to decode
// (4K HEVC: the thumbnailer decodes in software) gets no decoding ahead and
// one frame at a time, so that playback keeps the processor.
class ThumbnailFrameCache : public std::enable_shared_from_this<ThumbnailFrameCache>
{
public:
    // The seek bar's frames: multiples of 1/gridSteps of the length (the
    // seek goes to the nearest key frame anyway)
    static constexpr int gridSteps = 200;

    explicit ThumbnailFrameCache(vlc_object_t *obj)
        : m_obj(obj)
    {
        m_clock.start();
    }

    ~ThumbnailFrameCache()
    {
        if (m_preparser)
            vlc_preparser_Delete(m_preparser);
    }

    // On the owner's thread, before its reference goes: ends the requests,
    // whose callbacks can then hold the last reference
    void shutdown()
    {
        vlc_preparser_t *preparser;
        {
            std::lock_guard<std::mutex> guard(m_lock);
            preparser = m_preparser;
            m_preparser = nullptr;
            m_closed = true;
        }
        if (preparser)
            vlc_preparser_Delete(preparser);
    }

    // The frame of @mrl at @pos (0 to 1), @width pixels wide, for @state;
    // @seek: a frame of the seek bar, on the grid
    void request(const QString &mrl, double pos, int width, bool seek,
                 const std::shared_ptr<ThumbnailResponseState> &state)
    {
        Outbox outbox;
        {
            std::lock_guard<std::mutex> guard(m_lock);
            if (!preparserLocked())
                outbox.emplace_back(state, QImage());
            else
            {
                if (seek)
                {
                    m_seekMrl = mrl;
                    m_seekWidth = width;
                    m_seekCenter = qRound(pos * gridSteps);
                    m_lastSeek = m_clock.elapsed();
                }
                const QString k = key(mrl, pos, width);
                Entry &entry = m_entries[k];
                if (entry.mrl.isEmpty())
                {
                    entry.mrl = mrl;
                    entry.pos = pos;
                    entry.width = width;
                }
                entry.lastUse = ++m_uses;
                if (entry.state == Entry::Done || entry.state == Entry::Failed)
                    outbox.emplace_back(state, entry.image);
                else
                {
                    entry.waiters.push_back(state);
                    if (entry.state == Entry::Idle)
                        m_wanted = k;
                }
                pumpLocked(outbox);
            }
        }
        send(outbox);
    }

    // No frame for @state any more (its decoding goes on, for the cache)
    void forget(const std::shared_ptr<ThumbnailResponseState> &state)
    {
        std::lock_guard<std::mutex> guard(m_lock);
        for (Entry &entry : m_entries)
            entry.waiters.erase(std::remove(entry.waiters.begin(), entry.waiters.end(), state),
                                entry.waiters.end());
    }

private:
    struct Entry
    {
        enum State { Idle, Running, Done, Failed } state = Idle;
        QString mrl;
        double pos = 0;
        int width = 0;
        bool ahead = false;
        qint64 started = 0;
        QImage image;
        std::vector<std::shared_ptr<ThumbnailResponseState>> waiters;
        quint64 lastUse = 0;
    };
    using Outbox = std::vector<std::pair<std::shared_ptr<ThumbnailResponseState>, QImage>>;

    struct CallbackData
    {
        std::weak_ptr<ThumbnailFrameCache> cache;
        QString key;
        int width;
    };

    // Frames decoding at once, at most one of them ahead of the pointer
    static constexpr int maxRunning = 2;
    // Frames kept: the grid of a media and some more (about 30 MB at 280 x 158)
    static constexpr int maxFrames = 240;
    // Decoding ahead stops when the seek bar was not asked for so long
    static constexpr qint64 aheadIdleMs = 4000;
    // A frame of the seek bar taking longer on average makes its media heavy
    static constexpr qint64 heavyMs = 150;

    bool heavyLocked() const
    {
        return !m_seekMrl.isEmpty() && m_timedMrl == m_seekMrl && m_averageMs > heavyMs;
    }
    int maxRunningLocked() const { return heavyLocked() ? 1 : maxRunning; }

    static QString key(const QString &mrl, double pos, int width)
    {
        return mrl + QLatin1Char('|') + QString::number(qRound(pos * 1000)) + QLatin1Char('|')
               + QString::number(width);
    }

    static void send(const Outbox &outbox)
    {
        for (const auto &delivery : outbox)
            delivery.first->deliver(delivery.second);
    }

    bool preparserLocked()
    {
        if (m_preparser || m_closed)
            return m_preparser != nullptr;
        struct vlc_preparser_cfg cfg = {};
        cfg.types = VLC_PREPARSER_TYPE_THUMBNAIL;
        cfg.max_thumbnailer_threads = maxRunning;
        cfg.timeout = VLC_TICK_FROM_SEC(10);
        cfg.external_process = false;
        m_preparser = vlc_preparser_New(m_obj, &cfg);
        return m_preparser != nullptr;
    }

    void submitLocked(const QString &k, Entry &entry, bool ahead, Outbox &outbox)
    {
        vlc_preparser_req *request = nullptr;
        auto *data = new CallbackData { weak_from_this(), k, entry.width };
        if (input_item_t *item = input_item_New(qtu(entry.mrl), nullptr))
        {
            struct vlc_thumbnailer_arg arg = {};
            arg.seek.type = vlc_thumbnailer_arg::seek::VLC_THUMBNAILER_SEEK_POS;
            arg.seek.pos = entry.pos;
            arg.seek.speed = vlc_thumbnailer_arg::seek::VLC_THUMBNAILER_SEEK_FAST;
            arg.hw_dec = false;

            static const struct vlc_thumbnailer_cbs cbs = { &ThumbnailFrameCache::onEnded };
            request = vlc_preparser_req_NewThumbnail(m_preparser, item, &arg, &cbs, data);
            input_item_Release(item);
        }
        if (request && vlc_preparser_Submit(m_preparser, request) == VLC_SUCCESS)
        {
            entry.state = Entry::Running;
            entry.ahead = ahead;
            entry.started = m_clock.elapsed();
            ++m_running;
            if (ahead)
                ++m_ahead;
            return;
        }
        if (request)
            vlc_preparser_req_Release(request);
        delete data;
        entry.state = Entry::Failed;
        for (const auto &waiter : entry.waiters)
            outbox.emplace_back(waiter, QImage());
        entry.waiters.clear();
    }

    // Starts what can run: the frame asked last, other frames waited for,
    // then one ahead of the pointer
    void pumpLocked(Outbox &outbox)
    {
        if (!m_preparser)
            return;
        const int limit = maxRunningLocked();
        if (!m_wanted.isEmpty() && m_running < limit)
        {
            const auto it = m_entries.find(m_wanted);
            if (it != m_entries.end() && it->state == Entry::Idle)
                submitLocked(it.key(), *it, false, outbox);
            m_wanted.clear();
        }
        for (auto it = m_entries.begin(); it != m_entries.end() && m_running < limit; ++it)
            if (it->state == Entry::Idle && !it->waiters.empty())
                submitLocked(it.key(), *it, false, outbox);

        if (m_running >= limit || m_ahead > 0 || m_seekMrl.isEmpty() || heavyLocked()
            || m_clock.elapsed() - m_lastSeek > aheadIdleMs)
            return;
        // the nearest frame of the grid not decoded yet
        const auto tryStep = [&](int step) {
            if (step < 0 || step > gridSteps)
                return false;
            const double pos = double(step) / gridSteps;
            const QString k = key(m_seekMrl, pos, m_seekWidth);
            const auto it = m_entries.find(k);
            if (it != m_entries.end() && it->state != Entry::Idle)
                return false;
            if (it == m_entries.end() && m_entries.size() >= maxFrames)
                return false;
            Entry &entry = m_entries[k];
            entry.mrl = m_seekMrl;
            entry.pos = pos;
            entry.width = m_seekWidth;
            entry.lastUse = ++m_uses;
            submitLocked(k, entry, true, outbox);
            return true;
        };
        for (int d = 0; d <= gridSteps; ++d)
            if (tryStep(m_seekCenter + d) || (d > 0 && tryStep(m_seekCenter - d)))
                break;
    }

    // The least recently asked frames go, never one decoding or waited for
    void evictLocked()
    {
        while (m_entries.size() > maxFrames)
        {
            auto oldest = m_entries.end();
            for (auto it = m_entries.begin(); it != m_entries.end(); ++it)
                if (it->state != Entry::Running && it->waiters.empty()
                    && (oldest == m_entries.end() || it->lastUse < oldest->lastUse))
                    oldest = it;
            if (oldest == m_entries.end())
                break;
            m_entries.erase(oldest);
        }
    }

    void finished(const QString &k, int status, const QImage &image)
    {
        Outbox outbox;
        {
            std::lock_guard<std::mutex> guard(m_lock);
            --m_running;
            const auto it = m_entries.find(k);
            if (it != m_entries.end() && it->state == Entry::Running)
            {
                if (it->ahead)
                    --m_ahead;
                // how long the frames of this media take (an average over the last few)
                if (!image.isNull())
                {
                    const qint64 ms = m_clock.elapsed() - it->started;
                    if (m_timedMrl != it->mrl)
                    {
                        m_timedMrl = it->mrl;
                        m_averageMs = ms;
                    }
                    else
                        m_averageMs = (3 * m_averageMs + ms) / 4;
                }
                if (image.isNull() && status == -EINTR)
                    it->state = Entry::Idle;  // closing
                else
                {
                    it->state = image.isNull() ? Entry::Failed : Entry::Done;
                    it->image = image;
                }
                for (const auto &waiter : it->waiters)
                    outbox.emplace_back(waiter, image);
                it->waiters.clear();
            }
            evictLocked();
            pumpLocked(outbox);
        }
        send(outbox);
    }

    static void onEnded(vlc_preparser_req *request, int status, picture_t *thumbnail, void *opaque)
    {
        std::unique_ptr<CallbackData> data(static_cast<CallbackData *>(opaque));
        if (const std::shared_ptr<ThumbnailFrameCache> self = data->cache.lock())
        {
            QImage image;
            if (thumbnail)
                image = toImage(self->m_obj, thumbnail, QSize(data->width, 0))
                            .convertToFormat(QImage::Format_RGB888);
            self->finished(data->key, status, image);
        }
        vlc_preparser_req_Release(request);
    }

    vlc_object_t *m_obj;
    std::mutex m_lock;
    vlc_preparser_t *m_preparser = nullptr;
    bool m_closed = false;
    QHash<QString, Entry> m_entries;
    QString m_wanted;
    int m_running = 0;
    int m_ahead = 0;
    quint64 m_uses = 0;
    QString m_seekMrl;
    int m_seekWidth = 0;
    int m_seekCenter = 0;
    qint64 m_lastSeek = 0;
    QString m_timedMrl;
    qint64 m_averageMs = 0;
    QElapsedTimer m_clock;
};

namespace {

class FrameResponse : public QQuickImageResponse
{
public:
    FrameResponse(const std::shared_ptr<ThumbnailFrameCache> &cache, const QString &mrl, double pos,
                  int width, bool seek)
        : m_cache(cache)
        , m_state(std::make_shared<ThumbnailResponseState>())
    {
        m_state->response = this;
        if (cache && !mrl.isEmpty())
            cache->request(mrl, pos, width, seek, m_state);
        else
            m_state->deliver(QImage());
    }

    ~FrameResponse() override
    {
        {
            std::lock_guard<std::recursive_mutex> guard(m_state->lock);
            m_state->response = nullptr;
        }
        if (const auto cache = m_cache.lock())
            cache->forget(m_state);
    }

    QQuickTextureFactory *textureFactory() const override
    {
        std::lock_guard<std::recursive_mutex> guard(m_state->lock);
        return m_state->image.isNull() ? nullptr
                                       : QQuickTextureFactory::textureFactoryForImage(m_state->image);
    }

    QString errorString() const override
    {
        std::lock_guard<std::recursive_mutex> guard(m_state->lock);
        return m_state->error;
    }

    // The frame still goes to the cache; the engine needs finished() anyway
    void cancel() override
    {
        if (const auto cache = m_cache.lock())
            cache->forget(m_state);
        m_state->deliver(QImage());
    }

private:
    std::weak_ptr<ThumbnailFrameCache> m_cache;
    std::shared_ptr<ThumbnailResponseState> m_state;
};

// Artwork of the media library: looked up and read on the thread of the
// image loader, the response is ready when created
class ArtworkResponse : public QQuickImageResponse
{
public:
    ArtworkResponse(vlc_object_t *obj, const QString &mrl, const QSize &requestedSize)
    {
        const QString path = artworkPath(obj, mrl);
        if (!path.isEmpty())
        {
            QImageReader reader(path);
            reader.setAutoTransform(true);
            const QSize size = reader.size();
            if (size.isValid() && (requestedSize.width() > 0 || requestedSize.height() > 0))
            {
                const QSize bound(requestedSize.width() > 0 ? requestedSize.width() : INT_MAX,
                                  requestedSize.height() > 0 ? requestedSize.height() : INT_MAX);
                if (size.width() > bound.width() || size.height() > bound.height())
                    reader.setScaledSize(size.scaled(bound, Qt::KeepAspectRatio));
            }
            m_image = reader.read();
        }
        if (m_image.isNull())
            m_error = QStringLiteral("No artwork for %1").arg(mrl);
        QMetaObject::invokeMethod(this, &QQuickImageResponse::finished, Qt::QueuedConnection);
    }

    QQuickTextureFactory *textureFactory() const override
    {
        return m_image.isNull() ? nullptr : QQuickTextureFactory::textureFactoryForImage(m_image);
    }

    QString errorString() const override { return m_error; }

private:
    static QString localPath(const vlc_ml_thumbnail_t &thumbnail)
    {
        if (thumbnail.i_status != VLC_ML_THUMBNAIL_STATUS_AVAILABLE || !thumbnail.psz_mrl)
            return {};
        const QUrl url(QString::fromUtf8(thumbnail.psz_mrl));
        return url.isLocalFile() ? url.toLocalFile() : QString();
    }

    static QString artworkPath(vlc_object_t *obj, const QString &mrl)
    {
        vlc_medialibrary_t *ml = vlc_ml_instance_get(obj);
        if (!ml || mrl.isEmpty())
            return {};
        vlc_ml_media_t *media = vlc_ml_get_media_by_mrl(ml, qtu(mrl));
        if (!media)
            return {};
        QString path = localPath(media->thumbnails[VLC_ML_THUMBNAIL_SMALL]);
        if (path.isEmpty() && media->i_subtype == VLC_ML_MEDIA_SUBTYPE_ALBUMTRACK
            && media->album_track.i_album_id > 0)
        {
            if (vlc_ml_album_t *album = vlc_ml_get_album(ml, media->album_track.i_album_id))
            {
                path = localPath(album->thumbnails[VLC_ML_THUMBNAIL_SMALL]);
                vlc_ml_release(album);
            }
        }
        vlc_ml_release(media);
        return path;
    }

    QImage m_image;
    QString m_error;
};

} // anonymous namespace

ThumbnailImageProvider::ThumbnailImageProvider(qt_intf_t *intf)
    : m_intf(intf)
{
}

ThumbnailImageProvider::~ThumbnailImageProvider()
{
    if (m_cache)
        m_cache->shutdown();
}

std::shared_ptr<ThumbnailFrameCache> ThumbnailImageProvider::cache()
{
    std::lock_guard<std::mutex> guard(m_lock);
    if (!m_cache)
        m_cache = std::make_shared<ThumbnailFrameCache>(VLC_OBJECT(m_intf));
    return m_cache;
}

QQuickImageResponse *ThumbnailImageProvider::requestImageResponse(const QString &id,
                                                                  const QSize &requestedSize)
{
    const QUrlQuery query { QUrl(id) };
    const QString mrl = decodeMrl(query.queryItemValue(QStringLiteral(URL_KEY)));
    if (query.hasQueryItem(QStringLiteral(ARTWORK_KEY)))
        return new ArtworkResponse(VLC_OBJECT(m_intf), mrl, requestedSize);
    bool ok = false;
    const double position = query.queryItemValue(QStringLiteral(POSITION_KEY)).toDouble(&ok);
    const bool seek = query.hasQueryItem(QStringLiteral(SEEK_KEY));
    const int width = requestedSize.width() > 0 ? requestedSize.width() : defaultWidth;

    return new FrameResponse(mrl.isEmpty() || !ok ? nullptr : cache(), mrl,
                             qBound(0.0, position, 1.0), width, seek);
}

QString ThumbnailImageProvider::uri(const QString &mrl, double position)
{
    QUrlQuery query;
    query.addQueryItem(QStringLiteral(URL_KEY), encodeMrl(mrl));
    query.addQueryItem(QStringLiteral(POSITION_KEY),
                       QString::number(qRound(qBound(0.0, position, 1.0) * 1000) / 1000.0));
    return QStringLiteral("image://%1/?").arg(QLatin1String(providerId))
           + query.toString(QUrl::FullyEncoded);
}

QString ThumbnailImageProvider::previewUri(const QString &mrl, double position)
{
    const int steps = ThumbnailFrameCache::gridSteps;
    QUrlQuery query;
    query.addQueryItem(QStringLiteral(URL_KEY), encodeMrl(mrl));
    query.addQueryItem(QStringLiteral(POSITION_KEY),
                       QString::number(qRound(qBound(0.0, position, 1.0) * steps) / double(steps)));
    query.addQueryItem(QStringLiteral(SEEK_KEY), QStringLiteral("1"));
    return QStringLiteral("image://%1/?").arg(QLatin1String(providerId))
           + query.toString(QUrl::FullyEncoded);
}

QString ThumbnailImageProvider::artworkUri(const QString &mrl)
{
    QUrlQuery query;
    query.addQueryItem(QStringLiteral(URL_KEY), encodeMrl(mrl));
    query.addQueryItem(QStringLiteral(ARTWORK_KEY), QStringLiteral("1"));
    return QStringLiteral("image://%1/?").arg(QLatin1String(providerId))
           + query.toString(QUrl::FullyEncoded);
}

QString VLCThumbnail::artwork(const QString &mrl) const
{
    if (mrl.isEmpty())
        return {};
    return ThumbnailImageProvider::artworkUri(mrl);
}

QString VLCThumbnail::uri(const QString &mrl, double position) const
{
    if (mrl.isEmpty())
        return {};
    return ThumbnailImageProvider::uri(mrl, position);
}

QString VLCThumbnail::preview(const QString &mrl, double position) const
{
    if (mrl.isEmpty())
        return {};
    return ThumbnailImageProvider::previewUri(mrl, position);
}
