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

#include "subtitle_search.hpp"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>
#include <QSettings>
#include <QThread>
#include <QUrlQuery>
#include <QtEndian>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <memory>

#include <vlc_common.h>
#include <vlc_configuration.h>

#include "util/http_request.hpp"
#include "util/redesign_version.hpp"
#include "util/subtitle_preferences.hpp"

namespace {

const char keyMatch[] = "redesign/subtitle-match/";
const char keyChoice[] = "redesign/subtitle-choice/";
// the media of those two kept (their setting keys), the most recent last:
// past maxRemembered, the oldest are forgotten
const char keyRecent[] = "redesign/subtitle-recent";
constexpr int maxRemembered = 500;
// the services the user turned off ("redesign/subtitle-service/<id>")
const char keyService[] = "redesign/subtitle-service/";
constexpr qsizetype maxJsonBytes = 4 * 1024 * 1024;
constexpr qsizetype maxSubtitleBytes = 8 * 1024 * 1024;
// the downloads of media not played for that long leave the cache
constexpr int cacheDays = 60;

HttpRequest jsonRequest(const QUrl &url, int timeoutMs = 20000)
{
    HttpRequest request;
    request.url = url;
    request.timeoutMs = timeoutMs;
    request.maxBytes = maxJsonBytes;
    // as Wikidata asks of programs: who they are, and where to read about them
    request.headers.append(qMakePair(QByteArray("User-Agent"),
                                     QByteArray("VLC-Redesign/" VLC_REDESIGN_VERSION " (" VLC_REDESIGN_REPO_URL ")")));
    request.headers.append(qMakePair(QByteArray("Accept"), QByteArray("application/json")));
    return request;
}

bool replyOk(const HttpResponse &reply, qsizetype maxBytes = maxJsonBytes)
{
    return reply.error.isEmpty() && reply.status == 200 && reply.body.size() <= maxBytes;
}

// The OpenSubtitles hash: the size plus the 64-bit words of the first and
// the last 64 KiB
QString openSubtitlesHash(const QString &path, qint64 *size)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    *size = file.size();
    constexpr qint64 chunk = 65536;
    if (*size < chunk)
        return {};
    const QByteArray head = file.read(chunk);
    if (!file.seek(*size - chunk))
        return {};
    const QByteArray tail = file.read(chunk);
    if (head.size() != chunk || tail.size() != chunk)
        return {};
    quint64 hash = quint64(*size);
    for (qint64 i = 0; i < chunk; i += 8)
    {
        hash += qFromLittleEndian<quint64>(head.constData() + i);
        hash += qFromLittleEndian<quint64>(tail.constData() + i);
    }
    return QStringLiteral("%1").arg(hash, 16, 16, QLatin1Char('0'));
}

// Lower case, without accents or punctuation: titles compare in any script
QString simplified(const QString &text)
{
    QString decomposed = text.normalized(QString::NormalizationForm_D).toLower();
    decomposed.replace(QLatin1Char('&'), QStringLiteral(" and "));
    QString result;
    result.reserve(decomposed.size());
    for (const QChar c : decomposed)
    {
        if (c.category() == QChar::Mark_NonSpacing)
            continue;
        result.append(c.isLetterOrNumber() ? c : QLatin1Char(' '));
    }
    return result.simplified();
}

double titleSimilarity(const QString &a, const QString &b)
{
    const QString x = simplified(a), y = simplified(b);
    if (x.isEmpty() || y.isEmpty())
        return 0;
    if (x == y)
        return 1;
    const QStringList xs = x.split(QLatin1Char(' ')), ys = y.split(QLatin1Char(' '));
    const QSet<QString> sx(xs.begin(), xs.end()), sy(ys.begin(), ys.end());
    const qsizetype common = QSet<QString>(sx).intersect(sy).size();
    return double(common) / double(QSet<QString>(sx).unite(sy).size());
}

struct ParsedName
{
    QString title;
    int year = 0;
    int season = -1;
    int episode = -1;
    QStringList tokens;
};

