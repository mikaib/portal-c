#ifndef PORTAL_NOOP_H
#define PORTAL_NOOP_H

#include "portal.h"

#ifdef __cplusplus
extern "C" {
#endif

// creation / destruction
PtBackend *pt_noop_create();
PT_BOOL pt_noop_init(PtBackend *backend, PtConfig *config);
void pt_noop_shutdown(PtBackend *backend);

// window
PtWindow* pt_noop_create_window(const char *title, int width, int height, PtWindowFlags flags);
void pt_noop_destroy_window(PtWindow *window);
void pt_noop_poll_events(PtWindow *window);
void pt_noop_swap_buffers(PtWindow *window);
void pt_noop_set_window_title(PtWindow *window, const char *title);
void pt_noop_set_window_size(PtWindow *window, int width, int height);
void pt_noop_set_video_mode(PtWindow *window, PtVideoMode mode);
void* pt_noop_get_handle(PtWindow *window);
int pt_noop_get_window_width(PtWindow *window);
int pt_noop_get_window_height(PtWindow *window);
int pt_noop_get_framebuffer_width(PtWindow *window);
int pt_noop_get_framebuffer_height(PtWindow *window);
PT_BOOL pt_noop_should_window_close(PtWindow *window);

// helper
int pt_noop_offset_zero(PtWindow *window);

// context
PT_BOOL pt_noop_use_gl_context(PtWindow *window);

// window state management
void pt_noop_show_window(PtWindow *window);
void pt_noop_hide_window(PtWindow *window);
void pt_noop_minimize_window(PtWindow *window);
void pt_noop_maximize_window(PtWindow *window);
void pt_noop_restore_window(PtWindow *window);
void pt_noop_focus_window(PtWindow *window);

// window state queries
PT_BOOL pt_noop_is_window_maximized(PtWindow *window);
PT_BOOL pt_noop_is_window_minimized(PtWindow *window);
PT_BOOL pt_noop_is_window_focused(PtWindow *window);
PT_BOOL pt_noop_is_window_visible(PtWindow *window);

#ifdef __cplusplus
}
#endif

#endif // PORTAL_NOOP_H
