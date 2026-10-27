/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
/****************************************************************************
 * profile_settings.hpp: Per-profile, non-secret interface configuration
 ****************************************************************************/
#ifndef VLC_QT_PROFILE_SETTINGS_HPP
#define VLC_QT_PROFILE_SETTINGS_HPP

#include <QVariantMap>
#include <QString>

class MainCtx;
class QSettings;

namespace ProfileSettings {
QVariantMap capture(MainCtx *ctx, QSettings *settings);
bool apply(MainCtx *ctx, QSettings *settings, const QVariantMap &target, QString *error);
}

// Writes the settings changed since the last sync, and tells whether this
// write worked. QSettings::status() keeps the first error met for the life
// of the object: one moment the file was locked (an antivirus, OneDrive)
// would fail every later save of the interface's settings
bool syncSettings(QSettings *settings);

#endif
