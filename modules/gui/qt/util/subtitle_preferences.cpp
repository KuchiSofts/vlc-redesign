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

#include "subtitle_preferences.hpp"

#include <QCollator>
#include <QHash>
#include <QLocale>
#include <QSet>
#include <QSettings>

#include <algorithm>

#include <vlc_common.h>
#include <vlc_configuration.h>
#include <vlc_iso_lang.h>
#include <vlc_player.h>

// VLC's table of the ISO 639 languages (p_languages), for the whole list:
// vlc_find_iso639 only looks one up
#include "../../../../src/text/iso-639_def.h"

namespace {

const char keyPrimary[] = "redesign/subtitle-primary";
const char keySecondary[] = "redesign/subtitle-secondary";
const char keyAutoSearch[] = "redesign/subtitle-auto-search";
// (a new name: "show both" was asked by the setup, and taken for the way
// the second language works; it is the fallback now, both at once a choice)
const char keyShowBoth[] = "redesign/subtitle-dual";

struct Language
{
    QString code;
    QString english; // untranslated, for qtr
    QString native;
};

// Languages of subtitles without an ISO 639-1 code, and the regional
// variants OpenSubtitles keeps apart
const struct { const char *code, *english, *native; } extraLanguages[] = {
    { "ast", N_("Asturian"), "Asturianu" },
    { "ceb", N_("Cebuano"), "Cebuano" },
    { "ckb", N_("Central Kurdish"), "کوردیی ناوەندی" },
    { "cnr", N_("Montenegrin"), "Crnogorski" },
    { "haw", N_("Hawaiian"), "ʻŌlelo Hawaiʻi" },
    { "hmn", N_("Hmong"), "Hmoob" },
    { "mni", N_("Manipuri"), "মৈতৈলোন্" },
    { "prs", N_("Dari"), "دری" },
    { "syr", N_("Syriac"), "ܣܘܪܝܝܐ" },
    { "yue", N_("Cantonese"), "粵語" },
    { "zgh", N_("Standard Moroccan Tamazight"), "ⵜⴰⵎⴰⵣⵉⵖⵜ" },
    { "pt-BR", N_("Portuguese (Brazil)"), "Português (Brasil)" },
    { "zh-TW", N_("Chinese (Traditional)"), "中文（繁體）" },
    { "es-419", N_("Spanish (Latin America)"), "Español (Latinoamérica)" },
};

QString nativeOf(const QString &code)
{
    const QLocale::Language language = QLocale::codeToLanguage(code);
    if (language == QLocale::AnyLanguage || language == QLocale::C)
        return {};
    // English: its default locale says "American English"
    if (language == QLocale::English)
        return QStringLiteral("English");
    const QString name = QLocale(language).nativeLanguageName();
    // the locale's own capitalisation: "עברית", "français" -> "Français"
    return name.isEmpty() ? name : name.left(1).toUpper() + name.mid(1);
}

const QList<Language> &allLanguages()
{
    static const QList<Language> list = [] {
        QList<Language> result;
        QSet<QString> seen;
        for (const iso639_lang_t *l = p_languages; l->psz_eng_name != nullptr; ++l)
        {
            const QString code = QString::fromLatin1(l->psz_iso639_1);
            // VLC's own entries: osd, and iw (the old code of Hebrew)
            if (code.size() != 2 || code == QLatin1String("od") || code == QLatin1String("iw")
                || seen.contains(code))
                continue;
            seen.insert(code);
            // "Adyghe; Adygei": the first name
            const QString english = QString::fromUtf8(l->psz_eng_name).section(';', 0, 0).trimmed();
            result.append({ code, english, nativeOf(code) });
        }
        for (const auto &extra : extraLanguages)
        {
            const QString code = QString::fromLatin1(extra.code);
            if (seen.contains(code))
                continue;
            seen.insert(code);
            result.append({ code, QString::fromUtf8(extra.english), QString::fromUtf8(extra.native) });
        }
        return result;
    }();
    return list;
}

const Language *findLanguage(const QString &code)
{
    for (const Language &language : allLanguages())
        if (language.code == code)
            return &language;
    return nullptr;
}

} // namespace

SubtitlePreferences::SubtitlePreferences(qt_intf_t *intf, QObject *parent)
    : QObject(parent)
    , m_intf(intf)
{
}

QString SubtitlePreferences::normalize(const QString &input)
{
    QString code = input.trimmed().toLower();
    code.replace('_', '-');
    if (code.isEmpty())
        return {};
    // OpenSubtitles' own codes, old codes and the regions kept apart
    static const QHash<QString, QString> special = {
        { "pob", "pt-BR" }, { "pb", "pt-BR" }, { "pt-br", "pt-BR" },
        { "zht", "zh-TW" }, { "cht", "zh-TW" }, { "zh-tw", "zh-TW" }, { "zh-hk", "zh-TW" },
        { "zh-mo", "zh-TW" }, { "zh-hant", "zh-TW" },
        { "zhs", "zh" }, { "chs", "zh" }, { "zhe", "zh" }, { "ze", "zh" }, { "zh-cn", "zh" },
        { "zh-sg", "zh" }, { "zh-hans", "zh" },
        { "spl", "es-419" }, { "ea", "es-419" }, { "es-419", "es-419" }, { "es-la", "es-419" },
        { "es-mx", "es-419" }, { "es-ar", "es-419" }, { "es-co", "es-419" }, { "spn", "es" },
        { "scc", "sr" }, { "scr", "hr" }, { "mne", "cnr" }, { "me", "cnr" },
        { "iw", "he" }, { "in", "id" }, { "ji", "yi" }, { "jw", "jv" }, { "mo", "ro" }, { "mol", "ro" },
        { "nb", "no" }, { "nob", "no" }, { "nn", "no" }, { "nno", "no" },
        { "fil", "tl" }, { "ma", "mni" }, { "sy", "syr" }, { "at", "ast" }, { "pr", "prs" },
        { "ku-ckb", "ckb" }, { "sdh", "ckb" },
    };
    const auto it = special.constFind(code);
    if (it != special.constEnd())
        return it.value();
    // a tag with a region or a script: its language
    if (code.contains('-'))
        return normalize(code.section('-', 0, 0));
    if (findLanguage(code))
        return code;
    const iso639_lang_t *l = vlc_find_iso639(qtu(code), true);
    if (l == nullptr || !strcmp(l->psz_iso639_2T, "osd") || !strcmp(l->psz_iso639_2T, "qaa"))
        return {};
    if (!strcmp(l->psz_iso639_1, "iw"))
        return QStringLiteral("he");
    return *l->psz_iso639_1 ? QString::fromLatin1(l->psz_iso639_1)
                            : QString::fromLatin1(l->psz_iso639_2T);
}

