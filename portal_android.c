#include "portal_android.h"
#include "portal.h"
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/native_activity.h>
#include <android_native_app_glue.h>
#include <android/log.h>
#include <android/input.h>
#include <EGL/egl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <jni.h>

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
    int display_width;
    int display_height;
    int usable_width;
    int usable_height;
    int usable_x_offset;
    int usable_y_offset;
    PT_BOOL vsync_enabled;
} PtAndroidData;

static struct android_app* pt_internal_android_app = NULL;
static PtAndroidData *android_data = NULL;

void pt_android_configure_fullscreen(struct android_app* state) {
    JNIEnv* env = NULL;
    (*state->activity->vm)->AttachCurrentThread(state->activity->vm, &env, NULL);

    jclass activityClass = (*env)->FindClass(env, "android/app/NativeActivity");
    jmethodID getWindow = (*env)->GetMethodID(env, activityClass, "getWindow", "()Landroid/view/Window;");

    jclass windowClass = (*env)->FindClass(env, "android/view/Window");
    jmethodID getDecorView = (*env)->GetMethodID(env, windowClass, "getDecorView", "()Landroid/view/View;");

    jclass viewClass = (*env)->FindClass(env, "android/view/View");
    jmethodID setSystemUiVisibility = (*env)->GetMethodID(env, viewClass, "setSystemUiVisibility", "(I)V");

    jobject window = (*env)->CallObjectMethod(env, state->activity->clazz, getWindow);

    jobject decorView = (*env)->CallObjectMethod(env, window, getDecorView);

    jfieldID flagFullscreenID = (*env)->GetStaticFieldID(env, viewClass, "SYSTEM_UI_FLAG_FULLSCREEN", "I");
    jfieldID flagHideNavigationID = (*env)->GetStaticFieldID(env, viewClass, "SYSTEM_UI_FLAG_HIDE_NAVIGATION", "I");
    jfieldID flagImmersiveStickyID = (*env)->GetStaticFieldID(env, viewClass, "SYSTEM_UI_FLAG_IMMERSIVE_STICKY", "I");

    const int flagFullscreen = (*env)->GetStaticIntField(env, viewClass, flagFullscreenID);
    const int flagHideNavigation = (*env)->GetStaticIntField(env, viewClass, flagHideNavigationID);
    const int flagImmersiveSticky = (*env)->GetStaticIntField(env, viewClass, flagImmersiveStickyID);
    const int flag = flagFullscreen | flagHideNavigation | flagImmersiveSticky;

    (*env)->CallVoidMethod(env, decorView, setSystemUiVisibility, flag);

    jclass layoutParamsClass = (*env)->FindClass(env, "android/view/WindowManager$LayoutParams");
    jfieldID cutoutModeField = (*env)->GetStaticFieldID(env, layoutParamsClass, "LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES", "I");

    if (cutoutModeField != NULL && !(*env)->ExceptionCheck(env)) {
        jint cutoutMode = (*env)->GetStaticIntField(env, layoutParamsClass, cutoutModeField);

        jmethodID getAttributesMethod = (*env)->GetMethodID(env, windowClass, "getAttributes", "()Landroid/view/WindowManager$LayoutParams;");
        jobject layoutParams = (*env)->CallObjectMethod(env, window, getAttributesMethod);

        jfieldID layoutInDisplayCutoutModeField = (*env)->GetFieldID(env, layoutParamsClass, "layoutInDisplayCutoutMode", "I");
        (*env)->SetIntField(env, layoutParams, layoutInDisplayCutoutModeField, cutoutMode);

        jmethodID setAttributesMethod = (*env)->GetMethodID(env, windowClass, "setAttributes", "(Landroid/view/WindowManager$LayoutParams;)V");
        (*env)->CallVoidMethod(env, window, setAttributesMethod, layoutParams);
    }

    (*env)->ExceptionClear(env);

    (*state->activity->vm)->DetachCurrentThread(state->activity->vm);
}

