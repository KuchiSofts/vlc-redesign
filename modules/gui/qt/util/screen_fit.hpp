/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
#pragma once

#include <QMargins>
#include <QRect>
#include <QSize>
#include <QSizeF>

class QObject;

// Windows that fit the screen they open on: the size of the main window and
// the scale of the interface at the first start (from the screen, not a
// fixed size that is tiny on a large screen and too large on a small one),
// a saved geometry that no longer fits (a monitor unplugged, another
// resolution), and every other window of the interface (the dialogs), which
// open inside the area of the screen a window may use.
// The rectangles are in Qt's coordinates (device independent pixels); a
// screen at the left of or above the main one has negative ones.
namespace ScreenFit
{

// "Interface size" of the settings (Settings > Interface > Window), the
// same on every app: Automatic, or one of these, in percent of the design's
// own size (100 %: a pixel of the design is one of the system's scaled
// pixels).
inline constexpr int interfaceSizes[] = { 100, 110, 125, 150, 175, 200 };

// The size Automatic finds for a screen, in percent: 100 whenever the system
// already scales the screen (a device pixel ratio over 1: Windows, macOS,
// Wayland) or the screen is of an ordinary density, whatever its size: a
// screen small in pixels has less room, not more. Above 100 only for a
// dense screen reported unscaled (4K at 100 % on X11), from its physical
// size when the monitor gives one; never over 150.
// `screenWidth` is the width of the screen in Qt's pixels, `systemScale`
// what the interface is already multiplied by for the system's text size
// (the logical DPI over 96), `physical` the size of the screen in
// millimetres (empty or absurd when the monitor does not say: ignored),
// `devicePixelRatio` the screen's.
int automaticInterfaceSize(int screenWidth, double systemScale, const QSizeF &physical,
                           double devicePixelRatio = 1.0);

// The choice nearest to a size in percent (a scale kept by an older
// version, a pinch)
int nearestInterfaceSize(double percent);

// The next larger (or smaller) choice, the same at the ends (the zoom keys)
int steppedInterfaceSize(int percent, bool larger);

// The size of the main window at the first start: 16:9, about two thirds of
// the screen, never under the design's 1280 x 720 at this scale when it
// fits, never over nine tenths of the area a window may use.
QSize defaultWindowSize(const QSize &available, double scale = 1.0);

// `wanted` inside `available`: moved in, and made smaller only when it is
// larger. `frame` is what the system draws around the rectangle (the title
// bar and the borders of a window given by its client area).
QRect fitted(const QRect &wanted, const QRect &available, const QMargins &frame = {});

// Whether a window at `geometry` can be used on a screen whose usable area
// is `available`: not larger than it, its title bar on the screen (it may
// hang over an edge, as a window the user moved there).
bool reachable(const QRect &geometry, const QRect &available);

// Every window of the application other than the main one (the dialogs of
// the interface, the detached play queue) is put inside its screen when it
// shows. `parent` owns the filter.
void install(QObject *parent);

}
