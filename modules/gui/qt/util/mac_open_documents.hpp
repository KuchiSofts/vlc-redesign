/*****************************************************************************
 * mac_open_documents.hpp: files macOS asks the app to open (Apple events)
 *****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
#ifndef VLC_QT_MAC_OPEN_DOCUMENTS_HPP
#define VLC_QT_MAC_OPEN_DOCUMENTS_HPP

#include <QUrl>

#include <functional>

/* The files opened from Finder ("Open With", a double click, a drop on the
 * Dock icon) come as an "open documents" Apple event, the launch's one too.
 * The app takes that event itself, from before its window exists to the
 * end, whoever (AppKit, Qt) would take it otherwise, and gives each file to
 * `sink`, on the main thread; each step is in the debug log (#84, #85).
 * Files that come before a sink is set are kept for it. Nothing elsewhere
 * than on macOS. */
namespace MacOpenDocuments
{
/* (again, when AppKit could have put its own handler back: harmless) */
void install();
void setSink(std::function<void(const QUrl &)> sink);
}

#endif
