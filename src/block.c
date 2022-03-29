#include "block.h"
#include "util.h"

bool block_is_opaque(block_type b)
{
    switch (b) {
    case BLOCK_AIR: return false;
    default: return true;
    }
}

/* 
 * A block is by definition 1 unit of measure (call it meter) long with lower coords (0, 0, 0) upper coords (1, 1, 1).
 * Triangles for each face goes anti clockwise starting from bottom left; this is defined in 
 * block_face_indices macro
 * All coordinates (including uv) are in local space.
 */
const shader_block_vertex block_face_vertices[DIRS_COUNT][BLOCK_FACE_VERTICES_COUNT] = {
    [DIR_NORTH] = {
        {1, 0, 0, 0, 0, 0, 0,-1, 0.8f},
        {0, 0, 0, 1, 0, 0, 0,-1, 0.8f},
        {0, 1, 0, 1, 1, 0, 0,-1, 0.8f},
        {1, 1, 0, 0, 1, 0, 0,-1, 0.8f},
    },
    [DIR_SOUTH] = {
        {0, 0, 1, 0, 0, 0, 0, 1, 0.8f},
        {1, 0, 1, 1, 0, 0, 0, 1, 0.8f},
        {1, 1, 1, 1, 1, 0, 0, 1, 0.8f},
        {0, 1, 1, 0, 1, 0, 0, 1, 0.8f},
    },
    [DIR_EAST] = { 
        {1, 0, 1, 0, 0,-1, 0, 0, 0.6f},
        {1, 0, 0, 1, 0,-1, 0, 0, 0.6f},
        {1, 1, 0, 1, 1,-1, 0, 0, 0.6f},
        {1, 1, 1, 0, 1,-1, 0, 0, 0.6f},
    },
    [DIR_WEST] = {
        {0, 0, 0, 0, 0,-1, 0, 0, 0.6f},
        {0, 0, 1, 1, 0,-1, 0, 0, 0.6f},
        {0, 1, 1, 1, 1,-1, 0, 0, 0.6f},
        {0, 1, 0, 0, 1,-1, 0, 0, 0.6f},
    },
    [DIR_UP] = {
        {0, 1, 1, 0, 0, 0, 1, 0, 1.0f},
        {1, 1, 1, 1, 0, 0, 1, 0, 1.0f},
        {1, 1, 0, 1, 1, 0, 1, 0, 1.0f},
        {0, 1, 0, 0, 1, 0, 1, 0, 1.0f},
    },
    [DIR_DOWN] = {
        {1, 0, 1, 0, 0, 0,-1, 0, 0.5f},
        {0, 0, 1, 1, 0, 0,-1, 0, 0.5f},
        {0, 0, 0, 1, 1, 0,-1, 0, 0.5f},
        {1, 0, 0, 0, 1, 0,-1, 0, 0.5f},
    },
};

const block_atlas_index block_atlas_indices[BLOCKS_COUNT][DIRS_COUNT] = {
    [BLOCK_GRASS] = {
        [DIR_NORTH] = {1, 0},
        [DIR_SOUTH] = {1, 0},
        [DIR_EAST]  = {1, 0},
        [DIR_WEST]  = {1, 0},
        [DIR_UP]    = {0, 0},
        [DIR_DOWN]  = {2, 0},
    },
    [BLOCK_COBBLESTONE] = {
        [DIR_NORTH] = {3, 0},
        [DIR_SOUTH] = {3, 0},
        [DIR_EAST]  = {3, 0},
        [DIR_WEST]  = {3, 0},
        [DIR_UP]    = {3, 0},
        [DIR_DOWN]  = {3, 0},
    },
    [BLOCK_DIRT] = {
        [DIR_NORTH] = {2, 0},
        [DIR_SOUTH] = {2, 0},
        [DIR_EAST]  = {2, 0},
        [DIR_WEST]  = {2, 0},
        [DIR_UP]    = {2, 0},
        [DIR_DOWN]  = {2, 0},
    },
};

// copied from block vertices, so make sure to expand a bit
static shader_selector_vertex block_selector_vertices[9] = {
    {1, 0, 0},
    {0, 0, 0},
    {0, 1, 0},
    {1, 1, 0},
    {0, 0, 1},
    {1, 0, 1},
    {1, 1, 1},
    {0, 1, 1},

    // cursor
    {0, 0, 0},
};

static const GLuint block_selector_indices[24] = {
    0, 1, 1, 2, 2, 3, 3, 0, 
    0, 5, 3, 6, 2, 7, 1, 4,
    4, 5, 5, 6, 6, 7, 7, 4
};

void selector_init(selector *s)
{
    glGenVertexArrays(1, &s->vao);
    glBindVertexArray(s->vao);
    glGenBuffers(1, &s->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, s->vbo);
    glGenBuffers(1, &s->ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s->ebo);
    shader_selector_set_up_attributes();

    // expand a bit
    const float expansion = 1.01f;
    for (size_t i = 0; i < 8; i++) {
        shader_selector_vertex *v = &block_selector_vertices[i];
        v->pos_x = (v->pos_x - 0.5f) * expansion + 0.5f;
        v->pos_y = (v->pos_y - 0.5f) * expansion + 0.5f;
        v->pos_z = (v->pos_z - 0.5f) * expansion + 0.5f;
    }

    glBufferData(GL_ARRAY_BUFFER, sizeof(block_selector_vertices), block_selector_vertices, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(block_selector_indices), block_selector_indices, GL_STATIC_DRAW);
}

void selector_render(const selector *s, const vec3 *pos, const camera *camera, shader_selector *shader)
{
    mat4 model_matrix;
    mat4_init_translation(&model_matrix, pos->x, pos->y, pos->z);
    mat4 mvp_matrix;
    mat4_mul(&mvp_matrix, &camera->vp_matrix, &model_matrix);
    glUniformMatrix4fv(shader->mvp_matrix_location, 1, GL_FALSE, (float *)mvp_matrix.arr);
    glBindVertexArray(s->vao);
    glPointSize(3);
    glDrawElements(GL_LINES, ARRAY_SIZE(block_selector_indices), GL_UNSIGNED_INT, 0);
}

void selector_render_cursor(const selector *s, shader_selector *shader)
{
    mat4 mvp_matrix = MAT4_IDENTITY;
    glUniformMatrix4fv(shader->mvp_matrix_location, 1, GL_FALSE, (float *)mvp_matrix.arr);
    glBindVertexArray(s->vao);
    glDrawArrays(GL_POINTS, 8, 1);
}