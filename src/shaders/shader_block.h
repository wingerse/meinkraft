#pragma once

#include <stddef.h>
#include "../glad.h"

typedef struct shader_block_vertex {
    float pos_x, pos_y, pos_z;
    float uv_s, uv_t;
    float normal_x, normal_y, normal_z;
    float brightness;
} shader_block_vertex;

typedef struct shader_block {
    GLuint program;
    GLuint mvp_matrix_location;
} shader_block;

void shader_block_init(shader_block *s);
void shader_block_use(shader_block *s);
void shader_block_set_up_attributes(void);