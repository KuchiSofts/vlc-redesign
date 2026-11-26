/*****************************************************************************
 * specific.c: stubs for POSIX OS-specific initialization
 *****************************************************************************
 * Copyright © 2008 Rémi Denis-Courmont
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>
#include "../libvlc.h"
#include "../lib/libvlc_internal.h"

#ifdef HAVE_DBUS
/* used for one-instance mode */
# include <dbus/dbus.h>
# include <vlc_url.h>
# define MPRIS_APPEND "/org/mpris/MediaPlayer2/TrackList/Append"
# define MPRIS_BUS_NAME "org.mpris.MediaPlayer2.vlc"
# define MPRIS_OBJECT_PATH "/org/mpris/MediaPlayer2"
# define MPRIS_TRACKLIST_INTERFACE "org.mpris.MediaPlayer2.TrackList"
/* Keep in sync with modules/control/dbus/dbus_common.h. */
# define VLC_PLAYLIST_INTERFACE "org.videolan.vlc.Playlist"

static const char *resolve_mrl(const char *arg, char **buffer)
{
    *buffer = NULL;
    if (strstr(arg, "://"))
        return arg;
    /* Resolve relative paths in the sending instance. */
    *buffer = vlc_path2uri(arg, NULL);
    return *buffer;
}

static bool send_handoff(libvlc_int_t *vlc, DBusConnection *conn, DBusMessage *req)
{
    DBusError error;
    dbus_error_init(&error);
    DBusMessage *reply = dbus_connection_send_with_reply_and_block(conn,
                                                               req, -1, &error);
    if (!reply)
    {
        msg_Err(vlc, "D-Bus error: %s", error.message);
        dbus_error_free(&error);
        return false;
    }

    dbus_message_unref(reply);
    return true;
}

static bool handoff_replace(libvlc_int_t *vlc, DBusConnection *conn, int argc,
                            const char *const argv[])
{
    DBusMessage *req = dbus_message_new_method_call(MPRIS_BUS_NAME,
        MPRIS_OBJECT_PATH, VLC_PLAYLIST_INTERFACE, "ReplaceAndPlay");
    if (!req)
        return false;

    DBusMessageIter args, uris;
    dbus_message_iter_init_append(req, &args);
    bool sent = false;
    if (!dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "s", &uris))
        goto out;

    for (int i = 0; i < argc; i++)
    {
        char *buffer;
        const char *mrl = resolve_mrl(argv[i], &buffer);
        bool ok = mrl && dbus_message_iter_append_basic(&uris, DBUS_TYPE_STRING, &mrl);
        free(buffer);
        if (!ok)
        {
            dbus_message_iter_abandon_container(&args, &uris);
            goto out;
        }
    }

    if (dbus_message_iter_close_container(&args, &uris))
        sent = send_handoff(vlc, conn, req);

out:
    dbus_message_unref(req);
    return sent;
}
#endif

void system_Init (void)
{
}

static void system_ConfigureDbus(libvlc_int_t *vlc, int argc,
                                 const char *const argv[])
{
/* FIXME: could be replaced by using Unix sockets */
#ifdef HAVE_DBUS
    dbus_threads_init_default();

    if (var_InheritBool(vlc, "dbus"))
        libvlc_InternalAddIntf(vlc, "dbus,none");

    if (!var_InheritBool(vlc, "one-instance")
     && !(var_InheritBool(vlc, "one-instance-when-started-from-file")
       && var_InheritBool(vlc, "started-from-file")))
         return;

    for (int i = 0; i < argc; i++)
        if (argv[i][0] == ':')
        {
            msg_Err(vlc, "item option %s incompatible with single instance",
                    argv[i]);
            return;
        }

    char *name = var_GetString(vlc, "dbus-mpris-name");
    if (name != NULL)
    {
        bool singleton = !strcmp(name, MPRIS_BUS_NAME);
        free(name);
        if (singleton)
        {
            msg_Dbg(vlc, "no running VLC instance - continuing normally...");
            return; /* This is the single instance */
        }
    }

    /* Initialise D-Bus interface, check for other instances */
    DBusError err;
    dbus_error_init(&err);

    /* connect to the session bus */
    DBusConnection *conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (conn == NULL)
    {
        msg_Err(vlc, "D-Bus session bus connection failure: %s",
                 err.message);
        dbus_error_free(&err);
        return;
    }

    const bool replace = !var_InheritBool(vlc, "playlist-enqueue");
    if (argc > 0 && replace && handoff_replace(vlc, conn, argc, argv))
    {
        dbus_connection_unref(conn);
        msg_Warn(vlc, "running VLC instance - exiting...");
        exit(0);
    }

    msg_Warn(vlc, "running VLC instance - exiting...");

    /* An older instance without ReplaceAndPlay still gets the files, played
     * as before. */
    const dbus_bool_t play = replace;
    for (int i = 0; i < argc; i++)
    {
        DBusMessage *req = dbus_message_new_method_call(MPRIS_BUS_NAME,
            MPRIS_OBJECT_PATH, MPRIS_TRACKLIST_INTERFACE, "AddTrack");
        if (unlikely(req == NULL))
            continue;

        char *mrlbuf;
        const char *mrl = resolve_mrl(argv[i], &mrlbuf);
        if (unlikely(mrl == NULL))
        {
            dbus_message_unref(req);
            continue;
        }

        /* append MRLs */
        msg_Dbg(vlc, "adding track %s to running instance", mrl);

        const char *after_track = MPRIS_APPEND;
        dbus_bool_t ok = dbus_message_append_args(req, DBUS_TYPE_STRING, &mrl,
                                           DBUS_TYPE_OBJECT_PATH, &after_track,
                                           DBUS_TYPE_BOOLEAN, &play,
                                           DBUS_TYPE_INVALID);
        free(mrlbuf);
        if (ok)
            send_handoff(vlc, conn, req);
        dbus_message_unref(req);
    }

    /* we unreference the connection when we've finished with it */
    dbus_connection_unref(conn);
    exit(0);
#else
    (void) vlc; (void) argc; (void) argv;
#endif // HAVE_DBUS
}

void system_Configure(libvlc_int_t *libvlc,
                      int argc, const char *const argv[])
{
    system_ConfigureDbus(libvlc, argc, argv);
}
