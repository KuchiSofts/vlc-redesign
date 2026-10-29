/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
#include "service.hpp"
#include "xtream.hpp"

#include <QCryptographicHash>
#include <QDataStream>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QRandomGenerator>
#include <QTimeZone>
#include <QUrl>

#include <algorithm>
#include <chrono>

namespace iptv {

namespace {

qint64 nowSecs()
{
    return QDateTime::currentSecsSinceEpoch();
}

QString hashOf(const QString& text)
{
    return QString::fromLatin1(
        QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Sha1).toHex().left(16));
}

// The host of a link, all the interface shows of it: the rest of a
// provider's link is often a personal token
QString hostOf(const QString& url)
{
    const QUrl u(url);
    if (u.isLocalFile() || !url.contains(QLatin1String("://")))
        return QFileInfo(u.isLocalFile() ? u.toLocalFile() : url).fileName();
    return u.host();
}

// A path typed by the user ("C:\TV\list.m3u", "/home/me/tv.m3u") as a URL
// VLC's access modules open
QString asUrl(const QString& address)
{
    const QString a = address.trimmed();
    if (a.contains(QLatin1String("://")))
        return a;
    return QUrl::fromLocalFile(QDir::fromNativeSeparators(a)).toString(QUrl::FullyEncoded);
}

qint64 ageOf(const QString& path)
{
    const QFileInfo info(path);
    if (!info.exists())
        return -1;
    return info.lastModified().secsTo(QDateTime::currentDateTime());
}

QByteArray readAll(const QString& path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

bool writeAll(const QString& path, const QByteArray& data)
{
    QFile file(path + QStringLiteral(".part"));
    if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size())
        return false;
    file.close();
    QFile::remove(path);
    return QFile::rename(path + QStringLiteral(".part"), path);
}

// The keys a channel can have in a guide, in the order they are tried
QStringList guideKeysOf(const Channel& c)
{
    QStringList keys;
    if (!c.tvgId.isEmpty())
        keys << guideKey(c.tvgId);
    if (!c.tvgName.isEmpty())
        keys << guideKey(c.tvgName);
    keys << guideKey(c.name);
    keys.removeDuplicates();
    keys.removeAll(QString());
    return keys;
}

int programmeIndex(const EntryList& list, qint64 t)
{
    auto it = std::upper_bound(list.cbegin(), list.cend(), t,
                               [](qint64 v, const Entry& e) { return v < e.start; });
    if (it == list.cbegin())
        return -1;
    --it;
    return t < it->stop ? int(it - list.cbegin()) : -1;
}

QString readDescription(const QString& path, qint64 offset, int length)
{
    if (offset < 0 || length <= 0 || path.isEmpty())
        return QString();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || !file.seek(offset))
        return QString();
    return QString::fromUtf8(file.read(length));
}

/* The index of a guide: what was read of its XMLTV file for these channels,
 * kept next to it. A guide of 200 MB takes seconds of a core to read again,
 * at every start; its index a fraction of that. It holds every programme
 * from the first day kept on (the days ahead are cut when it is read), the
 * texts once each, and names the file of the descriptions it goes with. */

const quint32 kIndexMagic = 0x56544749;   // "VTGI"
const qint32 kIndexVersion = 1;

struct GuideIndex
{
    QHash<QString, EntryList> lists;
    QHash<QString, QString> idByName;
    QSet<QString> languages;
    QString descName;    // in the folder of the guides
};

// What the index was made for: the channels, the languages, the days back
QByteArray signatureOf(const GuideFilter& filter, int days)
{
    QStringList keys(filter.keys.cbegin(), filter.keys.cend());
    keys.sort();
    QStringList languages(filter.languages.cbegin(), filter.languages.cend());
    languages.sort();
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(QByteArray::number(days));
    hash.addData(QByteArrayView("\n\n"));
    hash.addData(languages.join(u'\n').toUtf8());
    hash.addData(QByteArrayView("\n\n"));
    hash.addData(keys.join(u'\n').toUtf8());
    return hash.result();
}

bool writeIndex(const QString& path, const QString& xmltvPath, const QByteArray& signature,
                const GuideIndex& index)
{
    const QFileInfo xmltv(xmltvPath);
    const QFileInfo desc(xmltv.dir().filePath(index.descName));
    // the texts once each
    QHash<QString, quint32> ids;
    QStringList texts;
    auto idOf = [&](const QString& text) -> quint32 {
        auto it = ids.constFind(text);
        if (it != ids.cend())
            return *it;
        const quint32 id = quint32(texts.size());
        ids.insert(text, id);
        texts << text;
        return id;
    };
    QByteArray body;
    {
        QDataStream out(&body, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_6_0);
        out << qint32(index.lists.size());
        for (auto it = index.lists.cbegin(); it != index.lists.cend(); ++it)
        {
            out << it.key() << qint32(it->size());
            for (const Entry& e : *it)
                out << e.start << e.stop << idOf(e.title) << idOf(e.subtitle) << idOf(e.category)
                    << idOf(e.episode) << e.descOffset << qint32(e.descLength);
        }
    }
    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_0);
    out << kIndexMagic << kIndexVersion << xmltv.size() << xmltv.lastModified().toMSecsSinceEpoch()
        << signature << index.descName << (desc.exists() ? desc.size() : qint64(-1));
    QStringList languages(index.languages.cbegin(), index.languages.cend());
    out << languages << index.idByName << texts;
    out.writeRawData(body.constData(), int(body.size()));
    return out.status() == QDataStream::Ok && writeAll(path, data);
}

// The index when it is the one of this XMLTV file and these channels
bool readIndex(const QString& path, const QString& xmltvPath, const QByteArray& signature, GuideIndex& index)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    const QFileInfo xmltv(xmltvPath);
    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_6_0);
    quint32 magic = 0;
    qint32 version = 0;
    qint64 size = 0, modified = 0, descSize = 0;
    QByteArray madeFor;
    in >> magic >> version;
    if (magic != kIndexMagic || version != kIndexVersion)
        return false;
    in >> size >> modified >> madeFor >> index.descName >> descSize;
    if (in.status() != QDataStream::Ok || size != xmltv.size()
        || modified != xmltv.lastModified().toMSecsSinceEpoch() || madeFor != signature)
        return false;
    // the descriptions it points into, as they were written
    const QFileInfo desc(xmltv.dir().filePath(index.descName));
    if (index.descName.isEmpty() || index.descName.contains(u'/') || (desc.exists() ? desc.size() : -1) != descSize)
        return false;
    QStringList languages, texts;
    in >> languages >> index.idByName >> texts;
    index.languages = QSet<QString>(languages.cbegin(), languages.cend());
    qint32 keys = 0;
    in >> keys;
    if (in.status() != QDataStream::Ok || keys < 0)
        return false;
    const quint32 textCount = quint32(texts.size());
    index.lists.reserve(keys);
    for (qint32 k = 0; k < keys; ++k)
    {
        QString key;
        qint32 count = 0;
        in >> key >> count;
        if (in.status() != QDataStream::Ok || count < 0 || count > (1 << 24))
            return false;
        EntryList list;
        list.reserve(count);
        for (qint32 i = 0; i < count; ++i)
        {
            Entry e;
            quint32 title, subtitle, category, episode;
            qint32 length;
            in >> e.start >> e.stop >> title >> subtitle >> category >> episode >> e.descOffset >> length;
            if (title >= textCount || subtitle >= textCount || category >= textCount || episode >= textCount)
                return false;
            e.title = texts[title];
            e.subtitle = texts[subtitle];
            e.category = texts[category];
            e.episode = texts[episode];
            e.descLength = length;
            list.append(std::move(e));
        }
        index.lists.insert(key, std::move(list));
    }
    return in.status() == QDataStream::Ok;
}

}

