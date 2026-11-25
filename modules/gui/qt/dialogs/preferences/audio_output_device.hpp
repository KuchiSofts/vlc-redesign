/****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 ****************************************************************************/
#ifndef VLC_QT_PREFERENCES_AUDIO_OUTPUT_DEVICE_HPP
#define VLC_QT_PREFERENCES_AUDIO_OUTPUT_DEVICE_HPP

#include <string_view>

inline bool audioOutputDeviceExists(const char* const* ids, int count, std::string_view selectedId)
{
    for (int i = 0; i < count; ++i)
        if (selectedId == (ids[i] ? ids[i] : ""))
            return true;
    return false;
}

#endif
