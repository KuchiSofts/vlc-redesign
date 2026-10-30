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
#include <QPointer>
#include <QString>
#include <QVariantList>

class MainCtx;
Q_MOC_INCLUDE("maininterface/mainctx.hpp")

// "What's new" after an update: the first start of a new version (an
// installer upgrade, a small update put in place, a portable copy that took
// the settings of an older one) shows the headlines of its section of
// CHANGELOG.md, which the build carries in the resources
// (qrc:/redesign/CHANGELOG.md, the same file as CHANGELOG.txt of the
// package). The version last seen is kept in the interface settings
// (redesign/whats-new-seen); a new install records it without showing
// anything (the welcome shows instead), and so does a downgrade.
// Setting: redesign/show-whats-new (General, Startup), on by default.
// The notes are the English ones of the changelog (they are not translated).
class WhatsNewController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(MainCtx* ctx READ ctx WRITE setCtx NOTIFY ctxChanged FINAL)
    // the card waits to be shown in this session
    Q_PROPERTY(bool pending READ pending NOTIFY pendingChanged FINAL)
    Q_PROPERTY(QString version READ version CONSTANT FINAL)
    // the page of the release of this version (the changelog when the
    // build has no section of its own: a build between two releases)
    Q_PROPERTY(QString changelogUrl READ changelogUrl NOTIFY ctxChanged FINAL)

public:
    explicit WhatsNewController(QObject* parent = nullptr);

    MainCtx* ctx() const { return m_ctx; }
    void setCtx(MainCtx* ctx);

    bool pending() const { return m_pending; }
    QString version() const;
    QString changelogUrl() const;

    // [{ kind: "added" | "changed" | "fixed", items: [text...], more: n }]:
    // the bold lead of each bullet, at most a few per group (more: the
    // ones left out)
    Q_INVOKABLE QVariantList groups();
    // the interface is not in English: the card says the notes are
    Q_INVOKABLE bool notesInOtherLanguage() const;
    // the card is still to be shown (not turned off in Settings meanwhile:
    // then it is recorded as seen)
    Q_INVOKABLE bool showWanted();
    // the card was shown: not again for this version
    Q_INVOKABLE void markSeen();
    Q_INVOKABLE void openChangelog() const;

    // The headlines of the "## [version]" section of a changelog, else of
    // "## [Unreleased]" (a build between two releases); ownSection: the
    // version has its own
    static QVariantList parseChangelog(const QString& changelog, const QString& version,
                                       bool* ownSection = nullptr);

signals:
    void ctxChanged();
    void pendingChanged();

private:
    void decide();
    // the changelog read once, when the card or its link asks for it
    void load() const;

    QPointer<MainCtx> m_ctx;
    bool m_pending = false;
    mutable bool m_loaded = false;
    mutable bool m_ownSection = false;
    mutable QVariantList m_groups;
};
