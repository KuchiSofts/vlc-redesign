/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
#pragma once

#include "autoguide.hpp"
#include "guide.hpp"
#include "playlist.hpp"

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QPair>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVector>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

struct vlc_object_t;
struct vlc_iptv;
struct vlc_interrupt;

namespace iptv {

// A request of the API of a server of the user (its own Jellyfin,
// Tvheadend...): any method, headers, a body, the user it asks for
struct HttpCall
{
    QByteArray method = "GET";
    QString url;
    QList<QPair<QByteArray, QByteArray>> headers;
    QByteArray body;
    QString username;
    QString password;
    int timeoutMs = 15000;
    qint64 maxBytes = 64 << 20;
};
struct HttpReply
{
    int status = 0;     // 0: no answer
    QByteArray body;
    QString error;
};
// What answered a broadcast on the network: its address, and what it said
struct Datagram
{
    QString from;
    QByteArray data;
};

// The most a download keeps (a link that is a stream, not a file, would fill
// the disk otherwise): a playlist, a guide (XMLTV, gzip or not), a logo
constexpr qint64 kPlaylistMaxBytes = 512LL << 20;
constexpr qint64 kGuideMaxBytes = 2LL << 30;
constexpr qint64 kLogoMaxBytes = 2LL << 20;
// How long a download waits for its next bytes before it stops ("no answer"):
// a playlist, a guide, an API's answer; a logo
constexpr int kIdleSecs = 30;
constexpr int kLogoIdleSecs = 15;

// What VLC gives the service: downloads through its own access modules
// (the proxy and the network settings of VLC), its keystore, its folders.
// Implemented in iptv.cpp, the glue of the module.
struct Host
{
    virtual ~Host() = default;
    // Saves the resource at url (http(s), file...) to path, as it is (a
    // gzip-compressed guide stays so); false and why on an error, when it
    // is larger than limit bytes, cut short, stopped, or when nothing came
    // for idleSecs ("no answer"; path then keeps what it had). progress gets
    // the bytes received and the size when known (0).
    virtual bool download(const QString& url, const QString& path, QString* error, qint64 limit, int idleSecs,
                          const std::function<void(qint64, qint64)>& progress = {}) = 0;
    // A small answer (an Xtream Codes API call) read into memory; empty and
    // why on an error, or when nothing came for idleSecs ("no answer")
    virtual QByteArray fetch(const QString& url, QString* error, qint64 limit = 256 << 20,
                             int idleSecs = kIdleSecs) = 0;
    // The secrets of a source (its link, its password) in VLC's keystore;
    // false when there is no keystore
    virtual bool storeSecret(const QString& id, const QByteArray& secret) = 0;
    virtual QByteArray loadSecret(const QString& id, bool* found) = 0;
    virtual void removeSecret(const QString& id) = 0;
    // A change for the interface (from any thread)
    virtual void notify(const QJsonObject& event) = 0;
    virtual void log(const QString& message) = 0;
    // What took how long (VLC's debug messages)
    virtual void debug(const QString& message) = 0;
    // The worker thread of the service: its downloads stop when killed
    virtual void enterWorker() = 0;
    virtual void killWorker() = 0;
    // A request of a server's API (blocking, any thread)
    virtual HttpReply call(const HttpCall& request) = 0;
    // A UDP broadcast on the local network (or to a multicast group:
    // address), and the answers of the next ms milliseconds (the discovery
    // of the user's tuners and servers)
    virtual QVector<Datagram> broadcast(quint16 port, const QByteArray& packet, int ms,
                                        const QString& address = QString()) = 0;
    // A random id of this copy of VLC, kept (what servers name a client by)
    virtual QString deviceId() = 0;
};

// A source of channels: an M3U playlist (a link or a file) or an Xtream
// Codes account, with the guides (XMLTV) of its channels
struct SourceConfig
{
    QString id;
    QString name;
    // "m3u", "xtream", or a server of the user: "jellyfin", "emby",
    // "tvheadend", "nextpvr", "channels" (Channels DVR), "hdhomerun"; or a
    // guide alone, for the channels of every source: "schedulesdirect"
    QString kind;
    QString url;             // the playlist (secret: often holds a token)
    QStringList epg;         // the guides the user added
    QString server;          // Xtream Codes, the servers: its address
    QString username;        // (also for an M3U behind a login)
    QString password;        // (secret; NextPVR: its PIN)
    QString key;             // an API key (Jellyfin, Emby) (secret)
    QString catalogId;       // added from the catalog
    // (in memory) the user's changes of the automatic guide: one it was
    // looking for meanwhile does not undo them
    quint32 autoEdits = 0;
    // (in memory) its secrets are in VLC's keystore, not in the file (the
    // file's "keystore"; a new source's go there unless there is none)
    bool inKeystore = true;
    bool enabled = true;
    bool useHeaderGuides = true;   // the guides the playlist names itself
    QStringList epgOff;      // the guides turned off (of epg, the playlist's, autoEpg)
    // the automatic guide: the guides it attached, the iptv-org id of each
    // channel it matched (by identityOf), when it last looked; turned off
    // when the user removed it (not again by itself)
    QStringList autoEpg;
    QHash<QString, autoguide::Match> autoIds;
    qint64 autoChecked = 0;
    bool autoGuideOff = false;
    int autoFound = 0;       // the channels with a guide after it, of
    int autoTotal = 0;
    // the guide id the user gave a channel (by identityOf): it wins
    QHash<QString, QString> guideIds;
};

// A programme kept in memory; its description stays on disk (descOffset in
// the file of its guide), a guide of 200 MB would not fit otherwise
struct Entry
{
    qint64 start = 0;
    qint64 stop = 0;
    QString title;
    QString subtitle;
    QString category;
    QString episode;
    qint64 descOffset = -1;
    int descLength = 0;
};
using EntryList = QVector<Entry>;

struct GuideData
{
    QString url;
    QString sourceId;
    QString descPath;
    QHash<QString, std::shared_ptr<const EntryList>> byKey;
    QHash<QString, QString> idByName;
    QSet<QString> languages;
    qint64 updated = 0;      // when it was downloaded
    QByteArray signature;    // the channels and days it was read for
    int programmes = 0;
    QString error;
};

// A source refreshed by itself: after its refresh period, or, when its last
// refreshes failed (offline, a password refused), 15 minutes after the
// last, then 30, 60... up to the period (never a sign-in every minute).
// refreshHours 0: never.
qint64 retryDelaySecs(int failures, int refreshHours);
bool refreshDue(qint64 now, qint64 playlistUpdated, qint64 lastAttempt, int failures, int refreshHours);

// The programmes of a list (sorted by start) on between from and to: the
// ones that started before from and still run (a long one before shorter
// ones too), then those starting before to; their indexes
QVector<int> programmesBetween(const EntryList& list, qint64 from, qint64 to);

class Service
{
public:
    Service(Host& host, const QString& dataDir, const QString& cacheDir,
            const QByteArray& bundledCatalog);
    ~Service();

