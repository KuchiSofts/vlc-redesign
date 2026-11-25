// Copyright (C) 2026 VLC authors and VideoLAN
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef VLC_QT_PLAYLIST_DURATION_TOTAL_HPP_
#define VLC_QT_PLAYLIST_DURATION_TOTAL_HPP_

#include <cstddef>
#include <cstdint>
#include <optional>

namespace vlc {
namespace playlist {

class PlaylistDurationTotal
{
public:
    void add(std::optional<std::int64_t> duration)
    {
        if (duration)
        {
            m_ticks += *duration;
            ++m_known;
        }
    }

    void remove(std::optional<std::int64_t> duration)
    {
        if (duration)
        {
            m_ticks -= *duration;
            --m_known;
        }
    }

    std::optional<std::int64_t> total() const
    {
        // Zero is a valid length if any item has a known duration.
        if (m_known)
            return m_ticks;
        return std::nullopt;
    }

private:
    std::int64_t m_ticks = 0;
    std::size_t m_known = 0;
};

} // namespace playlist
} // namespace vlc

#endif
