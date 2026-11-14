/* Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Files opened from Finder (#84): the QFileOpenEvents of the application
 * reach the playlist, the files of one open together, also when they come
 * before the playlist exists (a launch by a double click); other events
 * and other objects are left alone. Run by ci/unit-tests.sh (offscreen). */
#include "modules/gui/qt/util/file_open_spy.hpp"

#include <QGuiApplication>
#include <QElapsedTimer>

#include <cstdio>
#include <cstdlib>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); ++failures; } } while (0)

static void spin(int ms)
{
    QElapsedTimer t;
    t.start();
    while (t.elapsed() < ms)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
}

int main(int argc, char **argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QGuiApplication app(argc, argv);

    bool ready = false;
    int offered = 0;
    QList<QVariantList> played;
    new FileOpenSpy(&app, [&](const QVariantList &files) {
        ++offered;
        if (!ready)
            return false;
        played.append(files);
        return true;
    }, 20);

    // a launch: three files of one open, before the playlist exists
    QCoreApplication::postEvent(&app, new QFileOpenEvent(QStringLiteral("/tmp/music/a list.m3u")));
    QCoreApplication::postEvent(&app, new QFileOpenEvent(QStringLiteral("/tmp/music/b.mp3")));
    QCoreApplication::postEvent(&app, new QFileOpenEvent(QUrl(QStringLiteral("vlc://pause:5"))));
    spin(60);
    CHECK(played.isEmpty());
    CHECK(offered >= 2);   // kept and offered again
    ready = true;
    spin(80);
    CHECK(played.size() == 1);
    if (played.size() == 1)
    {
        const QVariantList &files = played.first();
        CHECK(files.size() == 3);
        if (files.size() == 3)
        {
            CHECK(files[0].toUrl() == QUrl::fromLocalFile(QStringLiteral("/tmp/music/a list.m3u")));
            CHECK(files[0].toUrl().isLocalFile());
            CHECK(files[1].toUrl().toLocalFile() == QStringLiteral("/tmp/music/b.mp3"));
            CHECK(files[2].toUrl() == QUrl(QStringLiteral("vlc://pause:5")));
        }
    }

    // while running: one more file, played on its own, nothing kept after
    played.clear();
    offered = 0;
    QCoreApplication::postEvent(&app, new QFileOpenEvent(QStringLiteral("/tmp/c.m3u8")));
    spin(60);
    CHECK(played.size() == 1 && played.first().size() == 1);
    CHECK(offered == 1);

    // another event of the application, and a file event of another object
    played.clear();
    QObject other;
    QCoreApplication::postEvent(&app, new QEvent(QEvent::User));
    QFileOpenEvent toOther(QStringLiteral("/tmp/d.mp3"));
    QCoreApplication::sendEvent(&other, &toOther);
    spin(60);
    CHECK(played.isEmpty());

    // an empty open (no file, no link) plays nothing
    QCoreApplication::postEvent(&app, new QFileOpenEvent(QString()));
    spin(60);
    CHECK(played.isEmpty());

    if (failures)
    {
        std::fprintf(stderr, "%d failure(s)\n", failures);
        return EXIT_FAILURE;
    }
    std::puts("file-open-spy: Finder's opens reach the playlist");
    return EXIT_SUCCESS;
}
