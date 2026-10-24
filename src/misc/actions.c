/*****************************************************************************
 * actions.c: handle vlc actions
 *****************************************************************************
 * Copyright (C) 2003-2016 VLC authors and VideoLAN
 *
 * Authors: Sigmund Augdal Helberg <dnumgis@videolan.org>
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
# include <config.h>
#endif

/**
 * \file
 * This file defines functions and structures for hotkey handling in vlc
 */

#include <assert.h>
#include <stdlib.h>
#include <limits.h>
#ifdef HAVE_SEARCH_H
# include <search.h>
#endif
#include <errno.h>

#include <vlc_common.h>
#include <vlc_actions.h>
#include <vlc_charset.h>
#include "../libvlc.h"

/* A key name, translated in the context "key" (vlc_keycode2str): the Home
 * key is not the Home page, nor the Delete key the Delete command. */
#define NC_(ctx, str) str

static const struct key_descriptor
{
    const char psz[20];
    uint32_t i_code;
} s_keys[] =
{   /* Alphabetical order */
    { NC_("key", "Backspace"),         KEY_BACKSPACE         },
    { NC_("key", "Brightness Down"),   KEY_BRIGHTNESS_DOWN   },
    { NC_("key", "Brightness Up"),     KEY_BRIGHTNESS_UP     },
    { NC_("key", "Browser Back"),      KEY_BROWSER_BACK      },
    { NC_("key", "Browser Favorites"), KEY_BROWSER_FAVORITES },
    { NC_("key", "Browser Forward"),   KEY_BROWSER_FORWARD   },
    { NC_("key", "Browser Home"),      KEY_BROWSER_HOME      },
    { NC_("key", "Browser Refresh"),   KEY_BROWSER_REFRESH   },
    { NC_("key", "Browser Search"),    KEY_BROWSER_SEARCH    },
    { NC_("key", "Browser Stop"),      KEY_BROWSER_STOP      },
    { NC_("key", "Delete"),            KEY_DELETE            },
    { NC_("key", "Down"),              KEY_DOWN              },
    { NC_("key", "End"),               KEY_END               },
    { NC_("key", "Enter"),             KEY_ENTER             },
    { NC_("key", "Esc"),               KEY_ESC               },
    { NC_("key", "F1"),                KEY_F1                },
    { NC_("key", "F10"),               KEY_F10               },
    { NC_("key", "F11"),               KEY_F11               },
    { NC_("key", "F12"),               KEY_F12               },
    { NC_("key", "F13"),               KEY_F(13)             },
    { NC_("key", "F14"),               KEY_F(14)             },
    { NC_("key", "F15"),               KEY_F(15)             },
    { NC_("key", "F16"),               KEY_F(16)             },
    { NC_("key", "F17"),               KEY_F(17)             },
    { NC_("key", "F18"),               KEY_F(18)             },
    { NC_("key", "F19"),               KEY_F(19)             },
    { NC_("key", "F2"),                KEY_F2                },
    { NC_("key", "F20"),               KEY_F(20)             },
    { NC_("key", "F21"),               KEY_F(21)             },
    { NC_("key", "F22"),               KEY_F(22)             },
    { NC_("key", "F23"),               KEY_F(23)             },
    { NC_("key", "F24"),               KEY_F(24)             },
    { NC_("key", "F25"),               KEY_F(25)             },
    { NC_("key", "F26"),               KEY_F(26)             },
    { NC_("key", "F27"),               KEY_F(27)             },
    { NC_("key", "F28"),               KEY_F(28)             },
    { NC_("key", "F29"),               KEY_F(29)             },
    { NC_("key", "F3"),                KEY_F3                },
    { NC_("key", "F30"),               KEY_F(30)             },
    { NC_("key", "F31"),               KEY_F(31)             },
    { NC_("key", "F32"),               KEY_F(32)             },
    { NC_("key", "F33"),               KEY_F(33)             },
    { NC_("key", "F34"),               KEY_F(34)             },
    { NC_("key", "F35"),               KEY_F(35)             },
    { NC_("key", "F4"),                KEY_F4                },
    { NC_("key", "F5"),                KEY_F5                },
    { NC_("key", "F6"),                KEY_F6                },
    { NC_("key", "F7"),                KEY_F7                },
    { NC_("key", "F8"),                KEY_F8                },
    { NC_("key", "F9"),                KEY_F9                },
    { NC_("key", "Home"),              KEY_HOME              },
    { NC_("key", "Insert"),            KEY_INSERT            },
    { NC_("key", "Left"),              KEY_LEFT              },
    { NC_("key", "Media Angle"),       KEY_MEDIA_ANGLE       },
    { NC_("key", "Media Audio Track"), KEY_MEDIA_AUDIO       },
    { NC_("key", "Media Forward"),     KEY_MEDIA_FORWARD     },
    { NC_("key", "Media Menu"),        KEY_MEDIA_MENU        },
    { NC_("key", "Media Next Frame"),  KEY_MEDIA_FRAME_NEXT  },
    { NC_("key", "Media Next Track"),  KEY_MEDIA_NEXT_TRACK  },
    { NC_("key", "Media Play Pause"),  KEY_MEDIA_PLAY_PAUSE  },
    { NC_("key", "Media Prev Frame"),  KEY_MEDIA_FRAME_PREV  },
    { NC_("key", "Media Prev Track"),  KEY_MEDIA_PREV_TRACK  },
    { NC_("key", "Media Record"),      KEY_MEDIA_RECORD      },
    { NC_("key", "Media Repeat"),      KEY_MEDIA_REPEAT      },
    { NC_("key", "Media Rewind"),      KEY_MEDIA_REWIND      },
    { NC_("key", "Media Select"),      KEY_MEDIA_SELECT      },
    { NC_("key", "Media Shuffle"),     KEY_MEDIA_SHUFFLE     },
    { NC_("key", "Media Stop"),        KEY_MEDIA_STOP        },
    { NC_("key", "Media Subtitle"),    KEY_MEDIA_SUBTITLE    },
    { NC_("key", "Media Time"),        KEY_MEDIA_TIME        },
    { NC_("key", "Media View"),        KEY_MEDIA_VIEW        },
    { NC_("key", "Menu"),              KEY_MENU              },
    { NC_("key", "Mouse Wheel Down"),  KEY_MOUSEWHEELDOWN    },
    { NC_("key", "Mouse Wheel Left"),  KEY_MOUSEWHEELLEFT    },
    { NC_("key", "Mouse Wheel Right"), KEY_MOUSEWHEELRIGHT   },
    { NC_("key", "Mouse Wheel Up"),    KEY_MOUSEWHEELUP      },
    { NC_("key", "Page Down"),         KEY_PAGEDOWN          },
    { NC_("key", "Page Up"),           KEY_PAGEUP            },
    { NC_("key", "Pause"),             KEY_PAUSE             },
    { NC_("key", "Print"),             KEY_PRINT             },
    { NC_("key", "Right"),             KEY_RIGHT             },
    { NC_("key", "Space"),             ' '                   },
    { NC_("key", "Tab"),               KEY_TAB               },
    { NC_("key", "Unset"),             KEY_UNSET             },
    { NC_("key", "Up"),                KEY_UP                },
    { NC_("key", "Volume Down"),       KEY_VOLUME_DOWN       },
    { NC_("key", "Volume Mute"),       KEY_VOLUME_MUTE       },
    { NC_("key", "Volume Up"),         KEY_VOLUME_UP         },
    { NC_("key", "Zoom In"),           KEY_ZOOM_IN           },
    { NC_("key", "Zoom Out"),          KEY_ZOOM_OUT          },
};

