#ifndef PORTAL_ANDROID_H
#define PORTAL_ANDROID_H

#include "portal.h"
#include <android/native_window.h>
#include <android/native_activity.h>
#include <android_native_app_glue.h>
#include <android/input.h>

#ifdef __cplusplus
extern "C"
{
#endif

// creation / destruction
PtBackend *pt_android_create();
PT_BOOL pt_android_init(PtBackend *backend, PtConfig *config);
void pt_android_shutdown(PtBackend *backend);

// window
PtWindow* pt_android_create_window(const char *title, int width, int height, PtWindowFlags flags);
void pt_android_destroy_window(PtWindow *window);
void pt_android_poll_events(PtWindow *window);
void pt_android_swap_buffers(PtWindow *window);
void* pt_android_get_handle(PtWindow *window);
int pt_android_get_window_width(PtWindow *window);
int pt_android_get_window_height(PtWindow *window);
int pt_android_get_framebuffer_width(PtWindow *window);
int pt_android_get_framebuffer_height(PtWindow *window);
PT_BOOL pt_android_should_window_close(PtWindow *window);

// context
PT_BOOL pt_android_use_gl_context(PtWindow *window);

// android specific
void pt_android_set_native_window(ANativeWindow *native_window, ANativeActivity *activity);
PT_BOOL pt_android_init_egl();
void pt_android_handle_surface_changed(int width, int height);
void pt_android_set_should_close(int should_close);
void pt_android_internal_poll();
void pt_android_configure_fullscreen(struct android_app* state);
int pt_android_handle_input(struct android_app* app, AInputEvent* event);
int pt_android_get_usable_framebuffer_width(PtWindow *window);
int pt_android_get_usable_framebuffer_height(PtWindow *window);
int pt_android_get_usable_framebuffer_xoffset(PtWindow *window);
int pt_android_get_usable_framebuffer_yoffset(PtWindow *window);

#ifdef __cplusplus
}
#endif

#endif //PORTAL_ANDROID_H