#include "portal.h"
#include "portal_noop.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#include <timeapi.h>
#else
#include <time.h>
#include <unistd.h>
#endif

#ifdef PT_GLFW
#include "portal_glfw.h"
#endif

#ifdef PT_ANDROID
#include "portal_android.h"
#endif

static PtConfig *active_config = NULL;

#ifdef _WIN32
static PT_BOOL high_precision_timer_init = PT_FALSE;
#endif

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

PtWindow* pt_create_window(const char *title, int width, int height, PtWindowFlags flags) {
    PT_ASSERT(active_config != NULL);
    PT_ASSERT(active_config->backend != NULL);

    PtWindow *window = active_config->backend->create_window(title, width, height, flags);
    return window;
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
    PT_ASSERT(window != NULL);

    active_config->backend->swap_buffers(window);

    if (window->throttle_enabled) {
        double current_time = pt_get_time();
        double elapsed = current_time - window->last_frame_time;
        double sleep_time = window->frame_duration - elapsed;

        if (sleep_time > 0.0) {
            pt_sleep(sleep_time);
        }

        window->last_frame_time = pt_get_time();
    }
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

int pt_get_usable_width(PtWindow *window) {
    PT_ASSERT(active_config != NULL);
    PT_ASSERT(active_config->backend != NULL);

    return active_config->backend->get_usable_width(window);
}

int pt_get_usable_height(PtWindow *window) {
    PT_ASSERT(active_config != NULL);
    PT_ASSERT(active_config->backend != NULL);

    return active_config->backend->get_usable_height(window);
}

int pt_get_usable_xoffset(PtWindow *window) {
    PT_ASSERT(active_config != NULL);
    PT_ASSERT(active_config->backend != NULL);

    return active_config->backend->get_usable_xoffset(window);
}

int pt_get_usable_yoffset(PtWindow *window) {
    PT_ASSERT(active_config != NULL);
    PT_ASSERT(active_config->backend != NULL);

    return active_config->backend->get_usable_yoffset(window);
}

void pt_enable_throttle(PtWindow *window, int fps) {
    PT_ASSERT(window != NULL);
    PT_ASSERT(fps > 0);

    window->throttle_enabled = PT_TRUE;
    window->target_fps = fps;
    window->frame_duration = 1.0 / fps;
    window->last_frame_time = pt_get_time();
}

void pt_disable_throttle(PtWindow *window) {
    PT_ASSERT(window != NULL);

    window->throttle_enabled = PT_FALSE;
}

void pt_sleep(double seconds) {
    if (seconds <= 0.0) return;

    #ifdef _WIN32
        if (!high_precision_timer_init) {
            timeBeginPeriod(1);
            high_precision_timer_init = PT_TRUE;
        }

        double sleep_threshold = 0.002;

        if (seconds > sleep_threshold) {
            double sleep_time = seconds - sleep_threshold;
            Sleep((DWORD)(sleep_time * 1000.0));

            double start_time = pt_get_time();
            while ((pt_get_time() - start_time) < sleep_threshold) {
                if ((pt_get_time() - start_time) < sleep_threshold * 0.5) {
                    Sleep(0);
                }
            }
        } else {
            double start_time = pt_get_time();
            while ((pt_get_time() - start_time) < seconds) {
            }
        }
    #else
        struct timespec ts;
        ts.tv_sec = (time_t)seconds;
        ts.tv_nsec = (long)((seconds - ts.tv_sec) * 1000000000.0);
        nanosleep(&ts, NULL);
    #endif
}

double pt_get_time() {
    #ifdef _WIN32
        static LARGE_INTEGER frequency;
        static PT_BOOL frequency_initialized = PT_FALSE;
        LARGE_INTEGER counter;

        if (!frequency_initialized) {
            QueryPerformanceFrequency(&frequency);
            frequency_initialized = PT_TRUE;
        }

        QueryPerformanceCounter(&counter);
        return (double)counter.QuadPart / (double)frequency.QuadPart;
    #else
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
    #endif
}

void pt_shutdown() {
    PT_ASSERT(active_config != NULL);
    PT_ASSERT(active_config->backend != NULL);

    active_config->backend->shutdown(active_config->backend);

    #ifdef _WIN32
        if (high_precision_timer_init) {
            timeEndPeriod(1);
            high_precision_timer_init = PT_FALSE;
        }
    #endif

    active_config = NULL;
}

void* pt_get_window_handle(PtWindow *window) {
    PT_ASSERT(active_config != NULL);
    PT_ASSERT(active_config->backend != NULL);
    PT_ASSERT(window != NULL);

    return active_config->backend->get_handle(window);
}

void pt_set_window_title(PtWindow *window, const char *title) {
    PT_ASSERT(active_config != NULL);
    PT_ASSERT(active_config->backend != NULL);
    PT_ASSERT(window != NULL);
    PT_ASSERT(title != NULL);

    if (active_config->backend->set_window_title) {
        active_config->backend->set_window_title(window, title);
    }
}

void pt_set_window_size(PtWindow *window, int width, int height) {
    PT_ASSERT(active_config != NULL);
    PT_ASSERT(active_config->backend != NULL);
    PT_ASSERT(window != NULL);
    PT_ASSERT(width > 0);
    PT_ASSERT(height > 0);

    if (active_config->backend->set_window_size) {
        active_config->backend->set_window_size(window, width, height);
    }
}

void pt_set_video_mode(PtWindow *window, PtVideoMode mode) {
    PT_ASSERT(active_config != NULL);
    PT_ASSERT(active_config->backend != NULL);
    PT_ASSERT(window != NULL);

    if (active_config->backend->set_video_mode) {
        active_config->backend->set_video_mode(window, mode);
    }
}

void pt_show_window(PtWindow *window) {
    PT_ASSERT(active_config != NULL);
    PT_ASSERT(active_config->backend != NULL);
    PT_ASSERT(window != NULL);

    if (active_config->backend->show_window) {
        active_config->backend->show_window(window);
    }
}

void pt_hide_window(PtWindow *window) {
    PT_ASSERT(active_config != NULL);
    PT_ASSERT(active_config->backend != NULL);
    PT_ASSERT(window != NULL);

    if (active_config->backend->hide_window) {
        active_config->backend->hide_window(window);
    }
}

void pt_minimize_window(PtWindow *window) {
    PT_ASSERT(active_config != NULL);
    PT_ASSERT(active_config->backend != NULL);
    PT_ASSERT(window != NULL);

    if (active_config->backend->minimize_window) {
        active_config->backend->minimize_window(window);
    }
}

void pt_maximize_window(PtWindow *window) {
    PT_ASSERT(active_config != NULL);
    PT_ASSERT(active_config->backend != NULL);
    PT_ASSERT(window != NULL);

    if (active_config->backend->maximize_window) {
        active_config->backend->maximize_window(window);
    }
}

void pt_restore_window(PtWindow *window) {
    PT_ASSERT(active_config != NULL);
    PT_ASSERT(active_config->backend != NULL);
    PT_ASSERT(window != NULL);

    if (active_config->backend->restore_window) {
        active_config->backend->restore_window(window);
    }
}

void pt_focus_window(PtWindow *window) {
    PT_ASSERT(active_config != NULL);
    PT_ASSERT(active_config->backend != NULL);
    PT_ASSERT(window != NULL);

    if (active_config->backend->focus_window) {
        active_config->backend->focus_window(window);
    }
}

PT_BOOL pt_is_window_maximized(PtWindow *window) {
    PT_ASSERT(active_config != NULL);
    PT_ASSERT(active_config->backend != NULL);
    PT_ASSERT(window != NULL);

    if (active_config->backend->is_window_maximized) {
        return active_config->backend->is_window_maximized(window);
    }
    return PT_FALSE;
}

PT_BOOL pt_is_window_minimized(PtWindow *window) {
    PT_ASSERT(active_config != NULL);
    PT_ASSERT(active_config->backend != NULL);
    PT_ASSERT(window != NULL);

    if (active_config->backend->is_window_minimized) {
        return active_config->backend->is_window_minimized(window);
    }
    return PT_FALSE;
}

PT_BOOL pt_is_window_focused(PtWindow *window) {
    PT_ASSERT(active_config != NULL);
    PT_ASSERT(active_config->backend != NULL);
    PT_ASSERT(window != NULL);

    if (active_config->backend->is_window_focused) {
        return active_config->backend->is_window_focused(window);
    }
    return PT_FALSE;
}

PT_BOOL pt_is_window_visible(PtWindow *window) {
    PT_ASSERT(active_config != NULL);
    PT_ASSERT(active_config->backend != NULL);
    PT_ASSERT(window != NULL);

    if (active_config->backend->is_window_visible) {
        return active_config->backend->is_window_visible(window);
    }
    return PT_FALSE;
}
