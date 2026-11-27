/*
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "modules/gui/qt/player/time_display.hpp"

#include <cassert>

int main()
{
    const auto atMs = [](int ms) { return VLC_TICK_0 + VLC_TICK_FROM_MS(ms); };

    assert(timeDisplayTick(VLC_TICK_INVALID, false) == VLC_TICK_INVALID);
    assert(timeDisplayTick(VLC_TICK_INVALID, true) == VLC_TICK_INVALID);
    assert(timeDisplayTick(atMs(0), true) == VLC_TICK_0);

    // Elapsed time for regular media stays in seconds, including its first second.
    assert(timeDisplayTick(atMs(999), false) == atMs(0));
    assert(timeDisplayTick(atMs(1000), false) == atMs(1000));
    assert(timeDisplayTick(atMs(1999), false) == atMs(1000));
    assert(timeDisplayTick(atMs(2000), false) == atMs(2000));

    // Short elapsed times and the last second remaining retain milliseconds.
    assert(timeDisplayTick(atMs(42) + 999, true) == atMs(42));
    assert(timeDisplayTick(atMs(43), true) == atMs(43));
    assert(timeDisplayTick(atMs(999) + 999, true) == atMs(999));
    assert(timeDisplayTick(atMs(1000), true) == atMs(1000));
    assert(timeDisplayTick(atMs(1999), true) == atMs(1000));
    assert(timeDisplayTick(atMs(-1) - 999, true) == atMs(-1));
}
