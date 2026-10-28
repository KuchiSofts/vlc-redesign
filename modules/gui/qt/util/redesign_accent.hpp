/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
#pragma once

#include <QObject>
#include <QColor>
#include <QString>
#include <QTimer>

#ifdef _WIN32
# include <QAbstractNativeEventFilter>
#endif

class QEvent;

// The accent of the redesign. redesign/accent keeps one of the colors of
// VLCStyle.redesign.accentChoices ("#d37918", the default) or "system": the
// accent color of the system, followed while VLC runs (Windows:
// Personalization > Colors; macOS: the accent of the Appearance; Linux: the
// highlight of the desktop when Qt gets it, else the default orange).
//
// The system's color is made as readable as the accents of the design: they
// are drawn as they are on the dark and on the light theme, from the blue
// (#185DD4, relative luminance 0.128, 3.3:1 on the dark window) to the grey
// (#AFAFAF, 0.429). A system color outside that range gets the lightness
// (HSL, hue and saturation kept) that brings its luminance to the nearest
// end: never darker than the blue on the dark theme, never lighter than the
// grey on the light one. Dim, Bright and the light theme's tint are then
// derived from it as from the other accents.
class RedesignAccent : public QObject
#ifdef _WIN32
    , public QAbstractNativeEventFilter
#endif
{
    Q_OBJECT

public:
    explicit RedesignAccent(QObject *parent = nullptr);
    ~RedesignAccent() override;

    // "system": the value of redesign/accent that follows the system
    static QString systemChoice();
    static bool isSystemChoice(const QString &value);
    // the redesign orange
    static QColor defaultAccent();
    // a value redesign/accent may keep (a color or "system")
    static bool isValidChoice(const QString &value);
    // the color made as readable as the accents of the design (see above)
    static QColor readable(const QColor &color);
    // the accent a value of redesign/accent stands for (the default if it
    // is invalid): the system's as last read for "system"
    static QColor resolve(const QString &value);

    // the system's accent, readable (the default orange if it gives none)
    QColor systemAccent() const { return m_accent; }

signals:
    void systemAccentChanged();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
#ifdef _WIN32
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;
#endif

private:
    void refresh();
    // the color as the system gives it (invalid: none)
    QColor readSystem();

    QColor m_accent;
    // a change of the system comes in a few messages and palette changes:
    // read once they are over
    QTimer m_refreshTimer;
#ifdef _WIN32
    // what Qt's palette and the DWM registry said the last time, and whether
    // Windows changed its accent without Qt's palette following
    QColor m_palette;
    QColor m_registry;
    bool m_registryAhead = false;
#endif
};
