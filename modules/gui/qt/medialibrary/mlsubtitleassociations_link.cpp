/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or later.
 *****************************************************************************/
#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "mlsubtitleassociations.hpp"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QSignalBlocker>

#include <vlc_configuration.h>
#include <vlc_input_item.h>

#include "util/http_request.hpp"

namespace {
constexpr qint64 maxSubtitleBytes = 20 * 1024 * 1024;

bool isSubtitleName(const QString& name)
{
    enum slave_type type;
    const QByteArray encoded = QFile::encodeName(name);
    return input_item_slave_GetType(encoded.constData(), &type) && type == SLAVE_TYPE_SPU;
}
}

void MLSubtitleAssociations::addLink(const QUrl& url)
{
    addLinkWithName(url, QFileInfo(url.path()).fileName());
}

void MLSubtitleAssociations::addLinkWithName(const QUrl& url, const QString& filename)
{
    if (m_busy)
        return;
    const QString name = QFileInfo(filename).fileName();
    if (name != filename)
    {
        setError(tr("The subtitle filename is invalid."));
        return;
    }
    if (!url.isValid() || url.scheme() != QStringLiteral("https") || url.host().isEmpty() ||
        !url.userInfo().isEmpty() || !isSubtitleName(name))
    {
        setError(tr("Enter a direct HTTPS link to a subtitle file (.srt, .ass, .ssa, .sub, .vtt, .ttml or .smi)."));
        return;
    }

    if (!httpRequestSupported())
    {
        setError(tr("Downloading subtitles is not available in this build."));
        return;
    }
    HttpRequest request;
    request.url = url;
    request.headers = {{"Accept", "text/plain, application/octet-stream, */*"}};
    request.timeoutMs = 30000;
    // the whole download, however slowly it comes (each step has its own
    // timeout): the dialog is never left busy
    request.deadlineMs = 90 * 1000;
    request.maxBytes = maxSubtitleBytes;

    m_busy = true;
    m_error.clear();
    emit changed();
    const quint64 revision = m_revision;
    // Redirects never downgrade to HTTP (see util/http_request.cpp).
    httpRequestAsync(this, request, [this, revision, url, name](const HttpResponse& reply) {
        if (revision != m_revision)
            return;
        // changed() is emitted once the file is added and matched (or with
        // the error): the dialog saves on the first change that is not busy
        const QByteArray& payload = reply.body;
        if (payload.size() > maxSubtitleBytes)
        {
            m_busy = false;
            setError(tr("The subtitle exceeds the 20 MB download limit."));
            return;
        }
        if (!reply.error.isEmpty() || reply.status != 200 || payload.isEmpty())
        {
            m_busy = false;
            setError(tr("Could not download the subtitle: %1")
                     .arg(reply.error.isEmpty() ? tr("HTTP status %1").arg(reply.status) : reply.error));
            return;
        }

        // in the profile of VLC Redesign, not in %APPDATA%\vlc of an installed
        // VLC (Qt's AppDataLocation); apart from the finder's cache, which it
        // cleans up: these are kept
        char *profile = config_GetUserDir(VLC_USERDATA_DIR);
        // (none: nowhere, not "/linked-subtitles" at the root of the drive)
        const QString folder = profile && *profile ? QString::fromUtf8(profile) + QStringLiteral("/linked-subtitles")
                                                   : QString();
        free(profile);
        const QString hash = QString::fromLatin1(QCryptographicHash::hash(
            url.toString(QUrl::FullyEncoded).toUtf8(), QCryptographicHash::Sha256).toHex());
        const QString path = folder + QLatin1Char('/') + hash + QLatin1Char('.')
                             + QFileInfo(name).suffix().toLower();
        // Written on a worker thread (up to 20 MB, maybe to a slow drive),
        // still busy meanwhile; its answer dropped if the dialog moved on
        // (cancel(), another destination) or is gone
        enum class Saved { Yes, NoFolder, NoFile };
        auto saved = std::make_shared<Saved>(Saved::NoFile);
        const QByteArray data = payload;
        const bool started = runInBackground(this, [saved, folder, path, data]() {
            if (folder.isEmpty() || !QDir().mkpath(folder))
            {
                *saved = Saved::NoFolder;
                return;
            }
            QSaveFile file(path);
            if (file.open(QIODevice::WriteOnly) && file.write(data) == data.size() && file.commit())
                *saved = Saved::Yes;
        }, [this, revision, saved, path, name]() {
            if (revision != m_revision)
                return;
            m_busy = false;
            if (*saved == Saved::NoFolder)
            {
                setError(tr("Could not create the subtitle storage folder."));
                return;
            }
            if (*saved == Saved::NoFile)
            {
                setError(tr("Could not save the downloaded subtitle."));
                return;
            }
            const QUrl local = QUrl::fromLocalFile(path);
            {
                // the file is matched under its name (not the hash), then the
                // change is signalled
                const QSignalBlocker blocker(this);
                addFiles({local});
            }
            for (auto& source : m_sources)
                if (source.url == local)
                    source.label = name;
            match();
        });
        if (!started)
            m_busy = false;
    });
}