static void pt_android_get_real_display_size() {
    if (android_data == NULL || android_data->activity == NULL) {
        LOGE("no activity");
        return;
    }

    ANativeActivity* activity = android_data->activity;
    JNIEnv* env = NULL;

    (*activity->vm)->AttachCurrentThread(activity->vm, &env, NULL);
    if (env == NULL) {
        LOGE("Failed to get JNI environment for display info");
        return;
    }

    jobject activityObject = activity->clazz;
    jclass activityClass = (*env)->GetObjectClass(env, activityObject);

    jmethodID getSystemServiceMethod = (*env)->GetMethodID(env, activityClass, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
    jstring windowServiceString = (*env)->NewStringUTF(env, "window");
    jobject windowManager = (*env)->CallObjectMethod(env, activityObject, getSystemServiceMethod, windowServiceString);

    if (windowManager != NULL) {
        jclass windowManagerClass = (*env)->GetObjectClass(env, windowManager);
        jmethodID getDefaultDisplayMethod = (*env)->GetMethodID(env, windowManagerClass, "getDefaultDisplay", "()Landroid/view/Display;");
        jobject display = (*env)->CallObjectMethod(env, windowManager, getDefaultDisplayMethod);

        if (display != NULL) {
            jclass displayClass = (*env)->GetObjectClass(env, display);
            jmethodID getRealSizeMethod = (*env)->GetMethodID(env, displayClass, "getRealSize", "(Landroid/graphics/Point;)V");

            if (getRealSizeMethod != NULL) {
                jclass pointClass = (*env)->FindClass(env, "android/graphics/Point");
                jmethodID pointConstructor = (*env)->GetMethodID(env, pointClass, "<init>", "()V");
                jobject point = (*env)->NewObject(env, pointClass, pointConstructor);

                (*env)->CallVoidMethod(env, display, getRealSizeMethod, point);

                jfieldID xField = (*env)->GetFieldID(env, pointClass, "x", "I");
                jfieldID yField = (*env)->GetFieldID(env, pointClass, "y", "I");

                android_data->display_width = (*env)->GetIntField(env, point, xField);
                android_data->display_height = (*env)->GetIntField(env, point, yField);

                LOGI("Real display size: %dx%d", android_data->display_width, android_data->display_height);

                (*env)->DeleteLocalRef(env, point);
                (*env)->DeleteLocalRef(env, pointClass);
            } else {
                jmethodID getSizeMethod = (*env)->GetMethodID(env, displayClass, "getSize", "(Landroid/graphics/Point;)V");
                jclass pointClass = (*env)->FindClass(env, "android/graphics/Point");
                jmethodID pointConstructor = (*env)->GetMethodID(env, pointClass, "<init>", "()V");
                jobject point = (*env)->NewObject(env, pointClass, pointConstructor);

                (*env)->CallVoidMethod(env, display, getSizeMethod, point);

                jfieldID xField = (*env)->GetFieldID(env, pointClass, "x", "I");
                jfieldID yField = (*env)->GetFieldID(env, pointClass, "y", "I");

                android_data->display_width = (*env)->GetIntField(env, point, xField);
                android_data->display_height = (*env)->GetIntField(env, point, yField);

                LOGI("Display size (fallback): %dx%d", android_data->display_width, android_data->display_height);

                (*env)->DeleteLocalRef(env, point);
                (*env)->DeleteLocalRef(env, pointClass);
            }

            (*env)->DeleteLocalRef(env, displayClass);
            (*env)->DeleteLocalRef(env, display);
        }

        (*env)->DeleteLocalRef(env, windowManagerClass);
        (*env)->DeleteLocalRef(env, windowManager);
    }

    (*env)->DeleteLocalRef(env, windowServiceString);
    (*env)->DeleteLocalRef(env, activityClass);

    (*activity->vm)->DetachCurrentThread(activity->vm);
}

PT_BOOL pt_android_init_egl() {
    if (android_data == NULL) {
        LOGE("Android data not initialized");
        return PT_FALSE;
    }

    if (android_data->native_window == NULL) {
        LOGE("Native window is NULL");
        return PT_FALSE;
    }

    pt_android_get_real_display_size();

    ANativeWindow_setBuffersGeometry(android_data->native_window, android_data->display_width, android_data->display_height, WINDOW_FORMAT_RGBA_8888);

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
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
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
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_CONTEXT_MINOR_VERSION, 2,
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

    eglSwapInterval(android_data->display, android_data->vsync_enabled ? 1 : 0);

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
            android_data->display_width = 0;
            android_data->display_height = 0;
            android_data->vsync_enabled = PT_TRUE;
        }

        android_data->native_window = app->window;
        android_data->activity = app->activity;

        if (android_data->display != EGL_NO_DISPLAY &&
            android_data->context != EGL_NO_CONTEXT &&
            android_data->surface == EGL_NO_SURFACE) {

            LOGI("Recreating EGL surface after minimization");

            pt_android_get_real_display_size();

            android_data->surface = eglCreateWindowSurface(android_data->display,
                                                         android_data->config,
                                                         android_data->native_window,
                                                         NULL);

            if (android_data->surface != EGL_NO_SURFACE) {
                if (eglMakeCurrent(android_data->display,
                                android_data->surface,
                                android_data->surface,
                                android_data->context)) {
                    eglSwapInterval(android_data->display, android_data->vsync_enabled ? 1 : 0);
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

    while (ALooper_pollOnce(0, NULL, &events, (void**)&source) > ALOOPER_POLL_TIMEOUT) {
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

    pt_android_configure_fullscreen(app);

    app->onAppCmd = pt_android_handle_cmd;
    app->onInputEvent = pt_android_handle_input;
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

int pt_android_handle_input(struct android_app* app, AInputEvent* event) {
    if (AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION) return 0;

    int32_t action = AMotionEvent_getAction(event);
    int32_t pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
    int32_t pointerId = AMotionEvent_getPointerId(event, pointerIndex);
    float x = AMotionEvent_getX(event, pointerIndex);
    float y = AMotionEvent_getY(event, pointerIndex);

    PtWindow* window = (PtWindow*)app->userData;
    PT_ASSERT(window != NULL);

    PtInputEventData ptEvent = pt_create_input_event_data();
    ptEvent.touch.finger = pointerId;
    ptEvent.touch.x = (int)x;
    ptEvent.touch.y = (int)y;

    switch (action & AMOTION_EVENT_ACTION_MASK) {
        case AMOTION_EVENT_ACTION_DOWN:
        case AMOTION_EVENT_ACTION_POINTER_DOWN:
            ptEvent.type = PT_INPUT_EVENT_TOUCHDOWN;
            break;
        case AMOTION_EVENT_ACTION_UP:
        case AMOTION_EVENT_ACTION_POINTER_UP:
            ptEvent.type = PT_INPUT_EVENT_TOUCHUP;
            break;
        case AMOTION_EVENT_ACTION_MOVE:
            ptEvent.type = PT_INPUT_EVENT_TOUCHMOVE;
            for (size_t i = 0; i < AMotionEvent_getPointerCount(event); ++i) {
                PtInputEventData moveEvent = pt_create_input_event_data();
                moveEvent.type = PT_INPUT_EVENT_TOUCHMOVE;
                moveEvent.touch.finger = AMotionEvent_getPointerId(event, i);
                moveEvent.touch.x = (int)AMotionEvent_getX(event, i);
                moveEvent.touch.y = (int)AMotionEvent_getY(event, i);
                pt_push_input_event(window, moveEvent);
            }
            return 1;
        default:
            return 0;
    }

    pt_push_input_event(window, ptEvent);
    return 1;
}

void pt_android_set_native_window(ANativeWindow *native_window, ANativeActivity *activity) {
    LOGI("External request to set native window to %p", native_window);

    if (android_data == NULL) {
        android_data = PT_ALLOC(PtAndroidData);
        memset(android_data, 0, sizeof(PtAndroidData));
        android_data->display = EGL_NO_DISPLAY;
        android_data->surface = EGL_NO_SURFACE;
        android_data->context = EGL_NO_CONTEXT;
        android_data->display_width = 0;
        android_data->display_height = 0;
        android_data->vsync_enabled = PT_TRUE;
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

void* pt_android_get_handle(PtWindow *window) {
    PT_ASSERT(window != NULL);

    if (android_data && android_data->native_window) {
        return android_data->native_window;
    }

    return NULL;
}

PtBackend *pt_android_create() {
    PtBackend *backend = PT_ALLOC(PtBackend);
    backend->type = PT_BACKEND_ANDROID;
    backend->kind = PT_BACKEND_KIND_MOBILE;
    backend->capabilities = PT_CAPABILITY_CREATE_WINDOW;
    backend->input_event_count = 0;

    backend->init = pt_android_init;
    backend->shutdown = pt_android_shutdown;
    backend->get_handle = pt_android_get_handle;
    backend->create_window = pt_android_create_window;
    backend->destroy_window = pt_android_destroy_window;
    backend->poll_events = pt_android_poll_events;
    backend->swap_buffers = pt_android_swap_buffers;
    backend->set_window_title = pt_android_set_window_title;
    backend->set_window_size = pt_android_set_window_size;
    backend->set_video_mode = pt_android_set_video_mode;
    backend->show_window = pt_android_show_window;
    backend->hide_window = pt_android_hide_window;
    backend->minimize_window = pt_android_minimize_window;
    backend->maximize_window = pt_android_maximize_window;
    backend->restore_window = pt_android_restore_window;
    backend->focus_window = pt_android_focus_window;
    backend->get_window_width = pt_android_get_window_width;
    backend->get_window_height = pt_android_get_window_height;
    backend->get_framebuffer_width = pt_android_get_framebuffer_width;
    backend->get_framebuffer_height = pt_android_get_framebuffer_height;
    backend->get_usable_width = pt_android_get_usable_framebuffer_width;
    backend->get_usable_height = pt_android_get_usable_framebuffer_height;
    backend->get_usable_xoffset = pt_android_get_usable_framebuffer_xoffset;
    backend->get_usable_yoffset = pt_android_get_usable_framebuffer_yoffset;
    backend->is_window_maximized = pt_android_is_window_maximized;
    backend->is_window_minimized = pt_android_is_window_minimized;
    backend->is_window_focused = pt_android_is_window_focused;
    backend->is_window_visible = pt_android_is_window_visible;
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

PtWindow* pt_android_create_window(const char *title, int width, int height, PtWindowFlags flags) {
    PT_ASSERT(title != NULL);

    PtWindow *window = PT_ALLOC(PtWindow);

    if (android_data == NULL) {
        android_data = PT_ALLOC(PtAndroidData);
        memset(android_data, 0, sizeof(PtAndroidData));
        android_data->display = EGL_NO_DISPLAY;
        android_data->surface = EGL_NO_SURFACE;
        android_data->context = EGL_NO_CONTEXT;
        android_data->display_width = 0;
        android_data->display_height = 0;
    }

    android_data->vsync_enabled = (flags & PT_FLAG_VSYNC) != 0;

    window->handle = android_data;
    window->throttle_enabled = PT_FALSE;
    window->target_fps = 60;
    window->last_frame_time = 0.0;
    window->frame_duration = 1.0 / 60.0;

    pt_internal_android_app->userData = window;

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
        PT_BOOL result = eglMakeCurrent(android_data->display, android_data->surface,
                             android_data->surface, android_data->context);
        if (result) {
            eglSwapInterval(android_data->display, android_data->vsync_enabled ? 1 : 0);
        }
        return result;
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

    if (android_data) {
        if (android_data->display_width > 0) {
            return android_data->display_width;
        }

        if (android_data->native_window) {
            return ANativeWindow_getWidth(android_data->native_window);
        }
    }

    return 0;
}

int pt_android_get_window_height(PtWindow *window) {
    PT_ASSERT(window != NULL);

    if (android_data) {
        if (android_data->display_height > 0) {
            return android_data->display_height;
        }

        if (android_data->native_window) {
            return ANativeWindow_getHeight(android_data->native_window);
        }
    }

    return 0;
}

int pt_android_get_framebuffer_width(PtWindow *window) {
    return pt_android_get_window_width(window);
}

int pt_android_get_framebuffer_height(PtWindow *window) {
    return pt_android_get_window_height(window);
}

static void pt_android_get_display_cutout_info() {
    if (android_data == NULL || android_data->activity == NULL) {
        LOGE("no activity");
        return;
    }

    android_data->usable_width = android_data->display_width;
    android_data->usable_height = android_data->display_height;
    android_data->usable_x_offset = 0;
    android_data->usable_y_offset = 0;

    ANativeActivity* activity = android_data->activity;
    JNIEnv* env = NULL;

    (*activity->vm)->AttachCurrentThread(activity->vm, &env, NULL);
    if (env == NULL) {
        LOGE("Failed to get JNI environment for cutout info");
        return;
    }

    jobject activityObject = activity->clazz;
    jclass activityClass = (*env)->GetObjectClass(env, activityObject);

    jmethodID getWindowMethod = (*env)->GetMethodID(env, activityClass, "getWindow", "()Landroid/view/Window;");
    jobject window = (*env)->CallObjectMethod(env, activityObject, getWindowMethod);

    if (window != NULL) {
        jclass windowClass = (*env)->GetObjectClass(env, window);
        jmethodID getDecorViewMethod = (*env)->GetMethodID(env, windowClass, "getDecorView", "()Landroid/view/View;");
        jobject decorView = (*env)->CallObjectMethod(env, window, getDecorViewMethod);

        if (decorView != NULL) {
            jclass viewClass = (*env)->GetObjectClass(env, decorView);
            jmethodID getRootWindowInsetsMethod = (*env)->GetMethodID(env, viewClass, "getRootWindowInsets", "()Landroid/view/WindowInsets;");

            if (getRootWindowInsetsMethod != NULL && !(*env)->ExceptionCheck(env)) {
                jobject windowInsets = (*env)->CallObjectMethod(env, decorView, getRootWindowInsetsMethod);

                if (windowInsets != NULL && !(*env)->ExceptionCheck(env)) {
                    jclass windowInsetsClass = (*env)->GetObjectClass(env, windowInsets);
                    jmethodID getDisplayCutoutMethod = (*env)->GetMethodID(env, windowInsetsClass, "getDisplayCutout", "()Landroid/view/DisplayCutout;");

                    if (getDisplayCutoutMethod != NULL && !(*env)->ExceptionCheck(env)) {
                        jobject displayCutout = (*env)->CallObjectMethod(env, windowInsets, getDisplayCutoutMethod);

                        if (displayCutout != NULL && !(*env)->ExceptionCheck(env)) {
                            jclass displayCutoutClass = (*env)->GetObjectClass(env, displayCutout);

                            jmethodID getSafeInsetLeftMethod = (*env)->GetMethodID(env, displayCutoutClass, "getSafeInsetLeft", "()I");
                            jmethodID getSafeInsetTopMethod = (*env)->GetMethodID(env, displayCutoutClass, "getSafeInsetTop", "()I");
                            jmethodID getSafeInsetRightMethod = (*env)->GetMethodID(env, displayCutoutClass, "getSafeInsetRight", "()I");
                            jmethodID getSafeInsetBottomMethod = (*env)->GetMethodID(env, displayCutoutClass, "getSafeInsetBottom", "()I");

                            if (getSafeInsetLeftMethod != NULL && getSafeInsetTopMethod != NULL &&
                                getSafeInsetRightMethod != NULL && getSafeInsetBottomMethod != NULL) {

                                jint leftInset = (*env)->CallIntMethod(env, displayCutout, getSafeInsetLeftMethod);
                                jint topInset = (*env)->CallIntMethod(env, displayCutout, getSafeInsetTopMethod);
                                jint rightInset = (*env)->CallIntMethod(env, displayCutout, getSafeInsetRightMethod);
                                jint bottomInset = (*env)->CallIntMethod(env, displayCutout, getSafeInsetBottomMethod);

                                android_data->usable_x_offset = leftInset;
                                android_data->usable_y_offset = topInset;
                                android_data->usable_width = android_data->display_width - leftInset - rightInset;
                                android_data->usable_height = android_data->display_height - topInset - bottomInset;

                                LOGI("Display cutout insets - left: %d, top: %d, right: %d, bottom: %d",
                                     leftInset, topInset, rightInset, bottomInset);
                                LOGI("Usable area: %dx%d at offset (%d, %d)",
                                     android_data->usable_width, android_data->usable_height,
                                     android_data->usable_x_offset, android_data->usable_y_offset);
                            }

                            (*env)->DeleteLocalRef(env, displayCutoutClass);
                            (*env)->DeleteLocalRef(env, displayCutout);
                        }
                    }

                    (*env)->DeleteLocalRef(env, windowInsetsClass);
                    (*env)->DeleteLocalRef(env, windowInsets);
                }
            }

            (*env)->DeleteLocalRef(env, viewClass);
            (*env)->DeleteLocalRef(env, decorView);
        }

        (*env)->DeleteLocalRef(env, windowClass);
        (*env)->DeleteLocalRef(env, window);
    }

    if ((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionClear(env);
    }

    (*env)->DeleteLocalRef(env, activityClass);
    (*activity->vm)->DetachCurrentThread(activity->vm);
}

int pt_android_get_usable_framebuffer_width(PtWindow *window) {
    PT_ASSERT(window != NULL);

    if (android_data) {
        if (android_data->usable_width == 0) {
            pt_android_get_display_cutout_info();
        }
        return android_data->usable_width;
    }

    return 0;
}

int pt_android_get_usable_framebuffer_height(PtWindow *window) {
    PT_ASSERT(window != NULL);

    if (android_data) {
        if (android_data->usable_height == 0) {
            pt_android_get_display_cutout_info();
        }
        return android_data->usable_height;
    }

    return 0;
}

int pt_android_get_usable_framebuffer_xoffset(PtWindow *window) {
    PT_ASSERT(window != NULL);

    if (android_data) {
        if (android_data->usable_width == 0) {
            pt_android_get_display_cutout_info();
        }
        return android_data->usable_x_offset;
    }

    return 0;
}

int pt_android_get_usable_framebuffer_yoffset(PtWindow *window) {
    PT_ASSERT(window != NULL);

    if (android_data) {
        if (android_data->usable_height == 0) {
            pt_android_get_display_cutout_info();
        }
        return android_data->usable_y_offset;
    }

    return 0;
}

void pt_android_set_window_title(PtWindow *window, const char *title) {
    PT_ASSERT(window != NULL);
    PT_ASSERT(title != NULL);
    // noop
}

void pt_android_set_window_size(PtWindow *window, int width, int height) {
    PT_ASSERT(window != NULL);
    PT_ASSERT(width > 0);
    PT_ASSERT(height > 0);
    // noop
}

void pt_android_set_video_mode(PtWindow *window, PtVideoMode mode) {
    PT_ASSERT(window != NULL);
    // noop
}

void pt_android_show_window(PtWindow *window) {
    PT_ASSERT(window != NULL);
    // noop
}

void pt_android_hide_window(PtWindow *window) {
    PT_ASSERT(window != NULL);
    // unsupported
}

void pt_android_minimize_window(PtWindow *window) {
    PT_ASSERT(window != NULL);
    // unsupported
}

void pt_android_maximize_window(PtWindow *window) {
    PT_ASSERT(window != NULL);
    // unsupported
}

void pt_android_restore_window(PtWindow *window) {
    PT_ASSERT(window != NULL);
    // unsupported
}

void pt_android_focus_window(PtWindow *window) {
    PT_ASSERT(window != NULL);
    // unsupported
}

PT_BOOL pt_android_is_window_maximized(PtWindow *window) {
    PT_ASSERT(window != NULL);
    return PT_TRUE; // sane default
}

PT_BOOL pt_android_is_window_minimized(PtWindow *window) {
    PT_ASSERT(window != NULL);
    return PT_FALSE; // sane default
}

PT_BOOL pt_android_is_window_focused(PtWindow *window) {
    PT_ASSERT(window != NULL);
    return PT_TRUE; // sane default
}

PT_BOOL pt_android_is_window_visible(PtWindow *window) {
    PT_ASSERT(window != NULL);
    return PT_TRUE; // sane default
}
