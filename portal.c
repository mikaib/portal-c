#include "portal.h"
#include "portal_noop.h"
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

    return PT_BACKEND_NOOP;
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

        case PT_BACKEND_NOOP:
            return pt_noop_create();

        default:
            printf("Unknown backend type: %d\n", type);
    }

    return NULL;
}

int pt_get_input_event_count(PtWindow *window) {
    PT_ASSERT(active_config != NULL);
    PT_ASSERT(active_config->backend != NULL);

    return active_config->backend->input_event_count;
}

PtInputEventData pt_create_input_event_data() {
    PtInputEventData event;

    event.type = PT_INPUT_EVENT_NONE;
    event.mouse.button = 0;
    event.mouse.x = 0;
    event.mouse.y = 0;
    event.mouse.dx = 0;
    event.mouse.dy = 0;
    event.mouse.modifiers = 0;
    event.key.key = 0;
    event.key.modifiers = 0;
    event.touch.finger = 0;
    event.touch.x = 0;
    event.touch.y = 0;
    event.text.codepoint = 0;
    event.timestamp = 0.0;

    return event;
}

PtInputEventData pt_pull_input_event(PtWindow *window) {
    PT_ASSERT(active_config != NULL);
    PT_ASSERT(active_config->backend != NULL);

    if (active_config->backend->input_event_count > 0) {
        PtInputEventData event = active_config->backend->input_events[0];

        for (int i = 1; i < active_config->backend->input_event_count; i++) {
            active_config->backend->input_events[i - 1] = active_config->backend->input_events[i];
        }

        active_config->backend->input_event_count--;
        return event;
    }

    return pt_create_input_event_data();
}

void pt_push_input_event(PtWindow *window, PtInputEventData event) {
    PT_ASSERT(active_config != NULL);
    PT_ASSERT(active_config->backend != NULL);

    if (active_config->backend->input_event_count >= PT_MAX_EVENT_COUNT) {
        return;
    }

    active_config->backend->input_events[active_config->backend->input_event_count++] = event;
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

int pt_get_framebuffer_width(PtWindow *window) {
    PT_ASSERT(active_config != NULL);
    PT_ASSERT(active_config->backend != NULL);

    return active_config->backend->get_framebuffer_width(window);
}

int pt_get_framebuffer_height(PtWindow *window) {
    PT_ASSERT(active_config != NULL);
    PT_ASSERT(active_config->backend != NULL);

    return active_config->backend->get_framebuffer_height(window);
}

void pt_shutdown() {
    PT_ASSERT(active_config != NULL);
    PT_ASSERT(active_config->backend != NULL);

    active_config->backend->shutdown(active_config->backend);
    active_config = NULL;
}