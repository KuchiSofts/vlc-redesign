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
#include "menus/redesign_menu_style.hpp"
#include "util/imagehelper.hpp"

#include <QApplication>
#include <QFile>
#include <QHash>
#include <QIconEngine>
#include <QPainter>
#include <QStyle>
#include <QStyleOption>
#include <QGuiApplication>
#include <QImage>
#include <QImageIOHandler>
#include <QMutex>
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

// A raster of the redesign (the icons, the player's, the others below
// /redesign/), or its SVG sent by the interceptor
bool isRedesignIcon(const QUrl &url)
{
    if (url.scheme() != QLatin1String("qrc"))
        return false;
    const QString path = url.path();
    if (!path.startsWith(QLatin1String("/redesign/")))
        return false;
    if (path.endsWith(QLatin1String(".png")))
        return true;
    // an SVG of the icons or of the player (not one of its own, such as
    // play.svg): the URL of the PNG it was drawn from before (the PNG files
    // are gone, their names stay the icons' ids)
    return path.endsWith(QLatin1String(".svg"))
           && (path.startsWith(iconsPath) || path.startsWith(playerPath))
           && QFile::exists(QLatin1Char(':') + path);
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

// The size to draw a picture of this natural size at for a request (Qt asks
// the sourceSize times the device pixel ratio; one side 0 keeps the ratio)
QSize svgSize(const QString &svg)
{
    // (asked from the image reader's thread and the interface's)
    static QMutex lock;
    static QHash<QString, QSize> sizes;
    const QMutexLocker locker(&lock);
    const auto it = sizes.constFind(svg);
    if (it != sizes.cend())
        return *it;
    QSize size;
    const QScopedPointer<QImageIOHandler> handler(ImageHelper::createSvgImageIOHandler());
    QFile file(svg);
    if (handler && file.open(QFile::ReadOnly))
    {
        handler->setDevice(&file);
        if (handler->canRead())
            size = handler->option(QImageIOHandler::Size).toSize();
    }
    sizes.insert(svg, size);
    return size;
}

QSize sizeFor(const QSize &natural, const QSize &requested)
{
    if (!requested.isValid() || (requested.width() <= 0 && requested.height() <= 0) || natural.isEmpty())
        return natural;
    if (requested.width() <= 0)
        return QSize(qMax(1, natural.width() * requested.height() / natural.height()), requested.height());
    if (requested.height() <= 0)
        return QSize(requested.width(), qMax(1, natural.height() * requested.width() / natural.width()));
    return natural.scaled(requested, Qt::KeepAspectRatio);
}

// An icon engine drawing a redesign icon from its SVG at each size asked
class RedesignIconEngine : public QIconEngine
{
public:
    RedesignIconEngine(const QString &file, const QColor &tint) : m_file(file), m_tint(tint) {}

    void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) override
    {
        const qreal ratio = painter->device() ? painter->device()->devicePixelRatioF() : qApp->devicePixelRatio();
        painter->drawPixmap(rect, scaledPixmap(rect.size(), mode, state, ratio));
    }

    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override
    {
        return scaledPixmap(size, mode, state, 1.0);
    }

    QPixmap scaledPixmap(const QSize &size, QIcon::Mode mode, QIcon::State, qreal scale) override
    {
        const QSize pixels = (QSizeF(size) * scale).toSize();
        const QString key = QStringLiteral("%1x%2/%3").arg(pixels.width()).arg(pixels.height()).arg(int(mode));
        const auto it = m_cache.constFind(key);
        if (it != m_cache.cend())
            return *it;
        QPixmap pixmap = QPixmap::fromImage(RedesignIcons::image(m_file, pixels, m_tint));
        if (pixmap.isNull())
            return pixmap;
        if (mode == QIcon::Disabled && qApp->style())
        {
            QStyleOption option;
            option.palette = qApp->palette();
            pixmap = qApp->style()->generatedIconPixmap(QIcon::Disabled, pixmap, &option);
        }
        pixmap.setDevicePixelRatio(scale);
        m_cache.insert(key, pixmap);
        return pixmap;
    }

    QSize actualSize(const QSize &size, QIcon::Mode, QIcon::State) override { return size; }
    QString key() const override { return QStringLiteral("RedesignIconEngine"); }
    QIconEngine *clone() const override { return new RedesignIconEngine(m_file, m_tint); }

private:
    QString m_file;
    QColor m_tint;
    QHash<QString, QPixmap> m_cache;
};

}

QString RedesignIcons::svgOf(const QString &pngPath)
{
    if (!pngPath.endsWith(QLatin1String(".png")))
        return {};
    QString svg = pngPath.chopped(4) + QLatin1String(".svg");
    // "qrc:///redesign/..." reads ":/redesign/..."
    if (svg.startsWith(QLatin1String("qrc:")))
        svg = QLatin1Char(':') + QUrl(svg).path();
    return QFile::exists(svg) ? svg : QString();
}

