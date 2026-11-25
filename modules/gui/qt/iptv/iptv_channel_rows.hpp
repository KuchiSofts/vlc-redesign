/* Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef IPTV_CHANNEL_ROWS_HPP
#define IPTV_CHANNEL_ROWS_HPP

#include <QString>
#include <QStringList>
#include <QVector>

#include <optional>

class IptvChannelSearch
{
public:
    explicit IptvChannelSearch(const QString& search)
        : m_needle(search.trimmed().toCaseFolded())
        , m_number(m_needle.toInt(&m_numeric))
    {
    }

    bool matches(const QString& nameFolded, int number) const
    {
        return m_needle.isEmpty() || nameFolded.contains(m_needle) || (m_numeric && number == m_number);
    }

    template <typename FilterRows>
    std::optional<QVector<int>> rowsAfterRenumbering(const QVector<int>& previousRows, FilterRows filterRows) const
    {
        if (!m_numeric)
            return std::nullopt;
        auto rows = filterRows();
        if (rows == previousRows)
            return std::nullopt;
        return rows;
    }

private:
    QString m_needle;
    bool m_numeric = false;
    int m_number;
};

template <typename Channel, typename IndexOfKey>
QVector<int> iptvChannelRows(const QVector<Channel>& channels, const QString& group,
                            const QStringList& keys, const IptvChannelSearch& search,
                            bool showAdult, IndexOfKey indexOfKey)
{
    const auto matches = [&](const Channel& channel) {
        return (!channel.adult || showAdult) && search.matches(channel.nameFolded, channel.number);
    };
    QVector<int> rows;
    if (group == QLatin1String("recent") || group == QLatin1String("favorites"))
    {
        // the order of the list kept (the last watched first)
        for (const QString& key : keys)
        {
            const int i = indexOfKey(key);
            if (i >= 0 && matches(channels[i]))
                rows.append(i);
        }
    }
    else
    {
        for (int i = 0; i < channels.size(); ++i)
            if ((group.isEmpty() || channels[i].group == group) && matches(channels[i]))
                rows.append(i);
    }
    return rows;
}

#endif
