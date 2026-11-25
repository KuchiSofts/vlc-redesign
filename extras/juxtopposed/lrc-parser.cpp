/* Copyright (C) 2026 VLC authors and VideoLAN. GPL version 2 or later.
 *
 * The lyrics of an .lrc file beside a song (modules/gui/qt/player/lrc_parser.cpp):
 * the time tags (several on a line, 1 to 3 digits after the seconds), the
 * offset, the tags and the enhanced word times left out, the order, the
 * text's encodings, the form the player reads, the file beside the song.
 *   g++ -std=c++17 -O1 -Wall -Wextra -fPIC -I. $(pkg-config --cflags Qt6Core) \
 *       extras/juxtopposed/lrc-parser.cpp modules/gui/qt/player/lrc_parser.cpp \
 *       $(pkg-config --libs Qt6Core) -o lrc-parser && ./lrc-parser
 */
#include "modules/gui/qt/player/lrc_parser.hpp"
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <cstdio>

using namespace Lrc;

static int checks = 0;
static int failures = 0;
#define CHECK(condition) do { ++checks; if (!(condition)) { \
    std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); ++failures; } } while (0)

static QString S(const char *utf8)
{
    return QString::fromUtf8(utf8);
}

static void write(const QString &path, const QByteArray &data)
{
    QFile f(path);
    if (f.open(QIODevice::WriteOnly))
        f.write(data);
}

int main()
{
    // ---- the lines ---------------------------------------------------
    {
        const auto l = parse(S("[ti:Song]\n[ar:Someone]\n[00:01.50]First\n[00:03.00][00:09.25] Chorus \n[00:05.123]Third"));
        CHECK(l.size() == 4);
        CHECK(l.size() == 4 && l[0].ms == 1500 && l[0].text == S("First"));
        CHECK(l.size() == 4 && l[1].ms == 3000 && l[1].text == S("Chorus"));
        CHECK(l.size() == 4 && l[2].ms == 5123 && l[2].text == S("Third"));
        CHECK(l.size() == 4 && l[3].ms == 9250 && l[3].text == S("Chorus"));
    }
    // one or no digit after the seconds, ':' for the '.', minutes past 99
    {
        const auto l = parse(S("[01:02.5]a\n[01:03]b\n[01:04:20]c\n[100:00.00]d"));
        CHECK(l.size() == 4 && l[0].ms == 62500 && l[1].ms == 63000 && l[2].ms == 64200 && l[3].ms == 6000000);
    }
    // the offset: a positive one shows the lines sooner, never before 0
    {
        const auto l = parse(S("[offset:+500]\n[00:01.50]A\n[00:00.20]B"));
        CHECK(l.size() == 2 && l[0].ms == 0 && l[1].ms == 1000);
        const auto m = parse(S("[OFFSET: -250]\n[00:01.00]A"));
        CHECK(m.size() == 1 && m[0].ms == 1250);
    }
    // an empty line (a pause) stays; the word times of enhanced .lrc go
    {
        const auto l = parse(S("[00:10.00]\n[00:12.00]<00:12.00>Hel<00:12.40>lo <00:13.00>world"));
        CHECK(l.size() == 2 && l[0].text.isEmpty() && l[1].text == S("Hello world"));
    }
    // Windows and old Mac line ends, spaces before the tag, Hebrew text
    {
        const auto l = parse(S("  [00:01.00]\xd7\xa9\xd7\x9c\xd7\x95\xd7\x9d\r\n[00:02.00]two\r[00:03.00]three"));
        CHECK(l.size() == 3 && l[0].text == S("\xd7\xa9\xd7\x9c\xd7\x95\xd7\x9d") && l[2].ms == 3000);
    }
    // what is not an .lrc: no lines
    CHECK(parse(S("Just words\nno times at all")).isEmpty());
    CHECK(parse(S("[ar:Someone]\n[00:61.00]bad seconds\n[0a:01.00]bad\n[00:01.00")).isEmpty());
    CHECK(parse(QString()).isEmpty());

    // ---- the encodings -----------------------------------------------
    CHECK(decode(QByteArray("\xef\xbb\xbf[00:01.00]caf\xc3\xa9")) == S("[00:01.00]caf\xc3\xa9"));
    CHECK(decode(QByteArray("[00:01.00]caf\xc3\xa9")) == S("[00:01.00]caf\xc3\xa9"));
    {
        QByteArray utf16("\xff\xfe", 2);
        for (const char c : QByteArray("[00:01.00]x"))
            utf16 += c, utf16 += '\0';
        CHECK(decode(utf16) == S("[00:01.00]x"));
    }
    // not UTF-8: the system's code page (still the times)
    CHECK(parse(decode(QByteArray("[00:01.00]caf\xe9"))).size() == 1);

    // ---- the form the player reads -----------------------------------
    {
        const QByteArray sylt = toSylt({ { 1500, S("First") }, { 3000, S("a\x1e" "b") } });
        CHECK(sylt == QByteArray("1500\x1f" "First\x1e" "3000\x1f" "ab\x1e"));
        CHECK(toSylt({}).isEmpty());
    }

    // ---- the file beside the song ------------------------------------
    {
        QTemporaryDir dir;
        CHECK(dir.isValid());
        const QString song = dir.filePath(QStringLiteral("My Song [Live].mp3"));
        write(song, "x");
        CHECK(besideFile(song).isEmpty());
        CHECK(loadBesideFile(song).isEmpty());
        const QString sidecar = dir.filePath(QStringLiteral("My Song [Live].lrc"));
        write(sidecar, "[00:01.00]a");
        CHECK(besideFile(song) == sidecar);
        CHECK(loadBesideFile(song) == QByteArray("1000\x1f" "a\x1e"));
        // another song's .lrc, or a folder named like one, is not taken
        const QString other = dir.filePath(QStringLiteral("Other.flac"));
        write(other, "x");
        QDir(dir.path()).mkdir(QStringLiteral("Other.lrc"));
        CHECK(besideFile(other).isEmpty());
        const QString upper = dir.filePath(QStringLiteral("Upper.ogg"));
        write(upper, "x");
        write(dir.filePath(QStringLiteral("Upper.LRC")), "[00:01.00]a");
        CHECK(!besideFile(upper).isEmpty());
        CHECK(loadBesideFile(upper) == QByteArray("1000\x1f" "a\x1e"));

        write(sidecar, "No timed lyrics");
        CHECK(loadBesideFile(song).isEmpty());

        QByteArray data("[00:01.00]a\n");
        data.append(QByteArray(maxFileSize - data.size(), ' '));
        write(sidecar, data);
        CHECK(loadBesideFile(song) == QByteArray("1000\x1f" "a\x1e"));
        data.append(' ');
        write(sidecar, data);
        CHECK(loadBesideFile(song).isEmpty());
    }

    if (failures)
        return std::fprintf(stderr, "lrc parser: %d of %d checks failed\n", failures, checks), 1;
    std::printf("lrc parser: %d checks, 0 failed\n", checks);
    return 0;
}
