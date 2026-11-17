/*****************************************************************************
 * Copyright (C) 2021 VLC authors and VideoLAN
 *
 * Authors: Benjamin Arnaud <bunjee@omega.gg>
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

#include "mlgroup.hpp"

#include "util/vlctick.hpp"

// VLC includes
#include "qt.hpp"

#include <QRegularExpression>

// The library names a group of videos by the start their names share: the
// episodes of a show give "My Show S01E" (S01E01, S01E02...), "Show
// 1x0" or "Show Episode ". Shown as the show's name, without what is left of
// an episode's number (#108); a name with nothing else stays as it is
static QString showName(const QString &name)
{
    static const QRegularExpression marker(
        QStringLiteral("(?:^|[\\s._\\-(\\[])(?:S\\d{0,2}(?:\\s*E\\d{0,3})?|Season\\s*\\d*|\\d{1,2}x\\d{0,3}"
                       "|E(?:p(?:isode)?)?\\.?\\s*\\d{0,4}|Part\\s*\\d*)$"),
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression tail(QStringLiteral("[\\s._\\-\\x{2013}\\x{2014}(\\[]+$"));
    QString shown = name;
    shown.remove(marker);
    shown.remove(tail);
    shown = shown.trimmed();
    return shown.isEmpty() ? name : shown;
}

//-------------------------------------------------------------------------------------------------
// Ctor / dtor
//-------------------------------------------------------------------------------------------------

MLGroup::MLGroup(const vlc_ml_group_t * data)
    : MLItem(MLItemId(data->i_id, VLC_ML_PARENT_GROUP))
    , m_title(showName(qfu(data->psz_name)))
    , m_duration(data->i_duration)
    , m_date(QDateTime::fromSecsSinceEpoch(data->i_creation_date))
    , m_count(data->i_nb_total_media)
{
    assert(data);
}

//-------------------------------------------------------------------------------------------------
// Interface
//-------------------------------------------------------------------------------------------------

QString MLGroup::getTitle() const
{
    return m_title;
}

//-------------------------------------------------------------------------------------------------

VLCDuration MLGroup::getDuration() const
{
    return VLCDuration::fromMS(m_duration);
}

QDateTime MLGroup::getDate() const
{
    return m_date;
}

unsigned int MLGroup::getCount() const
{
    return m_count;
}