// "Home.Sweet.Home.Alone.2021.2160p.4K.WEB.x265-[YTS.MX].mkv": the title,
// its year, a season and episode ("S01E02", "1x02", "Season 1 Episode 2"),
// and the words of the release
ParsedName parseName(const QString &name, const QString &folder)
{
    using RE = QRegularExpression;
    static const RE episodeRes[] = {
        RE(QStringLiteral(R"((?:^|[\s._\-\[(])s(\d{1,2})[\s._\-]*e(\d{1,3})(?=\D|$))"), RE::CaseInsensitiveOption),
        RE(QStringLiteral(R"((?:^|[\s._\-\[(])(\d{1,2})x(\d{1,3})(?=\D|$))"), RE::CaseInsensitiveOption),
        RE(QStringLiteral(R"((?:^|[\s._\-])season[\s._\-]*(\d{1,2})[\s._\-]*episode[\s._\-]*(\d{1,3}))"),
           RE::CaseInsensitiveOption),
    };
    static const RE yearRe(QStringLiteral(R"((?:^|[\s._\-\[(])((?:19|20)\d\d)(?=[\s._\-\])]|$))"));
    static const RE junkRe(QStringLiteral(
        R"((?:^|[\s._\-\[(])(2160p|1440p|1080p|1080i|720p|576p|480p|4k|8k|uhd|hdr10\+?|hdr|dv|dovi|)"
        R"(web[\s._\-]?dl|webrip|web|bluray|blu[\s._\-]?ray|bdrip|brrip|dvdrip|dvdscr|hdtv|hdrip|remux|)"
        R"(x264|x265|h[\s.]?264|h[\s.]?265|hevc|avc|av1|xvid|divx|aac[\d.]*|ac3|e?ac3|ddp?[\d.]*|)"
        R"(dts(?:[\s._\-]?hd)?|truehd|atmos|10bit|8bit|proper|repack|extended|unrated|uncut|)"
        R"(remastered|imax|internal|limited|multi|dual|subbed|dubbed)(?=[\s._\-\])]|$))"),
        RE::CaseInsensitiveOption);
    static const RE bracketsRe(QStringLiteral(R"(\[[^\]]*\]|\{[^}]*\}|\([^)]*\))"));
    static const RE tokenRe(QStringLiteral(R"([^\p{L}\p{N}]+)"));

    ParsedName parsed;
    QString base = QFileInfo(name).completeBaseName();
    if (base.isEmpty())
        base = name;
    for (const QString &token : base.toLower().split(tokenRe, Qt::SkipEmptyParts))
        parsed.tokens.append(token);

    qsizetype cut = base.size();
    for (const RE &re : episodeRes)
    {
        const QRegularExpressionMatch m = re.match(base);
        if (m.hasMatch() && m.capturedStart() < cut)
        {
            cut = m.capturedStart();
            parsed.season = m.captured(1).toInt();
            parsed.episode = m.captured(2).toInt();
        }
    }
    // the year: not the first word ("2012.2009.1080p": the film 2012)
    for (QRegularExpressionMatchIterator it = yearRe.globalMatch(base); it.hasNext();)
    {
        const QRegularExpressionMatch m = it.next();
        if (m.capturedStart(1) == 0)
            continue;
        if (m.capturedStart() < cut)
        {
            cut = m.capturedStart();
            parsed.year = m.captured(1).toInt();
        }
        break;
    }
    const QRegularExpressionMatch junk = junkRe.match(base);
    if (junk.hasMatch() && junk.capturedStart() > 0 && junk.capturedStart() < cut)
        cut = junk.capturedStart();

    QString title = base.left(cut);
    title.replace(bracketsRe, QStringLiteral(" "));
    title.replace(QLatin1Char('.'), QLatin1Char(' '));
    title.replace(QLatin1Char('_'), QLatin1Char(' '));
    title = title.simplified();
    while (!title.isEmpty() && QStringLiteral("-([{,").contains(title.back()))
        title.chop(1);
    parsed.title = title.trimmed();

    // "Show Name/Season 1/02.mkv", "Movie (2021)/movie.mkv": the folder
    if (!folder.isEmpty() && (parsed.title.isEmpty() || parsed.title.size() <= 2))
    {
        const ParsedName fromFolder = parseName(folder, QString());
        if (!fromFolder.title.isEmpty())
        {
            parsed.title = fromFolder.title;
            if (!parsed.year)
                parsed.year = fromFolder.year;
        }
    }
    return parsed;
}

QString matchKey(const QString &mediaKey)
{
    return QString::fromLatin1(keyMatch) + SubtitleSearch::folderName(mediaKey);
}

QString choiceKey(const QString &mediaKey)
{
    return QString::fromLatin1(keyChoice) + SubtitleSearch::folderName(mediaKey);
}

// The title or subtitle of a media was kept: it is the most recent, and
// the oldest past maxRemembered are forgotten (the settings don't grow
// with every file ever played)
void remember(QSettings *settings, const QString &mediaKey)
{
    QStringList recent;
    if (settings->contains(QLatin1String(keyRecent)))
        recent = settings->value(QLatin1String(keyRecent)).toStringList();
    else
    {
        // (those kept before the list was)
        QSet<QString> known;
        for (const char *prefix : { keyMatch, keyChoice })
        {
            settings->beginGroup(QString::fromLatin1(prefix).chopped(1));
            for (const QString &key : settings->childKeys())
                if (!known.contains(key))
                {
                    known.insert(key);
                    recent.append(key);
                }
            settings->endGroup();
        }
    }
    const QString key = SubtitleSearch::folderName(mediaKey);
    recent.removeAll(key);
    recent.append(key);
    while (recent.size() > maxRemembered)
    {
        const QString old = recent.takeFirst();
        settings->remove(QString::fromLatin1(keyMatch) + old);
        settings->remove(QString::fromLatin1(keyChoice) + old);
    }
    settings->setValue(QLatin1String(keyRecent), recent);
}

// The folders of the cache (one a media, named by its setting key) whose
// newest file is older than cacheDays: deleted, but for keep's (on a worker
// thread; SubtitleFinder renews the file it uses)
void pruneCache(const QString &root, const QString &keep)
{
    static const QRegularExpression keyRe(QStringLiteral("^[0-9a-f]{20}$"));
    const QDateTime oldest = QDateTime::currentDateTime().addDays(-cacheDays);
    for (const QFileInfo &folder : QDir(root).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot))
    {
        if (folder.fileName() == keep || !keyRe.match(folder.fileName()).hasMatch())
            continue;
        QDateTime newest = folder.lastModified();
        for (const QFileInfo &file : QDir(folder.filePath()).entryInfoList(QDir::Files))
            newest = std::max(newest, file.lastModified());
        if (newest < oldest)
            QDir(folder.filePath()).removeRecursively();
    }
}

// A part of a file name from a service's answer: only letters, digits, '_',
// '.' and '-', without "..", not starting with a dot (nothing of a path)
QString safeFileName(const QString &name)
{
    QString safe;
    safe.reserve(name.size());
    for (const QChar c : name)
    {
        const char16_t u = c.unicode();
        const bool allowed = (u >= 'a' && u <= 'z') || (u >= 'A' && u <= 'Z') || (u >= '0' && u <= '9')
                          || u == '_' || u == '.' || u == '-';
        safe.append(allowed ? c : QLatin1Char('_'));
    }
    while (safe.contains(QLatin1String("..")))
        safe.replace(QLatin1String(".."), QLatin1String("."));
    if (safe.startsWith(QLatin1Char('.')))
        safe[0] = QLatin1Char('_');
    return safe.left(100);
}

