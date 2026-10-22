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
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "redesign_icon_provider.hpp"

#include "maininterface/mainctx.hpp"
#include "util/color_scheme_model.hpp"

#include <QFile>
#include <QGuiApplication>
#include <QImage>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScreen>
#include <QStyleHints>
#include <QUrl>

#include <algorithm>

namespace {

constexpr QLatin1String iconsPath { "/redesign/icons/" };
// the icons of the player (window buttons...), its id "player/<name>.png"
constexpr QLatin1String playerPath { "/redesign/player/" };

// Greys lighter than this are drawn for the dark background
constexpr int lightGreyThreshold = 0x60;
// Channels this close make a grey
constexpr int greyTolerance = 24;

// A grey of the dark theme in the light one (Figma "Player - Light"):
// #6B6B6B and darker stay, #8F8F8F reads #2E2E2E, #EAEAEA #111111
int lightGrey(int v)
{
    if (v <= 0x6B)
        return v;
    if (v <= 0x8F)
        return 0x6B + (v - 0x6B) * (0x2E - 0x6B) / (0x8F - 0x6B);
    if (v <= 0xEA)
        return 0x2E + (v - 0x8F) * (0x11 - 0x2E) / (0xEA - 0x8F);
    return 0x11;
}

// An icon the interceptor sends to its light variant
bool isRedesignIcon(const QUrl &url)
{
    if (url.scheme() != QLatin1String("qrc"))
        return false;
    const QString path = url.path();
    return path.endsWith(QLatin1String(".png"))
           && (path.startsWith(iconsPath) || path.startsWith(playerPath));
}

// The images of the item and its children showing such an icon load it
// again (through the interceptor: the variant of the theme). Their binding
// stays, a write from C++ does not remove it.
void reloadIcons(QQuickItem *item)
{
    const QList<QQuickItem *> children = item->childItems();
    for (QQuickItem *child : children)
        reloadIcons(child);
    if (!item->inherits("QQuickImageBase"))
        return;
    const QUrl source = item->property("source").toUrl();
    if (!isRedesignIcon(source))
        return;
    item->setProperty("source", QUrl());
    item->setProperty("source", source);
}

}

RedesignIconProvider::RedesignIconProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QImage RedesignIconProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    // The id is the path below the icons, without a query
    const QString path = id.section(QLatin1Char('?'), 0, 0);
    if (path.contains(QLatin1String("..")))
        return {};

    const QString file = path.startsWith(QLatin1String("player/"))
                         ? QString(QLatin1String(":/redesign/") + path)
                         : QString(QLatin1String(":") + iconsPath + path);
    QImage image(file);
    if (image.isNull())
        return {};
    // asked larger than drawn (a sourceSize on a dense screen): the icon
    // drawn twice as large, when there is one, scaled down
    if (requestedSize.isValid()
     && (requestedSize.width() > image.width() || requestedSize.height() > image.height()))
    {
        const QImage twice(file.chopped(4) + QLatin1String("@2x.png"));
        if (!twice.isNull())
            image = twice;
    }
    image = image.convertToFormat(QImage::Format_ARGB32);
    // the divider of the top bar: #9A9A9A (Figma: #9E9E9E over the bar)
    const bool divider = path.startsWith(QLatin1String("divider/"));

    for (int y = 0; y < image.height(); ++y)
    {
        QRgb *line = reinterpret_cast<QRgb *>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x)
        {
            const QRgb pixel = line[x];
            if (qAlpha(pixel) == 0)
                continue;
            if (divider)
            {
                line[x] = qRgba(0x9A, 0x9A, 0x9A, qAlpha(pixel));
                continue;
            }
            const int r = qRed(pixel), g = qGreen(pixel), b = qBlue(pixel);
            const int high = std::max({ r, g, b });
            const int low = std::min({ r, g, b });
            if (high - low > greyTolerance || high < lightGreyThreshold)
                continue;
            line[x] = qRgba(lightGrey(r), lightGrey(g), lightGrey(b), qAlpha(pixel));
        }
    }

    if (requestedSize.isValid() && !requestedSize.isEmpty() && requestedSize != image.size())
        image = image.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    if (size)
        *size = image.size();
    return image;
}

