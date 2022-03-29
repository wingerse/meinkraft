/*
 * Steve model.
 */
#pragma once

#include "glad.h"
#include <stdbool.h>
#include "cgmath.h"
#include "camera.h"
#include "shaders/shader_block.h"

typedef struct model {
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    GLuint atlas;
} model;

void model_init(model *m);
void model_render(const model *m, const vec3 *pos, const camera *camera, shader_block *shader, double t);
void model_destroy(model *m);