Service::Service(Host& host, const QString& dataDir, const QString& cacheDir,
                 const QByteArray& bundledCatalog)
    : m_host(host)
    , m_dataDir(dataDir)
    , m_cacheDir(cacheDir)
    , m_bundledCatalog(bundledCatalog)
{
    QDir().mkpath(m_dataDir);
    QDir().mkpath(m_cacheDir + QStringLiteral("/guides"));
    QDir().mkpath(m_cacheDir + QStringLiteral("/logos"));
    loadConfig();
    m_thread = std::thread([this]() { run(); });
}

Service::~Service()
{
    m_quit = true;
    m_host.killWorker();
    m_jobWait.notify_all();
    if (m_thread.joinable())
        m_thread.join();
}

/* The configuration: the sources, in their order (the guides of the first
 * ones win when several name the same channel). Their secrets are kept
 * in VLC's keystore, in the file only when there is none. */

void Service::loadConfig()
{
    const QJsonObject root = QJsonDocument::fromJson(readAll(m_dataDir + QStringLiteral("/sources.json"))).object();
    for (const QJsonValue& v : root.value(QLatin1String("sources")).toArray())
    {
        SourceConfig s = sourceFromJson(v.toObject());
        if (s.id.isEmpty())
            continue;
        if (v.toObject().value(QLatin1String("keystore")).toBool())
        {
            bool found = false;
            const QJsonObject secret = QJsonDocument::fromJson(m_host.loadSecret(s.id, &found)).object();
            s.url = secret.value(QLatin1String("url")).toString();
            s.password = secret.value(QLatin1String("password")).toString();
        }
        m_sources.append(s);
    }
    const QJsonObject options = root.value(QLatin1String("options")).toObject();
    m_catalogChecked = qint64(options.value(QLatin1String("catalogChecked")).toDouble());
}

void Service::saveConfig()
{
    // (under m_lock)
    QJsonArray list;
    for (const SourceConfig& s : std::as_const(m_sources))
    {
        QJsonObject o = sourceToJson(s, false);
        QJsonObject secret;
        secret.insert(QLatin1String("url"), s.url);
        secret.insert(QLatin1String("password"), s.password);
        if (m_host.storeSecret(s.id, QJsonDocument(secret).toJson(QJsonDocument::Compact)))
            o.insert(QLatin1String("keystore"), true);
        else
        {
            o.insert(QLatin1String("url"), s.url);
            o.insert(QLatin1String("password"), s.password);
        }
        list.append(o);
    }
    QJsonObject options;
    options.insert(QLatin1String("catalogChecked"), double(m_catalogChecked));
    QJsonObject root;
    root.insert(QLatin1String("version"), 1);
    root.insert(QLatin1String("sources"), list);
    root.insert(QLatin1String("options"), options);
    if (!writeAll(m_dataDir + QStringLiteral("/sources.json"), QJsonDocument(root).toJson()))
        m_host.log(QStringLiteral("the TV sources could not be saved"));
}

QJsonObject Service::sourceToJson(const SourceConfig& s, bool withSecrets) const
{
    QJsonObject o;
    o.insert(QLatin1String("id"), s.id);
    o.insert(QLatin1String("name"), s.name);
    o.insert(QLatin1String("kind"), s.kind);
    o.insert(QLatin1String("epg"), QJsonArray::fromStringList(s.epg));
    o.insert(QLatin1String("server"), s.server);
    o.insert(QLatin1String("username"), s.username);
    o.insert(QLatin1String("catalogId"), s.catalogId);
    o.insert(QLatin1String("enabled"), s.enabled);
    o.insert(QLatin1String("useHeaderGuides"), s.useHeaderGuides);
    if (withSecrets)
    {
        o.insert(QLatin1String("url"), s.url);
        o.insert(QLatin1String("password"), s.password);
    }
    return o;
}

SourceConfig Service::sourceFromJson(const QJsonObject& o) const
{
    SourceConfig s;
    s.id = o.value(QLatin1String("id")).toString();
    s.name = o.value(QLatin1String("name")).toString().trimmed();
    s.kind = o.value(QLatin1String("kind")).toString(QStringLiteral("m3u"));
    s.url = o.value(QLatin1String("url")).toString().trimmed();
    for (const QJsonValue& e : o.value(QLatin1String("epg")).toArray())
        if (!e.toString().trimmed().isEmpty())
            s.epg << e.toString().trimmed();
    s.server = o.value(QLatin1String("server")).toString().trimmed();
    s.username = o.value(QLatin1String("username")).toString().trimmed();
    s.password = o.value(QLatin1String("password")).toString();
    s.catalogId = o.value(QLatin1String("catalogId")).toString();
    s.enabled = o.value(QLatin1String("enabled")).toBool(true);
    s.useHeaderGuides = o.value(QLatin1String("useHeaderGuides")).toBool(true);
    return s;
}

SourceConfig* Service::findSource(const QString& id)
{
    for (SourceConfig& s : m_sources)
        if (s.id == id)
            return &s;
    return nullptr;
}

QString Service::sourceDir(const QString& id) const
{
    return m_cacheDir + QStringLiteral("/sources/") + id;
}

/* The worker: the downloads and the reading of the playlists and guides,
 * one after the other, so the interface never waits for them */

void Service::post(Job job)
{
    {
        std::lock_guard<std::mutex> guard(m_jobLock);
        m_jobs.push_back(std::move(job));
    }
    m_jobWait.notify_all();
}

void Service::run()
{
    m_host.enterWorker();
    auto nextCheck = std::chrono::steady_clock::now() + std::chrono::seconds(60);
    while (!m_quit)
    {
        Job job;
        {
            std::unique_lock<std::mutex> guard(m_jobLock);
            m_jobWait.wait_until(guard, nextCheck, [this]() { return m_quit || !m_jobs.empty(); });
            if (m_quit)
                break;
            if (!m_jobs.empty())
            {
                job = std::move(m_jobs.front());
                m_jobs.pop_front();
            }
        }
        if (job)
            job();
        else
        {
            nextCheck = std::chrono::steady_clock::now() + std::chrono::seconds(60);
            schedule();
        }
    }
}

