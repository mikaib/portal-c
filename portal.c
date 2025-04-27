#include "portal.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef PT_GLFW
#include "portal_glfw.h"
#endif

#ifdef PT_ANDROID
#include "portal_android.h"
#endif

static PtConfig *active_config = NULL;

PtConfig *pt_create_config() {
    PtConfig *config = PT_ALLOC(PtConfig);
    config->backend = NULL;

    return config;
}

void pt_destroy_config(PtConfig *config) {
    PT_ASSERT(config != NULL);
    PT_ASSERT(active_config == NULL || active_config != config);

    PT_FREE(config);
}

PtBackendType pt_get_optimal_backend_type() {
    #if PT_ANDROID
        return PT_BACKEND_ANDROID;
    #endif

    #if PT_GLFW
        return PT_BACKEND_GLFW;
    #endif
}

void pt_destroy_backend(PtBackend *backend) {
    PT_ASSERT(backend != NULL);
    PT_FREE(backend);
}

PtBackend *pt_create_backend(PtBackendType type) {
    switch (type) {
        #ifdef PT_GLFW
        case PT_BACKEND_GLFW:
            return pt_glfw_create();
        #endif

        #ifdef PT_ANDROID
        case PT_BACKEND_ANDROID:
            return pt_android_create();
        #endif

        default:
            printf("Unknown backend type: %d\n", type);
    }

    return NULL;
}

PtWindow* pt_create_window(const char *title, int width, int height) {
    PT_ASSERT(active_config != NULL);
    PT_ASSERT(active_config->backend != NULL);

    return active_config->backend->create_window(title, width, height);
}

void pt_destroy_window(PtWindow *window) {
    PT_ASSERT(active_config != NULL);
    PT_ASSERT(active_config->backend != NULL);

    active_config->backend->destroy_window(window);
}

void pt_poll_events(PtWindow *window) {
    PT_ASSERT(active_config != NULL);
    PT_ASSERT(active_config->backend != NULL);

    active_config->backend->poll_events(window);
}

void pt_swap_buffers(PtWindow *window) {
    PT_ASSERT(active_config != NULL);
    PT_ASSERT(active_config->backend != NULL);

    active_config->backend->swap_buffers(window);
}

PT_BOOL pt_use_gl_context(PtWindow *window) {
    PT_ASSERT(active_config != NULL);

    return active_config->backend->use_gl_context(window);
}

PT_BOOL pt_init(PtConfig *config) {
    PT_ASSERT(config != NULL);
    PT_ASSERT(config->backend != NULL);
    PT_ASSERT(active_config == NULL);

    if (!config->backend->init(config->backend, config)) {
        return PT_FALSE;
    }

    active_config = config;
    return PT_TRUE;
}

PT_BOOL pt_backend_supports(PtBackend *backend, PtCapability capability) {
    PT_ASSERT(backend != NULL);

    return (backend->capabilities & capability) == capability;
}

PT_BOOL pt_should_window_close(PtWindow *window) {
    PT_ASSERT(active_config != NULL);
    PT_ASSERT(active_config->backend != NULL);

    return active_config->backend->should_window_close(window);
}

int pt_get_window_width(PtWindow *window) {
    PT_ASSERT(active_config != NULL);
    PT_ASSERT(active_config->backend != NULL);

    return active_config->backend->get_window_width(window);
}

int pt_get_window_height(PtWindow *window) {
    PT_ASSERT(active_config != NULL);
    PT_ASSERT(active_config->backend != NULL);

    return active_config->backend->get_window_height(window);
}

void pt_shutdown() {
    PT_ASSERT(active_config != NULL);
    PT_ASSERT(active_config->backend != NULL);

    active_config->backend->shutdown(active_config->backend);
    active_config = NULL;
}