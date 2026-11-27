/*
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef VLC_QT_PLAYER_TIME_DISPLAY_HPP
#define VLC_QT_PLAYER_TIME_DISPLAY_HPP

#include <vlc_tick.h>

// Match VLCTick::formatHMS(): truncate toward zero, with a millisecond
// display below one second when requested. Keep an invalid time distinct
// from a valid zero.
inline vlc_tick_t timeDisplayTick(vlc_tick_t time, bool showSubSecond)
{
    if (time == VLC_TICK_INVALID)
        return time;

    const vlc_tick_t elapsed = time - VLC_TICK_0;
    if (showSubSecond)
    {
        const auto milliseconds = MS_FROM_VLC_TICK(elapsed);
        if (milliseconds < 1000)
            return VLC_TICK_0 + VLC_TICK_FROM_MS(milliseconds);
    }
    return VLC_TICK_0 + VLC_TICK_FROM_SEC(SEC_FROM_VLC_TICK(elapsed));
}

#endif
