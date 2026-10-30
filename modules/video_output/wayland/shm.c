/**
 * @file shm.c
 * @brief Wayland shared memory video output module for VLC media player
 */
/*****************************************************************************
 * Copyright © 2014, 2017 Rémi Denis-Courmont
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

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>

#include <wayland-client.h>
#include "viewporter-client-protocol.h"
#include "registry.h"

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_vout_display.h>
#include <vlc_fs.h>

#define MAX_PICTURES 4

typedef struct vout_display_sys_t
{
    vlc_window_t *embed; /* VLC window */
    struct wl_event_queue *eventq;
    struct wl_shm *shm;
    struct wp_viewporter *viewporter;
    struct wp_viewport *viewport; /* of the surface showing the picture */

    /* The picture in a surface of its own, placed where the video goes in
     * the window (centred), over the window's surface filled with black:
     * drawn in the window's surface itself, it started at its left and the
     * rest of the window showed what was below it */
    struct wl_subcompositor *subcompositor;
    struct wl_surface *video;
    struct wl_subsurface *subsurface;
    struct wp_viewport *background_viewport;
    struct wl_buffer *black;
    bool placement_changed;

    size_t active_buffers;
} vout_display_sys_t;

/* The surface the picture is attached to */
static struct wl_surface *PictureSurface(vout_display_t *vd)
{
    vout_display_sys_t *sys = vd->sys;
    return sys->video != NULL ? sys->video : sys->embed->handle.wl;
}

struct buffer_data
{
    picture_t *picture;
    size_t *counter;
};

static_assert (sizeof (vout_display_sys_t) >= MAX_PICTURES,
               "Pointer arithmetic error");

static void buffer_release_cb(void *data, struct wl_buffer *buffer)
{
    struct buffer_data *d = data;

    picture_Release(d->picture);
    (*(d->counter))--;
    free(d);
    wl_buffer_destroy(buffer);
}

static const struct wl_buffer_listener buffer_cbs =
{
    buffer_release_cb,
};

static void Prepare(vout_display_t *vd, picture_t *pic,
                    const struct vlc_render_subpicture *subpic,
                    vlc_tick_t date)
{
    VLC_UNUSED(date);
    vout_display_sys_t *sys = vd->sys;
    struct wl_display *display = sys->embed->display.wl;
    struct wl_surface *surface = PictureSurface(vd);
    struct picture_buffer_t *picbuf = pic->p_sys;

    if (picbuf->fd == -1)
        return;

    struct buffer_data *d = malloc(sizeof (*d));
    if (unlikely(d == NULL))
        return;

    d->picture = pic;
    d->counter = &sys->active_buffers;

    off_t offset = picbuf->offset;
    const size_t stride = pic->p->i_pitch;
    const size_t size = pic->p->i_lines * stride;
    struct wl_shm_pool *pool;
    struct wl_buffer *buf;

    pool = wl_shm_create_pool(sys->shm, picbuf->fd, offset + size);
    if (pool == NULL)
    {
        free(d);
        return;
    }

    if (sys->viewport == NULL) /* Poor man's crop */
        offset += 4 * vd->fmt->i_x_offset
                  + pic->p->i_pitch * vd->fmt->i_y_offset;

    buf = wl_shm_pool_create_buffer(pool, offset, vd->fmt->i_visible_width,
                                    vd->fmt->i_visible_height, stride,
                                    WL_SHM_FORMAT_XRGB8888);
    wl_shm_pool_destroy(pool);
    if (buf == NULL)
    {
        free(d);
        return;
    }

    picture_Hold(pic);

    wl_buffer_add_listener(buf, &buffer_cbs, d);
    wl_surface_attach(surface, buf, 0, 0);
    wl_surface_damage(surface, 0, 0, INT32_MAX, INT32_MAX);
    wl_display_flush(display);

    sys->active_buffers++;

    (void) subpic;
}