const QSet<QString> &seriesClasses()
{
    // television series, miniseries, animated series, web series,
    // animated television series, anime television series
    static const QSet<QString> classes = { "Q5398426", "Q1259759", "Q581714", "Q526877",
                                           "Q117467246", "Q63952888" };
    return classes;
}

const QSet<QString> &filmClasses()
{
    // film, short film, animated film, television film, 3D film, animated
    // short film, feature film, documentary film, animated feature film,
    // silent film, anime film
    static const QSet<QString> classes = { "Q11424", "Q24862", "Q202866", "Q506240", "Q229390",
                                           "Q17517379", "Q24869", "Q93204", "Q29168811", "Q226730",
                                           "Q20650540" };
    return classes;
}

} // namespace

SubtitleSearch::SubtitleSearch(qt_intf_t *intf, SubtitlePreferences *preferences, QObject *parent)
    : QObject(parent)
    , m_intf(intf)
    , m_preferences(preferences)
{
}

QString SubtitleSearch::folderName(const QString &mediaKey)
{
    return QString::fromLatin1(QCryptographicHash::hash(mediaKey.toUtf8(), QCryptographicHash::Sha1).toHex().left(20));
}

QString SubtitleSearch::cacheRoot()
{
    char *cache = config_GetUserDir(VLC_CACHE_DIR);
    const QString root = cache ? QString::fromUtf8(cache) + QStringLiteral("/subtitles") : QString();
    free(cache);
    return root;
}

bool SubtitleSearch::serviceEnabled(QSettings *settings, const QString &id)
{
    return settings->value(QString::fromLatin1(keyService) + id, true).toBool();
}

void SubtitleSearch::setServiceEnabled(QSettings *settings, const QString &id, bool enabled)
{
    settings->setValue(QString::fromLatin1(keyService) + id, enabled);
}

// (by its hash, else by its URL: streams and files too small to hash, as
// it is saved)
QStringList SubtitleSearch::rememberedChoice(QSettings *settings, const QString &mediaKey)
{
    if (mediaKey.isEmpty())
        return {};
    const QStringList choice = settings->value(choiceKey(mediaKey)).toStringList();
    if (choice.size() < 3 || !QFileInfo::exists(choice[0]))
        return {};
    return choice;
}

void SubtitleSearch::rememberChoice(QSettings *settings, const QString &mediaKey, const QStringList &choice)
{
    if (mediaKey.isEmpty())
        return;
    settings->setValue(choiceKey(mediaKey), choice);
    remember(settings, mediaKey);
}

void SubtitleSearch::setMedia(const QUrl &url, const QString &name, double fps)
{
    reset();
    m_url = url;
    m_media.path = url.isLocalFile() ? url.toLocalFile() : QString();
    m_media.name = !m_media.path.isEmpty() ? QFileInfo(m_media.path).fileName()
                                           : (url.fileName().isEmpty() ? name : url.fileName());
    const QString folder = !m_media.path.isEmpty() ? QFileInfo(m_media.path).dir().dirName() : QString();
    const ParsedName parsed = parseName(m_media.name, folder);
    m_media.title = parsed.title;
    m_media.year = parsed.year;
    m_media.season = parsed.season;
    m_media.episode = parsed.episode;
    m_media.tokens = parsed.tokens;
    m_media.fps = fps;
}

void SubtitleSearch::reset()
{
    ++m_revision;
    ++m_searchRevision;
    ++m_downloadRevision;
    m_url.clear();
    m_media = Media();
    m_match = Match();
    m_found.clear();
    m_runs.clear();
    m_pending = 0;
    m_results.clear();
    m_state = Idle;
    m_error.clear();
    emit runsChanged();
    emit resultsChanged();
    emit stateChanged();
}

void SubtitleSearch::prepare(std::function<void()> next)
{
    if (!m_media.key.isEmpty())
    {
        next();
        return;
    }
    struct Hashed
    {
        QString hash;
        qint64 size = 0;
    };
    const quint64 revision = m_revision;
    auto hashed = std::make_shared<Hashed>();
    const QString path = m_media.path;
    QThread *worker = QThread::create([hashed, path]() {
        if (!path.isEmpty())
            hashed->hash = openSubtitlesHash(path, &hashed->size);
    });
    // (by the finished signal: dropped if the search is gone)
    connect(worker, &QThread::finished, this, [this, hashed, revision, next]() {
        if (revision != m_revision)
            return;
        m_media.hash = hashed->hash;
        m_media.size = hashed->size;
        m_media.key = !hashed->hash.isEmpty() ? hashed->hash + QLatin1Char(':') + QString::number(hashed->size)
                                              : m_url.toString();
        next();
    });
    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    worker->start(QThread::LowPriority);
}

void SubtitleSearch::setState(State state, const QString &error)
{
    static const char *const names[] = { "idle", "identifying", "searching", "ready", "not found", "failed" };
    if (state != m_state || error != m_error)
        msg_Dbg(m_intf, "online subtitles: %s%s%s%s%s", qtu(m_media.name), m_media.name.isEmpty() ? "" : ": ",
                names[state], error.isEmpty() ? "" : ": ", qtu(error));
    m_state = state;
    m_error = error;
    emit stateChanged();
}

void SubtitleSearch::setError(const QString &error)
{
    setState(m_state, error);
}

