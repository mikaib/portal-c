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
PT_BOOL pt_glfw_should_window_close(PtWindow *window);

// context
PT_BOOL pt_glfw_use_gl_context(PtWindow *window);

#ifdef __cplusplus
}
#endif

#endif //PORTAL_GLFW_H