static void Display(vout_display_t *vd, picture_t *pic)
{
    vout_display_sys_t *sys = vd->sys;
    struct wl_display *display = sys->embed->display.wl;

    wl_surface_commit(PictureSurface(vd));
    if (sys->video != NULL && sys->placement_changed)
    {
        /* the position of a subsurface and the size of the black
         * background apply with the window's surface */
        struct wl_surface *window = sys->embed->handle.wl;
        unsigned width = vd->cfg->display.width;
        unsigned height = vd->cfg->display.height;

        wl_subsurface_set_position(sys->subsurface,
                                   vd->place->x, vd->place->y);
        if (width > 0 && height > 0)
            wp_viewport_set_destination(sys->background_viewport,
                                        width, height);
        wl_surface_damage(window, 0, 0, INT32_MAX, INT32_MAX);
        wl_surface_commit(window);
        sys->placement_changed = false;
    }
    wl_display_roundtrip_queue(display, sys->eventq);

    (void) pic;
}

static int ResetPictures(vout_display_t *vd, video_format_t *fmt)
{
    video_format_t src;
    vout_display_sys_t *sys = vd->sys;
    assert(sys->viewport == NULL);

    video_format_ApplyRotation(&src, vd->source);

    fmt->i_width  = src.i_width * vd->place->width
                                / src.i_visible_width;
    fmt->i_height = src.i_height * vd->place->height
                                    / src.i_visible_height;
    fmt->i_visible_width  = vd->place->width;
    fmt->i_visible_height = vd->place->height;
    fmt->i_x_offset = src.i_x_offset * vd->place->width
                                        / src.i_visible_width;
    fmt->i_y_offset = src.i_y_offset * vd->place->height
                                        / src.i_visible_height;
    return VLC_SUCCESS;
}

static int UpdateViewport(vout_display_t *vd)
{
    vout_display_sys_t *sys = vd->sys;

    if (sys->viewport == NULL)
        return VLC_EGENERIC;

    sys->placement_changed = true;

    video_format_t fmt;

    video_format_ApplyRotation(&fmt, vd->source);

    wp_viewport_set_source(sys->viewport,
                    wl_fixed_from_int(fmt.i_x_offset),
                    wl_fixed_from_int(fmt.i_y_offset),
                    wl_fixed_from_int(fmt.i_visible_width),
                    wl_fixed_from_int(fmt.i_visible_height));
    wp_viewport_set_destination(sys->viewport,
                                vd->place->width, vd->place->height);
    return VLC_SUCCESS;
}

static int SetDisplaySize(vout_display_t *vd, unsigned width, unsigned height)
{
    VLC_UNUSED(width); VLC_UNUSED(height);
    return UpdateViewport(vd);
}

static int PlacementChanged(vout_display_t *vd, const vout_display_place_t *place)
{
    VLC_UNUSED(place);
    return UpdateViewport(vd);
}

static int AspectChanged(vout_display_t *vd, const video_format_t *source)
{
    VLC_UNUSED(source);
    return PlacementChanged(vd, NULL);
}

static void shm_format_cb(void *data, struct wl_shm *shm, uint32_t format)
{
    vout_display_t *vd = data;
    char str[4];

    memcpy(str, &format, sizeof (str));

    if (format >= 0x20202020)
        msg_Dbg(vd, "format %.4s (0x%08"PRIx32")", str, format);
    else
        msg_Dbg(vd, "format %4"PRIu32" (0x%08"PRIx32")", format, format);
    (void) shm;
}

static const struct wl_shm_listener shm_cbs =
{
    shm_format_cb,
};

static void Close(vout_display_t *vd)
{
    vout_display_sys_t *sys = vd->sys;
    struct wl_display *display = sys->embed->display.wl;
    struct wl_surface *surface = sys->embed->handle.wl;

    if (sys->video != NULL)
    {
        /* The picture's surface goes away before the wait: Weston 14 and
         * later keep the buffer last shown by a surface emptied (unmapped)
         * until something else is attached or the surface is destroyed, so
         * Close waited for it forever */
        wl_surface_attach(sys->video, NULL, 0, 0);
        wl_surface_commit(sys->video);
        if (sys->viewport != NULL)
            wp_viewport_destroy(sys->viewport);
        sys->viewport = NULL;
        wl_subsurface_destroy(sys->subsurface);
        wl_surface_destroy(sys->video);
    }
    wl_surface_attach(surface, NULL, 0, 0);
    wl_surface_commit(surface);

    /* Wait until all picture buffers are released by the server */
    while (sys->active_buffers > 0) {
        msg_Dbg(vd, "%zu buffer(s) still active", sys->active_buffers);
        wl_display_roundtrip_queue(display, sys->eventq);
    }
    msg_Dbg(vd, "no active buffers left");

    if (sys->viewport != NULL)
        wp_viewport_destroy(sys->viewport);
    if (sys->video != NULL)
    {
        wp_viewport_destroy(sys->background_viewport);
        wl_buffer_destroy(sys->black);
    }
    if (sys->subcompositor != NULL)
        wl_subcompositor_destroy(sys->subcompositor);
    if (sys->viewporter != NULL)
        wp_viewporter_destroy(sys->viewporter);
    wl_shm_destroy(sys->shm);
    wl_display_flush(display);
    wl_event_queue_destroy(sys->eventq);
    free(sys);
}

