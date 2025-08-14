#ifndef PORTAL_GLFW_H
#define PORTAL_GLFW_H

#include "portal.h"

// Forward declaration for GLFW types
struct GLFWwindow;
typedef struct GLFWwindow GLFWwindow;

#ifdef __cplusplus
extern "C"
{
#endif

// Handle struct to cache sizes and hold GLFW window
typedef struct {
    void* glfw;
    int window_width;
    int window_height;
    int framebuffer_width;
    int framebuffer_height;
    PT_BOOL vsync_enabled;
} PtGlfwHandle;

// creation / destruction
PtBackend *pt_glfw_create();
PT_BOOL pt_glfw_init(PtBackend *backend, PtConfig *config);
void pt_glfw_shutdown(PtBackend *backend);

// window
PtWindow* pt_glfw_create_window(const char *title, int width, int height, PtWindowFlags flags);
void pt_glfw_destroy_window(PtWindow *window);
void pt_glfw_poll_events(PtWindow *window);
void pt_glfw_swap_buffers(PtWindow *window);
void pt_glfw_set_window_title(PtWindow *window, const char *title);
void pt_glfw_set_window_size(PtWindow *window, int width, int height);
void pt_glfw_set_video_mode(PtWindow *window, PtVideoMode mode);
void* pt_glfw_get_handle(PtWindow *window);
int pt_glfw_get_window_width(PtWindow *window);
int pt_glfw_get_window_height(PtWindow *window);
int pt_glfw_get_framebuffer_width(PtWindow *window);
int pt_glfw_get_framebuffer_height(PtWindow *window);
PT_BOOL pt_glfw_should_window_close(PtWindow *window);

// callbacks
void pt_glfw_cb_mouse_button(GLFWwindow *glfw_window, int button, int action);
void pt_glfw_cb_mouse_move(GLFWwindow *glfw_window, double x, double y);
void pt_glfw_cb_mouse_scroll(GLFWwindow *glfw_window, double x, double y);
void pt_glfw_cb_key(GLFWwindow *glfw_window, int key, int scancode, int action);
void pt_glfw_cb_char(GLFWwindow *glfw_window, unsigned int codepoint);
void pt_glfw_cb_window_size(GLFWwindow *glfw_window, int width, int height);
void pt_glfw_cb_framebuffer_size(GLFWwindow *glfw_window, int width, int height);

// helper
int pt_glfw_offset_zero(PtWindow *window);

// context
PT_BOOL pt_glfw_use_gl_context(PtWindow *window);

#ifdef __cplusplus
}
#endif

#endif //PORTAL_GLFW_H
