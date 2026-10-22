/*****************************************************************************
 * interface_language.hpp : language of the interface (Windows)
 *****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
#ifndef VLC_QT_INTERFACE_LANGUAGE_HPP
#define VLC_QT_INTERFACE_LANGUAGE_HPP

#include <QString>
#include <QVariantList>
#include <QVariantMap>

// The languages of the simple preferences, [{ value: iso, label: name }];
// empty where VLC follows the system language (not Windows)
QVariantList interfaceLanguages();
// The chosen language, applied at the next start: English when none is
// chosen in a portable copy (the installed one too), "auto" otherwise
QString interfaceLanguage();
bool setInterfaceLanguage(const QString &iso);
// Forgets the choice: back to the default above
void resetInterfaceLanguage();

#endif
