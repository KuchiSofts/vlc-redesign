/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
#include "screen_fit.hpp"

#include <QtGlobal>
#include <QtMath>

#include <algorithm>
#include <cmath>
#include <iterator>

// (the geometry only: QtCore. The windows are in screen_fit_filter.cpp, so
// that the tests build this file by itself.)
namespace ScreenFit
{

int automaticInterfaceSize(int screenWidth, double systemScale, const QSizeF &physical)
{
    if (screenWidth <= 0)
        return 100;
    // the width as the system wants it used
    const double scale = systemScale > 0.25 ? systemScale : 1.0;
    const int width = qRound(screenWidth / scale);

    const int page = std::clamp(width * 4 / 3, 1280, 1920);
    // (to 5 %: the sizes stay on whole pixels more often)
    int percent = qRound(1920. * 100. / page / 5.) * 5;

    // what the monitor says of its size: a laptop panel to a large TV
    const bool sized = physical.width() >= 100. && physical.height() >= 60.
                       && physical.width() <= 3000. && physical.height() <= 2000.;
    if (sized)
    {
        // pixels per inch as the interface sees them: about 96 on a desktop
        // monitor, 110 on a laptop the system scales
        const double density = width / (physical.width() / 25.4);
        if (density > 150.)
            // dense and left unscaled: the text as on a 120 dpi screen
            percent = std::max(percent, qRound(density / 120. * 100. / 5.) * 5);
    }
    return std::clamp(percent, 100, 150);
}

int nearestInterfaceSize(double percent)
{
    int nearest = interfaceSizes[0];
    for (int size : interfaceSizes)
        if (std::fabs(size - percent) < std::fabs(nearest - percent))
            nearest = size;
    return nearest;
}

int steppedInterfaceSize(int percent, bool larger)
{
    const int count = int(std::size(interfaceSizes));
    if (larger)
    {
        for (int i = 0; i < count; ++i)
            if (interfaceSizes[i] > percent)
                return interfaceSizes[i];
        return interfaceSizes[count - 1];
    }
    for (int i = count - 1; i >= 0; --i)
        if (interfaceSizes[i] < percent)
            return interfaceSizes[i];
    return interfaceSizes[0];
}

QSize defaultWindowSize(const QSize &available, double scale)
{
    if (available.width() <= 0 || available.height() <= 0)
        return QSize(1280, 720);

    // two thirds of the width, three quarters of the height: the smaller
    // (an ultra-wide screen is not filled from top to bottom)
    double width = std::min(available.width() * 0.66,
                            available.height() * 0.75 * 16. / 9.);
    // the design's window at this scale
    width = std::max(width, 1280. * std::max(scale, 0.5));

    const double maxWidth = std::floor(available.width() * 0.9);
    const double maxHeight = std::floor(available.height() * 0.9);
    width = std::min(width, maxWidth);
    double height = width * 9. / 16.;
    if (height > maxHeight)
    {
        height = maxHeight;
        width = std::min(maxWidth, height * 16. / 9.);
    }
    return QSize(qRound(width), qRound(height));
}

QRect fitted(const QRect &wanted, const QRect &available, const QMargins &frame)
{
    if (!available.isValid() || !wanted.isValid())
        return wanted;

    // the window with its frame
    QRect outer = wanted.marginsAdded(frame);
    outer.setSize(outer.size().boundedTo(available.size()));
    if (outer.right() > available.right())
        outer.moveRight(available.right());
    if (outer.bottom() > available.bottom())
        outer.moveBottom(available.bottom());
    // the top left corner last: the title bar is what must be reached
    if (outer.left() < available.left())
        outer.moveLeft(available.left());
    if (outer.top() < available.top())
        outer.moveTop(available.top());
    return outer.marginsRemoved(frame);
}

bool reachable(const QRect &geometry, const QRect &available)
{
    if (!available.isValid() || !geometry.isValid())
        return false;
    if (geometry.width() > available.width() || geometry.height() > available.height())
        return false;
    // the strip a window is moved by: its top 40 pixels, at least 120 wide
    // on the screen (and not above it)
    const QRect bar(geometry.left(), geometry.top(), geometry.width(), 40);
    const QRect shown = bar.intersected(available);
    return geometry.top() >= available.top() - 8
           && shown.height() >= 32
           && shown.width() >= std::min(120, geometry.width());
}

}
