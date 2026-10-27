/****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 ****************************************************************************/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "config_model.hpp"

#include <vlc_common.h>
#include <vlc_configuration.h>
#include <vlc_modules.h>
#include <vlc_aout.h>

#include "interface_language.hpp"
#include "maininterface/mainctx.hpp"
#include "dialogs/toolbar/controlbar_profile_model.hpp"
#include "qt.hpp"

#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QPointer>
#include <QSettings>
#include <QUrl>
#include <QFileInfo>
#include <QProcess>

#include <cmath>

#include "../../../../audio_filter/equalizer_presets.h"

QVariant ConfigModel::valueFor(const QString& key) const
{
    for (const Entry& entry : m_entries)
        if (entry.key == key) return entry.staged;
    return {};
}

bool ConfigModel::hasOption(const QString& key) const
{
    return valueFor(key).isValid();
}

// As the native preferences (getDefaultAudioVolume, simple_preferences.cpp)
QString ConfigModel::startupVolumeOption() const
{
    const QString aout = valueFor(QStringLiteral("aout")).toString();
    const bool automatic = aout.isEmpty() || aout == QLatin1String("any");
    QString option;
#ifdef _WIN32
    // the outputs Windows chooses automatically all keep it: mmdevice first
    if (automatic || aout == QLatin1String("mmdevice"))
        option = QStringLiteral("mmdevice-volume");
    else if (aout == QLatin1String("waveout"))
        option = QStringLiteral("waveout-volume");
#else
    Q_UNUSED(automatic);
#endif
#ifdef __linux__
    if (aout == QLatin1String("alsa"))
        option = QStringLiteral("alsa-gain");
#endif
#ifdef __APPLE__
    if (aout == QLatin1String("auhal"))
        option = QStringLiteral("auhal-volume");
#endif
    if (aout == QLatin1String("jack"))
        option = QStringLiteral("jack-gain");
    return !option.isEmpty() && hasOption(option) ? option : QString();
}

// (the gains are the cube of the volume; auhal counts 256 for 100 %)
QVariant ConfigModel::startupVolume() const
{
    const QString option = startupVolumeOption();
    if (option.isEmpty())
        return {};
    const double value = valueFor(option).toDouble();
    if (option.endsWith(QLatin1String("-gain")))
        return std::cbrt(value);
    if (option == QLatin1String("auhal-volume"))
        return value / AOUT_VOLUME_DEFAULT;
    return value;
}

bool ConfigModel::setStartupVolume(double volume)
{
    const QString option = startupVolumeOption();
    if (option.isEmpty()) {
        setError(qtr("The audio output in use does not keep a startup volume."));
        return false;
    }
    double value = volume;
    if (option.endsWith(QLatin1String("-gain")))
        value = volume * volume * volume;
    else if (option == QLatin1String("auhal-volume"))
        value = volume * AOUT_VOLUME_DEFAULT;
    return setValue(option, value);
}

#ifndef _WIN32
// The path of a file in one of VLC's folders (config_GetSysPath)
static QString sysPath(vlc_sysdir_t dir, const char *name)
{
    char *path = config_GetSysPath(dir, name);
    const QString result = QString::fromUtf8(path ? path : "");
    free(path);
    return result;
}
#endif

bool ConfigModel::rescanPlugins()
{
#ifdef _WIN32
    const QString folder = QCoreApplication::applicationDirPath();
    const QString generator = folder + QStringLiteral("/vlc-cache-gen.exe");
    const QString plugins = folder + QStringLiteral("/plugins");
#else
    // elsewhere the generator is in libexec, not next to the program
    const QString generator = sysPath(VLC_PKG_LIBEXEC_DIR, "vlc-cache-gen");
    const QString plugins = sysPath(VLC_PKG_LIB_DIR, "plugins");
#endif
    if (generator.isEmpty() || plugins.isEmpty() || !QFileInfo::exists(generator)) {
        setError(qtr("The plugins can not be rescanned on this installation."));
        return false;
    }
    // Waited for, without blocking, to tell whether it worked: it ends well
    // even when it could not write the cache (a folder of Program Files
    // without the rights), which is then older than the scan. Not a child of
    // the model: closing the settings must not kill it.
    const QDateTime started = QDateTime::currentDateTime();
    const QPointer<ConfigModel> model(this);
    auto *process = new QProcess;
    connect(process, &QProcess::errorOccurred, process, [model, process](QProcess::ProcessError error) {
        // a crash ends with finished()
        if (error != QProcess::FailedToStart)
            return;
        if (model)
            model->setError(qtr("The plugins could not be rescanned."));
        process->deleteLater();
    });
    connect(process, &QProcess::finished, process,
            [model, process, plugins, started](int exitCode, QProcess::ExitStatus status) {
        process->deleteLater();
        // (the time of a file may be rounded to 2 s)
        const QFileInfo cache(plugins + QStringLiteral("/plugins.dat"));
        if (status == QProcess::NormalExit && exitCode == 0 && cache.exists()
            && cache.lastModified() >= started.addSecs(-2))
            return;
        if (model)
            model->setError(qtr("The plugins could not be rescanned: the folder of VLC may need administrator rights."));
    });
    process->start(generator, { QDir::toNativeSeparators(plugins) });
    return true;
}

