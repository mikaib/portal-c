#include "portal_glfw.h"
#include "portal.h"
#include "glfw/include/GLFW/glfw3.h"
#include <stdio.h>
#include <stdlib.h>

PtBackend *pt_glfw_create() {
    PtBackend *backend = PT_ALLOC(PtBackend);
    backend->type = PT_BACKEND_GLFW;
    backend->capabilities = PT_CAPABILITY_CREATE_WINDOW | PT_CAPABILITY_WINDOW_SIZE | PT_CAPABILITY_WINDOW_VIDEO_MODE;
    backend->kind = PT_BACKEND_KIND_DESKTOP;
    backend->input_event_count = 0;

    backend->init = pt_glfw_init;
    backend->shutdown = pt_glfw_shutdown;
    backend->get_handle = pt_glfw_get_handle;
    backend->create_window = pt_glfw_create_window;
    backend->destroy_window = pt_glfw_destroy_window;
    backend->poll_events = pt_glfw_poll_events;
    backend->swap_buffers = pt_glfw_swap_buffers;
    backend->set_window_title = pt_glfw_set_window_title;
    backend->set_window_size = pt_glfw_set_window_size;
    backend->set_video_mode = pt_glfw_set_video_mode;
    backend->show_window = pt_glfw_show_window;
    backend->hide_window = pt_glfw_hide_window;
    backend->minimize_window = pt_glfw_minimize_window;
    backend->maximize_window = pt_glfw_maximize_window;
    backend->restore_window = pt_glfw_restore_window;
    backend->focus_window = pt_glfw_focus_window;
    backend->get_window_width = pt_glfw_get_window_width;
    backend->get_window_height = pt_glfw_get_window_height;
    backend->get_framebuffer_width = pt_glfw_get_framebuffer_width;
    backend->get_framebuffer_height = pt_glfw_get_framebuffer_height;
    backend->get_usable_width = pt_glfw_get_framebuffer_width;
    backend->get_usable_height = pt_glfw_get_framebuffer_height;
    backend->get_usable_xoffset = pt_glfw_offset_zero;
    backend->get_usable_yoffset = pt_glfw_offset_zero;
    backend->is_window_maximized = pt_glfw_is_window_maximized;
    backend->is_window_minimized = pt_glfw_is_window_minimized;
    backend->is_window_focused = pt_glfw_is_window_focused;
    backend->is_window_visible = pt_glfw_is_window_visible;
    backend->use_gl_context = pt_glfw_use_gl_context;
    backend->should_window_close = pt_glfw_should_window_close;

    return backend;
}

PT_BOOL pt_glfw_init(PtBackend *backend, PtConfig *config) {
    PT_ASSERT(config != NULL);
    PT_ASSERT(backend != NULL);

    if (!glfwInit()) {
        return PT_FALSE;
    }

    return PT_TRUE;
}

void pt_glfw_shutdown(PtBackend *backend) {
    PT_ASSERT(backend != NULL);

    glfwTerminate();
}

