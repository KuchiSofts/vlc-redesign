/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
/****************************************************************************
 * config_model.cpp : Staged VLC configuration for QML preferences
 ****************************************************************************/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "config_model.hpp"
#include "catalog.hpp"
#include "maininterface/mainctx.hpp"
#include "qt.hpp"

#include <vlc_configuration.h>
#include <vlc_modules.h>
#include <vlc_plugin.h>
#include <vlc_player.h>
#include <vlc_vout.h>

#ifdef _WIN32
# include <windows.h>
#endif

#include <QGuiApplication>
#include <QLocale>
#include <QScreen>
#include <QVariantMap>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <limits>

ConfigModel::ConfigModel(QObject *parent) : QAbstractListModel(parent) {}

namespace {

// Preferred track languages as a dropdown: VLC expects ISO 639-2 codes; a
// value of the user that is not listed (a list of codes...) is kept
QVariantList languageChoices(const QString &current, bool withSystem = false)
{
    static const struct { const char *code; const char *name; } languages[] = {
        { "ara", N_("Arabic") },
        { "chi", N_("Chinese") },
        { "cze", N_("Czech") },
        { "dan", N_("Danish") },
        { "dut", N_("Dutch") },
        { "eng", N_("English") },
        { "fin", N_("Finnish") },
        { "fre", N_("French") },
        { "ger", N_("German") },
        { "gre", N_("Greek") },
        { "heb", N_("Hebrew") },
        { "hin", N_("Hindi") },
        { "hun", N_("Hungarian") },
        { "ind", N_("Indonesian") },
        { "ita", N_("Italian") },
        { "jpn", N_("Japanese") },
        { "kor", N_("Korean") },
        { "may", N_("Malay") },
        { "nor", N_("Norwegian") },
        { "per", N_("Persian") },
        { "pol", N_("Polish") },
        { "por", N_("Portuguese") },
        { "rum", N_("Romanian") },
        { "rus", N_("Russian") },
        { "spa", N_("Spanish") },
        { "swe", N_("Swedish") },
        { "tha", N_("Thai") },
        { "tur", N_("Turkish") },
        { "ukr", N_("Ukrainian") },
        { "vie", N_("Vietnamese") },
    };
    const auto choice = [](const QString &value, const QString &label) {
        return QVariant(QVariantMap{{QStringLiteral("value"), value}, {QStringLiteral("label"), label}});
    };
    QVariantList choices{ choice(QString(), qtr("Auto")) };
    bool listed = current.isEmpty();
    // The preferred tracks of the design: "System language", the language of
    // the system (its ISO 639-2/B code, as the list), in place of its name
    QString system;
    if (withSystem && QLocale::system().language() != QLocale::C)
        system = QLocale::languageToCode(QLocale::system().language(), QLocale::ISO639Part2B);
    if (!system.isEmpty()) {
        choices.append(choice(system, qtr("System language")));
        listed = listed || system == current;
    }
    for (const auto &language : languages)
    {
        const QString code = QString::fromLatin1(language.code);
        if (code == system)
            continue;
        choices.append(choice(code, qtr(language.name)));
        listed = listed || code == current;
    }
    if (!listed)
        choices.append(choice(current, current));
    return choices;
}

// Dropdowns of the design for options VLC lets type freely: aspect ratio,
// crop, zoom, width of the snapshots; a value of the user not listed is kept
QVariantList presetChoices(const QString &key, const QVariant &current)
{
    QVariantList choices;
    bool listed = false;
    const auto add = [&](const QVariant &value, const QString &label) {
        choices.append(QVariantMap{{QStringLiteral("value"), value}, {QStringLiteral("label"), label}});
        listed = listed || value == current;
    };
    if (key == QLatin1String("aspect-ratio") || key == QLatin1String("crop")) {
        const bool aspect = key == QLatin1String("aspect-ratio");
        add(QString(), aspect ? qtr("Automatic") : qtr("None"));
        for (const char *ratio : { "16:9", "4:3", "1:1", "16:10", "2.21:1", "2.35:1", "2.39:1", "5:4" })
            add(QString::fromLatin1(ratio), QString::fromLatin1(ratio));
        if (!listed && !current.toString().isEmpty())
            add(current, current.toString());
    } else if (key == QLatin1String("zoom")) {
        const struct { float value; const char *label; } zooms[] = {
            { .25f, N_("1:4 Quarter") }, { .5f, N_("1:2 Half") },
            { 1.f, N_("1:1 Original") }, { 2.f, N_("2:1 Double") } };
        for (const auto &zoom : zooms)
            add(QVariant::fromValue(zoom.value), qtr(zoom.label));
        if (!listed)
            add(current, QString::number(current.toDouble()));
    } else if (key == QLatin1String("postproc-q")) {
        const char *const levels[] = { N_("Off"), N_("Lowest"), N_("Low"), N_("Medium"),
                                       N_("High"), N_("Higher"), N_("Highest") };
        for (int64_t level = 0; level < 7; ++level)
            add(QVariant::fromValue(level), qtr(levels[level]));
        if (!listed)
            add(current, QString::number(current.toLongLong()));
    } else if (key == QLatin1String("codec")) {
        add(QStringLiteral("any"), qtr("Automatic"));
        if (!listed)
            add(current, current.toString());
    } else if (key == QLatin1String("sub-text-scale")) {
        const struct { int64_t value; const char *label; } sizes[] = {
            { 50, N_("Smaller") }, { 75, N_("Small") }, { 100, N_("Automatic") },
            { 125, N_("Large") }, { 150, N_("Larger") }, { 200, N_("Largest") } };
        for (const auto &size : sizes)
            add(QVariant::fromValue(size.value), qtr(size.label));
        if (!listed)
            add(current, QStringLiteral("%1%").arg(current.toLongLong()));
    } else if (key == QLatin1String("freetype-shadow-distance")) {
        const struct { float value; const char *label; } distances[] = {
            { .03f, N_("Small") }, { .06f, N_("Medium") }, { .1f, N_("Large") } };
        for (const auto &distance : distances)
            add(QVariant::fromValue(distance.value), qtr(distance.label));
        if (!listed)
            add(current, QString::number(current.toDouble()));
    } else if (key == QLatin1String("sub-margin")) {
        const struct { int64_t value; const char *label; } margins[] = {
            { 0, N_("Bottom") }, { 40, N_("Slightly raised") }, { 100, N_("Raised") } };
        for (const auto &margin : margins)
            add(QVariant::fromValue(margin.value), qtr(margin.label));
        if (!listed)
            add(current, QStringLiteral("%1 px").arg(current.toLongLong()));
    } else if (key == QLatin1String("input-timeshift-granularity")) {
        // -1: VLC's own size (50 MiB)
        add(QVariant::fromValue<int64_t>(10 * 1024 * 1024), qtr("Small"));
        add(QVariant::fromValue<int64_t>(-1), qtr("Medium"));
        add(QVariant::fromValue<int64_t>(200 * 1024 * 1024), qtr("Large"));
        if (!listed)
            add(current, qtr("%1 MiB").arg(current.toLongLong() / (1024 * 1024)));
    } else if (key == QLatin1String("snapshot-width")) {
        add(QVariant::fromValue<int64_t>(-1), qtr("Same as video"));
        for (int64_t width : { 3840, 1920, 1280, 640 })
            add(QVariant::fromValue(width), qtr("%1 px wide").arg(width));
        if (!listed)
            add(current, qtr("%1 px wide").arg(current.toLongLong()));
    }
    return choices;
}

// The subtitles shown move at once with the position chosen (sub-position
// of the videos playing), before it is kept
void showSubtitlePosition(MainCtx *ctx, int64_t position)
{
    if (!ctx || !ctx->getIntf() || !ctx->getIntf()->p_player)
        return;
    size_t count = 0;
    vout_thread_t **vouts = vlc_player_vout_HoldAll(ctx->getIntf()->p_player, &count);
    for (size_t i = 0; i < count; ++i)
    {
        var_SetInteger(vouts[i], "sub-position", position);
        vout_Release(vouts[i]);
    }
    free(vouts);
}

// A module in a list of modules ("equalizer:compressor")
bool listHas(const QVariant &list, const QString &module)
{
    return list.toString().split(':', Qt::SkipEmptyParts).contains(module);
}

// Buffering of the design: Lowest latency, Low, Normal (the defaults of
// VLC), High; then Custom
const char *const cachingKeys[4] = { "file-caching", "disc-caching", "network-caching", "live-caching" };
const int64_t bufferingPresets[][4] = {
    { 100, 100, 150, 100 }, { 300, 150, 500, 150 }, { 1000, 300, 1000, 300 }, { 3000, 1000, 3000, 1000 } };

int64_t bufferingPreset(const int64_t delays[4])
{
    for (size_t preset = 0; preset < std::size(bufferingPresets); ++preset)
        if (std::equal(delays, delays + 4, bufferingPresets[preset]))
            return int64_t(preset);
    return int64_t(std::size(bufferingPresets));
}

// Night mode of the design: the compressor with these settings
constexpr float nightRatio = 8.f, nightThreshold = -30.f, nightMakeupGain = 9.f;

// The http interface (Web remote control) in a list of interfaces
bool hasHttp(const QVariant &interfaces)
{
    return interfaces.toString().split(':', Qt::SkipEmptyParts).contains(QStringLiteral("http"));
}

// Settings of the design kept by the interface (QSettings): "qt:<key>" in
// the catalog
struct QtSettingOption
{
    const char *key;
    char type; // 'b'oolean, 'i'nteger, 's'tring
    int defaultValue, minimum, maximum;
    const char *label;
    const char *help;
    // An integer of choices: their labels for 0, 1... ("Nothing|Fullscreen")
    const char *choices = nullptr;
};

const QtSettingOption qtSettingOptions[] = {
    { "redesign/discover-cast-devices", 'b', 1, 0, 0, N_("Discover cast devices"),
      N_("Looks for the cast devices of the local network when the Renderer menu opens.") },
    { "redesign/remember-window-geometry", 'b', 1, 0, 0, N_("Remember window size and position"),
      N_("Opens the window where it was when VLC was closed.") },
    { "redesign/notification-duration", 'i', 3000, 0, 10000, N_("Popup duration"),
      N_("How long the popup of the track changes stays on the screen.") },
    { "redesign/process-priority", 'i', 0, -2, 2, N_("Process priority"),
      N_("How much processor time the system gives VLC compared with the other programs.") },
    { "redesign/double-click-action", 'i', 1, 0, 1, N_("Double click on the video"),
      N_("What a double click on the video does."), N_("Nothing|Toggle fullscreen") },
    { "redesign/subtitle-size-mode", 'i', 0, 0, 1, N_("Size values"),
      N_("Choose the size of the subtitles by name (relative to the picture), or in pixels."),
      N_("Relative|Pixels") },
    { "redesign/subtitle-background-opacity", 'i', 128, 0, 255, N_("Background opacity"),
      N_("How opaque the box behind the subtitles is when there is one (kept while there is none).") },
    { "redesign/recentplay-filter", 's', 0, 0, 0, N_("Never remember items matching"),
      N_("A regular expression: the items whose address or name matches it are not saved "
         "in the history, nor their playback position.") },
};

// "Default optical drive": the CD / DVD drives of the computer ("-" when
// there is none); a drive of the user that is not listed is kept
QVariantList opticalDriveChoices(const QString &current)
{
    const auto choice = [](const QString &value, const QString &label) {
        return QVariant(QVariantMap{{QStringLiteral("value"), value}, {QStringLiteral("label"), label}});
    };
    QVariantList choices;
    bool listed = false;
#ifdef _WIN32
    const DWORD drives = GetLogicalDrives();
    for (int i = 0; i < 26; ++i)
    {
        if (!(drives & (1u << i)))
            continue;
        const wchar_t root[] = { wchar_t(L'A' + i), L':', L'\\', 0 };
        if (GetDriveTypeW(root) != DRIVE_CDROM)
            continue;
        const QString drive = QString(QChar('A' + i)) + QLatin1Char(':');
        choices.append(choice(drive, drive));
        listed = listed || drive.compare(current, Qt::CaseInsensitive) == 0;
    }
#endif
    if (!listed)
        choices.prepend(choice(current, current.isEmpty() ? QStringLiteral("-") : current));
    return choices;
}

// Encoder presets and tunings of x264: "" (the default of the encoder) is
// the Medium preset and no tuning
QVariantList x264Choices(const char *name, const QString &current)
{
    const auto choice = [](const QString &value, const QString &label) {
        return QVariant(QVariantMap{{QStringLiteral("value"), value}, {QStringLiteral("label"), label}});
    };
    const bool preset = QByteArray(name).endsWith("preset");
    QVariantList choices;
    if (!preset)
        choices.append(choice(QString(), qtr("None")));
    static const struct { const char *value; const char *label; } presets[] = {
        { "ultrafast", N_("Ultra fast") }, { "superfast", N_("Super fast") },
        { "veryfast", N_("Very fast") }, { "faster", N_("Faster") }, { "fast", N_("Fast") },
        { "", N_("Medium") }, { "slow", N_("Slow") }, { "slower", N_("Slower") },
        { "veryslow", N_("Very slow") }, { "placebo", N_("Placebo") },
    }, tunes[] = {
        { "film", N_("Film") }, { "animation", N_("Animation") }, { "grain", N_("Grain") },
        { "stillimage", N_("Still image") }, { "psnr", "PSNR" }, { "ssim", "SSIM" },
        { "fastdecode", N_("Fast decoding") }, { "zerolatency", N_("Zero latency") },
    };
    bool listed = false;
    const auto *items = preset ? presets : tunes;
    const size_t count = preset ? std::size(presets) : std::size(tunes);
    for (size_t i = 0; i < count; ++i)
    {
        const auto &item = items[i];
        choices.append(choice(QString::fromLatin1(item.value), qtr(item.label)));
        listed = listed || current == QLatin1String(item.value);
    }
    if (!listed && !current.isEmpty())
        choices.append(choice(current, current));
    return choices;
}

} // namespace

