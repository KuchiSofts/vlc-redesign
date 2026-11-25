// Copyright (C) 2026 VLC authors and VideoLAN
// SPDX-License-Identifier: GPL-2.0-or-later

#include "modules/gui/qt/playlist/playlist_duration_total.hpp"

#include <cassert>
#include <cstdio>

int main()
{
    vlc::playlist::PlaylistDurationTotal total;
    assert(!total.total());
    total.add(std::nullopt);
    total.add(std::nullopt);
    assert(!total.total());

    // A known zero tick value is distinct from an unknown value.
    total.remove(std::nullopt);
    total.add(0);
    assert(total.total() == 0);
    total.remove(std::nullopt);
    constexpr std::int64_t length = 4'000'000'000;
    total.add(length);
    total.add(std::nullopt);
    assert(total.total() == length);

    // Metadata revises a known length, then makes it unknown again.
    total.remove(length);
    total.add(length + 1);
    assert(total.total() == length + 1);
    total.remove(length + 1);
    total.add(std::nullopt);
    assert(total.total() == 0);

    // Removing the last known length leaves an unknown total.
    total.remove(0);
    assert(!total.total());
    total.remove(std::nullopt);
    total.remove(std::nullopt);
    assert(!total.total());

    // A model reset rebuilds the total from just the replacement rows.
    total.add(length);
    total = {};
    total.add(std::nullopt);
    total.add(length);
    total.add(length);
    assert(total.total() == 2 * length);
    total.remove(length);
    assert(total.total() == length);

    std::puts("playlist duration total: all checks passed");
}
