#pragma once

#include "../glad.h"
#include <stddef.h>

typedef struct shader_selector_vertex {
    float pos_x;
    float pos_y;
    float pos_z;
} shader_selector_vertex;

typedef struct shader_selector {
    GLint program;
    GLint mvp_matrix_location;
} shader_selector;

void shader_selector_init(shader_selector *s);
void shader_selector_use(shader_selector *s);
void shader_selector_set_up_attributes(void);