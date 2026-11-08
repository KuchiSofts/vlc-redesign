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

#ifdef _WIN32
# include <windows.h>
# include <wincrypt.h>
#endif

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
// those removed from the list
const char keyRemoved[] = "redesign/subtitle-service-removed/";
// the user's credentials ("redesign/subtitle-credentials/<id>/<field>")
const char keyCredentials[] = "redesign/subtitle-credentials/";
// the credentials a search found refused ("<kind>:<their fingerprint>"):
// shown on the service's row until they change or a search succeeds
const char keyRefused[] = "redesign/subtitle-service-refused/";
// a service that asked to wait (429): shown so for a minute
constexpr qint64 busyMs = 60 * 1000;

QHash<QString, qint64> &busyUntil()
{
    static QHash<QString, qint64> until;
    return until;
}

// A secret as kept: on Windows sealed for this user on this PC ("dpapi:"
// and the sealed bytes), elsewhere as it is (VLC keeps its own passwords so)
QString seal(const QString &plain)
{
    if (plain.isEmpty())
        return {};
#ifdef _WIN32
    QByteArray in = plain.toUtf8();
    DATA_BLOB input = { DWORD(in.size()), reinterpret_cast<BYTE *>(in.data()) };
    DATA_BLOB output = {};
    if (!CryptProtectData(&input, L"VLC Redesign", nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output))
        return {};
    const QByteArray sealed(reinterpret_cast<const char *>(output.pbData), int(output.cbData));
    LocalFree(output.pbData);
    return QStringLiteral("dpapi:") + QString::fromLatin1(sealed.toBase64());
#else
    return plain;
#endif
}

QString unseal(const QString &kept)
{
    if (!kept.startsWith(QLatin1String("dpapi:")))
        return kept;
#ifdef _WIN32
    QByteArray in = QByteArray::fromBase64(kept.mid(6).toLatin1());
    DATA_BLOB input = { DWORD(in.size()), reinterpret_cast<BYTE *>(in.data()) };
    DATA_BLOB output = {};
    // (sealed for another user or PC: unknown here)
    if (!CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output))
        return {};
    const QString plain = QString::fromUtf8(reinterpret_cast<const char *>(output.pbData), int(output.cbData));
    SecureZeroMemory(output.pbData, output.cbData);
    LocalFree(output.pbData);
    return plain;
#else
    return {};
#endif
}

// What identifies credentials without them (their refusal is of these)
QString fingerprint(const SubtitleProviders::Credentials &c)
{
    const QByteArray all = (c.key + QChar(1) + c.username + QChar(1) + c.password + QChar(1) + c.email).toUtf8();
    return QString::fromLatin1(QCryptographicHash::hash(all, QCryptographicHash::Sha256).toHex().left(16));
}
constexpr qsizetype maxJsonBytes = 4 * 1024 * 1024;
constexpr qsizetype maxSubtitleBytes = 8 * 1024 * 1024;
// the downloads of media not played for that long leave the cache
constexpr int cacheDays = 60;
// The whole of a request, however slowly it is answered (each step has its
// own timeout, but an answer a byte at a time would go on for days): a
// question, a download
constexpr int jsonDeadlineMs = 45 * 1000;
constexpr int downloadDeadlineMs = 90 * 1000;
// A search's bounds (its watchdog): the video identified (its file hashed,
// IMDb, Wikidata, Cinemeta: seconds), then the services asked together
// (each a few requests of at most jsonDeadlineMs). The services that have
// not answered by then are left out of its results
constexpr int identifyTimeoutMs = 60 * 1000;
constexpr int searchTimeoutMs = 90 * 1000;

