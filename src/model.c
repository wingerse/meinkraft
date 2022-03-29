#include "model.h"
#include "containers/list.h"
#include "containers/gl_list.h"
#include "block.h"
#include "util.h"
#include "../obj/res/steve.png.h"
#include <math.h>
//#include "dir.h"

#define MODEL_ATLAS_SIDE 64
#define MODEL_ATLAS_SIDE_BITS 6
#define MODEL_ATLAS_PIXEL (1.0/MODEL_ATLAS_SIDE)

typedef enum body_part {
    HEAD,
    BODY,
    RIGHT_ARM,
    LEFT_ARM,
    RIGHT_LEG,
    LEFT_LEG,

    BODY_PARTS_COUNT,
} body_part;

// all coordinates are in pixels in these 2 structs
typedef struct sub_tex {
    int off_s, off_t;
    int w, h;
} sub_tex;

typedef struct body_part_info {
    // translation
    int tx, ty, tz;
    // how much to scale compared to PIX_PER_M
    int sx, sy, sz;
    sub_tex face_sub_texs[DIRS_COUNT];
} body_part_info;

// information for how to modify block_face_vertices from block.c to form body parts.
static const body_part_info body_part_infos[BODY_PARTS_COUNT] = {
    [HEAD] = {
        -8, 0,-8,
         8, 8, 8,
        {
            [DIR_NORTH] = { 8, 48, 8, 8},
            [DIR_SOUTH] = {24, 48, 8, 8},
            [DIR_EAST]  = { 0, 48, 8, 8},
            [DIR_WEST]  = {16, 48, 8, 8},
            [DIR_UP]    = { 8, 56, 8, 8},
            [DIR_DOWN]  = {16, 56, 8, 8},
        }
    },
    [BODY] = {
        -8,  0,-8,
         8, 12, 4,
        {
            [DIR_NORTH] = {20, 32, 8, 12},
            [DIR_SOUTH] = {32, 32, 8, 12},
            [DIR_EAST]  = {16, 32, 4, 12},
            [DIR_WEST]  = {28, 32, 4, 12},
            [DIR_UP]    = {20, 44, 8,  4},
            [DIR_DOWN]  = {28, 44, 8,  4},
        }
    },
    [RIGHT_ARM] = {
         0,-16,-8,
         4, 12, 4,
        {
            [DIR_NORTH] = {44, 32, 4, 12},
            [DIR_SOUTH] = {52, 32, 4, 12},
            [DIR_EAST]  = {48, 32, 4, 12},
            [DIR_WEST]  = {40, 32, 4, 12},
            [DIR_UP]    = {44, 44, 4,  4},
            [DIR_DOWN]  = {48, 44, 4,  4},
        }
    },
    [LEFT_ARM] = {
        -16,-16,-8,
         4, 12, 4,
        {
            [DIR_NORTH] = {36,  0, 4, 12},
            [DIR_SOUTH] = {44,  0, 4, 12},
            [DIR_EAST]  = {32,  0, 4, 12},
            [DIR_WEST]  = {40,  0, 4, 12},
            [DIR_UP]    = {36, 12, 4,  4},
            [DIR_DOWN]  = {40, 12, 4,  4},
        }
    },
    [RIGHT_LEG] = {
        -8,-16,-8,
         4, 12, 4,
        {
            [DIR_NORTH] = { 4, 32, 4, 12},
            [DIR_SOUTH] = {12, 32, 4, 12},
            [DIR_EAST]  = { 0, 32, 4, 12},
            [DIR_WEST]  = { 8, 32, 4, 12},
            [DIR_UP]    = { 4, 44, 4,  4},
            [DIR_DOWN]  = { 8, 44, 4,  4},
        }
    },
    [LEFT_LEG] = {
        -8,-16,-8,
         4, 12, 4,
        {
            [DIR_NORTH] = {20,  0, 4, 12},
            [DIR_SOUTH] = {28,  0, 4, 12},
            [DIR_EAST]  = {16,  0, 4, 12},
            [DIR_WEST]  = {24,  0, 4, 12},
            [DIR_UP]    = {20, 12, 4,  4},
            [DIR_DOWN]  = {24, 12, 4,  4},
        }
    },
};

static shader_block_vertex vertex_list[BODY_PARTS_COUNT * BLOCK_VERTICES_COUNT];
static GLuint index_list[BODY_PARTS_COUNT * BLOCK_INDICES_COUNT];

static void setup_vertices(void) 
{
    size_t vertex_list_index = 0;
    size_t index_list_index = 0;

    for (body_part part = 0; part < BODY_PARTS_COUNT; part++) {
        for (dir face = 0; face < DIRS_COUNT; face++) {
            for (int i = 0; i < BLOCK_FACE_VERTICES_COUNT; i++) {
                shader_block_vertex *v = &vertex_list[vertex_list_index++];
                *v = block_face_vertices[face][i];
                body_part_info info = body_part_infos[part];

                v->pos_x += pix_to_m(info.tx);
                v->pos_y += pix_to_m(info.ty);
                v->pos_z += pix_to_m(info.tz);

                v->pos_x *= (float)info.sx / PIX_PER_M;
                v->pos_y *= (float)info.sy / PIX_PER_M;
                v->pos_z *= (float)info.sz / PIX_PER_M;

                sub_tex face_sub_tex = info.face_sub_texs[face];
                v->uv_s = (face_sub_tex.off_s + v->uv_s * info.face_sub_texs[face].w) * MODEL_ATLAS_PIXEL; 
                v->uv_t = (face_sub_tex.off_t + v->uv_t * info.face_sub_texs[face].h) * MODEL_ATLAS_PIXEL; 
            }
            GLuint indices[] = {block_face_indices(part * DIRS_COUNT + face)};
            for (GLuint i = 0; i < BLOCK_FACE_INDICES_COUNT; i++) {
                index_list[index_list_index++] = indices[i];
            }
        }
    }

    glBufferData(GL_ARRAY_BUFFER, vertex_list_index * sizeof(shader_block_vertex), vertex_list, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_list_index * sizeof(GLuint), index_list, GL_STATIC_DRAW);
}

