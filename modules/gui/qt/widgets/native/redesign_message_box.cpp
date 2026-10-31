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

#include "redesign_message_box.hpp"

#include "qt.hpp"
#include "maininterface/mainctx.hpp"
#include "maininterface/compositor.hpp"
#include "menus/redesign_menu_style.hpp"
#include "util/redesign_icon_provider.hpp"
#include "util/redesign_version.hpp"

#include <QColor>
#include <QFont>
#include <QGraphicsDropShadowEffect>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPalette>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScreen>
#include <QShowEvent>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWindow>

#include <cmath>

QVariantMap RedesignMessage::toVariantMap() const
{
    QVariantMap map;
    map[QStringLiteral("title")] = title;
    map[QStringLiteral("heading")] = heading;
    map[QStringLiteral("text")] = text;
    map[QStringLiteral("acceptText")] = acceptText;
    map[QStringLiteral("cancelText")] = cancelText;
    map[QStringLiteral("extraText")] = extraText;
    map[QStringLiteral("cancelVisible")] = cancelVisible;
    map[QStringLiteral("defaultAccept")] = defaultAccept;
    map[QStringLiteral("input")] = static_cast<int>(input);
    map[QStringLiteral("inputText")] = inputText;
    map[QStringLiteral("placeholder")] = placeholder;
    return map;
}

namespace {

// The values of RedesignDialog.qml, RedesignDialogHeading.qml and
// RedesignTextArea.qml (dark, and what VLCStyle.redesign.color() gives in
// light)
struct BoxColors
{
    QColor card, titleBar, title, closeHover;
    QColor heading, text;
    QColor button, buttonHover, buttonText;
    QColor accent, accentBackground, accentHover;
    QColor field, fieldBorder, fieldText, placeholder, selection, selectedText;
};

QColor mix(const QColor &from, const QColor &to, float amount)
{
    return QColor::fromRgbF(from.redF() + (to.redF() - from.redF()) * amount,
                            from.greenF() + (to.greenF() - from.greenF()) * amount,
                            from.blueF() + (to.blueF() - from.blueF()) * amount);
}

BoxColors boxColors(bool dark, const QColor &accent)
{
    BoxColors c;
    // "Dark/BG/A3 Transparent" as Figma shows it, "Dark/BG/B2"
    c.card = dark ? QColor(0x19, 0x19, 0x19) : QColor(0xAB, 0xAB, 0xAA);
    c.titleBar = dark ? QColor(0x09, 0x09, 0x09) : QColor(0x9C, 0x9C, 0x9B);
    c.title = dark ? QColor(0x8F, 0x8F, 0x8F) : QColor(0x2E, 0x2E, 0x2E);
    c.closeHover = dark ? QColor(0x22, 0x22, 0x22) : QColor(0x9C, 0x9C, 0x9B);
    // "Dark/FG/2", "Dark/FG/A3"
    c.heading = dark ? QColor(0xC6, 0xC6, 0xC6) : QColor(0x2E, 0x2E, 0x2E);
    c.text = dark ? QColor(0x6B, 0x6B, 0x6B) : QColor(0x4D, 0x4D, 0x4D);
    // "Cancel": #1B1B1B and #8F8F8F
    c.button = dark ? QColor(0x1B, 0x1B, 0x1B) : QColor(0xB8, 0xB8, 0xB7);
    c.buttonHover = dark ? QColor(0x22, 0x22, 0x22) : QColor(0x9C, 0x9C, 0x9B);
    c.buttonText = dark ? QColor(0x8F, 0x8F, 0x8F) : QColor(0x4D, 0x4D, 0x4D);
    // "Save": Accent Bright and the accent (VLCStyle.redesign.accentBackground)
    c.accent = accent;
    if (dark)
        c.accentBackground = accent == QColor(0xD3, 0x79, 0x18)
                             ? QColor(0x37, 0x1F, 0x06)
                             : QColor::fromRgbF(accent.redF() * 0.26f, accent.greenF() * 0.26f,
                                                accent.blueF() * 0.26f);
    else
        c.accentBackground = mix(QColor(0xDE, 0xDE, 0xDE), accent, 0.35f);
    // Qt.lighter(accentBackground, 1.15)
    c.accentHover = c.accentBackground.lighter(115);
    // the field of RedesignTextArea
    c.field = dark ? QColor(0x09, 0x09, 0x09) : QColor(0xDE, 0xDE, 0xDE);
    c.fieldBorder = dark ? QColor(0x19, 0x19, 0x19) : QColor(0xC4, 0xC4, 0xC3);
    c.fieldText = dark ? QColor(0xC6, 0xC6, 0xC6) : QColor(0x2E, 0x2E, 0x2E);
    c.placeholder = dark ? QColor(0x40, 0x40, 0x40) : QColor(0x7C, 0x7C, 0x7B);
    c.selection = dark ? c.accentBackground : QColor(0xDE, 0xDE, 0xDE);
    c.selectedText = dark ? QColor(0xEA, 0xEA, 0xEA) : QColor(0x19, 0x19, 0x19);
    return c;
}

QString rgb(const QColor &color)
{
    return color.name(QColor::HexRgb);
}

QWindow *interfaceWindow(qt_intf_t *p_intf)
{
    if (!p_intf || p_intf->b_isDialogProvider || !p_intf->p_compositor)
        return nullptr;
    return p_intf->p_compositor->interfaceMainWindow();
}

} // anonymous namespace