static int keystrcmp (const void *key, const void *elem)
{
    const char *sa = key, *sb = elem;
    return strcmp (sa, sb);
}

/* Convert Unicode code point to UTF-8 */
static char *utf8_cp (uint_fast32_t cp, char *buf)
{
    if (cp < (1 << 7))
    {
        buf[1] = 0;
        buf[0] = cp;
    }
    else if (cp < (1 << 11))
    {
        buf[2] = 0;
        buf[1] = 0x80 | (cp & 0x3F);
        cp >>= 6;
        buf[0] = 0xC0 | cp;
    }
    else if (cp < (1 << 16))
    {
        buf[3] = 0;
        buf[2] = 0x80 | (cp & 0x3F);
        cp >>= 6;
        buf[1] = 0x80 | (cp & 0x3F);
        cp >>= 6;
        buf[0] = 0xE0 | cp;
    }
    else if (cp < (1 << 21))
    {
        buf[4] = 0;
        buf[3] = 0x80 | (cp & 0x3F);
        cp >>= 6;
        buf[2] = 0x80 | (cp & 0x3F);
        cp >>= 6;
        buf[1] = 0x80 | (cp & 0x3F);
        cp >>= 6;
        buf[0] = 0xE0 | cp;
    }
    else
        return NULL;
    return buf;
}