HttpRequest jsonRequest(const QUrl &url, int timeoutMs = 20000)
{
    HttpRequest request;
    request.url = url;
    request.timeoutMs = timeoutMs;
    request.deadlineMs = jsonDeadlineMs;
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

// "Some.Film.Title.2021.2160p.4K.WEB.x265-[GROUP].mkv": the title,
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

// The answers of IMDb, Wikidata and Cinemeta read on the worker thread of
// their request (up to 4 MB each): nothing here touches the interface

// IMDb's suggestions: { imdb, label, description, year, series, film, rank }
QVariantList imdbCandidates(const QByteArray &body)
{
    QVariantList candidates;
    int rank = 0;
    for (const QJsonValue item : QJsonDocument::fromJson(body).object().value(QLatin1String("d")).toArray())
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
    return candidates;
}

// Wikidata's items of a name: their ids, labels and descriptions
struct WikidataFound
{
    QStringList ids;
    QHash<QString, QPair<QString, QString>> labels; // id -> label, description
};

WikidataFound wikidataFound(const QByteArray &body)
{
    WikidataFound found;
    for (const QJsonValue item : QJsonDocument::fromJson(body).object().value(QLatin1String("search")).toArray())
    {
        const QJsonObject o = item.toObject();
        const QString id = o.value(QLatin1String("id")).toString();
        if (id.isEmpty())
            continue;
        found.ids.append(id);
        found.labels.insert(id, { o.value(QLatin1String("label")).toString(),
                                  o.value(QLatin1String("description")).toString() });
    }
    return found;
}

// Those items: their IMDb id (P345), what they are (P31) and their date
// (P577, or P580 for a series), as IMDb's candidates
QVariantList wikidataCandidates(const QByteArray &body, const QStringList &ids,
                                const QHash<QString, QPair<QString, QString>> &labels)
{
    QVariantList candidates;
    const QJsonObject all = QJsonDocument::fromJson(body).object().value(QLatin1String("entities")).toObject();
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
    return candidates;
}

// Cinemeta's catalogue: its titles, in its order
struct CinemetaMeta
{
    QString id;
    QString name;
    int year = 0;
};

QList<CinemetaMeta> cinemetaMetas(const QByteArray &body)
{
    QList<CinemetaMeta> metas;
    for (const QJsonValue item : QJsonDocument::fromJson(body).object().value(QLatin1String("metas")).toArray())
    {
        const QJsonObject o = item.toObject();
        metas.append({ o.value(QLatin1String("id")).toString(), o.value(QLatin1String("name")).toString(),
                       o.value(QLatin1String("releaseInfo")).toString().left(4).toInt() });
    }
    return metas;
}

} // namespace

SubtitleSearch::SubtitleSearch(qt_intf_t *intf, SubtitlePreferences *preferences, QObject *parent)
    : QObject(parent)
    , m_intf(intf)
    , m_preferences(preferences)
{
    m_watchdog.setSingleShot(true);
    connect(&m_watchdog, &QTimer::timeout, this, &SubtitleSearch::onWatchdog);
}

QString SubtitleSearch::folderName(const QString &mediaKey)
{
    return QString::fromLatin1(QCryptographicHash::hash(mediaKey.toUtf8(), QCryptographicHash::Sha1).toHex().left(20));
}

QString SubtitleSearch::cacheRoot()
{
    char *cache = config_GetUserDir(VLC_CACHE_DIR);
    // (none, or an empty one: no cache, not "/subtitles" at the root)
    const QString root = cache && *cache ? QString::fromUtf8(cache) + QStringLiteral("/subtitles") : QString();
    free(cache);
    return root;
}

bool SubtitleSearch::serviceEnabled(QSettings *settings, const QString &id)
{
    // on at first: the free services without an account; the others once
    // turned on (or their credentials checked)
    const SubtitleProviders::Provider *provider = SubtitleProviders::provider(id);
    return settings->value(QString::fromLatin1(keyService) + id, provider && provider->fields == 0).toBool();
}

bool SubtitleSearch::serviceSwitched(QSettings *settings, const QString &id)
{
    return settings->contains(QString::fromLatin1(keyService) + id);
}

