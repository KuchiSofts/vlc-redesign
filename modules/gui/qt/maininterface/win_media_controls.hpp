/*****************************************************************************
 * win_media_controls.hpp: Windows System Media Transport Controls
 *****************************************************************************
 * SPDX-License-Identifier: GPL-2.0-or-later
 *****************************************************************************/

#ifndef VLC_WIN_MEDIA_CONTROLS_HPP
#define VLC_WIN_MEDIA_CONTROLS_HPP

#include "qt.hpp"

#include <QObject>
#include <windef.h>

#include <memory>

class WinMediaControls final : public QObject
{
public:
    WinMediaControls(qt_intf_t *intf, HWND hwnd);
    ~WinMediaControls() override;

    bool isActive() const;

private:
    struct Private;
    std::unique_ptr<Private> d;
};

#endif
