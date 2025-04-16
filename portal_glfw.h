#ifndef PORTAL_GLFW_H
#define PORTAL_GLFW_H

#include "portal.h"

#ifdef __cplusplus
extern "C"
{
#endif

// creation / destruction
PtBackend *pt_glfw_create();
PT_BOOL pt_glfw_init(PtBackend *backend, PtConfig *config);
void pt_glfw_shutdown(PtBackend *backend);

#ifdef __cplusplus
}
#endif

#endif //PORTAL_GLFW_H