PtWindow* pt_glfw_create_window(const char *title, int width, int height, PtWindowFlags flags) {
    PT_ASSERT(title != NULL);

    glfwDefaultWindowHints();

    if (!(flags & PT_FLAG_RESIZABLE)) {
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    }

    if (flags & PT_FLAG_MAXIMIZED) {
        glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
    }

    if (flags & PT_FLAG_HIDDEN) {
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    }

    if (flags & PT_FLAG_ALWAYS_ON_TOP) {
        glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
    }

    if (flags & PT_FLAG_NO_TITLEBAR) {
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    }

    if (flags & PT_FLAG_TRANSPARENT) {
        glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    }

    if (flags & PT_FLAG_NO_FOCUS) {
        glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_FALSE);
        glfwWindowHint(GLFW_FOCUSED, GLFW_FALSE);
    }

    GLFWmonitor* monitor = NULL;
    int window_x = 0, window_y = 0;

    if (flags & PT_FLAG_FULLSCREEN) {
        monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        width = mode->width;
        height = mode->height;
    } else if (flags & PT_FLAG_BORDERLESS) {
        const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        width = mode->width;
        height = mode->height;
        window_x = 0;
        window_y = 0;
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    } else if (flags & PT_FLAG_CENTERED) {
        const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        window_x = (mode->width - width) / 2;
        window_y = (mode->height - height) / 2;
    }

    PtWindow *window = PT_ALLOC(PtWindow);
    PtGlfwHandle *handle = PT_ALLOC(PtGlfwHandle);
    
    handle->glfw = glfwCreateWindow(width, height, title, monitor, NULL);
    handle->window_width = width;
    handle->window_height = height;
    handle->vsync_enabled = (flags & PT_FLAG_VSYNC) != 0;

    if (!monitor && ((flags & PT_FLAG_CENTERED) || (flags & PT_FLAG_BORDERLESS))) {
        glfwSetWindowPos((GLFWwindow*)handle->glfw, window_x, window_y);
    }

    if (flags & PT_FLAG_MINIMIZED) {
        glfwIconifyWindow((GLFWwindow*)handle->glfw);
    }

    glfwGetFramebufferSize((GLFWwindow*)handle->glfw, &handle->framebuffer_width, &handle->framebuffer_height);
    
    window->handle = handle;

    glfwSetWindowUserPointer((GLFWwindow*)handle->glfw, window);
    glfwSetMouseButtonCallback((GLFWwindow*)handle->glfw, (GLFWmousebuttonfun)pt_glfw_cb_mouse_button);
    glfwSetCursorPosCallback((GLFWwindow*)handle->glfw, (GLFWcursorposfun)pt_glfw_cb_mouse_move);
    glfwSetScrollCallback((GLFWwindow*)handle->glfw, (GLFWscrollfun)pt_glfw_cb_mouse_scroll);
    glfwSetKeyCallback((GLFWwindow*)handle->glfw, (GLFWkeyfun)pt_glfw_cb_key);
    glfwSetCharCallback((GLFWwindow*)handle->glfw, (GLFWcharfun)pt_glfw_cb_char);
    glfwSetWindowSizeCallback((GLFWwindow*)handle->glfw, (GLFWwindowsizefun)pt_glfw_cb_window_size);
    glfwSetFramebufferSizeCallback((GLFWwindow*)handle->glfw, (GLFWframebuffersizefun)pt_glfw_cb_framebuffer_size);

    PT_ASSERT(handle->glfw != NULL);
    return window;
}

void* pt_glfw_get_handle(PtWindow *window) {
    PT_ASSERT(window->handle != NULL);

    return NULL;
}

void pt_glfw_cb_mouse_button(GLFWwindow *glfw_window, int button, int action) {
    PtWindow *window = (PtWindow*)glfwGetWindowUserPointer(glfw_window);
    PT_ASSERT(window != NULL);

    PtInputEventData event = pt_create_input_event_data();
    event.type = action == GLFW_PRESS ? PT_INPUT_EVENT_MOUSEDOWN : PT_INPUT_EVENT_MOUSEUP;
    event.mouse.button = button;

    pt_push_input_event(window, event);
}

void pt_glfw_cb_mouse_move(GLFWwindow *glfw_window, double x, double y) {
    PtWindow *window = (PtWindow*)glfwGetWindowUserPointer(glfw_window);
    PT_ASSERT(window != NULL);

    PtInputEventData event = pt_create_input_event_data();
    event.type = PT_INPUT_EVENT_MOUSEMOVE;
    event.mouse.x = x;
    event.mouse.y = y;

    pt_push_input_event(window, event);
}

void pt_glfw_cb_mouse_scroll(GLFWwindow *glfw_window, double x, double y) {
    PtWindow *window = (PtWindow*)glfwGetWindowUserPointer(glfw_window);
    PT_ASSERT(window != NULL);

    PtInputEventData event = pt_create_input_event_data();
    event.type = PT_INPUT_EVENT_MOUSEWHEEL;
    event.mouse.x = x;
    event.mouse.y = y;

    pt_push_input_event(window, event);
}

void pt_glfw_cb_key(GLFWwindow *glfw_window, int key, int scancode, int action) {
    PtWindow *window = (PtWindow*)glfwGetWindowUserPointer(glfw_window);
    PT_ASSERT(window != NULL);

    PtInputEventData event = pt_create_input_event_data();
    event.type = action == GLFW_PRESS ? PT_INPUT_EVENT_KEYDOWN : (action == GLFW_RELEASE ? PT_INPUT_EVENT_KEYUP : PT_INPUT_EVENT_KEYPRESS);
    event.key.key = key;
    event.key.modifiers = scancode;

    // keydown and keypress go together
    if (action == GLFW_PRESS) {
        PtInputEventData secondary_event = pt_create_input_event_data();
        secondary_event.type = PT_INPUT_EVENT_KEYPRESS;
        secondary_event.key.key = key;
        secondary_event.key.modifiers = glfwGetKeyScancode(key);
        pt_push_input_event(window, secondary_event);
    }

    pt_push_input_event(window, event);
}

void pt_glfw_cb_char(GLFWwindow *glfw_window, unsigned int codepoint) {
    PtWindow *window = (PtWindow*)glfwGetWindowUserPointer(glfw_window);
    PT_ASSERT(window != NULL);

    PtInputEventData event = pt_create_input_event_data();
    event.type = PT_INPUT_EVENT_TEXT;
    event.text.codepoint = codepoint;

    pt_push_input_event(window, event);
}

