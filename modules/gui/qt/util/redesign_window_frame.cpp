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

#include "redesign_window_frame.hpp"

#include "qt.hpp"
#include "util/redesign_icon_provider.hpp"

#include <QAbstractButton>
#include <QAbstractNativeEventFilter>
#include <QApplication>
#include <QEvent>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMargins>
#include <QPointer>
#include <QToolButton>
#include <QVariant>
#include <QWidget>
#include <QWindow>

#include <cmath>

#ifdef _WIN32
# include <windows.h>
# include <dwmapi.h>
// the themed caption and frame (undocumented)
# ifndef WM_NCUAHDRAWCAPTION
#  define WM_NCUAHDRAWCAPTION 0x00AE
# endif
# ifndef WM_NCUAHDRAWFRAME
#  define WM_NCUAHDRAWFRAME 0x00AF
# endif
#endif

namespace {

// set on a window given the bar (and the bar it got)
const char frameProperty[] = "_vlc_redesignFrame";
// the window's own contents margins, under the bar
const char marginsProperty[] = "_vlc_redesignFrameMargins";

int dp(double px, double scale)
{
    return static_cast<int>(std::lround(px * scale));
}

/**
 * "Frame 120" of RedesignDialog.qml: padding 9, the title (10 in, up to the
 * buttons) and the buttons 32 x 32 with the icons of 24
 */
class TitleBar : public QWidget
{
public:
    explicit TitleBar(QWidget *window)
        : QWidget(window)
        , m_window(window)
    {
        setObjectName(QStringLiteral("redesignTitleBar"));
        setAttribute(Qt::WA_StyledBackground);
        // not in the window's layout: placed by hand over the margin
        // left for it
        m_layout = new QHBoxLayout(this);
        m_layout->setSpacing(0);

        m_title = new QLabel(this);
        m_title->setObjectName(QStringLiteral("redesignTitle"));
        m_title->setTextFormat(Qt::PlainText);
        m_title->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        m_title->setAlignment(Qt::AlignVCenter | Qt::AlignLeading);
        // the clicks go to the window (dragging it)
        m_title->setAttribute(Qt::WA_TransparentForMouseEvents);
        m_layout->addWidget(m_title, 1);

        m_minimize = makeButton(QStringLiteral("minimize"), qtr("Minimize"));
        m_maximize = makeButton(QStringLiteral("maximize"), qtr("Maximize"));
        m_close = makeButton(QStringLiteral("close"), qtr("Close"));
        connect(m_minimize, &QToolButton::clicked, this, [this]() { m_window->showMinimized(); });
        connect(m_maximize, &QToolButton::clicked, this, [this]() {
            if (m_window->isMaximized())
                m_window->showNormal();
            else
                m_window->showMaximized();
        });
        connect(m_close, &QToolButton::clicked, this, [this]() { m_window->close(); });

        updateTitle();
    }

    void setScale(double scale)
    {
        m_scale = scale;
        m_layout->setContentsMargins(dp(9 + 10, scale), dp(9, scale), dp(9, scale), dp(9, scale));
        for (QToolButton *button : { m_minimize, m_maximize, m_close })
        {
            button->setFixedSize(dp(32, scale), dp(32, scale));
            button->setIconSize(QSize(dp(24, scale), dp(24, scale)));
        }
        setFixedHeight(barHeight(scale));
    }

    static int barHeight(double scale) { return dp(9 + 32 + 9, scale); }

    void updateTitle()
    {
        m_title->setText(m_window->windowTitle());
    }

