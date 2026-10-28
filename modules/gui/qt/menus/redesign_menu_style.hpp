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
#ifndef REDESIGN_MENU_STYLE_HPP
#define REDESIGN_MENU_STYLE_HPP

#include <QColor>
#include <QIcon>
#include <QPointer>
#include <QProxyStyle>
#include <QString>

class MainCtx;
class QWindow;

/**
 * Draws the widget menus (right-click menu, track, device and sort menus)
 * like the redesign menus (Figma "right-click menu"): #191919 surface,
 * 4px padding, 48px rows with 6px hover corners, 24px icons, Inter 16px,
 * shortcuts right aligned and muted, 1px separators.
 * Installed on the application, it takes over each menu when it is first
 * polished, and has Windows round its corners when it shows.
 */
class RedesignMenuStyle : public QProxyStyle
{
    Q_OBJECT
public:
    explicit RedesignMenuStyle(MainCtx *ctx);

    // Two-tone icon of the design's menus (pixmaps/redesign/duo/<name>.svg),
    // else the icon of the redesign set (pixmaps/redesign/icons/<name>) in
    // the accent color
    static QIcon accentIcon(MainCtx *ctx, const QString &name);
    // Icon of a classic menu entry (pixmaps/menu): the two-tone icon of the
    // design standing for the same action, else the classic icon
    static QIcon menuIcon(const QString &classicPath, const QWidget *widget = nullptr);

    // Shared with the classic windows (RedesignWidgetTheme):
    // the scheme chosen (System follows the system)
    static bool darkScheme(MainCtx *ctx);
    // the accent chosen in the settings
    static QColor accentColor(MainCtx *ctx);
    // Accent Dim of the design: #AB6314 for the default accent, 0.81 of
    // another one (VLCStyle.redesign.accentDim)
    static QColor accentDim(const QColor &accent);
    // the family of the bundled Inter, loaded once
    static QString fontFamily();
    // The 1 px frame of every window (the main window, the classic ones,
    // the menus; the QML cards draw it as VLCStyle.redesign.windowBorder)
    static QColor windowBorder(bool dark);
    // Has the system draw it around the window (Windows 11)
    static void setWindowBorder(QWindow *window, bool dark);

    int pixelMetric(PixelMetric metric, const QStyleOption *option = nullptr,
                    const QWidget *widget = nullptr) const override;
    QSize sizeFromContents(ContentsType type, const QStyleOption *option,
                           const QSize &size, const QWidget *widget) const override;
    void drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                       QPainter *painter, const QWidget *widget = nullptr) const override;
    void drawControl(ControlElement element, const QStyleOption *option,
                     QPainter *painter, const QWidget *widget = nullptr) const override;
    int styleHint(StyleHint hint, const QStyleOption *option = nullptr,
                  const QWidget *widget = nullptr, QStyleHintReturn *returnData = nullptr) const override;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    struct Palette
    {
        QColor surface, text, textHover, textMuted, hover, disabled, separator, accent;
    };

    bool isDark() const;
    Palette palette() const;
    QFont menuFont() const;

    QPointer<MainCtx> m_ctx;
    QString m_fontFamily;
};

#endif // REDESIGN_MENU_STYLE_HPP
