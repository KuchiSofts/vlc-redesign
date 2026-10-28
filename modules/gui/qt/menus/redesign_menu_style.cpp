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

#include "redesign_menu_style.hpp"

#include "maininterface/mainctx.hpp"
#include "util/color_scheme_model.hpp"
#include "util/colorizedsvgicon.hpp"
#include "util/redesign_icon_provider.hpp"
#include "util/redesign_accent.hpp"

#include <QApplication>
#include <QEvent>
#include <QFile>
#include <QHash>
#include <QFontDatabase>
#include <QRegularExpression>
#include <QMenu>
#include <QOperatingSystemVersion>
#include <QPainter>
#include <QPainterPath>
#include <QStyleHints>
#include <QStyleOptionMenuItem>
#include <QTransform>
#include <QWindow>

#include <algorithm>
#include <cmath>

#ifdef _WIN32
# include <windows.h>
# include <dwmapi.h>
#endif

namespace {

// Figma "right-click menu" metrics, in device independent pixels
constexpr int rowHeight = 48;
// a line across the menu, between two rows
constexpr int separatorHeight = 1;
// above and below the rows, and the inset of their hover background (the
// rows span the menu: the separators reach its sides)
constexpr int panelPadding = 4;
constexpr int sidePadding = 14;
constexpr int iconSize = 24;
constexpr int iconGap = 10;
constexpr int shortcutGap = 32;
constexpr int arrowWidth = 24;
constexpr int arrowGap = 8;
constexpr int hoverRadius = 6;
// Figma menus are 265 wide (rows of 257) unless their content needs more
// (the right-click menu is 292: RedesignPopupMenu)
constexpr int minimumWidth = 257;
constexpr int fontPixelSize = 14;

bool isMenu(const QWidget *widget)
{
    return qobject_cast<const QMenu *>(widget) != nullptr;
}

// Label and shortcut of a menu item: QMenu passes "label\tshortcut"
void splitText(const QString &text, QString *label, QString *shortcut)
{
    const qsizetype tab = text.indexOf('\t');
    *label = tab >= 0 ? text.left(tab) : text;
    *shortcut = tab >= 0 ? text.mid(tab + 1) : QString();
    // "Ctrl+E" of the key sequences reads "Ctrl + E" as in the design
    static const QRegularExpression joint(QStringLiteral("(\\S)\\+(?=\\S)"));
    shortcut->replace(joint, QStringLiteral("\\1 + "));
}

QString withoutMnemonic(QString text)
{
    // "&&" is a literal ampersand, a single one marks the mnemonic
    text.replace(QLatin1String("&&"), QChar(0xFFFF));
    text.remove('&');
    text.replace(QChar(0xFFFF), '&');
    return text;
}

// A line of text as Figma draws it: the outlines of its glyphs filled with
// grey antialiasing, no hinting (drawText takes ClearType's colour fringes
// on Windows whatever the style strategy), the text box of the design
// (1.2109 x the size) centred in the rect, the baseline 0.96875 x the size
// under its top
void drawFigmaText(QPainter *painter, const QRect &rect, Qt::Alignment align,
                   const QString &text, const QFont &font, const QColor &color)
{
    if (text.isEmpty())
        return;
    const qreal size = font.pixelSize() > 0 ? font.pixelSize() : font.pointSizeF() * 96 / 72;
    const qreal box = std::round(1.2109375 * size);
    const qreal baseline = rect.top() + std::round((rect.height() - box) / 2)
                           + (box - 1.2109375 * size) / 2 + 0.96875 * size;
    const qreal width = QFontMetricsF(font).horizontalAdvance(text);
    const qreal x = (align & Qt::AlignRight) ? rect.left() + rect.width() - width : rect.left();
    QPainterPath path;
    path.addText(QPointF(x, baseline), font, text);
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(Qt::NoPen);
    painter->fillPath(path, color);
    painter->restore();
}

// The icon drawn at that size for the painter's device (its SVG when it has
// one: sharp at any scale), in one colour
QPixmap tinted(const QString &path, const QColor &color, const QSize &size, const QPainter *painter)
{
    const qreal ratio = painter && painter->device() ? painter->device()->devicePixelRatioF() : qApp->devicePixelRatio();
    return RedesignIcons::pixmap(path, size, ratio, color);
}

// The interface the menus belong to, for the menus built without it
QPointer<MainCtx> s_ctx;

// Set on a menu once it has this style: in a window with a style sheet the
// menu style is the style sheet style wrapping this one, never this one
const char styledProperty[] = "_vlc_redesignMenuStyle";

} // anonymous namespace

