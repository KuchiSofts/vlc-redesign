/*****************************************************************************
 * mac_integration.hpp: the interface in macOS (Now Playing, media keys, Dock)
 *****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/
#ifndef MAC_INTEGRATION_HPP
#define MAC_INTEGRATION_HPP

#include <QObject>
#include <QList>
#include <QPair>
#include <QPoint>
#include <QStringList>
#include <memory>

#include "qt.hpp"

class QWindow;

/* What Windows gives the interface through its taskbar and its media keys
 * (mainctx_win32.cpp), macOS gives through Now Playing and the Dock:
 * - the media keys, the Touch Bar, headphones and the Now Playing of Control
 *   Center (MPRemoteCommandCenter) play, pause, stop, go to the previous or
 *   next item, jump and seek;
 * - Now Playing shows the title, artist, album, artwork, length and time;
 * - the Dock menu has Play / Pause, Stop, Previous, Next and Mute, as the
 *   taskbar thumbnail has its buttons.
 * Made with the interface window (CompositorVideo), gone with it. */
class MacIntegration : public QObject
{
public:
    MacIntegration(qt_intf_t *intf, QWindow *window);
    ~MacIntegration() override;

private:
    struct Private;
    std::unique_ptr<Private> d;
};

namespace mac_platform
{
/* "Process priority" of the settings (-2 ... 2, MainCtx::loadPrefs). macOS
 * lets a program lower its priority and give it back (the background state
 * of Darwin) and ask to be kept responsive (an activity of the user: no App
 * Nap, no timer coalescing), never more without the administrator */
void setProcessPriority(int priority);

/* The graphics processors, by name ("Apple M2", "AMD Radeon Pro 5500M"),
 * for the bug report (util/redesign_feedback.cpp) */
QStringList graphicsAdapters();

/* Every screen, by the top left corner of each in the coordinates of Qt,
 * and whether it can show HDR (extended dynamic range) */
QList<QPair<QPoint, bool>> screensHdr();
}

#endif // MAC_INTEGRATION_HPP
