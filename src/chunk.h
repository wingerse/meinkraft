#pragma once

#include "glad.h"
#include "block.h"
#include <stdint.h>
#include "pos.h"
#include "cgmath.h"
#include "camera.h"
#include "shaders/shader_block.h"

typedef struct chunk_sec {
    // yzx
    uint8_t  data[CHUNK_SEC_HEIGHT][CHUNK_SIDE][CHUNK_SIDE];
    GLuint   vao, ebo, vbo;
    uint16_t block_count;
    size_t   index_count;
} chunk_sec;

typedef struct chunk {
    chunk_sec secs[CHUNK_SEC_COUNT];
} chunk;

void       chunk_init(chunk *c);
block_type chunk_get_block(const chunk *c, cbpos pos);
void       chunk_set_block(chunk *const c, cbpos pos, block_type b);
uint8_t    chunk_setr_block(chunk *c, cbpos pos, block_type b, chunk *(*dir_chunks)[4]);
void       chunk_render(const chunk *c, cpos pos, const camera *camera, shader_block *shader);
void       chunk_remesh(chunk *c, const chunk * (*dir_chunks)[4]);
void       chunk_remesh_sec(chunk *c, int sec, const chunk *(*dir_chunks)[4]);
void       chunk_destroy(chunk *c);