MainCtx *ConfigModel::ctx() const { return m_ctx.data(); }

void ConfigModel::setCtx(MainCtx *ctx)
{
    if (m_ctx == ctx)
        return;
    m_ctx = ctx;
    emit ctxChanged();
    reload();
}

void ConfigModel::setCategory(const QString &category)
{
    if (m_category == category)
        return;
    m_category = category;
    emit categoryChanged();
    rebuildRows();
}

void ConfigModel::setSearch(const QString &search)
{
    if (m_search == search)
        return;
    const bool wasSearching = !m_search.isEmpty();
    m_search = search;
    emit searchChanged();
    // the rows only change when a search starts or ends (the rows shown
    // again); the view matches the text without recreating every row
    if (wasSearching != !m_search.isEmpty())
        rebuildRows();
}

bool ConfigModel::dirty() const
{
    for (const Entry &entry : m_entries)
        if (entry.staged != entry.current)
            return true;
    return false;
}

QStringList ConfigModel::categories() const
{
    QStringList result;
    for (const Entry &entry : m_entries)
        if (!result.contains(entry.category))
            result.append(entry.category);
    return result;
}

int ConfigModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_rows.size();
}

QVariant ConfigModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return {};
    const Entry &entry = m_entries[m_rows[index.row()]];
    switch (role) {
    case KeyRole: return entry.key;
    case CategoryRole: return entry.category;
    case LabelRole: return entry.label;
    case HelpRole: return entry.help;
    case TypeRole: return entry.type;
    case ValueRole: return entry.staged;
    case CurrentRole: return entry.current;
    case DefaultRole: return entry.original;
    case MinimumRole: return entry.minimum;
    case MaximumRole: return entry.maximum;
    case ChoicesRole: return entry.choices;
    case DirtyRole: return entry.staged != entry.current;
    case PendingRestartRole: return entry.staged != runningValue(entry);
    default: return {};
    }
}

