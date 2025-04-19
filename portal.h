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

typedef enum {
    PT_BACKEND_GLFW,
    PT_BACKEND_ANDROID
} PtBackendType;

typedef enum {
    PT_CAPABILITY_CREATE_WINDOW = 1 << 0,
} PtCapability;

typedef struct PtConfig PtConfig;
typedef struct PtBackend PtBackend;
typedef struct PtWindow PtWindow;

typedef struct PtBackend {
    PtBackendType type;
    PtCapability capabilities;

    // core
    PT_BOOL (*init)(PtBackend *backend, PtConfig *config);
    void (*shutdown)(PtBackend *backend);

    // window
    PtWindow *(*create_window)(const char *title, int width, int height);
    void (*destroy_window)(PtWindow *window);
    void (*poll_events)(PtWindow *window);
    void (*swap_buffers)(PtWindow *window);
    PT_BOOL (*should_window_close)(PtWindow *window);

    // context
    PT_BOOL (*use_gl_context)(PtWindow *window);
} PtBackend;

typedef struct PtConfig {
    PtBackend *backend;
} PtConfig;

typedef struct PtWindow {
    void *handle;
} PtWindow;

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
PT_BOOL pt_should_window_close(PtWindow *window);

// context
PT_BOOL pt_use_gl_context(PtWindow *window);

#ifdef __cplusplus
}
#endif

#endif //PORTAL_H
