/*****************************************************************************
 * textdomain.c : Modules text domain management
 *****************************************************************************
 * Copyright (C) 2010 Rémi Denis-Courmont
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
#include <vlc_configuration.h>
#include "modules/modules.h"

#ifdef ENABLE_NLS
# include <libintl.h>
# include <vlc_charset.h>
#endif

int vlc_bindtextdomain (const char *domain)
{
#if defined (ENABLE_NLS)
    /* Specify where to find the locales for current domain */
    char *upath = config_GetSysPath(VLC_LOCALE_DIR, NULL);
    if (unlikely(upath == NULL))
        return -1;

    char *lpath = ToLocale(upath);
    if (lpath == NULL || bindtextdomain (domain, lpath) == NULL)
    {
        LocaleFree(lpath);
        fprintf (stderr, "%s: text domain not found in %s\n", domain, upath);
        free (upath);
        return -1;
    }
    LocaleFree(lpath);
    free (upath);

    /* LibVLC wants all messages in UTF-8.
     * Unfortunately, we cannot ask UTF-8 for strerror_r(), strsignal_r()
     * and other functions that are not part of our text domain.
     */
    if (bind_textdomain_codeset (PACKAGE_NAME, "UTF-8") == NULL)
    {
        fprintf (stderr, "%s: UTF-8 encoding not available\n", domain);
        // Unbinds the text domain to avoid broken encoding
        bindtextdomain (PACKAGE_NAME, "/DOES_NOT_EXIST");
        return -1;
    }

    /* LibVLC does NOT set the default textdomain, since it is a library.
     * This could otherwise break programs using LibVLC (other than VLC).
     * textdomain (PACKAGE_NAME);
     */

#else /* !ENABLE_NLS */
    (void)domain;
#endif

    return 0;
}

/**
 * In-tree plugins share their gettext domain with LibVLC.
 */
const char *vlc_gettext(const char *msgid)
{
#ifdef ENABLE_NLS
    if (likely(*msgid))
        return dgettext (PACKAGE_NAME, msgid);
#endif
    return msgid;
}

void vlc_settextlanguage(const char *lang)
{
#ifdef ENABLE_NLS
    /* gettext reads LANGUAGE at each lookup, before the locale VLC started
     * with (LANG), and keeps what it found by language: the next messages
     * are in the new one. Empty, it is the locale's. */
    if (lang == NULL)
        lang = "";
# ifdef _WIN32
    char *env;
    if (asprintf(&env, "LANGUAGE=%s", lang) >= 0)
    {
        _putenv(env); /* copies it */
        free(env);
    }
# else
    /* Empty rather than unset (gettext takes an empty LANGUAGE as none):
     * once it is there, a change only replaces its value, and the
     * environment's array is not moved under another thread's getenv() */
    setenv("LANGUAGE", lang, 1);
# endif
    /* What gettext found is kept by message, not by language (libintl,
     * without the locale of each thread): Arabic then British English
     * showed the Arabic texts already looked up. Told the catalogs changed,
     * it looks them up again. Setting the text domain tells it (it bumps
     * _nl_msg_cat_cntr, the counter the gettext manual gives, which only
     * glibc and GNU libintl have, and links nowhere else); setting the one
     * it has changes nothing else. */
    textdomain(textdomain(NULL));
#else
    (void)lang;
#endif
}

const char *vlc_ngettext(const char *msgid, const char *plural,
                         unsigned long n)
{
#ifdef ENABLE_NLS
    if (likely(*msgid))
        return dngettext (PACKAGE_NAME, msgid, plural, n);
#endif
    return ((n == 1) ? msgid : plural);
}