    // minimize where the window has it, maximize where it can also be
    // resized (as the system's bar)
    void updateButtons()
    {
        const Qt::WindowFlags flags = m_window->windowFlags();
        const Qt::WindowType type = m_window->windowType();
        const bool customized = flags.testFlag(Qt::CustomizeWindowHint);
        const bool resizable = m_window->minimumSize() != m_window->maximumSize();
        bool minimize = type != Qt::Tool
                        && (flags.testFlag(Qt::WindowMinimizeButtonHint) || (type == Qt::Window && !customized));
        bool maximize = type != Qt::Tool
                        && (flags.testFlag(Qt::WindowMaximizeButtonHint) || (type == Qt::Window && !customized));
#ifdef _WIN32
        // what the system made of the flags
        if (m_window->testAttribute(Qt::WA_WState_Created) && m_window->windowHandle())
        {
            const LONG style = GetWindowLongW(reinterpret_cast<HWND>(m_window->winId()), GWL_STYLE);
            minimize = (style & WS_MINIMIZEBOX) != 0;
            maximize = (style & WS_MAXIMIZEBOX) != 0;
        }
#endif
        m_minimize->setVisible(minimize);
        m_maximize->setVisible(maximize && resizable);
        m_close->setVisible(!customized || flags.testFlag(Qt::WindowCloseButtonHint));
    }

private:
    QToolButton *makeButton(const QString &icon, const QString &name)
    {
        auto *button = new QToolButton(this);
        button->setObjectName(QStringLiteral("redesignTitleButton"));
        // drawn from its SVG at the size and scale shown
        button->setIcon(RedesignIcons::icon(QStringLiteral(":/redesign/icons/%1/default.png").arg(icon)));
        button->setAccessibleName(name);
        button->setToolTip(name);
        button->setAutoRaise(true);
        button->setFocusPolicy(Qt::NoFocus);
        m_layout->addWidget(button);
        return button;
    }

