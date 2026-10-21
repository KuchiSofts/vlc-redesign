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

#include "interface_language.hpp"
#include "maininterface/mainctx.hpp"
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

bool ConfigModel::rescanPlugins()
{
    const QString folder = QCoreApplication::applicationDirPath();
    const QString generator = folder + QStringLiteral("/vlc-cache-gen.exe");
    if (!QFileInfo::exists(generator)) {
        setError(qtr("The plugins can not be rescanned on this installation."));
        return false;
    }
    // Waited for, without blocking, to tell whether it worked: it ends well
    // even when it could not write the cache (a folder of Program Files
    // without the rights), which is then older than the scan. Not a child of
    // the model: closing the settings must not kill it.
    const QString plugins = folder + QStringLiteral("/plugins");
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
    // Not preferencesResetPending: the interface goes on with the defaults it
    // reloads, and its state is saved at exit as usual (the old dialog sets
    // it only until VLC restarts or the restart is declined)
    m_ctx->reloadPrefs();
    m_ctx->reloadFromSettings();
#ifdef _WIN32
    // the language of the interface is not a setting (registry, portable file)
    setInterfaceLanguage(QStringLiteral("auto"));
#endif
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
