#pragma once

#include <stdint.h>
#include <stdbool.h>

// all these constants are measured in blocks
// and should be power of 2
#define CHUNK_SEC_HEIGHT      16
#define CHUNK_SEC_HEIGHT_BITS 4
#define CHUNK_SEC_COUNT       16
#define CHUNK_SEC_SIZE        (CHUNK_SEC_HEIGHT * CHUNK_SIDE * CHUNK_SIDE)

#define CHUNK_SIDE            16
#define CHUNK_SIDE_BITS       4
#define CHUNK_HEIGHT          (CHUNK_SEC_HEIGHT * CHUNK_SEC_COUNT)
#define CHUNK_HEIGHT_BITS     8
#define CHUNK_SIZE            (CHUNK_SEC_COUNT * CHUNK_SEC_SIZE)

typedef enum dir {
    DIR_NORTH, // -z
    DIR_SOUTH, // +z
    DIR_EAST,  // +x
    DIR_WEST,  // -x
    DIR_UP,    // +y
    DIR_DOWN,  // -y

    DIRS_COUNT
} dir;

typedef struct cpos {
    int32_t x, z;
} cpos;

uint32_t cpos_hash(const cpos *p);
bool     cpos_eq(const cpos *a, const cpos *b);

typedef struct cbpos {
    uint32_t x: CHUNK_SIDE_BITS;
    uint32_t y: CHUNK_HEIGHT_BITS;
    uint32_t z: CHUNK_SIDE_BITS;
} cbpos;

typedef struct csbpos {
    uint32_t x: CHUNK_SIDE_BITS;
    uint32_t y: CHUNK_SEC_HEIGHT_BITS;
    uint32_t z: CHUNK_SIDE_BITS;
} csbpos;

typedef struct bpos {
    int32_t  x;
    uint32_t y: CHUNK_HEIGHT_BITS;
    int32_t  z;
} bpos;

typedef struct ubpos {
    int32_t x;
    int32_t y;
    int32_t z;
} ubpos;

bpos   cpos_to_bpos(cpos p);
cpos   bpos_to_cpos(bpos p);
cpos   cpos_offset(cpos p, dir d);
cbpos  bpos_to_cbpos(bpos p);
csbpos cbpos_to_csbpos(cbpos p);
csbpos csbpos_offset(csbpos p, dir d);
bpos   cpos_cbpos_to_bpos(cpos cp, cbpos cbp);
int    section_from_cbpos(cbpos p);