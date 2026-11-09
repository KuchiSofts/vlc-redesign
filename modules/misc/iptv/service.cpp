/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
#include "service.hpp"
#include "servers.hpp"
#include "xtream.hpp"

#include <QCryptographicHash>
#include <QDataStream>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QLocale>
#include <QRandomGenerator>
#include <QSaveFile>
#include <QTimeZone>
#include <QUrl>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <exception>
#include <set>
#include <tuple>

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
    // (a file of its own, put in place at once: two threads writing the same
    // file never mix their parts, and a reader never finds it missing)
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size())
        return false;
    return file.commit();
}

// What a server answered is kept when it reads as JSON (not one cut short)
bool isJson(const QByteArray& data)
{
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    return error.error == QJsonParseError::NoError && !doc.isNull();
}

// Seconds of a request or of the settings (a double of JSON): within the
// years a guide can name, never a number out of range of a qint64
qint64 secsOf(const QJsonValue& v)
{
    const double d = v.toDouble();
    if (!std::isfinite(d))
        return 0;
    return qint64(qBound(-1e13, d, 1e13));
}

// The id of a source, as its folder is named: letters, digits, '-' and '_'
// only (never empty, "..", a path)
bool isSourceId(const QString& id)
{
    if (id.isEmpty() || id.size() > 64)
        return false;
    for (const QChar c : id)
    {
        const char16_t u = c.unicode();
        if (!((u >= u'0' && u <= u'9') || (u >= u'a' && u <= u'z') || (u >= u'A' && u <= u'Z') || u == u'-'
              || u == u'_'))
            return false;
    }
    return true;
}

// The servers of the user (their own Jellyfin, Tvheadend...): written as an
// M3U and an XMLTV guide of the source, then read as any other
bool isServerKind(const QString& kind)
{
    static const QSet<QString> kinds = { "jellyfin", "emby", "tvheadend", "nextpvr", "channels", "hdhomerun" };
    return kinds.contains(kind);
}

int defaultPort(const QString& kind)
{
    if (kind == QLatin1String("jellyfin") || kind == QLatin1String("emby"))
        return 8096;
    if (kind == QLatin1String("tvheadend"))
        return 9981;
    if (kind == QLatin1String("nextpvr"))
        return 8866;
    if (kind == QLatin1String("channels"))
        return 8089;
    return 80;
}

QString kindName(const QString& kind)
{
    static const QHash<QString, QString> names = {
        { "jellyfin", "Jellyfin" }, { "emby", "Emby" }, { "tvheadend", "Tvheadend" }, { "nextpvr", "NextPVR" },
        { "channels", "Channels DVR" }, { "hdhomerun", "HDHomeRun" }, { "schedulesdirect", "Schedules Direct" },
    };
    return names.value(kind);
}

// A guide alone, for the channels of every source (Schedules Direct)
bool isGuideOnly(const QString& kind)
{
    return kind == QLatin1String("schedulesdirect");
}

// The guides a source names itself are read: those of its playlist (the
// user's choice) or of its account (always)
bool usesOwnGuides(const SourceConfig& s)
{
    return s.useHeaderGuides || s.kind == QLatin1String("xtream") || isServerKind(s.kind) || isGuideOnly(s.kind);
}

// A playlist naming this guide (the one of its server, written next to it)
QByteArray m3uNamingGuide(const QByteArray& m3u, const QString& guideUrl)
{
    const qsizetype end = m3u.indexOf('\n');
    const QByteArray first = (end < 0 ? m3u : m3u.left(end)).trimmed();
    const QByteArray header = "#EXTM3U url-tvg=\"" + guideUrl.toUtf8() + "\"";
    if (!first.startsWith("#EXTM3U"))
        return header + '\n' + m3u;
    return header + (end < 0 ? QByteArray() : m3u.mid(end));
}

