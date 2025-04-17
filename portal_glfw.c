#include "portal_glfw.h"
#include "portal.h"
#include "glfw/include/GLFW/glfw3.h"
#include <stdio.h>
#include <stdlib.h>

PtBackend *pt_glfw_create() {
    PtBackend *backend = PT_ALLOC(PtBackend);
    backend->type = PT_BACKEND_GLFW;
    backend->capabilities = PT_CAPABILITY_CREATE_WINDOW;

    backend->init = pt_glfw_init;
    backend->shutdown = pt_glfw_shutdown;
    backend->create_window = pt_glfw_create_window;
    backend->destroy_window = pt_glfw_destroy_window;
    backend->poll_events = pt_glfw_poll_events;
    backend->swap_buffers = pt_glfw_swap_buffers;
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

    PT_ASSERT(window->handle != NULL);
    return window;
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