void pt_glfw_cb_window_size(GLFWwindow *glfw_window, int width, int height) {
    PtWindow *window = (PtWindow*)glfwGetWindowUserPointer(glfw_window);
    PT_ASSERT(window != NULL);
    
    PtGlfwHandle *handle = (PtGlfwHandle*)window->handle;
    handle->window_width = width;
    handle->window_height = height;
}

void pt_glfw_cb_framebuffer_size(GLFWwindow *glfw_window, int width, int height) {
    PtWindow *window = (PtWindow*)glfwGetWindowUserPointer(glfw_window);
    PT_ASSERT(window != NULL);
    
    PtGlfwHandle *handle = (PtGlfwHandle*)window->handle;
    handle->framebuffer_width = width;
    handle->framebuffer_height = height;
}

void pt_glfw_destroy_window(PtWindow *window) {
    PT_ASSERT(window->handle != NULL);

    PtGlfwHandle *handle = (PtGlfwHandle*)window->handle;
    glfwDestroyWindow((GLFWwindow*)handle->glfw);
    PT_FREE(handle);
    PT_FREE(window);
}

void pt_glfw_poll_events(PtWindow *window) {
    PT_ASSERT(window->handle != NULL);

    glfwPollEvents();
}

void pt_glfw_swap_buffers(PtWindow *window) {
    PT_ASSERT(window->handle != NULL);

    PtGlfwHandle *handle = (PtGlfwHandle*)window->handle;
    glfwSwapBuffers((GLFWwindow*)handle->glfw);
}

PT_BOOL pt_glfw_should_window_close(PtWindow *window) {
    PT_ASSERT(window->handle != NULL);

    PtGlfwHandle *handle = (PtGlfwHandle*)window->handle;
    return glfwWindowShouldClose((GLFWwindow*)handle->glfw);
}

PT_BOOL pt_glfw_use_gl_context(PtWindow *window) {
    PT_ASSERT(window->handle != NULL);

    PtGlfwHandle *handle = (PtGlfwHandle*)window->handle;
    glfwMakeContextCurrent((GLFWwindow*)handle->glfw);
    glfwSwapInterval(handle->vsync_enabled ? 1 : 0);

    return PT_TRUE;
}

int pt_glfw_get_window_width(PtWindow *window) {
    PT_ASSERT(window->handle != NULL);

    PtGlfwHandle *handle = (PtGlfwHandle*)window->handle;
    return handle->window_width;
}

int pt_glfw_get_window_height(PtWindow *window) {
    PT_ASSERT(window->handle != NULL);

    PtGlfwHandle *handle = (PtGlfwHandle*)window->handle;
    return handle->window_height;
}

int pt_glfw_get_framebuffer_width(PtWindow *window) {
    PT_ASSERT(window->handle != NULL);

    PtGlfwHandle *handle = (PtGlfwHandle*)window->handle;
    return handle->framebuffer_width;
}

int pt_glfw_get_framebuffer_height(PtWindow *window) {
    PT_ASSERT(window->handle != NULL);

    PtGlfwHandle *handle = (PtGlfwHandle*)window->handle;
    return handle->framebuffer_height;
}

int pt_glfw_offset_zero(PtWindow *window) {
    return 0;
}

void pt_glfw_set_window_title(PtWindow *window, const char *title) {
    PT_ASSERT(window != NULL);
    PT_ASSERT(window->handle != NULL);
    PT_ASSERT(title != NULL);

    PtGlfwHandle *handle = (PtGlfwHandle*)window->handle;
    glfwSetWindowTitle((GLFWwindow*)handle->glfw, title);
}

void pt_glfw_set_window_size(PtWindow *window, int width, int height) {
    PT_ASSERT(window != NULL);
    PT_ASSERT(window->handle != NULL);
    PT_ASSERT(width > 0);
    PT_ASSERT(height > 0);

    PtGlfwHandle *handle = (PtGlfwHandle*)window->handle;
    glfwSetWindowSize((GLFWwindow*)handle->glfw, width, height);
    handle->window_width = width;
    handle->window_height = height;
}