void Service::startup()
{
    // the caches first, offline: the channels show at once, the guides a
    // moment later; then whatever is missing or old is downloaded
    QVector<SourceConfig> sources;
    {
        std::lock_guard<std::mutex> guard(m_lock);
        sources = m_sources;
    }
    QElapsedTimer took;
    took.start();
    for (const SourceConfig& cfg : sources)
    {
        SourceState state;
        loadPlaylist(cfg, false, state);
        std::lock_guard<std::mutex> guard(m_lock);
        m_state.insert(cfg.id, state);
    }
    rebuild();
    changed("channels");
    {
        std::lock_guard<std::mutex> guard(m_lock);
        m_host.debug(QStringLiteral("%1 channels of the cache in %2 ms").arg(m_channels.size()).arg(took.elapsed()));
    }
    loadGuides(false, {});
    rebuild();
    changed("guide");
    m_host.debug(QStringLiteral("the guides of the cache in %1 ms").arg(took.elapsed()));
    QStringList missing;
    {
        std::lock_guard<std::mutex> guard(m_lock);
        for (const SourceConfig& cfg : sources)
            if (cfg.enabled && m_state.value(cfg.id).channels.isEmpty())
                missing << cfg.id;
    }
    if (!missing.isEmpty())
        refreshSources(missing, true);
    schedule();
}

void Service::schedule()
{
    if (m_quit)
        return;
    QStringList due;
    {
        std::lock_guard<std::mutex> guard(m_lock);
        if (!m_started)
            return;
        if (m_refreshHours > 0)
            for (const SourceConfig& s : std::as_const(m_sources))
            {
                const SourceState state = m_state.value(s.id);
                if (s.enabled && !state.busy
                    && (state.playlistUpdated == 0
                        || nowSecs() - state.playlistUpdated > qint64(m_refreshHours) * 3600))
                    due << s.id;
            }
    }
    if (!due.isEmpty())
        refreshSources(due, true);
    updateCatalog(false);
}

void Service::progress(const QString& id, const QString& stage, qint64 received, qint64 total)
{
    {
        std::lock_guard<std::mutex> guard(m_lock);
        auto it = m_state.find(id);
        if (it != m_state.end())
        {
            it->stage = stage;
            it->received = received;
            it->total = total;
            it->busy = !stage.isEmpty();
        }
    }
    QJsonObject event;
    event.insert(QLatin1String("event"), QStringLiteral("progress"));
    event.insert(QLatin1String("id"), id);
    event.insert(QLatin1String("stage"), stage);
    event.insert(QLatin1String("received"), double(received));
    event.insert(QLatin1String("total"), double(total));
    m_host.notify(event);
}

void Service::changed(const char* what)
{
    QJsonObject event;
    event.insert(QLatin1String("event"), QStringLiteral("changed"));
    event.insert(QLatin1String("what"), QLatin1String(what));
    {
        std::lock_guard<std::mutex> guard(m_lock);
        event.insert(QLatin1String("revision"), m_revision);
    }
    m_host.notify(event);
}

void Service::refreshSources(const QStringList& ids, bool guides)
{
    // every playlist first (each source's channels show as they come), then
    // each guide read once for the channels of all of them: one source at a
    // time read every guide again for each source
    QStringList done;
    for (const QString& id : ids)
    {
        if (m_quit)
            return;
        SourceConfig cfg;
        SourceState state;
        {
            std::lock_guard<std::mutex> guard(m_lock);
            const SourceConfig* found = findSource(id);
            if (!found)
                continue;
            cfg = *found;
            state = m_state.value(id);
        }
        progress(id, QStringLiteral("playlist"), 0, 0);
        SourceState fresh = state;
        if (!loadPlaylist(cfg, true, fresh) && !state.channels.isEmpty())
        {
            // offline, or the provider is down: the last channels stay
            const QString error = fresh.error;
            fresh = state;
            fresh.error = error;
        }
        {
            std::lock_guard<std::mutex> guard(m_lock);
            if (!findSource(id))
                continue;
            fresh.busy = guides;
            fresh.stage = guides ? QStringLiteral("guide") : QString();
            m_state.insert(id, fresh);
        }
        done << id;
        rebuild();
        changed("channels");
    }
    if (guides && !m_quit && !done.isEmpty())
    {
        loadGuides(true, done);
        rebuild();
    }
    for (const QString& id : std::as_const(done))
        progress(id, QString(), 0, 0);
    changed("guide");
}

bool Service::loadPlaylist(const SourceConfig& cfg, bool download, SourceState& state)
{
    const QString dir = sourceDir(cfg.id);
    QDir().mkpath(dir);
    QString error;
    if (cfg.kind == QLatin1String("xtream"))
    {
        xtream::Account account { cfg.server, cfg.username, cfg.password };
        const QString accountPath = dir + QStringLiteral("/account.json");
        const QString categoriesPath = dir + QStringLiteral("/live_categories.json");
        const QString streamsPath = dir + QStringLiteral("/live_streams.json");
        if (download)
        {
            const QByteArray info = m_host.fetch(xtream::apiUrl(account).toString(QUrl::FullyEncoded), &error);
            const xtream::AccountInfo parsed = xtream::parseAccountInfo(info);
            if (info.isEmpty() || !parsed.authenticated)
            {
                state.error = info.isEmpty() ? error : parsed.error;
                download = false;
            }
            else
            {
                writeAll(accountPath, info);
                const QByteArray categories = m_host.fetch(
                    xtream::apiUrl(account, QStringLiteral("get_live_categories")).toString(QUrl::FullyEncoded), &error);
                const QByteArray streams = m_host.fetch(
                    xtream::apiUrl(account, QStringLiteral("get_live_streams")).toString(QUrl::FullyEncoded), &error,
                    512 << 20);
                if (streams.isEmpty())
                    state.error = error;
                else
                {
                    writeAll(categoriesPath, categories);
                    writeAll(streamsPath, streams);
                    state.error.clear();
                }
                // what else the account has: films, series
                state.hasVod = !xtream::parseCategories(m_host.fetch(
                    xtream::apiUrl(account, QStringLiteral("get_vod_categories")).toString(QUrl::FullyEncoded), &error)).isEmpty();
                state.hasSeries = !xtream::parseCategories(m_host.fetch(
                    xtream::apiUrl(account, QStringLiteral("get_series_categories")).toString(QUrl::FullyEncoded), &error)).isEmpty();
                writeAll(dir + QStringLiteral("/library.json"),
                         QJsonDocument(QJsonObject { { QStringLiteral("vod"), state.hasVod },
                                                     { QStringLiteral("series"), state.hasSeries } })
                             .toJson(QJsonDocument::Compact));
            }
        }
        const QByteArray streams = readAll(streamsPath);
        if (streams.isEmpty())
        {
            if (state.error.isEmpty())
                state.error = QStringLiteral("no channels yet");
            return false;
        }
        const xtream::AccountInfo info = xtream::parseAccountInfo(readAll(accountPath));
        const QJsonObject library = QJsonDocument::fromJson(readAll(dir + QStringLiteral("/library.json"))).object();
        state.hasVod = library.value(QLatin1String("vod")).toBool();
        state.hasSeries = library.value(QLatin1String("series")).toBool();
        state.expires = info.expires;
        state.channels = xtream::parseLiveStreams(streams, account, xtream::parseCategories(readAll(categoriesPath)), info);
        // the guide of the account
        QUrl guide(xtream::normalizeServer(cfg.server) + QStringLiteral("/xmltv.php"));
        guide.setQuery(xtream::apiUrl(account).query(QUrl::FullyEncoded), QUrl::TolerantMode);
        state.headerGuides = QStringList { guide.toString(QUrl::FullyEncoded) };
        state.playlistUpdated = QFileInfo(streamsPath).lastModified().toSecsSinceEpoch();
        return !state.channels.isEmpty();
    }

    const QString path = dir + QStringLiteral("/playlist.m3u");
    if (download)
    {
        if (m_host.download(asUrl(cfg.url), path, &error,
                            [this, &cfg](qint64 received, qint64 total) {
                                progress(cfg.id, QStringLiteral("playlist"), received, total);
                            }))
            state.error.clear();
        else
            state.error = error;
    }
    const QByteArray data = readAll(path);
    if (data.isEmpty())
    {
        if (state.error.isEmpty())
            state.error = download ? QStringLiteral("the playlist is empty") : QString();
        return false;
    }
    if (!isChannelList(data))
    {
        state.error = QStringLiteral("this link is a single stream, not a list of channels");
        return false;
    }
    Playlist list = parseM3u(data, asUrl(cfg.url));
    if (!list.error.isEmpty())
    {
        state.error = list.error;
        return false;
    }
    state.channels = std::move(list.channels);
    state.headerGuides = list.guideUrls;
    state.playlistUpdated = QFileInfo(path).lastModified().toSecsSinceEpoch();
    return true;
}

