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

#include "redesign_widget_theme.hpp"

#include "maininterface/mainctx.hpp"
#include "menus/redesign_menu_style.hpp"
#include "dialogs/toolbar/controlbar_profile_model.hpp"
#include "util/color_scheme_model.hpp"
#include "util/redesign_window_frame.hpp"

#include <QColor>
#include <QEvent>
#include <QFont>
#include <QGuiApplication>
#include <QPushButton>
#include <QHash>
#include <QList>
#include <QMenu>
#include <QPalette>
#include <QPointer>
#include <QRegularExpression>
#include <QString>
#include <QStyleHints>
#include <QVariant>
#include <QWidget>

#include <cassert>
#include <cmath>

namespace {

// The design's sizes in px: $(dp:N) is N px at the interface scale, $(name)
// a colour of Colors ($(theme) is dark or light, for the arrows and checks
// of pixmaps/redesign/widgets)
const char styleSheetSource[] = R"qss(
QWidget {
    color: $(text);
    font-family: "$(family)";
}
QWidget:disabled {
    color: $(disabled);
}
*[vlcRedesignWindow="true"] {
    background: $(window);
}
QToolTip {
    background: $(raised);
    color: $(text);
    border: 1px solid $(separator);
    padding: $(dp:4)px $(dp:8)px;
    font-size: $(dp:13)px;
}
QMenu::separator {
    height: 1px;
    margin: 0px;
    background: $(separator);
}

QGroupBox {
    border: 1px solid $(separator);
    border-radius: $(dp:6)px;
    margin-top: $(dp:22)px;
    padding: $(dp:4)px;
}
QGroupBox::title {
    subcontrol-origin: margin;
    left: $(dp:4)px;
    padding: 0px $(dp:2)px;
    color: $(text);
}
QGroupBox::title:disabled {
    color: $(disabled);
}

QTabWidget::pane {
    background: transparent;
    border: 1px solid $(separator);
    border-radius: $(dp:6)px;
    padding: $(dp:2)px;
}
QTabWidget::tab-bar {
    left: 0px;
}
QTabBar {
    qproperty-drawBase: 0;
}
QTabBar::tab {
    background: $(raised);
    color: $(muted);
    border: none;
    border-radius: $(dp:4)px;
    padding: $(dp:6)px $(dp:12)px;
    margin: 0px $(dp:2)px $(dp:6)px 0px;
    font-size: $(dp:14)px;
}
QTabBar::tab:hover {
    background: $(hover);
    color: $(text);
}
QTabBar::tab:selected {
    background: $(accentBackground);
    color: $(onAccent);
}
QTabBar::tab:disabled {
    color: $(disabled);
}
QTabBar QToolButton {
    background: $(window);
    border: none;
    border-radius: 0px;
}

QPushButton {
    background: $(raised);
    color: $(text);
    border: 1px solid $(windowBorder);
    border-radius: $(dp:6)px;
    padding: $(dp:5)px $(dp:12)px;
    font-size: $(dp:14)px;
    font-weight: 600;
}
QPushButton:hover {
    background: $(hover);
}
QPushButton:pressed {
    background: $(field);
}
QPushButton:checked, QPushButton:default {
    background: $(accentBackground);
    border-color: $(accentBackground);
    color: $(onAccent);
}
QPushButton:default:hover {
    background: $(accentHover);
}
QPushButton:focus {
    border-color: $(accent);
}
QPushButton:flat, QPushButton[vlcIconOnly="true"] {
    border-color: transparent;
}
QPushButton:flat {
    background: transparent;
}
QPushButton:flat:focus, QPushButton[vlcIconOnly="true"]:focus {
    border-color: $(accent);
}
QPushButton:disabled {
    background: $(raised);
    color: $(disabled);
}
QPushButton::menu-indicator {
    image: url(:/redesign/widgets/chevron-down-$(theme).svg);
    subcontrol-origin: padding;
    subcontrol-position: center right;
    width: $(dp:12)px;
    height: $(dp:12)px;
}
QDialogButtonBox QPushButton {
    min-width: $(dp:56)px;
    padding: $(dp:6)px $(dp:16)px;
}

QToolButton {
    background: transparent;
    color: $(text);
    border: 1px solid transparent;
    border-radius: $(dp:4)px;
    padding: $(dp:3)px;
}
QToolButton:hover {
    background: $(hover);
}
QToolButton:pressed {
    background: $(field);
}
QToolButton:checked {
    background: $(accentBackground);
    color: $(onAccent);
}
QToolButton:focus {
    border-color: $(accent);
}
QToolButton:disabled {
    color: $(disabled);
}
QToolButton#playButton {
    background: $(accentBackground);
    color: $(onAccent);
    border-radius: $(dp:6)px;
    padding: $(dp:5)px $(dp:30)px $(dp:5)px $(dp:14)px;
    font-size: $(dp:14)px;
    font-weight: 600;
}
QToolButton#playButton:hover {
    background: $(accentHover);
}
QToolButton::menu-button {
    border: none;
    border-left: 1px solid $(separator);
    width: $(dp:20)px;
}
QToolButton::menu-arrow, QToolButton::menu-indicator {
    image: url(:/redesign/widgets/chevron-down-$(theme).svg);
    width: $(dp:12)px;
    height: $(dp:12)px;
}
QToolButton::menu-arrow:disabled, QToolButton::menu-indicator:disabled {
    image: url(:/redesign/widgets/chevron-down-disabled-$(theme).svg);
}
QToolButton::menu-indicator {
    subcontrol-origin: padding;
    subcontrol-position: bottom right;
}

