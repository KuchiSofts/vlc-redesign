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
#ifndef REDESIGN_ICON_PROVIDER_HPP
#define REDESIGN_ICON_PROVIDER_HPP

#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QPointer>
#include <QQmlAbstractUrlInterceptor>
#include <QQuickImageProvider>

#include <atomic>

class MainCtx;

/**
 * The icons of the redesign drawn from their SVG (the source next to each
 * PNG, from Figma), at the size they are shown times the device pixel ratio:
 * sharp at any scale and interface zoom. Without an SVG, the PNG.
 */
namespace RedesignIcons
{
/* the SVG next to a ":/redesign/...png" in the resources, empty when none */
QString svgOf(const QString &pngPath);
/* the icon drawn at that size (in pixels), tinted when the color is valid
 * (its alpha kept), a null image when the file is missing */
QImage image(const QString &pngPath, const QSize &pixelSize, const QColor &tint = QColor());
/* the same at a logical size for a device pixel ratio */
QPixmap pixmap(const QString &pngPath, const QSize &size, qreal devicePixelRatio, const QColor &tint = QColor());
/* an icon drawing it at every size it is asked (cached per size) */
QIcon icon(const QString &pngPath, const QColor &tint = QColor());
}

/**
 * The icons of the redesign (qrc:///redesign/icons/<name>/<state>.png, and
 * those of the player, qrc:///redesign/player/<name>.png) are drawn for the
 * dark theme: light greys that fade on the light theme.
 * Their light variant, image://redesignicon/<name>/<state>.svg (its SVG, at
 * the size asked) or .png (or player/<name>...), maps the
 * greys as Figma "Player - Light" draws them (#8F8F8F reads #2E2E2E, the
 * dimmed #6B6B6B stays, #EAEAEA reads #111111) and keeps the colors (accent
 * states) and the dark greys (icons drawn on the accent); the divider of the
 * top bar (a dark grey) reads #9A9A9A.
 */
class RedesignIconProvider : public QQuickImageProvider
{
public:
    static constexpr const char *providerId = "redesignicon";

    RedesignIconProvider();

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;
};

/**
 * Sends the icons of the redesign to their SVG when they have one (an Image
 * with a sourceSize draws it at that size times the device pixel ratio), and
 * to their light variant while the light theme is used. When the theme
 * changes, the images of the windows showing one load it again (an image
 * keeps the variant it was loaded with).
 * Owned by the engine it is installed on (its parent).
 */
class RedesignIconInterceptor : public QObject, public QQmlAbstractUrlInterceptor
{
public:
    RedesignIconInterceptor(MainCtx *ctx, QObject *parent);

    QUrl intercept(const QUrl &url, QQmlAbstractUrlInterceptor::DataType type) override;

private:
    void updateTheme();
    void updateDensity();

    QPointer<MainCtx> m_ctx;
    // Read from the threads of the engine
    std::atomic<bool> m_light { false };
    // A screen shows more than a pixel per unit (150 %, 200 %): the icons
    // drawn twice as large are taken where there are
    std::atomic<bool> m_dense { false };
};

#endif // REDESIGN_ICON_PROVIDER_HPP
