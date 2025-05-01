#include "portal_android.h"
#include "portal.h"
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/native_activity.h>
#include <android_native_app_glue.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

extern void hxcpp_main();

#define LOG_TAG "Portal"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

typedef struct {
    ANativeWindow* native_window;
    ANativeActivity* activity;
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    EGLConfig config;
    int should_close;
    int initialized;
    int pending_surface_destroy;
} PtAndroidData;

static struct android_app* pt_internal_android_app = NULL;
static PtAndroidData *android_data = NULL;

PT_BOOL pt_android_init_egl() {
    if (android_data == NULL) {
        LOGE("Android data not initialized");
        return PT_FALSE;
    }

    if (android_data->native_window == NULL) {
        LOGE("Native window is NULL");
        return PT_FALSE;
    }

    EGLint error = eglGetError();
    if (error != EGL_SUCCESS) {
        LOGI("Cleared EGL error: %d", error);
    }

    android_data->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (android_data->display == EGL_NO_DISPLAY) {
        LOGE("Unable to get EGL display: %d", eglGetError());
        return PT_FALSE;
    }

    EGLint major, minor;
    if (!eglInitialize(android_data->display, &major, &minor)) {
        LOGE("Unable to initialize EGL: %d", eglGetError());
        android_data->display = EGL_NO_DISPLAY;
        return PT_FALSE;
    }
    LOGI("EGL initialized: version %d.%d", major, minor);

    const EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 16,
        EGL_STENCIL_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    EGLint num_configs;
    if (!eglChooseConfig(android_data->display, attribs, &android_data->config, 1, &num_configs)) {
        LOGE("Failed to choose EGL config: %d", eglGetError());
        return PT_FALSE;
    }

    if (num_configs <= 0) {
        LOGE("No compatible EGL configs found");
        return PT_FALSE;
    }

    LOGI("Found %d matching EGL configs", num_configs);

    const EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    android_data->context = eglCreateContext(android_data->display,
                                            android_data->config,
                                            EGL_NO_CONTEXT,
                                            context_attribs);
    if (android_data->context == EGL_NO_CONTEXT) {
        LOGE("Failed to create EGL context: %d", eglGetError());
        return PT_FALSE;
    }

    android_data->surface = eglCreateWindowSurface(android_data->display,
                                                  android_data->config,
                                                  android_data->native_window,
                                                  NULL);
    if (android_data->surface == EGL_NO_SURFACE) {
        LOGE("Failed to create EGL surface: %d", eglGetError());
        return PT_FALSE;
    }

    if (!eglMakeCurrent(android_data->display,
                       android_data->surface,
                       android_data->surface,
                       android_data->context)) {
        LOGE("Failed to make EGL context current: %d", eglGetError());
        return PT_FALSE;
    }

    eglSwapInterval(android_data->display, 1);

    LOGI("EGL initialization successful");
    android_data->initialized = 1;
    android_data->pending_surface_destroy = 0;
    return PT_TRUE;
}

static void pt_android_handle_init(struct android_app* app) {
    LOGI("APP_CMD_INIT_WINDOW");
    if (app->window != NULL) {
        if (android_data == NULL) {
            android_data = PT_ALLOC(PtAndroidData);
            memset(android_data, 0, sizeof(PtAndroidData));
            android_data->display = EGL_NO_DISPLAY;
            android_data->surface = EGL_NO_SURFACE;
            android_data->context = EGL_NO_CONTEXT;
        }

        android_data->native_window = app->window;
        android_data->activity = app->activity;

        if (android_data->display != EGL_NO_DISPLAY &&
            android_data->context != EGL_NO_CONTEXT &&
            android_data->surface == EGL_NO_SURFACE) {

            LOGI("Recreating EGL surface after minimization");
            android_data->surface = eglCreateWindowSurface(android_data->display,
                                                         android_data->config,
                                                         android_data->native_window,
                                                         NULL);

            if (android_data->surface != EGL_NO_SURFACE) {
                if (eglMakeCurrent(android_data->display,
                                android_data->surface,
                                android_data->surface,
                                android_data->context)) {
                    LOGI("EGL context restored successfully");
                    android_data->initialized = 1;
                    android_data->pending_surface_destroy = 0;
                } else {
                    LOGE("Failed to restore EGL context: %d", eglGetError());
                }
            } else {
                LOGE("Failed to recreate EGL surface: %d", eglGetError());
            }
        } else if (!android_data->initialized) {
            if (pt_android_init_egl()) {
                LOGI("EGL initialized successfully, calling hxcpp_main");
                hxcpp_main();
            } else {
                LOGE("Failed to initialize EGL");
            }
        }
    }
}

static void pt_android_handle_cmd(struct android_app* app, int32_t cmd) {
    LOGD("Handling command: %d", cmd);

    switch (cmd) {
       case APP_CMD_INIT_WINDOW:
            pt_android_handle_init(app);
            break;

        case APP_CMD_TERM_WINDOW:
            LOGI("APP_CMD_TERM_WINDOW");
            if (android_data) {
                android_data->pending_surface_destroy = 1;
                LOGI("Surface marked for destruction on next swap");
            }
            break;
    }
}

