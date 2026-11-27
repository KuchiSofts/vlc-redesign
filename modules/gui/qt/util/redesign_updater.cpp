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

#include "redesign_updater.hpp"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QSaveFile>
#include <QLocale>
#include <QSysInfo>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QProcess>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QStringList>
#include <QThread>
#include <QtEndian>
#include <QUrl>

#include <algorithm>
#include <cstring>
#include <memory>

#ifdef _WIN32
# include <QDirIterator>
# include <windows.h>
# include <wincrypt.h>
# include <wintrust.h>
# include <softpub.h>
# include "util/redesign_update_key.h"
# include "util/redesign_update_verify.h"
#elif defined(__linux__)
# include <cstdio>
# include "util/redesign_update_key.h"
# include "util/redesign_update_verify.h"
#elif defined(__APPLE__)
# include <cerrno>
# include <csignal>
# include <vector>
# include <sys/wait.h>
# include <QTemporaryFile>
# include <vlc_spawn.h>
# include "util/redesign_update_key.h"
# include "util/redesign_update_verify.h"
#endif

#include "dialogs/dialogs_provider.hpp"
#include "dialogs/firstrun/whats_new_text.hpp"
#include "maininterface/mainctx.hpp"
#include "player/player_controller.hpp"
#include "util/http_request.hpp"
#include "util/redesign_version.hpp"