void pt_glfw_set_video_mode(PtWindow *window, PtVideoMode mode) {
    PT_ASSERT(window != NULL);
    PT_ASSERT(window->handle != NULL);

    PtGlfwHandle *handle = (PtGlfwHandle*)window->handle;
    GLFWwindow *glfw_window = (GLFWwindow*)handle->glfw;

    switch (mode) {
        case PT_VIDEO_MODE_WINDOWED:
            if (glfwGetWindowMonitor(glfw_window) != NULL) {
                glfwSetWindowMonitor(glfw_window, NULL, 100, 100, handle->window_width, handle->window_height, GLFW_DONT_CARE);
            }
            break;
        case PT_VIDEO_MODE_FULLSCREEN:
            {
                GLFWmonitor *monitor = glfwGetPrimaryMonitor();
                const GLFWvidmode *mode_info = glfwGetVideoMode(monitor);
                glfwSetWindowMonitor(glfw_window, monitor, 0, 0, mode_info->width, mode_info->height, GLFW_DONT_CARE);
            }
            break;
        case PT_VIDEO_MODE_BORDERLESS:
            {
                GLFWmonitor *monitor = glfwGetPrimaryMonitor();
                const GLFWvidmode *mode_info = glfwGetVideoMode(monitor);
                glfwSetWindowMonitor(glfw_window, NULL, 0, 0, mode_info->width, mode_info->height, GLFW_DONT_CARE);
            }
            break;
        case PT_VIDEO_MODE_MAXIMIZED:
            if (glfwGetWindowMonitor(glfw_window) != NULL) {
                glfwSetWindowMonitor(glfw_window, NULL, 100, 100, handle->window_width, handle->window_height, GLFW_DONT_CARE);
            }
            glfwMaximizeWindow(glfw_window);
            break;
    }
}

void pt_glfw_show_window(PtWindow *window) {
    PT_ASSERT(window != NULL);
    PT_ASSERT(window->handle != NULL);

    PtGlfwHandle *handle = (PtGlfwHandle*)window->handle;
    glfwShowWindow((GLFWwindow*)handle->glfw);
}

void pt_glfw_hide_window(PtWindow *window) {
    PT_ASSERT(window != NULL);
    PT_ASSERT(window->handle != NULL);

    PtGlfwHandle *handle = (PtGlfwHandle*)window->handle;
    glfwHideWindow((GLFWwindow*)handle->glfw);
}

void pt_glfw_minimize_window(PtWindow *window) {
    PT_ASSERT(window != NULL);
    PT_ASSERT(window->handle != NULL);

    PtGlfwHandle *handle = (PtGlfwHandle*)window->handle;
    glfwIconifyWindow((GLFWwindow*)handle->glfw);
}

void pt_glfw_maximize_window(PtWindow *window) {
    PT_ASSERT(window != NULL);
    PT_ASSERT(window->handle != NULL);

    PtGlfwHandle *handle = (PtGlfwHandle*)window->handle;
    glfwMaximizeWindow((GLFWwindow*)handle->glfw);
}

void pt_glfw_restore_window(PtWindow *window) {
    PT_ASSERT(window != NULL);
    PT_ASSERT(window->handle != NULL);

    PtGlfwHandle *handle = (PtGlfwHandle*)window->handle;
    glfwRestoreWindow((GLFWwindow*)handle->glfw);
}

void pt_glfw_focus_window(PtWindow *window) {
    PT_ASSERT(window != NULL);
    PT_ASSERT(window->handle != NULL);

    PtGlfwHandle *handle = (PtGlfwHandle*)window->handle;
    glfwFocusWindow((GLFWwindow*)handle->glfw);
}

PT_BOOL pt_glfw_is_window_maximized(PtWindow *window) {
    PT_ASSERT(window != NULL);
    PT_ASSERT(window->handle != NULL);

    PtGlfwHandle *handle = (PtGlfwHandle*)window->handle;
    return glfwGetWindowAttrib((GLFWwindow*)handle->glfw, GLFW_MAXIMIZED);
}

PT_BOOL pt_glfw_is_window_minimized(PtWindow *window) {
    PT_ASSERT(window != NULL);
    PT_ASSERT(window->handle != NULL);

    PtGlfwHandle *handle = (PtGlfwHandle*)window->handle;
    return glfwGetWindowAttrib((GLFWwindow*)handle->glfw, GLFW_ICONIFIED);
}

PT_BOOL pt_glfw_is_window_focused(PtWindow *window) {
    PT_ASSERT(window != NULL);
    PT_ASSERT(window->handle != NULL);

    PtGlfwHandle *handle = (PtGlfwHandle*)window->handle;
    return glfwGetWindowAttrib((GLFWwindow*)handle->glfw, GLFW_FOCUSED);
}

PT_BOOL pt_glfw_is_window_visible(PtWindow *window) {
    PT_ASSERT(window != NULL);
    PT_ASSERT(window->handle != NULL);

    PtGlfwHandle *handle = (PtGlfwHandle*)window->handle;
    return glfwGetWindowAttrib((GLFWwindow*)handle->glfw, GLFW_VISIBLE);
}
