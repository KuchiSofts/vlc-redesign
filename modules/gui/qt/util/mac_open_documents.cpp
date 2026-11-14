/*****************************************************************************
 * mac_open_documents.cpp: files macOS asks the app to open (Apple events)
 *****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/
#include "mac_open_documents.hpp"

#include <QtGlobal>

#ifdef Q_OS_MACOS

/* the Apple Event Manager's C interface (the Qt module is C++ only); without
 * AssertMacros' short names (check, verify, require), which Qt uses */
#define __ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES 0
#include <CoreServices/CoreServices.h>
#include <CoreFoundation/CoreFoundation.h>

#include <QByteArray>
#include <QDebug>
#include <QList>

#include <utility>

namespace
{

OSErr openDocuments(const AppleEvent *event, AppleEvent *, SRefCon);

AEEventHandlerUPP handlerUpp()
{
    static const AEEventHandlerUPP handler = NewAEEventHandlerUPP(openDocuments);
    return handler;
}

/* whose handler takes the open-documents event now (for the log) */
const char *currentHandler()
{
    AEEventHandlerUPP handler = nullptr;
    SRefCon refcon = 0;
    if (AEGetEventHandler(kCoreEventClass, kAEOpenDocuments, &handler, &refcon, false) != noErr
        || handler == nullptr)
        return "none";
    return handler == handlerUpp() ? "ours" : "another";
}

std::function<void(const QUrl &)> &sink()
{
    static std::function<void(const QUrl &)> s;
    return s;
}

QList<QUrl> &pending()
{
    static QList<QUrl> p;
    return p;
}

void deliver(const QUrl &url)
{
    if (sink())
        sink()(url);
    else
        pending().append(url);
}

/* kAEOpenDocuments: a list of files (each coerced to a file URL) */
OSErr openDocuments(const AppleEvent *event, AppleEvent *, SRefCon)
{
    AEDescList list;
    const OSErr listErr = AEGetParamDesc(event, keyDirectObject, typeAEList, &list);
    if (listErr != noErr)
    {
        qDebug("Apple events: an open-documents event without a list (%d)", int(listErr));
        return errAEDescNotFound;
    }
    long count = 0;
    if (AECountItems(&list, &count) != noErr)
        count = 0;
    qDebug("Apple events: an open-documents event of %ld file(s)", count);
    for (long i = 1; i <= count; ++i)
    {
        AEKeyword keyword;
        AEDesc item;
        const OSErr itemErr = AEGetNthDesc(&list, i, typeFileURL, &keyword, &item);
        if (itemErr != noErr)
        {
            qDebug("Apple events: file %ld is not a file URL (%d)", i, int(itemErr));
            continue;
        }
        const Size size = AEGetDescDataSize(&item);
        QByteArray data(int(size), Qt::Uninitialized);
        const bool read = size > 0 && AEGetDescData(&item, data.data(), size) == noErr;
        AEDisposeDesc(&item);
        if (!read)
            continue;
        const QUrl url = QUrl::fromEncoded(data);
        if (url.isValid() && !url.isEmpty())
            deliver(url);
    }
    AEDisposeDesc(&list);
    return noErr;
}

void installHandler(const char *when)
{
    const char *before = currentHandler();
    const OSErr err = AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments, handlerUpp(), 0, false);
    qDebug("Apple events: the open-documents handler installed %s (it was %s; %d)",
           when, before, int(err));
}

/* AppKit puts its own handlers in place as the launch ends: Apple's advice is
 * to replace them when the application will finish launching */
void willFinishLaunching(CFNotificationCenterRef, void *, CFStringRef,
                         const void *, CFDictionaryRef)
{
    installHandler("as the launch ends");
}

void didFinishLaunching(CFNotificationCenterRef, void *, CFStringRef,
                        const void *, CFDictionaryRef info)
{
    // (false: the system launched the app to open documents)
    const void *value = info ? CFDictionaryGetValue(info, CFSTR("NSApplicationLaunchIsDefaultLaunchKey"))
                             : nullptr;
    const char *defaultLaunch = "unknown";
    if (value && CFGetTypeID(value) == CFBooleanGetTypeID())
        defaultLaunch = CFBooleanGetValue(static_cast<CFBooleanRef>(value)) ? "yes" : "no";
    qDebug("Apple events: the launch finished (a default launch: %s; the handler: %s)",
           defaultLaunch, currentHandler());
}

} // namespace

void MacOpenDocuments::install()
{
    static bool observing = false;
    if (!observing)
    {
        observing = true;
        CFNotificationCenterRef center = CFNotificationCenterGetLocalCenter();
        CFNotificationCenterAddObserver(center, &observing, willFinishLaunching,
                                        CFSTR("NSApplicationWillFinishLaunchingNotification"),
                                        nullptr, CFNotificationSuspensionBehaviorDeliverImmediately);
        CFNotificationCenterAddObserver(center, &observing, didFinishLaunching,
                                        CFSTR("NSApplicationDidFinishLaunchingNotification"),
                                        nullptr, CFNotificationSuspensionBehaviorDeliverImmediately);
        installHandler("at the start");
        return;
    }
    installHandler("again");
}

void MacOpenDocuments::setSink(std::function<void(const QUrl &)> s)
{
    sink() = std::move(s);
    if (!sink())
        return;
    const QList<QUrl> kept = std::exchange(pending(), {});
    for (const QUrl &url : kept)
        sink()(url);
}

#else

void MacOpenDocuments::install() {}
void MacOpenDocuments::setSink(std::function<void(const QUrl &)>) {}

#endif
