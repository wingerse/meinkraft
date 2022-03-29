#include "pos.h"
#include "util.h"

uint32_t cpos_hash(const cpos *p) 
{
    uint32_t hash = 7;
    hash = 31 * hash + (uint32_t)p->x;
    hash = 31 * hash + (uint32_t)p->z;
    return hash;
}

bool cpos_eq(const cpos *a, const cpos *b)
{
    return a->x == b->x && a->z == b->z;
}

bpos cpos_to_bpos(cpos p)
{
    return (bpos){p.x << CHUNK_SIDE_BITS, 0, p.z << CHUNK_SIDE_BITS};
}

cpos bpos_to_cpos(bpos p)
{
    // should round toward negative by using shift
    return (cpos){p.x >> CHUNK_SIDE_BITS, p.z >> CHUNK_SIDE_BITS};
}

cpos cpos_offset(cpos p, dir d)
{
    switch (d) {
    // will wrap due to bitfields
    case DIR_NORTH: return (cpos){p.x, p.z-1};
    case DIR_SOUTH: return (cpos){p.x, p.z+1};
    case DIR_EAST:  return (cpos){p.x+1, p.z};
    case DIR_WEST:  return (cpos){p.x-1, p.z};
    default:
        unreachable();
        return (cpos){0};
    }
}

cbpos bpos_to_cbpos(bpos p)
{
    // cbp's x and z are CHUNK_SIDE_BITS wide so the masks are done automatically
    return (cbpos){(uint32_t)p.x, (uint32_t)p.y, (uint32_t)p.z};
}

csbpos cbpos_to_csbpos(cbpos p)
{
    // masks done automatically due to bitfields
    return (csbpos){(uint32_t)p.x, (uint32_t)p.y, (uint32_t)p.z};
}

csbpos csbpos_offset(csbpos p, dir d)
{
    switch (d) {
    // will wrap due to bitfields
    case DIR_NORTH: return (csbpos){p.x, p.y, p.z-1};
    case DIR_SOUTH: return (csbpos){p.x, p.y, p.z+1};
    case DIR_EAST:  return (csbpos){p.x+1, p.y, p.z};
    case DIR_WEST:  return (csbpos){p.x-1, p.y, p.z};
    case DIR_UP:    return (csbpos){p.x, p.y+1, p.z};
    case DIR_DOWN:  return (csbpos){p.x, p.y-1, p.z};
    default:
        unreachable();
        return (csbpos){0};
    }
}

bpos cpos_cbpos_to_bpos(cpos cp, cbpos cbp)
{
    bpos p = cpos_to_bpos(cp);
    return (bpos){p.x + cbp.x, cbp.y, p.z + cbp.z};
}

int section_from_cbpos(cbpos p) 
{
    return p.y / CHUNK_SEC_HEIGHT;
}