QLineEdit, QAbstractSpinBox, QTextEdit, QPlainTextEdit {
    background: $(field);
    color: $(text);
    border: $(dp:2)px solid $(raised);
    border-radius: $(dp:6)px;
    selection-background-color: $(accentBackground);
    selection-color: $(onAccent);
    font-size: $(dp:14)px;
}
QLineEdit, QAbstractSpinBox {
    padding: $(dp:3)px $(dp:8)px;
}
QTextEdit, QPlainTextEdit {
    padding: $(dp:2)px;
}
QLineEdit:focus, QAbstractSpinBox:focus, QTextEdit:focus, QPlainTextEdit:focus {
    border-color: $(accentDim);
}
QLineEdit:disabled, QAbstractSpinBox:disabled, QTextEdit:disabled, QPlainTextEdit:disabled {
    color: $(disabled);
}
QAbstractSpinBox QLineEdit, QComboBox QLineEdit,
QAbstractSpinBox QLineEdit:focus, QComboBox QLineEdit:focus {
    background: transparent;
    border: none;
    border-radius: 0px;
    padding: 0px;
}
QAbstractSpinBox::up-button, QAbstractSpinBox::down-button {
    subcontrol-origin: border;
    width: $(dp:18)px;
    border: none;
    background: transparent;
}
QAbstractSpinBox::up-button {
    subcontrol-position: top right;
}
QAbstractSpinBox::down-button {
    subcontrol-position: bottom right;
}
QAbstractSpinBox::up-button:hover, QAbstractSpinBox::down-button:hover {
    background: $(hover);
}
QAbstractSpinBox::up-arrow {
    image: url(:/redesign/widgets/chevron-up-$(theme).svg);
    width: $(dp:10)px;
    height: $(dp:10)px;
}
QAbstractSpinBox::down-arrow {
    image: url(:/redesign/widgets/chevron-down-$(theme).svg);
    width: $(dp:10)px;
    height: $(dp:10)px;
}
QAbstractSpinBox::up-arrow:disabled, QAbstractSpinBox::up-arrow:off {
    image: url(:/redesign/widgets/chevron-up-disabled-$(theme).svg);
}
QAbstractSpinBox::down-arrow:disabled, QAbstractSpinBox::down-arrow:off {
    image: url(:/redesign/widgets/chevron-down-disabled-$(theme).svg);
}

