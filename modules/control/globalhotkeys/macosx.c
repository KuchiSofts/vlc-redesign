/*****************************************************************************
 * macosx.c: Global-Hotkey handling for VLC on macOS
 *****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * After win32.c (Domani Hannes) and xcb.c: the keys of the "global-key-*"
 * options work while VLC is in the background, as on Windows and Linux.
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

#include <ctype.h>
#include <stdatomic.h>
#include <stdlib.h>

/* (the check() and verify() of <AssertMacros.h> would clash) */
#ifndef __ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES
# define __ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES 0
#endif
#include <Carbon/Carbon.h>
#include <dispatch/dispatch.h>
#include <pthread.h>
#include <stdbool.h>

#define VLC_MODULE_LICENSE VLC_LICENSE_GPL_2_PLUS
#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_interface.h>
#include <vlc_actions.h>

/*****************************************************************************
 * Local prototypes
 *****************************************************************************/
static int Open(vlc_object_t *);
static void Close(vlc_object_t *);

static void AutoRun(libvlc_int_t *libvlc)
{
    intf_Create(libvlc, MODULE_STRING);
}

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
vlc_module_begin()
    set_shortname(N_("Global Hotkeys"))
    set_subcategory(SUBCAT_INTERFACE_HOTKEYS)
    set_description(N_("Global Hotkeys interface"))
    set_capability("interface", 0)
    set_callbacks(Open, Close)
    add_shortcut("globalhotkeys")

    add_submodule()
    set_capability("autorun", 10)
    set_callback(AutoRun)
vlc_module_end()

/* The keys are registered with the system on the main thread (Carbon and
 * the keyboard layouts want it), where they are also pressed. What the main
 * thread uses lives here, apart from the interface: it goes on the main
 * thread once the interface has closed, the keys unregistered first. */
typedef struct
{
    EventHotKeyRef ref;
    vlc_action_id_t action;
} hotkey_t;

typedef struct
{
    pthread_mutex_t lock;
    bool closed;                /* the interface is gone: nothing reached */
    libvlc_int_t *libvlc;
    vlc_object_t *obj;          /* (for the log, while not closed) */
    UInt32 generation;          /* tells the keys of this one from others */

    EventHandlerRef handler;
    hotkey_t *keys;
    size_t count;

    /* what to register, read on the interface's thread */
    struct
    {
        uint_fast32_t key;
        vlc_action_id_t action;
    } *wanted;
    size_t wanted_count;
} hotkeys_t;

struct intf_sys_t
{
    hotkeys_t *hotkeys;
};

#define SIGNATURE 'VLCh'

static atomic_uint generations = 0;

/* The virtual key code of the key that types this character in a keyboard
 * layout, with the Shift it needs; false when no key types it */
static bool KeyInLayout(TISInputSourceRef source, UniChar wanted, UInt16 *code, bool *shift)
{
    bool found = false;
    if (source == NULL)
        return false;
    CFDataRef data = TISGetInputSourceProperty(source, kTISPropertyUnicodeKeyLayoutData);
    const UCKeyboardLayout *layout = data ? (const UCKeyboardLayout *)CFDataGetBytePtr(data) : NULL;
    if (layout != NULL)
    {
        const UInt32 states[] = { 0, shiftKey >> 8 };
        for (size_t s = 0; s < ARRAY_SIZE(states) && !found; s++)
            for (UInt16 vk = 0; vk < 128 && !found; vk++)
            {
                UInt32 dead = 0;
                UniChar chars[4];
                UniCharCount length = 0;
                if (UCKeyTranslate(layout, vk, kUCKeyActionDown, states[s], LMGetKbdType(),
                                   kUCKeyTranslateNoDeadKeysMask, &dead, ARRAY_SIZE(chars),
                                   &length, chars) == noErr
                 && length == 1 && chars[0] == wanted)
                {
                    *code = vk;
                    *shift = s > 0;
                    found = true;
                }
            }
    }
    CFRelease(source);
    return found;
}

/* In the layout of now, else in the Latin one the system keeps for the
 * shortcuts (a Hebrew or Russian layout types no "s") */
static bool KeyOfCharacter(UniChar wanted, UInt16 *code, bool *shift)
{
    return KeyInLayout(TISCopyCurrentKeyboardLayoutInputSource(), wanted, code, shift)
        || KeyInLayout(TISCopyCurrentASCIICapableKeyboardLayoutInputSource(), wanted, code, shift);
}