/**
 * Parse a human-readable string representation of a VLC key code.
 * @note This only works with the American English representation
 * (a.k.a. C or POSIX), not with the local representation returned from
 * vlc_keycode2str().
 * @return a VLC key code, or KEY_UNSET on failure.
 */
uint_fast32_t vlc_str2keycode (const char *name)
{
    uint_fast32_t mods = 0;
    uint32_t code;

    for (;;)
    {
        size_t len = strcspn (name, "-+");
        if (len == 0 || name[len] == '\0')
            break;

        if (len == 4 && !strncasecmp (name, "Ctrl", 4))
            mods |= KEY_MODIFIER_CTRL;
        if (len == 3 && !strncasecmp (name, "Alt", 3))
            mods |= KEY_MODIFIER_ALT;
        if (len == 5 && !strncasecmp (name, "Shift", 5))
            mods |= KEY_MODIFIER_SHIFT;
        if (len == 4 && !strncasecmp (name, "Meta", 4))
            mods |= KEY_MODIFIER_META;
        if (len == 7 && !strncasecmp (name, "Command", 7))
            mods |= KEY_MODIFIER_COMMAND;

        name += len + 1;
    }

    const struct key_descriptor *d = bsearch(name, s_keys, ARRAY_SIZE(s_keys),
                                             sizeof (s_keys[0]), keystrcmp);
    if (d != NULL)
        code = d->i_code;
    else if (vlc_towc (name, &code) <= 0)
        return KEY_UNSET;

    if (code != KEY_UNSET)
        code |= mods;
    return code;
}

static const char *nooptext (const char *txt)
{
    return txt;
}

/* The name of a key in the language of the interface: "key\004Home" */
static const char *keytext (const char *name)
{
    char msgid[sizeof ("key\004") + sizeof (s_keys[0].psz)];
    const char *tr;

    if (snprintf (msgid, sizeof (msgid), "key\004%s", name) >= (int) sizeof (msgid))
        return name;
    tr = vlc_gettext (msgid);
    return (tr == msgid) ? name : tr;
}

/**
 * Format a human-readable and unique representation of a VLC key code
 * (including modifiers).
 * @param code key code to translate to a string
 * @param locale true to get a localized string,
 *               false to get a C string suitable for 'vlcrc'
 * @return a heap-allocated string, or NULL on error.
 */
char *vlc_keycode2str (uint_fast32_t code, bool locale)
{
    const char *(*tr)(const char *) = locale ? vlc_gettext : nooptext;
    const char *(*trkey)(const char *) = locale ? keytext : nooptext;
    const char *name;
    char *str, buf[5];
    uintptr_t key = code & ~KEY_MODIFIER;

    for (size_t i = 0; i < ARRAY_SIZE(s_keys); i++)
        if (s_keys[i].i_code == key)
        {
            name = s_keys[i].psz;
            goto found;
        }

    if (utf8_cp (key, buf) == NULL)
        return NULL;
    name = buf;

found:
    if (asprintf (&str, "%s%s%s%s%s%s",
                  (code & KEY_MODIFIER_CTRL) ? tr(N_("Ctrl+")) : "",
                  (code & KEY_MODIFIER_ALT) ? tr(N_("Alt+")) : "",
                  (code & KEY_MODIFIER_SHIFT) ? tr(N_("Shift+")) : "",
                  (code & KEY_MODIFIER_META) ? tr(N_("Meta+")) : "",
                  (code & KEY_MODIFIER_COMMAND) ? tr(N_("Command+")) : "",
                  trkey(name)) == -1)
        return NULL;
    return str;
}


