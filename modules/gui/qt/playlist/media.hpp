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

#ifndef VLC_QT_MEDIA_HPP_
#define VLC_QT_MEDIA_HPP_

#include <QRegularExpression>
#include <QString>
#include <QStringList>

#include <mutex>

#include "util/qt_dirs.hpp"
#include "util/shared_input_item.hpp"

namespace vlc {
  namespace playlist {

class Media
{
public:
    Media(input_item_t *media = nullptr, const QStringList &options = {})
    {
        if (media)
        {
            /* the media must be unique in the playlist */
            ptr.reset(input_item_Copy(media), false);
            if (!ptr)
                throw std::bad_alloc();
            setMediaOptions(options);
            applyHistoryFilter();
        }
    }

    // "Never remember items matching" of the settings (a regular
    // expression): the items whose address or name matches are not saved in
    // the history, nor their position. Set on the UI thread, read by the
    // threads of the media library too: a copy is taken under the lock
    static void setHistoryFilter(const QRegularExpression &filter)
    {
        std::lock_guard<std::mutex> guard(historyFilterLock());
        historyFilterValue() = filter;
    }

    static QRegularExpression historyFilter()
    {
        std::lock_guard<std::mutex> guard(historyFilterLock());
        return historyFilterValue();
    }

    void applyHistoryFilter()
    {
        const QRegularExpression filter = historyFilter();
        if (!ptr || filter.pattern().isEmpty() || !filter.isValid())
            return;
        char *uri = input_item_GetURI(ptr.get());
        char *name = input_item_GetName(ptr.get());
        const bool matches = (uri && filter.match(QString::fromUtf8(uri)).hasMatch())
                          || (name && filter.match(QString::fromUtf8(name)).hasMatch());
        free(uri);
        free(name);
        if (matches)
            input_item_AddOption(ptr.get(), "no-save-recentplay", VLC_INPUT_OPTION_TRUSTED);
    }

    Media(const QString &uri, const QString &name, const QStringList &options = {})
    {
        auto uUri = uri.toUtf8();
        auto uName = name.toUtf8();
        const char *rawUri = uUri.isNull() ? nullptr : uUri.constData();
        const char *rawName = uName.isNull() ? nullptr : uName.constData();
        ptr.reset(input_item_New(rawUri, rawName), false);
        if (!ptr)
            throw std::bad_alloc();
        setMediaOptions(options);
        applyHistoryFilter();
    }

    void setMediaOptions(const QStringList &options = {})
    {
        if (options.count() > 0)
        {
            char **ppsz_options = NULL;
            int i_options = 0;

            ppsz_options = new char *[options.count()];
            auto optionDeleter = vlc::wrap_carray<char*>(ppsz_options, [&i_options](char *ptr[]) {
                for(int i = 0; i < i_options; i++)
                    free(ptr[i]);
                delete[] ptr;
            });

            for (int i = 0; i < options.count(); i++)
            {
                QString option = colon_unescape( options[i] );
                ppsz_options[i] = strdup(option.toUtf8().constData());
                if (!ppsz_options[i])
                    throw std::bad_alloc();
                i_options++;
            }
            input_item_AddOptions( ptr.get(), i_options, ppsz_options, VLC_INPUT_OPTION_TRUSTED );
        }
    }

    operator bool() const
    {
        return static_cast<bool>(ptr);
    }

    input_item_t *raw() const
    {
        return ptr.get();
    }

private:
    static std::mutex &historyFilterLock()
    {
        static std::mutex lock;
        return lock;
    }

    static QRegularExpression &historyFilterValue()
    {
        static QRegularExpression filter;
        return filter;
    }

    SharedInputItem ptr;
};

  } // namespace playlist
} // namespace vlc

#endif