QComboBox {
    background: $(raised);
    color: $(text);
    border: $(dp:2)px solid $(field);
    border-radius: $(dp:6)px;
    padding: $(dp:3)px $(dp:4)px $(dp:3)px $(dp:12)px;
    selection-background-color: $(accentBackground);
    selection-color: $(onAccent);
    font-size: $(dp:14)px;
}
QComboBox:hover, QComboBox:on {
    background: $(hover);
}
QComboBox:editable {
    background: $(field);
    border-color: $(raised);
}
QComboBox:focus {
    border-color: $(accentDim);
}
QComboBox:disabled {
    color: $(disabled);
}
QComboBox::drop-down {
    subcontrol-origin: padding;
    subcontrol-position: center right;
    width: $(dp:24)px;
    border: none;
    background: transparent;
}
QComboBox::down-arrow {
    image: url(:/redesign/widgets/chevron-down-$(theme).svg);
    width: $(dp:16)px;
    height: $(dp:16)px;
}
QComboBox::down-arrow:disabled {
    image: url(:/redesign/widgets/chevron-down-disabled-$(theme).svg);
}
QComboBox QAbstractItemView {
    background: $(raised);
    border: 1px solid $(separator);
    border-radius: 0px;
    padding: $(dp:2)px;
    selection-background-color: $(hover);
    selection-color: $(onAccent);
}

QAbstractItemView {
    background: $(field);
    alternate-background-color: $(window);
    color: $(text);
    border: 1px solid $(separator);
    border-radius: $(dp:6)px;
    padding: $(dp:2)px;
    selection-background-color: $(accentBackground);
    selection-color: $(onAccent);
    outline: 0px;
    font-size: $(dp:14)px;
}
QAbstractItemView::item {
    padding: $(dp:2)px $(dp:4)px;
}
QAbstractItemView::item:hover {
    background: $(raised);
}
QAbstractItemView::item:selected {
    background: $(accentBackground);
    color: $(onAccent);
}
QTreeView {
    show-decoration-selected: 1;
}
QTreeView::branch {
    background: transparent;
}
QTreeView::branch:selected {
    background: $(accentBackground);
}
QTreeView::branch:has-children:!has-siblings:closed,
QTreeView::branch:closed:has-children:has-siblings {
    image: url(:/redesign/widgets/chevron-right-$(theme).svg);
}
QTreeView::branch:open:has-children:!has-siblings,
QTreeView::branch:open:has-children:has-siblings {
    image: url(:/redesign/widgets/chevron-down-$(theme).svg);
}
QHeaderView {
    background: transparent;
    border: none;
}
QHeaderView::section {
    background: $(raised);
    color: $(muted);
    border: none;
    border-right: 1px solid $(separator);
    border-bottom: 1px solid $(separator);
    padding: $(dp:4)px $(dp:18)px $(dp:4)px $(dp:8)px;
    font-size: $(dp:13)px;
}
QHeaderView::section:hover {
    background: $(hover);
}
QHeaderView::up-arrow, QHeaderView::down-arrow {
    subcontrol-origin: padding;
    subcontrol-position: center right;
    width: $(dp:12)px;
    height: $(dp:12)px;
}
QHeaderView::up-arrow {
    image: url(:/redesign/widgets/chevron-up-$(theme).svg);
}
QHeaderView::down-arrow {
    image: url(:/redesign/widgets/chevron-down-$(theme).svg);
}
QTableCornerButton::section {
    background: $(raised);
    border: none;
}
QGraphicsView {
    background: $(field);
    border: 1px solid $(separator);
    border-radius: $(dp:6)px;
    padding: $(dp:2)px;
}
QScrollArea {
    background: transparent;
    border: none;
}

