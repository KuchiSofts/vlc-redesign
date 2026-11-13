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
#include <QList>

#include <utility>

namespace
{

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
    if (AEGetParamDesc(event, keyDirectObject, typeAEList, &list) != noErr)
        return errAEDescNotFound;
    long count = 0;
    if (AECountItems(&list, &count) != noErr)
        count = 0;
    for (long i = 1; i <= count; ++i)
    {
        AEKeyword keyword;
        AEDesc item;
        if (AEGetNthDesc(&list, i, typeFileURL, &keyword, &item) != noErr)
            continue;
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

void installHandler()
{
    static const AEEventHandlerUPP handler = NewAEEventHandlerUPP(openDocuments);
    AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments, handler, 0, false);
}

/* AppKit puts its own handlers in place as the launch ends: Apple's advice is
 * to replace them when the application will finish launching */
void willFinishLaunching(CFNotificationCenterRef, void *, CFStringRef,
                         const void *, CFDictionaryRef)
{
    installHandler();
}

} // namespace

void MacOpenDocuments::install()
{
    static bool observing = false;
    if (!observing)
    {
        observing = true;
        CFNotificationCenterAddObserver(CFNotificationCenterGetLocalCenter(), &observing,
                                        willFinishLaunching,
                                        CFSTR("NSApplicationWillFinishLaunchingNotification"),
                                        nullptr, CFNotificationSuspensionBehaviorDeliverImmediately);
    }
    installHandler();
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
