#include "portal.h"
#include <stdlib.h>

#define NOOP_WIDTH 0
#define NOOP_HEIGHT 0

typedef struct {
    int width;
    int height;
    PT_BOOL should_close;
} NoopWindow;

static PT_BOOL pt_noop_init(PtBackend *backend, PtConfig *config) {
    return PT_TRUE;
}

static void pt_noop_shutdown(PtBackend *backend) {}

static PtWindow* pt_noop_create_window(const char *title, int width, int height, PtWindowFlags flags) {
    NoopWindow *noop = (NoopWindow*)PT_ALLOC(NoopWindow);
    noop->width = (width > 0) ? width : NOOP_WIDTH;
    noop->height = (height > 0) ? height : NOOP_HEIGHT;
    noop->should_close = PT_FALSE;

    PtWindow *window = (PtWindow*)PT_ALLOC(PtWindow);
    window->handle = noop;

    return window;
}

static void pt_noop_destroy_window(PtWindow *window) {
    if (!window) return;
    PT_FREE(window->handle);
    PT_FREE(window);
}

static void pt_noop_poll_events(PtWindow *window) {
}

static void pt_noop_swap_buffers(PtWindow *window) {
}

static int pt_noop_get_window_width(PtWindow *window) {
    NoopWindow *noop = (NoopWindow*)window->handle;
    return noop->width;
}

static int pt_noop_get_window_height(PtWindow *window) {
    NoopWindow *noop = (NoopWindow*)window->handle;
    return noop->height;
}

static int pt_noop_get_framebuffer_width(PtWindow *window) {
    NoopWindow *noop = (NoopWindow*)window->handle;
    return noop->width;
}

static int pt_noop_get_framebuffer_height(PtWindow *window) {
    NoopWindow *noop = (NoopWindow*)window->handle;
    return noop->height;
}

static PT_BOOL pt_noop_use_gl_context(PtWindow *window) {
    return PT_TRUE;
}

static PT_BOOL pt_noop_should_window_close(PtWindow *window) {
    NoopWindow *noop = (NoopWindow*)window->handle;
    return noop->should_close;
}

int pt_noop_offset_zero(PtWindow *window) {
    return 0;
}

static void* pt_noop_get_handle(PtWindow *window) {
    return NULL;
}

static void pt_noop_set_window_title(PtWindow *window, const char *title) {}

static void pt_noop_set_window_size(PtWindow *window, int width, int height) {
    if (!window || !window->handle) return;

    NoopWindow *noop = (NoopWindow*)window->handle;
    noop->width = width;
    noop->height = height;
}

static void pt_noop_set_video_mode(PtWindow *window, PtVideoMode mode) {}

static void pt_noop_show_window(PtWindow *window) {}

static void pt_noop_hide_window(PtWindow *window) {}

static void pt_noop_minimize_window(PtWindow *window) {}

static void pt_noop_maximize_window(PtWindow *window) {}

static void pt_noop_restore_window(PtWindow *window) {}

static void pt_noop_focus_window(PtWindow *window) {}

static PT_BOOL pt_noop_is_window_maximized(PtWindow *window) {
    return PT_FALSE;
}

static PT_BOOL pt_noop_is_window_minimized(PtWindow *window) {
    return PT_FALSE;
}

static PT_BOOL pt_noop_is_window_focused(PtWindow *window) {
    return PT_TRUE; // sane default
}

static PT_BOOL pt_noop_is_window_visible(PtWindow *window) {
    return PT_TRUE; // sane default
}

PtBackend* pt_noop_create() {
    PtBackend *backend = PT_ALLOC(PtBackend);
    backend->type = PT_BACKEND_NOOP;
    backend->capabilities = PT_CAPABILITY_CREATE_WINDOW | PT_CAPABILITY_WINDOW_SIZE;
    backend->kind = PT_BACKEND_KIND_HEADLESS;

    backend->init = pt_noop_init;
    backend->shutdown = pt_noop_shutdown;
    backend->get_handle = pt_noop_get_handle;
    backend->create_window = pt_noop_create_window;
    backend->destroy_window = pt_noop_destroy_window;
    backend->poll_events = pt_noop_poll_events;
    backend->swap_buffers = pt_noop_swap_buffers;
    backend->set_window_title = pt_noop_set_window_title;
    backend->set_window_size = pt_noop_set_window_size;
    backend->set_video_mode = pt_noop_set_video_mode;
    backend->show_window = pt_noop_show_window;
    backend->hide_window = pt_noop_hide_window;
    backend->minimize_window = pt_noop_minimize_window;
    backend->maximize_window = pt_noop_maximize_window;
    backend->restore_window = pt_noop_restore_window;
    backend->focus_window = pt_noop_focus_window;
    backend->get_window_width = pt_noop_get_window_width;
    backend->get_window_height = pt_noop_get_window_height;
    backend->get_framebuffer_width = pt_noop_get_framebuffer_width;
    backend->get_framebuffer_height = pt_noop_get_framebuffer_height;
    backend->get_usable_width = pt_noop_get_framebuffer_width;
    backend->get_usable_height = pt_noop_get_framebuffer_height;
    backend->get_usable_xoffset = pt_noop_offset_zero;
    backend->get_usable_yoffset = pt_noop_offset_zero;
    backend->is_window_maximized = pt_noop_is_window_maximized;
    backend->is_window_minimized = pt_noop_is_window_minimized;
    backend->is_window_focused = pt_noop_is_window_focused;
    backend->is_window_visible = pt_noop_is_window_visible;
    backend->use_gl_context = pt_noop_use_gl_context;
    backend->should_window_close = pt_noop_should_window_close;

    return backend;
}