QScrollBar:vertical {
    background: transparent;
    width: $(dp:10)px;
    margin: 0px;
    border: none;
}
QScrollBar:horizontal {
    background: transparent;
    height: $(dp:10)px;
    margin: 0px;
    border: none;
}
QScrollBar::handle {
    background: $(handle);
    border-radius: $(dp:3)px;
}
QScrollBar::handle:vertical {
    min-height: $(dp:32)px;
    margin: $(dp:2)px;
}
QScrollBar::handle:horizontal {
    min-width: $(dp:32)px;
    margin: $(dp:2)px;
}
QScrollBar::handle:hover, QScrollBar::handle:pressed {
    background: $(handleHover);
}
QScrollBar::add-line, QScrollBar::sub-line {
    width: 0px;
    height: 0px;
    border: none;
    background: none;
}
QScrollBar::add-page, QScrollBar::sub-page {
    background: none;
}
QAbstractScrollArea::corner {
    background: transparent;
    border: none;
}

QSlider::groove:horizontal {
    height: $(dp:6)px;
    background: $(field);
    border: 1px solid $(raised);
    border-radius: $(dp:3)px;
}
QSlider::groove:vertical {
    width: $(dp:6)px;
    background: $(field);
    border: 1px solid $(raised);
    border-radius: $(dp:3)px;
}
QSlider::sub-page:horizontal, QSlider::add-page:vertical {
    background: $(accentBackground);
    border-radius: $(dp:3)px;
}
QSlider::handle:horizontal {
    background: $(accentDim);
    width: $(dp:8)px;
    margin: -$(dp:5)px 0px;
    border-radius: $(dp:3)px;
}
QSlider::handle:vertical {
    background: $(accentDim);
    height: $(dp:8)px;
    margin: 0px -$(dp:5)px;
    border-radius: $(dp:3)px;
}
QSlider::handle:hover {
    background: $(accent);
}
QSlider::handle:disabled {
    background: $(disabled);
}
QSlider::sub-page:horizontal:disabled, QSlider::add-page:vertical:disabled {
    background: $(raised);
}

QProgressBar {
    background: $(field);
    color: $(text);
    border: none;
    border-radius: $(dp:4)px;
    text-align: center;
}
QProgressBar::chunk {
    background: $(accentDim);
    border-radius: $(dp:4)px;
}

QCheckBox, QRadioButton {
    spacing: $(dp:8)px;
}
QCheckBox::indicator, QGroupBox::indicator, QAbstractItemView::indicator, QRadioButton::indicator {
    width: $(dp:14)px;
    height: $(dp:14)px;
    background: $(field);
    border: 1px solid $(outline);
    border-radius: $(dp:4)px;
}
QRadioButton::indicator {
    border-radius: $(dp:8)px;
}
QCheckBox::indicator:hover, QGroupBox::indicator:hover, QAbstractItemView::indicator:hover, QRadioButton::indicator:hover {
    border-color: $(accentDim);
}
QCheckBox::indicator:checked, QGroupBox::indicator:checked, QAbstractItemView::indicator:checked {
    background: $(accent);
    border-color: $(accent);
    image: url(:/redesign/widgets/check.svg);
}
QCheckBox::indicator:indeterminate, QAbstractItemView::indicator:indeterminate {
    background: $(accentBackground);
    border-color: $(accent);
}
QRadioButton::indicator:checked {
    border-color: $(accent);
    background: qradialgradient(cx: 0.5, cy: 0.5, radius: 0.5, fx: 0.5, fy: 0.5, stop: 0 $(accent), stop: 0.45 $(accent), stop: 0.55 $(field), stop: 1 $(field));
}
QCheckBox::indicator:disabled, QGroupBox::indicator:disabled, QAbstractItemView::indicator:disabled, QRadioButton::indicator:disabled {
    border-color: $(disabled);
}
QCheckBox::indicator:checked:disabled, QGroupBox::indicator:checked:disabled, QAbstractItemView::indicator:checked:disabled {
    background: $(disabled);
}
QRadioButton::indicator:checked:disabled {
    background: qradialgradient(cx: 0.5, cy: 0.5, radius: 0.5, fx: 0.5, fy: 0.5, stop: 0 $(disabled), stop: 0.45 $(disabled), stop: 0.55 $(field), stop: 1 $(field));
}