/* The virtual key code and the modifiers of macOS for a key of VLC */
static bool SystemKey(uint_fast32_t key, UInt32 *code, UInt32 *modifiers)
{
    UInt32 mods = 0;
    if (key & KEY_MODIFIER_SHIFT)   mods |= shiftKey;
    if (key & KEY_MODIFIER_ALT)     mods |= optionKey;
    if (key & KEY_MODIFIER_CTRL)    mods |= controlKey;
    if (key & (KEY_MODIFIER_COMMAND | KEY_MODIFIER_META)) mods |= cmdKey;

    static const struct { uint_fast32_t vlc; UInt16 vk; } keys[] = {
        { KEY_LEFT, kVK_LeftArrow }, { KEY_RIGHT, kVK_RightArrow },
        { KEY_UP, kVK_UpArrow }, { KEY_DOWN, kVK_DownArrow },
        { ' ', kVK_Space }, { KEY_ESC, kVK_Escape }, { KEY_ENTER, kVK_Return },
        { KEY_TAB, kVK_Tab }, { KEY_BACKSPACE, kVK_Delete },
        { KEY_DELETE, kVK_ForwardDelete }, { KEY_INSERT, kVK_Help },
        { KEY_HOME, kVK_Home }, { KEY_END, kVK_End },
        { KEY_PAGEUP, kVK_PageUp }, { KEY_PAGEDOWN, kVK_PageDown },
        { KEY_F1, kVK_F1 }, { KEY_F2, kVK_F2 }, { KEY_F3, kVK_F3 },
        { KEY_F4, kVK_F4 }, { KEY_F5, kVK_F5 }, { KEY_F6, kVK_F6 },
        { KEY_F7, kVK_F7 }, { KEY_F8, kVK_F8 }, { KEY_F9, kVK_F9 },
        { KEY_F10, kVK_F10 }, { KEY_F11, kVK_F11 }, { KEY_F12, kVK_F12 },
    };
    const uint_fast32_t bare = key & ~KEY_MODIFIER;
    for (size_t i = 0; i < ARRAY_SIZE(keys); i++)
        if (keys[i].vlc == bare)
        {
            *code = keys[i].vk;
            *modifiers = mods;
            return true;
        }

    /* a character (VLC writes letters in lower case): the key that types
     * it; the media and volume keys are the system's (Now Playing) */
    if (bare == 0 || bare > 0xFFFF)
        return false;
    UniChar wanted = (UniChar)bare;
    if (wanted < 0x80)
        wanted = (UniChar)tolower((int)wanted);
    UInt16 vk;
    bool shift;
    if (!KeyOfCharacter(wanted, &vk, &shift))
        return false;
    *code = vk;
    *modifiers = mods | (shift ? shiftKey : 0);
    return true;
}

static OSStatus Pressed(EventHandlerCallRef next, EventRef event, void *data)
{
    (void)next;
    hotkeys_t *hk = data;
    EventHotKeyID id;
    if (GetEventParameter(event, kEventParamDirectObject, typeEventHotKeyID, NULL,
                          sizeof(id), NULL, &id) != noErr
     || id.signature != SIGNATURE || (id.id >> 16) != hk->generation)
        return eventNotHandledErr;
    const size_t index = id.id & 0xFFFF;
    if (index >= hk->count)
        return eventNotHandledErr;

    pthread_mutex_lock(&hk->lock);
    if (!hk->closed)
        var_SetInteger(hk->libvlc, "key-action", hk->keys[index].action);
    pthread_mutex_unlock(&hk->lock);
    return noErr;
}