namespace {

// The newest releases (not only GitHub's "latest"): a release made for some
// systems only (0.6.4 had no Android app) must not hide the last one of this
// system, nor be offered to it (onRelease picks the newest with its package).
// Fifteen keep the answer small (a release's notes and its files made 70 KB
// of JSON up to 0.6.9, about a third of it since: 30 releases were 1.4 MB,
// over the old 1 MB limit: "GitHub sent an unexpected answer (200)" in
// 0.6.5's first build) and are the way a copy finds from its version to the
// newest: a release carries the small update from the release before it only
// (findSteps), so a copy fourteen releases behind still updates with them.
const char releasesApi[] = "https://api.github.com/repos/" VLC_REDESIGN_REPO "/releases?per_page=15";
// (a way through more releases than this is no small update anymore)
[[maybe_unused]] constexpr int maxUpdateSteps = 14;
const qint64 releasesMaxBytes = 8 * 1024 * 1024;
const char latestReleasePage[] = VLC_REDESIGN_REPO_URL "/releases/latest";
const char keyLastCheck[] = "redesign/update-last-check";
const char keySkipped[] = "redesign/update-skip";
// each step of a download (a stalled read ends it), and the whole of it
constexpr int downloadStepMs = 30 * 1000;
constexpr int downloadDeadlineMs = 30 * 60 * 1000;
// Every release's SHA256SUMS.txt is signed with the project's key
// (SHA256SUMS.txt.sig, redesign_update_key.h) since 0.4.0: a release whose
// checksums don't carry a good signature is never installed.
const char sumsName[] = "SHA256SUMS.txt";
const char sumsSignatureName[] = "SHA256SUMS.txt.sig";
// The files of this build's architecture (docs/MASTER-PLAN.md, 13.5): an
// x64 copy never takes an ARM64 file or the other way round, even when it
// runs emulated. The x64 small updates keep the name they had before ARM64
// (the copies already installed look for it).
#if defined(_WIN32) && (defined(_M_ARM64) || defined(__aarch64__))
const char installerSuffix[] = "-winarm64-setup.exe";
const char deltaInfix[] = "-winarm64";
#elif defined(_WIN32)
const char installerSuffix[] = "-win64-setup.exe";
const char deltaInfix[] = "";
#elif defined(__APPLE__)
// macOS updates the application it runs from with the release's disk image
// of this Mac's architecture (extras/juxtopposed/macos/vlc-redesign-update.sh
// copies its application next to this one, then swaps them once VLC has
// quit); a release without that image is not an update of this Mac
# if defined(__aarch64__) || defined(__arm64__)
const char ownPackageSuffix[] = "-macos-arm64.dmg";
const char ownArchiveSuffix[] = "-macos-arm64.app.zip";
# else
const char ownPackageSuffix[] = "-macos-x86_64.dmg";
const char ownArchiveSuffix[] = "-macos-x86_64.app.zip";
# endif
// (#117) The release's zip of the application is taken before its disk image
// when there is one: the helper unpacks it with ditto, no image to open (on a
// user's Mac the attach of the image never answered when VLC started it)

// The application bundle this VLC runs from (…/VLC Redesign.app), empty when
// it doesn't run from one
QString ownBundle()
{
    const QString bundle = QDir(QCoreApplication::applicationDirPath() + QStringLiteral("/../..")).canonicalPath();
    return bundle.endsWith(QLatin1String(".app"), Qt::CaseInsensitive) ? bundle : QString();
}

QString updateScript()
{
    return QCoreApplication::applicationDirPath() + QStringLiteral("/../Resources/vlc-redesign-update.sh");
}

// A program run to its end, never longer than limitMs: its exit code (-1 when
// it was ended for taking too long, -2 when it did not end by itself, -3 when
// it could not be started) and what it printed.
//
// (#117) Not QProcess: VLC blocks SIGCHLD in all its threads (bin/darwinvlc.m)
// and Qt hears of a child's end on macOS only from that signal, so
// waitForFinished() never saw the helper end (a user's Mac waited the whole
// ten, then six, minutes on "Preparing the update", whatever the helper did).
// waitpid() needs no signal. vlc_spawn() also clears the blocked signals in
// the child: inherited, SIGTERM blocked kept the helper's shell from stopping
// the watcher of a step that had ended, and every step "did not answer" after
// its two minutes.
struct HelperRun
{
    int code = -3;
    QByteArray out;
    QByteArray err;
};

HelperRun runHelper(const QStringList &args, qint64 limitMs)
{
    HelperRun run;
    // (its outputs to files: a pipe nobody reads while it runs could fill)
    QTemporaryFile out, err;
    if (args.isEmpty() || !out.open() || !err.open())
        return run;
    std::vector<QByteArray> names;
    for (const QString &arg : args)
        names.push_back(QFile::encodeName(arg));
    std::vector<const char *> argv;
    for (const QByteArray &name : names)
        argv.push_back(name.constData());
    argv.push_back(nullptr);
    // (nothing typed to it: -1 is /dev/null)
    const int fds[] = { -1, out.handle(), err.handle(), -1 };
    pid_t pid;
    if (vlc_spawn(&pid, argv[0], fds, argv.data()) != 0)
        return run;
    QElapsedTimer taken;
    taken.start();
    int status = 0;
    pid_t ended;
    for (;;) {
        ended = waitpid(pid, &status, WNOHANG);
        if (ended == -1 && errno == EINTR)
            continue;
        if (ended != 0 || taken.elapsed() >= limitMs)
            break;
        QThread::msleep(200);
    }
    if (ended == 0) {
        // Asked to stop first: the helper then ends the step it is in,
        // releases the image and removes a copy that is not whole (its own
        // last lines). Ended outright only when it has not after 15 s: that
        // left the image attached and its tool running.
        kill(pid, SIGTERM);
        QElapsedTimer asked;
        asked.start();
        do {
            QThread::msleep(200);
            ended = waitpid(pid, &status, WNOHANG);
        } while ((ended == 0 || (ended == -1 && errno == EINTR)) && asked.elapsed() < 15000);
        if (ended == 0 || (ended == -1 && errno == EINTR)) {
            kill(pid, SIGKILL);
            vlc_waitpid(pid);
            ended = 0;
        }
        // (it ended well by itself just as its time was up: its copy is whole)
        run.code = ended == pid && WIFEXITED(status) && WEXITSTATUS(status) == 0 ? 0 : -1;
    } else {
        run.code = ended == pid && WIFEXITED(status) ? WEXITSTATUS(status) : -2;
    }
    QFile said(out.fileName());
    if (said.open(QIODevice::ReadOnly))
        run.out = said.readAll();
    QFile errors(err.fileName());
    if (errors.open(QIODevice::ReadOnly))
        run.err = errors.readAll();
    return run;
}
#elif defined(__linux__)
// Linux updates the AppImage it runs from (the release's AppImage of this
// architecture, put in its place); the other packages open the release page
# if defined(__aarch64__)
const char installerSuffix[] = "-aarch64.AppImage";
# else
const char installerSuffix[] = "-x86_64.AppImage";
# endif
#endif

// Whether a release carries this build's package: its installer (Windows),
// its AppImage (Linux) or its disk image (macOS) of this architecture
bool hasOwnPackage(const QJsonObject &release)
{
#if defined(_WIN32) || defined(__linux__)
    const QLatin1String suffix(installerSuffix);
#elif defined(__APPLE__)
    const QLatin1String suffix(ownPackageSuffix);
#else
    return false;
#endif
#if defined(_WIN32) || defined(__linux__) || defined(__APPLE__)
    for (const QJsonValue asset : release.value(QLatin1String("assets")).toArray()) {
        const QString name = asset.toObject().value(QLatin1String("name")).toString();
        if (name.endsWith(suffix, Qt::CaseInsensitive))
            return true;
#ifdef __APPLE__
        if (name.endsWith(QLatin1String(ownArchiveSuffix), Qt::CaseInsensitive))
            return true;
#endif
    }
    return false;
#endif
}

#if !defined(NDEBUG) && defined(VLC_REDESIGN_UPDATE_TEST_SERVER)
// for testing updates against a local server, not GitHub: only in a build
// made for it (VLC_REDESIGN_UPDATE_TEST_SERVER defined), never a release
QString testApi()
{
    return QString::fromUtf8(qgetenv("VLC_REDESIGN_UPDATE_API"));
}
#endif

// What is downloaded comes from the project's own releases on GitHub, over
// HTTPS (WinHTTP then follows its redirects to its storage, never to plain
// HTTP): never a file of another repository the release data could name
bool trustedUrl(const QString &address)
{
    const QUrl url(address);
    const QString host = url.host().toLower();
    if (url.isValid() && url.scheme() == QLatin1String("https") && url.userInfo().isEmpty()
        && url.port(443) == 443 && host == QLatin1String("github.com")
        && url.path().startsWith(QLatin1String("/" VLC_REDESIGN_REPO "/releases/download/"), Qt::CaseInsensitive)
        && !url.path().contains(QLatin1String("/../")))
        return true;
#if !defined(NDEBUG) && defined(VLC_REDESIGN_UPDATE_TEST_SERVER)
    // the test server's own files
    const QUrl api(testApi());
    return !api.host().isEmpty() && url.scheme() == api.scheme() && url.host() == api.host()
           && url.port() == api.port();
#else
    return false;
#endif
}

#ifdef _WIN32
// The publisher an Authenticode signature names, if the file carries one
// Windows trusts; empty otherwise. wintrust.dll and crypt32.dll are loaded
// here: nothing else of the interface needs them.
QString trustedSigner(const QString &path)
{
    using VerifyFn = LONG(WINAPI *)(HWND, GUID *, LPVOID);
    using ProvDataFn = CRYPT_PROVIDER_DATA *(WINAPI *)(HANDLE);
    using ProvSignerFn = CRYPT_PROVIDER_SGNR *(WINAPI *)(CRYPT_PROVIDER_DATA *, DWORD, BOOL, DWORD);
    using NameFn = DWORD(WINAPI *)(PCCERT_CONTEXT, DWORD, DWORD, void *, LPWSTR, DWORD);
    static HMODULE wintrust = LoadLibraryExW(L"wintrust.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    static HMODULE crypt32 = LoadLibraryExW(L"crypt32.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!wintrust || !crypt32)
        return {};
    const auto verify = reinterpret_cast<VerifyFn>(
        reinterpret_cast<void *>(GetProcAddress(wintrust, "WinVerifyTrust")));
    const auto provData = reinterpret_cast<ProvDataFn>(
        reinterpret_cast<void *>(GetProcAddress(wintrust, "WTHelperProvDataFromStateData")));
    const auto provSigner = reinterpret_cast<ProvSignerFn>(
        reinterpret_cast<void *>(GetProcAddress(wintrust, "WTHelperGetProvSignerFromChain")));
    const auto nameOf = reinterpret_cast<NameFn>(
        reinterpret_cast<void *>(GetProcAddress(crypt32, "CertGetNameStringW")));
    if (!verify || !provData || !provSigner || !nameOf)
        return {};

    const QString native = QDir::toNativeSeparators(path);
    WINTRUST_FILE_INFO file = {};
    file.cbStruct = sizeof(file);
    file.pcwszFilePath = reinterpret_cast<LPCWSTR>(native.utf16());
    WINTRUST_DATA data = {};
    data.cbStruct = sizeof(data);
    data.dwUIChoice = WTD_UI_NONE;
    // (no revocation lists: offline, no update would ever be accepted)
    data.fdwRevocationChecks = WTD_REVOKE_NONE;
    data.dwUnionChoice = WTD_CHOICE_FILE;
    data.pFile = &file;
    data.dwStateAction = WTD_STATEACTION_VERIFY;
    GUID action = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    QString name;
    if (verify(static_cast<HWND>(INVALID_HANDLE_VALUE), &action, &data) == ERROR_SUCCESS) {
        CRYPT_PROVIDER_DATA *provider = provData(data.hWVTStateData);
        CRYPT_PROVIDER_SGNR *signer = provider ? provSigner(provider, 0, FALSE, 0) : nullptr;
        if (signer && signer->csCertChain > 0 && signer->pasCertChain[0].pCert) {
            // the subject and the authority that issued its certificate: a
            // certificate of the same name from another authority is another
            // publisher (signerDisplayName shows the subject only)
            wchar_t subject[256] = {}, issuer[256] = {};
            if (nameOf(signer->pasCertChain[0].pCert, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, nullptr, subject, 256) > 1
                && nameOf(signer->pasCertChain[0].pCert, CERT_NAME_SIMPLE_DISPLAY_TYPE, CERT_NAME_ISSUER_FLAG,
                          nullptr, issuer, 256) > 1)
                name = QString::fromWCharArray(subject) + QChar(0x1F) + QString::fromWCharArray(issuer);
        }
    }
    data.dwStateAction = WTD_STATEACTION_CLOSE;
    verify(static_cast<HWND>(INVALID_HANDLE_VALUE), &action, &data);
    return name;
}
#endif

// Whether a release must come with signed checksums: always. Every release
// from 0.4.0 on has them, and a copy only updates to a newer one. (By the
// release's version, an old tag offered as an update, with a version taken
// from the test switches, was installed without its signature.)
bool signatureRequired(const QString &)
{
    return true;
}

// SHA256SUMS.txt checked against its signature by the key the app carries:
// whoever can publish on GitHub (an account, a token) still can't make the
// checksums of an update without the private key, which never leaves the
// release PC (extras/juxtopposed/signing/release-key.ps1)
bool goodSignature(const QByteArray &sums, const QByteArray &signature)
{
#if defined(_WIN32) || defined(__linux__) || defined(__APPLE__)
    return vlc_redesign_verify_signature(vlc_redesign_update_key, sums.constData(),
                                         static_cast<size_t>(sums.size()),
                                         reinterpret_cast<const unsigned char *>(signature.constData()),
                                         static_cast<size_t>(signature.size())) == 1;
#else
    // (no updater: nothing is installed from here)
    Q_UNUSED(sums);
    Q_UNUSED(signature);
    return false;
#endif
}

// The checksums don't tell who built the files they list unless they are
// signed (above). A copy whose vlc.exe is signed (the releases signed
// through SignPath, CODE_SIGNING_POLICY.md) also takes only programs signed
// by the same publisher: an error for a file that isn't. A copy that isn't
// signed has no publisher to compare with (its builds by hand, the releases
// before signing): the checksums are then the check.
QString publisherMismatch(const QString &path)
{
#ifdef _WIN32
    static const QString own = trustedSigner(QCoreApplication::applicationFilePath());
    if (own.isEmpty() || trustedSigner(path) == own)
        return {};
    return qtr("The update isn't signed by the publisher of this copy (%1), so it wasn't used.")
        .arg(own.section(QChar(0x1F), 0, 0));
#else
    Q_UNUSED(path);
    return {};
#endif
}

// The same for every program of a staged small update
QString stagedPublisherMismatch(const QString &staged)
{
#ifdef _WIN32
    QDirIterator it(staged, { QStringLiteral("*.exe"), QStringLiteral("*.dll") }, QDir::Files,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString error = publisherMismatch(it.next());
        if (!error.isEmpty())
            return error;
    }
#else
    Q_UNUSED(staged);
#endif
    return {};
}

HttpRequest githubRequest(const QString &url, qsizetype maxBytes, int timeoutMs)
{
    HttpRequest request;
    request.url = QUrl(url);
    request.maxBytes = maxBytes;
    request.timeoutMs = timeoutMs;
    // GitHub refuses API requests without a User-Agent
    request.headers.append(qMakePair(QByteArray("User-Agent"), QByteArray("VLC-Redesign/" VLC_REDESIGN_VERSION)));
    request.headers.append(qMakePair(QByteArray("Accept"), QByteArray("application/vnd.github+json")));
    // (the CI's test of the update through the app, #117: the shared addresses of the CI's machines soon reach
    // GitHub's limit for anonymous API requests. With the test switch on, the workflow's own token goes with the
    // requests to GitHub's API, and to nothing else)
    if (qEnvironmentVariable("VLC_REDESIGN_UPDATE_TEST_AUTO") == QLatin1String("1")
        && request.url.scheme() == QLatin1String("https") && request.url.host() == QLatin1String("api.github.com")) {
        const QByteArray token = qgetenv("VLC_REDESIGN_UPDATE_TEST_TOKEN");
        if (!token.isEmpty() && token.size() < 512 && !token.contains('\n') && !token.contains('\r'))
            request.headers.append(qMakePair(QByteArray("Authorization"), QByteArray("Bearer ") + token));
    }
    return request;
}

// "v1.2.3-beta.1" -> numbers {1, 2, 3} and pre-release "beta.1"
void splitVersion(QString v, QList<int> &numbers, QString &pre)
{
    v = v.trimmed();
    if (v.startsWith('v') || v.startsWith('V'))
        v.remove(0, 1);
    const qsizetype dash = v.indexOf('-');
    pre = dash >= 0 ? v.mid(dash + 1) : QString();
    const QString core = dash >= 0 ? v.left(dash) : v;
    for (const QString &part : core.split('.'))
        numbers.append(part.toInt());
    while (numbers.size() < 3)
        numbers.append(0);
}

// The release notes in a few plain lines (WhatsNewText::releaseSummary: the
// card of 0.8.3 on Windows began with a Samsung TV line and an Android TV
// one, and a bullet wrapped over two lines lost its second)
QString summarise(const QString &markdown)
{
    return WhatsNewText::releaseSummary(markdown);
}

// The delta of a release (make-delta.py): its blocks are in the qCompress
// format, a patch is bsdiff's (control triples, differences, extra bytes)
QByteArray block(const char *blobs, qsizetype available, qint64 offset, qint64 length)
{
    if (offset < 0 || length < 4 || offset + length > available)
        return {};
    return qUncompress(reinterpret_cast<const uchar *>(blobs + offset), static_cast<qsizetype>(length));
}

bool bspatch(const QByteArray &old, const QByteArray &control, const QByteArray &diff,
             const QByteArray &extra, qint64 size, QByteArray &out)
{
    if (control.size() % 24 || size < 0)
        return false;
    out = QByteArray(static_cast<qsizetype>(size), Qt::Uninitialized);
    char *dst = out.data();
    const char *src = old.constData();
    const qint64 oldSize = old.size();
    qint64 newPos = 0, oldPos = 0, diffPos = 0, extraPos = 0;
    for (qsizetype i = 0; i < control.size(); i += 24) {
        const qint64 x = qFromLittleEndian<qint64>(control.constData() + i);
        const qint64 y = qFromLittleEndian<qint64>(control.constData() + i + 8);
        const qint64 z = qFromLittleEndian<qint64>(control.constData() + i + 16);
        if (x < 0 || y < 0 || newPos + x + y > size || diffPos + x > diff.size() || extraPos + y > extra.size())
            return false;
        // x bytes: the old ones plus the difference
        for (qint64 k = 0; k < x; ++k) {
            const qint64 o = oldPos + k;
            const char base = (o >= 0 && o < oldSize) ? src[o] : 0;
            dst[newPos + k] = static_cast<char>(base + diff.constData()[diffPos + k]);
        }
        newPos += x;
        oldPos += x;
        diffPos += x;
        memcpy(dst + newPos, extra.constData() + extraPos, static_cast<size_t>(y));
        newPos += y;
        extraPos += y;
        oldPos += z;
    }
    return newPos == size;
}

QString sha256Hex(const QByteArray &data)
{
    return QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());
}

