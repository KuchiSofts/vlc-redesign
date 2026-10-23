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
#ifndef REDESIGN_WIDGET_THEME_HPP
#define REDESIGN_WIDGET_THEME_HPP

class QWidget;
struct qt_intf_t;

/**
 * The look of the redesign for the classic windows (Open, Convert, Media
 * Information, Bookmarks, Messages, Plugins, Preferences...): a style sheet
 * and a palette made from the tokens of VLCStyle.redesign, in the colours
 * of RedesignMenuStyle, with the bundled Inter.
 * A window keeps it when the theme (Dark, Light, System), the accent or the
 * interface scale change.
 */
namespace RedesignWidgetTheme
{

// Gives the look to a window and what it holds (called once for every
// classic window, by QVLCDialog::setWindowTransientParent()). p_intf may be
// null: the interface of the windows already styled is used. Menus keep
// RedesignMenuStyle.
void apply(QWidget *window, qt_intf_t *p_intf);

}

#endif // REDESIGN_WIDGET_THEME_HPP
