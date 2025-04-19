#ifndef PORTAL_ANDROID_H
#define PORTAL_ANDROID_H

#include "portal.h"

#ifdef __cplusplus
extern "C"
{
#endif

// creation / destruction
PtBackend *pt_android_create();
PT_BOOL pt_android_init(PtBackend *backend, PtConfig *config);
void pt_android_shutdown(PtBackend *backend);

// window
PtWindow* pt_android_create_window(const char *title, int width, int height);
void pt_android_destroy_window(PtWindow *window);
void pt_android_poll_events(PtWindow *window);
void pt_android_swap_buffers(PtWindow *window);
PT_BOOL pt_android_should_window_close(PtWindow *window);

// context
PT_BOOL pt_android_use_gl_context(PtWindow *window);

// android specific
void pt_android_set_native_window(PtWindow *window, void *native_window, void *activity);
void pt_android_handle_surface_changed(PtWindow *window, int width, int height);
void pt_android_set_should_close(PtWindow *window, int should_close);

#ifdef __cplusplus
}
#endif

#endif //PORTAL_ANDROID_H