static const struct vlc_display_operations ops = {
    .close = Close,
    .prepare = Prepare,
    .display = Display,
    .set_display_size = SetDisplaySize,
    .reset_pictures = ResetPictures,
    .video_place_changed = PlacementChanged,
    .set_source_aspect = AspectChanged,
    .set_source_crop = AspectChanged,
};

/* A buffer of one black pixel, stretched over the window by a viewport */
static struct wl_buffer *CreateBlackBuffer(struct wl_shm *shm)
{
    int fd = vlc_memfd();
    if (fd == -1)
        return NULL;

    struct wl_buffer *buf = NULL;
    if (ftruncate(fd, 4) == 0) /* zero filled: XRGB black */
    {
        struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, 4);
        if (pool != NULL)
        {
            buf = wl_shm_pool_create_buffer(pool, 0, 1, 1, 4,
                                            WL_SHM_FORMAT_XRGB8888);
            wl_shm_pool_destroy(pool);
        }
    }
    vlc_close(fd);
    return buf;
}

/* The picture in a subsurface over a black window surface; false: the
 * picture goes in the window's surface (no subcompositor or viewporter) */
static bool SetupSubsurface(vout_display_t *vd, struct vlc_wl_registry *registry)
{
    vout_display_sys_t *sys = vd->sys;
    struct wl_surface *window = sys->embed->handle.wl;

    if (sys->viewporter == NULL)
        return false;
    /* version 2 at least: the picture surface takes the buffer transform */
    uint32_t version = 4;
    struct wl_compositor *compositor = (struct wl_compositor *)
        vlc_wl_interface_bind(registry, "wl_compositor",
                              &wl_compositor_interface, &version);
    if (compositor == NULL)
        return false;
    sys->subcompositor = (struct wl_subcompositor *)
        vlc_wl_interface_bind(registry, "wl_subcompositor",
                              &wl_subcompositor_interface, NULL);
    if (sys->subcompositor == NULL)
        goto error;
    sys->black = CreateBlackBuffer(sys->shm);
    if (sys->black == NULL)
        goto error;
    sys->video = wl_compositor_create_surface(compositor);
    if (sys->video == NULL)
        goto error;

    /* the window (the interface) takes the pointer, not the picture */
    struct wl_region *empty = wl_compositor_create_region(compositor);
    if (empty != NULL)
    {
        wl_surface_set_input_region(sys->video, empty);
        wl_region_destroy(empty);
    }
    sys->subsurface = wl_subcompositor_get_subsurface(sys->subcompositor,
                                                      sys->video, window);
    wl_subsurface_set_desync(sys->subsurface);
    wl_subsurface_set_position(sys->subsurface, 0, 0);

    sys->background_viewport = wp_viewporter_get_viewport(sys->viewporter,
                                                          window);
    wl_surface_attach(window, sys->black, 0, 0);
    if (vd->cfg->display.width > 0 && vd->cfg->display.height > 0)
        wp_viewport_set_destination(sys->background_viewport,
                                    vd->cfg->display.width,
                                    vd->cfg->display.height);
    sys->placement_changed = true;
    wl_compositor_destroy(compositor);
    return true;

error:
    if (sys->video != NULL)
        wl_surface_destroy(sys->video);
    sys->video = NULL;
    if (sys->black != NULL)
        wl_buffer_destroy(sys->black);
    sys->black = NULL;
    if (sys->subcompositor != NULL)
        wl_subcompositor_destroy(sys->subcompositor);
    sys->subcompositor = NULL;
    wl_compositor_destroy(compositor);
    return false;
}

