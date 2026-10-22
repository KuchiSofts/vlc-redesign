/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/

#include "onboarding.hpp"
#include "dialogs/preferences/interface_language.hpp"

#include "maininterface/mainctx.hpp"
#include "medialibrary/medialib.hpp"
#include "util/color_scheme_model.hpp"
#include "util/vlchotkeyconverter.hpp"
#include "qt.hpp"

#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QVariantMap>

#include <vlc_actions.h>
#include <vlc_configuration.h>
#include <vlc_interface.h>
#include <vlc_playlist.h>

#include <cctype>
#include <cstdlib>
#include <cstring>

OnboardingController::OnboardingController(QObject* parent)
    : QObject(parent)
{
    connect(&m_folders, &QAbstractItemModel::modelReset,
            this, &OnboardingController::checkPendingFolders);
    connect(&m_folders, &MLFoldersBaseModel::operationFailed, this,
            [this](int operation, const QUrl& url) {
        if (!m_busy || operation != MLFoldersBaseModel::Add ||
            !m_requestedFolders.contains(url))
            return;
        m_requestedFolders.clear();
        setBusy(false);
        setError(tr("Could not add the media folder: %1").arg(url.toDisplayString()));
    });
}

OnboardingController::~OnboardingController()
{
    restoreTheme();
}

MainCtx* OnboardingController::ctx() const
{
    return m_ctx;
}

void OnboardingController::setCtx(MainCtx* ctx)
{
    if (m_ctx == ctx)
        return;
    // MLFoldersModel binds its event subscription to one MainCtx for life.
    if (m_ctx || !ctx)
        return;
    m_ctx = ctx;
    if (m_ctx && m_ctx->getMediaLibrary())
        m_folders.setCtx(m_ctx);
    emit ctxChanged();
    emit needsSetupChanged();
    emit themeIndexChanged();
}

bool OnboardingController::needsSetup() const
{
    return m_ctx && !m_completed && var_InheritBool(m_ctx->getIntf(), "qt-privacy-ask");
}

// The core playlist: the interface's model fills in later, it was still
// empty when the welcome was shown over the first file opened
bool OnboardingController::playlistHasMedia() const
{
    if (!m_ctx || !m_ctx->getIntf())
        return false;
    vlc_playlist_t *playlist = vlc_intf_GetMainPlaylist(m_ctx->getIntf()->intf);
    vlc_playlist_Lock(playlist);
    const size_t count = vlc_playlist_Count(playlist);
    vlc_playlist_Unlock(playlist);
    return count > 0;
}

int OnboardingController::themeIndex() const
{
    return m_ctx ? m_ctx->getColorScheme()->currentIndex() : -1;
}

QUrl OnboardingController::suggestedFolder() const
{
    const QString path = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    const QFileInfo folder(path);
    if (folder.exists() && folder.isDir() && folder.isReadable())
        return QUrl::fromLocalFile(folder.canonicalFilePath());
    return {};
}

QString OnboardingController::displayPath(const QUrl& folder) const
{
    if (!folder.isLocalFile())
        return QString();
    const QString path = QDir::cleanPath(folder.toLocalFile());
    const QString home = QDir::cleanPath(QDir::homePath());
    // the folders of the user from their home, as the design shows them
    if (path.compare(home, Qt::CaseInsensitive) == 0)
        return QStringLiteral("~");
    if (path.startsWith(home + QLatin1Char('/'), Qt::CaseInsensitive))
        return QStringLiteral("~") + path.mid(home.size());
    return QDir::toNativeSeparators(path);
}

bool OnboardingController::metadataNetworkAccess() const
{
    return m_ctx && config_GetInt("metadata-network-access") != 0;
}

void OnboardingController::setBusy(bool busy)
{
    if (m_busy == busy)
        return;
    m_busy = busy;
    emit busyChanged();
}

void OnboardingController::setError(const QString& error)
{
    if (m_error == error)
        return;
    m_error = error;
    emit errorChanged();
}

bool OnboardingController::begin()
{
    if (!m_ctx || m_busy)
        return false;
    if (m_previewActive)
        return true;
    setError({});
    m_initialTheme = themeIndex();
    m_previewActive = true;
    return true;
}

bool OnboardingController::previewTheme(int index)
{
    if (!m_ctx || !m_previewActive || m_busy || index < 0 ||
        index >= m_ctx->getColorScheme()->rowCount(QModelIndex()))
        return false;
    m_ctx->getColorScheme()->setCurrentIndex(index);
    emit themeIndexChanged();
    return true;
}

bool OnboardingController::acceptTheme()
{
    if (!m_ctx || m_busy || !m_previewActive) return false;
    m_ctx->setSettingValue("MainWindow/color-scheme-index", themeIndex());
    m_previewActive = false;
    m_initialTheme = -1;
    return true;
}