QHash<int, QByteArray> ConfigModel::roleNames() const
{
    return {{KeyRole, "key"}, {CategoryRole, "category"},
            {LabelRole, "label"}, {HelpRole, "help"}, {TypeRole, "type"},
            {ValueRole, "value"}, {CurrentRole, "currentValue"},
            {DefaultRole, "defaultValue"}, {MinimumRole, "minimum"},
            {MaximumRole, "maximum"}, {ChoicesRole, "choices"},
            {DirtyRole, "dirty"}, {PendingRestartRole, "pendingRestart"}};
}

QString ConfigModel::keyAt(int visibleRow) const
{
    if (visibleRow < 0 || visibleRow >= m_rows.size())
        return {};
    return m_entries[m_rows[visibleRow]].key;
}

QString ConfigModel::labelAt(int visibleRow) const
{
    if (visibleRow < 0 || visibleRow >= m_rows.size())
        return {};
    return m_entries[m_rows[visibleRow]].label;
}

QString ConfigModel::helpAt(int visibleRow) const
{
    if (visibleRow < 0 || visibleRow >= m_rows.size())
        return {};
    return m_entries[m_rows[visibleRow]].help;
}

// What VLC runs with until it restarts, for the settings read at the start
// only (a process lives as long as its settings)
QHash<QString, QVariant> &ConfigModel::startValues()
{
    static QHash<QString, QVariant> values;
    return values;
}

QVariant ConfigModel::runningValue(const Entry &entry) const
{
    const auto it = startValues().constFind(entry.key);
    return it != startValues().cend() ? it.value() : entry.current;
}

bool ConfigModel::visible(const Entry &entry) const
{
    if (!m_category.isEmpty() && entry.category != m_category)
        return false;
    // a search lists each setting once; the view matches the text against
    // the labels of the design (the keys and the texts of VLC are not what
    // the page shows: "Shuffle" is "random", every key of the interface
    // starts with "qt")
    return m_search.isEmpty() || !entry.key.startsWith(QLatin1String(mirrorPrefix));
}

void ConfigModel::rebuildRows()
{
    beginResetModel();
    m_rows.clear();
    for (int i = 0; i < m_entries.size(); ++i)
        if (visible(m_entries[i]))
            m_rows.append(i);
    endResetModel();
}

void ConfigModel::setError(const QString &error)
{
    if (m_error == error)
        return;
    m_error = error;
    emit errorChanged();
}

void ConfigModel::notifyEntry(int entryIndex)
{
    ++m_revision;
    emit revisionChanged();
    const int row = m_rows.indexOf(entryIndex);
    if (row >= 0)
        emit dataChanged(index(row), index(row), {ValueRole, DirtyRole, PendingRestartRole});
}

QVariant ConfigModel::readValue(const Entry &entry)
{
    // Repeat as the core reads it at the start (PlaylistConfigureFromVariables
    // in libvlc.c): two booleans, "repeat" (the current item: One) first,
    // then "loop" (the whole play queue: All); the play queue writes them so
    // (PlaylistController)
    if (entry.key == QLatin1String(repeatKey))
        return QVariant::fromValue<int64_t>(config_GetInt("repeat") != 0 ? 1
                                            : config_GetInt("loop") != 0 ? 2 : 0);
    if (entry.key == QLatin1String(playbackEndKey))
        return QVariant::fromValue<int64_t>(config_GetInt("play-and-exit") ? 1 : 0);
    if (entry.key == QLatin1String(seekPreciseKey))
        return config_GetInt("input-fast-seek") == 0;
    if (entry.key == QLatin1String(postprocKey)) {
        char *filters = config_GetPsz("video-filter");
        const bool running = listHas(QString::fromUtf8(filters ? filters : ""), QStringLiteral("postproc"));
        free(filters);
        return QVariant::fromValue<int64_t>(running ? config_GetInt("postproc-q") : 0);
    }
    if (entry.key == QLatin1String(visualSizeKey)) {
        const int64_t width = config_GetInt("effect-width"), height = config_GetInt("effect-height");
        for (size_t preset = 0; preset < std::size(visualSizes); ++preset)
            if (visualSizes[preset][0] == width && visualSizes[preset][1] == height)
                return QVariant::fromValue<int64_t>(int64_t(preset));
        return QVariant::fromValue<int64_t>(int64_t(std::size(visualSizes)));
    }
    if (entry.key == QLatin1String(rememberPositionKey))
        return config_GetInt("restore-playback-pos") != 0;
    if (entry.key.startsWith(QLatin1String(filterPrefix)) || entry.key == QLatin1String(nightModeKey)) {
        char *filters = config_GetPsz("audio-filter");
        const QString list = QString::fromUtf8(filters ? filters : "");
        free(filters);
        if (entry.key == QLatin1String(nightModeKey))
            return listHas(list, QStringLiteral("compressor"))
                   && config_GetFloat("compressor-ratio") == nightRatio
                   && config_GetFloat("compressor-threshold") == nightThreshold;
        return listHas(list, entry.key.mid(int(strlen(filterPrefix))));
    }
    if (entry.key == QLatin1String(fetchMetadataKey))
        return QVariant::fromValue<int64_t>(config_GetInt("metadata-network-access") ? 1 : 0);
    if (entry.key == QLatin1String(bufferingKey)) {
        int64_t delays[4];
        for (int i = 0; i < 4; ++i)
            delays[i] = config_GetInt(cachingKeys[i]);
        return QVariant::fromValue(bufferingPreset(delays));
    }
    if (entry.key == QLatin1String(subShadowKey))
        return config_GetInt("freetype-shadow-opacity") != 0;
    if (entry.key == QLatin1String(subBackgroundKey))
        return QVariant::fromValue<int64_t>(config_GetInt("freetype-background-opacity") != 0 ? 1 : 0);
    if (entry.key == QLatin1String(wheelReverseKey))
        return config_GetInt("hotkeys-y-wheel-mode") == 3 || config_GetInt("hotkeys-x-wheel-mode") == 3;
    if (entry.key == QLatin1String(webRemoteKey)) {
        char *interfaces = config_GetPsz("extraintf");
        const bool http = hasHttp(QString::fromUtf8(interfaces ? interfaces : ""));
        free(interfaces);
        return http;
    }
    if (entry.key == QLatin1String(customSkinKey)) {
        char *intf = config_GetPsz("intf");
        const bool skins = intf && !strcmp(intf, "skins2");
        free(intf);
        return skins;
    }
    const QByteArray key = entry.key.toUtf8();
    if (entry.type == QLatin1String("boolean"))
        return config_GetInt(key.constData()) != 0;
    if (entry.type == QLatin1String("integer"))
        return QVariant::fromValue(config_GetInt(key.constData()));
    if (entry.type == QLatin1String("float"))
        return config_GetFloat(key.constData());
    char *value = config_GetPsz(key.constData());
    const QString result = QString::fromUtf8(value ? value : "");
    free(value);
    return result;
}

bool ConfigModel::normalize(const Entry &entry, const QVariant &value,
                            QVariant *normalized)
{
    if (!value.isValid() || value.isNull())
        return false;
    if (entry.type == QLatin1String("boolean")) {
        const QString text = value.toString().toLower();
        if (text != QLatin1String("true") && text != QLatin1String("false")
            && text != QLatin1String("0") && text != QLatin1String("1"))
            return false;
        *normalized = text == QLatin1String("true") || text == QLatin1String("1");
    } else if (entry.type == QLatin1String("integer")) {
        bool ok = false;
        const qlonglong number = value.toString().toLongLong(&ok);
        if (!ok || (entry.bounded && (number < entry.minimum.toLongLong()
                                      || number > entry.maximum.toLongLong())))
            return false;
        *normalized = QVariant::fromValue(static_cast<int64_t>(number));
    } else if (entry.type == QLatin1String("float")) {
        bool ok = false;
        const double number = value.toString().toDouble(&ok);
        if (!ok || !std::isfinite(number)
            || number < -std::numeric_limits<float>::max()
            || number > std::numeric_limits<float>::max()
            || (entry.bounded && (number < entry.minimum.toDouble()
                                  || number > entry.maximum.toDouble())))
            return false;
        *normalized = static_cast<float>(number);
    } else {
        *normalized = value.toString();
    }
    if (!entry.choices.isEmpty()) {
        bool found = false;
        for (const QVariant &choice : entry.choices)
            if (choice.toMap().value(QStringLiteral("value")) == *normalized) {
                found = true;
                break;
            }
        if (!found)
            return false;
    }
    return true;
}

