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
    backend->create_window = pt_glfw_create_window;
    backend->destroy_window = pt_glfw_destroy_window;
    backend->poll_events = pt_glfw_poll_events;
    backend->swap_buffers = pt_glfw_swap_buffers;
    backend->get_window_width = pt_glfw_get_window_width;
    backend->get_window_height = pt_glfw_get_window_height;
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

PtWindow* pt_glfw_create_window(const char *title, int width, int height) {
    PT_ASSERT(title != NULL);

    PtWindow *window = PT_ALLOC(PtWindow);
    window->handle = (void*)glfwCreateWindow(width, height, title, NULL, NULL);

    glfwSetMouseButtonCallback((GLFWwindow*)window->handle, (GLFWmousebuttonfun)pt_glfw_cb_mouse_button);
    glfwSetCursorPosCallback((GLFWwindow*)window->handle, (GLFWcursorposfun)pt_glfw_cb_mouse_move);
    glfwSetScrollCallback((GLFWwindow*)window->handle, (GLFWscrollfun)pt_glfw_cb_mouse_scroll);
    glfwSetKeyCallback((GLFWwindow*)window->handle, (GLFWkeyfun)pt_glfw_cb_key);
    glfwSetCharCallback((GLFWwindow*)window->handle, (GLFWcharfun)pt_glfw_cb_char);

    PT_ASSERT(window->handle != NULL);
    return window;
}

void pt_glfw_cb_mouse_button(PtWindow *window, int button, int action) {
    PT_ASSERT(window != NULL);

    PtInputEventData event = pt_create_input_event_data();
    event.type = action == GLFW_PRESS ? PT_INPUT_EVENT_MOUSEDOWN : PT_INPUT_EVENT_MOUSEUP;
    event.mouse.button = button;

    pt_push_input_event(window, event);
}

void pt_glfw_cb_mouse_move(PtWindow *window, double x, double y) {
    PT_ASSERT(window != NULL);

    PtInputEventData event = pt_create_input_event_data();
    event.type = PT_INPUT_EVENT_MOUSEMOVE;
    event.mouse.x = x;
    event.mouse.y = y;

    pt_push_input_event(window, event);
}

void pt_glfw_cb_mouse_scroll(PtWindow *window, double x, double y) {
    PT_ASSERT(window != NULL);

    PtInputEventData event = pt_create_input_event_data();
    event.type = PT_INPUT_EVENT_MOUSEWHEEL;
    event.mouse.x = x;
    event.mouse.y = y;

    pt_push_input_event(window, event);
}

void pt_glfw_cb_key(PtWindow *window, int key, int scancode, int action) {
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

void pt_glfw_cb_char(PtWindow *window, unsigned int codepoint) {
    PT_ASSERT(window != NULL);

    PtInputEventData event = pt_create_input_event_data();
    event.type = PT_INPUT_EVENT_TEXT;
    event.text.codepoint = codepoint;

    pt_push_input_event(window, event);
}

void pt_glfw_destroy_window(PtWindow *window) {
    PT_ASSERT(window->handle != NULL);

    glfwDestroyWindow((GLFWwindow*)window->handle);
    PT_FREE(window);
}

void pt_glfw_poll_events(PtWindow *window) {
    PT_ASSERT(window->handle != NULL);

    glfwPollEvents();
}

void pt_glfw_swap_buffers(PtWindow *window) {
    PT_ASSERT(window->handle != NULL);

    glfwSwapBuffers((GLFWwindow*)window->handle);
}

PT_BOOL pt_glfw_should_window_close(PtWindow *window) {
    PT_ASSERT(window->handle != NULL);

    return glfwWindowShouldClose((GLFWwindow*)window->handle);
}

PT_BOOL pt_glfw_use_gl_context(PtWindow *window) {
    PT_ASSERT(window->handle != NULL);

    glfwMakeContextCurrent((GLFWwindow*)window->handle);
    return PT_TRUE;
}

int pt_glfw_get_window_width(PtWindow *window) {
    PT_ASSERT(window->handle != NULL);

    int width, height;
    glfwGetWindowSize((GLFWwindow*)window->handle, &width, &height);
    return width;
}

int pt_glfw_get_window_height(PtWindow *window) {
    PT_ASSERT(window->handle != NULL);

    int width, height;
    glfwGetWindowSize((GLFWwindow*)window->handle, &width, &height);
    return height;
}