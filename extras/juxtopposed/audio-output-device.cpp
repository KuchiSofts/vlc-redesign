/****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 ****************************************************************************/

#include "modules/gui/qt/dialogs/preferences/audio_output_device.hpp"

#include <cassert>

int main()
{
    // Two outputs named "Speakers" appear in reverse order in the live list.
    const char* ids[] = { "speakers-2", "", "speakers-1" };
    assert(audioOutputDeviceExists(ids, 3, "speakers-1"));
    assert(audioOutputDeviceExists(ids, 3, "speakers-2"));
    assert(audioOutputDeviceExists(ids, 3, ""));

    // A remaining output with the same name cannot stand in for a removed one.
    const char* remainingIds[] = { "speakers-2" };
    assert(!audioOutputDeviceExists(remainingIds, 1, "speakers-1"));
    assert(!audioOutputDeviceExists(remainingIds, 1, ""));
    assert(!audioOutputDeviceExists(nullptr, 0, "speakers-1"));

    const char* idsWithNullDefault[] = { nullptr, "speakers-2" };
    assert(audioOutputDeviceExists(idsWithNullDefault, 2, ""));
    assert(audioOutputDeviceExists(idsWithNullDefault, 2, "speakers-2"));
    assert(!audioOutputDeviceExists(idsWithNullDefault, 2, "speakers-1"));
}
