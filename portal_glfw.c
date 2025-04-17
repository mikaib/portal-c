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

    return backend;
}

PT_BOOL pt_glfw_init(PtBackend *backend, PtConfig *config) {
    PT_ASSERT(config != NULL);
    PT_ASSERT(backend != NULL);

    if (!glfwInit()) {
        return PT_FALSE;
    }

    config->backend = backend;
    return PT_TRUE;
}

void pt_glfw_shutdown(PtBackend *backend) {
    PT_ASSERT(backend != NULL);

    glfwTerminate();
}