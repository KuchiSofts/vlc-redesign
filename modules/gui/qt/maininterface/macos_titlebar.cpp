/*****************************************************************************
 * macos_titlebar.cpp: the unified title bar of the main window on macOS
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
#include "macos_titlebar.hpp"

#include <QWindow>

#ifdef Q_OS_MACOS

#include <algorithm>
#include <cmath>
#include <limits>

/* AppKit through the Objective-C runtime: the Qt module is C++ only (no
 * Objective-C++ in its build), and these are a few calls */
#include <objc/runtime.h>
#include <objc/message.h>
#include <CoreGraphics/CGGeometry.h>     /* NSRect is CGRect on 64-bit */

namespace
{

/* NSWindowStyleMaskFullSizeContentView */
constexpr unsigned long kFullSizeContentView = 1UL << 15;
/* NSWindowStyleMaskFullScreen */
constexpr unsigned long kFullScreen = 1UL << 14;
/* NSWindowTitleVisible, NSWindowTitleHidden */
constexpr long kTitleVisible = 0;
constexpr long kTitleHidden = 1;
/* NSWindowCloseButton, NSWindowMiniaturizeButton, NSWindowZoomButton */
constexpr unsigned long kWindowButtons[] = { 0, 1, 2 };
/* between the traffic lights and the first control of the top bar */
constexpr qreal kGap = 10;
/* the traffic lights' place when AppKit does not tell it (macOS 11 and
 * later: 7 pt in, three buttons of 14 pt, 6 pt apart) */
constexpr qreal kDefaultButtonsEnd = 61;

template <typename R = void, typename... Args>
R send(id object, const char *selector, Args... args)
{
    using Function = R (*)(id, SEL, Args...);
    return reinterpret_cast<Function>(objc_msgSend)(object, sel_registerName(selector), args...);
}

/* -[NSView frame]: a structure of 32 bytes, returned in memory on x86_64
 * (objc_msgSend_stret), in registers' place on arm64 (objc_msgSend) */
CGRect frameOf(id view)
{
    using Function = CGRect (*)(id, SEL);
#if defined(__x86_64__)
    return reinterpret_cast<Function>(objc_msgSend_stret)(view, sel_registerName("frame"));
#else
    return reinterpret_cast<Function>(objc_msgSend)(view, sel_registerName("frame"));
#endif
}

/* The NSWindow of a top-level QWindow (its winId is its NSView), or nil
 * before Qt made it */
id nativeWindow(QWindow *window)
{
    if (!window || !window->handle())
        return nil;
    const id view = reinterpret_cast<id>(window->winId());
    return view ? send<id>(view, "window") : nil;
}

}

namespace macos_titlebar
{

void apply(QWindow *window, bool unified)
{
    const id nsWindow = nativeWindow(window);
    if (!nsWindow)
        return;

    // never frameless here: the system's frame is what the unified bar keeps
    if (window->flags().testFlag(Qt::FramelessWindowHint))
        window->setFlag(Qt::FramelessWindowHint, false);

    // Qt 6.8 has no flag for this (Qt::ExpandedClientAreaHint is 6.9): it
    // keeps the full size content when it changes the window's other flags
    // (always on top...), and this is called again after them, cheaply
    const unsigned long mask = send<unsigned long>(nsWindow, "styleMask");
    // (not while in full screen: again once out of it)
    if (((mask & kFullSizeContentView) != 0) != unified && !(mask & kFullScreen))
    {
        const bool zoomed = window->windowStates() & (Qt::WindowMaximized | Qt::WindowFullScreen);
        const QRect content = window->geometry();
        send<void, unsigned long>(nsWindow, "setStyleMask:",
                                  unified ? (mask | kFullSizeContentView) : (mask & ~kFullSizeContentView));
        // the content where it was: the frame moves by the height of the
        // title bar instead (else a saved geometry would grow by it at every
        // start)
        if (!zoomed && content.isValid())
            window->setGeometry(content);
    }
    send<void, BOOL>(nsWindow, "setTitlebarAppearsTransparent:", unified ? YES : NO);
    // the window keeps its title (the Window menu, Mission Control, the
    // Dock), the top bar shows its own
    send<void, long>(nsWindow, "setTitleVisibility:", unified ? kTitleHidden : kTitleVisible);
}

QMargins buttonsInset(QWindow *window)
{
    const id nsWindow = nativeWindow(window);
    if (!nsWindow || !(send<unsigned long>(nsWindow, "styleMask") & kFullSizeContentView))
        return {};

    qreal start = std::numeric_limits<qreal>::max();
    qreal end = 0;
    qreal width = 0;
    for (const unsigned long type : kWindowButtons)
    {
        const id button = send<id, unsigned long>(nsWindow, "standardWindowButton:", type);
        if (!button || send<BOOL>(button, "isHidden"))
            continue;
        const CGRect frame = frameOf(button);
        start = std::min<qreal>(start, frame.origin.x);
        end = std::max<qreal>(end, frame.origin.x + frame.size.width);
        // the title bar view, as wide as the window
        if (const id bar = send<id>(button, "superview"))
            width = std::max<qreal>(width, frameOf(bar).size.width);
    }
    if (end <= 0 || width <= 0)
        return QMargins(int(std::ceil(kDefaultButtonsEnd + kGap)), 0, 0, 0);

    // on the right in a right to left system language
    if (start + end > width)
        return QMargins(0, 0, int(std::ceil(width - start + kGap)), 0);
    return QMargins(int(std::ceil(end + kGap)), 0, 0, 0);
}

}

#else

namespace macos_titlebar
{

void apply(QWindow *, bool)
{
}

QMargins buttonsInset(QWindow *)
{
    return {};
}

}

#endif
