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
#include <QString>
#include <QStringList>
#include <QVariantList>

#include "qt.hpp"

// The subtitle languages of the user: a default one and a second one, used
// when there are no subtitles in the first (in the video, next to it, or
// online), asked by the setup and in Settings. VLC picks the tracks of a
// media in this order (its sub-language option, kept in step), and the
// online search (SubtitleFinder) looks for them. With the list of the languages
// (every ISO 639-1 language, some more that subtitles use, and regional
// variants), their names in English and in their own language.
class SubtitlePreferences : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString primary READ primary NOTIFY changed FINAL)
    Q_PROPERTY(QString secondary READ secondary NOTIFY changed FINAL)
    // find subtitles online when a video has none in these languages
    Q_PROPERTY(bool autoSearch READ autoSearch NOTIFY changed FINAL)
    // the second language shown at the same time, above (dual subtitles;
    // off: it is only used when there are none in the first)
    Q_PROPERTY(bool showBoth READ showBoth NOTIFY changed FINAL)
    // whether the user chose them (setup or Settings), not the defaults
    Q_PROPERTY(bool chosen READ chosen NOTIFY changed FINAL)
    // { code, name, native }, by name
    Q_PROPERTY(QVariantList languages READ languages CONSTANT FINAL)

public:
    explicit SubtitlePreferences(qt_intf_t *intf, QObject *parent = nullptr);

    QString primary() const;
    QString secondary() const;
    bool autoSearch() const;
    bool showBoth() const;
    bool chosen() const;
    QVariantList languages() const;

    // The code of a language as kept here ("he", "pt-BR", "ast"): from an
    // ISO 639 code (1, 2/T, 2/B), an OpenSubtitles one ("pob", "zht"), a
    // tag with a region ("en-US") or an English name; empty when unknown
    Q_INVOKABLE static QString normalize(const QString &code);
    // the name in English, translated
    Q_INVOKABLE QString name(const QString &code) const;
    // the name in the language itself ("עברית"), empty when unknown
    Q_INVOKABLE QString nativeName(const QString &code) const;
    // the language of the system, the default of the setup
    Q_INVOKABLE QString systemLanguage() const;
    // the languages to look for, the default first
    QStringList wanted() const;

    Q_INVOKABLE void set(const QString &primary, const QString &secondary,
                         bool autoSearch, bool showBoth);

    // ISO 639-2/T, what VLC's sub-language option and the tracks use
    static QString iso639_2(const QString &code);

signals:
    void changed();

private:
    void syncCore();

    qt_intf_t *m_intf;
};
