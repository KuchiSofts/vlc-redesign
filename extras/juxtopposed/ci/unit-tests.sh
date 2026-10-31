#!/bin/sh
# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
#
# The tests of the redesign's parsers that read what comes from outside,
# built with the address and undefined-behaviour sanitizers: Live TV's
# playlists, guides and Xtream answers (fixtures/), the subtitle cues of the
# seek bar, and the encoding detection of text subtitles (the corpus).
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

echo "== Live TV parsers"
# (Qt's own allocations are not ours: only the parsers' leaks count)
g++ -std=c++17 -O1 -Wall -Wextra $san -fPIC -DHAVE_ZLIB -I. $(pkg-config --cflags Qt6Core) \
    extras/juxtopposed/iptv-parsers.cpp modules/misc/iptv/playlist.cpp \
    modules/misc/iptv/guide.cpp modules/misc/iptv/xtream.cpp \
    $(pkg-config --libs Qt6Core) -lz -o "$out/iptv-parsers"
"$out/iptv-parsers" extras/juxtopposed/fixtures

echo "== subtitle encodings"
python3 extras/juxtopposed/subtitle-charset-test.py --cc "gcc $san"
echo "all passed"