void OnboardingController::restoreTheme()
{
    if (!m_previewActive)
        return;
    if (m_ctx && m_initialTheme >= 0) {
        m_ctx->getColorScheme()->setCurrentIndex(m_initialTheme);
        emit themeIndexChanged();
    }
    m_previewActive = false;
    m_initialTheme = -1;
}

bool OnboardingController::cancel()
{
    if (m_busy)
        return false; // Folder registration cannot be canceled after submission.
    restoreTheme();
    setError({});
    return true;
}

bool OnboardingController::validateFolders(const QVariantList& values, QList<QUrl>* folders)
{
    folders->clear();
    if (!values.isEmpty() && (!m_ctx || !m_ctx->getMediaLibrary())) {
        setError(tr("The media library is unavailable."));
        return false;
    }

    for (const QVariant& value : values) {
        const QUrl url = value.toUrl();
        if (!url.isValid() || !url.isLocalFile()) {
            setError(tr("Choose a local media folder."));
            return false;
        }
        const QFileInfo info(url.toLocalFile());
        if (!info.exists() || !info.isDir() || !info.isReadable()) {
            setError(tr("The media folder does not exist or cannot be read: %1")
                         .arg(url.toDisplayString()));
            return false;
        }
        const QUrl canonical = QUrl::fromLocalFile(info.canonicalFilePath());
        if (!folders->contains(canonical))
            folders->append(canonical);
    }
    return true;
}

bool OnboardingController::folderExists(const QUrl& folder) const
{
    for (int row = 0; row < m_folders.rowCount(); ++row) {
        const QString mrl = m_folders.data(m_folders.index(row, 0),
                                           MLFoldersBaseModel::MRL).toString();
        const QUrl current = QUrl::fromEncoded(mrl.toUtf8());
        if (!current.isLocalFile())
            continue;
        if (QFileInfo(current.toLocalFile()).canonicalFilePath() == folder.toLocalFile())
            return true;
    }
    return false;
}

bool OnboardingController::finish(const QVariantList& folderUrls, bool metadataNetworkAccess)
{
    if (!m_ctx || m_busy || !m_previewActive)
        return false;
    setError({});
    QList<QUrl> folders;
    // Without the media library (disabled or failed), the setup finishes
    // without a folder: it would not end otherwise and come back each start
    const QVariantList requested = m_ctx->getMediaLibrary() ? folderUrls : QVariantList();
    if (!validateFolders(requested, &folders))
        return false;

    m_metadataNetworkAccess = metadataNetworkAccess;
    m_requestedFolders = folders;

    QList<QUrl> missing;
    for (const QUrl& folder : folders) {
        if (!folderExists(folder))
            missing.append(folder);
    }
    if (missing.isEmpty())
        return commit();

    setBusy(true);
    for (const QUrl& folder : missing)
        m_folders.add(folder);
    return true; // Accepted for asynchronous indexing; completed() is authoritative.
}

void OnboardingController::checkPendingFolders()
{
    if (!m_busy || m_requestedFolders.isEmpty())
        return;
    for (const QUrl& folder : m_requestedFolders) {
        if (!folderExists(folder))
            return;
    }
    setBusy(false);
    commit();
}

bool OnboardingController::skip()
{
    if (!m_ctx || m_busy || !m_previewActive)
        return false;
    if (!needsSetup())
        return cancel(); // Reopened from Settings: keep the existing choices.
    restoreTheme();
    m_metadataNetworkAccess = config_GetInt("metadata-network-access") != 0;
    m_requestedFolders.clear();
    setError({});
    return commit();
}

bool OnboardingController::commit()
{
    if (!m_ctx)
        return false;

    auto* const intf = m_ctx->getIntf();
    auto* const settings = intf->mainSettings;
    const int oldPrivacy = config_GetInt("qt-privacy-ask");
    const int oldMetadata = config_GetInt("metadata-network-access");
    const QVariant oldTheme = settings->value("MainWindow/color-scheme-index");
    const bool hadTheme = settings->contains("MainWindow/color-scheme-index");

    settings->setValue("MainWindow/color-scheme-index", themeIndex());
    settings->sync();
    if (settings->status() != QSettings::NoError) {
        if (hadTheme)
            settings->setValue("MainWindow/color-scheme-index", oldTheme);
        else
            settings->remove("MainWindow/color-scheme-index");
        settings->sync();
        setError(tr("Could not save the theme setting."));
        return false;
    }

    // written only when chosen otherwise: the existing value stays as it is
    if (m_metadataNetworkAccess != (oldMetadata != 0))
        config_PutInt("metadata-network-access", m_metadataNetworkAccess ? 1 : 0);
    config_PutInt("qt-privacy-ask", 0);
    if (config_SaveConfigFile(intf) != 0) {
        config_PutInt("metadata-network-access", oldMetadata);
        config_PutInt("qt-privacy-ask", oldPrivacy);
        if (hadTheme)
            settings->setValue("MainWindow/color-scheme-index", oldTheme);
        else
            settings->remove("MainWindow/color-scheme-index");
        settings->sync();
        setError(tr("Could not save the first-run preferences."));
        return false;
    }

    m_previewActive = false;
    m_initialTheme = -1;
    m_requestedFolders.clear();
    m_completed = true;
    if (m_ctx->getMediaLibrary())
        m_ctx->getMediaLibrary()->reload();
    m_ctx->reloadPrefs();
    emit needsSetupChanged();
    emit completed();
    return true;
}

