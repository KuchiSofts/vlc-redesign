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

#include "redesign_feedback.hpp"

#include <QClipboard>
#include <QCoreApplication>
#include <QFileInfo>
#include <QGuiApplication>
#include <QList>
#include <QPoint>
#include <QScreen>
#include <QStringList>
#include <QSysInfo>
#include <QUrl>

#include <vlc_common.h>
#include <vlc_variables.h>

#ifdef _WIN32
# include <windows.h>
# include <dxgi1_6.h>
#endif

#include "dialogs/preferences/interface_language.hpp"
#include "util/color_scheme_model.hpp"
#include "util/redesign_version.hpp"

namespace {

// The forms of the repository. Ideas go to the suggestion form until the
// Discussions of the repository are open: then "discussions/new?category=ideas"
// here (its form keeps a "version" field).
const char reportForm[] = VLC_REDESIGN_REPO_URL "/issues/new?template=bug_report.yml";
const char ideaForm[] = VLC_REDESIGN_REPO_URL "/issues/new?template=feature_request.yml";
// GitHub refuses longer links
constexpr int maxUrlLength = 8000;

QString installType()
{
#ifdef _WIN32
    // The installer leaves its uninstaller next to vlc.exe (as the updater
    // tells them apart); the values are the options of the bug form
    if (QFileInfo::exists(QCoreApplication::applicationDirPath() + QStringLiteral("/unins000.exe")))
        return QStringLiteral("Installer (setup.exe)");
    return QStringLiteral("Portable zip");
#else
    return QStringLiteral("Built from source");
#endif
}

#ifdef _WIN32
QString registryString(const wchar_t *name)
{
    wchar_t value[256];
    DWORD size = sizeof(value);
    if (RegGetValueW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", name,
                     RRF_RT_REG_SZ, nullptr, value, &size) != ERROR_SUCCESS)
        return {};
    return QString::fromWCharArray(value).trimmed();
}

DWORD registryNumber(const wchar_t *name)
{
    DWORD value = 0;
    DWORD size = sizeof(value);
    if (RegGetValueW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", name,
                     RRF_RT_REG_DWORD, nullptr, &value, &size) != ERROR_SUCCESS)
        return 0;
    return value;
}
#endif

// "Windows 11 Pro 24H2 (build 26100.6899)"
QString windowsVersion()
{
#ifdef _WIN32
    QString product = registryString(L"ProductName");
    const QString build = registryString(L"CurrentBuild");
    // Windows 11 still names itself Windows 10 there
    if (build.toInt() >= 22000)
        product.replace(QLatin1String("Windows 10"), QLatin1String("Windows 11"));
    QString release = registryString(L"DisplayVersion");
    if (release.isEmpty())
        release = registryString(L"ReleaseId");
    if (product.isEmpty())
        product = QSysInfo::prettyProductName();
    QString text = product;
    if (!release.isEmpty())
        text += QLatin1Char(' ') + release;
    if (!build.isEmpty()) {
        const DWORD ubr = registryNumber(L"UBR");
        text += QStringLiteral(" (build %1%2)").arg(build, ubr ? QStringLiteral(".%1").arg(ubr) : QString());
    }
    return text;
#else
    return QSysInfo::prettyProductName() + QStringLiteral(" (") + QSysInfo::kernelVersion() + QLatin1Char(')');
#endif
}

// The revision of VLC this build was made from (libvlc_get_changeset(): the
// interface does not link libvlc, it is found in the process)
QString revision()
{
#ifdef _WIN32
    HMODULE libvlc = GetModuleHandleW(L"libvlc.dll");
    if (!libvlc)
        return {};
    using Changeset = const char *(*)(void);
    const auto changeset = reinterpret_cast<Changeset>(reinterpret_cast<void *>(GetProcAddress(libvlc, "libvlc_get_changeset")));
    if (!changeset)
        return {};
    const QString text = QString::fromUtf8(changeset()).trimmed();
    return text.compare(QLatin1String("unknown"), Qt::CaseInsensitive) == 0 ? QString() : text;
#else
    return {};
#endif
}

struct Output
{
    QPoint topLeft;
    bool hdr;
};

