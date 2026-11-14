#!/bin/sh
# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
#
# The tests of the redesign's parsers that read what comes from outside,
# built with the address and undefined-behaviour sanitizers: Live TV's
# playlists, guides and Xtream answers (fixtures/), the subtitle cues of the
# seek bar, the subtitle services asked with the user's key or account (their
# addresses and answers, fixtures/subtitle-services), and the encoding
# detection of text subtitles (the corpus); what a shortcut shows in
# full screen (the kinds of the actions, the jumps added up, their texts);
# and the headlines "What's new" makes of a changelog.
# Run by .github/workflows/tests.yml; on Linux, from the root of the tree,
# with g++, Qt 6 Core, zlib and Python 3:
#   sh extras/juxtopposed/ci/unit-tests.sh
set -eu
out=${OUT:-build-tests}
mkdir -p "$out"
san="-fsanitize=address,undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer -g"
export ASAN_OPTIONS=detect_leaks=1:abort_on_error=1
export UBSAN_OPTIONS=print_stacktrace=1

echo "== subtitle cues"
g++ -std=c++17 -O1 -Wall -Wextra $san -I. extras/juxtopposed/subtitle-cues-parser.cpp \
    modules/gui/qt/player/subtitle_cues_parser.cpp -o "$out/subtitle-cues-parser"
"$out/subtitle-cues-parser"

echo "== shortcut feedback in full screen (the kinds, the jumps added up, the texts)"
g++ -std=c++17 -O1 -Wall -Wextra $san -I. extras/juxtopposed/shortcut-feedback.cpp \
    modules/gui/qt/player/shortcut_feedback.cpp -o "$out/shortcut-feedback"
"$out/shortcut-feedback"

echo "== windows that fit their screen (the first size and scale, a saved geometry, negative coordinates)"
g++ -std=c++17 -O1 -Wall -Wextra $san -fPIC -I. $(pkg-config --cflags Qt6Core) \
    extras/juxtopposed/screen-fit.cpp modules/gui/qt/util/screen_fit.cpp \
    $(pkg-config --libs Qt6Core) -o "$out/screen-fit"
"$out/screen-fit"

echo "== What's new (the headlines of a changelog: no issue references, a long lead's colon, whole words)"
g++ -std=c++17 -O1 -Wall -Wextra $san -fPIC -I. $(pkg-config --cflags Qt6Core) \
    extras/juxtopposed/whats-new.cpp modules/gui/qt/dialogs/firstrun/whats_new_text.cpp \
    $(pkg-config --libs Qt6Core) -o "$out/whats-new"
"$out/whats-new"

echo "== files opened from Finder (macOS's open events reach the playlist, the ones of a launch too)"
g++ -std=c++17 -O1 -Wall -Wextra $san -fPIC -I. $(pkg-config --cflags Qt6Gui) \
    extras/juxtopposed/file-open-spy.cpp $(pkg-config --libs Qt6Gui) -o "$out/file-open-spy"
"$out/file-open-spy"

echo "== the media library's task runner (VideoLAN's test: tasks, queues, cancels, objects destroyed while they wait)"
moc=$(pkg-config --variable=libexecdir Qt6Core 2>/dev/null)/moc
[ -x "$moc" ] || moc=/usr/lib/qt6/libexec/moc
mkdir -p "$out/thread-runner"
"$moc" modules/gui/qt/medialibrary/mlthreadpool.hpp -o "$out/thread-runner/moc_mlthreadpool.cpp"
"$moc" modules/gui/qt/tests/test_thread_runner.cpp -o "$out/thread-runner/test_thread_runner.moc"
g++ -std=c++17 -O1 -Wall $san -fPIC -I. -I"$out/thread-runner" -Imodules/gui/qt/medialibrary \
    $(pkg-config --cflags Qt6Core Qt6Test) \
    modules/gui/qt/tests/test_thread_runner.cpp modules/gui/qt/medialibrary/mlthreadpool.cpp \
    "$out/thread-runner/moc_mlthreadpool.cpp" \
    $(pkg-config --libs Qt6Core Qt6Test) -o "$out/thread-runner/test"
"$out/thread-runner/test" -silent

echo "== Cover tracks (a song's picture kept as a video track is its cover, not a video)"
g++ -std=c++17 -O1 -Wall -Wextra $san -I. extras/juxtopposed/cover-track.cpp -o "$out/cover-track"
"$out/cover-track"

echo "== Live TV parsers"
# (Qt's own allocations are not ours: only the parsers' leaks count; a
# number out of range made an int or a qint64, tvg-shift="1e9", aborts too)
fcast="-fsanitize=float-cast-overflow"
g++ -std=c++17 -O1 -Wall -Wextra $san $fcast -fPIC -DHAVE_ZLIB -I. $(pkg-config --cflags Qt6Core) \
    extras/juxtopposed/iptv-parsers.cpp modules/misc/iptv/playlist.cpp \
    modules/misc/iptv/guide.cpp modules/misc/iptv/xtream.cpp \
    $(pkg-config --libs Qt6Core) -lz -o "$out/iptv-parsers"
"$out/iptv-parsers" extras/juxtopposed/fixtures

echo "== Live TV: the user's servers and tuners"
g++ -std=c++17 -O1 -Wall -Wextra $san $fcast -fPIC -DHAVE_ZLIB -I. $(pkg-config --cflags Qt6Core) \
    extras/juxtopposed/iptv-servers.cpp modules/misc/iptv/servers.cpp \
    modules/misc/iptv/playlist.cpp modules/misc/iptv/guide.cpp \
    $(pkg-config --libs Qt6Core) -lz -o "$out/iptv-servers"
"$out/iptv-servers" extras/juxtopposed/fixtures/iptv-servers

