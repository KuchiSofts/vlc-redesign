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

#include "dialogs/preferences/interface_language.hpp"
#include "maininterface/mainctx.hpp"
#include "util/redesign_updater.hpp"
#include "util/redesign_version.hpp"
#include "qt.hpp"

#include <QDesktopServices>
#include <QFile>
#include <QLocale>
#include <QRegularExpression>
#include <QSettings>
#include <QStringList>
#include <QUrl>
#include <QVariantMap>

#include <vlc_interface.h>

#include <algorithm>

namespace {

// the version whose card was last shown (or recorded without it)
const char keySeen[] = "redesign/whats-new-seen";
// Settings, General, Startup: "Show what's new after an update"
const char keyShow[] = "redesign/show-whats-new";
// CHANGELOG.md of the source tree (assets.qrc)
const char changelogResource[] = ":/redesign/CHANGELOG.md";
// the headlines of a group shown, the others counted
constexpr int maxItemsPerGroup = 6;
constexpr int maxHeadlineLength = 140;

// The text of a bit of Markdown: links, code and emphasis without their marks
QString plainText(QString text)
{
    static const QRegularExpression link(QStringLiteral(R"(\[([^\]]*)\]\([^)]*\))"));
    static const QRegularExpression emphasis(QStringLiteral(R"((\*\*|\*|__)(.+?)\1)"));
    text.replace(link, QStringLiteral("\\1"));
    text.remove(QLatin1Char('`'));
    text.replace(emphasis, QStringLiteral("\\2"));
    return text.simplified();
}

// The headline of a bullet: its bold lead ("**Subtitle languages that
// learn.** Each subtitle..."), else its first sentence; short, without the
// full stop or colon that ends it
QString headline(const QString& bullet)
{
    QString lead;
    if (bullet.startsWith(QLatin1String("**"))) {
        const int end = bullet.indexOf(QLatin1String("**"), 2);
        if (end > 2)
            lead = bullet.mid(2, end - 2);
    }
    if (lead.isEmpty()) {
        lead = bullet;
        const int stop = lead.indexOf(QLatin1String(". "));
        if (stop > 0)
            lead.truncate(stop);
    }
    lead = plainText(lead);
    while (lead.endsWith(QLatin1Char('.')) || lead.endsWith(QLatin1Char(':')))
        lead.chop(1);
    lead = lead.trimmed();
    if (lead.size() > maxHeadlineLength)
        lead = lead.left(maxHeadlineLength - 1).trimmed() + QChar(0x2026);
    return lead;
}

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
    QStringList lines = changelog.split(QLatin1Char('\n'));
    for (QString& line : lines) {
        if (line.endsWith(QLatin1Char('\r')))
            line.chop(1);
    }
    const auto find = [&lines](const QString& name) {
        const QString heading = QStringLiteral("## [") + name + QLatin1Char(']');
        for (int i = 0; i < lines.size(); ++i) {
            if (lines[i].startsWith(heading))
                return i;
        }
        return -1;
    };
    // the section of the version (a release), else the one it prepares
    int start = find(version);
    if (ownSection)
        *ownSection = start >= 0;
    if (start < 0)
        start = find(QStringLiteral("Unreleased"));
    if (start < 0)
        return {};

    static const char* const kinds[] = { "added", "changed", "fixed" };
    QStringList items[3];
    int group = -1;
    QString bullet;
    // a bullet of a bullet: its lines are not the headline's
    bool nested = false;
    const auto flush = [&]() {
        if (group >= 0 && !bullet.isEmpty()) {
            const QString text = headline(bullet);
            if (!text.isEmpty())
                items[group].append(text);
        }
        bullet.clear();
        nested = false;
    };

    for (int i = start + 1; i < lines.size(); ++i) {
        const QString& line = lines[i];
        if (line.startsWith(QLatin1String("## ")))
            break; // the next version
        if (line.startsWith(QLatin1String("### "))) {
            flush();
            const QString name = line.mid(4).trimmed();
            group = -1;
            for (int k = 0; k < 3; ++k) {
                if (name.compare(QLatin1String(kinds[k]), Qt::CaseInsensitive) == 0)
                    group = k;
            }
            continue; // Technical, Documentation...: not for the card
        }
        if (group < 0)
            continue;
        if (line.startsWith(QLatin1String("- ")) || line.startsWith(QLatin1String("* "))) {
            flush();
            bullet = line.mid(2).trimmed();
            continue;
        }
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) {
            flush();
            continue;
        }
        if (bullet.isEmpty())
            continue;
        if (trimmed.startsWith(QLatin1String("- ")) || trimmed.startsWith(QLatin1String("* ")))
            nested = true;
        else if (!nested && line.startsWith(QLatin1Char(' ')))
            bullet += QLatin1Char(' ') + trimmed;
    }
    flush();

    QVariantList result;
    for (int k = 0; k < 3; ++k) {
        if (items[k].isEmpty())
            continue;
        const int more = std::max(0, int(items[k].size()) - maxItemsPerGroup);
        result.append(QVariantMap{
            {QStringLiteral("kind"), QString::fromLatin1(kinds[k])},
            {QStringLiteral("items"), QStringList(items[k].mid(0, maxItemsPerGroup))},
            {QStringLiteral("more"), more},
        });
    }
    return result;
}