/*** VLC key map ***/

#define MAXACTION 27
static const struct name2action
{
    char psz[MAXACTION];
    vlc_action_id_t id;
} s_names2actions[] =
{
    /* *MUST* be sorted (ASCII order) */
    { "aspect-ratio", ACTIONID_ASPECT_RATIO, },
    { "audio-track", ACTIONID_AUDIO_TRACK, },
    { "audiodelay-down", ACTIONID_AUDIODELAY_DOWN, },
    { "audiodelay-up", ACTIONID_AUDIODELAY_UP, },
    { "audiodevice-cycle", ACTIONID_AUDIODEVICE_CYCLE, },
    { "chapter-next", ACTIONID_CHAPTER_NEXT, },
    { "chapter-prev", ACTIONID_CHAPTER_PREV, },
    { "clear-playlist", ACTIONID_PLAY_CLEAR, },
    { "crop", ACTIONID_CROP, },
    { "crop-bottom", ACTIONID_CROP_BOTTOM, },
    { "crop-left", ACTIONID_CROP_LEFT, },
    { "crop-right", ACTIONID_CROP_RIGHT, },
    { "crop-top", ACTIONID_CROP_TOP, },
    { "decr-scalefactor", ACTIONID_SCALE_DOWN, },
    { "deinterlace", ACTIONID_DEINTERLACE, },
    { "deinterlace-mode", ACTIONID_DEINTERLACE_MODE, },
    { "disc-menu", ACTIONID_DISC_MENU, },
    { "faster", ACTIONID_RATE_FASTER, },
    { "frame-next", ACTIONID_FRAME_NEXT, },
    { "frame-previous", ACTIONID_FRAME_PREVIOUS, },
    { "incr-scalefactor", ACTIONID_SCALE_UP, },
    { "intf-boss", ACTIONID_INTF_BOSS, },
    { "intf-popup-menu", ACTIONID_INTF_POPUP_MENU, },
    { "intf-show", ACTIONID_INTF_TOGGLE_FSC, },
    { "jump+extrashort", ACTIONID_JUMP_FORWARD_EXTRASHORT, },
    { "jump+long", ACTIONID_JUMP_FORWARD_LONG, },
    { "jump+medium", ACTIONID_JUMP_FORWARD_MEDIUM, },
    { "jump+short", ACTIONID_JUMP_FORWARD_SHORT, },
    { "jump-extrashort", ACTIONID_JUMP_BACKWARD_EXTRASHORT, },
    { "jump-long", ACTIONID_JUMP_BACKWARD_LONG, },
    { "jump-medium", ACTIONID_JUMP_BACKWARD_MEDIUM, },
    { "jump-short", ACTIONID_JUMP_BACKWARD_SHORT, },
    { "leave-fullscreen", ACTIONID_LEAVE_FULLSCREEN, },
    { "loop", ACTIONID_LOOP, },
    { "nav-activate", ACTIONID_NAV_ACTIVATE, },
    { "nav-down", ACTIONID_NAV_DOWN, },
    { "nav-left", ACTIONID_NAV_LEFT, },
    { "nav-right", ACTIONID_NAV_RIGHT, },
    { "nav-up", ACTIONID_NAV_UP, },
    { "next", ACTIONID_NEXT, },
    { "pause", ACTIONID_PAUSE, },
    { "play", ACTIONID_PLAY, },
    { "play-bookmark1", ACTIONID_PLAY_BOOKMARK1, },
    { "play-bookmark10", ACTIONID_PLAY_BOOKMARK10, },
    { "play-bookmark2", ACTIONID_PLAY_BOOKMARK2, },
    { "play-bookmark3", ACTIONID_PLAY_BOOKMARK3, },
    { "play-bookmark4", ACTIONID_PLAY_BOOKMARK4, },
    { "play-bookmark5", ACTIONID_PLAY_BOOKMARK5, },
    { "play-bookmark6", ACTIONID_PLAY_BOOKMARK6, },
    { "play-bookmark7", ACTIONID_PLAY_BOOKMARK7, },
    { "play-bookmark8", ACTIONID_PLAY_BOOKMARK8, },
    { "play-bookmark9", ACTIONID_PLAY_BOOKMARK9, },
    { "play-pause", ACTIONID_PLAY_PAUSE, },
    { "position", ACTIONID_POSITION, },
    { "prev", ACTIONID_PREV, },
    { "program-sid-next", ACTIONID_PROGRAM_SID_NEXT, },
    { "program-sid-prev", ACTIONID_PROGRAM_SID_PREV, },
    { "projection-toggle", ACTIONID_PROJECTION_TOGGLE, },
    { "quit", ACTIONID_QUIT, },
    { "random", ACTIONID_RANDOM, },
    { "rate-faster-fine", ACTIONID_RATE_FASTER_FINE, },
    { "rate-normal", ACTIONID_RATE_NORMAL, },
    { "rate-slower-fine", ACTIONID_RATE_SLOWER_FINE, },
    { "record", ACTIONID_RECORD, },
    { "set-bookmark1", ACTIONID_SET_BOOKMARK1, },
    { "set-bookmark10", ACTIONID_SET_BOOKMARK10, },
    { "set-bookmark2", ACTIONID_SET_BOOKMARK2, },
    { "set-bookmark3", ACTIONID_SET_BOOKMARK3, },
    { "set-bookmark4", ACTIONID_SET_BOOKMARK4, },
    { "set-bookmark5", ACTIONID_SET_BOOKMARK5, },
    { "set-bookmark6", ACTIONID_SET_BOOKMARK6, },
    { "set-bookmark7", ACTIONID_SET_BOOKMARK7, },
    { "set-bookmark8", ACTIONID_SET_BOOKMARK8, },
    { "set-bookmark9", ACTIONID_SET_BOOKMARK9, },
    { "slower", ACTIONID_RATE_SLOWER, },
    { "snapshot", ACTIONID_SNAPSHOT, },
    { "stop", ACTIONID_STOP, },
    { "subdelay-down", ACTIONID_SUBDELAY_DOWN, },
    { "subdelay-up", ACTIONID_SUBDELAY_UP, },
    { "subpos-down", ACTIONID_SUBPOS_DOWN, },
    { "subpos-up", ACTIONID_SUBPOS_UP, },
    { "subsync-apply", ACTIONID_SUBSYNC_APPLY, },
    { "subsync-markaudio", ACTIONID_SUBSYNC_MARKAUDIO, },
    { "subsync-marksub", ACTIONID_SUBSYNC_MARKSUB, },
    { "subsync-reset", ACTIONID_SUBSYNC_RESET, },
    { "subtitle-control-secondary", ACTIONID_SUBTITLE_CONTROL_SECONDARY, },
    { "subtitle-revtrack", ACTIONID_SUBTITLE_REVERSE_TRACK, },
    { "subtitle-text-scale-down", ACTIONID_SUBTITLE_TEXT_SCALE_DOWN, },
    { "subtitle-text-scale-normal", ACTIONID_SUBTITLE_TEXT_SCALE_NORMAL, },
    { "subtitle-text-scale-up", ACTIONID_SUBTITLE_TEXT_SCALE_UP, },
    { "subtitle-toggle", ACTIONID_SUBTITLE_TOGGLE, },
    { "subtitle-track", ACTIONID_SUBTITLE_TRACK, },
    { "title-next", ACTIONID_TITLE_NEXT, },
    { "title-prev", ACTIONID_TITLE_PREV, },
    { "toggle-autoscale", ACTIONID_TOGGLE_AUTOSCALE, },
    { "toggle-fullscreen", ACTIONID_TOGGLE_FULLSCREEN, },
    { "uncrop-bottom", ACTIONID_UNCROP_BOTTOM, },
    { "uncrop-left", ACTIONID_UNCROP_LEFT, },
    { "uncrop-right", ACTIONID_UNCROP_RIGHT, },
    { "uncrop-top", ACTIONID_UNCROP_TOP, },
    { "unzoom", ACTIONID_UNZOOM, },
    { "viewpoint-fov-in", ACTIONID_VIEWPOINT_FOV_IN, },
    { "viewpoint-fov-out", ACTIONID_VIEWPOINT_FOV_OUT, },
    { "viewpoint-roll-anticlock", ACTIONID_VIEWPOINT_ROLL_ANTICLOCK, },
    { "viewpoint-roll-clock", ACTIONID_VIEWPOINT_ROLL_CLOCK, },
    { "vol-down", ACTIONID_VOL_DOWN, },
    { "vol-mute", ACTIONID_VOL_MUTE, },
    { "vol-up", ACTIONID_VOL_UP, },
    { "wallpaper", ACTIONID_WALLPAPER, },
    { "zoom", ACTIONID_ZOOM, },
    { "zoom-double", ACTIONID_ZOOM_DOUBLE, },
    { "zoom-half", ACTIONID_ZOOM_HALF, },
    { "zoom-original", ACTIONID_ZOOM_ORIGINAL, },
    { "zoom-quarter", ACTIONID_ZOOM_QUARTER, },
};