QString SubtitleSearch::matchTitle() const
{
    if (m_match.imdb.isEmpty())
        return {};
    QString title = m_match.title;
    if (m_match.series && m_media.season >= 0 && m_media.episode >= 0)
        title += QStringLiteral(" · S%1E%2").arg(m_media.season, 2, 10, QLatin1Char('0'))
                                             .arg(m_media.episode, 2, 10, QLatin1Char('0'));
    else if (m_match.year > 0)
        title += QStringLiteral(" (%1)").arg(m_match.year);
    return title;
}

void SubtitleSearch::start()
{
    if (!httpRequestSupported())
    {
        setState(Failed, qtr("This build can't search the internet."));
        emit finished();
        return;
    }
    if (m_state == Identifying || m_state == Searching)
        return;
    m_results.clear();
    emit resultsChanged();
    setState(Identifying);
    prepare([this]() { identify(); });
}

void SubtitleSearch::identify()
{
    // a title the user chose for this file before
    const QStringList remembered = m_intf->mainSettings->value(matchKey(m_media.key)).toStringList();
    if (remembered.size() == 4 && remembered[0].startsWith(QLatin1String("tt")))
    {
        m_match = { remembered[0], remembered[1], remembered[2].toInt(), remembered[3] == QLatin1String("series"), true };
        querySubtitles();
        return;
    }
    if (m_media.title.isEmpty())
    {
        // (a file of a known release is found by its hash all the same)
        querySubtitles();
        return;
    }
    searchImdb(m_media.title, true);
}

void SubtitleSearch::findTitles(const QString &query, std::function<void(const QVariantList &)> done)
{
    if (query.trimmed().isEmpty() || !httpRequestSupported())
    {
        done({});
        return;
    }
    searchImdb(query.trimmed(), false, done);
}

// IMDb's suggestions (what its search box shows): the titles named so,
// with their year and what they are; Wikidata then Cinemeta when it does not
// know the title
void SubtitleSearch::searchImdb(const QString &query, bool forMedia, std::function<void(const QVariantList &)> done)
{
    const quint64 revision = m_revision;
    QString words = simplified(query);
    if (forMedia && m_media.year > 0)
        words += QStringLiteral(" %1").arg(m_media.year);
    const QChar first = words.isEmpty() ? QLatin1Char('x') : words.at(0);
    const QString shard = first.isLetterOrNumber() && first.unicode() < 128 ? QString(first) : QStringLiteral("x");
    QByteArray address("https://v2.sg.media-imdb.com/suggestion/");
    address += shard.toUtf8() + '/' + QUrl::toPercentEncoding(words) + ".json";
    const QUrl url = QUrl::fromEncoded(address);
    httpRequestAsync(this, jsonRequest(url, 15000), [this, revision, forMedia, query, done](const HttpResponse &reply) {
        if (forMedia && revision != m_revision)
            return;
        QVariantList candidates;
        int rank = 0;
        if (replyOk(reply))
            for (const QJsonValue item : QJsonDocument::fromJson(reply.body).object()
                                             .value(QLatin1String("d")).toArray())
            {
                const QJsonObject o = item.toObject();
                const QString imdb = o.value(QLatin1String("id")).toString();
                if (!imdb.startsWith(QLatin1String("tt")))
                    continue;
                const QString kind = o.value(QLatin1String("qid")).toString();
                const bool series = kind == QLatin1String("tvSeries") || kind == QLatin1String("tvMiniSeries");
                const bool film = kind == QLatin1String("movie") || kind == QLatin1String("tvMovie")
                                || kind == QLatin1String("short") || kind == QLatin1String("video");
                if (!series && !film)
                    continue;
                const int year = o.value(QLatin1String("y")).toInt();
                QString description = o.value(QLatin1String("q")).toString();
                const QString cast = o.value(QLatin1String("s")).toString();
                if (!cast.isEmpty())
                    description += (description.isEmpty() ? QString() : QStringLiteral(" · ")) + cast;
                candidates.append(QVariantMap{
                    { QStringLiteral("imdb"), imdb },
                    { QStringLiteral("label"), o.value(QLatin1String("l")).toString() },
                    { QStringLiteral("description"), description },
                    { QStringLiteral("year"), year },
                    { QStringLiteral("series"), series },
                    { QStringLiteral("film"), film },
                    { QStringLiteral("rank"), rank++ },
                });
            }
        if (candidates.isEmpty())
        {
            // not known there (or no answer): Wikidata
            searchTitles(query, forMedia, done);
            return;
        }
        onTitles(candidates, forMedia, done, [this, query, forMedia, done]() { searchTitles(query, forMedia, done); });
    });
}

