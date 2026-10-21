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
#include <QTimer>

#include <functional>

#include "qt.hpp"

// Updates of VLC Redesign from its GitHub releases (not VideoLAN's feed,
// which offers the official VLC). Checks the latest release at startup
// (option redesign-update-check, at most once a day) or on request.
//
// A release carries small updates from its predecessors
// (vlc-redesign-<new>-delta-from-<old>.vrdelta, see
// extras/juxtopposed/release/make-delta.py): only the files that changed,
// most of them as binary patches. When one starts from this version and
// VLC's folder is writable (an installed copy for the user, or a portable
// one), it is downloaded in the background, checked against the release's
// SHA256SUMS.txt, and the new files are written next to the old ones
// (<folder>/.update/staged), each checked against its SHA-256, on a worker
// thread. vlc-redesign-update.exe then puts them in place once VLC has
// quit, either now (Restart) or when it closes, and opens VLC again where
// it was. Otherwise an installed copy downloads and runs the installer, and
// a portable one opens the release page.
class RedesignUpdater : public QObject
{
    Q_OBJECT

    Q_PROPERTY(State state READ state NOTIFY stateChanged FINAL)
    Q_PROPERTY(QString currentVersion READ currentVersion CONSTANT FINAL)
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY stateChanged FINAL)
    Q_PROPERTY(QString releaseTitle READ releaseTitle NOTIFY stateChanged FINAL)
    Q_PROPERTY(QString summary READ summary NOTIFY stateChanged FINAL)
    Q_PROPERTY(QString errorText READ errorText NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool canInstall READ canInstall NOTIFY stateChanged FINAL)
    // the update is a small one, put in place without the installer
    Q_PROPERTY(bool smallUpdate READ smallUpdate NOTIFY stateChanged FINAL)
    // bytes to download, 0 when unknown
    Q_PROPERTY(qint64 updateSize READ updateSize NOTIFY stateChanged FINAL)
    // of the download, 0 to 1
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged FINAL)
    Q_PROPERTY(bool manual READ manual NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool showBanner READ showBanner NOTIFY stateChanged FINAL)
    // About shows the state itself: no banner while it is open
    Q_PROPERTY(bool aboutOpen READ aboutOpen WRITE setAboutOpen NOTIFY stateChanged FINAL)
    Q_PROPERTY(QString repositoryUrl READ repositoryUrl CONSTANT FINAL)

public:
    enum State {
        Idle,
        Checking,
        UpToDate,
        Available,
        Downloading,
        // the new files are being written and checked
        Preparing,
        // they wait for VLC to restart or to close
        Ready,
        Failed
    };
    Q_ENUM(State)

    explicit RedesignUpdater(qt_intf_t *intf, QObject *parent = nullptr);
    ~RedesignUpdater() override;

    State state() const { return m_state; }
    QString currentVersion() const;
    QString latestVersion() const { return m_latestVersion; }
    QString releaseTitle() const { return m_releaseTitle; }
    QString summary() const { return m_summary; }
    QString errorText() const { return m_error; }
    bool canInstall() const;
    bool smallUpdate() const;
    qint64 updateSize() const;
    double progress() const { return m_progress; }
    bool manual() const { return m_manual; }
    bool showBanner() const;
    bool aboutOpen() const { return m_aboutOpen; }
    void setAboutOpen(bool open);
    QString repositoryUrl() const;

    // a < b, comparing "1.2.3" or "v1.2.3-beta.1" like semantic versions
    static bool versionLess(const QString &a, const QString &b);

    // Writes the files of a .vrdelta under <appDir>/.update/staged, each
    // checked against its SHA-256, the files it removes in removed.txt, the
    // helper that puts them in place, and the version in ready.txt (last).
    // Returns an error, empty on success.
    static QString stageDelta(const QByteArray &delta, const QString &appDir, const QString &version);

public slots:
    // manual: show "up to date" and errors too, ignore a skipped version
    void check(bool manual = true);
    void openReleasePage();
    void install();
    // the staged update: in place now, and VLC opens again where it was
    void restartNow();
    void skipThisVersion();
    // the banner closed; a download going on stops (the update stays
    // available)
    void dismiss();

signals:
    void stateChanged();
    void progressChanged();

private:
    void setState(State state, const QString &error = {});
    void setProgress(double progress);
    void onRelease(int status, const QByteArray &body);
    void withChecksum(const QString &assetName, std::function<void(const QString &)> next);
    void downloadDelta(const QString &expectedSha256);
    void downloadInstaller(const QString &expectedSha256);
    void launchInstaller(const QString &path);
    bool launchHelper(bool relaunch);
    void applyOnExit();
    void findStagedUpdate();
    void resumeAfterUpdate();
    bool isInstalledCopy() const;
    bool appFolderWritable() const;
    QString appDir() const;

    qt_intf_t *m_intf;
    QTimer m_startupTimer;
    State m_state = Idle;
    bool m_manual = false;
    bool m_dismissed = false;
    bool m_aboutOpen = false;
    bool m_helperStarted = false;
    // the small update failed in this session: the installer instead
    bool m_deltaFailed = false;
    // whether the folder of vlc.exe takes the update: -1 until checked
    mutable int m_writable = -1;
    // the download going on (the answers of one cancelled are dropped),
    // and how to stop its request
    quint64 m_attempt = 0;
    std::function<void()> m_cancel;
    double m_progress = 0;
    QString m_latestVersion;
    QString m_releaseTitle;
    QString m_summary;
    QString m_releaseUrl;
    QString m_installerUrl;
    QString m_installerName;
    qint64 m_installerSize = 0;
    QString m_deltaUrl;
    QString m_deltaName;
    qint64 m_deltaSize = 0;
    QString m_sumsUrl;
    QString m_error;
};
