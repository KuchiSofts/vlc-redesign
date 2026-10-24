/*****************************************************************************
 * language_change.hpp : texts of C++ read by QML in the interface language
 *****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
#ifndef VLC_QT_LANGUAGE_CHANGE_HPP
#define VLC_QT_LANGUAGE_CHANGE_HPP

#include <QObject>
#include <QQmlEngine>

// The language of the interface changes while VLC runs
// (MainCtx::applyInterfaceLanguage): the QML engine then makes the bindings
// of qsTr() again (QQmlEngine::retranslate).
//
// A property or a Q_INVOKABLE that translates its text when it is read
// calls this: the binding of QML reading it is made again as well, as one
// of qsTr(). Nothing out of a binding (or before Qt 6.6).
inline void markTranslationBinding(const QObject *object)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    if (QQmlEngine *engine = qmlEngine(object))
        engine->markCurrentFunctionAsTranslationBinding();
#else
    Q_UNUSED(object);
#endif
}

#endif
