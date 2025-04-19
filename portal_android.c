#include "portal_android.h"
#include "portal.h"
#include <android/native_window.h>
#include <android/native_activity.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct {
    ANativeWindow* native_window;
    ANativeActivity* activity;
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    EGLConfig config;
    int should_close;
} PtAndroidData;

PtBackend *pt_android_create() {
    PtBackend *backend = PT_ALLOC(PtBackend);
    backend->type = PT_BACKEND_ANDROID;
    backend->capabilities = PT_CAPABILITY_CREATE_WINDOW;

    backend->init = pt_android_init;
    backend->shutdown = pt_android_shutdown;
    backend->create_window = pt_android_create_window;
    backend->destroy_window = pt_android_destroy_window;
    backend->poll_events = pt_android_poll_events;
    backend->swap_buffers = pt_android_swap_buffers;
    backend->use_gl_context = pt_android_use_gl_context;
    backend->should_window_close = pt_android_should_window_close;

    return backend;
}

PT_BOOL pt_android_init(PtBackend *backend, PtConfig *config) {
    PT_ASSERT(config != NULL);
    PT_ASSERT(backend != NULL);

    config->backend = backend;
    return PT_TRUE;
}

void pt_android_shutdown(PtBackend *backend) {
    PT_ASSERT(backend != NULL);
}

PtWindow* pt_android_create_window(const char *title, int width, int height) {
    PT_ASSERT(title != NULL);

    PtWindow *window = PT_ALLOC(PtWindow);
    PtAndroidData *android_data = PT_ALLOC(PtAndroidData);

    window->handle = android_data;
    android_data->should_close = 0;

    return window;
}

void pt_android_set_native_window(PtWindow *window, ANativeWindow *native_window, ANativeActivity *activity) {
    PT_ASSERT(window != NULL);
    PT_ASSERT(native_window != NULL);

    PtAndroidData *android_data = (PtAndroidData*)window->handle;
    android_data->native_window = native_window;
    android_data->activity = activity;
    android_data->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    if (android_data->display == EGL_NO_DISPLAY) {
        __android_log_print(ANDROID_LOG_ERROR, "PortalEngine", "Unable to get EGL display");
        return;
    }

    if (!eglInitialize(android_data->display, NULL, NULL)) {
        __android_log_print(ANDROID_LOG_ERROR, "PortalEngine", "Unable to initialize EGL");
        return;
    }

    const EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_STENCIL_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    EGLint num_configs;
    if (!eglChooseConfig(android_data->display, attribs, &android_data->config, 1, &num_configs) || num_configs <= 0) {
        __android_log_print(ANDROID_LOG_ERROR, "PortalEngine", "Failed to choose EGL config");
        return;
    }

    const EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    android_data->context = eglCreateContext(android_data->display, android_data->config, EGL_NO_CONTEXT, context_attribs);
    if (android_data->context == EGL_NO_CONTEXT) {
        __android_log_print(ANDROID_LOG_ERROR, "Portal", "Failed to create EGL context");
        return;
    }

    android_data->surface = eglCreateWindowSurface(android_data->display, android_data->config, android_data->native_window, NULL);
    if (android_data->surface == EGL_NO_SURFACE) {
        __android_log_print(ANDROID_LOG_ERROR, "Portal", "Failed to create EGL surface");
        return;
    }

    if (!eglMakeCurrent(android_data->display, android_data->surface, android_data->surface, android_data->context)) {
        __android_log_print(ANDROID_LOG_ERROR, "Portal", "Failed to make EGL context current");
        return;
    }
}

void pt_android_destroy_window(PtWindow *window) {
    PT_ASSERT(window != NULL);

    PtAndroidData *android_data = (PtAndroidData*)window->handle;

    if (android_data->display != EGL_NO_DISPLAY) {
        eglMakeCurrent(android_data->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        if (android_data->surface != EGL_NO_SURFACE) {
            eglDestroySurface(android_data->display, android_data->surface);
        }

        if (android_data->context != EGL_NO_CONTEXT) {
            eglDestroyContext(android_data->display, android_data->context);
        }

        eglTerminate(android_data->display);
    }

    PT_FREE(android_data);
    PT_FREE(window);
}

void pt_android_poll_events(PtWindow *window) {
    PT_ASSERT(window != NULL);
    // TODO: event queue
}

void pt_android_swap_buffers(PtWindow *window) {
    PT_ASSERT(window != NULL);

    PtAndroidData *android_data = (PtAndroidData*)window->handle;
    eglSwapBuffers(android_data->display, android_data->surface);
}

PT_BOOL pt_android_should_window_close(PtWindow *window) {
    PT_ASSERT(window != NULL);

    PtAndroidData *android_data = (PtAndroidData*)window->handle;
    return android_data->should_close;
}

PT_BOOL pt_android_use_gl_context(PtWindow *window) {
    PT_ASSERT(window != NULL);

    PtAndroidData *android_data = (PtAndroidData*)window->handle;

    if (!eglMakeCurrent(android_data->display, android_data->surface, android_data->surface, android_data->context)) {
        return PT_FALSE;
    }

    return PT_TRUE;
}

void pt_android_set_should_close(PtWindow *window, int should_close) {
    PT_ASSERT(window != NULL);

    PtAndroidData *android_data = (PtAndroidData*)window->handle;
    android_data->should_close = should_close;
}