RedesignMessageBox::RedesignMessageBox(qt_intf_t *intf, const RedesignMessage &message, QWidget *parent)
    : QDialog(parent)
    , p_intf(intf)
    , m_message(message)
{
    MainCtx *ctx = p_intf ? p_intf->p_mi : nullptr;
    const bool dark = RedesignMenuStyle::darkScheme(ctx);
    const BoxColors c = boxColors(dark, RedesignMenuStyle::accentColor(ctx));
    const double scale = ctx ? ctx->getIntfScaleFactor() : 1.0;
    const auto dp = [scale](double px) { return static_cast<int>(std::lround(px * scale)); };

    // drawn by itself: no title bar of the system, the corners rounded and
    // the shadow of the design around the card
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowTitle(message.title.isEmpty() ? QStringLiteral(VLC_REDESIGN_NAME) : message.title);

    QFont font = QGuiApplication::font();
    const QString family = RedesignMenuStyle::fontFamily();
    if (!family.isEmpty())
        font.setFamily(family);
    font.setPixelSize(dp(14));
    font.setStyleStrategy(QFont::StyleStrategy(QFont::PreferAntialias | QFont::NoSubpixelAntialias));
    font.setHintingPreference(QFont::PreferNoHinting);
    setFont(font);

    QPalette pal = palette();
    pal.setColor(QPalette::PlaceholderText, c.placeholder);
    pal.setColor(QPalette::Highlight, c.selection);
    pal.setColor(QPalette::HighlightedText, c.selectedText);
    setPalette(pal);

    // shadow 0 8, blur 15, #000000 46 %: room for it around the card
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(dp(16), dp(8), dp(16), dp(24));
    outer->setSpacing(0);

    auto *card = new QWidget(this);
    card->setObjectName(QStringLiteral("redesignMessageCard"));
    card->setAttribute(Qt::WA_StyledBackground);
    auto *shadow = new QGraphicsDropShadowEffect(card);
    shadow->setOffset(0, dp(8));
    shadow->setBlurRadius(dp(15));
    shadow->setColor(QColor(0, 0, 0, 117));
    card->setGraphicsEffect(shadow);
    outer->addWidget(card);

    auto *column = new QVBoxLayout(card);
    column->setContentsMargins(0, 0, 0, 0);
    column->setSpacing(0);

    // "Frame 120": the title bar, padding 9, the row of 32 (the title with
    // 10 before it, the close button)
    m_titleBar = new QWidget(card);
    m_titleBar->setObjectName(QStringLiteral("redesignMessageTitleBar"));
    m_titleBar->setAttribute(Qt::WA_StyledBackground);
    auto *titleRow = new QHBoxLayout(m_titleBar);
    titleRow->setContentsMargins(dp(9), dp(9), dp(9), dp(9));
    titleRow->setSpacing(0);
    titleRow->addSpacing(dp(10));
    auto *title = new QLabel(message.title, m_titleBar);
    title->setObjectName(QStringLiteral("redesignMessageTitle"));
    title->setTextFormat(Qt::PlainText);
    title->setFixedHeight(dp(32));
    title->setMinimumWidth(0);
    title->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    title->setAlignment(Qt::AlignVCenter | Qt::AlignLeading);
    titleRow->addWidget(title, 1);
    auto *close = new QToolButton(m_titleBar);
    close->setObjectName(QStringLiteral("redesignMessageClose"));
    close->setFixedSize(dp(32), dp(32));
    close->setIcon(RedesignIcons::icon(QStringLiteral(":/redesign/icons/close/default.png")));
    close->setIconSize(QSize(dp(24), dp(24)));
    close->setFocusPolicy(Qt::TabFocus);
    close->setAccessibleName(qtr("Close"));
    close->setAutoRaise(true);
    connect(close, &QToolButton::clicked, this, [this]() { finish(RedesignMessage::Cancelled); });
    titleRow->addWidget(close);
    column->addWidget(m_titleBar);

    // "Frame 124": padding 22 / 20, 20 between its items; the heading
    // ("Frame 324": Inter 16, 5 above the explanation in Inter 14)
    auto *body = new QWidget(card);
    auto *bodyColumn = new QVBoxLayout(body);
    bodyColumn->setContentsMargins(dp(20), dp(22), dp(20), dp(22));
    bodyColumn->setSpacing(dp(20));
    if (!message.heading.isEmpty() || !message.text.isEmpty())
    {
        auto *headingColumn = new QVBoxLayout;
        headingColumn->setSpacing(dp(5));
        if (!message.heading.isEmpty())
        {
            auto *heading = new QLabel(message.heading, body);
            heading->setObjectName(QStringLiteral("redesignMessageHeading"));
            heading->setTextFormat(Qt::PlainText);
            heading->setWordWrap(true);
            heading->setMinimumHeight(dp(28));
            heading->setAlignment(Qt::AlignVCenter | Qt::AlignLeading);
            heading->setTextInteractionFlags(Qt::TextSelectableByMouse);
            headingColumn->addWidget(heading);
        }
        if (!message.text.isEmpty())
        {
            auto *text = new QLabel(message.text, body);
            text->setObjectName(QStringLiteral("redesignMessageText"));
            text->setTextFormat(Qt::PlainText);
            text->setWordWrap(true);
            text->setMinimumHeight(dp(25));
            text->setAlignment(Qt::AlignVCenter | Qt::AlignLeading);
            text->setTextInteractionFlags(Qt::TextSelectableByMouse);
            headingColumn->addWidget(text);
        }
        bodyColumn->addLayout(headingColumn);
    }
    if (message.input == RedesignMessage::LineInput)
    {
        // one line: 37 high
        m_line = new QLineEdit(message.inputText, body);
        m_line->setObjectName(QStringLiteral("redesignMessageField"));
        m_line->setPlaceholderText(message.placeholder);
        m_line->setFixedHeight(dp(37));
        bodyColumn->addWidget(m_line);
    }
    else if (message.input == RedesignMessage::TextInput)
    {
        // several lines: 194 high
        m_multiLine = new QPlainTextEdit(body);
        m_multiLine->setObjectName(QStringLiteral("redesignMessageField"));
        m_multiLine->setPlainText(message.inputText);
        m_multiLine->setPlaceholderText(message.placeholder);
        m_multiLine->setFixedHeight(dp(194));
        m_multiLine->setTabChangesFocus(true);
        bodyColumn->addWidget(m_multiLine);
    }
    column->addWidget(body, 1);

    // "Frame 225": 72 high, padding 20; Cancel on the left, the action on
    // the right (100 x 32)
    auto *footer = new QWidget(card);
    footer->setFixedHeight(dp(72));
    auto *buttons = new QHBoxLayout(footer);
    buttons->setContentsMargins(dp(20), dp(20), dp(20), dp(20));
    buttons->setSpacing(dp(8));
    const auto makeButton = [&](const QString &text, bool accent, int answer) {
        auto *button = new QPushButton(text, footer);
        button->setObjectName(accent ? QStringLiteral("redesignMessageAccent")
                                     : QStringLiteral("redesignMessageButton"));
        button->setFixedHeight(dp(32));
        button->setMinimumWidth(dp(100));
        button->setAutoDefault(false);
        button->setDefault(false);
        button->setFocusPolicy(Qt::TabFocus);
        button->setCursor(Qt::ArrowCursor);
        connect(button, &QPushButton::clicked, this, [this, answer]() { finish(answer); });
        return button;
    };
    if (message.cancelVisible)
        buttons->addWidget(makeButton(message.cancelText.isEmpty() ? qtr("Cancel") : message.cancelText,
                                      false, RedesignMessage::Cancelled));
    buttons->addStretch(1);
    if (!message.extraText.isEmpty())
        buttons->addWidget(makeButton(message.extraText, false, RedesignMessage::Extra));
    buttons->addWidget(makeButton(message.acceptText.isEmpty() ? qtr("OK") : message.acceptText,
                                  true, RedesignMessage::Accepted));
    column->addWidget(footer);

    const QString sheet = QStringLiteral(
        "QWidget { font-family: \"%1\"; font-size: %2px; }"
        "#redesignMessageCard { background: %3; border-radius: %4px; }"
        "#redesignMessageTitleBar { background: %5; border-top-left-radius: %4px;"
        " border-top-right-radius: %4px; }"
        "QLabel { background: transparent; }"
        "#redesignMessageTitle { color: %6; }"
        "#redesignMessageHeading { color: %7; font-size: %8px; }"
        "#redesignMessageText { color: %9; }")
            .arg(font.family(), QString::number(dp(14)), rgb(c.card), QString::number(dp(14)),
                 rgb(c.titleBar), rgb(c.title), rgb(c.heading), QString::number(dp(16)), rgb(c.text))
        + QStringLiteral(
        "QToolButton#redesignMessageClose { border: none; border-radius: %1px; background: transparent;"
        " padding: %2px; }"
        "QToolButton#redesignMessageClose:hover, QToolButton#redesignMessageClose:pressed"
        " { background: %3; }"
        "QToolButton#redesignMessageClose:focus { border: 1px solid %4; }")
            .arg(QString::number(dp(4)), QString::number(dp(4)), rgb(c.closeHover), rgb(c.accent))
        + QStringLiteral(
        "QPushButton { border: none; border-radius: %1px; padding: 0px %2px; font-weight: 600;"
        " background: %3; color: %4; }"
        "QPushButton:hover, QPushButton:pressed { background: %5; }"
        "QPushButton#redesignMessageAccent { background: %6; color: %7; }"
        "QPushButton#redesignMessageAccent:hover, QPushButton#redesignMessageAccent:pressed"
        " { background: %8; }"
        "QPushButton:focus { border: 1px solid %7; }")
            .arg(QString::number(dp(6)), QString::number(dp(16)), rgb(c.button), rgb(c.buttonText),
                 rgb(c.buttonHover), rgb(c.accentBackground), rgb(c.accent), rgb(c.accentHover))
        + QStringLiteral(
        "QLineEdit#redesignMessageField, QPlainTextEdit#redesignMessageField { background: %1;"
        " border: %2px solid %3; border-radius: %4px; color: %5; padding: %6px %7px %6px %8px; }")
            .arg(rgb(c.field), QString::number(dp(2)), rgb(c.fieldBorder), QString::number(dp(6)),
                 rgb(c.fieldText), QString::number(dp(8)), QString::number(dp(18)), QString::number(dp(12)));
    setStyleSheet(sheet);

    setMinimumWidth(dp(560) + dp(32));
    setMaximumWidth(dp(713) + dp(32));
}