// A path of the delta, relative to VLC's folder: never out of it, never
// the user's data
bool safeRelativePath(const QString &path)
{
    if (path.isEmpty() || path.contains(QLatin1String("..")) || path.contains(':')
        || path.startsWith('/') || path.startsWith('\\'))
        return false;
    // each part of it: none empty or ".", no short 8.3 name ("PORTAB~1"), none
    // ending in a dot or a space (Windows reads those as another name), so the
    // first part really is not the user's data nor the update's own files
    // ("./portable/vlcrc" passed a check of the start alone)
    const QStringList parts = QString(path).replace('\\', '/').split('/');
    for (const QString &part : parts)
        if (part.isEmpty() || part == QLatin1String(".") || part.contains('~')
            || part.endsWith('.') || part.endsWith(' '))
            return false;
    return !parts.first().startsWith(QLatin1String("portable"), Qt::CaseInsensitive)
           && !parts.first().startsWith(QLatin1String(".update"), Qt::CaseInsensitive);
}

// A file whole on the disk before it has its name: QSaveFile writes a
// temporary file, syncs it (fsync or fdatasync; FlushFileBuffers on
// Windows) and only then renames it into place
// (extras/juxtopposed/savefile-sync-test.cpp sees that order); with these
// permissions when some are given
bool writeDurably(const QString &path, const QByteArray &data,
                  QFileDevice::Permissions permissions = QFileDevice::Permissions())
{
    QSaveFile out(path);
    if (!out.open(QIODevice::WriteOnly) || out.write(data) != data.size())
        return false;
    if (permissions != QFileDevice::Permissions() && !out.setPermissions(permissions))
        return false;
    return out.commit();
}

// vlc-redesign-update.exe puts the update in place once VLC has quit: a copy
// of it, so the update can replace it too. Run once now, on the worker
// thread (it returns at once without arguments): Windows may check a new
// program for seconds when it first runs, better now than on Restart.
bool prepareHelper(const QString &dir)
{
    const QString helper = dir + QStringLiteral("/.update/vlc-redesign-update.exe");
    QFile::remove(helper);
    // (written to the disk too: ready.txt follows it)
    QFile source(dir + QStringLiteral("/vlc-redesign-update.exe"));
    if (!source.open(QIODevice::ReadOnly) || !writeDurably(helper, source.readAll()))
        return false;
    QProcess probe;
    probe.start(helper, {});
    probe.waitForFinished(30000);
    return true;
}

// Where an installer (Windows) or a disk image (macOS) is saved before it is
// run (downloadInstaller)
QString installerFolder()
{
    const QString temp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    // (none known: nowhere, never at the root of the drive)
    return temp.isEmpty() ? QString() : temp + QStringLiteral("/vlc-redesign-update");
}

// A link, of either kind: Qt tells a Windows junction apart from a symbolic
// link, and QDir::removeRecursively() goes into a junction
bool isLink(const QFileInfo &info)
{
    return info.isSymLink() || info.isJunction();
}

bool removeFile(const QString &path)
{
    if (QFile::remove(path))
        return true;
    // (a read-only file, as QDir::removeRecursively() does)
    const QFile::Permissions permissions = QFile::permissions(path);
    return !(permissions & QFile::WriteUser)
        && QFile::setPermissions(path, permissions | QFile::WriteUser) && QFile::remove(path);
}

// A folder of the updater's own removed with what it holds, never through a
// link: a junction planted in a folder that is cleaned up would have the
// files of its target removed. A link is removed as the link.
bool removeTree(const QString &path)
{
    const QFileInfo top(path);
    if (isLink(top))
        return QFile::remove(path) || QDir().rmdir(path);
    if (!top.isDir())
        return !top.exists() || removeFile(path);
    bool ok = true;
    const QFileInfoList entries = QDir(path).entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot
                                                           | QDir::Hidden | QDir::System);
    for (const QFileInfo &entry : entries) {
        const QString file = entry.filePath();
        if (isLink(entry))
            ok = (QFile::remove(file) || QDir().rmdir(file)) && ok;
        else if (entry.isDir())
            ok = removeTree(file) && ok;
        else
            ok = removeFile(file) && ok;
    }
    return QDir().rmdir(path) && ok;
}

// The installers the sessions before downloaded (run once, then left there:
// a hundred MB each) go at the start. Those of the last hour stay: another
// VLC may be saving one now, or the installer still running (it opens VLC
// again at its end); the next start takes them
void removeOldInstallers()
{
#ifdef __linux__
    // Nothing is saved there on Linux (the AppImage is replaced where it
    // is), and /tmp is every user's: a folder of this name can be another
    // user's, with links in it. Left alone.
    return;
#endif
    const QString folder = installerFolder();
    if (folder.isEmpty())
        return;
    const QDir dir(folder);
    if (!dir.exists())
        return;
    // (a link of this name, planted in a temporary folder that several users
    // share: the files of its target are not ours to remove)
    if (isLink(QFileInfo(folder)))
        return;
    const QDateTime before = QDateTime::currentDateTime().addSecs(-3600);
    for (const QFileInfo &entry : dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System))
    {
        // Read again: a download of this session may have just replaced it,
        // and what it is now is what is removed (a folder that became a link
        // or a junction meanwhile goes as the link, never what it leads to)
        const QFileInfo now(entry.filePath());
        if (now.lastModified() >= before)
            continue;
        removeTree(now.filePath());
    }
    // (the folder too, once it is empty)
    QDir().rmdir(folder);
}

} // namespace

RedesignUpdater::RedesignUpdater(qt_intf_t *intf, QObject *parent)
    : QObject(parent)
    , m_intf(intf)
{
    m_startupTimer.setSingleShot(true);
    m_startupTimer.setInterval(8000);
    connect(&m_startupTimer, &QTimer::timeout, this, [this]() {
        if (m_state != Idle || m_verifyingStaged)
            return;
        const QDateTime last = QDateTime::fromString(
            m_intf->mainSettings->value(keyLastCheck).toString(), Qt::ISODate);
        if (!last.isValid() || last.secsTo(QDateTime::currentDateTimeUtc()) > 20 * 3600)
            check(false);
    });
    connect(qApp, &QCoreApplication::aboutToQuit, this, &RedesignUpdater::applyOnExit);
    runInBackground(this, []() {
        QThread::currentThread()->setPriority(QThread::LowPriority);
        removeOldInstallers();
    });
    findStagedUpdate();
    resumeAfterUpdate();
    scheduleStartupCheck();

    // The CI's test of the update through the app (#117, extras/juxtopposed/ci/macos-app-update-test.sh): with
    // VLC_REDESIGN_UPDATE_TEST_AUTO=1 the check, Update now and Restart now are pressed by themselves, so the
    // app's own updater (its helper, the child process it waits for) runs as a user starts it. The update is
    // checked as always (the release key's signature of the checksums).
    if (qEnvironmentVariable("VLC_REDESIGN_UPDATE_TEST_AUTO") == QLatin1String("1")) {
        msg_Info(m_intf, "update test: the check, Update now and Restart now go by themselves");
        connect(this, &RedesignUpdater::stateChanged, this, [this]() {
            msg_Info(m_intf, "update test: state %d", int(m_state));
            if (m_state == Available && canInstall())
                QTimer::singleShot(500, this, &RedesignUpdater::install);
            else if (m_state == Ready)
                QTimer::singleShot(500, this, &RedesignUpdater::restartNow);
        });
        QTimer::singleShot(3000, this, [this]() {
            if (m_state == Idle || m_state == UpToDate)
                check(true);
        });
    }
}

// A few seconds after the start, unless the user is yet to say whether VLC
// may check (a new profile: the first-run setup asks)
void RedesignUpdater::scheduleStartupCheck()
{
    if (automaticCheck() && httpRequestSupported()
        && !(m_intf->p_mi && m_intf->p_mi->onlineChoicesPending()))
        m_startupTimer.start();
    else
        m_startupTimer.stop();
}

bool RedesignUpdater::automaticCheck() const
{
    return var_InheritBool(m_intf, "redesign-update-check");
}

void RedesignUpdater::setAutomaticCheck(bool on)
{
    if (on != automaticCheck())
    {
        config_PutInt("redesign-update-check", on ? 1 : 0);
        config_SaveConfigFile(m_intf);
    }
    scheduleStartupCheck();
}

RedesignUpdater::~RedesignUpdater()
{
    // VLC is quitting: the interface (and this) can go before the event
    // loop ends, so aboutToQuit alone may come too late
    if (qApp && qApp->property("isDying").toBool())
        applyOnExit();
}

QString RedesignUpdater::currentVersion() const
{
    // (the CI's test of the update through the app, #117: a build that takes itself for an older version,
    // VLC_REDESIGN_UPDATE_TEST_AS=0.8.6, so that the latest release is an update for it)
    static const QString testAs = []() {
        const QString as = qEnvironmentVariable("VLC_REDESIGN_UPDATE_TEST_AS");
        return as.size() <= 16 && QRegularExpression(QStringLiteral("^\\d+\\.\\d+\\.\\d+$")).match(as).hasMatch()
               ? as : QString();
    }();
    return testAs.isEmpty() ? QStringLiteral(VLC_REDESIGN_VERSION) : testAs;
}

