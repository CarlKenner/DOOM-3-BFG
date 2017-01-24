#include "gl/scene/rift_initializer.h"
#include "gl/scene/gl_rift_hmd.h"

void initialize_oculus_rift() {
    sharedRiftHmd->init_tracking();
}