void pt_android_internal_poll() {
    int events;
    struct android_poll_source* source;

    while (ALooper_pollAll(0, NULL, &events, (void**)&source) >= 0) {
        if (source != NULL) {
            source->process(pt_internal_android_app, source);
        }

        if (pt_internal_android_app->destroyRequested) {
            break;
        }
    }
}

void android_main(struct android_app* app) {
    LOGI("Android main entered");

    app->onAppCmd = pt_android_handle_cmd;
    pt_internal_android_app = app;

    while (!app->destroyRequested) {
        pt_android_internal_poll();
    }

    if (android_data) {
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
        android_data = NULL;
    }

    LOGI("Android main exiting");
}

void pt_android_set_native_window(ANativeWindow *native_window, ANativeActivity *activity) {
    LOGI("External request to set native window to %p", native_window);

    if (android_data == NULL) {
        android_data = PT_ALLOC(PtAndroidData);
        memset(android_data, 0, sizeof(PtAndroidData));
        android_data->display = EGL_NO_DISPLAY;
        android_data->surface = EGL_NO_SURFACE;
        android_data->context = EGL_NO_CONTEXT;
    }

    android_data->native_window = native_window;
    android_data->activity = activity;

    if (!android_data->initialized) {
        if (pt_android_init_egl()) {
            LOGI("EGL initialized from external request");
        } else {
            LOGE("Failed to initialize EGL from external request");
        }
    }
}

PtBackend *pt_android_create() {
    PtBackend *backend = PT_ALLOC(PtBackend);
    backend->type = PT_BACKEND_ANDROID;
    backend->kind = PT_BACKEND_KIND_MOBILE;
    backend->capabilities = PT_CAPABILITY_CREATE_WINDOW;

    backend->init = pt_android_init;
    backend->shutdown = pt_android_shutdown;
    backend->create_window = pt_android_create_window;
    backend->destroy_window = pt_android_destroy_window;
    backend->poll_events = pt_android_poll_events;
    backend->swap_buffers = pt_android_swap_buffers;
    backend->get_window_width = pt_android_get_window_width;
    backend->get_window_height = pt_android_get_window_height;
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

    window->handle = android_data;

    return window;
}

void pt_android_destroy_window(PtWindow *window) {
    PT_ASSERT(window != NULL);

    PT_FREE(window);
}

void pt_android_poll_events(PtWindow *window) {
    PT_ASSERT(window != NULL);

    if (android_data && android_data->activity) {
        pt_android_internal_poll();
    }
}

void pt_android_swap_buffers(PtWindow *window) {
    PT_ASSERT(window != NULL);

    if (android_data) {
        if (android_data->display != EGL_NO_DISPLAY && android_data->surface != EGL_NO_SURFACE) {
            eglSwapBuffers(android_data->display, android_data->surface);

            if (android_data->pending_surface_destroy) {
                LOGI("Processing delayed surface destruction after swap");
                eglMakeCurrent(android_data->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

                if (android_data->surface != EGL_NO_SURFACE) {
                    eglDestroySurface(android_data->display, android_data->surface);
                    android_data->surface = EGL_NO_SURFACE;
                }

                android_data->initialized = 0;
                android_data->pending_surface_destroy = 0;
                LOGI("Surface successfully destroyed after final frame");

                while (android_data->initialized == 0) {
                    pt_android_internal_poll();
                }
            }
        }

        EGLint error = eglGetError();
        if (error != EGL_SUCCESS) {
            LOGE("EGL error: %d", error);
        }
    }
}

PT_BOOL pt_android_should_window_close(PtWindow *window) {
    PT_ASSERT(window != NULL);

    if (android_data) {
        return android_data->should_close;
    }

    return PT_FALSE;
}

PT_BOOL pt_android_use_gl_context(PtWindow *window) {
    PT_ASSERT(window != NULL);

    if (android_data && android_data->display != EGL_NO_DISPLAY &&
        android_data->context != EGL_NO_CONTEXT &&
        android_data->surface != EGL_NO_SURFACE) {
        return eglMakeCurrent(android_data->display, android_data->surface,
                             android_data->surface, android_data->context);
    }

    return PT_FALSE;
}

void pt_android_handle_surface_changed(int width, int height) {
    LOGI("Surface changed: %dx%d", width, height);
}

void pt_android_set_should_close(int should_close) {
    if (android_data) {
        android_data->should_close = should_close;
    }
}

int pt_android_get_window_width(PtWindow *window) {
    PT_ASSERT(window != NULL);

    if (android_data && android_data->native_window) {
        return ANativeWindow_getWidth(android_data->native_window);
    }

    return 0;
}

int pt_android_get_window_height(PtWindow *window) {
    PT_ASSERT(window != NULL);

    if (android_data && android_data->native_window) {
        return ANativeWindow_getHeight(android_data->native_window);
    }

    return 0;
}