/* (on the main thread) */
static void Register(void *data)
{
    hotkeys_t *hk = data;
    pthread_mutex_lock(&hk->lock);
    const bool closed = hk->closed;
    pthread_mutex_unlock(&hk->lock);
    if (closed || hk->wanted_count == 0)
        return;

    hk->keys = calloc(hk->wanted_count, sizeof (*hk->keys));
    if (hk->keys == NULL)
        return;
    const EventTypeSpec type = { kEventClassKeyboard, kEventHotKeyPressed };
    if (InstallApplicationEventHandler(NewEventHandlerUPP(Pressed), 1, &type, hk,
                                       &hk->handler) != noErr)
    {
        hk->handler = NULL;
        return;
    }

    for (size_t i = 0; i < hk->wanted_count && i < 0xFFFF; i++)
    {
        UInt32 code, modifiers;
        if (!SystemKey(hk->wanted[i].key, &code, &modifiers))
            continue;
        const EventHotKeyID id = { SIGNATURE, (hk->generation << 16) | (UInt32)hk->count };
        EventHotKeyRef ref;
        if (RegisterEventHotKey(code, modifiers, id, GetApplicationEventTarget(), 0, &ref) != noErr)
            continue;
        hk->keys[hk->count].ref = ref;
        hk->keys[hk->count].action = hk->wanted[i].action;
        hk->count++;
    }

    pthread_mutex_lock(&hk->lock);
    if (!hk->closed)
        msg_Dbg(hk->obj, "%zu global hotkeys registered with macOS (of %zu)",
                hk->count, hk->wanted_count);
    pthread_mutex_unlock(&hk->lock);
}

/* (on the main thread, after Register) */
static void Unregister(void *data)
{
    hotkeys_t *hk = data;
    for (size_t i = 0; i < hk->count; i++)
        UnregisterEventHotKey(hk->keys[i].ref);
    if (hk->handler != NULL)
        RemoveEventHandler(hk->handler);
    free(hk->keys);
    free(hk->wanted);
    pthread_mutex_destroy(&hk->lock);
    free(hk);
}

/*****************************************************************************
 * Open: the keys of the settings, registered on the main thread
 *****************************************************************************/
static int Open(vlc_object_t *obj)
{
    intf_thread_t *intf = (intf_thread_t *)obj;
    intf_sys_t *sys = vlc_obj_malloc(obj, sizeof (*sys));
    if (sys == NULL)
        return VLC_ENOMEM;
    hotkeys_t *hk = calloc(1, sizeof (*hk));
    if (hk == NULL)
        return VLC_ENOMEM;
    pthread_mutex_init(&hk->lock, NULL);
    hk->libvlc = vlc_object_instance(intf);
    hk->obj = obj;
    hk->generation = (atomic_fetch_add(&generations, 1) + 1) & 0xFFFF;

    /* every key of the "global-key-*" options */
    size_t allocated = 0;
    for (const char *const *names = vlc_actions_get_key_names(intf); *names != NULL; names++)
    {
        uint_fast32_t *codes;
        const size_t n = vlc_actions_get_keycodes(intf, *names, true, &codes);
        if (n == 0)
            continue;
        /* ("play-pause": the action of "key-play-pause") */
        char option[64];
        snprintf(option, sizeof (option), "key-%s", *names);
        const vlc_action_id_t action = vlc_actions_get_id(option);
        for (size_t i = 0; i < n && action != ACTIONID_NONE; i++)
        {
            if (codes[i] == KEY_UNSET)
                continue;
            if (hk->wanted_count == allocated)
            {
                const size_t more = allocated ? allocated * 2 : 16;
                void *grown = realloc(hk->wanted, more * sizeof (*hk->wanted));
                if (grown == NULL)
                    break;
                hk->wanted = grown;
                allocated = more;
            }
            hk->wanted[hk->wanted_count].key = codes[i];
            hk->wanted[hk->wanted_count].action = action;
            hk->wanted_count++;
        }
        free(codes);
    }

    sys->hotkeys = hk;
    intf->p_sys = sys;
    /* (always queued, from the main thread too: run once its loop turns,
     * the application made; Close queues after it) */
    dispatch_async_f(dispatch_get_main_queue(), hk, Register);
    return VLC_SUCCESS;
}

/*****************************************************************************
 * Close: nothing reached from here on; the keys go on the main thread
 *****************************************************************************/
static void Close(vlc_object_t *obj)
{
    intf_thread_t *intf = (intf_thread_t *)obj;
    hotkeys_t *hk = intf->p_sys->hotkeys;

    pthread_mutex_lock(&hk->lock);
    hk->closed = true;
    pthread_mutex_unlock(&hk->lock);

    /* (after Register on the main queue, which is first in it; left to the
     * end of the process when the main loop turns no more) */
    dispatch_async_f(dispatch_get_main_queue(), hk, Unregister);
}