RedesignMenuStyle::RedesignMenuStyle(MainCtx *ctx)
    : QProxyStyle()
    , m_ctx(ctx)
{
    s_ctx = ctx;
    m_fontFamily = fontFamily();

    // the frame of the windows shown follows the theme
    const auto reframe = [this]() {
        const bool dark = isDark();
        const QWindowList windows = QGuiApplication::topLevelWindows();
        for (QWindow *window : windows)
        {
            if (window->handle() && window->isVisible())
                setWindowBorder(window, dark);
        }
    };
    if (ctx)
        connect(ctx->getColorScheme(), &ColorSchemeModel::currentChanged, this, reframe);
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this, reframe);
}

// Windows 11 draws a border around a menu window (DWMWA_BORDER_COLOR)
static bool systemDrawsMenuFrame()
{
#ifdef _WIN32
    static const bool drawn = QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows11;
    return drawn;
#else
    return false;
#endif
}

QColor RedesignMenuStyle::windowBorder(bool dark)
{
    // What Windows 11 draws around the main window (its accent #4C4A48,
    // "Show accent color on title bars and window borders"), set on every
    // window so that the frame is the same on every system, in both themes
    return dark ? QColor(0x4C, 0x4A, 0x48) : QColor(0x4C, 0x4A, 0x48);
}

void RedesignMenuStyle::setWindowBorder(QWindow *window, bool dark)
{
#ifdef _WIN32
    if (!window || !window->handle())
        return;
    const QColor border = windowBorder(dark);
    const COLORREF colour = RGB(border.red(), border.green(), border.blue());
    // older systems ignore it
    DwmSetWindowAttribute(reinterpret_cast<HWND>(window->winId()), 34 /* DWMWA_BORDER_COLOR */,
                          &colour, sizeof(colour));
#else
    (void)window;
    (void)dark;
#endif
}

QString RedesignMenuStyle::fontFamily()
{
    static const QString family = []() {
        const int id = QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/InterVariable.ttf"));
        const QStringList families = QFontDatabase::applicationFontFamilies(id);
        return families.isEmpty() ? QString() : families.first();
    }();
    return family;
}

QColor RedesignMenuStyle::accentColor(MainCtx *ctx)
{
    if (!ctx)
        ctx = s_ctx;
    // a color, or "system": the one of the system as RedesignAccent reads it
    return RedesignAccent::resolve(ctx ? ctx->settingValue(QStringLiteral("redesign/accent"), QStringLiteral("#D37918")).toString()
                                       : QStringLiteral("#D37918"));
}

QColor RedesignMenuStyle::accentDim(const QColor &accent)
{
    if (accent == QColor(0xD3, 0x79, 0x18))
        return QColor(0xAB, 0x63, 0x14);
    return QColor::fromRgbF(accent.redF() * 0.81f, accent.greenF() * 0.81f, accent.blueF() * 0.81f);
}