echo "== Live TV: the automatic guide"
g++ -std=c++17 -O1 -Wall -Wextra $san -fPIC -I. $(pkg-config --cflags Qt6Core) \
    extras/juxtopposed/iptv-autoguide.cpp modules/misc/iptv/autoguide.cpp \
    $(pkg-config --libs Qt6Core) -o "$out/iptv-autoguide"
"$out/iptv-autoguide" extras/juxtopposed/fixtures/iptv-autoguide

echo "== Live TV: quitting never waits without bound"
g++ -std=c++17 -O1 -Wall -Wextra $san -fPIC -DHAVE_ZLIB -I. $(pkg-config --cflags Qt6Core) \
    extras/juxtopposed/iptv-stop.cpp modules/misc/iptv/service.cpp modules/misc/iptv/servers.cpp \
    modules/misc/iptv/autoguide.cpp modules/misc/iptv/playlist.cpp modules/misc/iptv/guide.cpp \
    modules/misc/iptv/xtream.cpp $(pkg-config --libs Qt6Core) -lz -o "$out/iptv-stop"
# (the service a stuck download keeps is let go on purpose: not a leak of the code)
ASAN_OPTIONS=detect_leaks=0:abort_on_error=1 "$out/iptv-stop"

echo "== Live TV: downloads that stall (a local server that sends a few bytes, then nothing)"
g++ -std=c++17 -O1 -Wall -Wextra $san -pthread -I. extras/juxtopposed/iptv-idle.cpp -o "$out/iptv-idle"
"$out/iptv-idle"

echo "== Live TV: the service (limits, failures, back-off, answers cut short, removals, the keystore)"
g++ -std=c++17 -O1 -Wall -Wextra $san $fcast -fPIC -DHAVE_ZLIB -I. $(pkg-config --cflags Qt6Core) \
    extras/juxtopposed/iptv-service.cpp modules/misc/iptv/service.cpp modules/misc/iptv/servers.cpp \
    modules/misc/iptv/autoguide.cpp modules/misc/iptv/playlist.cpp modules/misc/iptv/guide.cpp \
    modules/misc/iptv/xtream.cpp $(pkg-config --libs Qt6Core) -lz -o "$out/iptv-service"
"$out/iptv-service"

echo "== subtitle services (keys and accounts, YIFY's page, the archives)"
g++ -std=c++17 -O1 -Wall -Wextra $san -fPIC -DHAVE_ZLIB -I. -Imodules/gui/qt $(pkg-config --cflags Qt6Core) \
    extras/juxtopposed/subtitle-services.cpp modules/gui/qt/player/subtitle_services.cpp \
    $(pkg-config --libs Qt6Core) -lz -o "$out/subtitle-services"
"$out/subtitle-services" extras/juxtopposed/fixtures/subtitle-services

echo "== the interface's HTTP client (redirects, deadlines, cancels) and its work off the thread"
# with libcurl (its development files), through local servers; without,
# the rules of its redirects only
if pkg-config --exists libcurl; then
    http="-DQT_HAS_LIBCURL $(pkg-config --cflags libcurl)"
    httplibs=$(pkg-config --libs libcurl)
else
    http=
    httplibs=
fi
g++ -std=c++17 -O1 -Wall -Wextra $san -fPIC $http -I. -Imodules/gui/qt $(pkg-config --cflags Qt6Core) \
    extras/juxtopposed/http-request-test.cpp modules/gui/qt/util/http_request.cpp \
    $(pkg-config --libs Qt6Core) $httplibs -o "$out/http-request-test"
"$out/http-request-test" python3 extras/juxtopposed/http-test-server.py
# quitting: a stuck worker is let go on purpose (not a leak of the code)
ASAN_OPTIONS=detect_leaks=0:abort_on_error=1 "$out/http-request-test" python3 extras/juxtopposed/http-test-server.py --shutdown

echo "== the updater's files: on the disk before they take their name (QSaveFile)"
# (no sanitizer: the probe of the system's calls is preloaded first)
gcc -shared -fPIC -O1 -Wall extras/juxtopposed/sync-probe.c -o "$out/sync-probe.so" -ldl
g++ -std=c++17 -O1 -Wall -Wextra -fPIC $(pkg-config --cflags Qt6Core) extras/juxtopposed/savefile-sync-test.cpp \
    $(pkg-config --libs Qt6Core) -ldl -o "$out/savefile-sync-test"
LD_PRELOAD="$(cd "$out" && pwd)/sync-probe.so" "$out/savefile-sync-test"

echo "== the parsers against malformed and huge input (fuzz)"
g++ -std=c++17 -O1 -g $san $fcast -fPIC -DHAVE_ZLIB -I. -Imodules/gui/qt $(pkg-config --cflags Qt6Core)     extras/juxtopposed/parsers-fuzz.cpp modules/misc/iptv/playlist.cpp modules/misc/iptv/guide.cpp     modules/misc/iptv/xtream.cpp modules/misc/iptv/servers.cpp modules/misc/iptv/autoguide.cpp     modules/gui/qt/player/subtitle_services.cpp modules/gui/qt/player/subtitle_cues_parser.cpp     $(pkg-config --libs Qt6Core) -lz -o "$out/parsers-fuzz"
"$out/parsers-fuzz" extras/juxtopposed/fixtures

echo "== subtitle encodings"
python3 extras/juxtopposed/subtitle-charset-test.py --cc "gcc $san"

echo "== outside text is never drawn as HTML (QML textFormat)"
python3 extras/juxtopposed/qml-plaintext-check.py

echo "== every text field and settings toggle has an accessible name"
python3 extras/juxtopposed/qml-a11y-name-check.py
echo "all passed"