// Wikidata: the items named so, then their IMDb id (P345), what they are
// (P31) and their date (P577, or P580 for a series)
void SubtitleSearch::searchTitles(const QString &query, bool forMedia, std::function<void(const QVariantList &)> done)
{
    const quint64 revision = m_revision;
    QUrl url(QStringLiteral("https://www.wikidata.org/w/api.php"));
    QUrlQuery params;
    params.addQueryItem(QStringLiteral("action"), QStringLiteral("wbsearchentities"));
    params.addQueryItem(QStringLiteral("search"), query);
    params.addQueryItem(QStringLiteral("language"), QStringLiteral("en"));
    params.addQueryItem(QStringLiteral("uselang"), QStringLiteral("en"));
    params.addQueryItem(QStringLiteral("type"), QStringLiteral("item"));
    params.addQueryItem(QStringLiteral("limit"), QStringLiteral("10"));
    params.addQueryItem(QStringLiteral("format"), QStringLiteral("json"));
    url.setQuery(params);
    httpRequestAsync(this, jsonRequest(url), [this, revision, forMedia, done](const HttpResponse &reply) {
        if (forMedia && revision != m_revision)
            return;
        if (!replyOk(reply))
        {
            if (forMedia)
                searchCinemeta();
            else if (done)
                done({});
            return;
        }
        QStringList ids;
        QHash<QString, QPair<QString, QString>> labels; // id -> label, description
        for (const QJsonValue item : QJsonDocument::fromJson(reply.body).object()
                                         .value(QLatin1String("search")).toArray())
        {
            const QJsonObject o = item.toObject();
            const QString id = o.value(QLatin1String("id")).toString();
            if (id.isEmpty())
                continue;
            ids.append(id);
            labels.insert(id, { o.value(QLatin1String("label")).toString(),
                                o.value(QLatin1String("description")).toString() });
        }
        if (ids.isEmpty())
        {
            if (forMedia)
                searchCinemeta();
            else
                onTitles({}, false, done);
            return;
        }
        QUrl entities(QStringLiteral("https://www.wikidata.org/w/api.php"));
        QUrlQuery q;
        q.addQueryItem(QStringLiteral("action"), QStringLiteral("wbgetentities"));
        q.addQueryItem(QStringLiteral("ids"), ids.join(QLatin1Char('|')));
        q.addQueryItem(QStringLiteral("props"), QStringLiteral("claims"));
        q.addQueryItem(QStringLiteral("format"), QStringLiteral("json"));
        entities.setQuery(q);
        httpRequestAsync(this, jsonRequest(entities), [this, revision, forMedia, ids, labels, done](const HttpResponse &reply) {
            if (forMedia && revision != m_revision)
                return;
            QVariantList candidates;
            const QJsonObject all = replyOk(reply)
                ? QJsonDocument::fromJson(reply.body).object().value(QLatin1String("entities")).toObject()
                : QJsonObject();
            for (qsizetype rank = 0; rank < ids.size(); ++rank)
            {
                const QJsonObject claims = all.value(ids[rank]).toObject().value(QLatin1String("claims")).toObject();
                const auto values = [&](const char *property) {
                    QList<QJsonValue> result;
                    for (const QJsonValue claim : claims.value(QLatin1String(property)).toArray())
                        result.append(claim.toObject().value(QLatin1String("mainsnak")).toObject()
                                          .value(QLatin1String("datavalue")).toObject().value(QLatin1String("value")));
                    return result;
                };
                QString imdb;
                for (const QJsonValue &v : values("P345"))
                    if (v.toString().startsWith(QLatin1String("tt")))
                    {
                        imdb = v.toString();
                        break;
                    }
                if (imdb.isEmpty())
                    continue;
                bool series = false, film = false;
                for (const QJsonValue &v : values("P31"))
                {
                    const QString cls = v.toObject().value(QLatin1String("id")).toString();
                    series = series || seriesClasses().contains(cls);
                    film = film || filmClasses().contains(cls);
                }
                const QString description = labels.value(ids[rank]).second;
                if (!series && !film)
                {
                    series = description.contains(QLatin1String("series"), Qt::CaseInsensitive);
                    film = description.contains(QLatin1String("film"), Qt::CaseInsensitive)
                        || description.contains(QLatin1String("movie"), Qt::CaseInsensitive);
                }
                if (description.contains(QLatin1String("episode of"), Qt::CaseInsensitive))
                    continue; // an episode's own item: the series has the subtitles
                int year = 0;
                for (const char *property : { "P577", "P580" })
                {
                    for (const QJsonValue &v : values(property))
                    {
                        const QString time = v.toObject().value(QLatin1String("time")).toString();
                        const int y = time.mid(1, 4).toInt();
                        if (y > 0 && (year == 0 || y < year))
                            year = y;
                    }
                    if (year)
                        break;
                }
                candidates.append(QVariantMap{
                    { QStringLiteral("imdb"), imdb },
                    { QStringLiteral("label"), labels.value(ids[rank]).first },
                    { QStringLiteral("description"), description },
                    { QStringLiteral("year"), year },
                    { QStringLiteral("series"), series },
                    { QStringLiteral("film"), film },
                    { QStringLiteral("rank"), int(rank) },
                });
            }
            onTitles(candidates, forMedia, done);
        });
    });
}

void SubtitleSearch::onTitles(const QVariantList &candidates, bool forMedia,
                              std::function<void(const QVariantList &)> done, std::function<void()> otherwise)
{
    if (!forMedia)
    {
        QVariantList titles;
        for (const QVariant &c : candidates)
        {
            QVariantMap entry = c.toMap();
            const int year = entry.value(QStringLiteral("year")).toInt();
            QString detail = entry.value(QStringLiteral("description")).toString();
            if (detail.isEmpty() && year)
                detail = QString::number(year);
            entry.insert(QStringLiteral("detail"), detail);
            titles.append(entry);
            if (titles.size() == 8)
                break;
        }
        if (done)
            done(titles);
        return;
    }

    // the best of them for the file: its title, its year, a series for an
    // episode, a film otherwise, and the order of the search
    const bool wantSeries = m_media.season >= 0;
    double bestScore = -1e9;
    QVariantMap best;
    bool bestStrong = false;
    for (const QVariant &c : candidates)
    {
        const QVariantMap entry = c.toMap();
        const double similarity = titleSimilarity(entry.value(QStringLiteral("label")).toString(), m_media.title);
        if (similarity < 0.5)
            continue;
        double score = similarity * 50;
        const int year = entry.value(QStringLiteral("year")).toInt();
        if (m_media.year && year)
            score += year == m_media.year ? 30 : std::abs(year - m_media.year) == 1 ? 15 : -20;
        const bool series = entry.value(QStringLiteral("series")).toBool();
        const bool film = entry.value(QStringLiteral("film")).toBool();
        score += wantSeries ? (series ? 30 : -40) : (film ? 20 : series ? -20 : 0);
        score += (10 - entry.value(QStringLiteral("rank")).toInt()) * 2;
        if (score > bestScore)
        {
            bestScore = score;
            best = entry;
            bestStrong = strongTitle(similarity, year, series);
        }
    }
    if (best.isEmpty())
    {
        if (otherwise)
            otherwise();
        else
            searchCinemeta();
        return;
    }
    m_match = { best.value(QStringLiteral("imdb")).toString(), best.value(QStringLiteral("label")).toString(),
                best.value(QStringLiteral("year")).toInt(), best.value(QStringLiteral("series")).toBool(), bestStrong };
    querySubtitles();
}