void SubtitleSearch::setServiceEnabled(QSettings *settings, const QString &id, bool enabled)
{
    settings->setValue(QString::fromLatin1(keyService) + id, enabled);
}

bool SubtitleSearch::serviceRemoved(QSettings *settings, const QString &id)
{
    return settings->value(QString::fromLatin1(keyRemoved) + id, false).toBool();
}

void SubtitleSearch::setServiceRemoved(QSettings *settings, const QString &id, bool removed)
{
    if (removed)
        settings->setValue(QString::fromLatin1(keyRemoved) + id, true);
    else
        settings->remove(QString::fromLatin1(keyRemoved) + id);
}

SubtitleProviders::Credentials SubtitleSearch::credentials(QSettings *settings, const QString &id)
{
    const QString prefix = QString::fromLatin1(keyCredentials) + id + QLatin1Char('/');
    const auto value = [&](const char *field) {
        return unseal(settings->value(prefix + QLatin1String(field)).toString());
    };
    SubtitleProviders::Credentials c;
    c.key = value("key");
    c.username = value("username");
    c.password = value("password");
    c.email = value("email");
    return c;
}

bool SubtitleSearch::setCredentials(QSettings *settings, const QString &id,
                                    const SubtitleProviders::Credentials &credentials)
{
    if (credentials == SubtitleSearch::credentials(settings, id))
        return false;
    const QString prefix = QString::fromLatin1(keyCredentials) + id + QLatin1Char('/');
    const auto store = [&](const char *field, const QString &value) {
        if (value.isEmpty())
            settings->remove(prefix + QLatin1String(field));
        else
            settings->setValue(prefix + QLatin1String(field), seal(value));
    };
    store("key", credentials.key.trimmed());
    store("username", credentials.username.trimmed());
    store("password", credentials.password);
    store("email", credentials.email.trimmed());
    // (new ones: not refused, nor signed in with the old)
    settings->remove(QString::fromLatin1(keyRefused) + id);
    busyUntil().remove(id);
    SubtitleProviders::forgetSession(id);
    return true;
}

SubtitleSearch::ServiceStatus SubtitleSearch::serviceStatus(QSettings *settings, const QString &id)
{
    const QString refused = settings->value(QString::fromLatin1(keyRefused) + id).toString();
    if (!refused.isEmpty() && refused.section(QLatin1Char(':'), 1) == fingerprint(credentials(settings, id)))
        return refused.startsWith(QLatin1String("login:")) ? StatusLoginRefused : StatusKeyRefused;
    if (busyUntil().value(id) > QDateTime::currentMSecsSinceEpoch())
        return StatusBusy;
    return StatusOk;
}

void SubtitleSearch::noteProblem(QSettings *settings, const QString &id, SubtitleProviders::Problem problem)
{
    using SubtitleProviders::Problem;
    const QString key = QString::fromLatin1(keyRefused) + id;
    switch (problem)
    {
        case Problem::None:
            if (settings->contains(key))
                settings->remove(key);
            busyUntil().remove(id);
            break;
        case Problem::KeyRefused:
        case Problem::LoginRefused:
            settings->setValue(key, QString(QLatin1String(problem == Problem::KeyRefused ? "key:" : "login:")
                                            + fingerprint(credentials(settings, id))));
            break;
        case Problem::Busy:
            busyUntil().insert(id, QDateTime::currentMSecsSinceEpoch() + busyMs);
            break;
        default:
            break;
    }
}

