#include "portal.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "portal_glfw.h"

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
    return PT_BACKEND_GLFW;
}

void pt_destroy_backend(PtBackend *backend) {
    PT_ASSERT(backend != NULL);
    PT_FREE(backend);
}

PtBackend *pt_create_backend(PtBackendType type) {
    if (type == PT_BACKEND_GLFW) {
        return pt_glfw_create();
    }

    return NULL;
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

void pt_shutdown() {
    PT_ASSERT(active_config != NULL);
    PT_ASSERT(active_config->backend != NULL);

    active_config->backend->shutdown(active_config->backend);
    active_config = NULL;
}