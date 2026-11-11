/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
/*****************************************************************************
 * shortcut_feedback.hpp: What a shortcut shows in full screen instead of the
 * controls ("When you use a shortcut in full screen", issue #72)
 *****************************************************************************/
#ifndef VLC_QT_SHORTCUT_FEEDBACK_HPP_
#define VLC_QT_SHORTCUT_FEEDBACK_HPP_

// Plain C++ (no Qt, no VLC): built alone by its test too
// (extras/juxtopposed/shortcut-feedback.cpp)
#include <cstdint>
#include <string>

namespace shortcutfeedback {

// How long the feedback of a key stays: keys pressed closer than this are
// "in a row" (their jumps add up), and the core's own message is not drawn
// on the video meanwhile (src/player/osd.c)
constexpr int64_t feedbackMs = 1500;

// What a key bound to an action of VLC shows in full screen while the
// controls stay hidden (MainCtx::ShortcutKind has the same values)
enum Kind : int
{
    None = 0,       // bound to no action: the controls show, as before
    Controls = 1,   // the controls are what it shows or needs (the I key, the
                    // menu key, Enter, Esc, the full screen): as before
    Seek = 2,       // a jump: its step ("+10 s") and the time
    Position = 3,   // the time
    PlayPause = 4,  // Paused or Playing, and the time
    Frame = 5,      // the next or previous frame, and the time
    Rate = 6,       // the speed ("1.5x")
    Volume = 7,     // the window's volume meter shows it (VolumeMeter.qml)
    Message = 8,    // what the core says it did ("Subtitle track: English"),
                    // else the name of the action
};

// The kind of an action of VLC by its name, as vlc_actions_get_key_names()
// gives them ("jump+short", "vol-up"); "" (no action) is None
Kind kindOf(const std::string &action);

// The seconds an action jumps, negative backwards, from the sizes of the
// settings in seconds (extrashort, short, medium, long); the arrows
// (nav-left, nav-right) jump as VLC's player does when the media has no menu:
// the short jump. 0 for the other actions and a size below 0.
int64_t jumpSeconds(const std::string &action, const int64_t sizes[4]);

// The jumps of keys pressed in a row, added up: started again after a pause
// of feedbackMs, with a jump the other way, or after another action (reset)
class SeekSum
{
public:
    // The sum with this jump (seconds) made at nowMs (a monotonic clock)
    int64_t add(int64_t seconds, int64_t nowMs);
    // The same, and how far the run really moves (seconds, rounded): the
    // sum kept within the media, from fromMs (the position before this
    // jump; the run's own start is kept from its first jump) to 0 and to
    // lengthMs (0: unknown, no end). At 0:05, three jumps of -10 s move -5 s
    int64_t add(int64_t seconds, int64_t nowMs, int64_t fromMs, int64_t lengthMs);
    void reset();
    int64_t sum() const { return m_sum; }

private:
    int64_t m_sum = 0;
    int64_t m_lastMs = 0;
    int64_t m_fromMs = 0;
    bool m_started = false;
};

// The step as the feedback shows it: "+10 s", "−10 s" (a minus sign),
// from a minute on "+1:10", from an hour on "+1:02:03"; secondsFormat is the
// translated "%1 s" (%1 the number). "" for 0. The sign and the number are
// in a left-to-right isolate (U+2066 ... U+2069): together in any language.
std::string stepText(int64_t seconds, const std::string &secondsFormat);

// The speed: "1.5×", "1.25×", "2×" (two decimals at most, a
// point in any locale), in a left-to-right isolate too
std::string rateText(double rate);

} // namespace shortcutfeedback

#endif