QIcon RedesignMenuStyle::menuIcon(const QString &classicPath, const QWidget *widget)
{
    static const QHash<QString, QString> duo = {
        { QStringLiteral(":/menu/add.svg"), QStringLiteral("add") },
        { QStringLiteral(":/menu/capture-card.svg"), QStringLiteral("capture-device") },
        { QStringLiteral(":/menu/clear.svg"), QStringLiteral("remove") },
        { QStringLiteral(":/menu/disc.svg"), QStringLiteral("disc") },
        { QStringLiteral(":/menu/exit.svg"), QStringLiteral("quit") },
        { QStringLiteral(":/menu/file.svg"), QStringLiteral("file") },
        { QStringLiteral(":/menu/folder.svg"), QStringLiteral("folder") },
        { QStringLiteral(":/menu/help.svg"), QStringLiteral("help") },
        { QStringLiteral(":/menu/ic_fluent_arrow_shuffle.svg"), QStringLiteral("shuffle") },
        { QStringLiteral(":/menu/ic_fluent_fast_forward.svg"), QStringLiteral("faster") },
        { QStringLiteral(":/menu/ic_fluent_next.svg"), QStringLiteral("next") },
        { QStringLiteral(":/menu/ic_fluent_options.svg"), QStringLiteral("spectrum") },
        { QStringLiteral(":/menu/ic_fluent_play_filled.svg"), QStringLiteral("play") },
        { QStringLiteral(":/menu/ic_fluent_previous.svg"), QStringLiteral("previous") },
        { QStringLiteral(":/menu/ic_fluent_record.svg"), QStringLiteral("record") },
        { QStringLiteral(":/menu/ic_fluent_rewind.svg"), QStringLiteral("slower") },
        { QStringLiteral(":/menu/ic_fluent_skip_back_10.svg"), QStringLiteral("backward-10") },
        { QStringLiteral(":/menu/ic_fluent_skip_forward_10.svg"), QStringLiteral("forward-10") },
        { QStringLiteral(":/menu/ic_fluent_stop.svg"), QStringLiteral("stop") },
        { QStringLiteral(":/menu/ic_pause_filled.svg"), QStringLiteral("pause") },
        { QStringLiteral(":/menu/ic_playlist.svg"), QStringLiteral("playlist") },
        { QStringLiteral(":/menu/info.svg"), QStringLiteral("info") },
        { QStringLiteral(":/menu/messages.svg"), QStringLiteral("lists") },
        { QStringLiteral(":/menu/network.svg"), QStringLiteral("streaming") },
        { QStringLiteral(":/menu/preferences.svg"), QStringLiteral("settings") },
        { QStringLiteral(":/menu/profile_new.svg"), QStringLiteral("add") },
        { QStringLiteral(":/menu/remove.svg"), QStringLiteral("remove") },
        { QStringLiteral(":/menu/search_clear.svg"), QStringLiteral("remove") },
        { QStringLiteral(":/menu/stream.svg"), QStringLiteral("streaming") },
        { QStringLiteral(":/menu/sync.svg"), QStringLiteral("sync") },
        { QStringLiteral(":/menu/update.svg"), QStringLiteral("sync") },
        { QStringLiteral(":/menu/volume-high.svg"), QStringLiteral("volume-high") },
        { QStringLiteral(":/menu/volume-low.svg"), QStringLiteral("volume-low") },
        { QStringLiteral(":/menu/volume-muted.svg"), QStringLiteral("volume-muted") },
    };
    const auto it = duo.constFind(classicPath);
    if (it != duo.cend())
        return accentIcon(nullptr, *it);
    return widget ? ColorizedSvgIcon::colorizedIconForWidget(classicPath, widget) : QIcon(classicPath);
}

QIcon RedesignMenuStyle::accentIcon(MainCtx *ctx, const QString &name)
{
    if (!ctx)
        ctx = s_ctx;
    const QColor accent = accentColor(ctx);
    // Two-tone icon of the design (grey outline, details in Accent Dim, the
    // full accent where the icon has it: #00FFFF, video)
    const QString duo = QStringLiteral(":/redesign/duo/%1.svg").arg(name);
    if (QFile::exists(duo))
        return ColorizedSvgIcon(duo, darkScheme(ctx) ? QColor(0x8F, 0x8F, 0x8F) : QColor(0x2E, 0x2E, 0x2E),
                                accent, accentDim(accent));
    // drawn from its SVG at each size asked (sharp at any scale)
    QString file = QStringLiteral(":/redesign/icons/%1/default.png").arg(name);
    // Some icons of the design only have their active state
    if (!QFile::exists(file))
        file = QStringLiteral(":/redesign/icons/%1/active.png").arg(name);
    return RedesignIcons::icon(file, accent);
}

bool RedesignMenuStyle::isDark() const
{
    return darkScheme(m_ctx);
}

