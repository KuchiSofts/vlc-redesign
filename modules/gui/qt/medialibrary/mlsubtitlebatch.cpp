/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * ( at your option ) any later version.
 *****************************************************************************/
#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "mlsubtitlebatch.hpp"

#include <QTimer>

#include <algorithm>

#include <vlc_common.h>
#include <vlc_configuration.h>

#include "medialib.hpp"
#include "mlsubtitleassociations.hpp"
#include "player/subtitle_finder.hpp"
#include "player/subtitle_search.hpp"
#include "util/subtitle_preferences.hpp"

namespace {
// videos searched at a time (each asks its services together): the
// services are not asked for a whole season at once
constexpr int maxSearches = 2;
constexpr int maxDownloads = 2;
// a search not over by then (a service that never answers): given up,
// the others go on
constexpr int searchTimeoutMs = 90 * 1000;
// the subtitles of a video listed at most
constexpr int maxCandidates = 12;

// with the files linked by the dialog (kept, unlike the finder's cache)
QString linkedFolder(const QString& mediaKey)
{
    char *profile = config_GetUserDir(VLC_USERDATA_DIR);
    const QString folder = QString::fromUtf8(profile ? profile : "") + QStringLiteral("/linked-subtitles/")
                           + SubtitleSearch::folderName(mediaKey);
    free(profile);
    return folder;
}
}

MLSubtitleBatch::MLSubtitleBatch(QObject* parent) : QAbstractListModel(parent) {}

MLSubtitleBatch::~MLSubtitleBatch()
{
    // (their answers on the way dropped with them)
    for (const Row& row : m_rows)
        delete row.search;
}

int MLSubtitleBatch::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : int(m_rows.size());
}

QVariant MLSubtitleBatch::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_rows.size())
        return {};
    const Row& row = m_rows[index.row()];
    switch (role)
    {
    case LabelRole: return row.label;
    case StateRole: return int(row.state);
    case TitleRole: return row.title;
    case CandidatesRole: return row.candidates;
    case ChoiceRole: return row.choice;
    case ErrorRole: return row.error;
    default: return {};
    }
}

QHash<int, QByteArray> MLSubtitleBatch::roleNames() const
{
    return {
        { LabelRole, "label" },
        { StateRole, "rowState" },
        { TitleRole, "title" },
        { CandidatesRole, "candidates" },
        { ChoiceRole, "choice" },
        { ErrorRole, "error" },
    };
}

int MLSubtitleBatch::count(RowState state) const
{
    return int(std::count_if(m_rows.cbegin(), m_rows.cend(), [state](const Row& row) { return row.state == state; }));
}

bool MLSubtitleBatch::searching() const
{
    return count(Waiting) + count(Searching) > 0;
}

bool MLSubtitleBatch::downloading() const
{
    return m_activeDownloads > 0 || !m_downloadQueue.isEmpty();
}

int MLSubtitleBatch::searched() const
{
    return total() - count(Waiting) - count(Searching);
}

int MLSubtitleBatch::chosenCount() const
{
    return int(std::count_if(m_rows.cbegin(), m_rows.cend(), [](const Row& row) {
        return row.choice >= 0 && (row.state == Exact || row.state == Guess);
    }));
}

QString MLSubtitleBatch::language() const
{
    return m_finder ? m_finder->preferences()->primary() : QString();
}

QString MLSubtitleBatch::secondLanguage() const
{
    return m_finder ? m_finder->preferences()->secondary() : QString();
}

void MLSubtitleBatch::setState(int row, RowState state, const QString& error)
{
    m_rows[row].state = state;
    m_rows[row].error = error;
    const QModelIndex at = index(row);
    emit dataChanged(at, at);
    emit progressChanged();
}

void MLSubtitleBatch::start(const QVariantList& media)
{
    cancel();
    QVector<Row> rows;
    for (const QVariant& value : media)
    {
        const QVariantMap entry = value.toMap();
        Row row;
        row.url = entry.value(QStringLiteral("url")).toUrl();
        row.label = entry.value(QStringLiteral("label")).toString();
        if (row.url.isLocalFile())
            rows.push_back(row);
    }
    if (rows.isEmpty())
        return;
    beginResetModel();
    m_rows = std::move(rows);
    endResetModel();
    emit progressChanged();
    pumpSearches();
}

