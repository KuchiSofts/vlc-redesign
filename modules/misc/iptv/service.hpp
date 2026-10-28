/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
#pragma once

#include "guide.hpp"
#include "playlist.hpp"

#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVector>

#include <atomic>
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

// What VLC gives the service: downloads through its own access modules
// (the proxy and the network settings of VLC), its keystore, its folders.
// Implemented in iptv.cpp, the glue of the module.
struct Host
{
    virtual ~Host() = default;
    // Saves the resource at url (http(s), file...) to path, as it is (a
    // gzip-compressed guide stays so); false and why on an error.
    // progress gets the bytes received and the size when known (0).
    virtual bool download(const QString& url, const QString& path, QString* error,
                          const std::function<void(qint64, qint64)>& progress = {}) = 0;
    // A small answer (an Xtream Codes API call) read into memory
    virtual QByteArray fetch(const QString& url, QString* error, qint64 limit = 256 << 20) = 0;
    // The secrets of a source (its link, its password) in VLC's keystore;
    // false when there is no keystore
    virtual bool storeSecret(const QString& id, const QByteArray& secret) = 0;
    virtual QByteArray loadSecret(const QString& id, bool* found) = 0;
    virtual void removeSecret(const QString& id) = 0;
    // A change for the interface (from any thread)
    virtual void notify(const QJsonObject& event) = 0;
    virtual void log(const QString& message) = 0;
    // The worker thread of the service: its downloads stop when killed
    virtual void enterWorker() = 0;
    virtual void killWorker() = 0;
};

// A source of channels: an M3U playlist (a link or a file) or an Xtream
// Codes account, with the guides (XMLTV) of its channels
struct SourceConfig
{
    QString id;
    QString name;
    QString kind;            // "m3u" or "xtream"
    QString url;             // the playlist (secret: often holds a token)
    QStringList epg;         // the guides the user added
    QString server;          // Xtream Codes
    QString username;
    QString password;        // (secret)
    QString catalogId;       // added from the catalog
    bool enabled = true;
    bool useHeaderGuides = true;   // the guides the playlist names itself
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
    int programmes = 0;
    QString error;
};

class Service
{
public:
    Service(Host& host, const QString& dataDir, const QString& cacheDir,
            const QByteArray& bundledCatalog);
    ~Service();

    // The answer of a request of the interface (see vlc_iptv.h)
    QJsonObject request(const QJsonObject& request);

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
    void saveConfig();
    QJsonObject sourceToJson(const SourceConfig& s, bool withSecrets) const;
    SourceConfig sourceFromJson(const QJsonObject& o) const;
    SourceConfig* findSource(const QString& id);
    QString sourceDir(const QString& id) const;

    // the worker
    void post(Job job);
    void run();
    void startup();
    void schedule();
    void refreshSource(const QString& id, bool force, bool guides);
    bool loadPlaylist(const SourceConfig& cfg, bool download, SourceState& state);
    void loadGuides(bool download, const QString& onlySource);
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
    // options of the interface
    int m_guideDays = 7;
    int m_refreshHours = 24;
    QSet<QString> m_languages;
    bool m_started = false;
    qint64 m_catalogChecked = 0;

    std::mutex m_jobLock;
    std::condition_variable m_jobWait;
    std::deque<Job> m_jobs;
    std::atomic<bool> m_quit { false };
    std::thread m_thread;
};

}
