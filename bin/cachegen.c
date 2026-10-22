/*****************************************************************************
 * cachegen.c: LibVLC plugins cache generator
 *****************************************************************************
 * Copyright (C) 2010 Rémi Denis-Courmont
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc/vlc.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#ifdef HAVE_GETOPT_H
# include <getopt.h>
#endif
#ifdef _WIN32
# include <windows.h>
#endif

#ifdef _WIN32
/* Installers and updaters wait for this program: a plugin that blocks while
 * it loads (a library of another program injected in this one, a security
 * suite holding the file) must not make them wait forever */
#define WATCHDOG_MS (90 * 1000)

static DWORD WINAPI Watchdog (void *data)
{
    SetEvent ((HANDLE) data);
    Sleep (WATCHDOG_MS);
    fputs ("vlc-cache-gen: the plugins did not load in time\n", stderr);
    /* not ExitProcess(): the loader lock may be held by the hung load */
    TerminateProcess (GetCurrentProcess (), 2);
    return 0;
}

static void StartWatchdog (void)
{
    HANDLE started = CreateEvent (NULL, TRUE, FALSE, NULL);
    if (started == NULL)
        return;
    HANDLE thread = CreateThread (NULL, 0, Watchdog, started, 0, NULL);
    /* running before the plugins load: a new thread needs the loader lock */
    if (thread != NULL)
    {
        if (WaitForSingleObject (started, 5000) == WAIT_OBJECT_0)
            CloseHandle (started);
        CloseHandle (thread);
    }
    else
        CloseHandle (started);
}
#endif

static void version (void)
{
    puts ("LibVLC plugins cache generation version "VERSION);
}

static void usage (const char *path)
{
    printf (
"Usage: %s <path>\n"
"Generate the LibVLC plugins cache for the specified plugins directory.\n",
            path);
}

int main (int argc, char *argv[])
{
#ifdef HAVE_GETOPT_H
    static const struct option opts[] =
    {
        { "help",       no_argument,       NULL, 'h' },
        { "version",    no_argument,       NULL, 'V' },
        { NULL,         no_argument,       NULL, '\0'}
    };

    int c;

    while ((c = getopt_long (argc, argv, "hV", opts, NULL)) != -1)
        switch (c)
        {
            case 'h':
                usage (argv[0]);
                return 0;
            case 'V':
                version ();
                return 0;
            default:
                usage (argv[0]);
                return 1;
        }
#else
    int optind = 1;
#endif

#ifdef _WIN32
    StartWatchdog ();
#endif

    for (int i = optind; i < argc; i++)
    {
        const char *path = argv[i];

        if (setenv ("VLC_PLUGIN_PATH", path, 1))
            abort ();

        const char *vlc_argv[5];
        int vlc_argc = 0;

        vlc_argv[vlc_argc++] = "--quiet";
        /* not the settings of the user: their interfaces (web, console),
         * one instance mode and VLM would start in this program too */
        vlc_argv[vlc_argc++] = "--ignore-config";
        vlc_argv[vlc_argc++] = "--reset-plugins-cache";
        vlc_argv[vlc_argc++] = "--"; /* end of options */
        vlc_argv[vlc_argc] = NULL;

        libvlc_instance_t *vlc = libvlc_new (vlc_argc, vlc_argv);
        if (vlc == NULL)
            return 1;
        libvlc_release(vlc);
    }

    return 0;
}