    // The answer of a request of the interface (see vlc_iptv.h)
    QJsonObject request(const QJsonObject& request);
    // VLC quits: the worker stops at once, its downloads and the calls of
    // the servers too (any thread, never blocks)
    void stop();
    // Stops the worker and waits for it, at most ms: false when it is still
    // in a download VLC could not stop (this service must then never be
    // destroyed: the worker uses it)
    bool waitStopped(int ms);

private:
    struct SourceState
    {
        QVector<Channel> channels;
        QStringList headerGuides;
        qint64 playlistUpdated = 0;   // the date of the cached playlist
        QString error;
        QString stage;                // "playlist", "guide", "" when idle
        qint64 received = 0;
        qint64 total = 0;
        bool busy = false;
        // Xtream Codes: what the account has
        bool hasVod = false;
        bool hasSeries = false;
        qint64 expires = 0;
        // its server refused the user's sign-in (shown on its row)
        bool refused = false;
        // its last refresh, and the ones that failed since one worked
        qint64 lastAttempt = 0;
        int failures = 0;
    };
    struct ChannelEntry
    {
        QString key;
        QString sourceId;
        Channel channel;
        std::shared_ptr<const EntryList> guide;
        QString guideDescPath;
    };
    using Job = std::function<void()>;

    // configuration
    void loadConfig();
    // (under m_lock) the file; the keystore is not touched (storeSecrets)
    void saveConfig();
    // The secrets of the sources kept in VLC's keystore, read on the worker
    // as it starts (a locked keyring's prompt froze the interface when they
    // were read as the module loaded)
    void loadSecrets();
    // The secrets of one source changed, to the keystore: out of m_lock (a
    // keystore may ask the user), one source at a time (m_secretLock)
    void storeSecrets(const QString& id);
    // (with m_lock held) waits for loadSecrets, 10 s at most: false when they
    // are still not read (what reads or changes a secret then fails)
    bool secretsReady(std::unique_lock<std::mutex>& guard);
    QJsonObject sourceToJson(const SourceConfig& s, bool withSecrets) const;
    SourceConfig sourceFromJson(const QJsonObject& o) const;
    SourceConfig* findSource(const QString& id);
    QString sourceDir(const QString& id) const;

