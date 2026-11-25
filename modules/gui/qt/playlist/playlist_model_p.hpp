/*****************************************************************************
 * playlist_model_p.hpp
 *****************************************************************************
 * Copyright (C) 2018 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/
#ifndef PLAYLIST_MODEL_P_HPP
#define PLAYLIST_MODEL_P_HPP

#include "playlist_model.hpp"
#include "playlist_duration_total.hpp"

namespace vlc {
namespace playlist {

class PlaylistListModelPrivate
{
    Q_DISABLE_COPY(PlaylistListModelPrivate)
public:
    Q_DECLARE_PUBLIC(PlaylistListModel)
    PlaylistListModel* const q_ptr;

public:
    PlaylistListModelPrivate(PlaylistListModel* playlistList);
    ~PlaylistListModelPrivate();

    ///call function @a fun on object thread
    template <typename Fun>
    inline void callAsync(Fun&& fun)
    {
        Q_Q(PlaylistListModel);
        // NOTE: Starting with Qt 6.7.0, lambda expression here without a return value
        //       causes compilation issues with some compilers.
        // TODO: Find out if a more recent Qt version does not behave that way.
        QMetaObject::invokeMethod(q, [this, generation = m_generation, fun = std::forward<Fun>(fun)]() -> std::monostate {
            if (generation == m_generation)
                fun();
            return {};
        }, Qt::QueuedConnection);
    }

    void onItemsReset(const QVector<PlaylistItem>&& items);
    void onItemsAdded(const QVector<PlaylistItem>&& added, size_t index);
    void onItemsMoved(size_t index, size_t count, size_t target);
    void onItemsRemoved(size_t index, size_t count);
    void scheduleDurationChanged();

    void notifyItemsChanged(int index, int count,
                            const QVector<int> &roles = {});

    vlc_playlist_t* m_playlist = nullptr;
    vlc_playlist_listener_id *m_listener = nullptr;

    /* Playlist callbacks read this under the playlist lock. Only change it on
     * the UI thread between removing the old listener and adding the new one. */
    std::uint64_t m_generation = 0;

    /* access only from the UI thread */
    QVector<PlaylistItem> m_items;
    ssize_t m_current = -1;

    PlaylistDurationTotal m_duration;
    bool m_durationChangePending = false;
};

} //namespace playlist
} //namespace vlc


#endif // PLAYLIST_MODEL_P_HPP