RedesignIconInterceptor::RedesignIconInterceptor(MainCtx *ctx, QObject *parent)
    : QObject(parent)
    , m_ctx(ctx)
{
    updateTheme();
    updateDensity();
    // a scale changed in Windows' settings: the screen's size in units
    // changes (its logical DPI stays 96 with Qt's rounding of the scale)
    const auto follow = [this](QScreen *screen) {
        connect(screen, &QScreen::geometryChanged, this, &RedesignIconInterceptor::updateDensity);
        connect(screen, &QScreen::logicalDotsPerInchChanged, this, &RedesignIconInterceptor::updateDensity);
    };
    connect(qApp, &QGuiApplication::screenAdded, this, [this, follow](QScreen *screen) {
        follow(screen);
        updateDensity();
    });
    connect(qApp, &QGuiApplication::screenRemoved, this, &RedesignIconInterceptor::updateDensity);
    for (QScreen *screen : QGuiApplication::screens())
        follow(screen);
    if (ColorSchemeModel *scheme = ctx ? ctx->getColorScheme() : nullptr)
        connect(scheme, &ColorSchemeModel::currentChanged, this, &RedesignIconInterceptor::updateTheme);
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
            this, &RedesignIconInterceptor::updateTheme);
}

void RedesignIconInterceptor::updateTheme()
{
    bool light = QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Light;
    if (ColorSchemeModel *scheme = m_ctx ? m_ctx->getColorScheme() : nullptr)
    {
        switch (scheme->currentScheme())
        {
        case ColorSchemeModel::Day:
            light = true;
            break;
        case ColorSchemeModel::Night:
            light = false;
            break;
        default:
            break;
        }
    }
    if (m_light.exchange(light) == light)
        return;
    // Images keep the variant they were loaded with: the icons shown load
    // again (popups are in the overlay, a child of the content item)
    const QWindowList windows = QGuiApplication::allWindows();
    for (QWindow *window : windows)
    {
        if (auto quickWindow = qobject_cast<QQuickWindow *>(window))
            if (QQuickItem *content = quickWindow->contentItem())
                reloadIcons(content);
    }
}

// On a screen past one pixel per unit, an icon drawn twice as large (its
// "@2x" file) when there is one: Qt reads its density from the name, and a
// sourceSize scales it down instead of a 1x one up (blurred at 150 and 200 %)
void RedesignIconInterceptor::updateDensity()
{
    bool dense = false;
    for (const QScreen *screen : QGuiApplication::screens())
        dense = dense || screen->devicePixelRatio() > 1.0;
    if (m_dense.exchange(dense) == dense)
        return;
    const QWindowList windows = QGuiApplication::allWindows();
    for (QWindow *window : windows)
    {
        if (auto quickWindow = qobject_cast<QQuickWindow *>(window))
            if (QQuickItem *content = quickWindow->contentItem())
                reloadIcons(content);
    }
}

QUrl RedesignIconInterceptor::intercept(const QUrl &url, QQmlAbstractUrlInterceptor::DataType type)
{
    if (type != QQmlAbstractUrlInterceptor::UrlString)
        return url;
    if (url.scheme() != QLatin1String("qrc"))
        return url;
    QString path = url.path();
    if (!path.endsWith(QLatin1String(".png")))
        return url;
    // an icon loaded again (theme, screens): from its own name
    const bool wasTwice = path.endsWith(QLatin1String("@2x.png"));
    if (wasTwice)
        path = path.chopped(7) + QLatin1String(".png");
    if (!m_light)
    {
        if (m_dense && path.startsWith(iconsPath))
        {
            const QString twice = path.chopped(4) + QLatin1String("@2x.png");
            if (QFile::exists(QLatin1Char(':') + twice))
            {
                QUrl dense(url);
                dense.setPath(twice);
                return dense;
            }
        }
        if (!wasTwice)
            return url;
        QUrl one(url);
        one.setPath(path);
        return one;
    }
    QString id;
    if (path.startsWith(iconsPath))
        id = path.mid(iconsPath.size());
    // the corners of the window have light variants of their own
    else if (path.startsWith(playerPath) && !path.mid(playerPath.size()).startsWith(QLatin1String("corner-")))
        id = QLatin1String("player/") + path.mid(playerPath.size());
    else
        return url;
    return QUrl(QLatin1String("image://") + QLatin1String(RedesignIconProvider::providerId)
                + QLatin1Char('/') + id);
}