bool RedesignMenuStyle::darkScheme(MainCtx *ctx)
{
    // (a window that knows no interface: the one of the menus)
    if (!ctx)
        ctx = s_ctx;
    if (ctx && ctx->getColorScheme())
    {
        switch (ctx->getColorScheme()->currentScheme())
        {
        case ColorSchemeModel::Day:
            return false;
        case ColorSchemeModel::Night:
            return true;
        default:
            break;
        }
    }
    switch (QGuiApplication::styleHints()->colorScheme())
    {
    case Qt::ColorScheme::Light:
        return false;
    case Qt::ColorScheme::Dark:
        return true;
    default:
        // A desktop that tells no scheme (Linux without a portal setting,
        // X11 window managers): as the interface's own System theme
        // (SystemPalette), from the colours of the application palette;
        // the menus and dialogs were dark over a light interface
        {
            const QPalette palette = QGuiApplication::palette();
            return palette.color(QPalette::Normal, QPalette::Base).lightness()
                   < palette.color(QPalette::Normal, QPalette::Text).lightness();
        }
    }
}

RedesignMenuStyle::Palette RedesignMenuStyle::palette() const
{
    const QColor accent = accentColor(m_ctx);

    // Dark values of the design, light ones from VLCStyle.redesign.color()
    if (isDark())
        return { QColor(0x19, 0x19, 0x19), QColor(0x8F, 0x8F, 0x8F), QColor(0x8F, 0x8F, 0x8F),
                 QColor(0x6B, 0x6B, 0x6B), QColor(0x1F, 0x1F, 0x1F), QColor(0x40, 0x40, 0x40),
                 QColor(0x22, 0x22, 0x22), accent };
    return { QColor(0xAB, 0xAB, 0xAA), QColor(0x2E, 0x2E, 0x2E), QColor(0x2E, 0x2E, 0x2E),
             QColor(0x6B, 0x6B, 0x6B), QColor(0x9C, 0x9C, 0x9B), QColor(0x92, 0x92, 0x91),
             QColor(0x9C, 0x9C, 0x9B), accent };
}

QFont RedesignMenuStyle::menuFont() const
{
    QFont font = QApplication::font();
    if (!m_fontFamily.isEmpty())
        font.setFamily(m_fontFamily);
    font.setPixelSize(fontPixelSize);
    // grey antialiasing without hinting, as Figma (and the QML text) draws
    font.setStyleStrategy(QFont::StyleStrategy(QFont::PreferAntialias | QFont::NoSubpixelAntialias));
    font.setHintingPreference(QFont::PreferNoHinting);
    return font;
}

int RedesignMenuStyle::pixelMetric(PixelMetric metric, const QStyleOption *option,
                                   const QWidget *widget) const
{
    if (isMenu(widget))
    {
        switch (metric)
        {
        case PM_SmallIconSize:
            return iconSize;
        case PM_MenuPanelWidth:
            // the frame of the main window: Windows 11 draws it
            // (eventFilter), elsewhere PE_FrameMenu
            return systemDrawsMenuFrame() ? 0 : 1;
        case PM_MenuHMargin:
            return 0;
        case PM_MenuVMargin:
            return panelPadding;
        default:
            break;
        }
    }
    return QProxyStyle::pixelMetric(metric, option, widget);
}

QSize RedesignMenuStyle::sizeFromContents(ContentsType type, const QStyleOption *option,
                                          const QSize &size, const QWidget *widget) const
{
    // no margin around the rows (the native style's one made each row 1 px
    // narrower than the menu: the shortcuts and arrows 1 px off)
    if (type == CT_Menu && isMenu(widget))
        return size;

    const auto *item = qstyleoption_cast<const QStyleOptionMenuItem *>(option);
    if (type != CT_MenuItem || !item || !isMenu(widget))
        return QProxyStyle::sizeFromContents(type, option, size, widget);

    if (item->menuItemType == QStyleOptionMenuItem::Separator)
        return QSize(size.width(), separatorHeight);

    QString label, shortcut;
    splitText(item->text, &label, &shortcut);
    const QFontMetrics metrics(item->font);
    int width = 2 * panelPadding + sidePadding + iconSize + iconGap
                + metrics.horizontalAdvance(withoutMnemonic(label)) + sidePadding;
    // QMenu adds the width of the shortcut column itself; the arrow of a
    // submenu takes the place of a shortcut
    if (item->menuItemType == QStyleOptionMenuItem::SubMenu)
        width += arrowGap + arrowWidth;
    else if (item->reservedShortcutWidth > 0 || !shortcut.isEmpty())
        width += shortcutGap;
    // the minimum width of the menu (eventFilter) widens the rows
    return QSize(width, rowHeight);
}