QString RedesignMessageBox::inputText() const
{
    if (m_line)
        return m_line->text();
    if (m_multiLine)
        return m_multiLine->toPlainText();
    return {};
}

void RedesignMessageBox::finish(int answer)
{
    m_answered = true;
    m_answer = answer;
    QDialog::done(answer == RedesignMessage::Accepted ? QDialog::Accepted : QDialog::Rejected);
}

void RedesignMessageBox::done(int r)
{
    // closed otherwise (Alt+F4, the system): cancelled
    if (!m_answered)
    {
        m_answered = true;
        m_answer = RedesignMessage::Cancelled;
    }
    QDialog::done(r);
}

void RedesignMessageBox::keyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
    case Qt::Key_Escape:
        finish(RedesignMessage::Cancelled);
        return;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        // a button chosen with Tab, else the field's text, else the action
        // the message gives Return
        if (auto *button = qobject_cast<QPushButton *>(focusWidget()))
            button->click();
        else if (auto *tool = qobject_cast<QToolButton *>(focusWidget()))
            tool->click();
        else if (m_line || m_multiLine || m_message.defaultAccept)
            finish(RedesignMessage::Accepted);
        else
            finish(RedesignMessage::Cancelled);
        return;
    default:
        break;
    }
    QDialog::keyPressEvent(event);
}