// The graphics adapters ("NVIDIA GeForce RTX 3080, driver 32.0.15.6094
// (560.94)") and whether each screen shows HDR, from DXGI (loaded here: the
// interface does not link it)
void graphics(QStringList &adapters, QList<Output> &outputs)
{
#ifdef _WIN32
    HMODULE dxgi = LoadLibraryExW(L"dxgi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!dxgi)
        return;
    using CreateFactory = HRESULT (WINAPI *)(REFIID, void **);
    const auto create = reinterpret_cast<CreateFactory>(reinterpret_cast<void *>(GetProcAddress(dxgi, "CreateDXGIFactory1")));
    IDXGIFactory1 *factory = nullptr;
    if (!create || FAILED(create(__uuidof(IDXGIFactory1), reinterpret_cast<void **>(&factory))) || !factory) {
        FreeLibrary(dxgi);
        return;
    }
    QStringList software;
    IDXGIAdapter1 *adapter = nullptr;
    for (UINT i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        if (!adapter)
            continue;
        DXGI_ADAPTER_DESC1 desc = {};
        if (SUCCEEDED(adapter->GetDesc1(&desc))) {
            QString text = QString::fromWCharArray(desc.Description).trimmed();
            LARGE_INTEGER driver = {};
            if (SUCCEEDED(adapter->CheckInterfaceSupport(__uuidof(IDXGIDevice), &driver))) {
                const unsigned parts[] = { HIWORD(driver.HighPart), LOWORD(driver.HighPart),
                                           HIWORD(driver.LowPart), LOWORD(driver.LowPart) };
                text += QStringLiteral(", driver %1.%2.%3.%4").arg(parts[0]).arg(parts[1]).arg(parts[2]).arg(parts[3]);
                // NVIDIA's own number: the last five digits (32.0.15.6094: 560.94)
                if (desc.VendorId == 0x10DE) {
                    const unsigned nvidia = (parts[2] % 10) * 10000 + parts[3];
                    text += QStringLiteral(" (%1.%2)").arg(nvidia / 100).arg(nvidia % 100, 2, 10, QLatin1Char('0'));
                }
            }
            // WARP only when there is nothing else
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                software.append(text);
            else if (!adapters.contains(text))
                adapters.append(text);
        }
        IDXGIOutput *output = nullptr;
        for (UINT j = 0; adapter->EnumOutputs(j, &output) != DXGI_ERROR_NOT_FOUND; ++j) {
            if (!output)
                continue;
            DXGI_OUTPUT_DESC outputDesc = {};
            IDXGIOutput6 *output6 = nullptr;
            if (SUCCEEDED(output->GetDesc(&outputDesc))
                && SUCCEEDED(output->QueryInterface(__uuidof(IDXGIOutput6), reinterpret_cast<void **>(&output6)))) {
                DXGI_OUTPUT_DESC1 desc1 = {};
                if (SUCCEEDED(output6->GetDesc1(&desc1)))
                    outputs.append({ QPoint(outputDesc.DesktopCoordinates.left, outputDesc.DesktopCoordinates.top),
                                     desc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020 });
                output6->Release();
            }
            output->Release();
            output = nullptr;
        }
        adapter->Release();
        adapter = nullptr;
    }
    factory->Release();
    FreeLibrary(dxgi);
    if (adapters.isEmpty())
        adapters = software;
#else
    Q_UNUSED(adapters);
    Q_UNUSED(outputs);
#endif
}

QString optionString(qt_intf_t *intf, const char *name, const char *fallback)
{
    char *value = var_InheritString(intf, name);
    QString text = value ? QString::fromUtf8(value).trimmed() : QString();
    free(value);
    return text.isEmpty() ? QString::fromLatin1(fallback) : text;
}

} // namespace

RedesignFeedback::RedesignFeedback(qt_intf_t *intf, QObject *parent)
    : QObject(parent)
    , m_intf(intf)
{
}

