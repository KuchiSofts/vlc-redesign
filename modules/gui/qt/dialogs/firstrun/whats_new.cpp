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

#include "whats_new.hpp"
#include "whats_new_text.hpp"

#include "dialogs/preferences/interface_language.hpp"
#include "maininterface/mainctx.hpp"
#include "util/redesign_updater.hpp"
#include "util/redesign_version.hpp"
#include "qt.hpp"

#include <QDesktopServices>
#include <QFile>
#include <QLocale>
#include <QSettings>
#include <QUrl>

#include <vlc_interface.h>

namespace {

// the version whose card was last shown (or recorded without it)
const char keySeen[] = "redesign/whats-new-seen";
// Settings, General, Startup: "Show what's new after an update"
const char keyShow[] = "redesign/show-whats-new";
// CHANGELOG.md of the source tree (assets.qrc)
const char changelogResource[] = ":/redesign/CHANGELOG.md";

} // namespace

WhatsNewController::WhatsNewController(QObject* parent)
    : QObject(parent)
{
}

void WhatsNewController::setCtx(MainCtx* ctx)
{
    if (m_ctx == ctx || m_ctx || !ctx)
        return;
    m_ctx = ctx;
    emit ctxChanged();
    decide();
}

QString WhatsNewController::version() const
{
    return QStringLiteral(VLC_REDESIGN_VERSION);
}

QString WhatsNewController::changelogUrl() const
{
    load();
    // the release notes on GitHub are the section of the changelog
    if (m_ownSection)
        return QStringLiteral(VLC_REDESIGN_REPO_URL "/releases/tag/v") + version();
    return QStringLiteral(VLC_REDESIGN_REPO_URL "/blob/main/CHANGELOG.md");
}

void WhatsNewController::decide()
{
    if (!m_ctx || !m_ctx->getIntf())
        return;
    QSettings* const settings = m_ctx->getIntf()->mainSettings;
    const QString current = version();
    const QString seen = settings->value(QLatin1String(keySeen)).toString();

    bool show;
    if (var_InheritBool(m_ctx->getIntf(), "qt-privacy-ask"))
        show = false; // a new install: the welcome shows instead
    else if (seen.isEmpty())
        // the versions up to 0.3.1 did not record it: a profile that went
        // through the welcome (it saves the theme) was used by one of them;
        // one without (VLC started with --no-qt-privacy-ask on a new
        // profile, as the tests do) is a new one
        show = settings->contains(QLatin1String("MainWindow/color-scheme-index"));
    else
        show = RedesignUpdater::versionLess(seen, current);

    if (show && !settings->value(QLatin1String(keyShow), true).toBool())
        show = false;
    if (!show) {
        // the same version, a downgrade, a new install or the card turned
        // off: nothing to show for this version later either
        if (seen != current)
            settings->setValue(QLatin1String(keySeen), current);
        return;
    }
    m_pending = true;
    emit pendingChanged();
}

void WhatsNewController::markSeen()
{
    if (m_ctx && m_ctx->getIntf()) {
        QSettings* const settings = m_ctx->getIntf()->mainSettings;
        settings->setValue(QLatin1String(keySeen), version());
        settings->sync();
    }
    if (m_pending) {
        m_pending = false;
        emit pendingChanged();
    }
}

bool WhatsNewController::showWanted()
{
    if (!m_pending || !m_ctx || !m_ctx->getIntf())
        return false;
    // turned off in Settings meanwhile (it takes effect at once)
    if (!m_ctx->getIntf()->mainSettings->value(QLatin1String(keyShow), true).toBool()) {
        markSeen();
        return false;
    }
    return true;
}

QVariantList WhatsNewController::groups()
{
    load();
    return m_groups;
}

bool WhatsNewController::notesInOtherLanguage() const
{
    QString language = interfaceLanguage();
    if (language.isEmpty() || language == QLatin1String("auto"))
        language = QLocale::system().name();
    return !language.startsWith(QLatin1String("en"), Qt::CaseInsensitive);
}

void WhatsNewController::openChangelog() const
{
    QDesktopServices::openUrl(QUrl(changelogUrl()));
}

void WhatsNewController::load() const
{
    if (m_loaded)
        return;
    m_loaded = true;
    QFile file(QString::fromLatin1(changelogResource));
    if (!file.open(QIODevice::ReadOnly))
        return; // the card offers the full changelog only
    m_groups = parseChangelog(QString::fromUtf8(file.readAll()), version(), &m_ownSection);
}

QVariantList WhatsNewController::parseChangelog(const QString& changelog, const QString& version,
                                                bool* ownSection)
{
    return WhatsNewText::parseChangelog(changelog, version, ownSection);
}
