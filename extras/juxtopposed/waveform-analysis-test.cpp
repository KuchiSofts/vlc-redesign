/* Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later */
#include "modules/gui/qt/player/waveform.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QThread>

#include <cassert>
#include <cmath>
#include <cstdio>
#include <memory>
#include <sys/file.h>

static void touch(const QString& path)
{
    QFile file(path);
    assert(file.open(QIODevice::WriteOnly));
}

// The real helper path points here in the test. Hold the process open until
// released, with a lock that detects even an overlap during cancellation.
static int fakeHelper(const QString& path)
{
    QFile launches(QFileInfo(path).absolutePath() + "/launches");
    assert(launches.open(QIODevice::WriteOnly | QIODevice::Append));
    const QByteArray entry = QFileInfo(path).fileName().toUtf8() + '\n';
    assert(launches.write(entry) == entry.size() && launches.flush());
    if (flock(launches.handle(), LOCK_EX | LOCK_NB) != 0)
        return 1;
    touch(path + ".started");

    QElapsedTimer deadline;
    deadline.start();
    while (!QFile::exists(path + ".finish")) {
        if (deadline.hasExpired(15000)) return 2;
        QThread::msleep(10);
    }
    const char* peak = path.endsWith("/a") ? "0.25" : "0.75";
    std::fputs("{\"version\":1,\"durationUs\":3000000,\"sampleRate\":48000,"
               "\"playbackRate\":16,\"decodedFrames\":512,\"bins\":[", stdout);
    for (int i = 0; i < 512; ++i)
        std::printf("%s%s", i == 0 ? "" : ",", peak);
    std::puts("]}");
    return 0;
}

template <typename Predicate>
static bool waitFor(Predicate done, int timeout = 5000)
{
    QElapsedTimer timer;
    timer.start();
    do {
        QCoreApplication::processEvents();
        if (done()) return true;
        QThread::msleep(10);
    } while (!timer.hasExpired(timeout));
    return false;
}

static void request(Waveform& waveform, const QString& path)
{
    waveform.setDurationSeconds(3);
    waveform.setSource(QUrl::fromLocalFile(path));
    assert(waveform.busy());
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    if (app.arguments().size() == 2)
        return fakeHelper(app.arguments().at(1));

    QTemporaryDir temp;
    assert(temp.isValid());
    const QString a = temp.filePath("a");
    const QString b = temp.filePath("b");
    const QString c = temp.filePath("c");
    const QString d = temp.filePath("d");
    touch(a);
    touch(b);
    touch(c);
    touch(d);

    Waveform first, shared, dropped;
    auto next = std::make_unique<Waveform>();
    request(first, a);
    assert(waitFor([&] { return QFile::exists(a + ".started"); }));
    request(shared, a);
    request(*next, b);
    request(dropped, c);
    assert(!waitFor([&] { return QFile::exists(b + ".started") || !next->busy(); }, 1350));
    dropped.setSource({});

    touch(a + ".finish");
    assert(waitFor([&] { return first.ready() && shared.ready() && QFile::exists(b + ".started"); }));
    assert(first.bins().first().toDouble() == 0.25 && first.bins() == shared.bins());

    // Queue a successor behind B, then drop its sole waiter. Request B
    // again before the killed process can report back.
    Waveform successor, retry, stale;
    request(stale, b);
    stale.setSource({});
    request(successor, d);
    assert(!waitFor([&] { return QFile::exists(d + ".started") || !successor.busy(); }, 1350));
    touch(d + ".finish");
    assert(QFile::remove(b + ".started"));
    next.reset();
    request(retry, b);
    QObject::connect(&retry, &Waveform::binsChanged, &retry, [&] {
        assert(QThread::currentThread() == app.thread());
    });
    assert(waitFor([&] { return successor.ready(); }));
    assert(waitFor([&] { return QFile::exists(b + ".started"); }));
    assert(retry.busy() && !retry.ready() && retry.lastError().isEmpty());
    touch(b + ".finish");
    assert(waitFor([&] { return retry.ready(); }));
    assert(retry.bins().first().toDouble() == 0.75 && !stale.ready());

    Waveform cached;
    cached.setDurationSeconds(3);
    cached.setSource(QUrl::fromLocalFile(a));
    assert(cached.ready() && !cached.busy() && cached.bins() == first.bins());
    QFile launches(temp.filePath("launches"));
    assert(launches.open(QIODevice::ReadOnly) && launches.readAll() == "a\nb\nd\nb\n");
    launches.close();

    // Kept on the disk between two runs: a song analyzed with a cache folder
    // set is ready at once in the "next run" (the memory emptied), without a
    // helper; a file that is not an envelope of ours is not believed.
    const QString store = temp.filePath("waveforms");
    const QString e = temp.filePath("e");
    touch(e);
    Waveform::setCacheFolder(store);
    Waveform kept;
    request(kept, e);
    assert(waitFor([&] { return QFile::exists(e + ".started"); }));
    touch(e + ".finish");
    assert(waitFor([&] { return kept.ready(); }));
    assert(waitFor([&] { return QDir(store).entryList({ "*.wf" }, QDir::Files).size() == 1; }));
    const QString file = store + '/' + QDir(store).entryList({ "*.wf" }, QDir::Files).first();
    assert(QFileInfo(file).size() == 8 + 512 * 2);

    Waveform::setCacheFolder(store);            // a new run: nothing in memory
    Waveform again;
    again.setDurationSeconds(3);
    again.setSource(QUrl::fromLocalFile(e));
    assert(again.ready() && !again.busy());
    assert(std::abs(again.bins().first().toDouble() - kept.bins().first().toDouble()) < 0.001);
    assert(launches.open(QIODevice::ReadOnly) && launches.readAll() == "a\nb\nd\nb\ne\n");
    launches.close();

    QFile damaged(file);
    assert(damaged.open(QIODevice::WriteOnly) && damaged.write("not an envelope") > 0);
    damaged.close();
    Waveform::setCacheFolder(store);
    Waveform doubted;
    assert(QFile::remove(e + ".started") && QFile::remove(e + ".finish"));
    request(doubted, e);                        // (it asserts busy(): analyzed again)
    assert(waitFor([&] { return QFile::exists(e + ".started"); }));
    touch(e + ".finish");
    assert(waitFor([&] { return doubted.ready(); }));
    Waveform::setCacheFolder({});
    return 0;
}
