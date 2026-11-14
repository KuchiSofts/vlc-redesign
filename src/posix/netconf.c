/*****************************************************************************
 * netconf.c : Network configuration
 *****************************************************************************
 * Copyright (C) 2013 Rémi Denis-Courmont
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

#include <stdlib.h>
#include <string.h>

#include <vlc_common.h>
#include <vlc_configuration.h>
#include <vlc_fs.h>
#include <vlc_network.h>
#include <vlc_spawn.h>
#include <vlc_url.h>

/* The proxy of VLC's settings ("http-proxy", with "http-proxy-pwd" when it
 * names no password of its own), as on Windows and macOS; NULL when there is
 * none (then the system's: libproxy's "proxy" tool, else http_proxy) */
static char *OwnProxy(void)
{
    char *proxy = config_GetPsz("http-proxy");
    if (proxy == NULL || *proxy == '\0') {
        free(proxy);
        return NULL;
    }

    char *proxy_pwd = config_GetPsz("http-proxy-pwd");
    if (proxy_pwd == NULL || *proxy_pwd == '\0') {
        free(proxy_pwd);
        return proxy;
    }

    vlc_url_t parsed;
    if (vlc_UrlParse(&parsed, proxy) < 0) {
        vlc_UrlClean(&parsed);
        free(proxy);
        free(proxy_pwd);
        return NULL;
    }
    if (parsed.psz_password == NULL)
        parsed.psz_password = proxy_pwd;

    char *proxy_url = vlc_uri_compose(&parsed);
    vlc_UrlClean(&parsed);
    free(proxy_pwd);
    free(proxy);
    return proxy_url;
}

char *vlc_getProxyUrl(const char *url)
{
    char *own = OwnProxy();
    if (own != NULL)
        return own;

    int fd[2];

    if (vlc_pipe(fd))
        return NULL;

    pid_t pid;
    int fdv[] = { -1, fd[1], STDERR_FILENO, -1 };
    const char *argv[] = { "proxy", url, NULL };

    if (vlc_spawnp(&pid, "proxy", fdv, argv))
        pid = -1;

    vlc_close(fd[1]);

    if (pid != -1)
    {
        char buf[1024];
        size_t len = 0;

        do
        {
             ssize_t val = read(fd[0], buf + len, sizeof (buf) - len);
             if (val <= 0)
                 break;
             len += val;
        }
        while (len < sizeof (buf));

        vlc_close(fd[0]);
        vlc_waitpid(pid);

        if (len >= 9 && !strncasecmp(buf, "direct://", 9))
            return NULL;

        char *end = memchr(buf, '\n', len);
        if (end != NULL)
        {
            *end = '\0';
            return strdup(buf);
        }
        /* Parse error: fallback (may be due to missing executable) */
    }
    else
        vlc_close(fd[0]);

    /* Fallback to environment variable */
    char *var = getenv("http_proxy");
    if (var != NULL)
        var = strdup(var);
    return var;
}