QString RedesignFeedback::reportText(int colorScheme, bool darkTheme, const QString &accent) const
{
    QStringList lines;
    const QString changeset = revision();
    lines << QStringLiteral("VLC Redesign: %1").arg(QStringLiteral(VLC_REDESIGN_VERSION))
          << QStringLiteral("Based on: VLC %1%2").arg(QStringLiteral(VERSION),
                                                       changeset.isEmpty() ? QString() : QStringLiteral(" (%1)").arg(changeset))
          << QStringLiteral("Install: %1").arg(installType())
          << QStringLiteral("Qt: %1").arg(QString::fromLatin1(qVersion()))
          << QStringLiteral("System: %1").arg(windowsVersion())
          << QStringLiteral("CPU architecture: %1 (app %2)").arg(QSysInfo::currentCpuArchitecture(),
                                                                 QSysInfo::buildCpuArchitecture());

    QStringList adapters;
    QList<Output> outputs;
    graphics(adapters, outputs);
    if (adapters.isEmpty())
        lines << QStringLiteral("GPU: unknown");
    for (const QString &adapter : adapters)
        lines << QStringLiteral("GPU: %1").arg(adapter);

    lines << QStringLiteral("Video output: %1").arg(optionString(m_intf, "vout", "any"))
          << QStringLiteral("Hardware decoding: %1 (decoder device: %2)")
                 .arg(var_InheritBool(m_intf, "hw-dec") ? QStringLiteral("on") : QStringLiteral("off"),
                      optionString(m_intf, "dec-dev", "any"));

    const QList<QScreen *> screens = QGuiApplication::screens();
    lines << QStringLiteral("Screens: %1").arg(screens.size());
    for (qsizetype i = 0; i < screens.size(); ++i) {
        const QScreen *screen = screens[i];
        const qreal ratio = screen->devicePixelRatio();
        QString text = QStringLiteral("Screen %1: %2x%3, scale %4%, %5 Hz")
                           .arg(i + 1)
                           .arg(qRound(screen->size().width() * ratio))
                           .arg(qRound(screen->size().height() * ratio))
                           .arg(qRound(ratio * 100))
                           .arg(qRound(screen->refreshRate()));
        // (the position of a screen stays in pixels with the scaling of Qt,
        // as DXGI gives it)
        for (const Output &output : outputs) {
            if (output.topLeft == screen->geometry().topLeft()) {
                text += output.hdr ? QStringLiteral(", HDR on") : QStringLiteral(", HDR off");
                break;
            }
        }
        if (screen == QGuiApplication::primaryScreen())
            text += QStringLiteral(", main");
        lines << text;
    }

    QString theme;
    switch (colorScheme) {
    case ColorSchemeModel::Day: theme = QStringLiteral("Light"); break;
    case ColorSchemeModel::Night: theme = QStringLiteral("Dark"); break;
    default: theme = darkTheme ? QStringLiteral("System (dark)") : QStringLiteral("System (light)"); break;
    }
    const QString accentName = accent.trimmed().toLower();
    lines << QStringLiteral("Theme: %1").arg(theme)
          << QStringLiteral("Accent: %1%2").arg(accentName, accentName == QLatin1String("#d37918")
                                                                ? QStringLiteral(" (default)") : QString())
          << QStringLiteral("Language: %1%2").arg(interfaceLanguage(),
                                                  QGuiApplication::layoutDirection() == Qt::RightToLeft
                                                      ? QStringLiteral(", right to left") : QString());
    return lines.join(QLatin1Char('\n'));
}

QString RedesignFeedback::reportUrl(const QString &report) const
{
    const auto parameter = [](const char *name, const QString &value) {
        return QByteArray(name) + '=' + QUrl::toPercentEncoding(value);
    };
    const QByteArray start = QByteArray(reportForm) + '&' + parameter("version", QStringLiteral(VLC_REDESIGN_VERSION))
                             + '&' + parameter("install", installType())
                             + '&' + parameter("windows", windowsVersion());

    // the end of the information goes first when the link is too long
    QStringList lines = report.split(QLatin1Char('\n'));
    QByteArray url = start + '&' + parameter("environment", lines.join(QLatin1Char('\n')));
    while (url.size() > maxUrlLength && lines.size() > 1) {
        lines.removeLast();
        url = start + '&' + parameter("environment", lines.join(QLatin1Char('\n')) + QStringLiteral("\n(cut)"));
    }
    if (url.size() > maxUrlLength)
        url = start;
    return QString::fromLatin1(url);
}

QString RedesignFeedback::ideaUrl() const
{
    return QString::fromLatin1(QByteArray(ideaForm) + "&version="
                               + QUrl::toPercentEncoding(QStringLiteral(VLC_REDESIGN_VERSION)));
}

void RedesignFeedback::copyText(const QString &text) const
{
    if (QClipboard *clipboard = QGuiApplication::clipboard())
        clipboard->setText(text);
}