struct mapping
{
    uint32_t     key; ///< Key code
    vlc_action_id_t action; ///< Action ID
};

static int keycmp (const void *a, const void *b)
{
    const struct mapping *ka = a, *kb = b;

    return (ka->key < kb->key) ? -1 : (ka->key > kb->key) ? +1 : 0;
}

struct vlc_actions_t
{
    vlc_mutex_t lock; /* the maps, made again when the bindings change */
    void *map; /* Key map */
    void *global_map; /* Grabbed/global key map */
    const char *ppsz_keys[];
};

static int vlc_key_to_action_in (vlc_object_t *obj, vlc_actions_t *as,
                                 void *const *map, uint32_t keycode)
{
    vlc_action_id_t action = ACTIONID_NONE;

    vlc_mutex_lock (&as->lock);
    const void **pent = tfind (&keycode, map, keycmp);
    if (pent != NULL)
    {
        const struct mapping *ent = *pent;
        action = ent->action;
    }
    vlc_mutex_unlock (&as->lock);

    /* (the action is done unlocked: it may take as long as it takes) */
    if (action == ACTIONID_NONE)
        return VLC_SUCCESS;
    return var_SetInteger (obj, "key-action", action);
}

static int vlc_key_to_action (vlc_object_t *obj, const char *varname,
                              vlc_value_t prevkey, vlc_value_t curkey, void *d)
{
    vlc_actions_t *as = d;

    (void) varname;
    (void) prevkey;
    return vlc_key_to_action_in (obj, as, &as->map, curkey.i_int);
}