void Service::loadGuides(bool download, const QStringList& onlySources)
{
    // what to read: the guides of every source, and the keys of every
    // channel (a guide of one source may hold channels of another)
    struct Wanted { QString url; QString sourceId; };
    QVector<Wanted> wanted;
    GuideFilter filter;
    int days, refreshHours;
    {
        std::lock_guard<std::mutex> guard(m_lock);
        for (const SourceConfig& s : std::as_const(m_sources))
        {
            if (!s.enabled)
                continue;
            const SourceState state = m_state.value(s.id);
            QStringList urls = s.epg;
            if (s.useHeaderGuides || s.kind == QLatin1String("xtream"))
                urls << state.headerGuides;
            for (const QString& url : std::as_const(urls))
                if (std::none_of(wanted.cbegin(), wanted.cend(), [&](const Wanted& w) { return w.url == url; }))
                    wanted.append({ url, s.id });
            for (const Channel& c : state.channels)
                for (const QString& k : guideKeysOf(c))
                    filter.keys.insert(k);
        }
        days = m_guideDays;
        refreshHours = m_refreshHours;
        filter.languages = m_languages;
    }
    if (filter.keys.isEmpty())
        return;
    const qint64 now = nowSecs();
    filter.from = now - qint64(days) * 86400;
    filter.until = now + qint64(days) * 86400;
    filter.cancel = &m_quit;
    const QByteArray signature = signatureOf(filter, days);

    QHash<QString, std::shared_ptr<GuideData>> result;
    for (const Wanted& w : std::as_const(wanted))
    {
        if (m_quit)
            return;
        const QString base = m_cacheDir + QStringLiteral("/guides/") + hashOf(w.url);
        const QString path = base + QStringLiteral(".xmltv");
        const qint64 age = ageOf(path);
        QString error;
        const bool mine = onlySources.isEmpty() || onlySources.contains(w.sourceId);
        if (download && mine && (age < 0 || refreshHours <= 0 || age > qint64(refreshHours) * 3600 - 600))
        {
            progress(w.sourceId, QStringLiteral("guide"), 0, 0);
            if (!m_host.download(asUrl(w.url), path, &error,
                                 [this, &w](qint64 received, qint64 total) {
                                     progress(w.sourceId, QStringLiteral("guide"), received, total);
                                 }))
                m_host.log(QStringLiteral("a guide of %1 could not be downloaded: %2").arg(hostOf(w.url), error));
        }
        auto data = std::make_shared<GuideData>();
        data->url = w.url;
        data->sourceId = w.sourceId;
        data->error = error;
        if (!QFileInfo::exists(path))
        {
            // nothing yet: kept for the status, with its error
            result.insert(w.url, data);
            continue;
        }
        QElapsedTimer took;
        took.start();
        data->updated = QFileInfo(path).lastModified().toSecsSinceEpoch();
        data->signature = signature;
        if (!mine)
        {
            // the guide of another source, not downloaded now: as it was
            // read, when it was for the same channels (one reading of every
            // guide per source refreshed, before)
            std::shared_ptr<GuideData> old;
            {
                std::lock_guard<std::mutex> guard(m_lock);
                old = m_guides.value(w.url);
            }
            if (old && old->signature == signature && old->updated == data->updated && !old->descPath.isEmpty())
            {
                result.insert(w.url, old);
                continue;
            }
        }
        const QString indexPath = base + QStringLiteral(".idx");
        GuideIndex index;
        const bool indexed = readIndex(indexPath, path, signature, index);
        if (!indexed)
        {
            index = GuideIndex();
            // every day ahead the file has: the index serves the next starts
            GuideFilter all = filter;
            all.until = 0;
            Guide guide = parseXmltvFile(path, all);
            if (m_quit)
                return;
            if (!guide.error.isEmpty() && guide.programmes.isEmpty())
                data->error = guide.error;
            // the descriptions to a file of their own, the rest kept; a new
            // file each time: the last one may be open for a reading
            index.descName = QFileInfo(base).fileName()
                             + QStringLiteral("-%1.desc").arg(QDateTime::currentMSecsSinceEpoch());
            QFile desc(QFileInfo(path).dir().filePath(index.descName));
            const bool canWrite = desc.open(QIODevice::WriteOnly);
            qint64 offset = 0;
            // the same titles and categories over and over: shared
            QHash<QString, QString> pool;
            auto shared = [&pool](const QString& text) -> QString {
                if (text.isEmpty())
                    return text;
                auto it = pool.constFind(text);
                if (it != pool.cend())
                    return *it;
                pool.insert(text, text);
                return text;
            };
            for (auto it = guide.programmes.begin(); it != guide.programmes.end(); ++it)
            {
                EntryList list;
                list.reserve(it->size());
                for (Programme& p : *it)
                {
                    Entry e;
                    e.start = p.start;
                    e.stop = p.stop;
                    e.title = shared(p.title);
                    e.subtitle = p.subtitle;
                    e.category = shared(p.category);
                    e.episode = p.episode;
                    if (canWrite && !p.description.isEmpty())
                    {
                        const QByteArray text = p.description.toUtf8();
                        if (desc.write(text) == text.size())
                        {
                            e.descOffset = offset;
                            e.descLength = int(text.size());
                            offset += text.size();
                        }
                    }
                    list.append(std::move(e));
                }
                index.lists.insert(it.key(), std::move(list));
                it->clear();
            }
            desc.close();
            index.idByName = std::move(guide.idByName);
            index.languages = std::move(guide.languages);
            if (guide.error.isEmpty() && canWrite && !writeIndex(indexPath, path, signature, index))
                m_host.log(QStringLiteral("the index of a guide of %1 could not be written").arg(hostOf(w.url)));
        }
        data->descPath = QFileInfo(path).dir().filePath(index.descName);
        // the days kept: from the days back to as many ahead
        for (auto it = index.lists.begin(); it != index.lists.end(); ++it)
        {
            auto list = std::make_shared<EntryList>();
            list->reserve(it->size());
            for (Entry& e : *it)
                if (!(e.stop > 0 && e.stop < filter.from) && e.start <= filter.until)
                    list->append(std::move(e));
            if (list->isEmpty())
                continue;
            list->squeeze();
            data->programmes += int(list->size());
            data->byKey.insert(it.key(), std::move(list));
        }
        data->idByName = std::move(index.idByName);
        data->languages = std::move(index.languages);
        m_host.debug(QStringLiteral("guide of %1: %2 programmes of %3 channels, %4 in %5 ms")
                         .arg(hostOf(w.url)).arg(data->programmes).arg(data->byKey.size())
                         .arg(indexed ? QStringLiteral("from its index") : QStringLiteral("read"))
                         .arg(took.elapsed()));
        result.insert(w.url, data);
    }

    std::lock_guard<std::mutex> guard(m_lock);
    QSet<QString> keep;
    for (auto it = result.cbegin(); it != result.cend(); ++it)
    {
        auto old = m_guides.value(it.key());
        // a guide that could not be read again keeps what it had
        if ((*it)->byKey.isEmpty() && old && !old->byKey.isEmpty())
        {
            auto kept = std::make_shared<GuideData>(*old);
            kept->error = (*it)->error;
            m_guides.insert(it.key(), kept);
        }
        else
        {
            if (old && old->descPath != (*it)->descPath)
                QFile::remove(old->descPath);
            m_guides.insert(it.key(), *it);
        }
        keep.insert(it.key());
    }
    for (auto it = m_guides.begin(); it != m_guides.end();)
    {
        if (!keep.contains(it.key()) && onlySources.isEmpty())
        {
            QFile::remove((*it)->descPath);
            it = m_guides.erase(it);
        }
        else
            ++it;
    }
    // the descriptions of the guides read that nothing points to anymore:
    // the ones of an earlier start or reading (13 MB each for a guide of
    // 200 MB, one more at every start before the index)
    QSet<QString> inUse;
    for (const auto& g : std::as_const(m_guides))
        inUse.insert(QFileInfo(g->descPath).fileName());
    QSet<QString> bases;
    for (auto it = result.cbegin(); it != result.cend(); ++it)
        bases.insert(hashOf(it.key()));
    // (no name filter: Qt matches those with a regular expression, whose
    // stack of this thread would outlive the module and crash at the end)
    const QDir dir(m_cacheDir + QStringLiteral("/guides"));
    for (const QString& name : dir.entryList(QDir::Files))
        if (name.endsWith(QLatin1String(".desc")) && bases.contains(name.section(u'-', 0, 0))
            && !inUse.contains(name))
            QFile::remove(dir.filePath(name));
}

