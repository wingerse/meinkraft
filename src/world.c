#include "world.h"
#include <math.h>
#include "util.h"
#include <stdio.h>
#include <stddef.h>
#include "perlin/noise1234.h"
#include "stb_image.h"
#include "../obj/res/atlas.png.h"

HMAP_DEFINE(cpos, chunk, cpos_hash, cpos_eq)

void world_init(world *w)
{
    w->block_atlas_texture = create_texture(res_atlas_png, ARRAY_SIZE(res_atlas_png), GL_NEAREST_MIPMAP_LINEAR, &(int){4});
    hmap_cpos_chunk_init(&w->chunks, NULL, chunk_destroy);
    world_generate(w);
}

void world_generate(world *w)
{
    for (int x = 0; x < CHUNK_SIDE * CHUNKS_PER_SIDE; x++) {
        for (int z = 0; z < CHUNK_SIDE * CHUNKS_PER_SIDE; z++) {
            float p = (noise2(x * 0.01, z * 0.01) + 1) / 2;
            int h = p * 100;
            for (int y = 0; y < h; y++) {
                world_set_block(w, (bpos){x, y, z}, BLOCK_GRASS);
            }
        }
    }

    HMAP_ITER_BEGIN(&w->chunks, e)
        cpos cp = e->key;
        cpos offsets[4] = {
            cpos_offset(cp, DIR_NORTH),
            cpos_offset(cp, DIR_SOUTH),
            cpos_offset(cp, DIR_EAST), 
            cpos_offset(cp, DIR_WEST), 
        };
        chunk *dir_chunks[4] = {
            hmap_cpos_chunk_get(&w->chunks, &offsets[DIR_NORTH]),
            hmap_cpos_chunk_get(&w->chunks, &offsets[DIR_SOUTH]),
            hmap_cpos_chunk_get(&w->chunks, &offsets[DIR_EAST]),
            hmap_cpos_chunk_get(&w->chunks, &offsets[DIR_WEST]),
        };

        chunk_remesh(&e->value, (const chunk * (*)[4])&dir_chunks);
    HMAP_ITER_END
}

block_type world_get_block(const world *w, bpos pos)
{
    cpos ckpos = bpos_to_cpos(pos);
    cbpos ckbpos = bpos_to_cbpos(pos);
    chunk *c = hmap_cpos_chunk_get(&w->chunks, &ckpos);
    if (!c) {
        return BLOCK_AIR;
    }

    return chunk_get_block(c, ckbpos);
}

void world_set_block(world *w, bpos pos, block_type b)
{
    cpos ckpos = bpos_to_cpos(pos);
    cbpos ckbpos = bpos_to_cbpos(pos);
    chunk *c = hmap_cpos_chunk_get(&w->chunks, &ckpos);
    if (!c) {
        c = hmap_cpos_chunk_put(&w->chunks, &ckpos);
        chunk_init(c);
    }

    chunk_set_block(c, ckbpos, b);
}

#if 0
void world_setr_block(world *w, bpos pos, block_type b) 
{
    cpos cp = bpos_to_cpos(pos);
    cbpos cbp = bpos_to_cbpos(pos);
    chunk *c = hmap_cpos_chunk_get(&w->chunks, &cp);
    if (!c) {
        c = hmap_cpos_chunk_put(&w->chunks, &cp);
        chunk_init(c);
    }
    uint8_t affected = chunk_setr_block(c, cbp, b, );

    for (dir d = DIR_NORTH; dir <= DIR_WEST; dir++) {
        if ((affected & (1 << d)) != 0) {
            cpos cp2 = cpos_offset(cp, d);
            cpos offsets[4] = {
                cpos_offset(cp2, DIR_NORTH),
                cpos_offset(cp2, DIR_SOUTH),
                cpos_offset(cp2, DIR_EAST), 
                cpos_offset(cp2, DIR_WEST), 
            };
            chunk *dir_chunks[4] = {
                hmap_cpos_chunk_get(&w->chunks, &offsets[DIR_NORTH]),
                hmap_cpos_chunk_get(&w->chunks, &offsets[DIR_SOUTH]),
                hmap_cpos_chunk_get(&w->chunks, &offsets[DIR_EAST]),
                hmap_cpos_chunk_get(&w->chunks, &offsets[DIR_WEST]),
            };

            chunk_remesh_sec(&e->value, section_from_cbpos(cbp), (const chunk *(*)[4])&dir_chunks);
            chunk_remesh(&e->value, (const chunk * (*)[4])&dir_chunks);
        }
    }
}
#endif