static int vlc_global_key_to_action (vlc_object_t *obj, const char *varname,
                                     vlc_value_t prevkey, vlc_value_t curkey,
                                     void *d)
{
    vlc_actions_t *as = d;

    (void) varname;
    (void) prevkey;
    return vlc_key_to_action_in (obj, as, &as->global_map, curkey.i_int);
}

/**
 * Adds a mapping from a certain key code to a certain action.
 */
static int add_mapping (void **map, uint32_t keycode, vlc_action_id_t action)
{
    struct mapping *entry = malloc (sizeof (*entry));
    if (entry == NULL)
        return ENOMEM;
    entry->key = keycode;
    entry->action = action;

    void **pent = tsearch (entry, map, keycmp);
    if (unlikely(pent == NULL))
        return ENOMEM;
    if (*pent != entry)
    {
        free (entry);
        return EEXIST;
    }
    return 0;
}

static void add_wheel_mapping (void **map, uint32_t kmore, uint32_t kless,
                                 int mode)
{
    vlc_action_id_t amore = ACTIONID_NONE, aless = ACTIONID_NONE;

    switch (mode)
    {
        case 0: /* volume up/down */
            amore = ACTIONID_COMBO_VOL_FOV_UP;
            aless = ACTIONID_COMBO_VOL_FOV_DOWN;
            break;
        case 2: /* position latter/earlier */
            amore = ACTIONID_JUMP_FORWARD_EXTRASHORT;
            aless = ACTIONID_JUMP_BACKWARD_EXTRASHORT;
            break;
        case 3: /* position earlier/latter */
            amore = ACTIONID_JUMP_BACKWARD_EXTRASHORT;
            aless = ACTIONID_JUMP_FORWARD_EXTRASHORT;
            break;
    }

    if (amore != ACTIONID_NONE)
        add_mapping (map, kmore, amore);
    if (aless != ACTIONID_NONE)
        add_mapping (map, kless, aless);
}


