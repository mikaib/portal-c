#include "portal_glfw.h"
#include "portal.h"
#include "glfw/include/GLFW/glfw3.h"
#include <stdio.h>
#include <stdlib.h>

PtBackend *pt_glfw_create() {
    PtBackend *backend = PT_ALLOC(PtBackend);
    backend->type = PT_BACKEND_GLFW;
    backend->capabilities = PT_CAPABILITY_CREATE_WINDOW;
    backend->kind = PT_BACKEND_KIND_DESKTOP;
    backend->input_event_count = 0;

    backend->init = pt_glfw_init;
    backend->shutdown = pt_glfw_shutdown;
    backend->get_handle = pt_glfw_get_handle;
    backend->create_window = pt_glfw_create_window;
    backend->destroy_window = pt_glfw_destroy_window;
    backend->poll_events = pt_glfw_poll_events;
    backend->swap_buffers = pt_glfw_swap_buffers;
    backend->get_window_width = pt_glfw_get_window_width;
    backend->get_window_height = pt_glfw_get_window_height;
    backend->get_framebuffer_width = pt_glfw_get_framebuffer_width;
    backend->get_framebuffer_height = pt_glfw_get_framebuffer_height;
    backend->get_usable_width = pt_glfw_get_framebuffer_width;
    backend->get_usable_height = pt_glfw_get_framebuffer_height;
    backend->get_usable_xoffset = pt_glfw_offset_zero;
    backend->get_usable_yoffset = pt_glfw_offset_zero;
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

    PtWindow *window = PT_ALLOC(PtWindow);
    PtGlfwHandle *handle = PT_ALLOC(PtGlfwHandle);
    
    handle->glfw = glfwCreateWindow(width, height, title, NULL, NULL);
    handle->window_width = width;
    handle->window_height = height;
    handle->vsync_enabled = (flags & PT_FLAG_VSYNC) != 0;

    // Get initial framebuffer size
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
