/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or later.
 *****************************************************************************/
#pragma once

#include <QCollator>
#include <QString>
#include <QVector>

#include <algorithm>

namespace LocalFolderOrder {
template <typename Entry>
QVector<int> matchingRows(const QVector<Entry>& entries, const QString& pattern)
{
    QVector<int> rows;
    for (int i = 0; i < entries.size(); ++i)
        if (pattern.isEmpty() || entries[i].name.contains(pattern, Qt::CaseInsensitive))
            rows.push_back(i);
    return rows;
}

template <typename Entry>
void sortRows(QVector<int>& rows, const QVector<Entry>& entries, const QString& criteria,
              Qt::SortOrder sortOrder, const QLocale& locale)
{
    // Folders first, then names the way people read them (2 before 10)
    QCollator collator(locale);
    collator.setNumericMode(true);
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    const bool byDate = criteria == QLatin1String("date");
    const bool byWatched = criteria == QLatin1String("unwatched");
    const bool descending = sortOrder == Qt::DescendingOrder;
    const auto played = [](const Entry& e) { return e.playCount > 0 || e.progress > 0.; };
    std::sort(rows.begin(), rows.end(), [&](int rowA, int rowB) {
        const Entry& a = entries[rowA];
        const Entry& b = entries[rowB];
        if (a.isFolder != b.isFolder)
            return a.isFolder;
        if (byWatched && played(a) != played(b))
            return descending ? played(a) : played(b);
        const int order = byDate ? (a.modified < b.modified ? -1 : a.modified > b.modified)
                                 : collator.compare(a.name, b.name);
        return (descending && !byWatched) ? order > 0 : order < 0;
    });
}
}