/**
 * Sets up all key mappings for a given action.
 *
 * \param obj the VLC object to inherit keys from and log to
 * \param map tree (of struct mapping entries) to write mappings to
 * \param confname VLC configuration item to read mappings from
 * \param action action ID
 */
static void init_action (vlc_object_t *obj, void **map,
                            const char *confname, vlc_action_id_t action)
{
    char *keys = var_InheritString (obj, confname);
    if (keys == NULL)
        return;

    for (char *buf, *key = strtok_r (keys, "\t", &buf);
         key != NULL;
         key = strtok_r (NULL, "\t", &buf))
    {
        uint32_t code = vlc_str2keycode (key);
        if (code == KEY_UNSET)
        {
            msg_Warn (obj, "Key \"%s\" unrecognized", key);
            continue;
        }

        if (add_mapping (map, code, action) == EEXIST)
            msg_Warn (obj, "Key \"%s\" bound to multiple actions", key);
    }
    free (keys);
}

#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)

/**
 * Sets up the key maps (hotkeys, global hotkeys, mouse wheels) from the
 * configuration.
 */
static void load_actions (vlc_object_t *obj, void **map, void **global_map)
{
    for (size_t i = 0; i < ARRAY_SIZE(s_names2actions); i++)
    {
        char name[12 + MAXACTION];

        snprintf (name, sizeof (name), "global-key-%." STRINGIFY(MAXACTION) "s", s_names2actions[i].psz);
        init_action (obj, map, name + 7, s_names2actions[i].id);
        init_action (obj, global_map, name, s_names2actions[i].id);
    }

    /* Initialize mouse wheel events */
    add_wheel_mapping (map, KEY_MOUSEWHEELRIGHT, KEY_MOUSEWHEELLEFT,
                         var_InheritInteger (obj, "hotkeys-x-wheel-mode"));
    add_wheel_mapping (map, KEY_MOUSEWHEELUP, KEY_MOUSEWHEELDOWN,
                         var_InheritInteger (obj, "hotkeys-y-wheel-mode"));
}

/**
 * Initializes the key map from configuration.
 */
int libvlc_InternalActionsInit (libvlc_int_t *libvlc)
{
    assert(libvlc != NULL);

    vlc_object_t *obj = VLC_OBJECT(libvlc);
    vlc_actions_t *as = malloc (sizeof (*as) + (1 + ARRAY_SIZE(s_names2actions))
                      * sizeof (*as->ppsz_keys));

    if (unlikely(as == NULL))
        return VLC_ENOMEM;
    vlc_mutex_init (&as->lock);
    as->map = NULL;
    as->global_map = NULL;

    var_Create (obj, "key-pressed", VLC_VAR_INTEGER);
    var_Create (obj, "global-key-pressed", VLC_VAR_INTEGER);
    var_Create (obj, "key-action", VLC_VAR_INTEGER);

    for (size_t i = 0; i < ARRAY_SIZE(s_names2actions); i++)
    {
#ifndef NDEBUG
        if (i > 0
         && strcmp (s_names2actions[i-1].psz, s_names2actions[i].psz) >= 0)
        {
            msg_Err (libvlc, "key-%s and key-%s are not ordered properly",
                     s_names2actions[i-1].psz, s_names2actions[i].psz);
            abort ();
        }
#endif
        as->ppsz_keys[i] = s_names2actions[i].psz;
    }
    as->ppsz_keys[ARRAY_SIZE(s_names2actions)] = NULL;

    /* Initialize from configuration */
    load_actions (obj, &as->map, &as->global_map);

    libvlc_priv(libvlc)->actions = as;
    var_AddCallback (obj, "key-pressed", vlc_key_to_action, as);
    var_AddCallback (obj, "global-key-pressed", vlc_global_key_to_action, as);
    return VLC_SUCCESS;
}

