/*****************************************************************************
 * macos_titlebar.hpp: the unified title bar of the main window on macOS
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
#ifndef VLC_QT_MACOS_TITLEBAR_HPP
#define VLC_QT_MACOS_TITLEBAR_HPP

#include <QMargins>

class QWindow;

namespace macos_titlebar
{

/*
 * The redesign's own title bar on macOS, the way macOS apps do it: the
 * window keeps its system frame (the traffic lights, full screen in its own
 * Space, tiling, resizing from every edge, the shadow and the border), its
 * title bar is made transparent and untitled, and the content fills the
 * whole window under it (NSWindowStyleMaskFullSizeContentView). The
 * interface's top bar is then the only bar, with the traffic lights in it.
 *
 * Off: the system's title bar again, above the content.
 *
 * The content keeps its place on the screen either way (the window frame
 * moves instead), so a saved geometry never grows by a title bar.
 * Does nothing before the window has its native window (the caller does it
 * again once it is created) and outside macOS.
 */
void apply(QWindow *window, bool unified);

/*
 * The room the traffic lights take in the top bar, in the window's logical
 * pixels: left() on the left side, right() on the right side (where macOS
 * puts them in a right to left system language); none when the title bar
 * is not unified, outside macOS, or before the window exists.
 */
QMargins buttonsInset(QWindow *window);

}

#endif // VLC_QT_MACOS_TITLEBAR_HPP
