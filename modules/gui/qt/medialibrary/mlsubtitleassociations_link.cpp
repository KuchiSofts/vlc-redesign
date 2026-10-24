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
        m_busy = false;

        const QByteArray& payload = reply.body;
        if (payload.size() > maxSubtitleBytes)
        {
            setError(tr("The subtitle exceeds the 20 MB download limit."));
            return;
        }
        if (!reply.error.isEmpty() || reply.status != 200 || payload.isEmpty())
        {
            setError(tr("Could not download the subtitle: %1")
                     .arg(reply.error.isEmpty() ? tr("HTTP status %1").arg(reply.status) : reply.error));
            return;
        }

        // in the profile of VLC Redesign, not in %APPDATA%\vlc of an installed
        // VLC (Qt's AppDataLocation); apart from the finder's cache, which it
        // cleans up: these are kept
        char *profile = config_GetUserDir(VLC_USERDATA_DIR);
        const QString folder = QString::fromUtf8(profile ? profile : "") + QStringLiteral("/linked-subtitles");
        free(profile);
        if (!QDir().mkpath(folder))
        {
            setError(tr("Could not create the subtitle storage folder."));
            return;
        }
        const QString hash = QString::fromLatin1(QCryptographicHash::hash(
            url.toString(QUrl::FullyEncoded).toUtf8(), QCryptographicHash::Sha256).toHex());
        const QString path = folder + QLatin1Char('/') + hash + QLatin1Char('.')
                             + QFileInfo(name).suffix().toLower();
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly) || file.write(payload) != payload.size() || !file.commit())
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
}