#undef vlc_actions_reload
void vlc_actions_reload (vlc_object_t *p_obj)
{
    libvlc_int_t *libvlc = vlc_object_instance(p_obj);
    vlc_actions_t *as = libvlc_priv(libvlc)->actions;
    void *map = NULL, *global_map = NULL;

    if (unlikely(as == NULL))
        return;

    /* made apart, then put in place of the old ones at once: a key pressed
     * meanwhile finds one or the other */
    load_actions (VLC_OBJECT(libvlc), &map, &global_map);

    vlc_mutex_lock (&as->lock);
    void *old_map = as->map, *old_global_map = as->global_map;
    as->map = map;
    as->global_map = global_map;
    vlc_mutex_unlock (&as->lock);

    tdestroy (old_global_map, free);
    tdestroy (old_map, free);
}

/**
 * Destroys the key map.
 */
void libvlc_InternalActionsClean (libvlc_int_t *libvlc)
{
    assert(libvlc != NULL);

    vlc_actions_t *as = libvlc_priv(libvlc)->actions;
    if (unlikely(as == NULL))
        return;

    var_DelCallback (libvlc, "global-key-pressed", vlc_global_key_to_action,
                     as);
    var_DelCallback (libvlc, "key-pressed", vlc_key_to_action, as);

    tdestroy (as->global_map, free);
    tdestroy (as->map, free);
    free (as);
    libvlc_priv(libvlc)->actions = NULL;
}


static int actcmp(const void *key, const void *ent)
{
    const struct name2action *act = ent;
    return strcmp(key, act->psz);
}

/**
 * Get the action ID from the action name in the configuration subsystem.
 * @return the action ID or ACTIONID_NONE on error.
 */
vlc_action_id_t
vlc_actions_get_id (const char *name)
{
    const struct name2action *act;

    if (strncmp (name, "key-", 4))
        return ACTIONID_NONE;
    name += 4;

    act = bsearch(name, s_names2actions, ARRAY_SIZE(s_names2actions),
                  sizeof(*act), actcmp);
    return (act != NULL) ? act->id : ACTIONID_NONE;
}

#undef vlc_actions_get_keycodes
size_t
vlc_actions_get_keycodes(vlc_object_t *p_obj, const char *psz_key_name,
                        bool b_global, uint_fast32_t **pp_keycodes)
{
    assert(strlen( psz_key_name ) <= MAXACTION);
    char varname[12 /* "global-key-" */ + MAXACTION];
    snprintf( varname, ARRAY_SIZE(varname), "%skey-%s", b_global ? "global-" : "", psz_key_name );

    *pp_keycodes = NULL;

    char *psz_keys = var_InheritString( p_obj, varname );
    if( psz_keys == NULL )
        return 0;

    size_t i_nb_keycodes = 0;
    for( const char* psz_it = psz_keys; *psz_it; ++psz_it )
    {
        if( *psz_it == '\t' )
            ++i_nb_keycodes;
    }
    ++i_nb_keycodes;
    *pp_keycodes = vlc_alloc( i_nb_keycodes, sizeof( **pp_keycodes ) );
    if( unlikely( !*pp_keycodes ) )
    {
        free( psz_keys );
        return 0;
    }
    size_t i = 0;
    for( char *buf, *key = strtok_r( psz_keys, "\t", &buf );
         key != NULL;
         key = strtok_r( NULL, "\t", &buf ), ++i )
    {
        (*pp_keycodes)[i] = vlc_str2keycode( key );
    }
    assert( i == i_nb_keycodes );
    free( psz_keys );
    return i_nb_keycodes;
}

#undef vlc_actions_get_key_names
const char* const*
vlc_actions_get_key_names(vlc_object_t *p_obj)
{
    vlc_actions_t *as = libvlc_priv(vlc_object_instance(p_obj))->actions;
    return as->ppsz_keys;
}
