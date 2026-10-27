/*****************************************************************************
 * netconf.c : Network configuration
 *****************************************************************************
 * Copyright (C) 2001-2008 VLC authors and VideoLAN
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

#include <ctype.h>
#include <string.h>

#include <vlc_common.h>
#include <vlc_charset.h>
#include <vlc_configuration.h>
#include <vlc_network.h>
#include <vlc_url.h>

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
# include <windows.h>
# include <winhttp.h>

/* Whether the host matches an entry of the bypass list, of len bytes: a '*'
 * stands for any text, anywhere, as in WinINet ("192.168.*", "*.corp",
 * "10.*.*.1") */
static bool HostMatches(const char *pat, size_t len, const char *host)
{
    if (len == 0)
        return *host == '\0';
    if (*pat == '*')
    {
        for (const char *h = host; ; h++)
        {
            if (HostMatches(pat + 1, len - 1, h))
                return true;
            if (*h == '\0')
                return false;
        }
    }
    return *host != '\0'
        && tolower((unsigned char)*pat) == tolower((unsigned char)*host)
        && HostMatches(pat + 1, len - 1, host + 1);
}

/* Whether the host of the URL is in the bypass list of Windows (entries
 * separated by ';' or spaces: a host with '*' wildcards, or "<local>" for the
 * names without a dot). As WinINet, the computer itself (localhost, 127.*,
 * ::1) never goes through the proxy, unless the list has "<-loopback>". */
static bool BypassesProxy(const char *bypass, const char *url)
{
    vlc_url_t parsed;
    if (vlc_UrlParse(&parsed, url) != 0 || parsed.psz_host == NULL)
    {
        vlc_UrlClean(&parsed);
        return false;
    }
    const char *host = parsed.psz_host;
    bool bypassed = (!strcasecmp(host, "localhost")
                  || !strcasecmp(host, "loopback")
                  || !strncmp(host, "127.", 4) || !strcmp(host, "::1"))
                 && vlc_strcasestr(bypass, "<-loopback>") == NULL;

    for (const char *p = bypass; *p != '\0' && !bypassed; )
    {
        size_t len = strcspn(p, "; \t");
        if (len == 7 && !strncasecmp(p, "<local>", 7))
            bypassed = strchr(host, '.') == NULL && strchr(host, ':') == NULL;
        else if (len > 0 && p[0] != '<')
            bypassed = HostMatches(p, len, host);
        p += len;
        p += strspn(p, "; \t");
    }
    vlc_UrlClean(&parsed);
    return bypassed;
}

/* The proxy of Windows' settings (Settings > Network > Proxy, manual setup),
 * for the scheme of the URL: "proxy:8080", or "http=a:1;https=b:2" with an
 * entry per scheme; NULL for none. An automatic setup script (PAC) is not
 * followed. */
static char *SystemProxy(const char *url)
{
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG config;
    if (!WinHttpGetIEProxyConfigForCurrentUser(&config))
        return NULL;

    char *proxy = NULL;
    char *list = config.lpszProxy ? FromWide(config.lpszProxy) : NULL;
    char *bypass = config.lpszProxyBypass ? FromWide(config.lpszProxyBypass) : NULL;

    if (list != NULL && !BypassesProxy(bypass ? bypass : "", url))
    {
        const char *scheme = strncasecmp(url, "https:", 6) ? "http=" : "https=";
        const char *entry = list;
        if (strchr(list, '=') != NULL)
        {   /* entries per scheme: this one's, else http's */
            entry = strstr(list, scheme);
            if (entry == NULL)
                entry = strstr(list, "http=");
            if (entry != NULL)
                entry = strchr(entry, '=') + 1;
        }
        if (entry != NULL)
        {
            size_t len = strcspn(entry, "; ");
            /* the scheme of this entry only, not of the ones after it */
            bool has_scheme = false;
            for (size_t i = 0; i + 3 <= len && !has_scheme; i++)
                has_scheme = !strncmp(entry + i, "://", 3);
            if (len > 0 && asprintf(&proxy, "%s%.*s",
                                    has_scheme ? "" : "http://",
                                    (int)len, entry) < 0)
                proxy = NULL;
        }
    }

    free(list);
    free(bypass);
    if (config.lpszProxy)
        GlobalFree(config.lpszProxy);
    if (config.lpszProxyBypass)
        GlobalFree(config.lpszProxyBypass);
    if (config.lpszAutoConfigUrl)
        GlobalFree(config.lpszAutoConfigUrl);
    return proxy;
}
#endif

char *vlc_getProxyUrl(const char *psz_url)
{
    char *proxy = config_GetPsz( "http-proxy" );
    if (proxy == NULL || *proxy == '\0')
    {
        /* no proxy of VLC's own: the one of Windows */
        free(proxy);
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        return psz_url != NULL ? SystemProxy(psz_url) : NULL;
#else
        return NULL;
#endif
    }

    char *proxy_pwd = config_GetPsz( "http-proxy-pwd" );
    if (proxy_pwd == NULL)
        return proxy;

    vlc_url_t url;
    if (vlc_UrlParse(&url, proxy) < 0) {
        vlc_UrlClean(&url);
        free (proxy);
        free (proxy_pwd);
        return NULL;
    }

    if (url.psz_password == NULL )
        url.psz_password = proxy_pwd;

    char *proxy_url = vlc_uri_compose (&url);
    vlc_UrlClean (&url);

    free (proxy_pwd);
    free (proxy);
    return proxy_url;
}
