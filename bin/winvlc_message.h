/*****************************************************************************
 * winvlc_message.h: the messages vlc.exe shows before the interface exists
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

#ifndef VLC_WINVLC_MESSAGE_H
#define VLC_WINVLC_MESSAGE_H

#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A message drawn as the dialogs of the redesign (RedesignDialog.qml): a
 * frameless window with the title bar and its close button, a heading, a
 * line of explanation (text, may be NULL), cancel on the left (NULL: none)
 * and the action on the right, in the colours of the Windows theme (dark
 * or light). Waits for the answer: 1 for the action, 0 otherwise. */
int RedesignMessageBox(const wchar_t *title, const wchar_t *heading,
                       const wchar_t *text, const wchar_t *accept,
                       const wchar_t *cancel);

/* The same with UTF-8 texts (the translations of _()) */
int RedesignMessageBoxUtf8(const char *title, const char *heading,
                           const char *text, const char *accept,
                           const char *cancel);

#ifdef __cplusplus
}
#endif

#endif