bool SubtitleSearch::strongTitle(double similarity, int year, bool series) const
{
    if (similarity < 0.999)
        return false;
    const bool yearsKnown = m_media.year > 0 && year > 0;
    if (yearsKnown)
        return std::abs(year - m_media.year) <= 1;
    if (series && m_media.season >= 0 && m_media.episode >= 0)
        return true;
    // one word without a year ("silent", "home"): too many titles share it
    return simplified(m_media.title).count(QLatin1Char(' ')) >= 1;
}

// Stremio's catalogue, when Wikidata doesn't know the title
void SubtitleSearch::searchCinemeta()
{
    const quint64 revision = m_revision;
    const bool series = m_media.season >= 0;
    const QString encoded = QString::fromUtf8(QUrl::toPercentEncoding(m_media.title));
    const QUrl url = QUrl::fromEncoded(QStringLiteral("https://v3-cinemeta.strem.io/catalog/%1/top/search=%2.json")
                                           .arg(series ? QStringLiteral("series") : QStringLiteral("movie"), encoded)
                                           .toUtf8());
    httpRequestAsync(this, jsonRequest(url), [this, revision, series](const HttpResponse &reply) {
        if (revision != m_revision)
            return;
        if (!replyOk(reply))
        {
            querySubtitles();
            return;
        }
        double bestScore = -1e9;
        Match best;
        int rank = 0;
        for (const QJsonValue item : QJsonDocument::fromJson(reply.body).object().value(QLatin1String("metas")).toArray())
        {
            const QJsonObject o = item.toObject();
            const QString id = o.value(QLatin1String("id")).toString();
            const QString name = o.value(QLatin1String("name")).toString();
            const int year = o.value(QLatin1String("releaseInfo")).toString().left(4).toInt();
            const double similarity = titleSimilarity(name, m_media.title);
            ++rank;
            if (!id.startsWith(QLatin1String("tt")) || similarity < 0.6)
                continue;
            double score = similarity * 50 - rank * 2;
            if (m_media.year && year)
                score += year == m_media.year ? 30 : std::abs(year - m_media.year) == 1 ? 15 : -20;
            if (score > bestScore)
            {
                bestScore = score;
                best = { id, name, year, series, strongTitle(similarity, year, series) };
            }
        }
        // (not identified: a file of a known release is found by its hash)
        if (!best.imdb.isEmpty())
            m_match = best;
        querySubtitles();
    });
}

void SubtitleSearch::useMatch(const Match &match)
{
    if (m_media.key.isEmpty())
        return;
    m_match = match;
    m_match.strong = true;
    // this file is that title from now on
    m_intf->mainSettings->setValue(matchKey(m_media.key),
                                   QStringList{ m_match.imdb, m_match.title, QString::number(m_match.year),
                                                m_match.series ? QStringLiteral("series") : QStringLiteral("movie") });
    remember(m_intf->mainSettings, m_media.key);
    ++m_revision;
    m_results.clear();
    emit resultsChanged();
    querySubtitles();
}

// The services asked together (each its own few requests); the results show
// as they come, the best first in each language
void SubtitleSearch::querySubtitles()
{
    ++m_searchRevision;
    const quint64 revision = m_searchRevision;
    msg_Dbg(m_intf, "online subtitles: \"%s\" is %s (%s%s), the hash %s",
            qtu(m_media.title), m_match.imdb.isEmpty() ? "unknown" : qtu(m_match.title),
            m_match.imdb.isEmpty() ? "-" : qtu(m_match.imdb),
            m_match.imdb.isEmpty() ? "" : m_match.strong ? ", sure" : ", a guess: not loaded by itself",
            m_media.hash.isEmpty() ? "unknown" : qtu(m_media.hash));
    setState(Searching);
    m_found.clear();
    m_runs.clear();
    m_pending = 0;

    SubtitleProviders::Query query;
    query.imdb = m_match.imdb;
    query.title = m_match.imdb.isEmpty() ? m_media.title : m_match.title;
    query.year = m_match.imdb.isEmpty() ? m_media.year : m_match.year;
    query.series = m_match.imdb.isEmpty() ? m_media.season >= 0 : m_match.series;
    query.season = m_media.season;
    query.episode = m_media.episode;
    query.fileName = m_media.name;
    query.hash = m_media.hash;
    query.size = m_media.size;
    query.languages = m_preferences->wanted();

    QStringList asked;
    for (const SubtitleProviders::Provider &provider : SubtitleProviders::providers())
        if (serviceEnabled(m_intf->mainSettings, provider.id) && (query.series ? provider.series : provider.movies))
            asked.append(provider.id);
    // all counted before any is asked: one can answer at once (nothing to
    // ask it), and the search ends only with the last
    m_pending = int(asked.size());
    for (const QString &id : asked)
        m_runs[id] = { true, 0, {} };
    emit runsChanged();
    if (asked.isEmpty())
    {
        finishSearch();
        return;
    }
    for (const QString &id : asked)
    {
        QElapsedTimer timer;
        timer.start();
        SubtitleProviders::search(this, id, query,
            [this, revision, id, timer](const QList<SubtitleProviders::Result> &results, const QString &error) {
                if (revision != m_searchRevision)
                    return;
                msg_Dbg(m_intf, "online subtitles: %s: %lld in %lld ms%s%s", qtu(id), (long long)results.size(),
                        (long long)timer.elapsed(), error.isEmpty() ? "" : ", ", qtu(error));
                onProviderDone(id, results, error);
            });
        if (revision != m_searchRevision)
            return; // (another search started from an answer)
    }
}