QFrame[frameShape="4"], QFrame[frameShape="HLine"] {
    border: none;
    background: $(separator);
    max-height: 1px;
}
QFrame[frameShape="5"], QFrame[frameShape="VLine"] {
    border: none;
    background: $(separator);
    max-width: 1px;
}

/* The title bar of the redesign's dialogs (RedesignWindowFrame) */
QWidget#redesignTitleBar {
    background: $(titleBar);
}
QLabel#redesignTitle {
    background: transparent;
    color: $(text);
    font-size: $(dp:14)px;
}
QToolButton#redesignTitleButton {
    background: transparent;
    border: none;
    border-radius: $(dp:4)px;
    padding: $(dp:4)px;
}
QToolButton#redesignTitleButton:hover, QToolButton#redesignTitleButton:pressed {
    background: $(hover);
}
)qss";

// Set on the windows given the look; the second holds what they got
const char windowProperty[] = "vlcRedesignWindow";
const char appliedProperty[] = "_vlc_redesignWidgetTheme";

struct Colors
{
    QColor window, raised, hover, field, separator;
    QColor text, bright, muted, disabled, outline, handle, handleHover;
    QColor accent, accentDim, accentBackground, accentHover;
    // the text on accentBackground; the frame of the window (buttons)
    QColor onAccent, windowBorder;
    QColor titleBar;
};

int dp(int px, double scale)
{
    return static_cast<int>(std::lround(px * scale));
}

QColor mix(const QColor &from, const QColor &to, float amount)
{
    return QColor::fromRgbF(from.redF() + (to.redF() - from.redF()) * amount,
                            from.greenF() + (to.greenF() - from.greenF()) * amount,
                            from.blueF() + (to.blueF() - from.blueF()) * amount);
}

// The tokens of VLCStyle.redesign; in light, the values of Figma "Player -
// Light" (VLCStyle.redesign.color(): #8F8F8F reads #2E2E2E, #6B6B6B and
// #404040 stay), the menus' ones where RedesignMenuStyle has them
Colors colors(bool dark, const QColor &accent)
{
    Colors c;
    if (dark)
    {
        c.window = QColor(0x0E, 0x0E, 0x0E);
        c.raised = QColor(0x19, 0x19, 0x19);
        c.hover = QColor(0x22, 0x22, 0x22);
        c.field = QColor(0x09, 0x09, 0x09);
        c.separator = QColor(0x22, 0x22, 0x22);
        c.text = QColor(0x8F, 0x8F, 0x8F);
        c.bright = QColor(0xEA, 0xEA, 0xEA);
        c.disabled = QColor(0x40, 0x40, 0x40);
        c.handle = QColor(0x40, 0x40, 0x40);
    }
    else
    {
        c.window = QColor(0xC2, 0xC2, 0xC1);
        c.raised = QColor(0xAB, 0xAB, 0xAA);
        c.hover = QColor(0x9C, 0x9C, 0x9B);
        c.field = QColor(0xDE, 0xDE, 0xDE);
        c.separator = QColor(0x9C, 0x9C, 0x9B);
        c.text = QColor(0x2E, 0x2E, 0x2E);
        c.bright = QColor(0x2E, 0x2E, 0x2E);
        c.disabled = QColor(0x92, 0x92, 0x91);
        c.handle = QColor(0x8E, 0x8E, 0x8D);
    }
    c.muted = QColor(0x6B, 0x6B, 0x6B);
    // "Dark/BG/B2" of the dialogs' title bar, as RedesignDialog.qml draws it
    // (#DEDEDE in the light theme, Figma "Player - Light" B2)
    c.titleBar = dark ? QColor(0x09, 0x09, 0x09) : QColor(0xDE, 0xDE, 0xDE);
    c.outline = QColor(0x40, 0x40, 0x40);
    c.handleHover = QColor(0x6B, 0x6B, 0x6B);

    // Accent, Accent Dim and Accent Bright (VLCStyle.redesign.accent*)
    c.accent = accent;
    c.accentDim = RedesignMenuStyle::accentDim(accent);
    if (dark)
        c.accentBackground = accent == QColor(0xD3, 0x79, 0x18)
                             ? QColor(0x37, 0x1F, 0x06)
                             : QColor::fromRgbF(accent.redF() * 0.26f, accent.greenF() * 0.26f,
                                                accent.blueF() * 0.26f);
    else
        c.accentBackground = mix(QColor(0xDE, 0xDE, 0xDE), accent, 0.35f);
    c.accentHover = dark ? c.accentBackground.lighter(140) : c.accentBackground.darker(110);
    // the accent reads 1.8:1 on the light theme's pale tint of it: darkened
    // there to the 4.5:1 of WCAG AA (5.1 to 6.4:1 for the 9 accents)
    c.onAccent = dark ? accent : accent.darker(220);
    // a neutral button (#191919 on the dialogs' #191919 panels, the same grey
    // in the light theme) has the frame of the window around it, so it never
    // melts into its panel (docs/MASTER-PLAN.md §6)
    c.windowBorder = QColor(0x4C, 0x4A, 0x48);
    return c;
}

