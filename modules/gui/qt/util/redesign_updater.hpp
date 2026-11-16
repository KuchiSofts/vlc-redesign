/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
#pragma once

#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include <QTimer>

#include <functional>
#include <memory>

class QJsonArray;

#include "qt.hpp"

// Updates of VLC Redesign from its GitHub releases (not VideoLAN's feed,
// which offers the official VLC). Checks the latest release at startup
// (option redesign-update-check, at most once a day) or on request.
//
// A release carries one small update, from the release before it
// (vlc-redesign-<new>-delta-from-<old>.vrdelta, see
// extras/juxtopposed/release/make-delta.py): only the files that changed,
// as binary patches. This version finds its way to the newest through the
// releases between them (0.7.0 -> 0.7.1 -> 0.7.2; a release up to 0.6.9
// also carries small updates from older ones, taken when shorter). When
// there is a way and VLC's folder is writable (an installed copy for the
// user, or a portable one), its small updates are downloaded in the
// background, each checked against its own release's signed
// SHA256SUMS.txt, and the new files are written next to the old ones
// (<folder>/.update/staged), each checked against its SHA-256, on a worker
// thread. vlc-redesign-update.exe then puts them in place once VLC has
// quit, either now (Restart) or when it closes, and opens VLC again where
// it was. Otherwise an installed copy downloads and runs the installer, and
// a portable one opens the release page. Linux replaces the AppImage it runs
// from; macOS replaces its application with the one of the release's disk
// image (extras/juxtopposed/macos/vlc-redesign-update.sh), both checked
// against the signed SHA256SUMS.txt and opened again where they were.
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
    // "Also on: Linux (soon), macOS (soon)…": the other platforms, from the
    // copy of platforms.yml built in (:/redesign/platforms.json); empty without it
    Q_PROPERTY(QString alsoOn READ alsoOn CONSTANT FINAL)
    // the system and processor this copy is built for (About): "Windows x64",
    // "macOS Apple Silicon", "Linux x86_64 · AppImage"...
    Q_PROPERTY(QString platformName READ platformName CONSTANT FINAL)
    // the system it runs on, for the tooltip of that name (About): "Windows 11
    // (10.0.26100) · x86_64 · Qt 6.8.3"
    Q_PROPERTY(QString platformDetails READ platformDetails CONSTANT FINAL)

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
    QString alsoOn() const;
    QString platformName() const;
    QString platformDetails() const;

    // The daily check (option redesign-update-check), for the first-run
    // setup: turned on, it starts as at a start of VLC
    Q_INVOKABLE bool automaticCheck() const;
    Q_INVOKABLE void setAutomaticCheck(bool on);

    // a < b, comparing "1.2.3" or "v1.2.3-beta.1" like semantic versions
    static bool versionLess(const QString &a, const QString &b);

    // An update is staged under <appDir>/.update/staged from one small update
    // (.vrdelta) or several in a row, each from the version the one before it
    // gives (0.7.0 -> 0.7.1 -> 0.7.2: a release carries the small update
    // from the release before it only). stageBegin empties the folder;
    // stageStep writes the files of one .vrdelta, each checked against its
    // SHA-256 (a file of a step before it is read from the staged folder, the
    // others from the application's) and keeps the files it removes in
    // `removed` (lower-case path -> path); stageFinish writes removed.txt, the
    // helper that puts them in place, and the version in ready.txt (last).
    // Each returns an error, empty on success.
    static QString stageBegin(const QString &appDir);
    static QString stageStep(const QByteArray &delta, const QString &appDir, const QString &from,
                             const QString &to, QMap<QString, QString> &removed);
    static QString stageFinish(const QString &appDir, const QString &version,
                               const QMap<QString, QString> &removed);

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
    void scheduleStartupCheck();
    void setState(State state, const QString &error = {});
    void setProgress(double progress);
    void onRelease(int status, const QByteArray &body);
    // One small update on the way from this version to the newest: a
    // release's .vrdelta and that release's signed checksums
    struct UpdateStep {
        QString from;       // the version it applies to
        QString version;    // the version it gives
        QString url;
        QString name;
        qint64 size = 0;
        QString sumsUrl;
        QString sigUrl;
    };
    void findSteps(const QJsonArray &releases);
    void downloadStep(int index, qint64 before, const QString &expectedSha256);
    void withChecksum(const QString &assetName, const QString &version, const QString &sumsUrl,
                      const QString &sigUrl, std::function<void(const QString &)> next);
    void useChecksums(const QByteArray &sums, const QString &assetName,
                      const std::function<void(const QString &)> &next);
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
    // an update staged by a session before, its programs checked again
    // (findStagedUpdate): no check of the releases meanwhile
    bool m_verifyingStaged = false;
    // the small update failed in this session: the installer instead
    bool m_deltaFailed = false;
    // a small update is being written (between two downloads): not stopped
    bool m_staging = false;
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
    // the small updates from this version to the newest, in their order
    // (empty: none, the installer then), and their size together
    QList<UpdateStep> m_steps;
    qint64 m_deltaSize = 0;
    // the files the small updates of this download take away (stageStep)
    std::shared_ptr<QMap<QString, QString>> m_removed;
    QString m_sumsUrl;
    // its signature (SHA256SUMS.txt.sig), from the first signed release on
    QString m_sigUrl;
    QString m_error;
};