void world_render(const world *w, const camera *camera, shader_block *shader)
{
    glEnable(GL_CULL_FACE);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, w->block_atlas_texture);
    HMAP_ITER_BEGIN(&w->chunks, e)
        chunk_render(&e->value, e->key, camera, shader);
    HMAP_ITER_END
}

ubpos world_ray_cast(const world *w, const camera *camera, uint8_t max_distance, block_type *block)
{
    // Algorithm: A Fast Voxel Traversal Algorithm for Ray Tracing
    // https://www.researchgate.net/publication/2611491_A_Fast_Voxel_Traversal_Algorithm_for_Ray_Tracing
    float   pos_x        = camera->pos.x;
    float   pos_y        = camera->pos.y;
    float   pos_z        = camera->pos.z;
    float   dir_x        = camera->dir.x;
    float   dir_y        = camera->dir.y;
    float   dir_z        = camera->dir.z;
    // ray equation is r = pos + t*dir
    int32_t x            = (int32_t)floorf(pos_x);
    int32_t y            = (int32_t)floorf(pos_y);
    int32_t z            = (int32_t)floorf(pos_z);
    int     step_x       = dir_x > 0 ? 1 : -1;
    int     step_y       = dir_y > 0 ? 1 : -1;
    int     step_z       = dir_z > 0 ? 1 : -1;

    block_type b = world_get_block(w, (bpos){x, y, z});
    if (block_is_opaque(b)) {
        *block = b;
        return (ubpos){x, y, z};
    }

    // If pos is an integer, floor or ceil wouldn't work, so we manually sub or add 1 to r respectively.
    // But for negative dirs, this r is now about before block (we are not looking at the block at pos anymore). 
    // So we need to go to that block as well
    #define set_r(r, step, pos, coord) \
    if (step > 0) {\
        r = ceilf(pos);\
        if (r == pos) r += 1;\
    }\
    else {\
        r = floorf(pos);\
        if (r == pos) {r -= 1; coord -= 1;}\
    }
    float   r_x;
    set_r(r_x, step_x, pos_x, x);
    float   r_y;
    set_r(r_y, step_y, pos_y, y);
    float   r_z;
    set_r(r_z, step_z, pos_z, z);
    #undef set_r

    #define dir_zero(dir) dir == 0.0f || dir == -0.0f
    // we know r, pos and dir for each axis, so we can find t for each axis
    // t = (r-pos)/dir
    float   t_boundary_x = dir_zero(dir_x) ? INFINITY : (r_x - pos_x) / dir_x;
    float   t_boundary_y = dir_zero(dir_y) ? INFINITY : (r_y - pos_y) / dir_y;
    float   t_boundary_z = dir_zero(dir_z) ? INFINITY : (r_z - pos_z) / dir_z;

    // same thing, now r is step + pos
    float   t_delta_x    = dir_zero(dir_x) ? INFINITY : step_x / dir_x;
    float   t_delta_y    = dir_zero(dir_y) ? INFINITY : step_y / dir_y;
    float   t_delta_z    = dir_zero(dir_z) ? INFINITY : step_z / dir_z;
    #undef dir_zero

    #define do_step_x() x += step_x; t_boundary_x += t_delta_x
    #define do_step_y() y += step_y; t_boundary_y += t_delta_y
    #define do_step_z() z += step_z; t_boundary_z += t_delta_z

    vec3       dist;
    float      max_distance_sq = max_distance * max_distance;
    do {
        if (t_boundary_x < t_boundary_z) {
            if (t_boundary_x < t_boundary_y) {
                do_step_x();
            } else {
                do_step_y();
            }
        } else {
            if (t_boundary_z < t_boundary_y) {
                do_step_z();
            } else {
                do_step_y();
            }
        }
        b = y < 0 || y > (CHUNK_HEIGHT-1) ? BLOCK_AIR : world_get_block(w, (bpos){x, y, z});
        vec3_sub(&dist, &(vec3){x, y, z}, &camera->pos);
    } while (!block_is_opaque(b) && vec3_len_squared(&dist) < max_distance_sq);
    *block = b;
    return (ubpos){x, y, z};

    #undef do_step_x
    #undef do_step_y
    #undef do_step_z
}