QString SubtitlePreferences::iso639_2(const QString &code)
{
    if (code == QLatin1String("pt-BR"))
        return QStringLiteral("por");
    if (code == QLatin1String("zh-TW"))
        return QStringLiteral("zho");
    if (code == QLatin1String("es-419"))
        return QStringLiteral("spa");
    if (code.size() == 3)
        return code;
    const iso639_lang_t *l = vlc_find_iso639(qtu(code), false);
    return l ? QString::fromLatin1(l->psz_iso639_2T) : QString();
}

QString SubtitlePreferences::name(const QString &code) const
{
    const QString normalized = normalize(code);
    if (const Language *language = findLanguage(normalized))
        return qtr(qtu(language->english));
    const iso639_lang_t *l = normalized.isEmpty() ? nullptr : vlc_find_iso639(qtu(normalized), false);
    return l ? qtr(l->psz_eng_name).section(';', 0, 0).trimmed() : code;
}

QString SubtitlePreferences::nativeName(const QString &code) const
{
    const Language *language = findLanguage(normalize(code));
    return language ? language->native : QString();
}

QVariantList SubtitlePreferences::languages() const
{
    QList<QVariantMap> entries;
    for (const Language &language : allLanguages())
        entries.append({ { QStringLiteral("code"), language.code },
                         { QStringLiteral("name"), qtr(qtu(language.english)) },
                         { QStringLiteral("native"), language.native } });
    QCollator collator;
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    std::sort(entries.begin(), entries.end(), [&](const QVariantMap &a, const QVariantMap &b) {
        return collator.compare(a.value(QStringLiteral("name")).toString(),
                                b.value(QStringLiteral("name")).toString()) < 0;
    });
    QVariantList result;
    for (const QVariantMap &entry : entries)
        result.append(entry);
    return result;
}

QString SubtitlePreferences::systemLanguage() const
{
    const QString code = normalize(QLocale::system().name());
    return code.isEmpty() ? QStringLiteral("en") : code;
}

QString SubtitlePreferences::primary() const
{
    const QString value = normalize(m_intf->mainSettings->value(keyPrimary).toString());
    return value.isEmpty() ? systemLanguage() : value;
}

QString SubtitlePreferences::secondary() const
{
    const QVariant stored = m_intf->mainSettings->value(keySecondary);
    if (stored.isValid())
        return normalize(stored.toString());   // "" when the user chose none
    // English as the second choice, unless it is the first
    return primary() == QLatin1String("en") ? QString() : QStringLiteral("en");
}

bool SubtitlePreferences::autoSearch() const
{
    return m_intf->mainSettings->value(keyAutoSearch, true).toBool();
}

bool SubtitlePreferences::showBoth() const
{
    return m_intf->mainSettings->value(keyShowBoth, false).toBool();
}

bool SubtitlePreferences::chosen() const
{
    return m_intf->mainSettings->contains(keyPrimary);
}

QStringList SubtitlePreferences::wanted() const
{
    QStringList result = { primary() };
    const QString second = secondary();
    if (!second.isEmpty() && second != result.first())
        result.append(second);
    return result;
}

void SubtitlePreferences::set(const QString &primary, const QString &secondary,
                              bool autoSearch, bool showBoth)
{
    const QString first = normalize(primary);
    QString second = normalize(secondary);
    if (second == first)
        second.clear();
    QSettings *settings = m_intf->mainSettings;
    settings->setValue(keyPrimary, first.isEmpty() ? systemLanguage() : first);
    settings->setValue(keySecondary, second);
    settings->setValue(keyAutoSearch, autoSearch);
    settings->setValue(keyShowBoth, showBoth);
    settings->sync();
    syncCore();
    emit changed();
}

// VLC picks the tracks of a media in these languages: its sub-language
// option, for the next start, and the player's, at once
void SubtitlePreferences::syncCore()
{
    QStringList codes;
    for (const QString &code : wanted())
    {
        const QString iso = iso639_2(code);
        if (!iso.isEmpty() && !codes.contains(iso))
            codes.append(iso);
    }
    const QByteArray value = codes.join(',').toUtf8();
    config_PutPsz("sub-language", value.constData());
    config_SaveConfigFile(m_intf);
    if (m_intf->p_player)
    {
        vlc_player_Lock(m_intf->p_player);
        vlc_player_SelectSubtitleLanguage(m_intf->p_player, value.constData());
        vlc_player_Unlock(m_intf->p_player);
    }
}
