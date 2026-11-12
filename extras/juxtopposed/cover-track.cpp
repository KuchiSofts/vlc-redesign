/* Copyright (C) 2026 VLC authors and VideoLAN. LGPL version 2.1 or later.
 *
 * A song's picture kept as a video track (modules/misc/medialibrary/cover_track.h):
 * a JPEG or PNG track in a file named as audio, beside its audio track, is the
 * song's cover and the file stays a song; a camera's Motion JPEG clip, a film
 * in an .ogg, a picture alone stay what they are.
 *   g++ -std=c++17 -O1 -Wall -Wextra -I. extras/juxtopposed/cover-track.cpp -o cover-track && ./cover-track
 */
#include "modules/misc/medialibrary/cover_track.h"

#include <cstdio>

using namespace redesign_cover;

static int checks = 0;
static int failures = 0;
#define CHECK(condition) do { ++checks; if (!(condition)) { \
    std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); ++failures; } } while (0)

int main()
{
    // the extension, from a path or an address
    CHECK(extensionOf("file:///C:/Music/Song.M4A") == "m4a");
    CHECK(extensionOf("C:\\Music\\A.song.mp3") == "mp3");
    CHECK(extensionOf("https://example.com/a/b.flac?x=1#t") == "flac");
    CHECK(extensionOf("file:///music/no-extension").empty());
    CHECK(extensionOf("file:///music/.hidden").empty());
    CHECK(extensionOf("file:///music/ends-with-dot.").empty());
    CHECK(extensionOf(nullptr).empty());

    // what the issue had: .m4a songs with their thumbnail as a JPEG or PNG track
    CHECK(isCoverTrack("file:///C:/Users/USER/Videos/Song.m4a", "MJPG", true));
    CHECK(isCoverTrack("file:///C:/Users/USER/Videos/Song.m4a", "jpeg", true));
    CHECK(isCoverTrack("file:///C:/Users/USER/Videos/Song.m4a", "png ", true));
    CHECK(isCoverTrack("file:///music/a.mp3", "png ", true));
    CHECK(isCoverTrack("file:///music/a.flac", "jpeg", true));
    CHECK(isCoverTrack("file:///music/a.mka", "MJPG", true));
    CHECK(isCoverTrack("file:///music/a.ogg", "jpeg", true));

    // videos stay videos
    CHECK(!isCoverTrack("file:///cam/clip.avi", "MJPG", true));
    CHECK(!isCoverTrack("file:///cam/clip.mov", "MJPG", true));
    CHECK(!isCoverTrack("file:///film/a.mp4", "h264", true));
    CHECK(!isCoverTrack("file:///film/a.m4v", "jpeg", true));
    CHECK(!isCoverTrack("file:///film/a.ogg", "theo", true));
    CHECK(!isCoverTrack("file:///film/a.m4a", "h264", true));
    // no audio track: not a song
    CHECK(!isCoverTrack("file:///music/a.m4a", "jpeg", false));
    // no name to tell by
    CHECK(!isCoverTrack(nullptr, "jpeg", true));
    CHECK(!isCoverTrack("file:///music/a.m4a", nullptr, true));

    std::printf("cover tracks: %d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
