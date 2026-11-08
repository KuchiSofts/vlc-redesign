/* Copyright (C) 2026 VLC authors and VideoLAN. GPL version 2 or later.
 *
 * What the updater counts on (modules/gui/qt/util/redesign_updater.cpp): a
 * QSaveFile's commit() puts its data on the disk (fsync or fdatasync of the
 * new file) before it renames the file into place, and keeps the
 * permissions of the file it replaces (the AppImage's execute bit) or those
 * set before commit(). Seen through extras/juxtopposed/sync-probe.c, which
 * notes those calls in their order. (On Windows, Qt's syncToDisk of a file
 * open for writing is FlushFileBuffers: QFSFileEnginePrivate::nativeSyncToDisk.)
 *   gcc -shared -fPIC -O1 extras/juxtopposed/sync-probe.c -o sync-probe.so -ldl
 *   g++ -std=c++17 -O1 -Wall -Wextra -fPIC $(pkg-config --cflags Qt6Core) \
 *       extras/juxtopposed/savefile-sync-test.cpp $(pkg-config --libs Qt6Core) -ldl -o savefile-sync-test
 *   LD_PRELOAD=./sync-probe.so ./savefile-sync-test
 * (no sanitizer: the probe is preloaded first)
 */
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QTemporaryDir>

#include <cstdio>
#include <dlfcn.h>

static int checks = 0;
static int failures = 0;
#define CHECK(condition) do { ++checks; if (!(condition)) { \
    std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); ++failures; } } while (0)

static QStringList notes()
{
    using Log = const char *(*)();
    const auto log = reinterpret_cast<Log>(dlsym(RTLD_DEFAULT, "sync_probe_log"));
    return log ? QString::fromUtf8(log()).split(QLatin1Char('\n'), Qt::SkipEmptyParts) : QStringList();
}

// The notes from the one at from: whether the file that first takes the
// name of target (or of its temporary copy, "<target>.XXXXXX") was synced
// before. Qt writes a temporary file (or an O_TMPFILE one: "#<inode>
// (deleted)"), syncs it, then links or renames it into place.
static bool syncedBeforeRename(const QStringList &all, qsizetype from, const QString &target)
{
    QStringList synced;
    for (qsizetype i = from; i < all.size(); ++i)
    {
        const QString &line = all[i];
        if (line.startsWith(QLatin1String("sync ")))
            synced.append(line.mid(5));
        const qsizetype arrow = line.indexOf(QLatin1String(" -> "));
        if (arrow < 0 || !(line.startsWith(QLatin1String("rename ")) || line.startsWith(QLatin1String("link "))))
            continue;
        const QString to = line.mid(arrow + 4);
        if (to != target && !to.startsWith(target + QLatin1Char('.')))
            continue;
        const QString source = line.mid(line.indexOf(QLatin1Char(' ')) + 1, arrow - line.indexOf(QLatin1Char(' ')) - 1);
        return synced.contains(source);
    }
    return false;
}

int main()
{
    if (!dlsym(RTLD_DEFAULT, "sync_probe_log"))
    {
        std::fprintf(stderr, "run it with LD_PRELOAD=sync-probe.so\n");
        return 1;
    }
    QTemporaryDir dir;
    CHECK(dir.isValid());
    const QString target = QDir(dir.path()).absoluteFilePath(QStringLiteral("VLC-Redesign.AppImage"));

    // an executable replaced: synced, renamed, still executable
    {
        QFile old(target);
        CHECK(old.open(QIODevice::WriteOnly) && old.write("old") == 3);
        old.close();
        CHECK(QFile::setPermissions(target, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner));
        const qsizetype from = notes().size();
        QSaveFile save(target);
        CHECK(save.open(QIODevice::WriteOnly));
        const QByteArray data(1 << 20, 'n');
        CHECK(save.write(data) == data.size());
        CHECK(save.commit());
        CHECK(syncedBeforeRename(notes(), from, target));
        QFile now(target);
        CHECK(now.open(QIODevice::ReadOnly) && now.readAll() == data);
        CHECK(QFile::permissions(target) & QFileDevice::ExeOwner);
    }
    // a new file, its permissions set before commit()
    {
        const QString fresh = QDir(dir.path()).absoluteFilePath(QStringLiteral("ready.txt"));
        const qsizetype from = notes().size();
        QSaveFile save(fresh);
        CHECK(save.open(QIODevice::WriteOnly) && save.write("0.5.9") == 5);
        CHECK(save.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner));
        CHECK(save.commit());
        CHECK(syncedBeforeRename(notes(), from, fresh));
        CHECK(QFile::permissions(fresh) & QFileDevice::ExeOwner);
    }
    if (failures)
        std::fprintf(stderr, "the probe's notes:\n%s\n", qPrintable(notes().join(QLatin1Char('\n'))));
    std::printf("QSaveFile: %d checks, %d failed (synced before its rename, permissions kept)\n", checks, failures);
    return failures ? 1 : 0;
}
