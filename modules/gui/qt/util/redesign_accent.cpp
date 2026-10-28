/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "redesign_accent.hpp"

#include <QCoreApplication>
#include <QEvent>
#include <QGuiApplication>
#include <QPalette>
#include <QPointer>
#include <QStyleHints>

#include <cmath>

#ifdef _WIN32
# include <windows.h>
# ifndef WM_DWMCOLORIZATIONCOLORCHANGED
#  define WM_DWMCOLORIZATIONCOLORCHANGED 0x0320
# endif
#endif

namespace {

QPointer<RedesignAccent> s_instance;

// the luminance of the darkest (#185DD4) and the lightest (#AFAFAF) accent
// of the design
constexpr double minLuminance = 0.128;
constexpr double maxLuminance = 0.429;

double channel(float value)
{
    return value <= 0.04045f ? value / 12.92 : std::pow((value + 0.055) / 1.055, 2.4);
}

// relative luminance (WCAG 2)
double luminance(const QColor &color)
{
    const QColor rgb = color.toRgb();
    return 0.2126 * channel(rgb.redF()) + 0.7152 * channel(rgb.greenF())
           + 0.0722 * channel(rgb.blueF());
}

// Qt's palette accent (Qt 6.6 and later take it from the system)
QColor paletteAccent()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    if (!qGuiApp)
        return {};
    const QPalette palette = QGuiApplication::palette();
    const QColor accent = palette.color(QPalette::Active, QPalette::Accent);
    if (!accent.isValid() || accent.alpha() == 0)
        return {};
# ifndef _WIN32
    // Where the platform gives no accent, Qt shows its own highlight (the
    // blue of its default palette): not one of the system
    if (accent == QColor(0x30, 0x8C, 0xC6)
        && palette.color(QPalette::Active, QPalette::Highlight) == accent)
        return {};
# endif
    return accent;
#else
    return {};
#endif
}

#ifdef _WIN32
// HKCU\Software\Microsoft\Windows\DWM\AccentColor, 0xAABBGGRR
QColor registryAccent()
{
    DWORD value = 0;
    DWORD size = sizeof(value);
    if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\DWM", L"AccentColor",
                     RRF_RT_REG_DWORD, nullptr, &value, &size) != ERROR_SUCCESS)
        return {};
    return QColor(int(value & 0xFF), int((value >> 8) & 0xFF), int((value >> 16) & 0xFF));
}
#endif

} // namespace

RedesignAccent::RedesignAccent(QObject *parent)
    : QObject(parent)
{
    s_instance = this;
    m_refreshTimer.setSingleShot(true);
    m_refreshTimer.setInterval(250);
    connect(&m_refreshTimer, &QTimer::timeout, this, &RedesignAccent::refresh);

    m_accent = readable(readSystem());

    // Qt tells a new palette of the system to the application object (an
    // event filter there sees the events of every object: only that type
    // is looked at)
    if (qApp)
        qApp->installEventFilter(this);
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    if (qGuiApp && qGuiApp->styleHints())
        connect(qGuiApp->styleHints(), &QStyleHints::colorSchemeChanged,
                &m_refreshTimer, qOverload<>(&QTimer::start));
#endif
#ifdef _WIN32
    // Windows tells a new accent to the windows (WM_SETTINGCHANGE
    // "ImmersiveColorSet", WM_DWMCOLORIZATIONCOLORCHANGED) whether or not Qt
    // changes its palette for it
    if (qApp)
        qApp->installNativeEventFilter(this);
#endif
}

RedesignAccent::~RedesignAccent()
{
#ifdef _WIN32
    if (qApp)
        qApp->removeNativeEventFilter(this);
#endif
    if (s_instance == this)
        s_instance = nullptr;
}

QString RedesignAccent::systemChoice()
{
    return QStringLiteral("system");
}

bool RedesignAccent::isSystemChoice(const QString &value)
{
    return value.trimmed().compare(systemChoice(), Qt::CaseInsensitive) == 0;
}

QColor RedesignAccent::defaultAccent()
{
    return QColor(0xD3, 0x79, 0x18);
}

bool RedesignAccent::isValidChoice(const QString &value)
{
    return isSystemChoice(value) || QColor(value).isValid();
}

QColor RedesignAccent::readable(const QColor &color)
{
    if (!color.isValid())
        return defaultAccent();
    QColor rgb = color.toRgb();
    rgb.setAlpha(255);
    const double value = luminance(rgb);
    if (value >= minLuminance && value <= maxLuminance)
        return rgb;

    // the lightness (hue and saturation kept) that gives the luminance of
    // the nearest end; the luminance grows with the lightness
    const double target = value < minLuminance ? minLuminance : maxLuminance;
    const QColor hsl = rgb.toHsl();
    float hue = hsl.hslHueF();
    float saturation = hsl.hslSaturationF();
    if (hue < 0.f) // achromatic
    {
        hue = 0.f;
        saturation = 0.f;
    }
    float low = 0.f;
    float high = 1.f;
    for (int i = 0; i < 20; ++i)
    {
        const float middle = (low + high) / 2.f;
        if (luminance(QColor::fromHslF(hue, saturation, middle)) < target)
            low = middle;
        else
            high = middle;
    }
    // darker colors brought up to the end (at least it), lighter ones down
    // (at most it)
    QColor result = QColor::fromHslF(hue, saturation, value < minLuminance ? high : low).toRgb();
    result.setAlpha(255);
    return result;
}

QColor RedesignAccent::resolve(const QString &value)
{
    if (isSystemChoice(value))
        return s_instance ? s_instance->systemAccent() : readable(paletteAccent());
    const QColor color(value);
    return color.isValid() ? color : defaultAccent();
}

bool RedesignAccent::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::ApplicationPaletteChange)
        m_refreshTimer.start();
    return QObject::eventFilter(watched, event);
}

#ifdef _WIN32
bool RedesignAccent::nativeEventFilter(const QByteArray &, void *message, qintptr *)
{
    const MSG *msg = static_cast<const MSG *>(message);
    if (!msg)
        return false;
    switch (msg->message)
    {
    case WM_SETTINGCHANGE:
        if (msg->lParam && !lstrcmpW(reinterpret_cast<LPCWSTR>(msg->lParam), L"ImmersiveColorSet"))
            m_refreshTimer.start();
        break;
    case WM_DWMCOLORIZATIONCOLORCHANGED:
    case WM_SYSCOLORCHANGE:
        m_refreshTimer.start();
        break;
    default:
        break;
    }
    return false;
}
#endif

QColor RedesignAccent::readSystem()
{
    const QColor palette = paletteAccent();
#ifdef _WIN32
    const QColor registry = registryAccent();
    if (m_registry.isValid() && registry.isValid() && registry != m_registry && palette == m_palette)
        m_registryAhead = true; // Windows changed it, Qt's palette did not follow
    else if (palette != m_palette)
        m_registryAhead = false; // Qt's palette caught up
    m_palette = palette;
    m_registry = registry;
    if (m_registryAhead || !palette.isValid())
        return registry;
#endif
    return palette;
}

void RedesignAccent::refresh()
{
    const QColor accent = readable(readSystem());
    if (accent == m_accent)
        return;
    m_accent = accent;
    emit systemAccentChanged();
}