void RedesignMenuStyle::drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                                      QPainter *painter, const QWidget *widget) const
{
    if (isMenu(widget))
    {
        if (element == PE_PanelMenu)
        {
            painter->fillRect(option->rect, palette().surface);
            return;
        }
        if (element == PE_FrameMenu)
        {
            // no system frame around a menu (Linux, macOS, Windows 10):
            // the main window's, over the rows' edges
            if (!systemDrawsMenuFrame())
            {
                painter->save();
                painter->setPen(QPen(windowBorder(isDark()), 1));
                painter->setBrush(Qt::NoBrush);
                painter->drawRect(option->rect.adjusted(0, 0, -1, -1));
                painter->restore();
            }
            return;
        }
    }
    QProxyStyle::drawPrimitive(element, option, painter, widget);
}

void RedesignMenuStyle::drawControl(ControlElement element, const QStyleOption *option,
                                    QPainter *painter, const QWidget *widget) const
{
    if (!isMenu(widget))
        return QProxyStyle::drawControl(element, option, painter, widget);

    if (element == CE_MenuEmptyArea)
    {
        painter->fillRect(option->rect, palette().surface);
        return;
    }

    const auto *item = qstyleoption_cast<const QStyleOptionMenuItem *>(option);
    if (element != CE_MenuItem || !item)
        return QProxyStyle::drawControl(element, option, painter, widget);

    const Palette colors = palette();
    const QRect rect = item->rect;
    painter->save();
    painter->fillRect(rect, colors.surface);

    if (item->menuItemType == QStyleOptionMenuItem::Separator)
    {
        painter->fillRect(QRect(rect.left(), rect.top(), rect.width(), separatorHeight), colors.separator);
        painter->restore();
        return;
    }

    const bool enabled = item->state & State_Enabled;
    const bool selected = enabled && (item->state & State_Selected);
    const bool checked = item->checkType != QStyleOptionMenuItem::NotCheckable && item->checked;
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);

    if (selected)
    {
        painter->setPen(Qt::NoPen);
        painter->setBrush(colors.hover);
        painter->drawRoundedRect(rect.adjusted(panelPadding, 0, -panelPadding, 0), hoverRadius, hoverRadius);
    }

    // Laid out left to right, then mirrored right to left (Hebrew, Arabic...):
    // the icon on the right, the label right-aligned, the shortcut on the
    // left and the arrow pointing left, on the side the submenu opens
    const Qt::LayoutDirection dir = item->direction;

    // Icon, or the check mark of the checkable items without one
    const QRect iconLogical(rect.left() + panelPadding + sidePadding, rect.top() + (rect.height() - iconSize) / 2,
                            iconSize, iconSize);
    const QRect iconRect = visualRect(dir, rect, iconLogical);
    if (!item->icon.isNull())
    {
        if (checked)
        {
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(colors.accent.red(), colors.accent.green(), colors.accent.blue(), 0x40));
            painter->drawRoundedRect(iconRect.adjusted(-3, -3, 3, 3), hoverRadius, hoverRadius);
        }
        item->icon.paint(painter, iconRect, Qt::AlignCenter, enabled ? QIcon::Normal : QIcon::Disabled);
    }
    else if (checked)
    {
        // icons/check is empty: the check of the track lists
        painter->drawPixmap(iconRect, tinted(QStringLiteral(":/redesign/track-check-active.png"),
                                             enabled ? colors.accent : colors.disabled, iconRect.size(), painter));
    }

    // Label on the left, shortcut on the right, then the submenu arrow
    const bool subMenu = item->menuItemType == QStyleOptionMenuItem::SubMenu;
    const int left = iconLogical.right() + 1 + iconGap;
    const int right = rect.right() + 1 - panelPadding - sidePadding - (subMenu ? arrowGap + arrowWidth : 0);
    QRect textRect(left, rect.top(), std::max(0, right - left), rect.height());

    QString label, shortcut;
    splitText(item->text, &label, &shortcut);
    painter->setFont(item->font);
    const QFontMetrics metrics(item->font);
    // The label is drawn without its mnemonic (a literal "&" stays:
    // "Properties & Metadata")
    if (!shortcut.isEmpty())
    {
        drawFigmaText(painter, visualRect(dir, rect, textRect), visualAlignment(dir, Qt::AlignRight), shortcut,
                      item->font, enabled ? colors.textMuted : colors.disabled);
        textRect.setRight(textRect.right() - metrics.horizontalAdvance(shortcut) - shortcutGap / 2);
    }
    drawFigmaText(painter, visualRect(dir, rect, textRect), visualAlignment(dir, Qt::AlignLeft),
                  metrics.elidedText(withoutMnemonic(label), Qt::ElideRight, textRect.width()), item->font,
                  !enabled ? colors.disabled : selected ? colors.textHover : colors.text);

    if (subMenu)
    {
        const QRect arrowRect = visualRect(dir, rect,
            QRect(rect.right() + 1 - panelPadding - sidePadding - arrowWidth,
                  rect.top() + (rect.height() - arrowWidth) / 2, arrowWidth, arrowWidth));
        // the colour of the labels (#8F8F8F), not of the shortcuts
        QPixmap arrow = tinted(QStringLiteral(":/redesign/icons/chevron-right/default.png"),
                               enabled ? colors.text : colors.disabled, arrowRect.size(), painter);
        if (dir == Qt::RightToLeft)
        {
            const qreal ratio = arrow.devicePixelRatio();
            arrow = arrow.transformed(QTransform().scale(-1, 1));
            arrow.setDevicePixelRatio(ratio);
        }
        painter->drawPixmap(arrowRect, arrow);
    }
    painter->restore();
}