// For the window itself and what reads its palette (its children follow
// the style sheet: a style sheet stops the palette propagation)
QPalette windowPalette(const Colors &c, bool dark)
{
    QPalette palette;
    palette.setColor(QPalette::Window, c.window);
    palette.setColor(QPalette::WindowText, c.text);
    palette.setColor(QPalette::Base, c.field);
    palette.setColor(QPalette::AlternateBase, c.window);
    palette.setColor(QPalette::Text, c.text);
    palette.setColor(QPalette::PlaceholderText, c.muted);
    palette.setColor(QPalette::Button, c.raised);
    palette.setColor(QPalette::ButtonText, c.text);
    palette.setColor(QPalette::BrightText, c.bright);
    palette.setColor(QPalette::Light, dark ? c.hover : c.field);
    palette.setColor(QPalette::Midlight, c.hover);
    palette.setColor(QPalette::Mid, c.separator);
    palette.setColor(QPalette::Dark, dark ? c.field : c.hover);
    palette.setColor(QPalette::Shadow, dark ? c.field : c.hover);
    palette.setColor(QPalette::Highlight, c.accentBackground);
    palette.setColor(QPalette::HighlightedText, c.accent);
    palette.setColor(QPalette::Link, c.accent);
    palette.setColor(QPalette::LinkVisited, c.accentDim);
    palette.setColor(QPalette::ToolTipBase, c.raised);
    palette.setColor(QPalette::ToolTipText, c.text);
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    palette.setColor(QPalette::Accent, c.accent);
#endif
    palette.setColor(QPalette::Disabled, QPalette::WindowText, c.disabled);
    palette.setColor(QPalette::Disabled, QPalette::Text, c.disabled);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, c.disabled);
    return palette;
}