void model_init(model *m)
{
    glGenVertexArrays(1, &m->vao);
    glBindVertexArray(m->vao);
    glGenBuffers(1, &m->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m->vbo);
    glGenBuffers(1, &m->ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m->ebo);
    shader_block_set_up_attributes();

    setup_vertices();
    
    m->atlas = create_texture(res_steve_png, ARRAY_SIZE(res_steve_png), GL_NEAREST_MIPMAP_LINEAR, NULL);
}

static void render_body_part(
    body_part part,
    const mat4 *first_rotation, const mat4 *first_translation,
    const mat4 *second_rotation, const mat4 *second_translation,
    const mat4 *scale_matrix, const mat4 *final_translation, 
    const camera *camera, shader_block *shader)
{
    mat4 model_matrix;
    mat4 mvp_matrix;
    mat4_mul(&model_matrix, first_translation, first_rotation);
    mat4_mul(&model_matrix, second_rotation, &model_matrix);
    mat4_mul(&model_matrix, second_translation, &model_matrix);
    mat4_mul(&model_matrix, scale_matrix, &model_matrix);
    mat4_mul(&model_matrix, final_translation, &model_matrix);
    mat4_mul(&mvp_matrix, &camera->vp_matrix, &model_matrix);
    glUniformMatrix4fv(shader->mvp_matrix_location, 1, GL_FALSE, (float *)mvp_matrix.arr);
    glDrawElements(GL_TRIANGLES, BLOCK_INDICES_COUNT, GL_UNSIGNED_INT, 
                   (void *)(part*BLOCK_INDICES_COUNT * sizeof(GLuint)));
}

void model_render(const model *m, const vec3 *pos, const camera *camera, shader_block *shader, double t)
{
    glBindVertexArray(m->vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m->atlas);
    glDisable(GL_CULL_FACE);

    mat4 scale_matrix;
    mat4 final_translation;

    float scale = 1.8f/2.0f;
    mat4_init_scale(&scale_matrix, scale, scale, scale);
    mat4_init_translation(&final_translation, pos->x, pos->y, pos->z);

    mat4 first_rotation;
    mat4 first_translation;
    mat4 second_rotation;
    mat4 second_translation;

    first_rotation = MAT4_IDENTITY;
    first_translation = MAT4_IDENTITY;
    second_rotation = MAT4_IDENTITY;
    mat4_init_translation(&second_translation, 0, pix_to_m(24), 0);
    render_body_part(HEAD, &first_rotation, &first_translation, 
                   &second_rotation, &second_translation, &scale_matrix, &final_translation, camera, shader);

    first_rotation = MAT4_IDENTITY;
    first_translation = MAT4_IDENTITY;
    second_rotation = MAT4_IDENTITY;
    mat4_init_translation(&second_translation, 0, pix_to_m(12), 0);
    render_body_part(BODY, &first_rotation, &first_translation, 
                   &second_rotation, &second_translation, &scale_matrix, &final_translation, camera, shader);

    double x_angle = (-cos(t * 2) + 1.0)/2;
    x_angle *= rad_from_deg(5);

    first_rotation = MAT4_IDENTITY;
    mat4_init_translation(&first_translation, 0, pix_to_m(2), 0);
    mat4_init_rotate_z(&second_rotation, x_angle);
    mat4_init_translation(&second_translation, pix_to_m(4), pix_to_m(24 - 2), 0);
    render_body_part(RIGHT_ARM, &first_rotation, &first_translation, 
                   &second_rotation, &second_translation, &scale_matrix, &final_translation, camera, shader);

    first_rotation = MAT4_IDENTITY;
    mat4_init_translation(&first_translation, 0, pix_to_m(2), 0);
    mat4_init_rotate_z(&second_rotation, -x_angle);
    mat4_init_translation(&second_translation, pix_to_m(-4), pix_to_m(24 - 2), 0);
    render_body_part(LEFT_ARM, &first_rotation, &first_translation, 
                   &second_rotation, &second_translation, &scale_matrix, &final_translation, camera, shader);

    first_rotation = MAT4_IDENTITY;
    first_translation = MAT4_IDENTITY;
    second_rotation = MAT4_IDENTITY;
    mat4_init_translation(&second_translation, pix_to_m(2), pix_to_m(12), 0);
    render_body_part(RIGHT_LEG, &first_rotation, &first_translation, 
                   &second_rotation, &second_translation, &scale_matrix, &final_translation, camera, shader);

    first_rotation = MAT4_IDENTITY;
    first_translation = MAT4_IDENTITY;
    second_rotation = MAT4_IDENTITY;
    mat4_init_translation(&second_translation, pix_to_m(-2), pix_to_m(12), 0);
    render_body_part(LEFT_LEG, &first_rotation, &first_translation, 
                   &second_rotation, &second_translation, &scale_matrix, &final_translation, camera, shader);
}

void model_destroy(model *m) 
{
    glDeleteVertexArrays(1, &m->vao);
    glDeleteBuffers(1, &m->vbo);
    glDeleteBuffers(1, &m->ebo);
}