    QWidget *const m_window;
    QHBoxLayout *m_layout = nullptr;
    QLabel *m_title = nullptr;
    QToolButton *m_minimize = nullptr;
    QToolButton *m_maximize = nullptr;
    QToolButton *m_close = nullptr;
    double m_scale = 1.0;
};

TitleBar *titleBarOf(const QWidget *window)
{
    // (no meta-object of its own: qobject_cast would take any widget)
    return dynamic_cast<TitleBar *>(window->property(frameProperty).value<QObject *>());
}

// a window drawn by the system as it is: popups, tooltips, frameless ones
bool keepsItsFrame(const QWidget *window)
{
    const Qt::WindowType type = window->windowType();
    if (type == Qt::Popup || type == Qt::ToolTip || type == Qt::SplashScreen
        || type == Qt::Desktop || type == Qt::Drawer || type == Qt::Sheet)
        return true;
    if (window->windowFlags().testFlag(Qt::FramelessWindowHint))
        return true;
    return window->property("vlcRedesignNoFrame").toBool();
}

#ifdef _WIN32

int systemMetric(int index, HWND hwnd)
{
    typedef UINT (WINAPI *GetDpiForWindowFunc)(HWND);
    typedef int (WINAPI *GetSystemMetricsForDpiFunc)(int, UINT);
    static const auto user32 = LoadLibraryEx(TEXT("user32.dll"), nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    static const auto getDpiForWindow = user32
        ? reinterpret_cast<GetDpiForWindowFunc>(GetProcAddress(user32, "GetDpiForWindow")) : nullptr;
    static const auto getSystemMetricsForDpi = user32
        ? reinterpret_cast<GetSystemMetricsForDpiFunc>(GetProcAddress(user32, "GetSystemMetricsForDpi")) : nullptr;
    if (getDpiForWindow && getSystemMetricsForDpi)
        return getSystemMetricsForDpi(index, getDpiForWindow(hwnd));
    return GetSystemMetrics(index);
}

int resizeBorder(HWND hwnd)
{
    return systemMetric(SM_CYSIZEFRAME, hwnd) + systemMetric(SM_CXPADDEDBORDER, hwnd);
}

/**
 * The caption taken out of the native frame of the windows given the bar,
 * and the bar standing for it (as CSDWin32EventHandler for the main window)
 */
class FrameEventFilter : public QAbstractNativeEventFilter
{
public:
    bool nativeEventFilter(const QByteArray &, void *message, qintptr *result) override
    {
        MSG *msg = static_cast<MSG *>(message);
        if (msg->message != WM_NCCALCSIZE && msg->message != WM_NCHITTEST
            && msg->message != WM_NCUAHDRAWCAPTION && msg->message != WM_NCUAHDRAWFRAME)
            return false;

        QWidget *window = QWidget::find(reinterpret_cast<WId>(msg->hwnd));
        if (!window || !window->isWindow())
            return false;
        TitleBar *bar = titleBarOf(window);
        if (!bar || !bar->isVisible())
            return false;

        switch (msg->message)
        {
        case WM_NCCALCSIZE:
        {
            if (!msg->wParam)
                return false;
            // the frame the system gives, without the caption: the left,
            // right and bottom borders stay (resizing, the shadow); on top
            // the client area starts with the window, the bar draws there
            auto *params = reinterpret_cast<NCCALCSIZE_PARAMS *>(msg->lParam);
            const LONG top = params->rgrc[0].top;
            *result = DefWindowProcW(msg->hwnd, WM_NCCALCSIZE, msg->wParam, msg->lParam);
            // maximized, the system puts the window beyond the screen by
            // its border: keep that much out
            params->rgrc[0].top = top + (IsZoomed(msg->hwnd) ? resizeBorder(msg->hwnd) : 0);
            return true;
        }
        case WM_NCUAHDRAWCAPTION:
        case WM_NCUAHDRAWFRAME:
            // the themed caption would be drawn over the bar
            *result = 0;
            return true;
        case WM_NCHITTEST:
        {
            const LRESULT hit = DefWindowProcW(msg->hwnd, WM_NCHITTEST, msg->wParam, msg->lParam);
            if (hit != HTCLIENT)
            {
                *result = hit;
                return true;
            }
            POINT point = { static_cast<short>(LOWORD(msg->lParam)), static_cast<short>(HIWORD(msg->lParam)) };
            ScreenToClient(msg->hwnd, &point);

            const LONG style = GetWindowLongW(msg->hwnd, GWL_STYLE);
            // the top border, now in the client area, still resizes
            if ((style & WS_THICKFRAME) && !IsZoomed(msg->hwnd)
                && window->minimumSize() != window->maximumSize())
            {
                const int border = resizeBorder(msg->hwnd);
                if (point.y >= 0 && point.y < border)
                {
                    RECT client;
                    GetClientRect(msg->hwnd, &client);
                    if (point.x < border)
                        *result = HTTOPLEFT;
                    else if (point.x >= client.right - border)
                        *result = HTTOPRIGHT;
                    else
                        *result = HTTOP;
                    return true;
                }
            }

            const qreal ratio = window->devicePixelRatioF();
            const QPoint position(static_cast<int>(point.x / ratio), static_cast<int>(point.y / ratio));
            if (!bar->geometry().contains(position))
                return false;
            // the buttons are clicked as widgets; the rest of the bar is the
            // caption: dragging, snapping, a double click maximizing, the
            // system menu on a right click
            QWidget *child = bar->childAt(position - bar->pos());
            if (qobject_cast<QAbstractButton *>(child))
                *result = HTCLIENT;
            else
                *result = HTCAPTION;
            return true;
        }
        }
        return false;
    }
};

FrameEventFilter *s_filter = nullptr;

void refreshNativeFrame(QWidget *window)
{
    if (!window->testAttribute(Qt::WA_WState_Created) || !window->windowHandle())
        return;
    const HWND hwnd = reinterpret_cast<HWND>(window->winId());
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE
                 | SWP_NOZORDER | SWP_NOOWNERZORDER);
}

void setFrameColours(QWidget *window, bool dark, const QColor &border)
{
    if (!window->testAttribute(Qt::WA_WState_Created) || !window->windowHandle())
        return;
    const HWND hwnd = reinterpret_cast<HWND>(window->winId());
    // the borders and the system menu in the theme; Windows 11 rounds the
    // corners (as the cards of the dialogs) and draws its border
    const BOOL darkMode = dark;
    DwmSetWindowAttribute(hwnd, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &darkMode, sizeof(darkMode));
    const DWORD corner = 2; // DWMWCP_ROUND
    DwmSetWindowAttribute(hwnd, 33 /* DWMWA_WINDOW_CORNER_PREFERENCE */, &corner, sizeof(corner));
    const COLORREF colour = RGB(border.red(), border.green(), border.blue());
    DwmSetWindowAttribute(hwnd, 34 /* DWMWA_BORDER_COLOR */, &colour, sizeof(colour));
}

#endif // _WIN32

/**
 * Keeps the bar over the top of the window, its title and buttons up to date
 */
class FrameWatcher : public QObject
{
public:
    explicit FrameWatcher(QWidget *window, TitleBar *bar)
        : QObject(window)
        , m_window(window)
        , m_bar(bar)
    {
        window->installEventFilter(this);
    }