QString RedesignUpdater::repositoryUrl() const
{
    return QStringLiteral(VLC_REDESIGN_REPO_URL);
}

QString RedesignUpdater::alsoOn() const
{
    // platforms.yml as extras/juxtopposed/platforms/render.py writes it for the
    // website (site/platforms.json), built in by assets.qrc: nothing is fetched
#if defined(_WIN32) && (defined(_M_ARM64) || defined(__aarch64__))
    const QString self = QStringLiteral("windows-arm64");
#elif defined(_WIN32)
    const QString self = QStringLiteral("windows");
#elif defined(__APPLE__)
    const QString self = QStringLiteral("macos");
#elif defined(__ANDROID__)
    const QString self = QStringLiteral("android");
#else
    const QString self = QStringLiteral("linux");
#endif
    QFile file(QStringLiteral(":/redesign/platforms.json"));
    if (!file.open(QIODevice::ReadOnly))
        return {};
    const QJsonArray platforms = QJsonDocument::fromJson(file.readAll()).object()
                                     .value(QLatin1String("platforms")).toArray();
    QStringList items;
    for (const QJsonValue &value : platforms)
    {
        const QJsonObject p = value.toObject();
        if (p.value(QLatin1String("id")).toString() == self)
            continue;
        const QString name = p.value(QLatin1String("short")).toString();
        if (name.isEmpty())
            continue;
        if (p.value(QLatin1String("available")).toBool())
            items << QStringLiteral("%1 %2").arg(name, p.value(QLatin1String("version")).toString());
        else if (p.value(QLatin1String("status")).toString() == QLatin1String("later"))
            items << qtr("%1 (later)").arg(name);
        else
            items << qtr("%1 (soon)").arg(name);
    }
    return items.join(QLatin1String(", "));
}

QString RedesignUpdater::platformName() const
{
#if defined(_WIN32) && (defined(_M_ARM64) || defined(__aarch64__))
    return QStringLiteral("Windows ARM64");
#elif defined(_WIN32)
    return QStringLiteral("Windows x64");
#elif defined(__APPLE__) && (defined(__aarch64__) || defined(__arm64__))
    return QStringLiteral("macOS Apple Silicon");
#elif defined(__APPLE__)
    return QStringLiteral("macOS Intel");
#else
# if defined(__aarch64__)
    QString name = QStringLiteral("Linux ARM64");
# else
    QString name = QStringLiteral("Linux x86_64");
# endif
    // the package it runs from, when it says so
    if (qEnvironmentVariableIsSet("APPIMAGE"))
        name += QStringLiteral(" · AppImage");
    else if (qEnvironmentVariableIsSet("FLATPAK_ID"))
        name += QStringLiteral(" · Flatpak");
    return name;
#endif
}

QString RedesignUpdater::platformDetails() const
{
    return QStringLiteral("%1 (%2) · %3 · Qt %4")
        .arg(QSysInfo::prettyProductName(), QSysInfo::kernelVersion(),
             QSysInfo::currentCpuArchitecture(), QString::fromLatin1(qVersion()));
}

QString RedesignUpdater::appDir() const
{
    return QCoreApplication::applicationDirPath();
}

bool RedesignUpdater::versionLess(const QString &a, const QString &b)
{
    QList<int> na, nb;
    QString pa, pb;
    splitVersion(a, na, pa);
    splitVersion(b, nb, pb);
    for (int i = 0; i < std::max(na.size(), nb.size()); ++i) {
        const int x = i < na.size() ? na[i] : 0;
        const int y = i < nb.size() ? nb[i] : 0;
        if (x != y)
            return x < y;
    }
    // a pre-release comes before its release
    if (pa.isEmpty() != pb.isEmpty())
        return !pa.isEmpty();
    // part by part, as SemVer does: numbers as numbers (beta.9 before
    // beta.10), before words; fewer parts first when the rest is the same
    const QStringList xa = pa.split('.'), xb = pb.split('.');
    for (qsizetype i = 0; i < std::min(xa.size(), xb.size()); ++i) {
        bool numa, numb;
        const qulonglong ia = xa[i].toULongLong(&numa), ib = xb[i].toULongLong(&numb);
        if (numa && numb) {
            if (ia != ib)
                return ia < ib;
        } else if (numa != numb) {
            return numa;
        } else if (const int c = QString::compare(xa[i], xb[i], Qt::CaseInsensitive)) {
            return c < 0;
        }
    }
    return xa.size() < xb.size();
}

bool RedesignUpdater::isInstalledCopy() const
{
#ifdef _WIN32
    // The installer leaves its uninstaller next to vlc.exe
    return QFileInfo::exists(appDir() + QStringLiteral("/unins000.exe"));
#elif defined(__linux__)
    // An AppImage the user can replace: the file and its folder writable
    // (the new one is written next to it, then renamed over it)
    const QString image = qEnvironmentVariable("APPIMAGE");
    if (image.isEmpty())
        return false;
    const QFileInfo file(image);
    return file.isFile() && file.isWritable() && QFileInfo(file.absolutePath()).isWritable();
#elif defined(__APPLE__)
    // An application the user can replace, not one run from its disk image
    // (or another read-only volume): the bundle and its folder writable, the
    // helper in it
    const QString bundle = ownBundle();
    if (bundle.isEmpty() || bundle.startsWith(QLatin1String("/Volumes/")) || !QFileInfo::exists(updateScript()))
        return false;
    return QFileInfo(bundle).isWritable() && QFileInfo(QFileInfo(bundle).absolutePath()).isWritable();
#else
    return false;
#endif
}

bool RedesignUpdater::appFolderWritable() const
{
#ifdef _WIN32
    // Program Files (an install for everyone) is not: the installer then
    // asks for the rights. Checked once: the properties read it often.
    if (m_writable < 0) {
        m_writable = 0;
        if (QFileInfo::exists(appDir() + QStringLiteral("/vlc-redesign-update.exe"))
            && QDir().mkpath(appDir() + QStringLiteral("/.update"))) {
            QFile probe(appDir() + QStringLiteral("/.update/write-test"));
            if (probe.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                probe.close();
                probe.remove();
                m_writable = 1;
            }
        }
    }
    return m_writable == 1;
#else
    return false;
#endif
}

bool RedesignUpdater::smallUpdate() const
{
    // (one that failed is not tried again: the installer then)
    if (m_deltaFailed || m_steps.isEmpty() || !appFolderWritable())
        return false;
    // a long way that weighs more than the installer: the installer, for a
    // copy that can run it (a portable one has the small updates only)
    return !(m_installerSize > 0 && m_deltaSize > m_installerSize && !m_installerUrl.isEmpty()
             && !m_sumsUrl.isEmpty() && isInstalledCopy());
}

bool RedesignUpdater::canInstall() const
{
    return smallUpdate()
           || (isInstalledCopy() && !m_inPlaceFailed && !m_installerUrl.isEmpty() && !m_sumsUrl.isEmpty());
}

qint64 RedesignUpdater::updateSize() const
{
    if (smallUpdate())
        return m_deltaSize;
    return canInstall() ? m_installerSize : 0;
}

bool RedesignUpdater::showBanner() const
{
    if (m_dismissed || m_aboutOpen)
        return false;
    switch (m_state) {
    case Available:
        return m_manual || m_latestVersion != m_intf->mainSettings->value(keySkipped).toString();
    case Downloading:
    case Preparing:
    case Ready:
        return true;
    case Checking:
    case UpToDate:
    case Failed:
        return m_manual;
    default:
        return false;
    }
}

void RedesignUpdater::setAboutOpen(bool open)
{
    if (open == m_aboutOpen)
        return;
    m_aboutOpen = open;
    // what a check made in About found was read there; a download goes on
    // in the banner
    if (!open && m_manual && m_state != Downloading && m_state != Preparing && m_state != Ready)
        m_dismissed = true;
    emit stateChanged();
}

void RedesignUpdater::setState(State state, const QString &error)
{
    m_state = state;
    m_error = error;
    emit stateChanged();
}

void RedesignUpdater::setProgress(double progress)
{
    if (qFuzzyCompare(progress + 1, m_progress + 1))
        return;
    m_progress = progress;
    emit progressChanged();
}

void RedesignUpdater::check(bool manual)
{
    if (m_state == Checking || m_state == Downloading || m_state == Preparing || m_state == Ready
        || m_verifyingStaged)
        return;
    m_manual = manual;
    m_dismissed = false;
    if (!httpRequestSupported()) {
        setState(Failed, qtr("This build cannot check for updates."));
        return;
    }
    setState(Checking);
    msg_Dbg(m_intf, "update check (%s): asking GitHub for its releases",
            manual ? "asked" : "daily");
    QString url = QString::fromLatin1(releasesApi);
#if !defined(NDEBUG) && defined(VLC_REDESIGN_UPDATE_TEST_SERVER)
    if (!testApi().isEmpty())
        url = testApi();
#endif
    httpRequestAsync(this, githubRequest(url, releasesMaxBytes, 20000),
                     [this](const HttpResponse &response) {
        // (an update staged before became Ready meanwhile: it stays so)
        if (m_state != Checking)
            return;
        if (!response.error.isEmpty() || response.status == 0) {
            setState(Failed, qtr("Couldn't reach GitHub. Check your connection and try again."));
            return;
        }
        // GitHub answers a network that asked too often (60 an hour without
        // an account, shared by every device behind one address) with 403
        // and no requests left, or 429: when it can be asked again, not an
        // "unexpected answer"
        if (response.status == 429 || (response.status == 403
                && response.header("x-ratelimit-remaining").trimmed() == "0")) {
            bool ok = false;
            const qint64 reset = response.header("x-ratelimit-reset").trimmed().toLongLong(&ok);
            msg_Dbg(m_intf, "update check: GitHub's limit of requests reached (%d)", response.status);
            if (ok && reset > QDateTime::currentSecsSinceEpoch())
                setState(Failed, qtr("GitHub was asked too often from this network. Try again after %1.")
                                     .arg(QLocale().toString(QDateTime::fromSecsSinceEpoch(reset).time(),
                                                             QLocale::ShortFormat)));
            else
                setState(Failed, qtr("GitHub was asked too often from this network. Try again later."));
            return;
        }
        onRelease(response.status, response.body);
    });
}

