#ifndef PORTAL_GLFW_H
#define PORTAL_GLFW_H

#include "portal.h"

#ifdef __cplusplus
extern "C"
{
#endif

// creation / destruction
PtBackend *pt_glfw_create();
PT_BOOL pt_glfw_init(PtBackend *backend, PtConfig *config);
void pt_glfw_shutdown(PtBackend *backend);

// window
PtWindow* pt_glfw_create_window(const char *title, int width, int height);
void pt_glfw_destroy_window(PtWindow *window);
void pt_glfw_poll_events(PtWindow *window);
void pt_glfw_swap_buffers(PtWindow *window);
int pt_glfw_get_window_width(PtWindow *window);
int pt_glfw_get_window_height(PtWindow *window);
int pt_glfw_get_framebuffer_width(PtWindow *window);
int pt_glfw_get_framebuffer_height(PtWindow *window);
PT_BOOL pt_glfw_should_window_close(PtWindow *window);

// callbacks
void pt_glfw_cb_mouse_button(PtWindow *window, int button, int action);
void pt_glfw_cb_mouse_move(PtWindow *window, double x, double y);
void pt_glfw_cb_mouse_scroll(PtWindow *window, double x, double y);
void pt_glfw_cb_key(PtWindow *window, int key, int scancode, int action);
void pt_glfw_cb_char(PtWindow *window, unsigned int codepoint);

// helper
int pt_glfw_offset_zero(PtWindow *window);

// context
PT_BOOL pt_glfw_use_gl_context(PtWindow *window);

#ifdef __cplusplus
}
#endif

#endif //PORTAL_GLFW_H
