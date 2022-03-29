/*
 * World uses a right handed coordinate system.
 * North is -z.
 */
#pragma once

#include "chunk.h"
#include "glad.h"
#include "block.h"
#include "containers/hmap.h"
#include "cgmath.h"
#include "pos.h"
#include "camera.h"
#include "shaders/shader_block.h"

#define VIEW_DISTANCE   16
#define CHUNKS_PER_SIDE ((VIEW_DISTANCE-1)*2 + 1)

HMAP_DECLARE(cpos, chunk)

typedef struct world {
    GLuint          block_atlas_texture;
    hmap_cpos_chunk chunks;
} world;

void       world_init(world *w);
void       world_generate(world *w);
block_type world_get_block(const world *w, bpos pos);
void       world_set_block(world *w, bpos pos, block_type b);
void       world_render(const world *w, const camera *camera, shader_block *shader);
ubpos      world_ray_cast(const world *w, const camera *camera, uint8_t max_distance, block_type *block);