void SubtitleSearch::onProviderDone(const QString &id, const QList<SubtitleProviders::Result> &results,
                                    const QString &error)
{
    m_runs[id] = { false, int(results.size()), error };
    m_found.append(results);
    rebuildResults();
    emit runsChanged();
    if (--m_pending <= 0)
        finishSearch();
}

// The results by language, the user's first; in each, the closest to this
// file first: made for it (its hash), sharing the words of its release, of
// its frame rate, then the services' own scores
void SubtitleSearch::rebuildResults()
{
    struct Entry
    {
        QVariantMap map;
        double score;
        int languageRank;
        QString languageName;
        int order;
    };
    const QStringList wanted = m_preferences->wanted();
    const QSet<QString> fileTokens(m_media.tokens.begin(), m_media.tokens.end());
    static const QSet<QString> ignored = { "the", "a", "an", "of", "and", "srt", "sub", "subs", "subtitles",
                                           "heb", "eng", "he", "en" };
    static const QRegularExpression tokenRe(QStringLiteral(R"([^\p{L}\p{N}]+)"));
    static const QRegularExpression hearingRe(QStringLiteral(R"((?:^|[\s._\-\[(])(hi|sdh|cc|hearing[\s._\-]?impaired)(?=[\s._\-\])]|$))"),
                                              QRegularExpression::CaseInsensitiveOption);
    // the release of the file: its name, and its group (its last word:
    // "YIFY", "DEMAND"), which made the subtitles timed for it
    const QString fileRelease = simplified(QFileInfo(m_media.name).completeBaseName());
    const QString fileGroup = m_media.tokens.isEmpty() ? QString() : m_media.tokens.last();
    QList<Entry> list;
    int order = 0;
    for (const SubtitleProviders::Result &r : m_found)
    {
        double score = r.exact ? 100 : 0;
        const QString release = simplified(r.release);
        const QString subtitleName = simplified(QFileInfo(r.fileName).completeBaseName());
        if (!fileRelease.isEmpty() && (release == fileRelease || subtitleName == fileRelease))
            score += 50;
        int shared = 0;
        bool group = false;
        for (const QString &token : (r.release + QLatin1Char(' ') + r.fileName).toLower().split(tokenRe, Qt::SkipEmptyParts))
        {
            if (token.size() > 1 && fileTokens.contains(token) && !ignored.contains(token))
                ++shared;
            if (!fileGroup.isEmpty() && token == fileGroup && fileGroup.size() > 1)
                group = true;
        }
        score += std::min(shared, 8) * 5;
        if (group)
            score += 15;
        if (r.fps > 1 && m_media.fps > 1)
            score += std::abs(r.fps - m_media.fps) < 0.05 ? 15 : -15;
        const bool hearingImpaired = r.hearingImpaired || hearingRe.match(r.fileName).hasMatch()
                                   || hearingRe.match(r.release).hasMatch();
        if (hearingImpaired)
            score -= 3;
        if (r.foreignOnly)
            score -= 25;
        score += std::clamp(r.rank, -30., 20.);
        const SubtitleProviders::Provider *provider = SubtitleProviders::provider(r.provider);
        const qsizetype rank = wanted.indexOf(r.language);
        QVariantMap map{
            { QStringLiteral("id"), r.id },
            { QStringLiteral("provider"), r.provider },
            { QStringLiteral("providerName"), provider ? provider->name : r.provider },
            { QStringLiteral("language"), r.language },
            { QStringLiteral("languageName"), m_preferences->name(r.language) },
            { QStringLiteral("nativeName"), m_preferences->nativeName(r.language) },
            { QStringLiteral("release"), r.release.isEmpty() ? qtr("Subtitles") : r.release },
            { QStringLiteral("exact"), r.exact },
            { QStringLiteral("hearingImpaired"), hearingImpaired },
            { QStringLiteral("foreignOnly"), r.foreignOnly },
            { QStringLiteral("downloads"), r.downloads },
        };
        list.append({ map, score, rank < 0 ? int(wanted.size()) : int(rank),
                      map.value(QStringLiteral("languageName")).toString(), order++ });
    }
    std::stable_sort(list.begin(), list.end(), [](const Entry &a, const Entry &b) {
        if (a.languageRank != b.languageRank)
            return a.languageRank < b.languageRank;
        if (a.languageName != b.languageName)
            return QString::localeAwareCompare(a.languageName, b.languageName) < 0;
        if (a.score != b.score)
            return a.score > b.score;
        return a.order < b.order;
    });
    m_results.clear();
    for (const Entry &e : list)
        m_results.append(e.map);
    emit resultsChanged();
}

