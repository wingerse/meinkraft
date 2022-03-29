#include "chunk.h"
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include "util.h"
#include "containers/gl_list.h"
#include <memory.h>

static void chunk_sec_init(chunk_sec *cs)
{
    glGenVertexArrays(1, &cs->vao);
    glBindVertexArray(cs->vao);
    glGenBuffers(1, &cs->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, cs->vbo);
    glGenBuffers(1, &cs->ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cs->ebo);
    shader_block_set_up_attributes();
    memset(cs->data, BLOCK_AIR, CHUNK_SEC_SIZE);
    cs->index_count = 0;
    cs->block_count = 0;
}

static block_type chunk_sec_get_block(const chunk_sec *cs, csbpos pos) 
{
    return cs->data[pos.y][pos.z][pos.x];
}

/*
 * Returned integer has (1 << dir) set if modifying block in pos would affect the 
 * chunk sec in that dir (As in need remeshing).
 */
uint8_t chunk_sec_get_block_affected(csbpos pos) 
{
    uint8_t ret = 0;
    if      (pos.z == 0)                  ret |= (1 << DIR_NORTH);
    else if (pos.z == CHUNK_SIDE-1)       ret |= (1 << DIR_SOUTH);
    if      (pos.x == CHUNK_SIDE-1)       ret |= (1 << DIR_EAST);
    else if (pos.x == 0)                  ret |= (1 << DIR_WEST);
    if      (pos.y == CHUNK_SEC_HEIGHT-1) ret |= (1 << DIR_UP);
    else if (pos.y == 0)                  ret |= (1 << DIR_DOWN);
    return ret;
}

static void chunk_sec_set_block(chunk_sec *cs, csbpos pos, block_type b)
{
    block_type prev = chunk_sec_get_block(cs, pos);
    cs->data[pos.y][pos.z][pos.x] = b;
    if (prev == BLOCK_AIR && b != BLOCK_AIR) {
        cs->block_count++;
    } else if (b == BLOCK_AIR && prev != BLOCK_AIR) {
        cs->block_count--;
    }
}

static void chunk_sec_destroy(chunk_sec *cs)
{
    glDeleteVertexArrays(1, &cs->vao);
    glDeleteBuffers(1, &cs->vbo);
    glDeleteBuffers(1, &cs->ebo);
}

static bool chunk_sec_get_next_block(
    const chunk_sec *cs, 
    csbpos          csbp, 
    dir             dir, 
    const chunk_sec *(*dir_secs)[DIRS_COUNT], 
    block_type      *ret)
{
    csbpos new_csbp = csbpos_offset(csbp, dir);
    bool from_dir = false;
    switch (dir) {
    case DIR_NORTH: if (csbp.z == 0)                  { from_dir = true; } break;
    case DIR_SOUTH: if (csbp.z == CHUNK_SIDE-1)       { from_dir = true; } break;
    case DIR_EAST:  if (csbp.x == CHUNK_SIDE-1)       { from_dir = true; } break;
    case DIR_WEST:  if (csbp.x == 0)                  { from_dir = true; } break;
    case DIR_UP:    if (csbp.y == CHUNK_SEC_HEIGHT-1) { from_dir = true; } break;
    case DIR_DOWN:  if (csbp.y == 0)                  { from_dir = true; } break;
    default:
        unreachable();
        return (block_type) {0};
    }

    if (from_dir) {
        cs = (*dir_secs)[dir];
        if (cs == NULL) {
            if (dir == DIR_UP || dir == DIR_DOWN) {
                *ret = BLOCK_AIR;
                return true;
            }
            return false;
        }
    } 
    *ret = chunk_sec_get_block(cs, new_csbp);
    return true;
}

static void chunk_sec_remesh(chunk_sec *cs, const chunk_sec * (*dir_secs)[DIRS_COUNT])
{
    static shader_block_vertex vertex_list[CHUNK_SEC_SIZE * BLOCK_VERTICES_COUNT];
    static GLuint index_list[CHUNK_SEC_SIZE * BLOCK_INDICES_COUNT];

    size_t vertex_list_index = 0;
    size_t index_list_index = 0;
    size_t faces_added = 0; 

    for (int y = 0; y < CHUNK_SEC_HEIGHT; y++) {
        for (int z = 0; z < CHUNK_SIDE; z++) {
            for (int x = 0; x < CHUNK_SIDE; x++) {
                csbpos csbp = {x, y, z};
                block_type b = chunk_sec_get_block(cs, csbp);
                if (b == BLOCK_AIR) continue;

                block_type next_blocks[DIRS_COUNT];
                bool next_blocks_present[DIRS_COUNT] = {
                    chunk_sec_get_next_block(cs, csbp, DIR_NORTH, dir_secs, &next_blocks[DIR_NORTH]),
                    chunk_sec_get_next_block(cs, csbp, DIR_SOUTH, dir_secs, &next_blocks[DIR_SOUTH]),
                    chunk_sec_get_next_block(cs, csbp, DIR_EAST, dir_secs, &next_blocks[DIR_EAST]),
                    chunk_sec_get_next_block(cs, csbp, DIR_WEST, dir_secs, &next_blocks[DIR_WEST]),
                    chunk_sec_get_next_block(cs, csbp, DIR_UP, dir_secs, &next_blocks[DIR_UP]),
                    chunk_sec_get_next_block(cs, csbp, DIR_DOWN, dir_secs, &next_blocks[DIR_DOWN]),
                };

                for (dir face = 0; face < DIRS_COUNT; face++) {
                    // don't render map edges
                    if (!next_blocks_present[face]) continue;
                    // no need to render face sandwiched between two blocks and can't be seen.
                    if (block_is_opaque(next_blocks[face])) continue;
                    for (int i = 0; i < BLOCK_FACE_VERTICES_COUNT; i++) {
                        shader_block_vertex *v = &vertex_list[vertex_list_index++];
                        *v = block_face_vertices[face][i];
                        v->pos_x += x;
                        v->pos_y += y;
                        v->pos_z += z;
                        v->uv_s = (v->uv_s + block_atlas_indices[b][face].s) * BLOCK_TEX_SIDE_S; 
                        v->uv_t = (v->uv_t + block_atlas_indices[b][face].t) * BLOCK_TEX_SIDE_T; 
                    }
                    GLuint indices[] = {block_face_indices(faces_added)};
                    for (GLuint i = 0; i < BLOCK_FACE_INDICES_COUNT; i++) {
                        index_list[index_list_index++] = indices[i];
                    }
                    faces_added++;
                }
            }
        }
    }
    
    cs->index_count = index_list_index;
    glBindBuffer(GL_ARRAY_BUFFER, cs->vbo);
    glBufferData(GL_ARRAY_BUFFER, vertex_list_index * sizeof(shader_block_vertex), vertex_list, GL_STATIC_DRAW);
    glBindVertexArray(cs->vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cs->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_list_index * sizeof(GLuint), index_list, GL_STATIC_DRAW);
}