int ConfigModel::indexOfKey(const QString &key) const
{
    for (int i = 0; i < m_entries.size(); ++i)
        if (m_entries[i].key == key)
            return i;
    return -1;
}

// Remember the playback position: Continue where you left off is not Never.
// Reverse scroll direction: the wheels that seek go backwards (VLC reverses
// seeking only). Web remote control: the http interface is loaded.
void ConfigModel::syncDerived()
{
    const int position = indexOfKey(QStringLiteral("restore-playback-pos"));
    const int wheelY = indexOfKey(QStringLiteral("hotkeys-y-wheel-mode"));
    const int wheelX = indexOfKey(QStringLiteral("hotkeys-x-wheel-mode"));
    const int extra = indexOfKey(QStringLiteral("extraintf"));
    const int filters = indexOfKey(QStringLiteral("audio-filter"));
    for (int i = 0; i < m_entries.size(); ++i) {
        Entry &entry = m_entries[i];
        if (entry.proxyFor.isEmpty())
            continue;
        const QVariant staged = entry.staged, current = entry.current;
        if (entry.key == QLatin1String(rememberPositionKey) && position >= 0) {
            entry.current = m_entries[position].current.toLongLong() != 0;
            entry.staged = m_entries[position].staged.toLongLong() != 0;
        } else if (entry.key == QLatin1String(wheelReverseKey) && wheelY >= 0 && wheelX >= 0) {
            const QVariant &y = m_entries[wheelY].staged, &x = m_entries[wheelX].staged;
            const QVariant &cy = m_entries[wheelY].current, &cx = m_entries[wheelX].current;
            // with no wheel seeking, the toggle keeps the state chosen
            if (y.toLongLong() >= 2 || x.toLongLong() >= 2)
                entry.staged = y.toLongLong() == 3 || x.toLongLong() == 3;
            if (cy.toLongLong() >= 2 || cx.toLongLong() >= 2)
                entry.current = cy.toLongLong() == 3 || cx.toLongLong() == 3;
            else
                entry.current = entry.staged;
        } else if (entry.key.startsWith(QLatin1String(filterPrefix)) && filters >= 0) {
            const QString module = entry.key.mid(int(strlen(filterPrefix)));
            entry.current = listHas(m_entries[filters].current, module);
            entry.staged = listHas(m_entries[filters].staged, module);
        } else if (entry.key == QLatin1String(nightModeKey) && filters >= 0) {
            const auto night = [&](bool staged) {
                const auto value = [&](const char *key) {
                    const int index = indexOfKey(QLatin1String(key));
                    return index < 0 ? 0.f
                                     : (staged ? m_entries[index].staged : m_entries[index].current).toFloat();
                };
                return listHas(staged ? m_entries[filters].staged : m_entries[filters].current,
                               QStringLiteral("compressor"))
                       && value("compressor-ratio") == nightRatio
                       && value("compressor-threshold") == nightThreshold;
            };
            entry.current = night(false);
            entry.staged = night(true);
        } else if (entry.key == QLatin1String(bufferingKey)) {
            int64_t current[4], staged[4];
            bool complete = true;
            for (int k = 0; k < 4; ++k) {
                const int caching = indexOfKey(QLatin1String(cachingKeys[k]));
                complete = complete && caching >= 0;
                current[k] = caching >= 0 ? m_entries[caching].current.toLongLong() : 0;
                staged[k] = caching >= 0 ? m_entries[caching].staged.toLongLong() : 0;
            }
            if (complete) {
                entry.current = QVariant::fromValue(bufferingPreset(current));
                // Custom chosen stays Custom while the delays are edited
                const int64_t preset = bufferingPreset(staged);
                if (entry.staged.toLongLong() != int64_t(std::size(bufferingPresets))
                    || preset == int64_t(std::size(bufferingPresets)))
                    entry.staged = QVariant::fromValue(preset);
            }
        } else if (entry.key == QLatin1String(fetchMetadataKey)) {
            const int network = indexOfKey(QStringLiteral("metadata-network-access"));
            if (network >= 0) {
                entry.current = QVariant::fromValue<int64_t>(m_entries[network].current.toBool() ? 1 : 0);
                entry.staged = QVariant::fromValue<int64_t>(m_entries[network].staged.toBool() ? 1 : 0);
            }
        } else if (entry.key == QLatin1String(subShadowKey) || entry.key == QLatin1String(subBackgroundKey)) {
            const int opacity = indexOfKey(entry.key == QLatin1String(subShadowKey)
                                           ? QStringLiteral("freetype-shadow-opacity")
                                           : QStringLiteral("freetype-background-opacity"));
            if (opacity >= 0) {
                const bool current = m_entries[opacity].current.toLongLong() != 0;
                const bool staged = m_entries[opacity].staged.toLongLong() != 0;
                if (entry.type == QLatin1String("boolean")) {
                    entry.current = current;
                    entry.staged = staged;
                } else {
                    entry.current = QVariant::fromValue<int64_t>(current ? 1 : 0);
                    entry.staged = QVariant::fromValue<int64_t>(staged ? 1 : 0);
                }
            }
        } else if (entry.key == QLatin1String(webRemoteKey) && extra >= 0) {
            entry.current = hasHttp(m_entries[extra].current);
            entry.staged = hasHttp(m_entries[extra].staged);
        }
        if (entry.staged != staged || entry.current != current)
            notifyEntry(i);
    }
    // the rows shown again: as the entries they show (after those)
    for (int i = 0; i < m_entries.size(); ++i) {
        Entry &entry = m_entries[i];
        if (!entry.key.startsWith(QLatin1String(mirrorPrefix)))
            continue;
        const int mirrored = indexOfKey(entry.proxyFor);
        if (mirrored < 0 || (entry.staged == m_entries[mirrored].staged
                             && entry.current == m_entries[mirrored].current))
            continue;
        entry.current = m_entries[mirrored].current;
        entry.staged = m_entries[mirrored].staged;
        notifyEntry(i);
    }
}