void Service::rebuild()
{
    std::lock_guard<std::mutex> guard(m_lock);
    m_channels.clear();
    m_channelIndex.clear();
    // the guides in their order: the sources', each in its own order
    QVector<std::shared_ptr<GuideData>> ordered;
    for (const SourceConfig& s : std::as_const(m_sources))
    {
        if (!s.enabled)
            continue;
        QStringList urls = s.epg;
        urls << m_state.value(s.id).headerGuides;
        for (const QString& url : std::as_const(urls))
        {
            const auto g = m_guides.value(url);
            if (g && !ordered.contains(g))
                ordered.append(g);
        }
    }
    int sequence = 0;
    for (const SourceConfig& s : std::as_const(m_sources))
    {
        if (!s.enabled)
            continue;
        const SourceState& state = m_state[s.id];
        // its own guides first, then the others'
        QVector<std::shared_ptr<GuideData>> guides;
        for (const auto& g : std::as_const(ordered))
            if (g->sourceId == s.id)
                guides.append(g);
        for (const auto& g : std::as_const(ordered))
            if (g->sourceId != s.id)
                guides.append(g);
        for (const Channel& c : state.channels)
        {
            ++sequence;
            ChannelEntry entry;
            entry.sourceId = s.id;
            entry.channel = c;
            if (entry.channel.number <= 0)
                entry.channel.number = sequence;
            QString key = s.id + (c.tvgId.isEmpty() ? QStringLiteral("/n:") + c.name
                                                    : QStringLiteral("/i:") + c.tvgId);
            // the same channel twice in a list (two qualities)
            if (m_channelIndex.contains(key))
            {
                int n = 2;
                while (m_channelIndex.contains(key + QStringLiteral("#%1").arg(n)))
                    ++n;
                key += QStringLiteral("#%1").arg(n);
            }
            entry.key = key;
            const QStringList keys = guideKeysOf(c);
            for (const auto& g : std::as_const(guides))
            {
                std::shared_ptr<const EntryList> found;
                for (const QString& k : keys)
                    if ((found = g->byKey.value(k)))
                        break;
                if (!found)
                    for (const QString& k : keys)
                    {
                        const QString id = g->idByName.value(k);
                        if (!id.isEmpty() && (found = g->byKey.value(id)))
                            break;
                    }
                if (found && !found->isEmpty())
                {
                    entry.guide = found;
                    entry.guideDescPath = g->descPath;
                    break;
                }
            }
            m_channelIndex.insert(key, int(m_channels.size()));
            m_channels.append(std::move(entry));
        }
    }
    ++m_revision;
}

void Service::updateCatalog(bool force)
{
    const qint64 now = nowSecs();
    {
        std::lock_guard<std::mutex> guard(m_lock);
        if (!force && now - m_catalogChecked < 86400)
            return;
        m_catalogChecked = now;
        saveConfig();
    }
    // the copy of the project's repository, newer than the one of this
    // build; quietly nothing when it is not there (yet)
    QString error;
    const QByteArray data = m_host.fetch(QStringLiteral(
        "https://raw.githubusercontent.com/KuchiSofts/vlc-redesign/main/modules/misc/iptv/catalog/catalog.json"),
        &error, 8 << 20);
    const QJsonObject root = QJsonDocument::fromJson(data).object();
    if (root.value(QLatin1String("version")).toInt() != 1 || !root.value(QLatin1String("sources")).isArray())
        return;
    if (writeAll(m_cacheDir + QStringLiteral("/catalog.json"), data))
        changed("catalog");
}

/* The answers */

