/* Copyright (C) 2026 VLC authors and VideoLAN. GPL version 2 or later. */
#include "modules/gui/qt/player/local_folder_order.hpp"

#include <QLocale>
#include <cstdio>

namespace {
struct Entry
{
    QString name;
    bool isFolder;
    qint64 modified;
    double progress = 0.;
    unsigned playCount = 0;
    bool isAudio = false;
};

bool checkRows(const char* label, const QVector<int>& rows, const QVector<int>& expected)
{
    if (rows == expected)
        return true;
    std::fprintf(stderr, "Wrong row order for %s\n", label);
    return false;
}
}

int main()
{
    const QLocale locale(QLocale::English, QLocale::UnitedStates);
    QVector<Entry> entries {
        { QStringLiteral("clip 10"), false, 500 },
        { QStringLiteral("clip 8"), true, 200 },
        { QStringLiteral("CLIP 2"), false, 300, 0.2 },
        { QStringLiteral("CLIP 3"), true, 100 },
        { QStringLiteral("Audio"), false, 400, 0., 1 },
        { QStringLiteral("clip 11"), false, 600 },
    };

    const auto sorted = [&](const QString& pattern, const QString& criteria, Qt::SortOrder order) {
        auto rows = LocalFolderOrder::matchingRows(entries, pattern);
        LocalFolderOrder::sortRows(rows, entries, criteria, order, locale);
        return rows;
    };
    bool ok = true;
    ok &= checkRows("case-insensitive search and natural names",
                    sorted(QStringLiteral("cLiP"), QStringLiteral("name"), Qt::AscendingOrder),
                    { 3, 1, 2, 0, 5 });
    ok &= checkRows("descending names with folders first",
                    sorted({}, QStringLiteral("name"), Qt::DescendingOrder), { 1, 3, 5, 0, 2, 4 });
    ok &= checkRows("ascending dates", sorted({}, QStringLiteral("date"), Qt::AscendingOrder),
                    { 3, 1, 2, 4, 0, 5 });
    ok &= checkRows("descending dates", sorted({}, QStringLiteral("date"), Qt::DescendingOrder),
                    { 1, 3, 5, 0, 4, 2 });
    ok &= checkRows("unwatched first", sorted({}, QStringLiteral("unwatched"), Qt::AscendingOrder),
                    { 3, 1, 0, 5, 4, 2 });
    ok &= checkRows("watched first", sorted({}, QStringLiteral("unwatched"), Qt::DescendingOrder),
                    { 3, 1, 4, 2, 0, 5 });
    ok &= checkRows("no matches",
                    sorted(QStringLiteral("missing"), QStringLiteral("name"), Qt::AscendingOrder), {});

    auto filtered = sorted(QStringLiteral("clip"), QStringLiteral("unwatched"), Qt::AscendingOrder);
    entries[0].playCount = 1;
    LocalFolderOrder::sortRows(filtered, entries, QStringLiteral("unwatched"), Qt::AscendingOrder, locale);
    ok &= checkRows("enrichment of filtered rows", filtered, { 3, 1, 5, 2, 0 });

    // a played song is not a watched media: it keeps its place by name
    QVector<Entry> songs = {
        { QStringLiteral("Track 01.mp3"), false, 1, 0., 3, true },
        { QStringLiteral("Track 02.mp3"), false, 2, 0., 0, true },
        { QStringLiteral("Track 03.mp3"), false, 3, 0.4, 0, true },
        { QStringLiteral("Clip.mp4"), false, 4, 0., 1, false },
        { QStringLiteral("Film.mp4"), false, 5, 0., 0, false },
    };
    QVector<int> order = LocalFolderOrder::matchingRows(songs, QString());
    LocalFolderOrder::sortRows(order, songs, QStringLiteral("unwatched"), Qt::AscendingOrder, locale);
    ok &= checkRows("played songs keep their place", order, { 4, 0, 1, 2, 3 });

    return ok ? 0 : 1;
}