static int Open(vout_display_t *vd,
                video_format_t *fmtp, vlc_video_context *context)
{
    if (vd->cfg->window->type != VLC_WINDOW_TYPE_WAYLAND)
        return VLC_EGENERIC;

    vout_display_sys_t *sys = malloc(sizeof (*sys));
    if (unlikely(sys == NULL))
        return VLC_ENOMEM;

    vd->sys = sys;
    sys->embed = NULL;
    sys->eventq = NULL;
    sys->shm = NULL;
    sys->subcompositor = NULL;
    sys->video = NULL;
    sys->subsurface = NULL;
    sys->background_viewport = NULL;
    sys->black = NULL;
    sys->placement_changed = false;
    sys->active_buffers = 0;

    /* Get window */
    sys->embed = vd->cfg->window;
    assert(sys->embed != NULL);

    struct wl_display *display = sys->embed->display.wl;
    struct vlc_wl_registry *registry = NULL;

    sys->eventq = wl_display_create_queue(display);
    if (sys->eventq == NULL)
        goto error;

    registry = vlc_wl_registry_get(display, sys->eventq);
    if (registry == NULL)
        goto error;

    sys->shm = vlc_wl_shm_get(registry);
    if (sys->shm == NULL)
        goto error;

    sys->viewporter = (struct wp_viewporter *)
                      vlc_wl_interface_bind(registry, "wp_viewporter",
                                            &wp_viewporter_interface, NULL);

    wl_shm_add_listener(sys->shm, &shm_cbs, vd);
    wl_display_roundtrip_queue(display, sys->eventq);

    if (!SetupSubsurface(vd, registry))
        msg_Dbg(vd, "picture in the window's surface (no subsurface)");

    struct wl_surface *surface = PictureSurface(vd);
    if (sys->viewporter != NULL)
        sys->viewport = wp_viewporter_get_viewport(sys->viewporter, surface);
    else
        sys->viewport = NULL;

    /* Determine our pixel format */
    static const enum wl_output_transform transforms[8] = {
        [ORIENT_TOP_LEFT] = WL_OUTPUT_TRANSFORM_NORMAL,
        [ORIENT_TOP_RIGHT] = WL_OUTPUT_TRANSFORM_FLIPPED,
        [ORIENT_BOTTOM_LEFT] = WL_OUTPUT_TRANSFORM_FLIPPED_180,
        [ORIENT_BOTTOM_RIGHT] = WL_OUTPUT_TRANSFORM_180,
        [ORIENT_LEFT_TOP] = WL_OUTPUT_TRANSFORM_FLIPPED_270,
        [ORIENT_LEFT_BOTTOM] = WL_OUTPUT_TRANSFORM_90,
        [ORIENT_RIGHT_TOP] = WL_OUTPUT_TRANSFORM_270,
        [ORIENT_RIGHT_BOTTOM] = WL_OUTPUT_TRANSFORM_FLIPPED_90,
    };

    if (vlc_wl_interface_get_version(registry, "wl_compositor") >= 2)
    {
        wl_surface_set_buffer_transform(surface,
                                        transforms[fmtp->orientation]);
    }
    else
    {
        video_format_t fmt = *fmtp;
        video_format_ApplyRotation(fmtp, &fmt);
    }

    fmtp->i_chroma = VLC_CODEC_BGRX;

    vd->ops = &ops;

    vlc_wl_registry_destroy(registry);
    (void) context;
    return VLC_SUCCESS;

error:
    if (sys->shm != NULL)
        wl_shm_destroy(sys->shm);

    if (registry != NULL)
        vlc_wl_registry_destroy(registry);

    if (sys->eventq != NULL)
        wl_event_queue_destroy(sys->eventq);
    free(sys);
    return VLC_EGENERIC;
}

vlc_module_begin()
    set_shortname(N_("WL SHM"))
    set_description(N_("Wayland shared memory video output"))
    set_subcategory(SUBCAT_VIDEO_VOUT)
    set_callback_display(Open, 170)
    add_shortcut("wl")
vlc_module_end()