QJsonObject Service::request(const QJsonObject& r)
{
    const QString op = r.value(QLatin1String("op")).toString();
    QJsonObject out;
    if (op == QLatin1String("start"))
    {
        bool first = false;
        {
            std::lock_guard<std::mutex> guard(m_lock);
            first = !m_started;
            m_started = true;
        }
        if (first)
            post([this]() { startup(); });
        return out;
    }
    if (op == QLatin1String("setOptions"))
    {
        bool reread = false;
        {
            std::lock_guard<std::mutex> guard(m_lock);
            const int days = qBound(1, r.value(QLatin1String("guideDays")).toInt(m_guideDays), 14);
            QSet<QString> languages;
            for (const QJsonValue& v : r.value(QLatin1String("languages")).toArray())
                if (!v.toString().trimmed().isEmpty())
                    languages.insert(v.toString().trimmed().toLower());
            reread = m_started && (days != m_guideDays || languages != m_languages);
            m_guideDays = days;
            m_languages = languages;
            m_refreshHours = qMax(0, r.value(QLatin1String("refreshHours")).toInt(m_refreshHours));
        }
        if (reread)
            post([this]() { loadGuides(false, {}); rebuild(); changed("guide"); });
        return out;
    }
    if (op == QLatin1String("status"))
        return status();
    if (op == QLatin1String("channels"))
        return channels();
    if (op == QLatin1String("programmes"))
        return programmes(r);
    if (op == QLatin1String("nowNext"))
        return nowNext(r);
    if (op == QLatin1String("description"))
        return description(r);
    if (op == QLatin1String("play") || op == QLatin1String("catchup"))
        return catchup(r);
    if (op == QLatin1String("catalog"))
        return catalog();
    if (op == QLatin1String("vod"))
        return vod(r);
    if (op == QLatin1String("seriesInfo"))
        return seriesInfo(r);
    // the channel an address plays (a queue restored at the start, a list
    // opened elsewhere): the interface then shows it as TV
    if (op == QLatin1String("channelOfUrl"))
    {
        const QString url = r.value(QLatin1String("url")).toString();
        std::lock_guard<std::mutex> guard(m_lock);
        for (const ChannelEntry& c : std::as_const(m_channels))
            if (c.channel.url == url)
                return QJsonObject { { QStringLiteral("key"), c.key } };
        return QJsonObject();
    }
    if (op == QLatin1String("logo"))
        return logo(r);
    if (op == QLatin1String("getSource"))
    {
        std::lock_guard<std::mutex> guard(m_lock);
        const SourceConfig* s = findSource(r.value(QLatin1String("id")).toString());
        if (!s)
            return QJsonObject { { QStringLiteral("error"), QStringLiteral("no such source") } };
        out.insert(QLatin1String("source"), sourceToJson(*s, true));
        return out;
    }
    if (op == QLatin1String("addSource") || op == QLatin1String("updateSource"))
    {
        SourceConfig s = sourceFromJson(r.value(QLatin1String("source")).toObject());
        if (s.kind != QLatin1String("xtream"))
            s.kind = QStringLiteral("m3u");
        if (s.kind == QLatin1String("m3u") ? s.url.isEmpty()
                                           : (s.server.isEmpty() || s.username.isEmpty()))
            return QJsonObject { { QStringLiteral("error"), QStringLiteral("incomplete") } };
        if (s.name.isEmpty())
            s.name = s.kind == QLatin1String("xtream") ? hostOf(xtream::normalizeServer(s.server))
                                                       : hostOf(s.url);
        QString id;
        {
            std::lock_guard<std::mutex> guard(m_lock);
            if (op == QLatin1String("updateSource"))
            {
                SourceConfig* old = findSource(r.value(QLatin1String("id")).toString());
                if (!old)
                    return QJsonObject { { QStringLiteral("error"), QStringLiteral("no such source") } };
                s.id = old->id;
                // a password left empty is the one kept
                if (s.kind == QLatin1String("xtream") && s.password.isEmpty())
                    s.password = old->password;
                *old = s;
            }
            else
            {
                s.id = QString::number(QRandomGenerator::global()->generate64(), 36);
                m_sources.append(s);
            }
            id = s.id;
            saveConfig();
        }
        post([this, id]() { refreshSources({ id }, true); });
        changed("sources");
        out.insert(QLatin1String("id"), id);
        return out;
    }
    if (op == QLatin1String("removeSource"))
    {
        const QString id = r.value(QLatin1String("id")).toString();
        {
            std::lock_guard<std::mutex> guard(m_lock);
            m_sources.erase(std::remove_if(m_sources.begin(), m_sources.end(),
                                           [&](const SourceConfig& s) { return s.id == id; }),
                            m_sources.end());
            m_state.remove(id);
            saveConfig();
        }
        m_host.removeSecret(id);
        QDir(sourceDir(id)).removeRecursively();
        post([this]() { loadGuides(false, {}); rebuild(); changed("channels"); changed("guide"); });
        changed("sources");
        return out;
    }
    if (op == QLatin1String("moveSource") || op == QLatin1String("enableSource"))
    {
        const QString id = r.value(QLatin1String("id")).toString();
        {
            std::lock_guard<std::mutex> guard(m_lock);
            const auto it = std::find_if(m_sources.begin(), m_sources.end(),
                                         [&](const SourceConfig& s) { return s.id == id; });
            if (it == m_sources.end())
                return QJsonObject { { QStringLiteral("error"), QStringLiteral("no such source") } };
            if (op == QLatin1String("moveSource"))
            {
                const int from = int(it - m_sources.begin());
                const int to = qBound(0, r.value(QLatin1String("index")).toInt(), int(m_sources.size()) - 1);
                m_sources.move(from, to);
            }
            else
                it->enabled = r.value(QLatin1String("enabled")).toBool(true);
            saveConfig();
        }
        post([this]() { loadGuides(false, {}); rebuild(); changed("channels"); changed("guide"); });
        changed("sources");
        return out;
    }
    if (op == QLatin1String("refresh"))
    {
        const QString id = r.value(QLatin1String("id")).toString();
        QStringList ids;
        {
            std::lock_guard<std::mutex> guard(m_lock);
            for (const SourceConfig& s : std::as_const(m_sources))
                if ((id.isEmpty() && s.enabled) || s.id == id)
                    ids << s.id;
        }
        if (!ids.isEmpty())
            post([this, ids]() { refreshSources(ids, true); });
        return out;
    }
    return QJsonObject { { QStringLiteral("error"), QStringLiteral("unknown request") } };
}

QJsonObject Service::status()
{
    std::lock_guard<std::mutex> guard(m_lock);
    QJsonArray sources;
    bool busy = false;
    for (const SourceConfig& s : std::as_const(m_sources))
    {
        const SourceState state = m_state.value(s.id);
        QJsonObject o = sourceToJson(s, false);
        o.insert(QLatin1String("host"), s.kind == QLatin1String("xtream") ? hostOf(xtream::normalizeServer(s.server))
                                                                          : hostOf(s.url));
        o.insert(QLatin1String("channels"), int(state.channels.size()));
        int catchupDays = 0, withGuide = 0;
        for (const ChannelEntry& c : std::as_const(m_channels))
            if (c.sourceId == s.id)
            {
                catchupDays = qMax(catchupDays, c.channel.catchup != CatchupMode::None ? c.channel.catchupDays : 0);
                if (c.guide)
                    ++withGuide;
            }
        o.insert(QLatin1String("withGuide"), withGuide);
        o.insert(QLatin1String("catchupDays"), catchupDays);
        o.insert(QLatin1String("vod"), state.hasVod);
        o.insert(QLatin1String("series"), state.hasSeries);
        o.insert(QLatin1String("expires"), double(state.expires));
        o.insert(QLatin1String("updated"), double(state.playlistUpdated));
        o.insert(QLatin1String("error"), state.error);
        o.insert(QLatin1String("stage"), state.stage);
        o.insert(QLatin1String("received"), double(state.received));
        o.insert(QLatin1String("total"), double(state.total));
        o.insert(QLatin1String("busy"), state.busy);
        busy = busy || state.busy;
        QJsonArray guides;
        QStringList urls = s.epg;
        if (s.useHeaderGuides || s.kind == QLatin1String("xtream"))
            urls << state.headerGuides;
        for (const QString& url : std::as_const(urls))
        {
            const auto g = m_guides.value(url);
            QJsonObject go;
            go.insert(QLatin1String("host"), hostOf(url));
            go.insert(QLatin1String("own"), s.epg.contains(url));
            go.insert(QLatin1String("programmes"), g ? g->programmes : 0);
            go.insert(QLatin1String("updated"), g ? double(g->updated) : 0.0);
            go.insert(QLatin1String("error"), g ? g->error : QString());
            guides.append(go);
        }
        o.insert(QLatin1String("guides"), guides);
        sources.append(o);
    }
    QSet<QString> languages;
    for (const auto& g : std::as_const(m_guides))
        languages.unite(g->languages);
    QStringList sortedLanguages(languages.cbegin(), languages.cend());
    sortedLanguages.sort();
    QJsonObject out;
    out.insert(QLatin1String("sources"), sources);
    out.insert(QLatin1String("busy"), busy);
    out.insert(QLatin1String("revision"), m_revision);
    out.insert(QLatin1String("languages"), QJsonArray::fromStringList(sortedLanguages));
    return out;
}

