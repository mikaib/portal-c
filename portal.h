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
    PT_BACKEND_GLFW = 1,
    PT_BACKEND_ANDROID = 2,
} PtBackendType;

typedef enum {
    PT_BACKEND_KIND_DESKTOP = 1,
    PT_BACKEND_KIND_MOBILE = 2,
} PtBackendKind;

typedef enum {
    PT_CAPABILITY_CREATE_WINDOW = 1 << 0,
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

    // window
    PtWindow *(*create_window)(const char *title, int width, int height);
    void (*destroy_window)(PtWindow *window);
    void (*poll_events)(PtWindow *window);
    void (*swap_buffers)(PtWindow *window);
    int (*get_window_width)(PtWindow *window);
    int (*get_window_height)(PtWindow *window);
    PT_BOOL (*should_window_close)(PtWindow *window);

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
PtWindow* pt_create_window(const char *title, int width, int height);
void pt_destroy_window(PtWindow *window);
void pt_poll_events(PtWindow *window);
void pt_swap_buffers(PtWindow *window);
int pt_get_window_width(PtWindow *window);
int pt_get_window_height(PtWindow *window);
PT_BOOL pt_should_window_close(PtWindow *window);

// events
int pt_get_input_event_count(PtWindow *window);
void pt_push_input_event(PtWindow *window, PtInputEventData event);
PtInputEventData pt_pull_input_event(PtWindow *window);
PtInputEventData pt_create_input_event_data();

// context
PT_BOOL pt_use_gl_context(PtWindow *window);

#ifdef __cplusplus
}
#endif

#endif //PORTAL_H