QImage RedesignIcons::image(const QString &pngPath, const QSize &pixelSize, const QColor &tint)
{
    QImage image;
    const QString svg = svgOf(pngPath);
    if (!svg.isEmpty())
        image = ImageHelper::loadSvgToImage(svg, pixelSize);
    if (image.isNull())
    {
        image = QImage(pngPath);
        if (!image.isNull() && pixelSize.isValid() && !pixelSize.isEmpty() && image.size() != pixelSize)
            image = image.scaled(pixelSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    if (image.isNull() || !tint.isValid())
        return image;
    image = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    QPainter painter(&image);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(image.rect(), tint);
    return image;
}

QPixmap RedesignIcons::pixmap(const QString &pngPath, const QSize &size, qreal devicePixelRatio, const QColor &tint)
{
    QPixmap pixmap = QPixmap::fromImage(image(pngPath, (QSizeF(size) * devicePixelRatio).toSize(), tint));
    pixmap.setDevicePixelRatio(devicePixelRatio);
    return pixmap;
}

QIcon RedesignIcons::icon(const QString &pngPath, const QColor &tint)
{
    if (!QFile::exists(pngPath) && svgOf(pngPath).isEmpty())
        return {};
    return QIcon(new RedesignIconEngine(pngPath, tint));
}

RedesignIconProvider::RedesignIconProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QImage RedesignIconProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    // The id is the path below the icons, without a query: "<name>/<state>"
    // with ".svg" (drawn from the SVG at the size asked) or ".png"
    QString path = id.section(QLatin1Char('?'), 0, 0);
    if (path.contains(QLatin1String("..")))
        return {};
    const bool vector = path.endsWith(QLatin1String(".svg"));
    if (vector)
        path = path.chopped(4) + QLatin1String(".png");

    const QString file = path.startsWith(QLatin1String("player/"))
                         ? QString(QLatin1String(":/redesign/") + path)
                         : QString(QLatin1String(":") + iconsPath + path);
    QImage image;
    const QString svg = vector ? RedesignIcons::svgOf(file) : QString();
    if (!svg.isEmpty())
    {
        // its natural size (the SVG's, the size of the PNG it replaced)
        // scaled to the request
        QSize base = svgSize(svg);
        if (base.isEmpty())
            base = QSize(24, 24);
        image = ImageHelper::loadSvgToImage(svg, sizeFor(base, requestedSize));
    }
    if (image.isNull())
    {
        image = QImage(file);
        if (image.isNull())
            return {};
        // asked larger than drawn: the icon drawn twice as large, scaled down
        if (requestedSize.isValid()
         && (requestedSize.width() > image.width() || requestedSize.height() > image.height()))
        {
            const QImage twice(file.chopped(4) + QLatin1String("@2x.png"));
            if (!twice.isNull())
                image = twice;
        }
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
    // (as the menus: a desktop telling no scheme follows the palette)
    bool light = !RedesignMenuStyle::darkScheme(nullptr);
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

// On a screen past one pixel per unit, an icon without an SVG drawn twice
// as large (its "@2x" file) when there is one: Qt reads its density from the
// name, and a sourceSize scales it down instead of a 1x one up (blurred at
// 150 and 200 %). (The SVGs follow the ratio by themselves.)
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
    if (!isRedesignIcon(url))
        return url;
    // an icon loaded again (theme, screens): from its own name
    QString path = url.path();
    const bool renamed = !path.endsWith(QLatin1String(".png")) || path.endsWith(QLatin1String("@2x.png"));
    if (path.endsWith(QLatin1String("@2x.png")))
        path = path.chopped(7) + QLatin1String(".png");
    else if (path.endsWith(QLatin1String(".svg")))
        path = path.chopped(4) + QLatin1String(".png");
    // its SVG when it has one: an Image draws it at its sourceSize times the
    // device pixel ratio, sharp at any scale and zoom. Qt decides that from
    // the URL as the QML wrote it, not from the one returned here (the
    // interceptor runs when the image loads): the QML names the icons by
    // their ".svg". With a ".png" name the icon was drawn at 1x and stretched
    // at 125-200 % (strokes 76 % strong, spread over 4 to 6 pixels)
    const bool vector = QFile::exists(QLatin1Char(':') + path.chopped(4) + QLatin1String(".svg"));

    QString id;
    if (m_light)
    {
        if (path.startsWith(iconsPath))
            id = path.mid(iconsPath.size());
        // the corners of the window have light variants of their own
        else if (path.startsWith(playerPath) && !path.mid(playerPath.size()).startsWith(QLatin1String("corner-")))
            id = QLatin1String("player/") + path.mid(playerPath.size());
    }
    if (!id.isEmpty())
    {
        if (vector)
            id = id.chopped(4) + QLatin1String(".svg");
        return QUrl(QLatin1String("image://") + QLatin1String(RedesignIconProvider::providerId)
                    + QLatin1Char('/') + id);
    }
    QUrl drawn(url);
    if (vector)
    {
        drawn.setPath(path.chopped(4) + QLatin1String(".svg"));
        return drawn;
    }
    // no SVG: on a dense screen the icon drawn twice as large
    if (m_dense && path.startsWith(iconsPath))
    {
        const QString twice = path.chopped(4) + QLatin1String("@2x.png");
        if (QFile::exists(QLatin1Char(':') + twice))
        {
            drawn.setPath(twice);
            return drawn;
        }
    }
    if (!renamed)
        return url;
    drawn.setPath(path);
    return drawn;
}