QString replyError(const HttpReply& reply)
{
    if (reply.status == 0)
        return reply.error.isEmpty() ? QStringLiteral("cannot be reached") : reply.error;
    return QStringLiteral("the server answered %1").arg(reply.status);
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

// A channel in the settings of its source: its guide id, else its name
QString identityOf(const Channel& c)
{
    return c.tvgId.isEmpty() ? QStringLiteral("n:") + c.name : QStringLiteral("i:") + c.tvgId;
}

// The keys of a channel in the guides: the guide id the user gave it,
// alone; else its own, then the one the automatic guide found for it
struct ChannelKeys
{
    QStringList keys;
    QString autoKey;
    int autoShift = 0;
};
ChannelKeys keysOf(const SourceConfig& s, const Channel& c)
{
    ChannelKeys out;
    const QString who = identityOf(c);
    const QString mine = s.guideIds.value(who);
    if (!mine.isEmpty())
    {
        out.keys << guideKey(mine);
        return out;
    }
    out.keys = guideKeysOf(c);
    // a feed of iptv-org ("BBCOne.uk@SD"): the channel's id too
    if (c.tvgId.contains(u'@'))
    {
        const QString plain = guideKey(c.tvgId.section(u'@', 0, 0));
        if (!plain.isEmpty() && !out.keys.contains(plain))
            out.keys.insert(1, plain);
    }
    const auto found = s.autoIds.constFind(who);
    if (found != s.autoIds.cend())
    {
        out.autoKey = guideKey(found->id);
        out.autoShift = found->shiftSecs;
        if (!out.keys.contains(out.autoKey))
            out.keys << out.autoKey;
    }
    return out;
}

// The guides of a source in their order: the user's, the playlist's, the
// automatic guide's; those turned off left out (unless all)
QStringList guidesOf(const SourceConfig& s, const QStringList& headerGuides, bool all)
{
    QStringList urls = s.epg;
    if (usesOwnGuides(s))
        urls << headerGuides;
    urls << s.autoEpg;
    urls.removeDuplicates();
    if (!all)
        for (const QString& off : s.epgOff)
            urls.removeAll(off);
    return urls;
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
// 2: no episode from the "original-air-date" and other ids of episode-num
const qint32 kIndexVersion = 2;

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

qint64 retryDelaySecs(int failures, int refreshHours)
{
    if (refreshHours <= 0)
        return -1;
    const qint64 period = qint64(refreshHours) * 3600;
    if (failures <= 0)
        return period;
    // 15 minutes, doubled at each failure, up to the period
    const qint64 delay = qint64(15 * 60) << qMin(failures - 1, 20);
    return qMin(delay, period);
}

bool refreshDue(qint64 now, qint64 playlistUpdated, qint64 lastAttempt, int failures, int refreshHours)
{
    const qint64 delay = retryDelaySecs(failures, refreshHours);
    if (delay < 0)
        return false;
    if (failures > 0)
        return now - lastAttempt >= delay;
    return playlistUpdated == 0 || now - playlistUpdated > delay;
}

EntryList withoutOverlaps(const EntryList& list)
{
    bool overlap = false;
    for (qsizetype i = 1; i < list.size() && !overlap; ++i)
        overlap = list[i].start < list[i - 1].stop;
    if (!overlap)
        return list;
    // the moments a programme starts or ends, and between two of them the
    // shortest of those on (when they are as long, the later start: the
    // next programme starts on time when the one before runs over; then the
    // first in the list). The rule of every app (Android, the Samsung TV app)
    struct Edge
    {
        qint64 at;
        int index;
        bool starts;
    };
    QVector<Edge> edges;
    edges.reserve(list.size() * 2);
    for (int i = 0; i < int(list.size()); ++i)
        if (list[i].stop > list[i].start)
        {
            edges.append({ list[i].start, i, true });
            edges.append({ list[i].stop, i, false });
        }
    std::sort(edges.begin(), edges.end(), [](const Edge& a, const Edge& b) { return a.at < b.at; });
    using Key = std::tuple<qint64, qint64, int>;   // length, -start, index
    std::set<Key> on;
    EntryList out;
    out.reserve(list.size() + list.size() / 2);
    int current = -1;
    qint64 since = 0;
    for (qsizetype k = 0; k < edges.size();)
    {
        const qint64 at = edges[k].at;
        for (; k < edges.size() && edges[k].at == at; ++k)
        {
            const Entry& e = list[edges[k].index];
            const Key key { e.stop - e.start, -e.start, edges[k].index };
            if (edges[k].starts)
                on.insert(key);
            else
                on.erase(key);
        }
        const int chosen = on.empty() ? -1 : std::get<2>(*on.begin());
        if (chosen == current)
            continue;
        // the part of the one shown until now, then the next from here
        if (current >= 0 && at > since)
        {
            Entry part = list[current];
            part.start = since;
            part.stop = at;
            out.append(std::move(part));
        }
        current = chosen;
        since = at;
    }
    return out;
}

QVector<int> programmesBetween(const EntryList& list, qint64 from, qint64 to)
{
    QVector<int> out;
    // the first one starting at from or later
    const auto first = std::lower_bound(list.cbegin(), list.cend(), from,
                                        [](const Entry& e, qint64 v) { return e.start < v; });
    // the ones before still on at from: the one just before, a long one
    // before shorter ones; looked for among those that started up to a day
    // before from (one of more than a day before them is missed)
    auto back = first;
    for (auto it = first; it != list.cbegin();)
    {
        --it;
        if (it->stop > from)
            back = it;
        else if (it->start <= from - 86400)
            break;
    }
    for (auto it = back; it != first; ++it)
        if (it->stop > from && it->start < to)
            out.append(int(it - list.cbegin()));
    for (auto it = first; it != list.cend() && it->start < to; ++it)
        out.append(int(it - list.cbegin()));
    return out;
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
    stop();
    if (m_thread.joinable())
        m_thread.join();
}

void Service::stop()
{
    {
        // (under the lock the worker waits with: its wakeup is never lost,
        // it would sleep until its next check otherwise, a minute)
        std::lock_guard<std::mutex> guard(m_jobLock);
        m_quit = true;
    }
    m_host.killWorker();
    m_jobWait.notify_all();
    // (a request waiting for the keystore's secrets: no longer, 10 s at most
    // if this wakeup comes before its wait)
    m_secretsWait.notify_all();
}

bool Service::waitStopped(int ms)
{
    stop();
    {
        std::unique_lock<std::mutex> guard(m_endLock);
        if (!m_endWait.wait_for(guard, std::chrono::milliseconds(ms), [this]() { return m_workerEnded; }))
        {
            // (stuck in a download: left as it is, with this service)
            m_thread.detach();
            return false;
        }
    }
    if (m_thread.joinable())
        m_thread.join();
    return true;
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
        // (its secrets in VLC's keystore: read by the worker, see loadSecrets;
        // here, as the module loads, a locked keyring's prompt froze the
        // interface)
        s.inKeystore = v.toObject().value(QLatin1String("keystore")).toBool();
        if (s.inKeystore)
            m_secretsReady = false;
        m_sources.append(s);
    }
    const QJsonObject options = root.value(QLatin1String("options")).toObject();
    m_catalogChecked = secsOf(options.value(QLatin1String("catalogChecked")));
}

void Service::loadSecrets()
{
    QStringList ids;
    {
        std::lock_guard<std::mutex> guard(m_lock);
        if (m_secretsReady)
            return;
        for (const SourceConfig& s : std::as_const(m_sources))
            if (s.inKeystore)
                ids << s.id;
    }
    // (out of m_lock: the interface's requests are answered meanwhile)
    QHash<QString, QJsonObject> secrets;
    {
        std::lock_guard<std::mutex> guard(m_secretLock);
        for (const QString& id : std::as_const(ids))
        {
            if (m_quit)
                break;
            bool found = false;
            const QByteArray secret = m_host.loadSecret(id, &found);
            if (found)
                secrets.insert(id, QJsonDocument::fromJson(secret).object());
        }
    }
    std::lock_guard<std::mutex> guard(m_lock);
    for (SourceConfig& s : m_sources)
    {
        const auto it = secrets.constFind(s.id);
        if (it == secrets.cend())
            continue;
        // (what a request set meanwhile wins: only what is still empty)
        if (s.url.isEmpty())
            s.url = it->value(QLatin1String("url")).toString();
        if (s.password.isEmpty())
            s.password = it->value(QLatin1String("password")).toString();
        if (s.key.isEmpty())
            s.key = it->value(QLatin1String("key")).toString();
    }
    m_secretsReady = true;
    m_secretsWait.notify_all();
}

bool Service::secretsReady(std::unique_lock<std::mutex>& guard)
{
    return m_secretsWait.wait_for(guard, std::chrono::seconds(10), [this]() { return m_secretsReady || m_quit; })
           && m_secretsReady;
}

void Service::storeSecrets(const QString& id)
{
    std::lock_guard<std::mutex> secretGuard(m_secretLock);
    QByteArray secret;
    {
        std::lock_guard<std::mutex> guard(m_lock);
        const SourceConfig* s = findSource(id);
        if (!s)
            return;
        secret = QJsonDocument(QJsonObject { { QStringLiteral("url"), s->url },
                                             { QStringLiteral("password"), s->password },
                                             { QStringLiteral("key"), s->key } })
                     .toJson(QJsonDocument::Compact);
    }
    const bool stored = m_host.storeSecret(id, secret);
    bool removed = false;
    {
        std::lock_guard<std::mutex> guard(m_lock);
        SourceConfig* s = findSource(id);
        removed = !s;
        // (no keystore: the secrets in the file, as before)
        if (s && (!stored || !s->inKeystore))
        {
            s->inKeystore = stored;
            saveConfig();
        }
    }
    // (removed while it was stored: not left in the keystore)
    if (removed && stored)
        m_host.removeSecret(id);
}

void Service::saveConfig()
{
    // (under m_lock; never the keystore here: a change of one source's name
    // or of the catalog's date stored every secret, under the lock)
    QJsonArray list;
    for (const SourceConfig& s : std::as_const(m_sources))
    {
        QJsonObject o = sourceToJson(s, false);
        if (s.inKeystore)
            o.insert(QLatin1String("keystore"), true);
        else
        {
            o.insert(QLatin1String("url"), s.url);
            o.insert(QLatin1String("password"), s.password);
            o.insert(QLatin1String("key"), s.key);
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
    o.insert(QLatin1String("epgOff"), QJsonArray::fromStringList(s.epgOff));
    o.insert(QLatin1String("autoEpg"), QJsonArray::fromStringList(s.autoEpg));
    QJsonObject autoIds;
    for (auto it = s.autoIds.cbegin(); it != s.autoIds.cend(); ++it)
        autoIds.insert(it.key(), QJsonArray { it->id, it->country, it->shiftSecs });
    o.insert(QLatin1String("autoIds"), autoIds);
    o.insert(QLatin1String("autoChecked"), double(s.autoChecked));
    o.insert(QLatin1String("autoGuideOff"), s.autoGuideOff);
    o.insert(QLatin1String("autoFound"), s.autoFound);
    o.insert(QLatin1String("autoTotal"), s.autoTotal);
    QJsonObject guideIds;
    for (auto it = s.guideIds.cbegin(); it != s.guideIds.cend(); ++it)
        guideIds.insert(it.key(), it.value());
    o.insert(QLatin1String("guideIds"), guideIds);
    if (withSecrets)
    {
        o.insert(QLatin1String("url"), s.url);
        o.insert(QLatin1String("password"), s.password);
        o.insert(QLatin1String("key"), s.key);
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
    s.key = o.value(QLatin1String("key")).toString().trimmed();
    s.catalogId = o.value(QLatin1String("catalogId")).toString();
    s.enabled = o.value(QLatin1String("enabled")).toBool(true);
    s.useHeaderGuides = o.value(QLatin1String("useHeaderGuides")).toBool(true);
    for (const QJsonValue& e : o.value(QLatin1String("epgOff")).toArray())
        s.epgOff << e.toString();
    for (const QJsonValue& e : o.value(QLatin1String("autoEpg")).toArray())
        s.autoEpg << e.toString();
    const QJsonObject autoIds = o.value(QLatin1String("autoIds")).toObject();
    for (auto it = autoIds.constBegin(); it != autoIds.constEnd(); ++it)
    {
        const QJsonArray a = it.value().toArray();
        autoguide::Match m;
        m.id = a.at(0).toString();
        m.country = a.at(1).toString();
        m.shiftSecs = a.at(2).toInt();
        if (m.isValid())
            s.autoIds.insert(it.key(), m);
    }
    s.autoChecked = secsOf(o.value(QLatin1String("autoChecked")));
    s.autoGuideOff = o.value(QLatin1String("autoGuideOff")).toBool();
    s.autoFound = o.value(QLatin1String("autoFound")).toInt();
    s.autoTotal = o.value(QLatin1String("autoTotal")).toInt();
    const QJsonObject guideIds = o.value(QLatin1String("guideIds")).toObject();
    for (auto it = guideIds.constBegin(); it != guideIds.constEnd(); ++it)
        if (!it.value().toString().trimmed().isEmpty())
            s.guideIds.insert(it.key(), it.value().toString().trimmed());
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
    // the keystore's secrets first, before any job (they are what the
    // downloads ask with), here and not as the module loads
    try
    {
        loadSecrets();
    }
    catch (const std::exception& e)
    {
        jobFailed(e.what());
    }
    {
        // (none read, an exception: what waits for them goes on without)
        std::lock_guard<std::mutex> guard(m_lock);
        m_secretsReady = true;
    }
    m_secretsWait.notify_all();
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
        // (a job that fails, out of memory on an enormous answer, is given
        // up: the worker goes on, an exception would end VLC)
        try
        {
            if (job)
                job();
            else
            {
                nextCheck = std::chrono::steady_clock::now() + std::chrono::seconds(60);
                schedule();
            }
        }
        catch (const std::exception& e)
        {
            jobFailed(e.what());
        }
        catch (...)
        {
            jobFailed("unknown error");
        }
    }
    std::lock_guard<std::mutex> guard(m_endLock);
    m_workerEnded = true;
    m_endWait.notify_all();
}

void Service::jobFailed(const char* what) noexcept
{
    try
    {
        // what it was doing ends: no source stays busy (it would never be
        // refreshed again)
        {
            std::lock_guard<std::mutex> guard(m_lock);
            for (SourceState& state : m_state)
            {
                state.busy = false;
                state.stage.clear();
            }
        }
        m_host.log(QStringLiteral("Live TV: a task failed (%1)").arg(QString::fromUtf8(what)));
        changed("sources");
    }
    catch (...)
    {
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
            if (cfg.enabled && (isGuideOnly(cfg.kind) ? m_state.value(cfg.id).playlistUpdated == 0
                                                      : m_state.value(cfg.id).channels.isEmpty()))
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
        // (a source whose refresh failed: again later and later, not at
        // every check; an account would be refused for signing in so often)
        const qint64 now = nowSecs();
        for (const SourceConfig& s : std::as_const(m_sources))
        {
            const SourceState state = m_state.value(s.id);
            if (s.enabled && !state.busy
                && refreshDue(now, state.playlistUpdated, state.lastAttempt, state.failures, m_refreshHours))
                due << s.id;
        }
    }
    if (!due.isEmpty())
        refreshSources(due, true);
    // the catalog's newer copy: not in the first minutes (VLC may be quit
    // at once; a start downloads enough), else once a day
    if (std::chrono::steady_clock::now() - m_startedAt >= std::chrono::minutes(10))
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
        // (failed: refreshed by itself again later and later, see refreshDue)
        fresh.lastAttempt = nowSecs();
        fresh.failures = fresh.error.isEmpty() ? 0 : state.failures + 1;
        int refreshHours = 0;
        {
            std::lock_guard<std::mutex> guard(m_lock);
            if (!findSource(id))
                continue;
            fresh.busy = guides;
            fresh.stage = guides ? QStringLiteral("guide") : QString();
            m_state.insert(id, fresh);
            refreshHours = m_refreshHours;
        }
        // (its host only in the log: the rest of a link is often a token)
        if (fresh.failures > 0)
        {
            const qint64 retry = retryDelaySecs(fresh.failures, refreshHours);
            m_host.log(QStringLiteral("Live TV: %1 could not be refreshed (%2), %3")
                           .arg(cfg.kind == QLatin1String("m3u") ? hostOf(cfg.url) : hostOf(asUrl(cfg.server)),
                                fresh.error,
                                retry < 0 ? QStringLiteral("not again by itself")
                                          : QStringLiteral("again in %1 min").arg(retry / 60)));
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
    // the channels the guides miss: the automatic guide, once a week
    if (guides)
        for (const QString& id : std::as_const(done))
            if (!m_quit)
                findGuide(id, true);
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
                // (kept only when they read: an answer cut short, VLC quitting
                // or the connection lost, would replace the channels kept)
                if (streams.isEmpty())
                    state.error = error;
                else if (!isJson(streams))
                    state.error = QStringLiteral("the list of channels could not be read");
                else
                {
                    if (isJson(categories))
                        writeAll(categoriesPath, categories);
                    writeAll(streamsPath, streams);
                    state.error.clear();
                }
                // what else the account has: films, series
                const QByteArray vodCategories = m_host.fetch(
                    xtream::apiUrl(account, QStringLiteral("get_vod_categories")).toString(QUrl::FullyEncoded), &error);
                const QByteArray seriesCategories = m_host.fetch(
                    xtream::apiUrl(account, QStringLiteral("get_series_categories")).toString(QUrl::FullyEncoded), &error);
                if (isJson(vodCategories) && isJson(seriesCategories))
                    writeAll(dir + QStringLiteral("/library.json"),
                             QJsonDocument(QJsonObject {
                                 { QStringLiteral("vod"), !xtream::parseCategories(vodCategories).isEmpty() },
                                 { QStringLiteral("series"), !xtream::parseCategories(seriesCategories).isEmpty() } })
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

    if (isGuideOnly(cfg.kind))
    {
        // no channels: its guide (guide.xml) serves those of every source
        if (download && downloadSchedulesDirect(cfg, state))
            state.error.clear();
        const QString guidePath = dir + QStringLiteral("/guide.xml");
        state.channels.clear();
        if (!QFileInfo::exists(guidePath))
        {
            if (state.error.isEmpty() && download)
                state.error = QStringLiteral("no guide yet");
            return false;
        }
        state.headerGuides = QStringList { QUrl::fromLocalFile(guidePath).toString(QUrl::FullyEncoded) };
        state.playlistUpdated = QFileInfo(guidePath).lastModified().toSecsSinceEpoch();
        return state.error.isEmpty();
    }

    const QString path = dir + QStringLiteral("/playlist.m3u");
    QString base = asUrl(cfg.url);
    if (isServerKind(cfg.kind))
    {
        // asked by its API, written here (its playlist.m3u and guide.xml)
        base = servers::baseUrl(cfg.server, defaultPort(cfg.kind));
        if (download && downloadServer(cfg, state))
            state.error.clear();
    }
    else if (download && !cfg.username.isEmpty())
    {
        // an M3U behind a login (Basic or Digest)
        progress(cfg.id, QStringLiteral("playlist"), 0, 0);
        HttpCall call;
        call.url = asUrl(cfg.url);
        call.username = cfg.username;
        call.password = cfg.password;
        call.timeoutMs = 30000;
        call.maxBytes = 256 << 20;
        const HttpReply reply = m_host.call(call);
        state.refused = reply.status == 401 || reply.status == 403;
        // (an answer cut short, its time out or too large, is not kept)
        if (reply.status == 200 && reply.error.isEmpty() && !reply.body.isEmpty() && writeAll(path, reply.body))
            state.error.clear();
        else
            state.error = state.refused ? QStringLiteral("sign-in refused")
                        : reply.status == 200 && !reply.error.isEmpty() ? reply.error
                                                                         : replyError(reply);
    }
    else if (download)
    {
        if (m_host.download(asUrl(cfg.url), path, &error, kPlaylistMaxBytes, kIdleSecs,
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
    Playlist list = parseM3u(data, base);
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
            const QStringList urls = guidesOf(s, state.headerGuides, false);
            for (const QString& url : urls)
                if (std::none_of(wanted.cbegin(), wanted.cend(), [&](const Wanted& w) { return w.url == url; }))
                    wanted.append({ url, s.id });
            for (const Channel& c : state.channels)
                for (const QString& k : keysOf(s, c).keys)
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
            if (!m_host.download(asUrl(w.url), path, &error, kGuideMaxBytes, kIdleSecs,
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
            // (a long programme that contains shorter ones: they win their
            // slots, it shows around them; every app's rule)
            *list = withoutOverlaps(*list);
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
        const QStringList urls = guidesOf(s, m_state.value(s.id).headerGuides, false);
        for (const QString& url : urls)
        {
            const auto g = m_guides.value(url);
            if (g && !ordered.contains(g))
                ordered.append(g);
        }
    }
    int sequence = 0;
    // the last "#n" given to a key seen twice: the next one goes on from it
    // (looking from #2 again each time took minutes for a list of thousands
    // of entries of one name, under the lock every request waits for)
    QHash<QString, int> lastSuffix;
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
                // (the same keys as before: #2, #3... in the order of the list;
                // one a channel's own name already has is skipped)
                int& n = lastSuffix[key];
                if (n < 2)
                    n = 1;
                QString numbered;
                do
                    numbered = key + QLatin1Char('#') + QString::number(++n);
                while (m_channelIndex.contains(numbered));
                key = numbered;
            }
            entry.key = key;
            const ChannelKeys keys = keysOf(s, c);
            for (const auto& g : std::as_const(guides))
            {
                std::shared_ptr<const EntryList> found;
                QString by;
                for (const QString& k : keys.keys)
                    if ((found = g->byKey.value(k)))
                    {
                        by = k;
                        break;
                    }
                if (!found)
                    for (const QString& k : keys.keys)
                    {
                        const QString id = g->idByName.value(k);
                        if (!id.isEmpty() && (found = g->byKey.value(id)))
                        {
                            by = k;
                            break;
                        }
                    }
                if (found && !found->isEmpty())
                {
                    entry.guide = found;
                    entry.guideDescPath = g->descPath;
                    // a "+1" channel: the guide of the one it repeats, later
                    if (!keys.autoKey.isEmpty() && by == keys.autoKey)
                        entry.channel.guideShiftSecs += keys.autoShift;
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
    // the copy of the project's website (its lists and guides checked every
    // day), else of its repository; quietly nothing when neither answers
    QString error;
    QByteArray data = m_host.fetch(QStringLiteral("https://kuchisofts.github.io/vlc-redesign/tv/catalog.json"),
                                   &error, 8 << 20);
    if (QJsonDocument::fromJson(data).object().value(QLatin1String("version")).toInt() != 1)
        data = m_host.fetch(QStringLiteral(
            "https://raw.githubusercontent.com/KuchiSofts/vlc-redesign/main/modules/misc/iptv/catalog/catalog.json"),
            &error, 8 << 20);
    const QJsonObject root = QJsonDocument::fromJson(data).object();
    if (root.value(QLatin1String("version")).toInt() != 1 || !root.value(QLatin1String("sources")).isArray())
        return;
    if (writeAll(m_cacheDir + QStringLiteral("/catalog.json"), data))
        changed("catalog");
}

/* The automatic guide: the channels without a guide matched to iptv-org's
 * database (their guide id, else their name and country), and the free
 * guides of the catalog that carry their countries attached. */

QVector<autoguide::Known> Service::knownChannels(QString* error)
{
    // (8 MB: once a week, only the fields the matching needs kept)
    const QString path = m_cacheDir + QStringLiteral("/iptv-org-channels.json");
    const qint64 age = ageOf(path);
    if (age < 0 || age > 7 * 86400)
    {
        const QString part = path + QStringLiteral(".part");
        if (m_host.download(QStringLiteral("https://iptv-org.github.io/api/channels.json"), part, error,
                            128LL << 20, kIdleSecs))
        {
            QByteArray data = readAll(part);
            if (isGzip(data))
                data = gunzip(data, 256 << 20);
            const QVector<autoguide::Known> fresh = autoguide::parseIptvOrgChannels(data);
            QFile::remove(part);
            if (!fresh.isEmpty())
            {
                writeAll(path, autoguide::compactChannels(fresh));
                return fresh;
            }
            if (error)
                *error = QStringLiteral("iptv-org's list of channels could not be read");
        }
        QFile::remove(part);
    }
    return autoguide::readCompactChannels(readAll(path));
}

void Service::findGuide(const QString& id, bool automatic)
{
    SourceConfig cfg;
    QVector<Channel> channels;
    QStringList headerGuides;
    QSet<QString> covered;
    {
        std::lock_guard<std::mutex> guard(m_lock);
        const SourceConfig* s = findSource(id);
        // (a server of the user has its own guide)
        if (!s || !s->enabled || isServerKind(s->kind))
            return;
        if (automatic && (s->autoGuideOff || nowSecs() - s->autoChecked < 7 * 86400))
            return;
        cfg = *s;
        channels = m_state.value(id).channels;
        headerGuides = m_state.value(id).headerGuides;
        for (const ChannelEntry& c : std::as_const(m_channels))
            if (c.sourceId == id && c.guide)
                covered.insert(identityOf(c.channel));
    }
    const int total = int(channels.size());
    const int before = int(covered.size());
    // a guide that has nearly every channel: nothing to look for
    if (total == 0 || (automatic && (total - before < 3 || (total - before) * 10 < total)))
        return;
    QElapsedTimer took;
    took.start();
    progress(id, QStringLiteral("guide"), 0, 0);
    QString error;
    const QVector<autoguide::Known> known = knownChannels(&error);
    QHash<QString, autoguide::Match> ids;
    QStringList guides;
    if (!known.isEmpty())
    {
        const autoguide::Matcher matcher(known);
        const QJsonArray catalogSources = catalog().value(QLatin1String("catalog")).toObject()
                                              .value(QLatin1String("sources")).toArray();
        // the country of the list (a catalog entry of one country), the user's
        QString listCountry;
        for (const QJsonValue& v : catalogSources)
        {
            const QJsonObject o = v.toObject();
            const QJsonArray countries = o.value(QLatin1String("countries")).toArray();
            if (!cfg.catalogId.isEmpty() && o.value(QLatin1String("id")).toString() == cfg.catalogId
                && countries.size() == 1)
                listCountry = countries.at(0).toString().toUpper();
        }
        if (listCountry == QLatin1String("UK"))
            listCountry = QStringLiteral("GB");
        const QString preferred = QLocale::system().name().section(u'_', 1, 1).toUpper();
        QHash<QString, int> byCountry;
        for (const Channel& c : std::as_const(channels))
        {
            const QString who = identityOf(c);
            if (covered.contains(who) || cfg.guideIds.contains(who))
                continue;
            const autoguide::Match m = matcher.match(c.tvgId, c.name, listCountry, preferred);
            if (!m.isValid())
                continue;
            ids.insert(who, m);
            ++byCountry[m.country];
        }
        QStringList skip = cfg.epg;
        skip << headerGuides << cfg.autoEpg;
        guides = autoguide::pickGuides(catalogSources, byCountry, skip);
    }
    // (what it found before stays: the channels its guides cover)
    const QStringList previousEpg = cfg.autoEpg;
    const QHash<QString, autoguide::Match> previousIds = cfg.autoIds;
    // the user removed the automatic guide, or one of its guides, while it
    // looked: what it found is not put back
    auto editedMeanwhile = [&](const SourceConfig* s) { return s->autoEdits != cfg.autoEdits; };
    bool edited = false;
    {
        std::lock_guard<std::mutex> guard(m_lock);
        SourceConfig* s = findSource(id);
        if (!s)
            return;
        edited = editedMeanwhile(s);
        if (!edited)
        {
            for (auto it = ids.cbegin(); it != ids.cend(); ++it)
                s->autoIds.insert(it.key(), it.value());
            for (const QString& url : std::as_const(guides))
                if (!s->autoEpg.contains(url))
                    s->autoEpg << url;
            s->autoChecked = nowSecs();
            s->autoGuideOff = false;
            saveConfig();
        }
    }
    if (edited)
    {
        progress(id, QString(), 0, 0);
        changed("sources");
        return;
    }
    m_host.debug(QStringLiteral("automatic guide of %1: %2 of %3 channels matched (%4 of them without a guide), "
                                "%5 guides, in %6 ms")
                     .arg(cfg.name).arg(ids.size()).arg(total).arg(total - before).arg(guides.size())
                     .arg(took.elapsed()));
    if (!guides.isEmpty() || !ids.isEmpty())
    {
        loadGuides(true, { id });
        rebuild();
    }
    progress(id, QString(), 0, 0);
    int found = 0;
    {
        std::lock_guard<std::mutex> guard(m_lock);
        for (const ChannelEntry& c : std::as_const(m_channels))
            if (c.sourceId == id && c.guide)
                ++found;
        SourceConfig* s = findSource(id);
        // (removed by the user while its guides were read: left as the user
        // made it, its guides read again by the removal)
        edited = !s || editedMeanwhile(s);
        if (!edited)
        {
            // nothing more found: as it was
            if (found <= before)
            {
                s->autoEpg = previousEpg;
                s->autoIds = previousIds;
            }
            s->autoFound = qMax(found, before);
            s->autoTotal = total;
            saveConfig();
        }
    }
    if (!edited && found <= before && (!guides.isEmpty() || !ids.isEmpty()))
    {
        loadGuides(false, {});
        rebuild();
    }
    changed("guide");
    changed("sources");
    // what the user is told (quietly nothing when it found nothing by itself)
    if (!edited && (found > before || !automatic))
    {
        QJsonObject event;
        event.insert(QLatin1String("event"), QStringLiteral("guideFound"));
        event.insert(QLatin1String("id"), id);
        event.insert(QLatin1String("name"), cfg.name);
        event.insert(QLatin1String("found"), found);
        event.insert(QLatin1String("before"), before);
        event.insert(QLatin1String("total"), total);
        event.insert(QLatin1String("error"), known.isEmpty() ? error : QString());
        m_host.notify(event);
    }
}

/* The servers of the user: Jellyfin, Emby, Tvheadend, NextPVR, Channels
 * DVR, HDHomeRun. What they answer is written as the source's playlist.m3u
 * and guide.xml; the rest of the service reads them as any other source. */

namespace {

HttpCall jsonCall(const QString& url, const QList<QPair<QByteArray, QByteArray>>& headers = {})
{
    HttpCall call;
    call.url = url;
    call.headers = headers;
    call.headers.append(qMakePair(QByteArray("Accept"), QByteArray("application/json")));
    return call;
}

QList<QPair<QByteArray, QByteArray>> mediaBrowserHeaders(Host& host, const QString& token)
{
    const QByteArray value = servers::mediaBrowserAuthorization(host.deviceId(), QStringLiteral("1.0"), token);
    // (Jellyfin reads Authorization, Emby X-Emby-Authorization)
    return { { "Authorization", value }, { "X-Emby-Authorization", value }, { "Content-Type", "application/json" } };
}

} // namespace

bool Service::mediaBrowserSession(const SourceConfig& cfg, QString* token, QString* userId, QString* error,
                                  bool* refused)
{
    *refused = false;
    {
        std::lock_guard<std::mutex> guard(m_lock);
        const auto it = m_sessions.constFind(cfg.id);
        if (!cfg.id.isEmpty() && it != m_sessions.cend())
        {
            *token = it->first;
            *userId = it->second;
            return true;
        }
    }
    const bool emby = cfg.kind == QLatin1String("emby");
    const QString base = servers::baseUrl(cfg.server, defaultPort(cfg.kind));
    if (cfg.username.isEmpty() && !cfg.key.isEmpty())
    {
        // an API key: the server's, no user (the channels of every user)
        HttpReply reply = m_host.call(jsonCall(base + (emby ? QStringLiteral("/emby") : QString()) + QStringLiteral("/System/Info"),
                                               mediaBrowserHeaders(m_host, cfg.key)));
        if (reply.status != 200)
        {
            *refused = reply.status == 401 || reply.status == 403;
            *error = *refused ? QStringLiteral("sign-in refused") : replyError(reply);
            return false;
        }
        *token = cfg.key;
        userId->clear();
    }
    else
    {
        HttpCall login = jsonCall(base + (emby ? QStringLiteral("/emby") : QString()) + QStringLiteral("/Users/AuthenticateByName"),
                                  mediaBrowserHeaders(m_host, QString()));
        login.method = "POST";
        login.body = servers::mediaBrowserLoginBody(cfg.username, cfg.password);
        const HttpReply reply = m_host.call(login);
        const servers::Session session = servers::parseMediaBrowserLogin(reply.body);
        if (reply.status != 200 || session.token.isEmpty())
        {
            *refused = reply.status == 401 || reply.status == 403 || (reply.status == 200 && session.token.isEmpty());
            *error = *refused ? QStringLiteral("sign-in refused") : replyError(reply);
            return false;
        }
        *token = session.token;
        *userId = session.userId;
    }
    if (!cfg.id.isEmpty())
    {
        std::lock_guard<std::mutex> guard(m_lock);
        m_sessions.insert(cfg.id, { *token, *userId });
    }
    return true;
}

bool Service::downloadServer(const SourceConfig& cfg, SourceState& state)
{
    const QString dir = sourceDir(cfg.id);
    QDir().mkpath(dir);
    const QString playlistPath = dir + QStringLiteral("/playlist.m3u");
    const QString guidePath = dir + QStringLiteral("/guide.xml");
    const QString guideUrl = QUrl::fromLocalFile(guidePath).toString(QUrl::FullyEncoded);
    const QString base = servers::baseUrl(cfg.server, defaultPort(cfg.kind));
    state.refused = false;
    if (base.isEmpty())
    {
        state.error = QStringLiteral("the server's address is not one");
        return false;
    }
    int days;
    {
        std::lock_guard<std::mutex> guard(m_lock);
        days = m_guideDays;
    }
    const qint64 from = nowSecs() - 3 * 3600;
    const qint64 to = nowSecs() + qint64(qMin(days, 7)) * 86400;
    QElapsedTimer took;
    took.start();
    auto refusedBy = [&state](const HttpReply& reply) {
        state.refused = reply.status == 401 || reply.status == 403;
        state.error = state.refused ? QStringLiteral("sign-in refused") : replyError(reply);
        return false;
    };

    // Tvheadend and Channels DVR: their own M3U and XMLTV
    if (cfg.kind == QLatin1String("tvheadend") || cfg.kind == QLatin1String("channels"))
    {
        const bool tvh = cfg.kind == QLatin1String("tvheadend");
        HttpCall call;
        call.url = (tvh ? servers::tvheadendPlaylistUrl(base) : servers::channelsPlaylistUrl(base)).toString(QUrl::FullyEncoded);
        call.username = cfg.username;
        call.password = cfg.password;
        call.timeoutMs = 30000;
        const HttpReply playlist = m_host.call(call);
        // (an answer cut short, its time out, is not kept)
        if (playlist.status == 200 && !playlist.error.isEmpty())
        {
            state.error = playlist.error;
            return false;
        }
        if (playlist.status != 200 || playlist.body.isEmpty())
            return refusedBy(playlist);
        call.url = (tvh ? servers::tvheadendGuideUrl(base) : servers::channelsGuideUrl(base)).toString(QUrl::FullyEncoded);
        call.timeoutMs = 120000;
        call.maxBytes = 512 << 20;
        const HttpReply guide = m_host.call(call);
        if (guide.status == 200 && guide.error.isEmpty() && !guide.body.isEmpty())
            writeAll(guidePath, guide.body);
        const QByteArray m3u = tvh ? servers::tvheadendWithUser(playlist.body, cfg.username, cfg.password) : playlist.body;
        m_host.debug(QStringLiteral("%1: its playlist and guide in %2 ms").arg(cfg.kind).arg(took.elapsed()));
        return writeAll(playlistPath, m3uNamingGuide(m3u, guideUrl));
    }

    QVector<servers::Channel> channels;
    QVector<servers::Programme> programmes;
    if (cfg.kind == QLatin1String("hdhomerun"))
    {
        const HttpReply discover = m_host.call(jsonCall(base + QStringLiteral("/discover.json")));
        if (discover.status != 200)
            return refusedBy(discover);
        const servers::Tuner tuner = servers::parseHdhrDiscover(discover.body);
        const HttpReply lineup = m_host.call(jsonCall(tuner.lineupUrl.isEmpty() ? base + QStringLiteral("/lineup.json")
                                                                                : tuner.lineupUrl));
        if (lineup.status != 200)
            return refusedBy(lineup);
        channels = servers::parseHdhrLineup(lineup.body);
        // its free guide (the next hours), by the tuner's DeviceAuth
        if (!tuner.deviceAuth.isEmpty())
        {
            const HttpReply guide = m_host.call(jsonCall(servers::hdhrGuideUrl(tuner.deviceAuth).toString(QUrl::FullyEncoded)));
            if (guide.status == 200)
                programmes = servers::parseHdhrGuide(guide.body);
        }
    }
    else if (cfg.kind == QLatin1String("jellyfin") || cfg.kind == QLatin1String("emby"))
    {
        const bool emby = cfg.kind == QLatin1String("emby");
        QString token, userId, error;
        bool refused = false;
        if (!mediaBrowserSession(cfg, &token, &userId, &error, &refused))
        {
            state.refused = refused;
            state.error = error;
            return false;
        }
        const auto headers = mediaBrowserHeaders(m_host, token);
        HttpReply list = m_host.call(jsonCall(servers::mediaBrowserChannelsUrl(base, userId, emby).toString(QUrl::FullyEncoded), headers));
        if (list.status == 401)
        {
            // (its token ran out: signed in again, once)
            {
                std::lock_guard<std::mutex> guard(m_lock);
                m_sessions.remove(cfg.id);
            }
            if (!mediaBrowserSession(cfg, &token, &userId, &error, &refused))
            {
                state.refused = refused;
                state.error = error;
                return false;
            }
            list = m_host.call(jsonCall(servers::mediaBrowserChannelsUrl(base, userId, emby).toString(QUrl::FullyEncoded),
                                        mediaBrowserHeaders(m_host, token)));
        }
        if (list.status != 200)
            return refusedBy(list);
        channels = servers::parseMediaBrowserChannels(list.body, base, emby);
        HttpCall guide = jsonCall(servers::mediaBrowserProgrammesUrl(base, userId, from, to, emby).toString(QUrl::FullyEncoded),
                                  mediaBrowserHeaders(m_host, token));
        guide.timeoutMs = 120000;
        const HttpReply answer = m_host.call(guide);
        if (answer.status == 200)
            programmes = servers::parseMediaBrowserProgrammes(answer.body);
        // (the logos need no token: the server shows its images to anyone, and the
        // token stays out of the playlist written to the disk and out of the logs)
    }
    else if (cfg.kind == QLatin1String("nextpvr"))
    {
        const HttpReply initiate = m_host.call(jsonCall(servers::nextPvrInitiateUrl(base).toString(QUrl::FullyEncoded)));
        const servers::Salted salted = servers::parseNextPvrInitiate(initiate.body);
        if (initiate.status != 200 || salted.sid.isEmpty())
            return refusedBy(initiate);
        // (its PIN is 0000 until the user sets one)
        const QString pin = cfg.password.isEmpty() ? QStringLiteral("0000") : cfg.password;
        const HttpReply login = m_host.call(jsonCall(
            servers::nextPvrLoginUrl(base, salted.sid, servers::nextPvrLoginKey(pin, salted.salt)).toString(QUrl::FullyEncoded)));
        if (login.status != 200 || !servers::parseNextPvrLogin(login.body))
        {
            state.refused = login.status == 200 || login.status == 401 || login.status == 403;
            state.error = state.refused ? QStringLiteral("sign-in refused") : replyError(login);
            return false;
        }
        const HttpReply list = m_host.call(jsonCall(servers::nextPvrChannelsUrl(base, salted.sid).toString(QUrl::FullyEncoded)));
        if (list.status != 200)
            return refusedBy(list);
        channels = servers::parseNextPvrChannels(list.body, base, salted.sid);
        // their listings, a channel at a time (at most 300)
        for (int i = 0; i < channels.size() && i < 300 && !m_quit; ++i)
        {
            const HttpReply listings = m_host.call(jsonCall(
                servers::nextPvrListingsUrl(base, salted.sid, channels[i].id, from, to).toString(QUrl::FullyEncoded)));
            if (listings.status == 200)
                programmes += servers::parseNextPvrListings(listings.body, channels[i].id);
        }
    }
    else
    {
        state.error = QStringLiteral("unknown kind of server");
        return false;
    }
    if (channels.isEmpty())
    {
        state.error = QStringLiteral("the server has no TV channels");
        return false;
    }
    m_host.debug(QStringLiteral("%1: %2 channels and %3 programmes in %4 ms")
                     .arg(cfg.kind).arg(channels.size()).arg(programmes.size()).arg(took.elapsed()));
    writeAll(guidePath, servers::xmltvOf(channels, programmes));
    return writeAll(playlistPath, servers::m3uOf(channels, guideUrl));
}

QString Service::openServerStream(const QString& sourceId, const QString& channelUrl, QString* error)
{
    SourceConfig cfg;
    {
        std::unique_lock<std::mutex> guard(m_lock);
        if (!secretsReady(guard))
        {
            *error = QStringLiteral("the keystore is not open yet");
            return {};
        }
        const SourceConfig* s = findSource(sourceId);
        if (!s)
            return {};
        cfg = *s;
    }
    const bool emby = cfg.kind == QLatin1String("emby");
    const QString base = servers::baseUrl(cfg.server, defaultPort(cfg.kind));
    // (".../Videos/<id>/stream.ts")
    const QStringList parts = QUrl(channelUrl).path().split(QLatin1Char('/'), Qt::SkipEmptyParts);
    const qsizetype at = parts.indexOf(QStringLiteral("Videos"));
    if (at < 0 || at + 1 >= parts.size())
        return {};
    const QString channelId = parts[at + 1];
    for (int attempt = 0; attempt < 2; ++attempt)
    {
        QString token, userId;
        bool refused = false;
        if (!mediaBrowserSession(cfg, &token, &userId, error, &refused))
            return {};
        HttpCall info = jsonCall(servers::mediaBrowserPlaybackInfoUrl(base, channelId, userId, emby).toString(QUrl::FullyEncoded),
                                 mediaBrowserHeaders(m_host, token));
        info.method = "POST";
        info.body = servers::mediaBrowserPlaybackInfoBody();
        info.timeoutMs = 30000;
        const HttpReply reply = m_host.call(info);
        if (reply.status == 401 && attempt == 0)
        {
            std::lock_guard<std::mutex> guard(m_lock);
            m_sessions.remove(cfg.id);
            continue;
        }
        if (reply.status != 200)
        {
            *error = replyError(reply);
            return {};
        }
        const QString url = servers::mediaBrowserStreamUrl(reply.body, base, channelId, token, emby);
        if (url.isEmpty())
            *error = QStringLiteral("the server gave no stream");
        return url;
    }
    return {};
}

bool Service::downloadSchedulesDirect(const SourceConfig& cfg, SourceState& state)
{
    const QString dir = sourceDir(cfg.id);
    QDir().mkpath(dir);
    state.refused = false;
    int days;
    {
        std::lock_guard<std::mutex> guard(m_lock);
        days = qBound(1, m_guideDays, 14);
    }
    QElapsedTimer took;
    took.start();
    progress(cfg.id, QStringLiteral("guide"), 0, 0);
    // its token
    HttpCall login = jsonCall(servers::sdBase() + QStringLiteral("/token"));
    login.method = "POST";
    login.body = servers::sdTokenBody(cfg.username, servers::sdPasswordHash(cfg.password));
    login.headers.append(qMakePair(QByteArray("Content-Type"), QByteArray("application/json")));
    const HttpReply signedIn = m_host.call(login);
    const servers::SdAnswer token = servers::parseSdToken(signedIn.body);
    if (token.token.isEmpty())
    {
        state.refused = (token.code >= 4001 && token.code <= 4005) || signedIn.status == 401 || signedIn.status == 403;
        state.error = state.refused ? QStringLiteral("sign-in refused")
                    : !token.message.isEmpty() ? token.message : replyError(signedIn);
        return false;
    }
    const QList<QPair<QByteArray, QByteArray>> headers {
        qMakePair(QByteArray("token"), token.token.toUtf8()),
        qMakePair(QByteArray("Content-Type"), QByteArray("application/json")) };
    // the lineups of the account, their stations
    const HttpReply lineups = m_host.call(jsonCall(servers::sdBase() + QStringLiteral("/lineups"), headers));
    int code = -1;
    const QStringList uris = servers::parseSdLineups(lineups.body, &code);
    if (uris.isEmpty())
    {
        state.error = code == 4102 ? QStringLiteral("no lineup in the account: add one on schedulesdirect.org")
                                   : replyError(lineups);
        return false;
    }
    QVector<servers::Channel> stations;
    QSet<QString> seen;
    for (const QString& uri : uris.mid(0, 4))
    {
        const HttpReply lineup = m_host.call(jsonCall(QStringLiteral("https://json.schedulesdirect.org") + uri, headers));
        for (const servers::Channel& c : servers::parseSdLineup(lineup.body))
            if (!seen.contains(c.id))
            {
                seen.insert(c.id);
                stations.append(c);
            }
    }
    if (stations.isEmpty())
    {
        state.error = QStringLiteral("the lineups have no stations");
        return false;
    }
    // their schedules, then the programmes they name (5000 a request)
    QStringList ids;
    for (const servers::Channel& c : std::as_const(stations))
        ids << c.id;
    QVector<servers::SdAiring> airings;
    for (qsizetype i = 0; i < ids.size(); i += 5000)
    {
        HttpCall call = jsonCall(servers::sdBase() + QStringLiteral("/schedules"), headers);
        call.method = "POST";
        call.body = servers::sdSchedulesBody(ids.mid(i, 5000), nowSecs(), days);
        call.timeoutMs = 120000;
        call.maxBytes = 256 << 20;
        const HttpReply reply = m_host.call(call);
        if (reply.status != 200)
        {
            state.error = replyError(reply);
            return false;
        }
        airings += servers::parseSdSchedules(reply.body);
        if (m_quit)
            return false;
    }
    QStringList programs;
    {
        QSet<QString> unique;
        for (const servers::SdAiring& a : std::as_const(airings))
            if (!unique.contains(a.program))
            {
                unique.insert(a.program);
                programs << a.program;
            }
    }
    QHash<QString, servers::Programme> details;
    for (qsizetype i = 0; i < programs.size(); i += 5000)
    {
        HttpCall call = jsonCall(servers::sdBase() + QStringLiteral("/programs"), headers);
        call.method = "POST";
        call.body = servers::sdProgramsBody(programs.mid(i, 5000));
        call.timeoutMs = 120000;
        call.maxBytes = 256 << 20;
        const HttpReply reply = m_host.call(call);
        if (reply.status != 200)
        {
            state.error = replyError(reply);
            return false;
        }
        details.insert(servers::parseSdPrograms(reply.body));
        progress(cfg.id, QStringLiteral("guide"), qint64(i + 5000) * 1024, qint64(programs.size()) * 1024);
        if (m_quit)
            return false;
    }
    QVector<servers::Programme> programmes;
    programmes.reserve(airings.size());
    for (const servers::SdAiring& a : std::as_const(airings))
    {
        const auto it = details.constFind(a.program);
        if (it == details.cend())
            continue;
        servers::Programme p = *it;
        p.channel = a.station;
        p.start = a.start;
        p.stop = a.start + a.duration;
        programmes.append(p);
    }
    if (!writeAll(dir + QStringLiteral("/guide.xml"), servers::xmltvOf(stations, programmes)))
    {
        state.error = QStringLiteral("the guide could not be written");
        return false;
    }
    m_host.debug(QStringLiteral("Schedules Direct: %1 stations and %2 programmes in %3 ms")
                     .arg(stations.size()).arg(programmes.size()).arg(took.elapsed()));
    return true;
}

QJsonObject Service::testSource(const QJsonObject& r)
{
    SourceConfig s = sourceFromJson(r.value(QLatin1String("source")).toObject());
    // (an empty password or key of a source kept: its own)
    const QString id = r.value(QLatin1String("id")).toString();
    if (!id.isEmpty())
    {
        std::unique_lock<std::mutex> guard(m_lock);
        // (its own secrets: read from the keystore first)
        if (!secretsReady(guard))
            return QJsonObject { { QStringLiteral("result"), QStringLiteral("noAnswer") },
                                 { QStringLiteral("error"), QStringLiteral("the keystore is not open yet") } };
        if (const SourceConfig* old = findSource(id))
        {
            if (s.password.isEmpty())
                s.password = old->password;
            if (s.key.isEmpty())
                s.key = old->key;
        }
    }
    s.id.clear();   // (no session kept for a test)
    auto answer = [](const char* result, const QString& who = {}, int channels = -1) {
        QJsonObject o { { QStringLiteral("result"), QLatin1String(result) } };
        if (!who.isEmpty())
            o.insert(QLatin1String("who"), who);
        if (channels >= 0)
            o.insert(QLatin1String("channels"), channels);
        return o;
    };
    auto ofReply = [&](const HttpReply& reply, const QString& who) {
        if (reply.status == 200)
            return answer(who.isEmpty() ? "works" : "signedIn", who);
        if (reply.status == 401 || reply.status == 403)
            return answer("refused");
        return answer("noAnswer");
    };
    const QString base = servers::baseUrl(s.server, defaultPort(s.kind));
    if (s.kind == QLatin1String("xtream"))
    {
        QString error;
        const xtream::Account account { s.server, s.username, s.password };
        const QByteArray info = m_host.fetch(xtream::apiUrl(account).toString(QUrl::FullyEncoded), &error);
        if (info.isEmpty())
            return answer("noAnswer");
        return xtream::parseAccountInfo(info).authenticated ? answer("signedIn", s.username) : answer("refused");
    }
    if (s.kind == QLatin1String("m3u"))
    {
        HttpCall call;
        call.url = asUrl(s.url);
        call.username = s.username;
        call.password = s.password;
        call.maxBytes = 64 << 20;
        return ofReply(m_host.call(call), s.username);
    }
    if (isGuideOnly(s.kind))
    {
        HttpCall login = jsonCall(servers::sdBase() + QStringLiteral("/token"));
        login.method = "POST";
        login.body = servers::sdTokenBody(s.username, servers::sdPasswordHash(s.password));
        login.headers.append(qMakePair(QByteArray("Content-Type"), QByteArray("application/json")));
        const HttpReply reply = m_host.call(login);
        const servers::SdAnswer token = servers::parseSdToken(reply.body);
        if (!token.token.isEmpty())
            return answer("signedIn", s.username);
        const bool refused = (token.code >= 4001 && token.code <= 4005) || reply.status == 401 || reply.status == 403;
        return answer(refused ? "refused" : "noAnswer");
    }
    if (base.isEmpty())
        return answer("noAnswer");
    if (s.kind == QLatin1String("jellyfin") || s.kind == QLatin1String("emby"))
    {
        QString token, userId, error;
        bool refused = false;
        if (!mediaBrowserSession(s, &token, &userId, &error, &refused))
            return answer(refused ? "refused" : "noAnswer");
        return answer(s.username.isEmpty() ? "works" : "signedIn", s.username);
    }
    if (s.kind == QLatin1String("tvheadend"))
    {
        HttpCall call = jsonCall(base + QStringLiteral("/api/serverinfo"));
        call.username = s.username;
        call.password = s.password;
        return ofReply(m_host.call(call), s.username);
    }
    if (s.kind == QLatin1String("channels"))
        return ofReply(m_host.call(jsonCall(base + QStringLiteral("/devices"))), {});
    if (s.kind == QLatin1String("hdhomerun"))
    {
        const HttpReply reply = m_host.call(jsonCall(base + QStringLiteral("/discover.json")));
        if (reply.status != 200)
            return answer("noAnswer");
        return answer("works", servers::parseHdhrDiscover(reply.body).name);
    }
    if (s.kind == QLatin1String("nextpvr"))
    {
        const HttpReply initiate = m_host.call(jsonCall(servers::nextPvrInitiateUrl(base).toString(QUrl::FullyEncoded)));
        const servers::Salted salted = servers::parseNextPvrInitiate(initiate.body);
        if (initiate.status != 200 || salted.sid.isEmpty())
            return answer("noAnswer");
        const QString pin = s.password.isEmpty() ? QStringLiteral("0000") : s.password;
        const HttpReply login = m_host.call(jsonCall(
            servers::nextPvrLoginUrl(base, salted.sid, servers::nextPvrLoginKey(pin, salted.salt)).toString(QUrl::FullyEncoded)));
        if (login.status != 200)
            return answer("noAnswer");
        return servers::parseNextPvrLogin(login.body) ? answer("works") : answer("refused");
    }
    return answer("noAnswer");
}

QJsonObject Service::discover()
{
    QJsonArray found;
    QSet<QString> seen;
    auto add = [&](const servers::Found& f) {
        if (f.address.isEmpty() || seen.contains(f.address))
            return;
        seen.insert(f.address);
        found.append(QJsonObject { { QStringLiteral("kind"), f.kind }, { QStringLiteral("name"), f.name },
                                   { QStringLiteral("address"), f.address }, { QStringLiteral("id"), f.id } });
    };
    // Jellyfin and Emby answer "who is ...?" on UDP 7359
    for (const bool emby : { false, true })
        for (const Datagram& d : m_host.broadcast(7359, servers::mediaBrowserDiscoverPacket(emby), 1200))
            add(servers::parseMediaBrowserDiscoverReply(d.data, emby));
    // HDHomeRun: its own discovery on UDP 65001, then its discover.json, once
    // a tuner (it answers on each of its addresses, any computer can answer)
    // and briefly: a tuner on the local network answers at once
    QSet<QString> asked;
    for (const Datagram& d : m_host.broadcast(65001, servers::hdhrDiscoverPacket(), 1200))
    {
        if (!servers::isHdhrDiscoverReply(d.data) || asked.contains(d.from))
            continue;
        asked.insert(d.from);
        const QString base = servers::baseUrl(d.from, 80);
        HttpCall call = jsonCall(base + QStringLiteral("/discover.json"));
        call.timeoutMs = 2000;
        const HttpReply reply = m_host.call(call);
        const servers::Tuner tuner = servers::parseHdhrDiscover(reply.body);
        add({ QStringLiteral("hdhomerun"), tuner.name.isEmpty() ? QStringLiteral("HDHomeRun") : tuner.name,
              tuner.baseUrl.isEmpty() ? base : tuner.baseUrl, tuner.id });
    }
    // Channels DVR: multicast DNS (Bonjour)
    const QString channelsService = QStringLiteral("_channels_dvr._tcp.local");
    for (const Datagram& d : m_host.broadcast(5353, servers::mdnsQuery(channelsService), 1200,
                                              QStringLiteral("224.0.0.251")))
        for (const servers::MdnsService& svc : servers::parseMdnsAnswer(d.data, channelsService))
            add({ QStringLiteral("channels"), svc.name.isEmpty() ? QStringLiteral("Channels DVR") : svc.name,
                  servers::baseUrl((svc.address.isEmpty() ? d.from : svc.address) + QLatin1Char(':')
                                       + QString::number(svc.port > 0 ? svc.port : 8089), 8089),
                  svc.host });
    // and those its cloud knows on this network (when UDP is blocked)
    HttpCall cloud = jsonCall(QStringLiteral("https://api.hdhomerun.com/discover"));
    cloud.timeoutMs = 5000;
    for (const servers::Tuner& t : servers::parseHdhrCloudDiscover(m_host.call(cloud).body))
        if (!t.lineupUrl.isEmpty())
            add({ QStringLiteral("hdhomerun"), t.name, t.baseUrl, t.id });
    // the ones added already: not offered again
    QJsonArray offered;
    {
        std::lock_guard<std::mutex> guard(m_lock);
        for (const QJsonValue v : std::as_const(found))
        {
            const QString address = v.toObject().value(QLatin1String("address")).toString();
            const bool added = std::any_of(m_sources.cbegin(), m_sources.cend(), [&](const SourceConfig& s) {
                return isServerKind(s.kind) && servers::baseUrl(s.server, defaultPort(s.kind)) == address;
            });
            QJsonObject o = v.toObject();
            o.insert(QLatin1String("added"), added);
            offered.append(o);
        }
    }
    return QJsonObject { { QStringLiteral("found"), offered } };
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
    if (op == QLatin1String("play"))
    {
        // a channel of Jellyfin or Emby: its stream opened now
        QString sourceId, url;
        {
            std::lock_guard<std::mutex> guard(m_lock);
            const int index = m_channelIndex.value(r.value(QLatin1String("key")).toString(), -1);
            if (index >= 0)
            {
                const SourceConfig* s = findSource(m_channels[index].sourceId);
                if (s && (s->kind == QLatin1String("jellyfin") || s->kind == QLatin1String("emby")))
                {
                    sourceId = s->id;
                    url = m_channels[index].channel.url;
                }
            }
        }
        if (!sourceId.isEmpty())
        {
            QString error;
            const QString stream = openServerStream(sourceId, url, &error);
            QJsonObject answer = catchup(r);
            if (stream.isEmpty())
                return QJsonObject { { QStringLiteral("error"), error } };
            answer.insert(QLatin1String("url"), stream);
            return answer;
        }
        return catchup(r);
    }
    if (op == QLatin1String("catchup"))
        return catchup(r);
    if (op == QLatin1String("testSource"))
        return testSource(r);
    if (op == QLatin1String("discover"))
        return discover();
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
        std::unique_lock<std::mutex> guard(m_lock);
        // (with its secrets: read from the keystore first)
        if (!secretsReady(guard))
            return QJsonObject { { QStringLiteral("error"), QStringLiteral("the keystore is not open yet") } };
        const SourceConfig* s = findSource(r.value(QLatin1String("id")).toString());
        if (!s)
            return QJsonObject { { QStringLiteral("error"), QStringLiteral("no such source") } };
        out.insert(QLatin1String("source"), sourceToJson(*s, true));
        return out;
    }
    if (op == QLatin1String("addSource") || op == QLatin1String("updateSource"))
    {
        SourceConfig s = sourceFromJson(r.value(QLatin1String("source")).toObject());
        if (s.kind != QLatin1String("xtream") && !isServerKind(s.kind) && !isGuideOnly(s.kind))
            s.kind = QStringLiteral("m3u");
        // (Schedules Direct takes the SHA-1 of the password: only it is kept)
        if (isGuideOnly(s.kind) && !s.password.isEmpty())
            s.password = servers::sdPasswordHash(s.password);
        // what each needs: an M3U its link; Xtream its server and user; a
        // server its address (Jellyfin and Emby a user or an API key)
        const bool mediaBrowser = s.kind == QLatin1String("jellyfin") || s.kind == QLatin1String("emby");
        const bool incomplete = s.kind == QLatin1String("m3u") ? s.url.isEmpty()
                              : s.kind == QLatin1String("xtream") ? (s.server.isEmpty() || s.username.isEmpty())
                              : isGuideOnly(s.kind) ? (s.username.isEmpty()
                                                       || (s.password.isEmpty() && op == QLatin1String("addSource")))
                              : servers::baseUrl(s.server, defaultPort(s.kind)).isEmpty()
                                    || (mediaBrowser && s.username.isEmpty() && s.key.isEmpty()
                                        && op == QLatin1String("addSource"));
        if (incomplete)
            return QJsonObject { { QStringLiteral("error"), QStringLiteral("incomplete") } };
        if (s.name.isEmpty())
            s.name = isGuideOnly(s.kind) ? kindName(s.kind)
                   : s.kind == QLatin1String("xtream") ? hostOf(xtream::normalizeServer(s.server))
                   : isServerKind(s.kind) ? kindName(s.kind) + QStringLiteral(" · ")
                                                + hostOf(servers::baseUrl(s.server, defaultPort(s.kind)))
                                          : hostOf(s.url);
        QString id;
        {
            std::unique_lock<std::mutex> guard(m_lock);
            if (op == QLatin1String("updateSource"))
            {
                // (a password left empty is the one kept: read from the
                // keystore first, never an empty one stored over it)
                if (!secretsReady(guard))
                    return QJsonObject { { QStringLiteral("error"), QStringLiteral("the keystore is not open yet") } };
                SourceConfig* old = findSource(r.value(QLatin1String("id")).toString());
                if (!old)
                    return QJsonObject { { QStringLiteral("error"), QStringLiteral("no such source") } };
                s.id = old->id;
                // a password or key left empty is the one kept
                if (s.kind != QLatin1String("m3u") || !s.username.isEmpty())
                {
                    if (s.password.isEmpty())
                        s.password = old->password;
                    if (s.key.isEmpty())
                        s.key = old->key;
                }
                m_sessions.remove(s.id);
                // (what the dialog does not show)
                s.epgOff = old->epgOff;
                s.autoEpg = old->autoEpg;
                s.autoIds = old->autoIds;
                s.autoChecked = old->autoChecked;
                s.autoGuideOff = old->autoGuideOff;
                s.autoFound = old->autoFound;
                s.autoTotal = old->autoTotal;
                s.autoEdits = old->autoEdits;
                s.inKeystore = old->inKeystore;
                s.guideIds = old->guideIds;
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
        // its secrets alone to the keystore, out of the lock
        storeSecrets(id);
        post([this, id]() { refreshSources({ id }, true); });
        changed("sources");
        out.insert(QLatin1String("id"), id);
        return out;
    }
    if (op == QLatin1String("removeSource"))
    {
        const QString id = r.value(QLatin1String("id")).toString();
        // (its folder is removed: never one of an id that is a path, or
        // empty, the folder of every source)
        if (!isSourceId(id))
            return QJsonObject { { QStringLiteral("error"), QStringLiteral("no such source") } };
        {
            std::lock_guard<std::mutex> guard(m_lock);
            const auto end = std::remove_if(m_sources.begin(), m_sources.end(),
                                            [&](const SourceConfig& s) { return s.id == id; });
            if (end == m_sources.end())
                return QJsonObject { { QStringLiteral("error"), QStringLiteral("no such source") } };
            m_sources.erase(end, m_sources.end());
            m_state.remove(id);
            saveConfig();
        }
        {
            // (after a storing of it in progress, not before: one at a time)
            std::lock_guard<std::mutex> secretGuard(m_secretLock);
            m_host.removeSecret(id);
        }
        // its files, on the worker: after a refresh of it in progress, which
        // would write them again
        post([this, id]() {
            QDir(sourceDir(id)).removeRecursively();
            loadGuides(false, {});
            rebuild();
            changed("channels");
            changed("guide");
        });
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
    // Sign Out: the password and key of an account forgotten, the source off
    if (op == QLatin1String("signOut"))
    {
        const QString id = r.value(QLatin1String("id")).toString();
        {
            std::unique_lock<std::mutex> guard(m_lock);
            // (forgotten after they were read: never filled again from the
            // keystore afterwards)
            if (!secretsReady(guard))
                return QJsonObject { { QStringLiteral("error"), QStringLiteral("the keystore is not open yet") } };
            SourceConfig* s = findSource(id);
            if (!s)
                return QJsonObject { { QStringLiteral("error"), QStringLiteral("no such source") } };
            s->password.clear();
            s->key.clear();
            s->enabled = false;
            m_sessions.remove(id);
            saveConfig();
        }
        storeSecrets(id);
        post([this]() { loadGuides(false, {}); rebuild(); changed("channels"); changed("guide"); });
        changed("sources");
        return out;
    }
    // the automatic guide: look now, or remove it (not again by itself)
    if (op == QLatin1String("findGuide"))
    {
        const QString id = r.value(QLatin1String("id")).toString();
        post([this, id]() { findGuide(id, false); });
        return out;
    }
    if (op == QLatin1String("removeAutoGuide"))
    {
        {
            std::lock_guard<std::mutex> guard(m_lock);
            SourceConfig* s = findSource(r.value(QLatin1String("id")).toString());
            if (!s)
                return QJsonObject { { QStringLiteral("error"), QStringLiteral("no such source") } };
            for (const QString& url : std::as_const(s->autoEpg))
                s->epgOff.removeAll(url);
            s->autoEpg.clear();
            s->autoIds.clear();
            s->autoGuideOff = true;
            s->autoFound = 0;
            s->autoTotal = 0;
            ++s->autoEdits;
            saveConfig();
        }
        post([this]() { loadGuides(false, {}); rebuild(); changed("guide"); });
        changed("sources");
        return out;
    }
    // a guide of a source turned off or on, or removed (the playlist's own
    // ones are only turned off)
    if (op == QLatin1String("setGuideEnabled") || op == QLatin1String("removeGuide"))
    {
        const QString id = r.value(QLatin1String("id")).toString();
        const QString guide = r.value(QLatin1String("guide")).toString();
        const bool enabled = r.value(QLatin1String("enabled")).toBool(true);
        {
            std::lock_guard<std::mutex> guard(m_lock);
            SourceConfig* s = findSource(id);
            if (!s)
                return QJsonObject { { QStringLiteral("error"), QStringLiteral("no such source") } };
            QString url;
            for (const QString& u : guidesOf(*s, m_state.value(id).headerGuides, true))
                if (hashOf(u) == guide)
                    url = u;
            if (url.isEmpty())
                return QJsonObject { { QStringLiteral("error"), QStringLiteral("no such guide") } };
            if (op == QLatin1String("removeGuide") && (s->epg.contains(url) || s->autoEpg.contains(url)))
            {
                // (a guide found for the user, removed: not found again by itself)
                if (s->autoEpg.contains(url))
                {
                    s->autoGuideOff = true;
                    ++s->autoEdits;
                }
                s->epg.removeAll(url);
                s->autoEpg.removeAll(url);
                s->epgOff.removeAll(url);
            }
            else if (op == QLatin1String("removeGuide") || !enabled)
            {
                if (!s->epgOff.contains(url))
                    s->epgOff << url;
            }
            else
                s->epgOff.removeAll(url);
            saveConfig();
        }
        post([this, id]() { loadGuides(true, { id }); rebuild(); changed("guide"); });
        changed("sources");
        return out;
    }
    // the guide id of a channel: the user's, the one of its list, the one the
    // automatic guide found; and the user's own (empty: none)
    if (op == QLatin1String("channelGuide") || op == QLatin1String("setChannelGuide"))
    {
        const QString key = r.value(QLatin1String("key")).toString();
        {
            std::lock_guard<std::mutex> guard(m_lock);
            const int index = m_channelIndex.value(key, -1);
            if (index < 0)
                return QJsonObject { { QStringLiteral("error"), QStringLiteral("no such channel") } };
            const ChannelEntry& c = m_channels[index];
            SourceConfig* s = findSource(c.sourceId);
            if (!s)
                return QJsonObject { { QStringLiteral("error"), QStringLiteral("no such source") } };
            const QString who = identityOf(c.channel);
            if (op == QLatin1String("channelGuide"))
            {
                out.insert(QLatin1String("name"), c.channel.name);
                out.insert(QLatin1String("user"), s->guideIds.value(who));
                out.insert(QLatin1String("own"), c.channel.tvgId);
                out.insert(QLatin1String("auto"), s->autoIds.value(who).id);
                out.insert(QLatin1String("hasGuide"), bool(c.guide));
                return out;
            }
            const QString id = r.value(QLatin1String("guideId")).toString().trimmed();
            if (id.isEmpty())
                s->guideIds.remove(who);
            else
                s->guideIds.insert(who, id);
            saveConfig();
        }
        post([this]() { loadGuides(false, {}); rebuild(); changed("guide"); });
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
        o.insert(QLatin1String("host"), isGuideOnly(s.kind) ? QStringLiteral("schedulesdirect.org")
                                        : s.kind == QLatin1String("xtream") ? hostOf(xtream::normalizeServer(s.server))
                                        : isServerKind(s.kind) ? hostOf(servers::baseUrl(s.server, defaultPort(s.kind)))
                                                               : hostOf(s.url));
        o.insert(QLatin1String("refused"), state.refused);
        // (its refreshes that failed since one worked: waited for longer)
        o.insert(QLatin1String("failures"), state.failures);
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
        for (const QString& url : guidesOf(s, state.headerGuides, true))
        {
            const auto g = m_guides.value(url);
            QJsonObject go;
            // (by a hash: a guide's address may hold the user's password)
            go.insert(QLatin1String("id"), hashOf(url));
            // (a server's own guide, written next to it: the server's name)
            go.insert(QLatin1String("host"), url.startsWith(QLatin1String("file:"))
                                                 && (isServerKind(s.kind) || isGuideOnly(s.kind))
                                             ? kindName(s.kind) : hostOf(url));
            go.insert(QLatin1String("own"), s.epg.contains(url));
            go.insert(QLatin1String("auto"), s.autoEpg.contains(url) && !s.epg.contains(url));
            go.insert(QLatin1String("enabled"), !s.epgOff.contains(url));
            go.insert(QLatin1String("programmes"), g ? g->programmes : 0);
            go.insert(QLatin1String("updated"), g ? double(g->updated) : 0.0);
            go.insert(QLatin1String("error"), g ? g->error : QString());
            guides.append(go);
        }
        o.insert(QLatin1String("guides"), guides);
        QJsonObject automatic;
        automatic.insert(QLatin1String("matched"), int(s.autoIds.size()));
        automatic.insert(QLatin1String("guides"), int(s.autoEpg.size()));
        automatic.insert(QLatin1String("found"), s.autoFound);
        automatic.insert(QLatin1String("total"), s.autoTotal);
        automatic.insert(QLatin1String("checked"), double(s.autoChecked));
        automatic.insert(QLatin1String("off"), s.autoGuideOff);
        o.insert(QLatin1String("autoGuide"), automatic);
        o.insert(QLatin1String("isServer"), isServerKind(s.kind));
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
    const qint64 from = secsOf(r.value(QLatin1String("from")));
    const qint64 to = secsOf(r.value(QLatin1String("to")));
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
            // (by their starts: their ends are not in order, a long programme
            // before shorter ones)
            for (const int i : programmesBetween(entries, from - shift, to - shift))
                list.append(entryJson(c, entries[i]));
        }
        result.insert(k.toString(), list);
    }
    return QJsonObject { { QStringLiteral("programmes"), result } };
}

QJsonObject Service::nowNext(const QJsonObject& r)
{
    const qint64 at = r.contains(QLatin1String("at")) ? secsOf(r.value(QLatin1String("at"))) : nowSecs();
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
    const qint64 start = secsOf(r.value(QLatin1String("start")));
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
    const qint64 start = secsOf(r.value(QLatin1String("start")));
    const qint64 stop = secsOf(r.value(QLatin1String("stop")));
    const QDateTime now = QDateTime::currentDateTimeUtc();
    if (start >= now.toSecsSinceEpoch())
        return QJsonObject { { QStringLiteral("error"), QStringLiteral("not yet") } };
    if (now.toSecsSinceEpoch() - start > qint64(c.channel.catchupDays) * 86400)
        return QJsonObject { { QStringLiteral("error"), QStringLiteral("too old") } };
    // what is still on: up to now; a programme over: its whole length (a
    // week at most: an int, whatever the request says)
    const qint64 week = 7 * 86400;
    const int duration = int(qBound<qint64>(60, qMin(stop, now.toSecsSinceEpoch()) - start, week));
    const QString url = catchupUrl(c.channel, QDateTime::fromSecsSinceEpoch(start, QTimeZone::utc()),
                                   stop > now.toSecsSinceEpoch() ? int(qBound<qint64>(0, stop - start, week)) : duration,
                                   now);
    if (url.isEmpty())
        return QJsonObject { { QStringLiteral("error"), QStringLiteral("no archive") } };
    out.insert(QLatin1String("url"), url);
    return out;
}

QJsonObject Service::catalog()
{
    // shown: its newer copy is looked for (once a day), the one there answers now
    if (!m_quit)
        post([this]() { updateCatalog(false); });
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
        std::unique_lock<std::mutex> guard(m_lock);
        // (its password: read from the keystore first)
        if (!secretsReady(guard))
            return QJsonObject { { QStringLiteral("error"), QStringLiteral("the keystore is not open yet") } };
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
        // (kept only when it reads: one cut short would be the films shown
        // for a day)
        if (!list.isEmpty() && isJson(list))
        {
            if (isJson(categories))
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
        std::unique_lock<std::mutex> guard(m_lock);
        // (its password: read from the keystore first)
        if (!secretsReady(guard))
            return QJsonObject { { QStringLiteral("error"), QStringLiteral("the keystore is not open yet") } };
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
        if (!m_host.download(url, path, &error, kLogoMaxBytes, kLogoIdleSecs) && age < 0)
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