void MLSubtitleBatch::cancel()
{
    ++m_generation;
    beginResetModel();
    for (const Row& row : m_rows)
        delete row.search;
    m_rows.clear();
    endResetModel();
    m_downloadQueue.clear();
    m_activeSearches = 0;
    m_activeDownloads = 0;
    m_downloadsStarted = false;
    emit progressChanged();
}

void MLSubtitleBatch::pumpSearches()
{
    for (int i = 0; i < m_rows.size() && m_activeSearches < maxSearches; ++i)
        if (m_rows[i].state == Waiting)
            startSearch(i);
}

void MLSubtitleBatch::startSearch(int row)
{
    Row& entry = m_rows[row];
    if (!m_finder || !m_finder->supported())
    {
        setState(row, Failed, qtr("This build can't search the internet."));
        return;
    }
    ++m_activeSearches;
    SubtitleSearch* search = m_finder->newSearch(this);
    entry.search = search;
    search->setMedia(entry.url, entry.label);
    const quint64 generation = m_generation;
    connect(search, &SubtitleSearch::finished, this, [this, generation, row, search]() {
        if (generation == m_generation && m_rows[row].search == search && m_rows[row].state == Searching)
            onSearched(row);
    });
    // (a service that never answers: this video given up, not the others)
    QTimer::singleShot(searchTimeoutMs, search, [this, generation, row, search]() {
        if (generation != m_generation || m_rows[row].search != search || m_rows[row].state != Searching)
            return;
        m_rows[row].search = nullptr;
        search->deleteLater();
        --m_activeSearches;
        setState(row, Failed, qtr("The subtitle services took too long to answer."));
        pumpSearches();
    });
    setState(row, Searching);
    search->start();
}

// Its subtitles of the user's language, else of the second one: the one
// made for this file chosen, else the best one of a title identified for
// sure; a guess from its name alone is left to the user
void MLSubtitleBatch::onSearched(int row)
{
    --m_activeSearches;
    Row& entry = m_rows[row];
    SubtitleSearch* search = entry.search;
    const QString primary = language();
    const QString secondary = secondLanguage();
    const QVariantList results = search->results();
    for (const QString& wanted : { primary, secondary })
    {
        if (wanted.isEmpty() || !entry.candidates.isEmpty())
            continue;
        for (const QVariant& value : results)
        {
            const QVariantMap result = value.toMap();
            if (result.value(QStringLiteral("language")).toString() != wanted)
                continue;
            QString detail = result.value(QStringLiteral("providerName")).toString();
            if (wanted != primary)
                detail += QStringLiteral(" · ") + result.value(QStringLiteral("languageName")).toString();
            entry.candidates.append(QVariantMap{
                { QStringLiteral("id"), result.value(QStringLiteral("id")) },
                { QStringLiteral("label"), result.value(QStringLiteral("release")) },
                { QStringLiteral("detail"), detail },
                { QStringLiteral("exact"), result.value(QStringLiteral("exact")) },
            });
            if (entry.candidates.size() == maxCandidates)
                break;
        }
    }
    entry.title = search->matchTitle();
    if (entry.candidates.isEmpty())
    {
        if (search->state() == SubtitleSearch::Failed)
            setState(row, Failed, search->errorText());
        else
            // (subtitles of other languages only: not found either)
            setState(row, NotFound, search->errorText());
        // nothing to download: forgotten
        entry.search = nullptr;
        search->deleteLater();
    }
    else
    {
        const bool exact = entry.candidates.front().toMap().value(QStringLiteral("exact")).toBool();
        entry.choice = exact || search->match().strong ? 0 : -1;
        setState(row, exact ? Exact : Guess);
    }
    pumpSearches();
}