QString styleSheet(const Colors &c, bool dark, const QString &family, double scale)
{
    const QHash<QString, QString> values = {
        { QStringLiteral("window"), c.window.name() },
        { QStringLiteral("raised"), c.raised.name() },
        { QStringLiteral("hover"), c.hover.name() },
        { QStringLiteral("field"), c.field.name() },
        { QStringLiteral("separator"), c.separator.name() },
        { QStringLiteral("text"), c.text.name() },
        { QStringLiteral("muted"), c.muted.name() },
        { QStringLiteral("disabled"), c.disabled.name() },
        { QStringLiteral("outline"), c.outline.name() },
        { QStringLiteral("handle"), c.handle.name() },
        { QStringLiteral("handleHover"), c.handleHover.name() },
        { QStringLiteral("accent"), c.accent.name() },
        { QStringLiteral("accentDim"), c.accentDim.name() },
        { QStringLiteral("accentBackground"), c.accentBackground.name() },
        { QStringLiteral("accentHover"), c.accentHover.name() },
        { QStringLiteral("onAccent"), c.onAccent.name() },
        { QStringLiteral("windowBorder"), c.windowBorder.name() },
        { QStringLiteral("titleBar"), c.titleBar.name() },
        { QStringLiteral("theme"), dark ? QStringLiteral("dark") : QStringLiteral("light") },
        { QStringLiteral("family"), family },
    };

    static const QRegularExpression token(QStringLiteral("\\$\\((\\w+)(?::(\\d+))?\\)"));
    const QString source = QString::fromLatin1(styleSheetSource);
    QString sheet;
    sheet.reserve(source.size() + source.size() / 8);
    qsizetype last = 0;
    QRegularExpressionMatchIterator it = token.globalMatch(source);
    while (it.hasNext())
    {
        const QRegularExpressionMatch match = it.next();
        sheet += source.mid(last, match.capturedStart() - last);
        const QString name = match.captured(1);
        if (name == QLatin1String("dp"))
            sheet += QString::number(dp(match.captured(2).toInt(), scale));
        else
        {
            assert(values.contains(name));
            sheet += values.value(name);
        }
        last = match.capturedEnd();
    }
    sheet += source.mid(last);
    return sheet;
}

/**
 * Follows the theme, the accent and the scale of one interface for all the
 * windows given the look
 */
class ThemeController : public QObject
{
public:
    explicit ThemeController(MainCtx *ctx);

    void add(QWidget *window);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void refresh();
    void update(QWidget *window);
    void updateAll();

    MainCtx *const m_ctx;
    QList<QPointer<QWidget>> m_windows;
    // what the windows get, for m_key (scheme, accent and scale)
    QString m_key;
    QString m_styleSheet;
    QPalette m_palette;
    QFont m_font;
    // for the title bar and the frame of the system
    bool m_dark = true;
    double m_scale = 1.0;
    QColor m_border;
};

QPointer<ThemeController> s_controller;

ThemeController::ThemeController(MainCtx *ctx)
    : QObject(ctx)
    , m_ctx(ctx)
{
    const auto changed = [this]() { updateAll(); };
    connect(ctx->getColorScheme(), &ColorSchemeModel::currentChanged, this, changed);
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this, changed);
    connect(ctx, &MainCtx::intfScaleFactorChanged, this, changed);
    // "Follow the Windows accent color" follows the system (refresh() sees whether
    // the accent the windows have changed)
    connect(ctx, &MainCtx::systemAccentChanged, this, changed);
    connect(ctx, &MainCtx::settingValueChanged, this, [this](const QString &key) {
        if (key == QLatin1String("redesign/accent"))
            updateAll();
    });
    // a profile brings its own accent
    if (ControlbarProfileModel *profiles = ctx->controlbarProfileModel())
        connect(profiles, &ControlbarProfileModel::selectedProfileChanged, this, changed);
}

void ThemeController::add(QWidget *window)
{
    if (window->property(windowProperty).toBool())
        return;
    window->setProperty(windowProperty, true);
    // a window that is a plain QWidget draws its style sheet background
    // only so
    window->setAttribute(Qt::WA_StyledBackground);
    window->installEventFilter(this);
    // the redesign's title bar instead of the system's
    RedesignWindowFrame::install(window);
    m_windows.removeIf([](const QPointer<QWidget> &other) { return other.isNull(); });
    m_windows.append(window);
    update(window);
}

