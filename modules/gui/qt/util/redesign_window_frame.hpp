/*****************************************************************************
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
#ifndef REDESIGN_WINDOW_FRAME_HPP
#define REDESIGN_WINDOW_FRAME_HPP

#include <QColor>

class QWidget;

/**
 * The title bar of the redesign's dialogs for the classic windows: the
 * bar of RedesignDialog.qml ("Frame 120": #090909, padding 9, the muted
 * title in Inter 14 10 px in, the 32 x 32 buttons: minimize and maximize
 * where the window has them and can be resized, close) instead of the
 * system's.
 *
 * On Windows the window keeps its native frame (snapping, the resize
 * borders, the shadow, the rounded corners of Windows 11): only the caption
 * is taken out of the non-client area (WM_NCCALCSIZE), and the bar says
 * where the caption is (WM_NCHITTEST), as the main window does
 * (CSDWin32EventHandler). Elsewhere the system title bar stays.
 *
 * The bar's colours come from the style sheet of RedesignWidgetTheme
 * (#redesignTitleBar, #redesignTitle, #redesignTitleButton).
 */
namespace RedesignWindowFrame
{

// Gives the title bar to the window (once); a window that should keep its
// frame (a popup, a frameless one) is left as it is
void install(QWidget *window);

// The sizes at the interface scale and the colours of the frame drawn by
// the system (the border of Windows 11, dark or light)
void update(QWidget *window, double scale, bool dark, const QColor &border);

}

#endif // REDESIGN_WINDOW_FRAME_HPP
