/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
#pragma once

#include <cstdint>
#include <utility>

// Replies arrive on the interface thread, but a slow server can answer after
// a later selection. Only that latest selection may act on its reply.
class IptvTuneRequests
{
public:
    void advance() { ++m_generation; }
    bool hasSelection() const { return m_generation != 0; }

    template <typename Callback>
    auto forNextTune(Callback done)
    {
        advance();
        return [this, generation = m_generation, done = std::move(done)](const auto& answer) {
            if (generation == m_generation)
                done(answer);
        };
    }

private:
    std::uint64_t m_generation = 0;
};