    // the worker
    void post(Job job);
    void run();
    // a job threw (out of memory): logged, its sources no longer busy
    void jobFailed(const char* what) noexcept;
    void startup();
    void schedule();
    void refreshSources(const QStringList& ids, bool guides);
    bool loadPlaylist(const SourceConfig& cfg, bool download, SourceState& state);
    // a server of the user: its channels and guide written as the source's
    // playlist.m3u and guide.xml (false and why when it can't be asked)
    bool downloadServer(const SourceConfig& cfg, SourceState& state);
    // Schedules Direct: the guide of the stations of the user's lineups,
    // written as the source's guide.xml (a guide alone: no channels)
    bool downloadSchedulesDirect(const SourceConfig& cfg, SourceState& state);
    // Test, Sign in: one request with the fields typed
    QJsonObject testSource(const QJsonObject& request);
    // the tuners and servers found on the local network
    QJsonObject discover();
    // a channel of Jellyfin or Emby: its stream, opened when it plays
    QString openServerStream(const QString& sourceId, const QString& channelUrl, QString* error);
    // the automatic guide of a source (automatic: after an update, only
    // when its guides miss channels and it did not look this week)
    void findGuide(const QString& id, bool automatic);
    // iptv-org's channels (downloaded once a week, the fields kept)
    QVector<autoguide::Known> knownChannels(QString* error);
    // signed in to Jellyfin or Emby (kept for this run)
    bool mediaBrowserSession(const SourceConfig& cfg, QString* token, QString* userId, QString* error,
                             bool* refused);
    void loadGuides(bool download, const QStringList& onlySources);
    void rebuild();
    void updateCatalog(bool force);
    void progress(const QString& id, const QString& stage, qint64 received, qint64 total);
    void changed(const char* what);

    // answers
    QJsonObject status();
    QJsonObject channels();
    QJsonObject programmes(const QJsonObject& request);
    QJsonObject nowNext(const QJsonObject& request);
    QJsonObject description(const QJsonObject& request);
    QJsonObject catchup(const QJsonObject& request);
    QJsonObject catalog();
    QJsonObject vod(const QJsonObject& request);
    QJsonObject seriesInfo(const QJsonObject& request);
    QJsonObject logo(const QJsonObject& request);

    QJsonObject entryJson(const ChannelEntry& c, const Entry& e) const;

    Host& m_host;
    const QString m_dataDir;
    const QString m_cacheDir;
    const QByteArray m_bundledCatalog;

    // m_lock: everything below, between the worker and the requests
    mutable std::mutex m_lock;
    QVector<SourceConfig> m_sources;
    QHash<QString, SourceState> m_state;
    QHash<QString, std::shared_ptr<GuideData>> m_guides;   // by url
    QVector<ChannelEntry> m_channels;
    QHash<QString, int> m_channelIndex;
    int m_revision = 0;
    // the sessions of the servers signed in, by source (token, user id)
    QHash<QString, QPair<QString, QString>> m_sessions;
    // options of the interface
    int m_guideDays = 7;
    int m_refreshHours = 24;
    QSet<QString> m_languages;
    bool m_started = false;
    qint64 m_catalogChecked = 0;
    // the keystore's secrets read (loadSecrets), waited for with m_lock
    bool m_secretsReady = true;
    std::condition_variable m_secretsWait;
    // the keystore's calls, one at a time (never with m_lock held)
    std::mutex m_secretLock;

    std::mutex m_jobLock;
    std::condition_variable m_jobWait;
    std::deque<Job> m_jobs;
    std::atomic<bool> m_quit { false };
    std::thread m_thread;
    // the worker ended (waitStopped)
    std::mutex m_endLock;
    std::condition_variable m_endWait;
    bool m_workerEnded = false;
    // the catalog is refreshed this long after the start at the earliest
    // (or when it is shown): nothing downloads in the first moments
    std::chrono::steady_clock::time_point m_startedAt = std::chrono::steady_clock::now();
};

}
