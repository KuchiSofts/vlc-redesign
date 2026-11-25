/*
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef VLC_INPUT_PLAYBACK_RATE_H
#define VLC_INPUT_PLAYBACK_RATE_H

#include <stdbool.h>

struct input_playback_rate
{
    float effective;
    float base;
};

static inline float
input_playback_rate_target(const struct input_playback_rate *rate, float request,
                           bool temporary)
{
    return temporary && request == 0.f ? rate->base : request;
}

static inline bool
input_playback_rate_update(struct input_playback_rate *rate, float effective,
                           bool persistent)
{
    const float base = persistent ? effective : rate->base;
    const bool changed = effective != rate->effective || base != rate->base;
    rate->effective = effective;
    rate->base = base;
    return changed;
}

#endif
