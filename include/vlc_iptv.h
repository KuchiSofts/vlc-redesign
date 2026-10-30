/*****************************************************************************
 * vlc_iptv.h: the Live TV (IPTV) add-on
 *****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *****************************************************************************/

#ifndef VLC_IPTV_H
#define VLC_IPTV_H 1

/**
 * \defgroup iptv Live TV (IPTV) add-on
 * \ingroup interface
 *
 * The module of capability "iptv" (modules/misc/iptv, the "iptv" module)
 * keeps the user's TV sources (M3U playlists, Xtream Codes accounts, XMLTV
 * guides), downloads and reads them on its own thread, caches them on disk
 * and answers what an interface asks about the channels and the guide.
 *
 * The interface and the module only exchange JSON text, so that neither
 * links to the other: the add-on can be left out of a package (its plugin
 * file deleted) and the interface simply has no Live TV.
 *
 * The interface creates the object with vlc_object_create(), sets the owner
 * callbacks, then loads the module with module_need(obj, "iptv", "iptv",
 * true) once module_exists("iptv") said it is there.
 * @{
 */

typedef struct vlc_iptv vlc_iptv_t;

struct vlc_iptv_owner_cbs
{
    /**
     * Something changed (sources, channels, guide, catalog, progress): a
     * JSON object {"event": "...", ...}. Called from a thread of the
     * module; request() must not be called from it.
     */
    void (*on_event)(vlc_iptv_t *iptv, const char *json, void *data);
};

struct vlc_iptv
{
    struct vlc_object_t obj;

    /* set by the interface before module_need() */
    const struct vlc_iptv_owner_cbs *owner_cbs;
    void *owner_data;

    /**
     * Set by the module: answers a JSON request {"op": "...", ...} with a
     * JSON object to free(), {"error": "..."} when it cannot be done, or
     * NULL when out of memory. May block (a download): call it from a
     * thread of the interface, not its event loop. Thread-safe.
     */
    char *(*request)(vlc_iptv_t *iptv, const char *json);

    void *sys;
};

/** @} */

#endif