bool ThemeController::eventFilter(QObject *watched, QEvent *event)
{
    // catches up on what changed while the window was hidden
    if (event->type() == QEvent::Show)
    {
        if (auto *window = qobject_cast<QWidget *>(watched))
            update(window);
    }
    return QObject::eventFilter(watched, event);
}

void ThemeController::refresh()
{
    const bool dark = RedesignMenuStyle::darkScheme(m_ctx);
    const QColor accent = RedesignMenuStyle::accentColor(m_ctx);
    const double scale = m_ctx->getIntfScaleFactor();
    const QString key = QStringLiteral("%1 %2 %3")
                            .arg(dark ? QStringLiteral("dark") : QStringLiteral("light"), accent.name(),
                                 QString::number(scale, 'f', 3));
    if (key == m_key)
        return;
    m_key = key;

    const Colors c = colors(dark, accent);
    m_palette = windowPalette(c, dark);
    m_dark = dark;
    m_scale = scale;
    // the frame of the main window
    m_border = RedesignMenuStyle::windowBorder(dark);

    // Inter 14 px, drawn as the menus and the QML text (grey antialiasing,
    // no hinting); the style sheet sets the size of the controls only, the
    // labels take it from here and keep the sizes the dialogs give them
    m_font = QGuiApplication::font();
    const QString family = RedesignMenuStyle::fontFamily();
    if (!family.isEmpty())
        m_font.setFamily(family);
    m_font.setPixelSize(dp(14, scale));
    m_font.setStyleStrategy(QFont::StyleStrategy(QFont::PreferAntialias | QFont::NoSubpixelAntialias));
    m_font.setHintingPreference(QFont::PreferNoHinting);

    m_styleSheet = styleSheet(c, dark, m_font.family(), scale);
}

void ThemeController::update(QWidget *window)
{
    refresh();
    if (window->property(appliedProperty).toString() == m_key)
        return;
    window->setProperty(appliedProperty, m_key);
    // its menus given the menu style before (shared by every menu) leave
    // it: the sheet draws them, and wrapped by the sheet's style it would
    // be left pointing to a freed style (see RedesignMenuStyle)
    const QList<QMenu *> menus = window->findChildren<QMenu *>();
    for (QMenu *menu : menus)
    {
        if (qobject_cast<RedesignMenuStyle *>(menu->style()))
            menu->setStyle(nullptr);
    }
    // a button of an icon only keeps no frame (the frame is for the text
    // buttons that melt into their panel): marked before the sheet polishes
    // it
    const QList<QPushButton *> buttons = window->findChildren<QPushButton *>();
    for (QPushButton *button : buttons)
        button->setProperty("vlcIconOnly", button->text().isEmpty() && !button->icon().isNull());
    // the palette and the font before the style sheet: polishing the
    // children for it takes them up
    window->setPalette(m_palette);
    window->setFont(m_font);
    window->setStyleSheet(m_styleSheet);
    RedesignWindowFrame::update(window, m_scale, m_dark, m_border);
}

void ThemeController::updateAll()
{
    m_windows.removeIf([](const QPointer<QWidget> &window) { return window.isNull(); });
    const QList<QPointer<QWidget>> windows = m_windows;
    for (const QPointer<QWidget> &window : windows)
    {
        if (window)
            update(window.data());
    }
}

} // anonymous namespace

void RedesignWidgetTheme::apply(QWidget *window, qt_intf_t *p_intf)
{
    assert(window);
    // the menus are drawn by RedesignMenuStyle
    if (qobject_cast<QMenu *>(window))
        return;

    MainCtx *ctx = p_intf ? p_intf->p_mi : nullptr;
    if (!s_controller && ctx)
        s_controller = new ThemeController(ctx);
    // without an interface (dialogs provider), the classic look stays
    if (s_controller)
        s_controller->add(window);
}
