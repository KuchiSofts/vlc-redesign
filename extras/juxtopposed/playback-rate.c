/*
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <assert.h>

#include "src/input/playback_rate.h"

int main(void)
{
    struct input_playback_rate first = { .effective = 1.f, .base = 1.f };
    const struct input_playback_rate next = first;

    /* Stop or switch inputs with the hold still applied. */
    assert(input_playback_rate_update(&first, 2.f, false));
    assert(first.effective == 2.f && first.base == 1.f);
    assert(next.effective == 1.f && next.base == 1.f);

    /* Release uses a base that was applied in input queue order. */
    assert(input_playback_rate_update(&first, 1.5f, true));
    assert(input_playback_rate_target(&first, 2.f, true) == 2.f);
    assert(input_playback_rate_update(&first, 2.f, false));
    const float release = input_playback_rate_target(&first, 0.f, true);
    assert(release == 1.5f && first.base == 1.5f);
    assert(input_playback_rate_update(&first, release, false));
    assert(first.effective == 1.5f && first.base == 1.5f);

    /* A rejected release must still leave the right rate for the save. */
    assert(input_playback_rate_update(&first, 2.f, false));
    assert(!input_playback_rate_update(&first, 2.f, false));
    assert(first.base == 1.5f);

    /* Explicit 2x during a hold is a base change even at the same speed. */
    assert(input_playback_rate_update(&first, 2.f, true));
    assert(first.effective == 2.f && first.base == 2.f);
    assert(input_playback_rate_target(&first, 0.f, true) == 2.f);
    assert(input_playback_rate_target(&first, 1.25f, false) == 1.25f);
    assert(input_playback_rate_update(&first, 1.25f, true));
    assert(first.effective == 1.25f && first.base == 1.25f);
    return 0;
}