bool ConfigModel::setDerived(int index, const QVariant &value)
{
    Entry &entry = m_entries[index];
    const bool on = value.toBool() || value.toString() == QLatin1String("true");
    if (entry.key == QLatin1String(bufferingKey)) {
        // a preset: its four delays; Custom: the delays stay
        const qlonglong preset = value.toLongLong();
        if (preset >= 0 && preset < qlonglong(std::size(bufferingPresets))) {
            for (int i = 0; i < 4; ++i)
                if (indexOfKey(QLatin1String(cachingKeys[i])) >= 0)
                    setValue(QLatin1String(cachingKeys[i]),
                             QVariant::fromValue<int64_t>(bufferingPresets[preset][i]));
        } else {
            entry.staged = QVariant::fromValue<int64_t>(qlonglong(std::size(bufferingPresets)));
            notifyEntry(index);
        }
        syncDerived();
        return true;
    }
    if (entry.key == QLatin1String(rememberPositionKey)) {
        // On: Ask, unless the option already says Always
        const qlonglong position = valueFor(QStringLiteral("restore-playback-pos")).toLongLong();
        return setValue(QStringLiteral("restore-playback-pos"),
                        QVariant::fromValue<int64_t>(on ? (position ? position : 1) : 0));
    }
    if (entry.key == QLatin1String(wheelReverseKey)) {
        const bool wasDirty = dirty();
        entry.staged = on;
        notifyEntry(index);
        for (const char *wheel : { "hotkeys-y-wheel-mode", "hotkeys-x-wheel-mode" }) {
            const qlonglong mode = valueFor(QLatin1String(wheel)).toLongLong();
            if (mode >= 2)
                setValue(QLatin1String(wheel), QVariant::fromValue<int64_t>(on ? 3 : 2));
        }
        syncDerived();
        if (wasDirty != dirty())
            emit dirtyChanged();
        return true;
    }
    if (entry.key.startsWith(QLatin1String(filterPrefix)))
        return setFilter(entry.key.mid(int(strlen(filterPrefix))), on);
    if (entry.key == QLatin1String(fetchMetadataKey))
        return setValue(QStringLiteral("metadata-network-access"), on);
    if (entry.key == QLatin1String(subShadowKey) || entry.key == QLatin1String(subBackgroundKey)) {
        // On: the opacity of VLC (or a half one for the background)
        const QString key = entry.key == QLatin1String(subShadowKey) ? QStringLiteral("freetype-shadow-opacity")
                                                                     : QStringLiteral("freetype-background-opacity");
        const int opacity = indexOfKey(key);
        if (opacity < 0)
            return false;
        const qlonglong original = m_entries[opacity].original.toLongLong();
        qlonglong onValue = original ? original : 128;
        // the background: the opacity kept while there was none
        const int kept = indexOfKey(QLatin1String(subBackgroundOpacityKey));
        if (entry.key == QLatin1String(subBackgroundKey) && kept >= 0 && m_entries[kept].staged.toLongLong() > 0)
            onValue = m_entries[kept].staged.toLongLong();
        return setValue(key, QVariant::fromValue<int64_t>(on ? onValue : 0));
    }
    if (entry.key == QLatin1String(nightModeKey)) {
        // On: the compressor set for the night, what it was kept (in the
        // settings of the interface: Night mode may be turned off in another
        // session); off: the compressor as it was (on or off, its values),
        // else VLC's defaults
        const std::pair<const char *, float> night[] = {
            { "compressor-ratio", nightRatio }, { "compressor-threshold", nightThreshold },
            { "compressor-makeup-gain", nightMakeupGain } };
        const QString compressor = QStringLiteral("compressor");
        QVariantMap previous;
        if (m_ctx)
            previous = m_ctx->settingValue(QLatin1String(nightPreviousKey), QVariantMap()).toMap();
        if (on && !entry.staged.toBool() && m_ctx) {
            previous.clear();
            for (const auto &pair : night) {
                const int i = indexOfKey(QLatin1String(pair.first));
                if (i >= 0)
                    previous.insert(QLatin1String(pair.first), m_entries[i].staged);
            }
            previous.insert(compressor, listHas(valueFor(QStringLiteral("audio-filter")), compressor));
            m_ctx->setSettingValue(QLatin1String(nightPreviousKey), previous);
        }
        // kept while Night mode was already on (its values): nothing to put back
        if (previous.value(QStringLiteral("compressor-ratio")).toFloat() == nightRatio
            && previous.value(QStringLiteral("compressor-threshold")).toFloat() == nightThreshold)
            previous.clear();
        for (const auto &[key, value] : night) {
            const int i = indexOfKey(QLatin1String(key));
            if (i < 0)
                continue;
            const QVariant kept = previous.value(QLatin1String(key));
            setValue(QLatin1String(key), on ? QVariant(value) : kept.isValid() ? kept : m_entries[i].original);
        }
        return setFilter(compressor, on || previous.value(compressor, false).toBool());
    }
    if (entry.key.startsWith(QLatin1String(mirrorPrefix))) {
        const QString mirrored = entry.proxyFor;
        const bool done = setValue(mirrored, value);
        syncDerived();
        return done;
    }
    if (entry.key == QLatin1String(webRemoteKey)) {
        QStringList interfaces = valueFor(QStringLiteral("extraintf")).toString()
                                     .split(':', Qt::SkipEmptyParts);
        interfaces.removeAll(QStringLiteral("http"));
        if (on)
            interfaces.append(QStringLiteral("http"));
        return setValue(QStringLiteral("extraintf"), interfaces.join(':'));
    }
    return false;
}

bool ConfigModel::setFilter(const QString &module, bool on)
{
    QStringList filters = valueFor(QStringLiteral("audio-filter")).toString().split(':', Qt::SkipEmptyParts);
    filters.removeAll(module);
    if (on)
        filters.append(module);
    return setValue(QStringLiteral("audio-filter"), filters.join(':'));
}

bool ConfigModel::setValue(const QString &key, const QVariant &value)
{
    for (int i = 0; i < m_entries.size(); ++i) {
        Entry &entry = m_entries[i];
        if (entry.key != key)
            continue;
        if (!entry.proxyFor.isEmpty())
            return setDerived(i, value);
        QVariant normalized;
        if (!normalize(entry, value, &normalized)) {
            setError(qtr("%1: this value can not be used.").arg(displayName(key)));
            return false;
        }
        // Seek on a wheel: backwards when Reverse scroll direction is on
        if ((key == QLatin1String("hotkeys-y-wheel-mode") || key == QLatin1String("hotkeys-x-wheel-mode"))
            && normalized.toLongLong() >= 2) {
            const int reverse = indexOfKey(QLatin1String(wheelReverseKey));
            if (reverse >= 0)
                normalized = QVariant::fromValue<int64_t>(m_entries[reverse].staged.toBool() ? 3 : 2);
        }
        const bool wasDirty = dirty();
        entry.staged = normalized;
        notifyEntry(i);
        if (key == QLatin1String("sub-position"))
            showSubtitlePosition(m_ctx, normalized.toLongLong());
        if (key == QLatin1String("pitch-shift"))
            setFilter(QStringLiteral("scaletempo_pitch"), normalized.toFloat() != 0.f);
        // Size values: the size of the other way back to its default (both
        // apply together in VLC: pixels times the scale)
        if (key == QLatin1String("redesign/subtitle-size-mode")) {
            if (normalized.toLongLong() == 1 && indexOfKey(QStringLiteral("sub-text-scale")) >= 0)
                setValue(QStringLiteral("sub-text-scale"), QVariant::fromValue<int64_t>(100));
            else if (normalized.toLongLong() == 0 && indexOfKey(QStringLiteral("freetype-fontsize")) >= 0)
                setValue(QStringLiteral("freetype-fontsize"), QVariant::fromValue<int64_t>(0));
        }
        // the kept background opacity applies while there is a background
        // (0: no background any more)
        if (key == QLatin1String(subBackgroundOpacityKey)) {
            const int background = indexOfKey(QStringLiteral("freetype-background-opacity"));
            if (background >= 0 && m_entries[background].staged.toLongLong() != 0)
                setValue(QStringLiteral("freetype-background-opacity"), normalized);
        }
        if (key == QLatin1String("equalizer-preset")) {
            const QVariantList preset = equalizerPreset(normalized.toString());
            if (indexOfKey(QStringLiteral("equalizer-bands")) >= 0)
                setValue(QStringLiteral("equalizer-bands"), QString());
            if (!preset.isEmpty() && indexOfKey(QStringLiteral("equalizer-preamp")) >= 0)
                setValue(QStringLiteral("equalizer-preamp"), preset.first());
        }
        syncDerived();
        if (wasDirty != dirty())
            emit dirtyChanged();
        setError({});
        return true;
    }
    setError(qtr("%1 is not available in this VLC installation.").arg(displayName(key)));
    return false;
}

QString ConfigModel::displayName(const QString &key) const
{
    // the name of the row, else the name VLC gives the option
    const int index = indexOfKey(key);
    if (index >= 0 && !m_entries[index].label.isEmpty())
        return m_entries[index].label;
    const QByteArray name = key.toUtf8();
    const module_config_t *option = config_FindConfig(name.constData());
    if (option && option->psz_text && *option->psz_text)
        return qfut(option->psz_text);
    return qtr("This setting");
}

bool ConfigModel::setDirectoryUrl(const QString &key, const QUrl &url)
{
    for (const Entry &entry : m_entries) {
        if (entry.key != key)
            continue;
        if (entry.type != QLatin1String("directory") || !url.isLocalFile()
            || url.toLocalFile().isEmpty()) {
            setError(qtr("%1: choose a folder of this computer.").arg(displayName(key)));
            return false;
        }
        return setValue(key, url.toLocalFile());
    }
    setError(qtr("%1 is not available in this VLC installation.").arg(displayName(key)));
    return false;
}