QVariantList OnboardingController::activeShortcuts() const
{
    QVariantList result;
    if (!m_ctx)
        return result;

    // Rows of the design (Figma 1902:6770, the left column then the right
    // one): an action, or two opposite ones, the hotkeys and the key the
    // design shows for each; or a key of the interface (else its fallback)
    struct Shortcut {
        const char* action;
        const char* first; const char* firstKey;
        const char* second; const char* secondKey;
        const char* interfaceKey; const char* fallback;
    };
    static const Shortcut shortcuts[] = {
        {QT_TR_NOOP("Play / Pause"), "key-play-pause", "Space", nullptr, nullptr, nullptr, nullptr},
        // the navigation keys seek and change the volume without a menu
        {QT_TR_NOOP("Seek Backward / Forward"), "key-nav-left", "Left", "key-nav-right", "Right", nullptr, nullptr},
        {QT_TR_NOOP("Previous / Next Item"), "key-prev", "<", "key-next", ">", nullptr, nullptr},
        {QT_TR_NOOP("Previous / Next Frame"), "key-frame-previous", ",", "key-frame-next", ".", nullptr, nullptr},
        {QT_TR_NOOP("Cycle Subtitle Track"), "key-subtitle-track", "v", nullptr, nullptr, nullptr, nullptr},
        {QT_TR_NOOP("Cycle Audio Track"), "key-audio-track", "b", nullptr, nullptr, nullptr, nullptr},
        {QT_TR_NOOP("Toggle Fullscreen"), "key-toggle-fullscreen", "f", nullptr, nullptr, nullptr, nullptr},
        {QT_TR_NOOP("Increase / Decrease Volume"), "key-nav-up", "Up", "key-nav-down", "Down", nullptr, nullptr},
        {QT_TR_NOOP("Show / Hide Queue"), nullptr, nullptr, nullptr, nullptr, "q", "Ctrl+L"},
        {QT_TR_NOOP("Show / Hide File Browser"), nullptr, nullptr, nullptr, nullptr, "l", nullptr},
        {QT_TR_NOOP("Take a Snapshot"), "key-snapshot", "Shift+s", nullptr, nullptr, nullptr, nullptr},
        {QT_TR_NOOP("Record"), "key-record", "Shift+r", nullptr, nullptr, nullptr, nullptr},
    };

    // the key of the design when it is bound to the action (a sign also
    // with Shift, as most layouts type it), else the first key bound
    const auto hotkey = [](const char* name, const char* designKey) {
        if (!name)
            return QString();
        char* const configured = config_GetPsz(name);
        const QStringList bound = configured
            ? QString::fromUtf8(configured).split(QLatin1Char('\t'), Qt::SkipEmptyParts)
            : QStringList();
        std::free(configured);
        if (bound.isEmpty())
            return QString();
        if (designKey) {
            const uint_fast32_t code = vlc_str2keycode(designKey);
            const bool sign = std::strlen(designKey) == 1 && !std::isalnum(static_cast<unsigned char>(designKey[0]));
            for (const QString& key : bound) {
                const uint_fast32_t boundCode = vlc_str2keycode(qtu(key));
                if (code != KEY_UNSET && (boundCode == code || (sign && boundCode == (code | KEY_MODIFIER_SHIFT))))
                    return QString::fromLatin1(designKey);
            }
        }
        return bound.first();
    };

    for (const Shortcut& shortcut : shortcuts) {
        QStringList sequences;
        if (shortcut.interfaceKey) {
            // the interface handles it while no hotkey of VLC takes it
            if (!isVLCKeyBound(VLC_OBJECT(m_ctx->getIntf()), vlc_str2keycode(shortcut.interfaceKey)))
                sequences << QString::fromLatin1(shortcut.interfaceKey);
            else if (shortcut.fallback)
                sequences << QString::fromLatin1(shortcut.fallback);
        } else {
            sequences << hotkey(shortcut.first, shortcut.firstKey);
            if (shortcut.second)
                sequences << hotkey(shortcut.second, shortcut.secondKey);
        }
        sequences.removeAll(QString());
        if (!sequences.isEmpty())
            result.append(QVariantMap{{"action", tr(shortcut.action)},
                                      {"sequences", sequences}});
    }
    return result;
}

QVariantList OnboardingController::interfaceLanguages() const
{
    return ::interfaceLanguages();
}

QString OnboardingController::interfaceLanguage() const
{
    return ::interfaceLanguage();
}

bool OnboardingController::setInterfaceLanguage(const QString& iso)
{
    return ::setInterfaceLanguage(iso);
}
