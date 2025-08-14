#ifndef PORTAL_H
#define PORTAL_H

#ifdef __cplusplus
extern "C"
{
#endif

#define PT_VERSION_MAJOR 0
#define PT_VERSION_MINOR 1
#define PT_VERSION_PATCH 0

#define PT_MALLOC(size) malloc(size)
#define PT_ALLOC_MULTIPLE(obj, count) (obj*)malloc(sizeof(obj) * count)
#define PT_ASSERT(cond) if(!(cond)) { printf("Assertion failed: %s, file: %s, line: %d\n", #cond, __FILE__, __LINE__); exit(1); }
#define PT_ASSERT_WARN(cond, message) if(!(cond)) { printf("Warning: %s, file: %s, line: %d\n", message, __FILE__, __LINE__); }
#define PT_MEMSET(ptr, value, size) memset(ptr, value, size)

#define PT_ALLOC(obj) PT_ALLOC_MULTIPLE(obj, 1)
#define PT_FREE(obj) free(obj)

#define PT_TABLE_SIZE(arr) (int)(sizeof(arr) / sizeof((arr)[0]))
#define PT_BOOL int
#define PT_TRUE 1
#define PT_FALSE 0

#define PT_MAX_EVENT_COUNT 256

typedef enum {
    PT_BACKEND_NOOP = 1,
    PT_BACKEND_GLFW = 2,
    PT_BACKEND_ANDROID = 3,
} PtBackendType;

typedef enum {
    PT_BACKEND_KIND_DESKTOP = 1,
    PT_BACKEND_KIND_MOBILE = 2,
    PT_BACKEND_KIND_HEADLESS = 3
} PtBackendKind;

typedef enum {
    PT_CAPABILITY_CREATE_WINDOW = 1 << 0,
    PT_CAPABILITY_WINDOW_SIZE = 1 << 1,
    PT_CAPABILITY_WINDOW_POSITION = 1 << 2,
    PT_CAPABILITY_WINDOW_VIDEO_MODE = 1 << 3
} PtCapability;

typedef enum {
    // None
    PT_INPUT_EVENT_NONE = 0,

    // Keyboard (Does work on mobile but requires explicitly requesting a keyboard)
    PT_INPUT_EVENT_KEYUP = 1,         // { key: PtKey, modifiers: PtModifier }
    PT_INPUT_EVENT_KEYDOWN = 2,       // { key: PtKey, modifiers: PtModifier }
    PT_INPUT_EVENT_KEYPRESS = 3,      // { key: PtKey, modifiers: PtModifier }
    PT_INPUT_EVENT_TEXT = 4,          // { text: char* }

    // Mouse
    PT_INPUT_EVENT_MOUSEUP = 100,       // { button: PtMouseButton, modifiers: PtModifier }
    PT_INPUT_EVENT_MOUSEDOWN = 101,     // { button: PtMouseButton, modifiers: PtModifier }
    PT_INPUT_EVENT_MOUSEMOVE = 102,     // { x: int, y: int, dx: int, dy: int }
    PT_INPUT_EVENT_MOUSEWHEEL = 103,    // { x: int, y: int, dx: int, dy: int }

    // Touch
    PT_INPUT_EVENT_TOUCHUP = 200,       // { finger: int, x: int, y: int }
    PT_INPUT_EVENT_TOUCHDOWN = 201,     // { finger: int, x: int, y: int }
    PT_INPUT_EVENT_TOUCHMOVE = 202,     // { finger: int, x: int, y: int }
} PtInputEventType;

typedef enum {
    PT_FLAG_NONE = 0,
    PT_FLAG_VSYNC = 1 << 0,
    PT_FLAG_RESIZABLE = 1 << 1,
    PT_FLAG_MAXIMIZED = 1 << 2,
    PT_FLAG_MINIMIZED = 1 << 3,
    PT_FLAG_HIDDEN = 1 << 4,
    PT_FLAG_ALWAYS_ON_TOP = 1 << 5,
    PT_FLAG_NO_TITLEBAR = 1 << 6,
    PT_FLAG_TRANSPARENT = 1 << 7,
    PT_FLAG_FULLSCREEN = 1 << 8,
    PT_FLAG_BORDERLESS = 1 << 9,
    PT_FLAG_CENTERED = 1 << 10,
    PT_FLAG_NO_FOCUS = 1 << 11,
} PtWindowFlags;

typedef enum {
    PT_VIDEO_MODE_FULLSCREEN = 0,
    PT_VIDEO_MODE_WINDOWED = 1,
    PT_VIDEO_MODE_BORDERLESS = 2,
    PT_VIDEO_MODE_MAXIMIZED = 3,
} PtVideoMode;

typedef struct PtConfig PtConfig;
typedef struct PtBackend PtBackend;
typedef struct PtWindow PtWindow;
typedef struct PtInputEventKeyData PtInputEventKeyData;
typedef struct PtInputEventMouseData PtInputEventMouseData;
typedef struct PtInputEventTouchData PtInputEventTouchData;
typedef struct PtInputEventTextData PtInputEventTextData;
typedef struct PtInputEventData PtInputEventData;

typedef struct PtConfig {
    PtBackend *backend;
} PtConfig;

typedef struct PtWindow {
    void *handle;
    PT_BOOL throttle_enabled;
    int target_fps;
    double last_frame_time;
    double frame_duration;
} PtWindow;

typedef struct PtInputEventKeyData {
    int key;
    int modifiers;
} PtInputEventKeyData;

typedef struct PtInputEventMouseData {
    int button;
    int modifiers;
    int x;
    int y;
    int dx;
    int dy;
} PtInputEventMouseData;

typedef struct PtInputEventTouchData {
    int finger;
    int x;
    int y;
} PtInputEventTouchData;

typedef struct PtInputEventTextData {
    unsigned int codepoint;
} PtInputEventTextData;

typedef struct PtInputEventData {
    PtInputEventType type;
    PtInputEventKeyData key;
    PtInputEventMouseData mouse;
    PtInputEventTouchData touch;
    PtInputEventTextData text;
    double timestamp;
} PtInputEventData;

typedef struct PtBackend {
    PtBackendType type;
    PtBackendKind kind;
    PtCapability capabilities;
    PtInputEventData input_events[PT_MAX_EVENT_COUNT];
    int input_event_count;

    // core
    PT_BOOL (*init)(PtBackend *backend, PtConfig *config);
    void (*shutdown)(PtBackend *backend);
    void* (*get_handle)(PtWindow *window);

    // window
    PtWindow *(*create_window)(const char *title, int width, int height, PtWindowFlags flags);
    void (*destroy_window)(PtWindow *window);
    void (*poll_events)(PtWindow *window);
    void (*swap_buffers)(PtWindow *window);
    void (*set_window_title)(PtWindow *window, const char *title);
    void (*set_window_size)(PtWindow *window, int width, int height);
    void (*set_video_mode)(PtWindow *window, PtVideoMode mode);
    void (*show_window)(PtWindow *window);
    void (*hide_window)(PtWindow *window);
    void (*minimize_window)(PtWindow *window);
    void (*maximize_window)(PtWindow *window);
    void (*restore_window)(PtWindow *window);
    void (*focus_window)(PtWindow *window);
    int (*get_window_width)(PtWindow *window);
    int (*get_window_height)(PtWindow *window);
    int (*get_framebuffer_width)(PtWindow *window);
    int (*get_framebuffer_height)(PtWindow *window);
    int (*get_usable_width)(PtWindow *window);
    int (*get_usable_height)(PtWindow *window);
    int (*get_usable_xoffset)(PtWindow *window);
    int (*get_usable_yoffset)(PtWindow *window);
    PT_BOOL (*should_window_close)(PtWindow *window);
    PT_BOOL (*is_window_maximized)(PtWindow *window);
    PT_BOOL (*is_window_minimized)(PtWindow *window);
    PT_BOOL (*is_window_focused)(PtWindow *window);
    PT_BOOL (*is_window_visible)(PtWindow *window);

    // lifecycle
    void (*activate)(PtWindow *window);
    void (*deactivate)(PtWindow *window);
    PT_BOOL (*should_be_active)(PtWindow *window);

    // context
    PT_BOOL (*use_gl_context)(PtWindow *window);
} PtBackend;

// Global
PT_BOOL pt_init(PtConfig *config);
void pt_shutdown();

// Config
PtConfig *pt_create_config();
void pt_destroy_config(PtConfig *config);

// Backend
PtBackend *pt_create_backend(PtBackendType type);
void pt_destroy_backend(PtBackend *backend);
PtBackendType pt_get_optimal_backend_type();

// Window
PtWindow* pt_create_window(const char *title, int width, int height, PtWindowFlags flags);
void pt_destroy_window(PtWindow *window);
void pt_poll_events(PtWindow *window);
void pt_swap_buffers(PtWindow *window);
void* pt_get_window_handle(PtWindow *window); // os-handle
void pt_set_window_title(PtWindow *window, const char *title);
void pt_set_window_size(PtWindow *window, int width, int height);
void pt_set_video_mode(PtWindow *window, PtVideoMode mode);
int pt_get_window_width(PtWindow *window);
int pt_get_window_height(PtWindow *window);
int pt_get_framebuffer_width(PtWindow *window);
int pt_get_framebuffer_height(PtWindow *window);
int pt_get_usable_width(PtWindow *window);
int pt_get_usable_height(PtWindow *window);
int pt_get_usable_xoffset(PtWindow *window);
int pt_get_usable_yoffset(PtWindow *window);
PT_BOOL pt_should_window_close(PtWindow *window);

// events
int pt_get_input_event_count(PtWindow *window);
void pt_push_input_event(PtWindow *window, PtInputEventData event);
PtInputEventData pt_pull_input_event(PtWindow *window);
PtInputEventData pt_create_input_event_data();

// context
PT_BOOL pt_use_gl_context(PtWindow *window);

// throttling
void pt_enable_throttle(PtWindow *window, int fps);
void pt_disable_throttle(PtWindow *window);
void pt_sleep(double seconds);
double pt_get_time();

// Window state management
void pt_show_window(PtWindow *window);
void pt_hide_window(PtWindow *window);
void pt_minimize_window(PtWindow *window);
void pt_maximize_window(PtWindow *window);
void pt_restore_window(PtWindow *window);
void pt_focus_window(PtWindow *window);

// Window state queries
PT_BOOL pt_is_window_maximized(PtWindow *window);
PT_BOOL pt_is_window_minimized(PtWindow *window);
PT_BOOL pt_is_window_focused(PtWindow *window);
PT_BOOL pt_is_window_visible(PtWindow *window);

#ifdef __cplusplus
}
#endif

#endif //PORTAL_H
