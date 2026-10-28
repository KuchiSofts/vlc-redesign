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

#include <QFontDatabase>
#include <QLocale>
#include <QObject>
#include <QQmlEngine>
#include <QStringList>

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

// The characters Inter does not draw come from the fallback fonts Windows
// lists for its own language: Chinese in Simplified on an English or a
// Japanese Windows had its characters in two fonts, the ones the first
// lacked thinner. The interface language picks the fonts of its script.
inline void applyLanguageFonts(const QLocale &locale)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0) && defined(_WIN32)
    QStringList han, kana, hangul;
    switch (locale.language())
    {
    case QLocale::Chinese:
        han = locale.script() == QLocale::TraditionalHanScript
            ? QStringList{ QStringLiteral("Microsoft JhengHei UI"), QStringLiteral("Microsoft JhengHei") }
            : QStringList{ QStringLiteral("Microsoft YaHei UI"), QStringLiteral("Microsoft YaHei") };
        break;
    case QLocale::Japanese:
        han = kana = { QStringLiteral("Yu Gothic UI"), QStringLiteral("Meiryo UI"), QStringLiteral("Meiryo") };
        break;
    case QLocale::Korean:
        han = hangul = { QStringLiteral("Malgun Gothic") };
        break;
    default:
        break;
    }
    const auto set = [](QChar::Script script, const QStringList &families) {
        if (QFontDatabase::applicationFallbackFontFamilies(script) != families)
            QFontDatabase::setApplicationFallbackFontFamilies(script, families);
    };
    set(QChar::Script_Han, han);
    set(QChar::Script_Hiragana, kana);
    set(QChar::Script_Katakana, kana);
    set(QChar::Script_Hangul, hangul);

    // The other scripts have one font of Windows' own interface: Hindi was
    // drawn in an installed Arial Unicode MS (Office), thinner and serif,
    // before Nirmala UI. Whatever the language (a file name can be in them).
    const QStringList nirmala{ QStringLiteral("Nirmala UI") };
    for (const QChar::Script script : { QChar::Script_Devanagari, QChar::Script_Bengali, QChar::Script_Gurmukhi,
                                        QChar::Script_Gujarati, QChar::Script_Oriya, QChar::Script_Tamil,
                                        QChar::Script_Telugu, QChar::Script_Kannada, QChar::Script_Malayalam })
        set(script, nirmala);
    set(QChar::Script_Sinhala, { QStringLiteral("Nirmala UI"), QStringLiteral("Iskoola Pota") });
    set(QChar::Script_Thai, { QStringLiteral("Leelawadee UI"), QStringLiteral("Tahoma") });
    set(QChar::Script_Lao, { QStringLiteral("Leelawadee UI"), QStringLiteral("Lao UI") });
    set(QChar::Script_Khmer, { QStringLiteral("Leelawadee UI"), QStringLiteral("Khmer UI") });
    set(QChar::Script_Myanmar, { QStringLiteral("Myanmar Text") });
    set(QChar::Script_Ethiopic, { QStringLiteral("Ebrima") });
    const QStringList segoe{ QStringLiteral("Segoe UI") };
    for (const QChar::Script script : { QChar::Script_Arabic, QChar::Script_Hebrew, QChar::Script_Georgian,
                                        QChar::Script_Armenian })
        set(script, segoe);
#elif QT_VERSION >= QT_VERSION_CHECK(6, 8, 0) && defined(__APPLE__)
    // macOS: Core Text finds a font for every script, but the Han
    // characters follow the system's language; the interface language
    // picks the fonts of its own (the other scripts keep the system's)
    QStringList han, kana, hangul;
    switch (locale.language())
    {
    case QLocale::Chinese:
        han = locale.script() == QLocale::TraditionalHanScript
            ? QStringList{ QStringLiteral("PingFang TC") }
            : QStringList{ QStringLiteral("PingFang SC") };
        break;
    case QLocale::Japanese:
        han = kana = { QStringLiteral("Hiragino Sans") };
        break;
    case QLocale::Korean:
        han = hangul = { QStringLiteral("Apple SD Gothic Neo") };
        break;
    default:
        break;
    }
    const auto set = [](QChar::Script script, const QStringList &families) {
        if (QFontDatabase::applicationFallbackFontFamilies(script) != families)
            QFontDatabase::setApplicationFallbackFontFamilies(script, families);
    };
    set(QChar::Script_Han, han);
    set(QChar::Script_Hiragana, kana);
    set(QChar::Script_Katakana, kana);
    set(QChar::Script_Hangul, hangul);
#else
    Q_UNUSED(locale);
#endif
}

#endif