bool ConfigModel::openConfigFolder()
{
    char *folder = config_GetUserDir(VLC_CONFIG_DIR);
    const QString path = QString::fromUtf8(folder ? folder : "");
    free(folder);
    if (path.isEmpty() || !QDesktopServices::openUrl(QUrl::fromLocalFile(path))) {
        setError(qtr("The configurations folder could not be opened."));
        return false;
    }
    return true;
}

// As "Reset Preferences" of the native preferences
void ConfigModel::resetAll()
{
    if (!m_ctx || !m_ctx->getIntf())
        return;
    qt_intf_t *p_intf = m_ctx->getIntf();
    config_ResetAll();
    config_SaveConfigFile(p_intf);
    getSettings()->clear();
    // The interface goes on with the defaults it reloads (the ones VLC reads
    // once too: MainCtx::reloadPrefs), its state saved at exit as usual
    m_ctx->reloadPrefs();
    m_ctx->reloadFromSettings();
    // the profiles would put their own settings back (when saved, when one
    // is chosen): they take the defaults now in use
    if (auto *profiles = m_ctx->controlbarProfileModel())
        profiles->forgetSettingsSnapshots();
    // the language of the interface is not a setting (a file of the profile)
    resetInterfaceLanguage();
    m_ctx->applyInterfaceLanguage(interfaceLanguage());
    m_ctx->setRightToLeft(m_ctx->languageRightToLeft());
    reload();
}

QVariantList ConfigModel::equalizerPreset(const QString& name) const
{
    for (const eqz_preset_t &preset : eqz_preset_10b) {
        if (name != QLatin1String(preset.psz_name))
            continue;
        QVariantList values{ preset.f_preamp };
        for (int i = 0; i < preset.i_band; ++i)
            values.append(preset.f_amp[i]);
        return values;
    }
    return {};
}

QString ConfigModel::defaultText(const QString& key) const
{
    vlc_userdir_t folder;
    if (key == QLatin1String("input-record-path"))
        folder = VLC_VIDEOS_DIR;
    else if (key == QLatin1String("snapshot-path"))
        folder = VLC_PICTURES_DIR;
    else
        return {};
    char *path = config_GetUserDir(folder);
    const QString result = path ? QDir::toNativeSeparators(QString::fromUtf8(path)) : QString();
    free(path);
    return result;
}

QString ConfigModel::displayPath(const QString& path) const
{
    if (path.isEmpty())
        return {};
    const QString clean = QDir::cleanPath(QDir::fromNativeSeparators(path));
    const QString home = QDir::cleanPath(QDir::homePath());
    if (clean.compare(home, Qt::CaseInsensitive) == 0)
        return QStringLiteral("~");
    if (clean.startsWith(home + QLatin1Char('/'), Qt::CaseInsensitive))
        return QStringLiteral("~") + clean.mid(home.size());
    return QDir::toNativeSeparators(clean);
}

bool ConfigModel::audioscrobblerAvailable() const
{
    return module_exists("audioscrobbler") && hasOption(QStringLiteral("lastfm-username"))
        && hasOption(QStringLiteral("lastfm-password"))
        && hasOption(QStringLiteral("scrobbler-url"))
        && hasOption(QStringLiteral("extraintf"))
        && hasOption(QStringLiteral("control"));
}

bool ConfigModel::audioscrobblerEnabled() const
{
    const auto contains = [](const QString& list) {
        return list.split(':', Qt::SkipEmptyParts).contains(QStringLiteral("audioscrobbler"));
    };
    return contains(valueFor(QStringLiteral("extraintf")).toString())
        || contains(valueFor(QStringLiteral("control")).toString());
}

bool ConfigModel::setAudioscrobblerEnabled(bool enabled)
{
    if (!audioscrobblerAvailable()) {
        setError(qtr("Last.fm is unavailable in this VLC installation."));
        return false;
    }
    const QString extra = valueFor(QStringLiteral("extraintf")).toString();
    const QString control = valueFor(QStringLiteral("control")).toString();
    if (enabled) {
        if (audioscrobblerEnabled()) return true;
        return setValue(QStringLiteral("extraintf"),
                        extra.isEmpty() ? QStringLiteral("audioscrobbler")
                                        : extra + QStringLiteral(":audioscrobbler"));
    }
    const auto withoutScrobbler = [](const QString& list) {
        QStringList parts = list.split(':', Qt::KeepEmptyParts);
        parts.removeAll(QStringLiteral("audioscrobbler"));
        return parts.join(':');
    };
    if (!setValue(QStringLiteral("extraintf"), withoutScrobbler(extra))) return false;
    if (!setValue(QStringLiteral("control"), withoutScrobbler(control))) {
        setValue(QStringLiteral("extraintf"), extra);
        return false;
    }
    return true;
}