    bool dark = true;
    QColor border;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (watched != m_window || !m_bar)
            return false;
        switch (event->type())
        {
        case QEvent::Resize:
            place();
            break;
        case QEvent::WindowTitleChange:
            m_bar->updateTitle();
            break;
        case QEvent::Show:
            // the flags may have changed since (a native window made again)
            if (keepsItsFrame(m_window))
            {
                remove();
                break;
            }
            m_bar->updateButtons();
            place();
#ifdef _WIN32
            refreshNativeFrame(m_window);
            setFrameColours(m_window, dark, border);
#endif
            break;
        case QEvent::LayoutRequest:
            // a fixed size set later (QLayout::SetFixedSize)
            m_bar->updateButtons();
            break;
        default:
            break;
        }
        return false;
    }

private:
    void place()
    {
        m_bar->setGeometry(0, 0, m_window->width(), m_bar->height());
        m_bar->raise();
    }

    void remove()
    {
        m_bar->hide();
        m_window->setContentsMargins(m_window->property(marginsProperty).value<QMargins>());
    }

    QWidget *const m_window;
    QPointer<TitleBar> m_bar;
};

FrameWatcher *watcherOf(const QWidget *window)
{
    for (QObject *child : window->children())
    {
        if (auto *watcher = dynamic_cast<FrameWatcher *>(child))
            return watcher;
    }
    return nullptr;
}

} // anonymous namespace

void RedesignWindowFrame::install(QWidget *window)
{
#ifdef _WIN32
    if (!window || !window->isWindow() || titleBarOf(window) || keepsItsFrame(window))
        return;

    if (!s_filter)
    {
        s_filter = new FrameEventFilter;
        qApp->installNativeEventFilter(s_filter);
    }

    // the room of the bar, above what the window holds (its layout lays
    // out in the contents rectangle)
    const QMargins margins = window->contentsMargins();
    window->setProperty(marginsProperty, QVariant::fromValue(margins));

    auto *bar = new TitleBar(window);
    bar->setScale(1.0);
    window->setProperty(frameProperty, QVariant::fromValue<QObject *>(bar));
    window->setContentsMargins(margins.left(), margins.top() + bar->height(), margins.right(), margins.bottom());
    new FrameWatcher(window, bar);
    bar->updateButtons();
    bar->setGeometry(0, 0, window->width(), bar->height());
    bar->show();
    refreshNativeFrame(window);
#else
    // the system's title bar stays (a frameless window would lose the
    // window manager's moving and resizing)
    (void)window;
#endif
}

void RedesignWindowFrame::update(QWidget *window, double scale, bool dark, const QColor &border)
{
    TitleBar *bar = window ? titleBarOf(window) : nullptr;
    if (!bar)
        return;
    const int previous = bar->height();
    bar->setScale(scale);
    if (!bar->isHidden())
    {
        const QMargins margins = window->contentsMargins();
        window->setContentsMargins(margins.left(), margins.top() - previous + bar->height(),
                                   margins.right(), margins.bottom());
    }
    bar->setGeometry(0, 0, window->width(), bar->height());
    if (FrameWatcher *watcher = watcherOf(window))
    {
        watcher->dark = dark;
        watcher->border = border;
    }
#ifdef _WIN32
    setFrameColours(window, dark, border);
#else
    (void)dark;
    (void)border;
#endif
}