void RedesignUpdater::onRelease(int status, const QByteArray &body)
{
    if (status == 404) {
        // no release published yet
        setState(UpToDate);
        return;
    }
    const QJsonDocument document = QJsonDocument::fromJson(body);
    QJsonObject release;
    QJsonArray releases;
    if (document.isArray()) {
        releases = document.array();
        // the list of releases: the newest published one that carries this
        // build's package (a draft, a pre-release or a release for the other
        // systems only is passed over)
        QString newest;
        for (const QJsonValue value : document.array()) {
            const QJsonObject candidate = value.toObject();
            if (candidate.value(QLatin1String("draft")).toBool()
                || candidate.value(QLatin1String("prerelease")).toBool() || !hasOwnPackage(candidate))
                continue;
            QString version = candidate.value(QLatin1String("tag_name")).toString();
            if (version.startsWith('v'))
                version = version.mid(1);
            if (!version.isEmpty() && (newest.isEmpty() || versionLess(newest, version))) {
                newest = version;
                release = candidate;
            }
        }
        if (status == 200 && release.isEmpty()) {
            // nothing of this system published yet
            msg_Dbg(m_intf, "update check: no release for this system among the newest");
            m_intf->mainSettings->setValue(keyLastCheck, QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
            setState(UpToDate);
            return;
        }
    } else {
        // one release (a local test server, or GitHub's answer of old)
        release = document.object();
        releases.append(release);
    }
    const QString tag = release.value(QLatin1String("tag_name")).toString();
    if (status != 200 || tag.isEmpty()) {
        setState(Failed, qtr("GitHub sent an unexpected answer (%1).").arg(status));
        return;
    }
    m_intf->mainSettings->setValue(keyLastCheck, QDateTime::currentDateTimeUtc().toString(Qt::ISODate));

    m_latestVersion = tag.startsWith('v') ? tag.mid(1) : tag;
    m_releaseTitle = release.value(QLatin1String("name")).toString();
    m_summary = summarise(release.value(QLatin1String("body")).toString());
    m_releaseUrl = release.value(QLatin1String("html_url")).toString();
    m_installerUrl.clear();
    m_installerName.clear();
    m_installerSize = 0;
    m_sumsUrl.clear();
    m_sigUrl.clear();
#ifdef __APPLE__
    bool ownPackage = false;
    // (the zip of the application, taken before the disk image)
    QString archiveUrl, archiveName;
    qint64 archiveSize = 0;
#endif
    for (const QJsonValue asset : release.value(QLatin1String("assets")).toArray()) {
        const QJsonObject a = asset.toObject();
        const QString name = a.value(QLatin1String("name")).toString();
        const QString url = a.value(QLatin1String("browser_download_url")).toString();
        const qint64 size = a.value(QLatin1String("size")).toInteger();
        if (!trustedUrl(url))
            continue;
        if (name.compare(QLatin1String(sumsName), Qt::CaseInsensitive) == 0) {
            m_sumsUrl = url;
        } else if (name.compare(QLatin1String(sumsSignatureName), Qt::CaseInsensitive) == 0) {
            m_sigUrl = url;
        }
#ifdef _WIN32
        else if (name.endsWith(QLatin1String(installerSuffix), Qt::CaseInsensitive)) {
            m_installerUrl = url;
            m_installerName = name;
            m_installerSize = size;
        }
#elif defined(__linux__)
        else if (name.endsWith(QLatin1String(installerSuffix), Qt::CaseInsensitive)) {
            m_installerUrl = url;
            m_installerName = name;
            m_installerSize = size;
        }
#elif defined(__APPLE__)
        else if (name.endsWith(QLatin1String(ownArchiveSuffix), Qt::CaseInsensitive)) {
            ownPackage = true;
            archiveUrl = url;
            archiveName = name;
            archiveSize = size;
        }
        else if (name.endsWith(QLatin1String(ownPackageSuffix), Qt::CaseInsensitive)) {
            ownPackage = true;
            m_installerUrl = url;
            m_installerName = name;
            m_installerSize = size;
        }
#endif
#if !defined(_WIN32) && !defined(__linux__) && !defined(__APPLE__)
        // (the files of Windows and Linux are never offered elsewhere)
        Q_UNUSED(size);
#endif
    }

#ifdef __APPLE__
    // a release made for the other systems only is not an update of this one
    if (!ownPackage) {
        setState(UpToDate);
        return;
    }
    if (!archiveUrl.isEmpty()) {
        m_installerUrl = archiveUrl;
        m_installerName = archiveName;
        m_installerSize = archiveSize;
    }
#endif
    msg_Dbg(m_intf, "update check: the newest release for this system is %s (this copy: %s)",
            qtu(m_latestVersion), qtu(currentVersion()));
    // the small updates that lead to it, if there is a way
    findSteps(releases);
    setState(versionLess(currentVersion(), m_latestVersion) ? Available : UpToDate);
}

// The small updates from this version to the newest (m_latestVersion). Every
// release names, in its files, the versions it has a small update from
// (…-delta-from-<version>.vrdelta): from 0.7.0 on the release before it
// only, up to 0.6.9 every release since the last full one. The lightest way
// through them is taken; none leaves m_steps empty (the installer then).
void RedesignUpdater::findSteps(const QJsonArray &releases)
{
    m_steps.clear();
    m_deltaSize = 0;
#ifdef _WIN32
    const QString current = currentVersion();
    if (!versionLess(current, m_latestVersion))
        return;
    QList<UpdateStep> edges;
    for (const QJsonValue value : releases) {
        const QJsonObject release = value.toObject();
        if (release.value(QLatin1String("draft")).toBool() || release.value(QLatin1String("prerelease")).toBool())
            continue;
        QString version = release.value(QLatin1String("tag_name")).toString();
        if (version.startsWith('v'))
            version = version.mid(1);
        // (the releases after this version, up to the newest of this system)
        if (version.isEmpty() || !versionLess(current, version) || versionLess(m_latestVersion, version))
            continue;
        // (the x64 name has no platform: "-winarm64-delta-from-" never
        // starts with it)
        const QString prefix = QStringLiteral("vlc-redesign-%1%2-delta-from-")
                                   .arg(version, QLatin1String(deltaInfix));
        const QLatin1String suffix(".vrdelta");
        QString sums, signature;
        QList<UpdateStep> found;
        for (const QJsonValue asset : release.value(QLatin1String("assets")).toArray()) {
            const QJsonObject a = asset.toObject();
            const QString name = a.value(QLatin1String("name")).toString();
            const QString url = a.value(QLatin1String("browser_download_url")).toString();
            if (!trustedUrl(url))
                continue;
            if (name.compare(QLatin1String(sumsName), Qt::CaseInsensitive) == 0) {
                sums = url;
            } else if (name.compare(QLatin1String(sumsSignatureName), Qt::CaseInsensitive) == 0) {
                signature = url;
            } else if (name.startsWith(prefix, Qt::CaseInsensitive) && name.endsWith(suffix, Qt::CaseInsensitive)) {
                UpdateStep step;
                step.from = name.mid(prefix.size(), name.size() - prefix.size() - suffix.size());
                step.version = version;
                step.url = url;
                step.name = name;
                step.size = a.value(QLatin1String("size")).toInteger();
                // from this version or a later one, older than its release
                if (!step.from.isEmpty() && step.size > 0 && !versionLess(step.from, current)
                    && versionLess(step.from, version))
                    found.append(step);
            }
        }
        // (nothing is used without the release's checksums)
        if (sums.isEmpty())
            continue;
        for (UpdateStep &step : found) {
            step.sumsUrl = sums;
            step.sigUrl = signature;
            edges.append(step);
        }
    }
    // The versions in their order, each reached by its lightest small
    // update from a version reached before it
    std::stable_sort(edges.begin(), edges.end(), [](const UpdateStep &a, const UpdateStep &b) {
        return versionLess(a.version, b.version);
    });
    struct Way { qint64 size; int edge; };
    QMap<QString, Way> reached;
    reached.insert(current, { 0, -1 });
    for (int i = 0; i < edges.size(); ++i) {
        const auto from = reached.constFind(edges[i].from);
        if (from == reached.constEnd())
            continue;
        const qint64 size = from->size + edges[i].size;
        const auto to = reached.constFind(edges[i].version);
        if (to == reached.constEnd() || size < to->size)
            reached.insert(edges[i].version, { size, i });
    }
    const auto end = reached.constFind(m_latestVersion);
    if (end == reached.constEnd() || end->edge < 0)
        return;
    const qint64 total = end->size;
    QList<UpdateStep> steps;
    for (QString version = m_latestVersion; version != current && steps.size() <= maxUpdateSteps; ) {
        const UpdateStep &step = edges[reached.value(version).edge];
        steps.prepend(step);
        version = step.from;
    }
    if (steps.size() > maxUpdateSteps)
        return;
    m_steps = steps;
    m_deltaSize = total;
    msg_Dbg(m_intf, "update check: %d small update(s) lead to %s (%lld KB)", int(m_steps.size()),
            qtu(m_latestVersion), static_cast<long long>(total / 1024));
#else
    // (small updates are Windows': Linux and macOS replace their package)
    Q_UNUSED(releases);
#endif
}

void RedesignUpdater::openReleasePage()
{
    // Always the page the project names, not a URL from the release data
    QDesktopServices::openUrl(QUrl(QString::fromLatin1(latestReleasePage)));
}

void RedesignUpdater::skipThisVersion()
{
    m_intf->mainSettings->setValue(keySkipped, m_latestVersion);
    m_dismissed = true;
    emit stateChanged();
}

void RedesignUpdater::dismiss()
{
    // (nor between two small updates, while one is written)
    if (m_state == Preparing || m_staging)
        return;
    if (m_state == Downloading) {
        // the download stops (its answer is dropped): the update stays
        // available
        ++m_attempt;
        if (m_cancel)
            m_cancel();
        m_cancel = {};
        setProgress(0);
        m_state = Available;
    }
    m_dismissed = true;
    emit stateChanged();
}

void RedesignUpdater::install()
{
    if (!canInstall()) {
        openReleasePage();
        return;
    }
    ++m_attempt;
    setProgress(0);
    // the user started it: its progress, and a failure, are shown even
    // after an automatic check or a dismissed banner
    m_manual = true;
    m_dismissed = false;
    setState(Downloading);
    // Nothing is used unless it matches the checksum published with it
    if (smallUpdate()) {
        // the small updates in their order, each with the checksums of its
        // own release
        m_removed = std::make_shared<QMap<QString, QString>>();
        const UpdateStep first = m_steps.first();
        withChecksum(first.name, first.version, first.sumsUrl, first.sigUrl,
                     [this](const QString &sha) { downloadStep(0, 0, sha); });
    } else {
        withChecksum(m_installerName, m_latestVersion, m_sumsUrl, m_sigUrl,
                     [this](const QString &sha) { downloadInstaller(sha); });
    }
}

// The SHA-256 of a file of the release of `version`, from that release's
// SHA256SUMS.txt (sumsUrl) once its signature (sigUrl) is checked
void RedesignUpdater::withChecksum(const QString &assetName, const QString &version, const QString &sumsUrl,
                                   const QString &sigUrl, std::function<void(const QString &)> next)
{
    const quint64 attempt = m_attempt;
    const bool signedRelease = signatureRequired(version);
    const QString unsignedError = qtr("This update isn't signed by VLC Redesign, so it wasn't used. "
                                      "Download it from the release page instead.");
    if (signedRelease) {
        if (sigUrl.isEmpty()) {
            msg_Warn(m_intf, "update to %s: the release has no %s", qtu(version), sumsSignatureName);
            setState(Failed, unsignedError);
            return;
        }
        // The signed checksums name the files of one version: never those
        // of another (signed) release published under this one's tag
        if (!assetName.startsWith(QStringLiteral("vlc-redesign-%1-").arg(version), Qt::CaseInsensitive)) {
            msg_Warn(m_intf, "update to %s: %s is not a file of that version", qtu(version), qtu(assetName));
            setState(Failed, unsignedError);
            return;
        }
    }
    m_cancel = httpRequestAsync(this, githubRequest(sumsUrl, 64 * 1024, 30000),
                                [this, attempt, assetName, version, sigUrl, next, signedRelease,
                                 unsignedError](const HttpResponse &response) {
        if (attempt != m_attempt)
            return; // cancelled
        if (!response.error.isEmpty() || response.status != 200) {
            setState(Failed, qtr("The update's checksum is missing; download it from the release page instead."));
            return;
        }
        if (!signedRelease) {
            useChecksums(response.body, assetName, next);
            return;
        }
        // Nothing of the checksums is used before their signature is checked
        const QByteArray sums = response.body;
        m_cancel = httpRequestAsync(this, githubRequest(sigUrl, 4096, 30000),
                                    [this, attempt, assetName, version, next, sums,
                                     unsignedError](const HttpResponse &signature) {
            if (attempt != m_attempt)
                return; // cancelled
            if (!signature.error.isEmpty() || signature.status == 0) {
                setState(Failed, qtr("The download failed. Check your connection and try again."));
                return;
            }
            if (signature.status != 200 || !goodSignature(sums, signature.body)) {
                msg_Warn(m_intf, "update to %s: %s doesn't carry a good signature (HTTP %d, %d bytes)",
                         qtu(version), sumsName, signature.status, int(signature.body.size()));
                setState(Failed, unsignedError);
                return;
            }
            msg_Dbg(m_intf, "update to %s: %s signed by the release key", qtu(version), sumsName);
            useChecksums(sums, assetName, next);
        });
    });
}

void RedesignUpdater::useChecksums(const QByteArray &sums, const QString &assetName,
                                   const std::function<void(const QString &)> &next)
{
    QString expected;
    for (const QString &line : QString::fromUtf8(sums).split('\n')) {
        QStringList parts = line.trimmed().split(QRegularExpression(QStringLiteral("\\s+")));
        // "<hash>  <name>", or "<hash> *<name>" in binary mode
        if (parts.size() == 2 && parts[1].remove('*') == assetName)
            expected = parts[0].toLower();
    }
    if (expected.size() != 64) {
        setState(Failed, qtr("The update's checksum is missing; download it from the release page instead."));
        return;
    }
    next(expected);
}

// The progress of a download, from its worker thread, at most once a percent.
// Posted to the application and the updater checked there: the worker can't
// tell whether it is still alive.
static std::function<void(qint64, qint64)> progressTo(RedesignUpdater *updater, qint64 announced,
                                                      std::function<void(double)> set)
{
    QPointer<RedesignUpdater> target(updater);
    auto last = std::make_shared<int>(-1);
    return [target, announced, set, last](qint64 received, qint64 total) {
        if (total <= 0)
            total = announced;
        if (total <= 0)
            return;
        const int percent = static_cast<int>(std::min<qint64>(100, received * 100 / total));
        if (percent == *last)
            return;
        *last = percent;
        QMetaObject::invokeMethod(qApp, [target, set, percent]() {
            if (target)
                set(percent / 100.0);
        }, Qt::QueuedConnection);
    };
}

// One small update of the way (m_steps[index]; `before`: the bytes of the
// ones before it, for the progress): downloaded, checked against its
// checksum and written next to the files in place, then the next one, or
// the update is ready
void RedesignUpdater::downloadStep(int index, qint64 before, const QString &expectedSha256)
{
    const quint64 attempt = m_attempt;
    const UpdateStep step = m_steps.value(index);
    const qint64 total = std::max<qint64>(m_deltaSize, 1);
    HttpRequest request = githubRequest(step.url, std::max<qint64>(step.size, 1) + 4 * 1024 * 1024,
                                        downloadStepMs);
    request.deadlineMs = downloadDeadlineMs;
    request.progress = progressTo(this, step.size, [this, attempt, before, total, size = step.size](double p) {
        if (attempt == m_attempt)
            setProgress(std::min(1.0, (before + p * size) / total));
    });
    m_cancel = httpRequestAsync(this, std::move(request),
                                [this, attempt, index, before, step, expectedSha256](const HttpResponse &response) {
        if (attempt != m_attempt)
            return; // cancelled
        m_cancel = {};
        if (!response.error.isEmpty() || response.status != 200) {
            setState(Failed, qtr("The download failed. Check your connection and try again."));
            return;
        }
        const bool first = index == 0;
        const bool last = index + 1 >= m_steps.size();
        // (the last one: "Preparing"; one before it is written while the
        // download goes on being shown, and can't be stopped meanwhile)
        if (last)
            setState(Preparing);
        else
            m_staging = true;
        // Checked and written on a worker thread: the interface stays
        // responsive; waited for at the exit, a few seconds at most
        // (runInBackground: a staging cut there leaves no ready.txt). Its
        // answer is dropped if the updater is gone.
        auto error = std::make_shared<QString>();
        const QByteArray delta = response.body;
        const QString dir = appDir();
        const QString version = m_latestVersion;
        const std::shared_ptr<QMap<QString, QString>> removed = m_removed;
        runInBackground(this, [error, delta, expectedSha256, dir, version, step, first, last, removed]() {
            QThread::currentThread()->setPriority(QThread::LowPriority);
            if (sha256Hex(delta) != expectedSha256) {
                *error = qtr("The update doesn't match its checksum, so it wasn't used.");
                return;
            }
            if (first)
                *error = stageBegin(dir);
            if (error->isEmpty())
                *error = stageStep(delta, dir, step.from, step.version, *removed);
            // (its programs' publisher checked there, before ready.txt)
            if (error->isEmpty() && last)
                *error = stageFinish(dir, version, *removed);
        }, [this, error, index, before, step, last]() {
            m_staging = false;
            if (!error->isEmpty()) {
                // the installer next time ("Update now" again), not the same
                // small updates
                m_deltaFailed = true;
                setState(Failed, *error);
                return;
            }
            if (last) {
                setState(Ready);
                return;
            }
            const UpdateStep next = m_steps.value(index + 1);
            const qint64 done = before + step.size;
            withChecksum(next.name, next.version, next.sumsUrl, next.sigUrl,
                         [this, index, done](const QString &sha) { downloadStep(index + 1, done, sha); });
        });
    });
}

QString RedesignUpdater::stageBegin(const QString &dir)
{
    const QString work = dir + QStringLiteral("/.update");
    const QString staged = work + QStringLiteral("/staged");
    QFile::remove(work + QStringLiteral("/ready.txt"));
    removeTree(staged);
    if (!QDir().mkpath(staged))
        return qtr("The update can't be written in %1.").arg(QDir::toNativeSeparators(dir));
    return {};
}

QString RedesignUpdater::stageStep(const QByteArray &delta, const QString &dir, const QString &from,
                                   const QString &to, QMap<QString, QString> &removed)
{
    if (delta.size() < 12 || !delta.startsWith("VRDELTA1"))
        return qtr("The update isn't in a format this version knows.");
    const quint32 headerSize = qFromLittleEndian<quint32>(delta.constData() + 8);
    if (12 + qint64(headerSize) > delta.size())
        return qtr("The update is damaged.");
    const QJsonObject header = QJsonDocument::fromJson(delta.mid(12, headerSize)).object();
    if (header.value(QLatin1String("format")).toInt() != 1
        || header.value(QLatin1String("to")).toString() != to
        || header.value(QLatin1String("from")).toString() != from)
        return qtr("The update isn't for this version.");
    const char *blobs = delta.constData() + 12 + headerSize;
    const qsizetype available = delta.size() - 12 - headerSize;

    const QString staged = dir + QStringLiteral("/.update/staged");
    if (!QFileInfo(staged).isDir())
        return qtr("The update can't be written in %1.").arg(QDir::toNativeSeparators(dir));

    const QString changed = qtr("A file of VLC Redesign was changed since it was installed, so the small "
                                "update can't be used: update with the installer instead.");
    for (const QJsonValue value : header.value(QLatin1String("files")).toArray()) {
        const QJsonObject file = value.toObject();
        const QString path = file.value(QLatin1String("path")).toString();
        const qint64 size = file.value(QLatin1String("size")).toInteger(-1);
        const qint64 offset = file.value(QLatin1String("offset")).toInteger(-1);
        if (!safeRelativePath(path) || size < 0)
            return qtr("The update is damaged.");
        const QString target = staged + QLatin1Char('/') + path;
        QByteArray data;
        if (file.value(QLatin1String("kind")).toString() == QLatin1String("patch")) {
            // (the file of the version this step starts from: the one a
            // step before it wrote, else the one in place)
            QFile oldFile(QFileInfo::exists(target) ? target : dir + QLatin1Char('/') + path);
            if (!oldFile.open(QIODevice::ReadOnly))
                return changed;
            const QByteArray old = oldFile.readAll();
            oldFile.close();
            if (sha256Hex(old) != file.value(QLatin1String("old_sha256")).toString())
                return changed;
            const QJsonArray parts = file.value(QLatin1String("parts")).toArray();
            if (parts.size() != 3)
                return qtr("The update is damaged.");
            const qint64 c = parts[0].toInteger(), d = parts[1].toInteger(), e = parts[2].toInteger();
            const QByteArray control = block(blobs, available, offset, c);
            const QByteArray diff = block(blobs, available, offset + c, d);
            const QByteArray extra = block(blobs, available, offset + c + d, e);
            if (!bspatch(old, control, diff, extra, size, data))
                return qtr("The update is damaged.");
        } else {
            data = block(blobs, available, offset, file.value(QLatin1String("length")).toInteger());
        }
        if (data.size() != size || sha256Hex(data) != file.value(QLatin1String("sha256")).toString())
            return qtr("The update is damaged: a file doesn't match its checksum.");
        QDir().mkpath(QFileInfo(target).absolutePath());
        // (on the disk before ready.txt names the update complete)
        if (!writeDurably(target, data))
            return qtr("The update can't be written in %1.").arg(QDir::toNativeSeparators(dir));
        // (a file a step before this one took away is back)
        removed.remove(path.toLower());
    }

    for (const QJsonValue value : header.value(QLatin1String("removed")).toArray()) {
        const QString path = value.toString();
        if (!safeRelativePath(path))
            continue;
        removed.insert(path.toLower(), path);
        // (a file a step before this one wrote is not of the newest version)
        QFile::remove(staged + QLatin1Char('/') + path);
    }
    return {};
}

QString RedesignUpdater::stageFinish(const QString &dir, const QString &version,
                                     const QMap<QString, QString> &removed)
{
    const QString work = dir + QStringLiteral("/.update");
    const QString staged = work + QStringLiteral("/staged");
    // (lines of '\n': the helper reads it as text, CRLF or not)
    if (!writeDurably(work + QStringLiteral("/removed.txt"), QStringList(removed.values()).join('\n').toUtf8()))
        return qtr("The update can't be written in %1.").arg(QDir::toNativeSeparators(dir));
    // Its programs from the publisher of this copy, before the update is
    // complete: VLC closed (or stopped) while they are checked leaves no
    // ready.txt, so no update put in place unchecked
    const QString publisher = stagedPublisherMismatch(staged);
    if (!publisher.isEmpty()) {
        removeTree(staged);
        return publisher;
    }
    if (!prepareHelper(dir))
        return qtr("The update can't be written in %1.").arg(QDir::toNativeSeparators(dir));
    // last: the update is complete. Every file above is on the disk by now
    // (each synced before its rename), so a power cut leaves either no
    // ready.txt or a whole update
    if (version.isEmpty() || !writeDurably(work + QStringLiteral("/ready.txt"), version.toUtf8()))
        return qtr("The update can't be written in %1.").arg(QDir::toNativeSeparators(dir));
    return {};
}

// An update staged by an earlier session (closed before it could be put in
// place, or a file was held): it is applied when VLC closes this time
void RedesignUpdater::findStagedUpdate()
{
    // A stage is moved aside at once (a rename), then removed on a worker
    // with any other one moved aside before: an update prepared in this
    // session meanwhile writes a new "staged" of its own
    const QString work = appDir() + QStringLiteral("/.update");
    const auto removeStaleStage = [this, work](const QString &stage) {
        // (a file of the stage held open, an antivirus reading it: the
        // rename fails. The stage is then removed where it is, as before,
        // on the worker too: what is held stays for the next start)
        QString held;
        if (!stage.isEmpty()
            && !QDir().rename(stage, stage + QStringLiteral(".old-")
                                     + QString::number(QDateTime::currentMSecsSinceEpoch())))
            held = stage;
        runInBackground(this, [work, held]() {
            QThread::currentThread()->setPriority(QThread::LowPriority);
            // A link or a junction of that name is not a stage of ours: it
            // goes as the link, never what it leads to, and so does a link
            // inside a stage (removeTree)
            const auto remove = [](const QFileInfo &entry) {
                if (isLink(entry) || entry.isDir())
                    removeTree(entry.filePath());
            };
            const QFileInfoList old = QDir(work).entryInfoList({ QStringLiteral("staged.old-*") },
                                                               QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden);
            for (const QFileInfo &entry : old)
                remove(entry);
            if (!held.isEmpty())
                remove(QFileInfo(held));
        });
    };
    QFile ready(appDir() + QStringLiteral("/.update/ready.txt"));
    if (!ready.open(QIODevice::ReadOnly)) {
        // the helper of the last update, done
        QFile::remove(appDir() + QStringLiteral("/.update/vlc-redesign-update.exe"));
        // an update left half written (VLC closed while it was prepared);
        // not one another VLC is writing now
        const QFileInfo staged(appDir() + QStringLiteral("/.update/staged"));
        // (and the ones a session quit before removing)
        const bool stale = staged.isDir() && staged.lastModified().secsTo(QDateTime::currentDateTime()) > 3600;
        removeStaleStage(stale ? staged.filePath() : QString());
        return;
    }
    const QString version = QString::fromUtf8(ready.readAll()).trimmed();
    ready.close();
    if (!version.isEmpty() && versionLess(currentVersion(), version)) {
#ifdef _WIN32
        // Its programs checked again before it may be put in place (they
        // were when it was staged; the folder may have changed since), on
        // a worker thread: it is Ready once they are
        const QString dir = appDir();
        auto error = std::make_shared<QString>();
        m_verifyingStaged = true;
        runInBackground(this, [error, dir]() {
            QThread::currentThread()->setPriority(QThread::LowPriority);
            *error = stagedPublisherMismatch(dir + QStringLiteral("/.update/staged"));
        }, [this, error, dir, version]() {
            m_verifyingStaged = false;
            if (!error->isEmpty()) {
                msg_Warn(m_intf, "update to %s: a program staged isn't of this copy's publisher: not used",
                         qtu(version));
                QFile::remove(dir + QStringLiteral("/.update/ready.txt"));
                removeTree(dir + QStringLiteral("/.update/staged"));
                return;
            }
            // (a download started meanwhile goes on: the installer then)
            if (m_state == Downloading || m_state == Preparing)
                return;
            m_latestVersion = version;
            setState(Ready);
        });
#else
        // (small updates are Windows': nothing of another publisher to check)
        m_latestVersion = version;
        m_state = Ready;
#endif
    } else {
        // applied already, or older than this version
        ready.remove();
        removeStaleStage(appDir() + QStringLiteral("/.update/staged"));
    }
}

// VLC opened again after an update, on what was playing: it goes on where
// it was (not with :start-time, which makes the media a clip starting there)
void RedesignUpdater::resumeAfterUpdate()
{
    const int64_t seconds = var_InheritInteger(m_intf, "redesign-resume-time");
    PlayerController *player = m_intf->p_mainPlayerController;
    if (seconds <= 0 || !player)
        return;
    auto done = std::make_shared<bool>(false);
    auto seek = [player, seconds, done]() {
        if (*done || !player->isSeekable())
            return;
        *done = true;
        player->setTime(VLCTime(VLC_TICK_FROM_SEC(seconds)));
    };
    // the media may have started before the interface
    seek();
    const QMetaObject::Connection seekable = connect(player, &PlayerController::seekableChanged, this, seek);
    // no "Continue where you left off?" there: it is where it was
    auto asked = std::make_shared<QMetaObject::Connection>();
    if (player->canRestorePlayback())
        player->acknowledgeRestoreCallback();
    else
        *asked = connect(player, &PlayerController::playbackRestoreQueried, this, [player, asked]() {
            if (!player->canRestorePlayback())
                return;
            player->acknowledgeRestoreCallback();
            disconnect(*asked);
        });
    QTimer::singleShot(30000, this, [seekable, asked]() {
        disconnect(seekable);
        disconnect(*asked);
    });
}

bool RedesignUpdater::launchHelper(bool relaunch)
{
    if (m_helperStarted || m_state != Ready)
        return m_helperStarted;
    // The copy made with the update (prepareHelper)
    const QString dir = appDir();
    const QString helper = dir + QStringLiteral("/.update/vlc-redesign-update.exe");
    if (!QFileInfo::exists(helper) && !QFile::copy(dir + QStringLiteral("/vlc-redesign-update.exe"), helper))
        return false;
    QStringList args = { QStringLiteral("--pid"), QString::number(QCoreApplication::applicationPid()),
                         QStringLiteral("--app"), QDir::toNativeSeparators(dir),
                         QStringLiteral("--version"), m_latestVersion };
    if (relaunch) {
        args << QStringLiteral("--relaunch");
        // Reopen on what was playing, where it was
        PlayerController *player = m_intf->p_mainPlayerController;
        const QUrl media = player ? player->getUrl() : QUrl();
        if (media.isValid() && !media.isEmpty()) {
            args << QStringLiteral("--resume") << media.toString(QUrl::FullyEncoded);
            const int seconds = player->getTime().toMilliseconds() / 1000;
            if (seconds > 0)
                args << QStringLiteral("--time") << QString::number(seconds);
        }
    }
    m_helperStarted = QProcess::startDetached(helper, args, dir);
    if (m_helperStarted)
        msg_Dbg(m_intf, "update to %s: put in place once VLC has quit", qtu(m_latestVersion));
    else
        msg_Warn(m_intf, "update to %s: the helper couldn't be started", qtu(m_latestVersion));
    return m_helperStarted;
}

void RedesignUpdater::restartNow()
{
    if (m_state != Ready)
        return;
    if (!launchHelper(true)) {
        setState(Failed, qtr("The update couldn't be started. It will be tried again next time."));
        return;
    }
    QTimer::singleShot(200, this, []() { THEDP->quit(); });
}

void RedesignUpdater::applyOnExit()
{
    if (m_state == Ready)
        launchHelper(false);
}

void RedesignUpdater::downloadInstaller(const QString &expectedSha256)
{
    const quint64 attempt = m_attempt;
    HttpRequest request = githubRequest(m_installerUrl, 512 * 1024 * 1024, downloadStepMs);
    request.deadlineMs = downloadDeadlineMs;
    request.progress = progressTo(this, m_installerSize, [this, attempt](double p) {
        if (attempt == m_attempt)
            setProgress(p);
    });
    m_cancel = httpRequestAsync(this, std::move(request),
                                [this, attempt, expectedSha256](const HttpResponse &response) {
        if (attempt != m_attempt)
            return; // cancelled
        m_cancel = {};
        if (!response.error.isEmpty() || response.status != 200) {
            setState(Failed, qtr("The download failed. Try again, or download it from the release page."));
            return;
        }
        setState(Preparing);
        // Checked and saved on a worker thread: the interface stays
        // responsive; waited for at the exit, a few seconds at most
        // (runInBackground). Its answer is dropped if the updater is gone.
        // (log: what the macOS helper said, for the debug log; inPlace: it
        // could not prepare the update on this Mac, the download is offered)
        struct Saved { QString error, path, log; bool inPlaceFailed = false; };
        auto saved = std::make_shared<Saved>();
        const QByteArray body = response.body;
        const QString name = m_installerName;
        runInBackground(this, [saved, body, expectedSha256, name]() {
            QThread::currentThread()->setPriority(QThread::LowPriority);
            if (sha256Hex(body) != expectedSha256) {
                saved->error = qtr("The downloaded installer doesn't match its checksum, so it wasn't run.");
            } else {
#ifdef __linux__
                // The new AppImage next to the one running, then renamed over
                // it: the running copy keeps its file open, the path (menus,
                // shortcuts) opens the new one. Synced before its rename
                // (writeDurably): a power cut leaves the old AppImage or the
                // whole new one, executable, never one cut short
                const QString image = qEnvironmentVariable("APPIMAGE");
                const QString dir = QFileInfo(image).absolutePath();
                if (!writeDurably(image, body,
                                  QFile(image).permissions() | QFileDevice::ExeOwner | QFileDevice::ReadOwner)) {
                    saved->error = qtr("The installer couldn't be saved to %1.").arg(QDir::toNativeSeparators(dir));
                    return;
                }
                saved->path = image;
                Q_UNUSED(name);
#else
                // (removed by a start of VLC after it ran: removeOldInstallers)
                const QString dir = installerFolder();
                if (!dir.isEmpty())
                    QDir().mkpath(dir);
                // (the name of the release's file, never a path)
                saved->path = dir + QLatin1Char('/') + QFileInfo(name).fileName();
                // (whole, or not there: never an installer cut short)
                if (dir.isEmpty() || !writeDurably(saved->path, body)) {
                    saved->error = qtr("The installer couldn't be saved to %1.").arg(QDir::toNativeSeparators(dir));
                } else {
                    saved->error = publisherMismatch(saved->path);
                    if (!saved->error.isEmpty())
                        QFile::remove(saved->path);
                }
#ifdef __APPLE__
                // The application of the zip (or of the image) copied next to
                // this one and checked (vlc-redesign-update.sh stage):
                // saved->path is that copy
                if (saved->error.isEmpty()) {
                    const QString image = saved->path;
                    const QString bundle = ownBundle();
                    // Nothing is typed to the helper (a pipe left open is an
                    // answer some tool could wait for); each of its steps
                    // ends by itself after two minutes (#117: one blocked on
                    // a user's Mac kept "Preparing the update" for the ten
                    // minutes waited here, and the end read "damaged")
                    QElapsedTimer taken;
                    taken.start();
                    // (runHelper, not QProcess: #117)
                    const HelperRun stage = runHelper({ QStringLiteral("/bin/sh"), updateScript(),
                                                        QStringLiteral("stage"), image, bundle },
                                                      6 * 60 * 1000);
                    QFile::remove(image);
                    const QString staged = QString::fromUtf8(stage.out).trimmed();
                    const int code = stage.code;
                    // what the helper said (its steps' own words), for the log
                    saved->log = QStringLiteral("the helper ended with %1 after %2 s: %3")
                                     .arg(code).arg(taken.elapsed() / 1000)
                                     .arg(QString::fromUtf8(stage.err).trimmed().right(900)
                                              .replace(QLatin1Char('\n'), QLatin1String(" | ")));
                    if (code != 0 || staged.isEmpty()) {
                        switch (code) {
                        case 3:
                            saved->error = qtr("The update can't be written in %1.")
                                               .arg(QDir::toNativeSeparators(QFileInfo(bundle).absolutePath()));
                            break;
                        case 4:
                            // (its code signature isn't intact)
                            saved->error = qtr("The update is damaged.");
                            break;
                        case 5:
                            saved->error = qtr("The update isn't for this version.");
                            break;
                        default:
                            // The image was right (its checksum, above) and
                            // this Mac did not let it be opened, read or
                            // copied in time: nothing of the update is
                            // damaged. The way that always works is offered
                            saved->error = qtr("The update couldn't be prepared on this Mac. Download it from the release page.");
                            saved->inPlaceFailed = true;
                            break;
                        }
                    } else {
                        saved->path = staged;
                    }
                }
#endif
#endif
            }
        }, [this, saved]() {
            if (!saved->log.isEmpty()) {
                // (a failure as an error: the log a report comes with keeps
                // errors only, unless its details were raised; a success for
                // the one who looks)
                if (saved->error.isEmpty())
                    msg_Dbg(m_intf, "update to %s: %s", qtu(m_latestVersion), qtu(saved->log));
                else
                    msg_Err(m_intf, "update to %s: %s", qtu(m_latestVersion), qtu(saved->log));
            }
            // (the button then offers the download: canInstall)
            m_inPlaceFailed = m_inPlaceFailed || saved->inPlaceFailed;
            if (!saved->error.isEmpty())
                setState(Failed, saved->error);
            else
                launchInstaller(saved->path);
        });
    });
}

void RedesignUpdater::launchInstaller(const QString &path)
{
#ifdef __linux__
    // The new AppImage is in place: opened again once this VLC has quit (not
    // before: a second VLC would hand the media to this one), where it was
    QStringList args = { QStringLiteral("-c"),
                         QStringLiteral("while kill -0 %1 2>/dev/null; do sleep 0.2; done; exec \"$0\" \"$@\"")
                             .arg(QCoreApplication::applicationPid()),
                         path };
    PlayerController *player = m_intf->p_mainPlayerController;
    const QUrl media = player ? player->getUrl() : QUrl();
    if (media.isValid() && !media.isEmpty()) {
        const int seconds = player->getTime().toMilliseconds() / 1000;
        if (seconds > 0)
            args << QStringLiteral("--redesign-resume-time=%1").arg(seconds);
        args << media.toString(QUrl::FullyEncoded);
    }
    if (!QProcess::startDetached(QStringLiteral("/bin/sh"), args)) {
        // in place already: the next start is the new version
        setState(Failed, qtr("The installer couldn't be started."));
        return;
    }
    msg_Dbg(m_intf, "update to %s: the AppImage is replaced, VLC opens again", qtu(m_latestVersion));
    QTimer::singleShot(300, this, []() { THEDP->quit(); });
#elif defined(__APPLE__)
    // The new application waits next to this one: swapped in once this VLC
    // has quit, then opened where it was (vlc-redesign-update.sh apply)
    QStringList args = { updateScript(), QStringLiteral("apply"),
                         QString::number(QCoreApplication::applicationPid()), path, ownBundle() };
    PlayerController *player = m_intf->p_mainPlayerController;
    const QUrl media = player ? player->getUrl() : QUrl();
    if (media.isValid() && !media.isEmpty()) {
        const int seconds = player->getTime().toMilliseconds() / 1000;
        if (seconds > 0)
            args << QStringLiteral("--redesign-resume-time=%1").arg(seconds);
        args << media.toString(QUrl::FullyEncoded);
    }
    if (!QProcess::startDetached(QStringLiteral("/bin/sh"), args)) {
        removeTree(path);
        setState(Failed, qtr("The update couldn't be started. It will be tried again next time."));
        return;
    }
    msg_Dbg(m_intf, "update to %s: the application is replaced once VLC quits, then opens again", qtu(m_latestVersion));
    QTimer::singleShot(300, this, []() { THEDP->quit(); });
#else
    QStringList args = { QStringLiteral("/SILENT"), QStringLiteral("/SUPPRESSMSGBOXES"),
                         QStringLiteral("/NORESTART"), QStringLiteral("/UPDATE") };
    // Reopen on what was playing, where it was
    PlayerController *player = m_intf->p_mainPlayerController;
    const QUrl media = player ? player->getUrl() : QUrl();
    if (media.isValid() && !media.isEmpty()) {
        args << QStringLiteral("/RESUME=") + media.toString(QUrl::FullyEncoded);
        const int seconds = player->getTime().toMilliseconds() / 1000;
        if (seconds > 0)
            args << QStringLiteral("/TIME=%1").arg(seconds);
    }
    if (!QProcess::startDetached(path, args)) {
        setState(Failed, qtr("The installer couldn't be started."));
        return;
    }
    // The installer waits for VLC to close, then reopens it
    QTimer::singleShot(300, this, []() { THEDP->quit(); });
#endif
}
