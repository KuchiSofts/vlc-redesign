/* Copyright (C) 2026 VLC authors and VideoLAN. GPL version 2 or later.
 *
 * What a shortcut shows in full screen instead of the controls (issue #72):
 * the kind of each action, the jumps added up, the texts of the step and
 * of the speed.
 *   g++ -std=c++17 -O1 -Wall -Wextra -I. extras/juxtopposed/shortcut-feedback.cpp \
 *       modules/gui/qt/player/shortcut_feedback.cpp -o shortcut-feedback && ./shortcut-feedback
 */
#include "modules/gui/qt/player/shortcut_feedback.hpp"
#include <clocale>
#include <cstdio>
#include <string>

using namespace shortcutfeedback;

static int failures = 0;
#define CHECK(condition) do { if (!(condition)) { \
    std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); ++failures; } } while (0)

static const std::string minus = "\xE2\x88\x92";
static const std::string times = "\xC3\x97";
// U+2066 LEFT-TO-RIGHT ISOLATE ... U+2069 POP DIRECTIONAL ISOLATE
static std::string iso(const std::string &text)
{
    return "\xE2\x81\xA6" + text + "\xE2\x81\xA9";
}

int main()
{
    // the kinds: every action of VLC that a key may have has one
    CHECK(kindOf("") == None);
    for (const char *action : { "intf-show", "intf-popup-menu", "intf-boss", "quit", "toggle-fullscreen",
                                "leave-fullscreen", "nav-activate", "disc-menu" })
        CHECK(kindOf(action) == Controls);
    for (const char *action : { "jump+extrashort", "jump-extrashort", "jump+short", "jump-short", "jump+medium",
                                "jump-medium", "jump+long", "jump-long", "nav-left", "nav-right" })
        CHECK(kindOf(action) == Seek);
    CHECK(kindOf("position") == Position);
    CHECK(kindOf("play-pause") == PlayPause && kindOf("play") == PlayPause && kindOf("pause") == PlayPause);
    CHECK(kindOf("frame-next") == Frame && kindOf("frame-previous") == Frame);
    for (const char *action : { "faster", "slower", "rate-normal", "rate-faster-fine", "rate-slower-fine" })
        CHECK(kindOf(action) == Rate);
    for (const char *action : { "vol-up", "vol-down", "vol-mute", "nav-up", "nav-down" })
        CHECK(kindOf(action) == Volume);
    for (const char *action : { "subtitle-track", "audio-track", "subdelay-up", "audiodelay-down", "aspect-ratio",
                                "zoom", "crop-top", "deinterlace", "subpos-up", "subtitle-text-scale-up", "snapshot",
                                "chapter-next", "next", "stop", "loop", "random", "record", "audiodevice-cycle" })
        CHECK(kindOf(action) == Message);

    // the jumps, with the sizes of the settings (VLC's defaults: 3, 10, 60, 300)
    const int64_t sizes[4] = { 3, 10, 60, 300 };
    CHECK(jumpSeconds("jump+extrashort", sizes) == 3);
    CHECK(jumpSeconds("jump-extrashort", sizes) == -3);
    CHECK(jumpSeconds("jump+short", sizes) == 10);
    CHECK(jumpSeconds("jump-medium", sizes) == -60);
    CHECK(jumpSeconds("jump+long", sizes) == 300);
    CHECK(jumpSeconds("nav-right", sizes) == 10);
    CHECK(jumpSeconds("nav-left", sizes) == -10);
    CHECK(jumpSeconds("vol-up", sizes) == 0);
    CHECK(jumpSeconds("jump+nothing", sizes) == 0);
    const int64_t off[4] = { 0, -1, 60, 300 };
    CHECK(jumpSeconds("jump+extrashort", off) == 0);
    CHECK(jumpSeconds("nav-right", off) == 0);

    // keys in a row add up; a pause, the other way or another action start again
    {
        SeekSum sum;
        CHECK(sum.add(10, 1000) == 10);
        CHECK(sum.add(10, 1400) == 20);
        CHECK(sum.add(10, 2900) == 30);   // 1.5 s after the last one: still in a row
        CHECK(sum.add(10, 4401) == 10);   // 1.501 s: a new sum
        CHECK(sum.add(-10, 4500) == -10); // the other way: from this jump on
        CHECK(sum.add(-60, 4600) == -70);
        CHECK(sum.add(10, 4700) == 10);
        sum.reset();
        CHECK(sum.add(10, 4800) == 10);   // another action came between
        CHECK(sum.add(3, 100) == 3);      // a clock that went back: not in a row
    }

    // the step: seconds, from a minute on m:ss, from an hour on h:mm:ss; the
    // sign and the number in a left-to-right isolate
    CHECK(stepText(10, "%1 s") == iso("+10") + " s");
    CHECK(stepText(-10, "%1 s") == iso(minus + "10") + " s");
    CHECK(stepText(30, "%1 s") == iso("+30") + " s");
    CHECK(stepText(59, "%1 s") == iso("+59") + " s");
    CHECK(stepText(60, "%1 s") == iso("+1:00"));
    CHECK(stepText(70, "%1 s") == iso("+1:10"));
    CHECK(stepText(-70, "%1 s") == iso(minus + "1:10"));
    CHECK(stepText(3723, "%1 s") == iso("+1:02:03"));
    CHECK(stepText(-3600, "%1 s") == iso(minus + "1:00:00"));
    CHECK(stepText(0, "%1 s").empty());
    // a translation's own order and unit (Hebrew: the unit after the number,
    // read from the right; Chinese before it)
    CHECK(stepText(10, "%1\xC2\xA0" "s") == iso("+10") + "\xC2\xA0" "s");
    CHECK(stepText(10, "%1 \xD7\xA9\xD7\xA0\xD7\xB3") == iso("+10") + " \xD7\xA9\xD7\xA0\xD7\xB3");
    CHECK(stepText(-5, "\xE7\xA7\x92%1") == "\xE7\xA7\x92" + iso(minus + "5"));
    // a translation that lost its %1: the number all the same
    CHECK(stepText(10, "s") == iso("+10") + " s");

    // the speed: two decimals at most, a point in any locale
    CHECK(rateText(1.5) == iso("1.5" + times));
    CHECK(rateText(1.25) == iso("1.25" + times));
    CHECK(rateText(2.0) == iso("2" + times));
    CHECK(rateText(0.25) == iso("0.25" + times));
    CHECK(rateText(1.0 / 3) == iso("0.33" + times));
    CHECK(rateText(1.05) == iso("1.05" + times));
    CHECK(rateText(4.0) == iso("4" + times));
    CHECK(rateText(0).empty());
    if (std::setlocale(LC_NUMERIC, "de_DE.UTF-8") || std::setlocale(LC_NUMERIC, "fr_FR.UTF-8"))
        CHECK(rateText(1.5) == iso("1.5" + times));

    if (failures)
    {
        std::fprintf(stderr, "%d failed\n", failures);
        return 1;
    }
    std::printf("shortcut feedback: all passed\n");
    return 0;
}