QJsonObject Service::channels()
{
    std::lock_guard<std::mutex> guard(m_lock);
    QJsonArray list;
    for (const ChannelEntry& c : std::as_const(m_channels))
    {
        QJsonObject o;
        o.insert(QLatin1String("key"), c.key);
        o.insert(QLatin1String("source"), c.sourceId);
        o.insert(QLatin1String("name"), c.channel.name);
        o.insert(QLatin1String("number"), c.channel.number);
        o.insert(QLatin1String("group"), displayGroup(c.channel.group));
        o.insert(QLatin1String("logo"), c.channel.logo);
        o.insert(QLatin1String("catchupDays"), c.channel.catchup != CatchupMode::None ? c.channel.catchupDays : 0);
        o.insert(QLatin1String("radio"), c.channel.radio);
        o.insert(QLatin1String("adult"), c.channel.adult);
        o.insert(QLatin1String("guide"), bool(c.guide));
        list.append(o);
    }
    QJsonObject out;
    out.insert(QLatin1String("revision"), m_revision);
    out.insert(QLatin1String("channels"), list);
    return out;
}

QJsonObject Service::entryJson(const ChannelEntry& c, const Entry& e) const
{
    QJsonObject o;
    const qint64 shift = c.channel.guideShiftSecs;
    o.insert(QLatin1String("start"), double(e.start + shift));
    o.insert(QLatin1String("stop"), double(e.stop + shift));
    o.insert(QLatin1String("title"), e.title);
    if (!e.subtitle.isEmpty())
        o.insert(QLatin1String("subtitle"), e.subtitle);
    if (!e.category.isEmpty())
        o.insert(QLatin1String("category"), e.category);
    if (!e.episode.isEmpty())
        o.insert(QLatin1String("episode"), e.episode);
    o.insert(QLatin1String("hasDescription"), e.descLength > 0);
    // the archive reaches it
    const qint64 now = nowSecs();
    const bool archived = c.channel.catchup != CatchupMode::None && e.start + shift < now
                          && now - (e.start + shift) <= qint64(c.channel.catchupDays) * 86400;
    o.insert(QLatin1String("archived"), archived);
    return o;
}

QJsonObject Service::programmes(const QJsonObject& r)
{
    const qint64 from = qint64(r.value(QLatin1String("from")).toDouble());
    const qint64 to = qint64(r.value(QLatin1String("to")).toDouble());
    std::lock_guard<std::mutex> guard(m_lock);
    QJsonObject result;
    for (const QJsonValue& k : r.value(QLatin1String("keys")).toArray())
    {
        const int index = m_channelIndex.value(k.toString(), -1);
        if (index < 0)
            continue;
        const ChannelEntry& c = m_channels[index];
        QJsonArray list;
        if (c.guide)
        {
            const qint64 shift = c.channel.guideShiftSecs;
            const EntryList& entries = *c.guide;
            // the first one still on at from
            auto it = std::lower_bound(entries.cbegin(), entries.cend(), from - shift,
                                       [](const Entry& e, qint64 v) { return e.stop <= v; });
            for (; it != entries.cend() && it->start + shift < to; ++it)
                list.append(entryJson(c, *it));
        }
        result.insert(k.toString(), list);
    }
    return QJsonObject { { QStringLiteral("programmes"), result } };
}

QJsonObject Service::nowNext(const QJsonObject& r)
{
    const qint64 at = r.contains(QLatin1String("at")) ? qint64(r.value(QLatin1String("at")).toDouble()) : nowSecs();
    const bool withDescription = r.value(QLatin1String("descriptions")).toBool();
    std::lock_guard<std::mutex> guard(m_lock);
    QJsonObject result;
    for (const QJsonValue& k : r.value(QLatin1String("keys")).toArray())
    {
        const int index = m_channelIndex.value(k.toString(), -1);
        if (index < 0 || !m_channels[index].guide)
            continue;
        const ChannelEntry& c = m_channels[index];
        const EntryList& entries = *c.guide;
        const qint64 t = at - c.channel.guideShiftSecs;
        const int now = programmeIndex(entries, t);
        // the next one: after the current, or the first to start later
        int next = now >= 0 ? now + 1
                            : int(std::upper_bound(entries.cbegin(), entries.cend(), t,
                                                   [](qint64 v, const Entry& e) { return v < e.start; })
                                  - entries.cbegin());
        QJsonObject o;
        if (now >= 0)
        {
            QJsonObject e = entryJson(c, entries[now]);
            if (withDescription)
                e.insert(QLatin1String("description"),
                         readDescription(c.guideDescPath, entries[now].descOffset, entries[now].descLength).left(300));
            o.insert(QLatin1String("now"), e);
        }
        if (next < entries.size())
            o.insert(QLatin1String("next"), entryJson(c, entries[next]));
        result.insert(k.toString(), o);
    }
    return QJsonObject { { QStringLiteral("channels"), result } };
}

QJsonObject Service::description(const QJsonObject& r)
{
    const qint64 start = qint64(r.value(QLatin1String("start")).toDouble());
    std::lock_guard<std::mutex> guard(m_lock);
    const int index = m_channelIndex.value(r.value(QLatin1String("key")).toString(), -1);
    if (index < 0 || !m_channels[index].guide)
        return QJsonObject();
    const ChannelEntry& c = m_channels[index];
    const int i = programmeIndex(*c.guide, start - c.channel.guideShiftSecs);
    if (i < 0)
        return QJsonObject();
    const Entry& e = c.guide->at(i);
    return QJsonObject { { QStringLiteral("description"),
                           readDescription(c.guideDescPath, e.descOffset, e.descLength) } };
}