void ConfigModel::cancel()
{
    const bool wasDirty = dirty();
    for (int i = 0; i < m_entries.size(); ++i) {
        Entry &entry = m_entries[i];
        if (entry.staged == entry.current)
            continue;
        entry.staged = entry.current;
        notifyEntry(i);
        if (entry.key == QLatin1String("sub-position"))
            showSubtitlePosition(m_ctx, entry.current.toLongLong());
    }
    if (wasDirty)
        emit dirtyChanged();
    setError({});
}

void ConfigModel::resetCategory()
{
    if (m_category.isEmpty()) {
        setError(qtr("Select a category to reset."));
        return;
    }
    const bool wasDirty = dirty();
    for (int i = 0; i < m_entries.size(); ++i) {
        Entry &entry = m_entries[i];
        if (entry.category != m_category || entry.staged == entry.original
            || !entry.proxyFor.isEmpty()
            || entry.key == QLatin1String("extraintf") || entry.key == QLatin1String("control"))
            continue;
        entry.staged = entry.original;
        notifyEntry(i);
    }
    syncDerived();
    if (wasDirty != dirty())
        emit dirtyChanged();
    if (m_category == QLatin1String("privacy") && audioscrobblerAvailable())
        setAudioscrobblerEnabled(false);
    setError({});
}

bool ConfigModel::apply()
{
    return applyWithEffects(nullptr, nullptr);
}

// Repeat (Off / One / All) stands for "repeat" and "loop", When playback
// finishes (Stop / Quit VLC) for "play-and-exit", Seek precisely for the
// inverse of "input-fast-seek", as in the design
void ConfigModel::addCombinedEntries()
{
    const auto indexOf = [this](const char *key) {
        for (int i = 0; i < m_entries.size(); ++i)
            if (m_entries[i].key == QLatin1String(key))
                return i;
        return -1;
    };
    const auto choice = [](int64_t value, const QString &label) {
        return QVariant(QVariantMap{{QStringLiteral("value"), QVariant::fromValue(value)},
                                    {QStringLiteral("label"), label}});
    };
    const auto combined = [](const char *key, const QString &category, const QString &label,
                             const QString &help, const QVariantList &choices) {
        Entry entry;
        entry.key = QLatin1String(key);
        entry.category = category;
        entry.label = label;
        entry.help = help;
        entry.type = QStringLiteral("integer");
        entry.original = QVariant::fromValue<int64_t>(0);
        entry.choices = choices;
        entry.current = readValue(entry);
        entry.staged = entry.current;
        return entry;
    };

    const int repeat = indexOf("repeat");
    const int loop = indexOf("loop");
    if (repeat >= 0 && loop >= 0)
    {
        const int first = std::min(repeat, loop);
        m_entries[first] = combined(repeatKey, m_entries[first].category, qtr("Repeat"),
                                    qtr("Repeat the current item (One) or the whole play queue (All)."),
                                    { choice(0, qtr("Off")), choice(1, qtr("One")), choice(2, qtr("All")) });
        m_entries.remove(std::max(repeat, loop));
    }

    const int stop = indexOf("play-and-stop");
    if (stop >= 0 && config_FindConfig("play-and-exit"))
        m_entries.insert(stop + 1, combined(playbackEndKey, m_entries[stop].category,
                                            qtr("When playback finishes"),
                                            qtr("Stop, or quit VLC at the end of the play queue."),
                                            { choice(0, qtr("Stop")), choice(1, qtr("Quit VLC")) }));

    // "Use a custom skin": the skins interface instead of this one (from the
    // next start)
    const int skinFile = indexOf("skins2-last");
    if (skinFile >= 0 && config_FindConfig("intf"))
    {
        Entry entry = combined(customSkinKey, m_entries[skinFile].category, qtr("Use a custom skin"),
                               qtr("Starts VLC with the skin chosen below instead of this interface."),
                               {});
        entry.type = QStringLiteral("boolean");
        entry.original = false;
        entry.current = readValue(entry);
        entry.staged = entry.current;
        m_entries.insert(skinFile, entry);
    }

    // "Remember the playback position" of the privacy: Continue where you
    // left off (Never / Ask / Always of the playback page) is not Never
    const int recent = indexOf("save-recentplay");
    const int position = indexOf("restore-playback-pos");
    if (recent >= 0 && position >= 0)
    {
        Entry entry = combined(rememberPositionKey, m_entries[recent].category,
                               qtr("Remember the playback position"),
                               qtr("Lets VLC continue the media where you left off."), {});
        entry.type = QStringLiteral("boolean");
        entry.proxyFor = m_entries[position].key;
        entry.original = m_entries[position].original.toLongLong() != 0;
        entry.current = m_entries[position].current.toLongLong() != 0;
        entry.staged = m_entries[position].staged.toLongLong() != 0;
        m_entries.insert(recent + 1, entry);
    }

    // "Reverse scroll direction" after the wheels
    const int wheelY = indexOf("hotkeys-y-wheel-mode");
    const int wheelX = indexOf("hotkeys-x-wheel-mode");
    if (wheelY >= 0 && wheelX >= 0)
    {
        Entry entry = combined(wheelReverseKey, m_entries[wheelX].category, qtr("Reverse scroll direction"),
                               qtr("The wheels that seek go backwards when turned forwards."), {});
        entry.type = QStringLiteral("boolean");
        entry.proxyFor = m_entries[wheelY].key;
        entry.original = false;
        entry.current = m_entries[wheelY].current.toLongLong() == 3
                        || m_entries[wheelX].current.toLongLong() == 3;
        entry.staged = entry.current;
        m_entries.insert(std::max(wheelY, wheelX) + 1, entry);
    }

    // "Web remote control" before its port: the http interface
    const int port = indexOf("http-port");
    const int extra = indexOf("extraintf");
    if (port >= 0 && extra >= 0)
    {
        Entry entry = combined(webRemoteKey, m_entries[port].category, qtr("Web remote control"),
                               qtr("Lets a web browser control VLC. Set its password in the Advanced Options."), {});
        entry.type = QStringLiteral("boolean");
        entry.proxyFor = m_entries[extra].key;
        entry.original = false;
        entry.current = hasHttp(m_entries[extra].current);
        entry.staged = entry.current;
        m_entries.insert(port, entry);
    }

    // (No "Ignore styling in ASS and SSA subtitles": VLC has no such option
    // any more; "subsdec-formatted" is read by the USF decoder only, libass
    // draws the ASS and SSA subtitles with their styles)

    // "Video post-processing": the level of the filter, Off when it does not
    // run (the quality option alone reads "Highest")
    const int postprocQuality = indexOf("postproc-q");
    if (postprocQuality >= 0 && config_FindConfig("video-filter"))
    {
        Entry entry = combined(postprocKey, m_entries[postprocQuality].category, qtr("Video post-processing"),
                               qtr("Improves the picture of poorly encoded videos, at the cost of processing power."),
                               {});
        entry.choices = presetChoices(QStringLiteral("postproc-q"), entry.current);
        m_entries.insert(postprocQuality, entry);
    }

    // "Visualization size" after the media role
    const int role = indexOf("role");
    if (role >= 0 && config_FindConfig("effect-width") && config_FindConfig("effect-height"))
    {
        Entry entry = combined(visualSizeKey, m_entries[role].category, qtr("Visualization size"),
                               qtr("The size the visualizations are drawn at."),
                               { choice(0, qtr("Match window")), choice(1, QStringLiteral("1280 \u00d7 720")),
                                 choice(2, QStringLiteral("1920 \u00d7 1080")) });
        if (entry.current.toLongLong() == int64_t(std::size(visualSizes)))
            entry.choices.append(choice(int64_t(std::size(visualSizes)),
                                        QStringLiteral("%1 \u00d7 %2").arg(config_GetInt("effect-width"))
                                                                     .arg(config_GetInt("effect-height"))));
        m_entries.insert(role + 1, entry);
    }

    // Rows of the design shown again elsewhere, mirrors of their entries:
    // the scroll rows under Remotes after the port (Figma 6536:14482), the
    // time-shift buffer size after the pixel aspect ratio of the video
    // (3073:15633), the visualization size after the monospace font of the
    // subtitles (6536:14229)
    const struct { const char *after; const char *keys[3]; } mirrors[] = {
        { "http-port", { "hotkeys-y-wheel-mode", "hotkeys-x-wheel-mode", wheelReverseKey } },
        { "monitor-par", { "input-timeshift-granularity", nullptr, nullptr } },
        { "freetype-monofont", { visualSizeKey, nullptr, nullptr } } };
    for (const auto &mirror : mirrors)
    {
        const int after = indexOf(mirror.after);
        if (after < 0)
            continue;
        int at = after + 1;
        for (const char *key : mirror.keys)
        {
            const int mirrored = key ? indexOf(key) : -1;
            if (mirrored < 0)
                continue;
            Entry entry = m_entries[mirrored];
            entry.key = QLatin1String(mirrorPrefix) + QLatin1String(key);
            entry.category = m_entries[after].category;
            entry.proxyFor = QLatin1String(key);
            entry.qtSetting = false;
            m_entries.insert(at++, entry);
        }
    }

    const int thumbnail = indexOf("qt-seek-thumbnail");
    if (thumbnail >= 0 && config_FindConfig("input-fast-seek"))
    {
        Entry entry = combined(seekPreciseKey, m_entries[thumbnail].category, qtr("Seek precisely"),
                               qtr("Seek to the exact time asked, slower than seeking to the nearest key frame."),
                               {});
        entry.type = QStringLiteral("boolean");
        entry.original = true;
        entry.current = readValue(entry);
        entry.staged = entry.current;
        m_entries.insert(thumbnail, entry);
    }
}

