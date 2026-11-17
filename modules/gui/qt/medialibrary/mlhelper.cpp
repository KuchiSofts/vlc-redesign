/*****************************************************************************
 * Copyright (C) 2019 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * ( at your option ) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#include "mlhelper.hpp"

#include <QDir>

QString toValidLocalFile(const char *mrl)
{
    QUrl url(mrl);
    return url.isLocalFile() ? url.toLocalFile() : QString {};
}

QString channelNameFromNbChannels(unsigned nbChannels)
{
    if (nbChannels >= 8)
        return QStringLiteral("7.1");
    else if (nbChannels >= 6)
        return QStringLiteral("5.1");
    return {};
}

QString resolutionNameFromSize(unsigned maxWidth, unsigned maxHeight)
{
    if (maxWidth == 0 || maxHeight == 0)
        return {};

    const unsigned int realHeight = (maxHeight < maxWidth) ? maxHeight : maxWidth;
    const unsigned int realWidth = (maxHeight < maxWidth) ? maxWidth : maxHeight;

    if (realHeight >= 4320 || realWidth >= 4320.0 * (16.0 / 9.0))
        return QStringLiteral("8K");
    else if (realHeight >= 2160 || realWidth >= 2160.0 * (16.0 / 9.0))
        return QStringLiteral("4K");
    else if (realHeight >= 1080 || realWidth >= 1080.0 * (16.0 / 9.0))
        return QStringLiteral("HD");
    else if (realHeight >= 540 || realWidth >= 540.0 * (16.0 / 9.0))
        return QStringLiteral("SD");
    return {};
}

QString urlToDisplayString(const QUrl &url)
{
    const QString displayString = url.toDisplayString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::NormalizePathSegments);
    if (url.isLocalFile())
        return QDir::toNativeSeparators(displayString);

    return displayString;
}

QUrl getParentURLFromURL(const QString &mrl)
{
    QUrl fileUrl(mrl);
    QUrl parentDirUrl = fileUrl.adjusted(QUrl::RemoveFilename);
    return parentDirUrl;
}

QString getFirstSymbol(const QString &str)
{
    QString ret("#");
    if ( str.length() > 0 && str[0].isLetter() )
        ret = str[0].toUpper();
    return ret;
}

int naturalCompare(const QString &a, const QString &b)
{
    // (by hand: QCollator's numeric mode needs ICU, which a build may lack)
    qsizetype i = 0, j = 0;
    while (i < a.size() && j < b.size())
    {
        if (a[i].isDigit() && b[j].isDigit())
        {
            // a run of digits each: the shorter number (without its leading
            // zeros) is the smaller, then digit by digit
            qsizetype si = i, sj = j;
            while (si < a.size() && a[si] == u'0') ++si;
            while (sj < b.size() && b[sj] == u'0') ++sj;
            qsizetype ei = si, ej = sj;
            while (ei < a.size() && a[ei].isDigit()) ++ei;
            while (ej < b.size() && b[ej].isDigit()) ++ej;
            if (ei - si != ej - sj)
                return (ei - si) < (ej - sj) ? -1 : 1;
            const int digits = QStringView(a).mid(si, ei - si).compare(QStringView(b).mid(sj, ej - sj));
            if (digits != 0)
                return digits;
            i = ei;
            j = ej;
            continue;
        }
        const QChar ca = a[i].toCaseFolded(), cb = b[j].toCaseFolded();
        if (ca != cb)
            return ca < cb ? -1 : 1;
        ++i;
        ++j;
    }
    if (i < a.size() || j < b.size())
        return i < a.size() ? 1 : -1;
    return QString::compare(a, b, Qt::CaseInsensitive);
}