int RedesignMenuStyle::styleHint(StyleHint hint, const QStyleOption *option,
                                 const QWidget *widget, QStyleHintReturn *returnData) const
{
    if (isMenu(widget))
    {
        switch (hint)
        {
        case SH_Menu_FlashTriggeredItem:
        case SH_UnderlineShortcut:
            return 0;
        default:
            break;
        }
    }
    return QProxyStyle::styleHint(hint, option, widget, returnData);
}

bool RedesignMenuStyle::eventFilter(QObject *watched, QEvent *event)
{
    const QEvent::Type type = event->type();
    if (type != QEvent::Polish && type != QEvent::Show)
        return false;

    // every window shown (the main window, the detached playlist, the
    // classic dialogs and the menus) gets the frame of the main window
    if (type == QEvent::Show && watched->isWindowType())
    {
        auto *window = static_cast<QWindow *>(watched);
        if (!window->parent())
            setWindowBorder(window, isDark());
        return false;
    }

    auto *menu = qobject_cast<QMenu *>(watched);
    if (!menu)
        return false;

    // In a window with a style sheet (the classic dialogs), the sheet draws
    // the menu: this style shared by every menu would be wrapped by the
    // sheet's style of that window, which takes it as its base and leaves
    // it pointing to a freed style when it goes (a crash, opening a dialog)
    for (const QWidget *parent = menu->parentWidget(); parent; parent = parent->parentWidget())
    {
        if (!parent->styleSheet().isEmpty())
            return false;
    }

    if (menu->style() != this && !menu->property(styledProperty).toBool())
    {
        menu->setProperty(styledProperty, true);
        menu->setStyle(this);
        menu->setFont(menuFont());
        menu->setMinimumWidth(std::max(menu->minimumWidth(), minimumWidth + 2 * panelPadding));
    }

#ifdef _WIN32
    if (type == QEvent::Show && menu->isWindow())
    {
        // Windows 11 rounds the menu and keeps its shadow; older systems
        // ignore these attributes
        const HWND hwnd = reinterpret_cast<HWND>(menu->winId());
        const DWORD corner = 2; // DWMWCP_ROUND
        DwmSetWindowAttribute(hwnd, 33 /* DWMWA_WINDOW_CORNER_PREFERENCE */, &corner, sizeof(corner));
        const QColor border = windowBorder(isDark());
        const COLORREF color = RGB(border.red(), border.green(), border.blue());
        DwmSetWindowAttribute(hwnd, 34 /* DWMWA_BORDER_COLOR */, &color, sizeof(color));
    }
#endif
    return false;
}