void ConfigModel::reload()
{
    const bool wasDirty = dirty();
    beginResetModel();
    m_entries.clear();
    m_rows.clear();
    if (m_ctx) {
        for (size_t i = 0; i < PrefsCatalog::count(); ++i) {
            const PrefsCatalog::Option &option = PrefsCatalog::options()[i];
            if (qstrncmp(option.name, "x:", 2) == 0) {
                // "x:x-filter:<module>", "x:x-night-mode": toggles of the
                // audio filters, where the module exists
                Entry entry;
                entry.key = QString::fromLatin1(option.name + 2);
                entry.category = QString::fromLatin1(option.category);
                entry.type = QStringLiteral("boolean");
                if (entry.key == QLatin1String(bufferingKey)) {
                    entry.proxyFor = QStringLiteral("network-caching");
                    entry.type = QStringLiteral("integer");
                    entry.original = QVariant::fromValue<int64_t>(2);
                    const char *const names[] = { N_("Lowest latency"), N_("Low"), N_("Normal"),
                                                  N_("High"), N_("Custom") };
                    for (int64_t preset = 0; preset < 5; ++preset)
                        entry.choices.append(QVariantMap{{QStringLiteral("value"), QVariant::fromValue(preset)},
                                                         {QStringLiteral("label"), qtr(names[preset])}});
                    entry.current = readValue(entry);
                    entry.staged = entry.current;
                    m_entries.append(entry);
                    continue;
                }
                if (entry.key == QLatin1String(fetchMetadataKey)) {
                    if (!config_FindConfig("metadata-network-access"))
                        continue;
                    entry.proxyFor = QStringLiteral("metadata-network-access");
                    entry.type = QStringLiteral("integer");
                    entry.original = QVariant::fromValue<int64_t>(0);
                    entry.choices = {
                        QVariantMap{{QStringLiteral("value"), QVariant::fromValue<int64_t>(0)},
                                    {QStringLiteral("label"), qtr("From the file only")}},
                        QVariantMap{{QStringLiteral("value"), QVariant::fromValue<int64_t>(1)},
                                    {QStringLiteral("label"), qtr("From the file and the internet")}} };
                    entry.current = readValue(entry);
                    entry.staged = entry.current;
                    m_entries.append(entry);
                    continue;
                }
                if (entry.key == QLatin1String(subShadowKey) || entry.key == QLatin1String(subBackgroundKey)) {
                    if (!config_FindConfig("freetype-shadow-opacity"))
                        continue;
                    entry.proxyFor = QStringLiteral("freetype");
                    if (entry.key == QLatin1String(subBackgroundKey)) {
                        entry.type = QStringLiteral("integer");
                        entry.original = QVariant::fromValue<int64_t>(0);
                        entry.choices = {
                            QVariantMap{{QStringLiteral("value"), QVariant::fromValue<int64_t>(0)},
                                        {QStringLiteral("label"), qtr("None")}},
                            QVariantMap{{QStringLiteral("value"), QVariant::fromValue<int64_t>(1)},
                                        {QStringLiteral("label"), qtr("Box")}} };
                    } else {
                        entry.original = true;
                    }
                    entry.current = readValue(entry);
                    entry.staged = entry.current;
                    m_entries.append(entry);
                    continue;
                }
                const QString module = entry.key.startsWith(QLatin1String(filterPrefix))
                    ? entry.key.mid(int(strlen(filterPrefix))) : QStringLiteral("compressor");
                if (!module_exists(module.toUtf8().constData()) || !config_FindConfig("audio-filter"))
                    continue;
                entry.proxyFor = QStringLiteral("audio-filter");
                entry.original = false;
                entry.current = readValue(entry);
                entry.staged = entry.current;
                m_entries.append(entry);
                continue;
            }
            if (qstrncmp(option.name, "qt:", 3) == 0) {
                for (const QtSettingOption &qtOption : qtSettingOptions) {
                    if (qstrcmp(qtOption.key, option.name + 3) != 0)
                        continue;
                    Entry entry;
                    entry.key = QString::fromLatin1(qtOption.key);
                    entry.category = QString::fromLatin1(option.category);
                    entry.label = qtr(qtOption.label);
                    entry.help = qtr(qtOption.help);
                    entry.qtSetting = true;
                    if (qtOption.type == 's') {
                        entry.type = QStringLiteral("string");
                        entry.original = QString();
                        entry.current = m_ctx->settingValue(entry.key, QString()).toString();
                    } else if (qtOption.type == 'b') {
                        entry.type = QStringLiteral("boolean");
                        entry.original = qtOption.defaultValue != 0;
                        entry.current = m_ctx->settingValue(entry.key, entry.original).toBool();
                    } else {
                        entry.type = QStringLiteral("integer");
                        entry.original = QVariant::fromValue<int64_t>(qtOption.defaultValue);
                        entry.bounded = true;
                        entry.minimum = QVariant::fromValue<int64_t>(qtOption.minimum);
                        entry.maximum = QVariant::fromValue<int64_t>(qtOption.maximum);
                        const qlonglong value = m_ctx->settingValue(entry.key, qtOption.defaultValue).toLongLong();
                        entry.current = QVariant::fromValue<int64_t>(
                            std::clamp<qlonglong>(value, qtOption.minimum, qtOption.maximum));
                    }
                    if (qtOption.choices) {
                        entry.bounded = false;
                        const QStringList labels = qtr(qtOption.choices).split('|');
                        for (int64_t value = 0; value < labels.size(); ++value)
                            entry.choices.append(QVariantMap{{QStringLiteral("value"), QVariant::fromValue(value)},
                                                             {QStringLiteral("label"), labels[value]}});
                    }
                    // Process priority: the classes of Windows by name, the
                    // usual ones first (-2 idle ... 2 high, see MainCtx)
                    if (entry.key == QLatin1String("redesign/process-priority")) {
                        entry.bounded = false;
                        const struct { int64_t value; const char *label; } priorities[] = {
                            { 0, N_("Normal") }, { 1, N_("Above normal") }, { 2, N_("High") },
                            { -1, N_("Below normal") }, { -2, N_("Idle") } };
                        for (const auto &priority : priorities)
                            entry.choices.append(QVariantMap{{QStringLiteral("value"), QVariant::fromValue(priority.value)},
                                                             {QStringLiteral("label"), qtr(priority.label)}});
                    }
                    entry.staged = entry.current;
                    m_entries.append(entry);
                }
                continue;
            }
            module_config_t *config = config_FindConfig(option.name);
            if (!config || !config->psz_text || !*config->psz_text)
                continue;
            Entry entry;
            switch (CONFIG_CLASS(config->i_type)) {
            case CONFIG_ITEM_BOOL: entry.type = QStringLiteral("boolean"); break;
            case CONFIG_ITEM_INTEGER: entry.type = QStringLiteral("integer"); break;
            case CONFIG_ITEM_FLOAT: entry.type = QStringLiteral("float"); break;
            case CONFIG_ITEM_STRING:
                entry.type = config->i_type == CONFIG_ITEM_DIRECTORY
                    ? QStringLiteral("directory") : QStringLiteral("string");
                break;
            default: continue;
            }
            entry.key = QString::fromLatin1(option.name);
            entry.category = QString::fromLatin1(option.category);
            entry.label = qfut(config->psz_text);
            entry.help = config->psz_longtext ? qfut(config->psz_longtext) : QString();
            if (entry.type == QLatin1String("boolean")) {
                entry.original = config->orig.i != 0;
            } else if (entry.type == QLatin1String("integer")) {
                entry.original = QVariant::fromValue(config->orig.i);
                entry.bounded = config->min.i < config->max.i;
                if (entry.bounded) {
                    entry.minimum = QVariant::fromValue(config->min.i);
                    entry.maximum = QVariant::fromValue(config->max.i);
                }
            } else if (entry.type == QLatin1String("float")) {
                entry.original = config->orig.f;
                entry.bounded = config->min.f < config->max.f;
                if (entry.bounded) {
                    entry.minimum = config->min.f;
                    entry.maximum = config->max.f;
                }
            } else {
                entry.original = QString::fromUtf8(config->orig.psz ? config->orig.psz : "");
            }
            entry.current = readValue(entry);
            entry.staged = entry.current;
            if (entry.type == QLatin1String("integer")) {
                int64_t *values = nullptr;
                char **texts = nullptr;
                const ssize_t count = config_GetIntChoices(option.name, &values, &texts);
                for (ssize_t j = 0; j < count; ++j) {
                    const QString label = texts && texts[j] ? qfut(texts[j]) : QString::number(values[j]);
                    entry.choices.append(QVariantMap{{QStringLiteral("value"), QVariant::fromValue(values[j])},
                                                     {QStringLiteral("label"), label}});
                    if (texts) free(texts[j]);
                }
                free(texts);
                free(values);
            } else if (entry.type == QLatin1String("string")
                       && entry.key != QLatin1String("extraintf")
                       && entry.key != QLatin1String("control")
                       && entry.key != QLatin1String("audio-filter")) {
                char **values = nullptr;
                char **texts = nullptr;
                const ssize_t count = config_GetPszChoices(option.name, &values, &texts);
                // the encoder of the conversions: "none" lets VLC pick one
                const bool encoder = entry.key == QLatin1String("sout-transcode-venc");
                for (ssize_t j = 0; j < count; ++j) {
                    const QString value = QString::fromUtf8(values[j] ? values[j] : "");
                    QString label = texts && texts[j] ? qfut(texts[j]) : value;
                    if (encoder && value == QLatin1String("any")) {
                        free(values[j]);
                        if (texts) free(texts[j]);
                        continue;
                    }
                    if (encoder && value == QLatin1String("none"))
                        label = qtr("Automatic");
                    entry.choices.append(QVariantMap{{QStringLiteral("value"), value},
                                                     {QStringLiteral("label"), label}});
                    free(values[j]);
                    if (texts) free(texts[j]);
                }
                // Automatic first
                if (encoder && !entry.choices.isEmpty())
                    entry.choices.move(entry.choices.size() - 1, 0);
                free(texts);
                free(values);
            }
            if (entry.key == QLatin1String("aout") && entry.choices.isEmpty())
                continue;
            if (entry.key == QLatin1String("menu-language") || entry.key == QLatin1String("sub-language")
                || entry.key == QLatin1String("audio-language"))
                entry.choices = languageChoices(entry.current.toString(),
                                                entry.key != QLatin1String("menu-language"));
            else if (entry.key == QLatin1String("aspect-ratio") || entry.key == QLatin1String("crop")
                     || entry.key == QLatin1String("zoom") || entry.key == QLatin1String("snapshot-width")
                     || entry.key == QLatin1String("sub-text-scale") || entry.key == QLatin1String("sub-margin")
                     || entry.key == QLatin1String("freetype-shadow-distance")
                     || entry.key == QLatin1String("postproc-q") || entry.key == QLatin1String("codec")
                     || entry.key == QLatin1String("input-timeshift-granularity"))
                entry.choices = presetChoices(entry.key, entry.current);
            // Colors of the subtitles: any color (a field of the design)
            else if (entry.key.startsWith(QLatin1String("freetype-")) && entry.key.endsWith(QLatin1String("color")))
                entry.choices = {};
            else if (entry.key == QLatin1String("dvd"))
                entry.choices = opticalDriveChoices(entry.current.toString());
            else if (entry.key == QLatin1String("qt-fullscreen-screennumber"))
            {
                // -1: the screen of the window; then the screens by number
                entry.choices = {};
                entry.choices.append(QVariantMap{{QStringLiteral("value"), QVariant::fromValue<int64_t>(-1)},
                                                 {QStringLiteral("label"), qtr("Current screen")}});
                const QList<QScreen *> screens = QGuiApplication::screens();
                bool listed = entry.current.toLongLong() == -1;
                for (int64_t i = 0; i < screens.size(); ++i) {
                    entry.choices.append(QVariantMap{{QStringLiteral("value"), QVariant::fromValue(i)},
                                                     {QStringLiteral("label"), qtr("Screen %1 (%2)").arg(i + 1)
                                                                               .arg(screens[i]->name())}});
                    listed = listed || entry.current.toLongLong() == i;
                }
                if (!listed)
                    entry.choices.append(QVariantMap{{QStringLiteral("value"), entry.current},
                                                     {QStringLiteral("label"), qtr("Screen %1").arg(entry.current.toLongLong() + 1)}});
            }
            else if (entry.key.startsWith(QLatin1String("sout-x264-")))
                entry.choices = x264Choices(option.name, entry.current.toString());
            // Volume step: a field in percent of the design (volumepercent)
            else if (entry.key == QLatin1String("sub-autodetect-fuzzy"))
            {
                // The levels of SUB_FUZZY_LONGTEXT
                const QString labels[] = { qtr("Off"), qtr("Any subtitle file"),
                                           qtr("Files containing the title"),
                                           qtr("Title with extra characters"), qtr("Exact title") };
                for (int64_t level = 0; level < 5; ++level)
                    entry.choices.append(QVariantMap{{QStringLiteral("value"), QVariant::fromValue(level)},
                                                     {QStringLiteral("label"), labels[level]}});
            }
            m_entries.append(entry);
        }
        addCombinedEntries();
        // Size values: Pixels while VLC has a size in pixels, else Relative
        const int pixels = indexOfKey(QStringLiteral("freetype-fontsize"));
        const int sizeMode = indexOfKey(QStringLiteral("redesign/subtitle-size-mode"));
        if (pixels >= 0 && sizeMode >= 0) {
            m_entries[sizeMode].current = QVariant::fromValue<int64_t>(m_entries[pixels].current.toLongLong() > 0 ? 1 : 0);
            m_entries[sizeMode].staged = m_entries[sizeMode].current;
        }
        // The kept opacity of the subtitle background: VLC's while there is one
        const int background = indexOfKey(QStringLiteral("freetype-background-opacity"));
        const int kept = indexOfKey(QLatin1String(subBackgroundOpacityKey));
        if (background >= 0 && kept >= 0 && m_entries[background].current.toLongLong() != 0) {
            m_entries[kept].current = QVariant::fromValue<int64_t>(m_entries[background].current.toLongLong());
            m_entries[kept].staged = m_entries[kept].current;
        }
        syncDerived();
        for (int i = 0; i < m_entries.size(); ++i)
            if (visible(m_entries[i]))
                m_rows.append(i);
    }
    endResetModel();
    ++m_revision;
    emit revisionChanged();
    emit categoriesChanged();
    if (wasDirty)
        emit dirtyChanged();
    setError({});
}