void SubtitleSearch::finishSearch()
{
    msg_Dbg(m_intf, "online subtitles: %lld found", (long long)m_results.size());
    if (m_results.isEmpty())
    {
        // why: the services did not answer, or have none
        QString error;
        bool anyAnswered = m_runs.isEmpty();
        for (auto it = m_runs.constBegin(); it != m_runs.constEnd(); ++it)
        {
            if (it->error.isEmpty())
                anyAnswered = true;
            else if (error.isEmpty())
                error = it->error;
        }
        if (!anyAnswered)
            setState(Failed, error.isEmpty() ? qtr("No connection to the internet.") : error);
        else if (m_match.imdb.isEmpty())
            setState(NotFound, m_media.title.isEmpty()
                               ? qtr("The video couldn't be identified from its name.")
                               : qtr("\"%1\" wasn't found. Search for its title.").arg(m_media.title));
        else
            setState(NotFound, qtr("No subtitles were found for %1.").arg(matchTitle()));
    }
    else
        setState(Ready);
    emit finished();
}

const SubtitleProviders::Result *SubtitleSearch::result(const QString &id) const
{
    for (const SubtitleProviders::Result &r : m_found)
        if (r.id == id)
            return &r;
    return nullptr;
}

QVariantMap SubtitleSearch::resultMap(const QString &id) const
{
    for (const QVariant &r : m_results)
        if (r.toMap().value(QStringLiteral("id")).toString() == id)
            return r.toMap();
    return {};
}

QVariantMap SubtitleSearch::bestOf(const QString &language) const
{
    for (const QVariant &r : m_results)
    {
        const QVariantMap map = r.toMap();
        if (map.value(QStringLiteral("language")).toString() != language)
            continue;
        if (map.value(QStringLiteral("exact")).toBool() || m_match.strong)
            return map;
    }
    return {};
}

void SubtitleSearch::download(const QString &id, const QString &folder, bool prune,
                              std::function<void(const QString &file, const QString &error)> done)
{
    const SubtitleProviders::Result *choice = result(id);
    if (!choice)
    {
        done({}, qtr("The subtitle couldn't be downloaded (%1).").arg(0));
        return;
    }
    const SubtitleProviders::Result result = *choice;
    // (no cache folder: nothing of the cache to clean up)
    const QString pruneRoot = prune ? QFileInfo(folder).absolutePath() : QString();
    // "<release>-<id>.<language>.<ext>": VLC reads the language from the name
    QString release = result.release;
    release.replace(QRegularExpression(QStringLiteral(R"([\\/:*?"<>|\s\p{Cc}]+)")), QStringLiteral("."));
    release = release.left(80);
    // (the id comes from the service's answer: nothing of a path in it)
    QString fileId = id;
    fileId.replace(QLatin1Char(':'), QLatin1Char('-'));
    const QString stem = folder + QLatin1Char('/') + release + QLatin1Char('-') + safeFileName(fileId)
                         + QLatin1Char('.') + SubtitlePreferences::iso639_2(result.language);

    for (const QString &ext : { QStringLiteral("srt"), QStringLiteral("vtt"), QStringLiteral("ass"),
                                QStringLiteral("ssa"), QStringLiteral("sub"), QStringLiteral("smi") })
        if (QFileInfo::exists(stem + QLatin1Char('.') + ext))
        {
            done(stem + QLatin1Char('.') + ext, {});
            return;
        }

    const quint64 revision = m_downloadRevision;
    HttpRequest request;
    request.url = result.url;
    request.timeoutMs = 30000;
    request.maxBytes = maxSubtitleBytes;
    request.headers = SubtitleProviders::downloadHeaders(result);
    httpRequestAsync(this, request, [this, revision, done, result, folder, stem, pruneRoot](const HttpResponse &reply) {
        if (revision != m_downloadRevision)
            return;
        if (!replyOk(reply, maxSubtitleBytes) || reply.body.isEmpty())
        {
            done({}, reply.status == 0 ? qtr("No connection to the internet.")
                                       : qtr("The subtitle couldn't be downloaded (%1).").arg(reply.status));
            return;
        }
        // Unpacked, converted and saved on a worker thread (an archive can
        // be large); the answer comes back by the finished signal, dropped
        // if the search is gone
        struct Saved
        {
            QString file;
            QString error;
        };
        auto saved = std::make_shared<Saved>();
        const QByteArray body = reply.body;
        const QString videoName = m_media.name;
        const QString keep = QFileInfo(folder).fileName();
        QThread *worker = QThread::create([saved, body, result, videoName, folder, stem, pruneRoot, keep]() {
            // the cache cleaned up once a session, as it grows
            static std::atomic<bool> pruned { false };
            if (!pruneRoot.isEmpty() && !pruned.exchange(true))
                pruneCache(pruneRoot, keep);
            QString extension;
            const QByteArray text = SubtitleProviders::subtitleOf(body, result, videoName, &extension,
                                                                  &saved->error);
            if (text.isEmpty())
                return;
            const QString file = stem + QLatin1Char('.') + extension;
            QDir().mkpath(folder);
            QSaveFile out(file);
            if (!out.open(QIODevice::WriteOnly) || out.write(text) != text.size() || !out.commit())
            {
                saved->error = qtr("The subtitle couldn't be saved.");
                return;
            }
            saved->file = file;
        });
        connect(worker, &QThread::finished, this, [this, revision, done, saved]() {
            if (revision != m_downloadRevision)
                return;
            if (saved->file.isEmpty())
                done({}, saved->error.isEmpty() ? qtr("The subtitle couldn't be unpacked.") : saved->error);
            else
                done(saved->file, {});
        });
        connect(worker, &QThread::finished, worker, &QObject::deleteLater);
        worker->start(QThread::LowPriority);
    });
}