void chunk_init(chunk *c) 
{
    for (int i = 0; i < CHUNK_SEC_COUNT; i++) {
        chunk_sec_init(&c->secs[i]);
    }
}

void chunk_set_block(chunk *c, cbpos pos, block_type b)
{
    int section = section_from_cbpos(pos);
    chunk_sec *cs = &c->secs[section];
    csbpos p = cbpos_to_csbpos(pos);
    chunk_sec_set_block(cs, p, b);
}

uint8_t chunk_setr_block(chunk *c, cbpos pos, block_type b, chunk *(*dir_chunks)[4])
{
    int section = section_from_cbpos(pos);
    chunk_sec *cs = &c->secs[section];
    csbpos p = cbpos_to_csbpos(pos);
    chunk_sec_set_block(cs, p, b);
    chunk_remesh_sec(c, section, (const chunk *(*)[4])dir_chunks);

    uint8_t affected = chunk_sec_get_block_affected(p);
    if ((affected & (1 << DIR_UP)) != 0) {
        if (section < CHUNK_SEC_HEIGHT-1) {
            chunk_remesh_sec(c, section+1, (const chunk *(*)[4])dir_chunks);
        }
    }
    if ((affected & (1 << DIR_DOWN)) != 0) {
        if (section > 0) {
            chunk_remesh_sec(c, section-1, (const chunk *(*)[4])dir_chunks);
        }
    }

    return affected;
}

block_type chunk_get_block(const chunk *c, cbpos pos)
{
    int section = section_from_cbpos(pos);
    const chunk_sec *cs = &c->secs[section];
    csbpos p = cbpos_to_csbpos(pos);
    return chunk_sec_get_block(cs, p);
}

void chunk_remesh_sec(chunk *c, int sec, const chunk *(*dir_chunks)[4])
{
    chunk_sec *cs = &c->secs[sec];
    if (cs->block_count == 0) return;
    const chunk_sec * dir_secs[DIRS_COUNT] = {
        (*dir_chunks)[DIR_NORTH] == NULL ? NULL : &(*dir_chunks)[DIR_NORTH]->secs[sec], 
        (*dir_chunks)[DIR_SOUTH] == NULL ? NULL : &(*dir_chunks)[DIR_SOUTH]->secs[sec], 
        (*dir_chunks)[DIR_EAST] == NULL ? NULL : &(*dir_chunks)[DIR_EAST]->secs[sec], 
        (*dir_chunks)[DIR_WEST] == NULL ? NULL : &(*dir_chunks)[DIR_WEST]->secs[sec], 
        sec == CHUNK_SEC_COUNT-1 ? NULL : &c->secs[sec+1], 
        sec == 0 ? NULL : &c->secs[sec-1], 
    };

    chunk_sec_remesh(cs, &dir_secs);
}

void chunk_remesh(chunk *c, const chunk * (*dir_chunks)[4])
{
    for (int section = 0; section < CHUNK_SEC_COUNT; section++) {
        chunk_remesh_sec(c, section, dir_chunks);
    }
}

void chunk_render(const chunk *c, cpos pos, const camera *camera, shader_block *shader)
{
    for (int section = 0; section < CHUNK_SEC_COUNT; section++) {
        const chunk_sec *cs = &c->secs[section];
        if (cs->block_count == 0) continue;
        mat4 model_matrix;
        bpos bp = cpos_to_bpos(pos);
        bp.y += section * CHUNK_SEC_HEIGHT;
        mat4_init_translation(&model_matrix, bp.x, bp.y, bp.z);
        mat4 mv_matrix;
        mat4_mul(&mv_matrix, &camera->view_matrix, &model_matrix);

        AABB aabb = {{0, 0, 0}, {CHUNK_SIDE, CHUNK_SEC_HEIGHT, CHUNK_SIDE}};
        if (AABB_outside_frustum(&aabb, &camera->frustum_planes, &mv_matrix)) continue;

        mat4 mvp_matrix;
        mat4_mul(&mvp_matrix, &camera->proj_matrix, &mv_matrix);
        glUniformMatrix4fv(shader->mvp_matrix_location, 1, GL_FALSE, (float *)mvp_matrix.arr);
        glBindVertexArray(cs->vao);
        glDrawElements(GL_TRIANGLES, cs->index_count, GL_UNSIGNED_INT, 0);
    }
}

void chunk_destroy(chunk *c)
{
    for (int i = 0; i < CHUNK_SEC_COUNT; i++) {
        chunk_sec_destroy(&c->secs[i]);
    }
}