void MLSubtitleBatch::choose(int row, int candidate)
{
    if (row < 0 || row >= m_rows.size() || downloading())
        return;
    Row& entry = m_rows[row];
    if ((entry.state != Exact && entry.state != Guess) || candidate < -1 || candidate >= entry.candidates.size())
        return;
    entry.choice = candidate;
    const QModelIndex at = index(row);
    emit dataChanged(at, at);
    emit progressChanged();
}

void MLSubtitleBatch::download()
{
    if (downloading() || !m_ml)
        return;
    for (int i = 0; i < m_rows.size(); ++i)
    {
        const Row& entry = m_rows[i];
        if (entry.choice >= 0 && (entry.state == Exact || entry.state == Guess) && entry.search)
            m_downloadQueue.append(i);
    }
    if (m_downloadQueue.isEmpty())
        return;
    m_downloadsStarted = true;
    for (int i : m_downloadQueue)
        setState(i, Downloading);
    pumpDownloads();
}

void MLSubtitleBatch::pumpDownloads()
{
    while (m_activeDownloads < maxDownloads && !m_downloadQueue.isEmpty())
        startDownload(m_downloadQueue.takeFirst());
    if (m_downloadsStarted && !downloading())
    {
        m_downloadsStarted = false;
        int failed = 0;
        for (const Row& row : m_rows)
            if (row.state == Failed && row.choice >= 0)
                ++failed;
        emit progressChanged();
        emit finished(linkedCount(), failed);
    }
}

void MLSubtitleBatch::startDownload(int row)
{
    ++m_activeDownloads;
    Row& entry = m_rows[row];
    SubtitleSearch* search = entry.search;
    const QString id = entry.candidates.value(entry.choice).toMap().value(QStringLiteral("id")).toString();
    const quint64 generation = m_generation;
    search->download(id, linkedFolder(search->mediaKey()), false,
                     [this, generation, row, search](const QString& file, const QString& error) {
        if (generation != m_generation || m_rows[row].search != search)
            return;
        if (file.isEmpty())
            downloadDone(row, Failed, error);
        else
            linkFile(row, file);
    });
}

// Linked to its video as a subtitle added by hand (MLSubtitleAssociations),
// then known as found online, on its service: in the player's list now,
// and when the video plays later (SubtitleFinder: the choice remembered)
void MLSubtitleBatch::linkFile(int row, const QString& file)
{
    Row& entry = m_rows[row];
    SubtitleSearch* search = entry.search;
    const QString id = entry.candidates.value(entry.choice).toMap().value(QStringLiteral("id")).toString();
    const QVariantMap result = search->resultMap(id);
    const QUrl fileUrl = QUrl::fromLocalFile(file);
    const QByteArray fileMrl = MLSubtitleAssociations::mrlOf(fileUrl);
    const quint64 generation = m_generation;
    MLSubtitleAssociations::link(m_ml, this, { { 0, MLSubtitleAssociations::mrlOf(entry.url), fileMrl } }, false,
                                 [this, generation, row, search, file, fileUrl, fileMrl, id, result](int, const QString& error) {
        if (generation != m_generation || m_rows[row].search != search)
            return;
        if (!error.isEmpty())
        {
            downloadDone(row, Failed, error);
            return;
        }
        const QString release = result.value(QStringLiteral("release")).toString();
        const QString provider = result.value(QStringLiteral("providerName")).toString();
        for (const QString& mrl : { fileUrl.toString(QUrl::FullyEncoded), QString::fromUtf8(fileMrl) })
            SubtitleFinder::noteFile(mrl, QStringLiteral("online"), release, provider);
        if (m_finder)
            SubtitleSearch::rememberChoice(m_finder->settings(), search->mediaKey(),
                                           { file, id, result.value(QStringLiteral("language")).toString(),
                                             release, provider });
        downloadDone(row, Linked);
    });
}

void MLSubtitleBatch::downloadDone(int row, RowState state, const QString& error)
{
    --m_activeDownloads;
    setState(row, state, error);
    if (state == Linked)
    {
        // done with it (deleted once its answer is over)
        m_rows[row].search->deleteLater();
        m_rows[row].search = nullptr;
    }
    pumpDownloads();
}