bool SubtitleSearch::serviceAsked(QSettings *settings, const SubtitleProviders::Provider &provider,
                                  const QStringList &languages)
{
    if (!serviceEnabled(settings, provider.id) || serviceRemoved(settings, provider.id))
        return false;
    if (provider.required && !SubtitleProviders::hasRequired(provider, credentials(settings, provider.id)))
        return false;
    if (provider.languages.isEmpty())
        return true;
    return std::any_of(provider.languages.cbegin(), provider.languages.cend(),
                       [&languages](const QString &l) { return languages.contains(l); });
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
    m_watchdog.stop();
    ++m_revision;
    ++m_searchRevision;
    ++m_downloadRevision;
    m_url.clear();
    m_media = Media();
    m_match = Match();
    m_hashAsked = false;
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
    // (waited for at the exit, a few seconds at most: runInBackground; its
    // answer dropped if the search is gone)
    runInBackground(this, [hashed, path]() {
        QThread::currentThread()->setPriority(QThread::LowPriority);
        if (!path.isEmpty())
            hashed->hash = openSubtitlesHash(path, &hashed->size);
    }, [this, hashed, revision, next]() {
        if (revision != m_revision)
            return;
        m_media.hash = hashed->hash;
        m_media.size = hashed->size;
        m_media.key = !hashed->hash.isEmpty() ? hashed->hash + QLatin1Char(':') + QString::number(hashed->size)
                                              : m_url.toString();
        next();
    });
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
    // (a file on a drive that no longer answers, a service that never ends)
    m_watchdog.start(identifyTimeoutMs);
    prepare([this]() { identify(); });
}