void RedesignMessageBox::mousePressEvent(QMouseEvent *event)
{
    // the title bar moves the window
    if (event->button() == Qt::LeftButton && m_titleBar
        && m_titleBar->rect().contains(m_titleBar->mapFrom(this, event->position().toPoint())))
    {
        if (QWindow *window = windowHandle())
        {
            window->startSystemMove();
            event->accept();
            return;
        }
    }
    QDialog::mousePressEvent(event);
}

void RedesignMessageBox::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    if (event->spontaneous())
        return;

    // centred over its window, else the window of the interface
    QRect over;
    if (parentWidget())
        over = parentWidget()->window()->frameGeometry();
    else if (QWindow *window = interfaceWindow(p_intf); window && window->isVisible())
        over = window->frameGeometry();
    else if (QScreen *screen = QGuiApplication::primaryScreen())
        over = screen->availableGeometry();
    if (over.isValid())
    {
        QRect frame = frameGeometry();
        frame.moveCenter(over.center());
        move(frame.topLeft());
    }

    if (m_line)
    {
        m_line->setFocus(Qt::OtherFocusReason);
        m_line->selectAll();
    }
    else if (m_multiLine)
        m_multiLine->setFocus(Qt::OtherFocusReason);
    else
        setFocus(Qt::OtherFocusReason);
}

