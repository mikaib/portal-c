#include "portal.c"

int main() {
    PtConfig *config = pt_create_config();
    PtBackend *backend = pt_create_backend(PT_BACKEND_GLFW);

    pt_init(config);
    pt_shutdown();

    pt_destroy_config(config);

    return 0;
}