void SubtitleSearch::onWatchdog()
{
    if (m_state == Identifying)
    {
        // the answers of this media on their way are dropped (the next
        // search hashes and identifies it again)
        ++m_revision;
        msg_Warn(m_intf, "online subtitles: %s: not identified in %d s", qtu(m_media.name), identifyTimeoutMs / 1000);
        setState(Failed, qtr("The subtitle services took too long to answer."));
        emit finished();
    }
    else if (m_state == Searching)
    {
        // the services that haven't answered are left out (their answers
        // dropped), the search ends with the others'
        ++m_searchRevision;
        for (auto it = m_runs.begin(); it != m_runs.end(); ++it)
        {
            if (!it->busy)
                continue;
            const SubtitleProviders::Provider *provider = SubtitleProviders::provider(it.key());
            msg_Warn(m_intf, "online subtitles: %s: no answer in %d s", qtu(it.key()), searchTimeoutMs / 1000);
            it->busy = false;
            it->error = qtr("%1 didn't answer.").arg(provider ? provider->name : it.key());
            it->problem = SubtitleProviders::Problem::NoAnswer;
        }
        m_pending = 0;
        emit runsChanged();
        finishSearch();
    }
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
    // (read on the worker thread of the request: a large answer doesn't
    // hold the interface)
    auto parsed = std::make_shared<QVariantList>();
    httpRequestAsync(this, jsonRequest(url, 15000), [parsed](const HttpResponse &reply) {
        if (replyOk(reply))
            *parsed = imdbCandidates(reply.body);
    }, [this, revision, forMedia, query, done, parsed](const HttpResponse &) {
        if (forMedia && revision != m_revision)
            return;
        const QVariantList candidates = *parsed;
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
    auto found = std::make_shared<WikidataFound>();
    httpRequestAsync(this, jsonRequest(url), [found](const HttpResponse &reply) {
        if (replyOk(reply))
            *found = wikidataFound(reply.body);
    }, [this, revision, forMedia, done, found](const HttpResponse &reply) {
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
        const QStringList ids = found->ids;
        const QHash<QString, QPair<QString, QString>> labels = found->labels; // id -> label, description
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
        auto candidates = std::make_shared<QVariantList>();
        httpRequestAsync(this, jsonRequest(entities), [candidates, ids, labels](const HttpResponse &reply) {
            if (replyOk(reply))
                *candidates = wikidataCandidates(reply.body, ids, labels);
        }, [this, revision, forMedia, done, candidates](const HttpResponse &) {
            if (forMedia && revision != m_revision)
                return;
            onTitles(*candidates, forMedia, done);
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
    auto metas = std::make_shared<QList<CinemetaMeta>>();
    httpRequestAsync(this, jsonRequest(url), [metas](const HttpResponse &reply) {
        if (replyOk(reply))
            *metas = cinemetaMetas(reply.body);
    }, [this, revision, series, metas](const HttpResponse &reply) {
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
        for (const CinemetaMeta &meta : std::as_const(*metas))
        {
            const QString &id = meta.id;
            const QString &name = meta.name;
            const int year = meta.year;
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
    // its name didn't say what it is, or only a guess: OpenSubtitles may know
    // this very file (its hash). With its title every service is asked (YIFY
    // and Wizdom go by the IMDb id), not only the hash's
    if ((m_match.imdb.isEmpty() || !m_match.strong) && !m_media.hash.isEmpty() && !m_hashAsked)
    {
        m_hashAsked = true;
        const quint64 revision = m_revision;
        SubtitleProviders::identifyByHash(this, m_media.hash, m_media.size,
            [this, revision](const SubtitleProviders::Identity &found) {
                if (revision != m_revision)
                    return;
                // (about this very file: it wins over a guess from the name)
                if (!found.imdb.isEmpty())
                {
                    msg_Dbg(m_intf, "online subtitles: the file's hash says %s (%s)%s", qtu(found.title),
                            qtu(found.imdb), found.sure ? "" : ", a guess");
                    m_match = { found.imdb, found.title, found.year, found.series, found.sure };
                    if (found.series && m_media.season < 0 && found.season >= 0)
                    {
                        m_media.season = found.season;
                        m_media.episode = found.episode;
                    }
                }
                querySubtitles();
            });
        return;
    }
    ++m_searchRevision;
    const quint64 revision = m_searchRevision;
    msg_Dbg(m_intf, "online subtitles: \"%s\" is %s (%s%s), the hash %s",
            qtu(m_media.title), m_match.imdb.isEmpty() ? "unknown" : qtu(m_match.title),
            m_match.imdb.isEmpty() ? "-" : qtu(m_match.imdb),
            m_match.imdb.isEmpty() ? "" : m_match.strong ? ", sure" : ", a guess: not loaded by itself",
            m_media.hash.isEmpty() ? "unknown" : qtu(m_media.hash));
    setState(Searching);
    m_watchdog.start(searchTimeoutMs);
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
    // the languages to search (Settings), and the others or not
    m_languages = m_preferences->searched();
    m_everyLanguage = m_preferences->everyLanguage();
    query.languages = m_languages;
    query.everyLanguage = m_everyLanguage;

    QStringList asked;
    for (const SubtitleProviders::Provider &provider : SubtitleProviders::providers())
        if (serviceAsked(m_intf->mainSettings, provider, query.languages)
            && (query.series ? provider.series : provider.movies))
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
    msg_Dbg(m_intf, "online subtitles: asking %s in %s%s", qtu(asked.join(QLatin1String(", "))),
            qtu(query.languages.join(QLatin1Char(','))), query.everyLanguage ? " and every other language" : "");
    for (const QString &id : asked)
    {
        QElapsedTimer timer;
        timer.start();
        // (each its own credentials, never logged)
        SubtitleProviders::Query own = query;
        own.credentials = credentials(m_intf->mainSettings, id);
        SubtitleProviders::search(this, id, own,
            [this, revision, id, timer](const QList<SubtitleProviders::Result> &results, const QString &error,
                                        SubtitleProviders::Problem problem) {
                if (revision != m_searchRevision)
                    return;
                msg_Dbg(m_intf, "online subtitles: %s: %lld in %lld ms%s%s", qtu(id), (long long)results.size(),
                        (long long)timer.elapsed(), error.isEmpty() ? "" : ", ", qtu(error));
                onProviderDone(id, results, error, problem);
            });
        if (revision != m_searchRevision)
            return; // (another search started from an answer)
    }
}

void SubtitleSearch::onProviderDone(const QString &id, const QList<SubtitleProviders::Result> &results,
                                    const QString &error, SubtitleProviders::Problem problem)
{
    // what it found of the user's credentials: on its row in Settings
    noteProblem(m_intf->mainSettings, id, problem);
    // only the languages to search, unless every other one is asked too
    QList<SubtitleProviders::Result> kept;
    for (const SubtitleProviders::Result &r : results)
        if (m_everyLanguage || m_languages.contains(r.language))
            kept.append(r);
    m_runs[id] = { false, int(kept.size()), error, problem };
    m_found.append(kept);
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
    // (in the order of the languages to search)
    const QStringList wanted = m_languages.isEmpty() ? m_preferences->searched() : m_languages;
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
        // (a rank that isn't a number would leave the list without an
        // order: the services' parsers give 0 then, and so does this)
        score += std::isfinite(r.rank) ? std::clamp(r.rank, -30., 20.) : 0.;
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
    m_watchdog.stop();
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
    // (a folder of the profile or of the cache: none known, nowhere)
    if (folder.isEmpty() || QDir::isRelativePath(folder))
    {
        done({}, qtr("The subtitle couldn't be saved."));
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
    const SubtitleProviders::Credentials own = credentials(m_intf->mainSettings, result.provider);
    // (OpenSubtitles.com gives the link of each when asked)
    SubtitleProviders::prepareDownload(this, result, own,
        [this, revision, done, result, folder, stem, pruneRoot](const SubtitleProviders::Download &prepared) {
        if (revision != m_downloadRevision)
            return;
        if (prepared.url.isEmpty())
        {
            if (prepared.problem == SubtitleProviders::Problem::KeyRefused
                || prepared.problem == SubtitleProviders::Problem::LoginRefused)
                noteProblem(m_intf->mainSettings, result.provider, prepared.problem);
            done({}, prepared.error.isEmpty() ? qtr("The subtitle couldn't be downloaded (%1).").arg(0) : prepared.error);
            return;
        }
        HttpRequest request;
        request.url = prepared.url;
        request.method = prepared.method;
        request.body = prepared.body;
        request.timeoutMs = 30000;
        request.deadlineMs = downloadDeadlineMs;
        request.maxBytes = maxSubtitleBytes;
        request.headers = prepared.headers;
        const auto unwrap = prepared.unwrap;
        httpRequestAsync(this, request, [this, revision, done, result, folder, stem, pruneRoot, unwrap](const HttpResponse &reply) {
            if (revision != m_downloadRevision)
                return;
            // (a file inside the answer: Napisy24's)
            const QByteArray file = unwrap && replyOk(reply, maxSubtitleBytes) ? unwrap(reply.body) : reply.body;
            if (!replyOk(reply, maxSubtitleBytes) || file.isEmpty())
            {
                done({}, reply.status == 0 ? qtr("No connection to the internet.")
                                           : qtr("The subtitle couldn't be downloaded (%1).").arg(reply.status));
                return;
            }
            // Unpacked, converted and saved on a worker thread (an archive can
            // be large), waited for at the exit (runInBackground); its
            // answer dropped if the search is gone
            struct Saved
            {
                QString file;
                QString error;
            };
            auto saved = std::make_shared<Saved>();
            const QByteArray body = file;
            const QString videoName = m_media.name;
            const QString keep = QFileInfo(folder).fileName();
            runInBackground(this, [saved, body, result, videoName, folder, stem, pruneRoot, keep]() {
                QThread::currentThread()->setPriority(QThread::LowPriority);
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
            }, [this, revision, done, saved]() {
                if (revision != m_downloadRevision)
                    return;
                if (saved->file.isEmpty())
                    done({}, saved->error.isEmpty() ? qtr("The subtitle couldn't be unpacked.") : saved->error);
                else
                    done(saved->file, {});
            });
        });
    });
}
