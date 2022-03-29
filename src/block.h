#pragma once

#include "glad.h"
#include "cgmath.h"
#include <stdbool.h>
#include "shaders/shader_selector.h"
#include "shaders/shader_block.h"
#include "camera.h"
#include "pos.h"

#define PIX_PER_M 16
#define pix_to_m(a) (((float)a)/PIX_PER_M) 
#define m_to_pix(a) ((a)*PIX_PER_M) 
#define BLOCK_TEX_SIDE PIX_PER_M
#define BLOCK_ATLAS_WIDTH (BLOCK_TEX_SIDE * 4)
#define BLOCK_ATLAS_HEIGHT (BLOCK_TEX_SIDE)
#define BLOCK_ATLAS_PIXEL_S (1.0/BLOCK_ATLAS_WIDTH)
#define BLOCK_ATLAS_PIXEL_T (1.0/BLOCK_ATLAS_HEIGHT)
#define BLOCK_TEX_SIDE_S (BLOCK_TEX_SIDE * BLOCK_ATLAS_PIXEL_S)
#define BLOCK_TEX_SIDE_T (BLOCK_TEX_SIDE * BLOCK_ATLAS_PIXEL_T)

typedef enum block_type {
    BLOCK_AIR,
    BLOCK_GRASS,
    BLOCK_COBBLESTONE,
    BLOCK_DIRT,

    BLOCKS_COUNT,
} block_type;

bool block_is_opaque(block_type b);

#define BLOCK_FACE_VERTICES_COUNT 4
#define BLOCK_VERTICES_COUNT (DIRS_COUNT*BLOCK_FACE_VERTICES_COUNT)
extern const shader_block_vertex block_face_vertices[DIRS_COUNT][BLOCK_FACE_VERTICES_COUNT];

#define BLOCK_FACE_INDICES_COUNT 6
#define BLOCK_INDICES_COUNT (DIRS_COUNT*BLOCK_FACE_INDICES_COUNT)

/* 
 * defines the indices of vertices for 'face'th face in vbo
 * this macro goes inside an array as it's comma separated
 */
#define block_face_indices(face) \
(face)*BLOCK_FACE_VERTICES_COUNT+0,\
(face)*BLOCK_FACE_VERTICES_COUNT+1,\
(face)*BLOCK_FACE_VERTICES_COUNT+2,\
(face)*BLOCK_FACE_VERTICES_COUNT+2,\
(face)*BLOCK_FACE_VERTICES_COUNT+3,\
(face)*BLOCK_FACE_VERTICES_COUNT+0

typedef struct block_atlas_index {
    int s, t;
} block_atlas_index;

extern const block_atlas_index block_atlas_indices[BLOCKS_COUNT][DIRS_COUNT]; 

typedef struct selector {
    GLuint vao, vbo, ebo;
} selector;

void selector_init(selector *s);
void selector_render(const selector *s, const vec3 *pos, const camera *camera, shader_selector *shader);
void selector_render_cursor(const selector *s, shader_selector *shader);