void RedesignMessageBox::setInterfaceTransientParent()
{
    QWindow *window = interfaceWindow(p_intf);
    if (!window)
        return;
    createWinId();
    if (QWindow *handle = windowHandle())
        handle->setTransientParent(window);
}

int RedesignMessageBox::ask(qt_intf_t *p_intf, const RedesignMessage &message, QWidget *parent, QString *input)
{
    RedesignMessageBox box(p_intf, message, parent);
    if (!parent)
        box.setInterfaceTransientParent();
    box.exec();
    if (input)
        *input = box.inputText();
    return box.answer();
}

void RedesignMessageBox::warning(qt_intf_t *p_intf, QWidget *parent, const QString &title, const QString &text)
{
    RedesignMessage message;
    message.title = title;
    message.heading = text;
    message.acceptText = qtr("OK");
    message.cancelVisible = false;
    ask(p_intf, message, parent);
}

bool RedesignMessageBox::question(qt_intf_t *p_intf, QWidget *parent, const QString &title,
                                  const QString &text, const QString &acceptText, bool defaultAccept)
{
    RedesignMessage message;
    message.title = title;
    message.heading = text;
    message.acceptText = acceptText;
    message.defaultAccept = defaultAccept;
    return ask(p_intf, message, parent) == RedesignMessage::Accepted;
}