QJsonObject Service::catchup(const QJsonObject& r)
{
    std::lock_guard<std::mutex> guard(m_lock);
    const int index = m_channelIndex.value(r.value(QLatin1String("key")).toString(), -1);
    if (index < 0)
        return QJsonObject { { QStringLiteral("error"), QStringLiteral("no such channel") } };
    const ChannelEntry& c = m_channels[index];
    QJsonObject out;
    out.insert(QLatin1String("name"), c.channel.name);
    out.insert(QLatin1String("number"), c.channel.number);
    out.insert(QLatin1String("logo"), c.channel.logo);
    out.insert(QLatin1String("options"), QJsonArray::fromStringList(c.channel.options));
    if (r.value(QLatin1String("op")).toString() == QLatin1String("play"))
    {
        out.insert(QLatin1String("url"), c.channel.url);
        return out;
    }
    if (c.channel.catchup == CatchupMode::None)
        return QJsonObject { { QStringLiteral("error"), QStringLiteral("no archive") } };
    const qint64 start = qint64(r.value(QLatin1String("start")).toDouble());
    const qint64 stop = qint64(r.value(QLatin1String("stop")).toDouble());
    const QDateTime now = QDateTime::currentDateTimeUtc();
    if (start >= now.toSecsSinceEpoch())
        return QJsonObject { { QStringLiteral("error"), QStringLiteral("not yet") } };
    if (now.toSecsSinceEpoch() - start > qint64(c.channel.catchupDays) * 86400)
        return QJsonObject { { QStringLiteral("error"), QStringLiteral("too old") } };
    // what is still on: up to now; a programme over: its whole length
    const int duration = int(qMax<qint64>(60, qMin(stop, now.toSecsSinceEpoch()) - start));
    const QString url = catchupUrl(c.channel, QDateTime::fromSecsSinceEpoch(start, QTimeZone::utc()),
                                   stop > now.toSecsSinceEpoch() ? int(stop - start) : duration, now);
    if (url.isEmpty())
        return QJsonObject { { QStringLiteral("error"), QStringLiteral("no archive") } };
    out.insert(QLatin1String("url"), url);
    return out;
}

QJsonObject Service::catalog()
{
    const QByteArray online = readAll(m_cacheDir + QStringLiteral("/catalog.json"));
    QJsonObject bundled = QJsonDocument::fromJson(m_bundledCatalog).object();
    const QJsonObject fetched = QJsonDocument::fromJson(online).object();
    // the newer of the two (the dates are ISO: they compare as text)
    if (fetched.value(QLatin1String("version")).toInt() == 1
        && fetched.value(QLatin1String("updated")).toString() >= bundled.value(QLatin1String("updated")).toString())
        bundled = fetched;
    return QJsonObject { { QStringLiteral("catalog"), bundled } };
}

QJsonObject Service::vod(const QJsonObject& r)
{
    SourceConfig cfg;
    {
        std::lock_guard<std::mutex> guard(m_lock);
        const SourceConfig* s = findSource(r.value(QLatin1String("id")).toString());
        if (!s || s->kind != QLatin1String("xtream"))
            return QJsonObject { { QStringLiteral("error"), QStringLiteral("no such account") } };
        cfg = *s;
    }
    const bool series = r.value(QLatin1String("kind")).toString() == QLatin1String("series");
    const xtream::Account account { cfg.server, cfg.username, cfg.password };
    const QString dir = sourceDir(cfg.id);
    const QString listPath = dir + (series ? QStringLiteral("/series.json") : QStringLiteral("/vod_streams.json"));
    const QString categoriesPath = dir + (series ? QStringLiteral("/series_categories.json")
                                                 : QStringLiteral("/vod_categories.json"));
    // a day old at most, or as cached when the server does not answer
    if (ageOf(listPath) < 0 || ageOf(listPath) > 86400 || r.value(QLatin1String("force")).toBool())
    {
        QString error;
        const QByteArray categories = m_host.fetch(
            xtream::apiUrl(account, series ? QStringLiteral("get_series_categories")
                                           : QStringLiteral("get_vod_categories")).toString(QUrl::FullyEncoded), &error);
        const QByteArray list = m_host.fetch(
            xtream::apiUrl(account, series ? QStringLiteral("get_series")
                                           : QStringLiteral("get_vod_streams")).toString(QUrl::FullyEncoded), &error,
            512 << 20);
        if (!list.isEmpty())
        {
            writeAll(categoriesPath, categories);
            writeAll(listPath, list);
        }
    }
    const auto categories = xtream::parseCategories(readAll(categoriesPath));
    const QByteArray data = readAll(listPath);
    const QVector<xtream::VodItem> items = series ? xtream::parseSeries(data, categories)
                                                  : xtream::parseVodStreams(data, account, categories);
    QJsonArray list;
    for (const xtream::VodItem& item : items)
    {
        QJsonObject o;
        o.insert(QLatin1String("id"), item.id);
        o.insert(QLatin1String("name"), item.name);
        o.insert(QLatin1String("poster"), item.poster);
        o.insert(QLatin1String("category"), item.category);
        o.insert(QLatin1String("url"), item.url);
        o.insert(QLatin1String("rating"), item.rating);
        o.insert(QLatin1String("year"), item.year);
        o.insert(QLatin1String("added"), double(item.added));
        o.insert(QLatin1String("adult"), item.adult);
        list.append(o);
    }
    return QJsonObject { { QStringLiteral("items"), list } };
}

QJsonObject Service::seriesInfo(const QJsonObject& r)
{
    SourceConfig cfg;
    {
        std::lock_guard<std::mutex> guard(m_lock);
        const SourceConfig* s = findSource(r.value(QLatin1String("id")).toString());
        if (!s || s->kind != QLatin1String("xtream"))
            return QJsonObject { { QStringLiteral("error"), QStringLiteral("no such account") } };
        cfg = *s;
    }
    const xtream::Account account { cfg.server, cfg.username, cfg.password };
    QString error;
    const QByteArray data = m_host.fetch(
        xtream::apiUrl(account, QStringLiteral("get_series_info"),
                       { { QStringLiteral("series_id"), r.value(QLatin1String("series")).toString() } })
            .toString(QUrl::FullyEncoded), &error);
    const xtream::SeriesInfo info = xtream::parseSeriesInfo(data, account);
    QJsonArray episodes;
    for (const xtream::Episode& e : info.episodes)
        episodes.append(QJsonObject { { QStringLiteral("title"), e.title }, { QStringLiteral("season"), e.season },
                                      { QStringLiteral("number"), e.number }, { QStringLiteral("url"), e.url },
                                      { QStringLiteral("plot"), e.plot }, { QStringLiteral("image"), e.image },
                                      { QStringLiteral("duration"), e.durationSecs } });
    return QJsonObject { { QStringLiteral("plot"), info.plot }, { QStringLiteral("cover"), info.cover },
                         { QStringLiteral("episodes"), episodes } };
}

QJsonObject Service::logo(const QJsonObject& r)
{
    // the logos on disk, a month: a list of channels scrolled shows them at
    // once, offline too
    const QString url = r.value(QLatin1String("url")).toString();
    if (url.isEmpty() || !(url.startsWith(QLatin1String("http://")) || url.startsWith(QLatin1String("https://"))))
        return QJsonObject { { QStringLiteral("error"), QStringLiteral("no logo") } };
    const QString path = m_cacheDir + QStringLiteral("/logos/") + hashOf(url);
    const qint64 age = ageOf(path);
    // a logo that could not be had is asked again an hour later, not at every
    // row shown (a list scrolled asked the server again and again)
    const QString missing = path + QStringLiteral(".none");
    if (age < 0)
    {
        const qint64 failed = ageOf(missing);
        if (failed >= 0 && failed < 3600)
            return QJsonObject { { QStringLiteral("error"), QStringLiteral("no logo") } };
    }
    if (age < 0 || age > 30 * 86400)
    {
        QString error;
        if (!m_host.download(url, path, &error) && age < 0)
        {
            if (!m_quit)
                writeAll(missing, QByteArray());
            return QJsonObject { { QStringLiteral("error"), error } };
        }
        QFile::remove(missing);
    }
    return QJsonObject { { QStringLiteral("path"), path } };
}

}
