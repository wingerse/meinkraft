#pragma once

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdbool.h>

#define rad_from_deg(deg) ((float)deg * (M_PI / 180.0))
#define deg_from_rad(rad) ((float)rad * (180.0 / M_PI))

// This will store an extra float for simd reasons. Make sure it is always 0 (always use struct initialization syntax)
typedef union vec3 {
    float arr[4];
    struct {
        float x;
        float y;
        float z;
    };
} __attribute__((__aligned__(16))) vec3;

#define VEC3_UNIT_X (vec3){1.0, 0.0, 0.0}
#define VEC3_UNIT_Y (vec3){0.0, 1.0, 0.0}
#define VEC3_UNIT_Z (vec3){0.0, 0.0, 1.0}
#define VEC3_ZERO (vec3){0.0}

void vec3_add(vec3 *res, const vec3 *a, const vec3 *b);
void vec3_scale(vec3 *res, const vec3 *a, float val); 
void vec3_neg(vec3 *res, const vec3 *a);
void vec3_sub(vec3 *res, const vec3 *a, const vec3 *b);
void vec3_cross(vec3 *res, const vec3 *a, const vec3 *b);
float vec3_dot(const vec3 *a, const vec3 *b);
float vec3_len_squared(const vec3 *a);
float vec3_len(const vec3 *a);
void vec3_normalize(vec3 *res, const vec3 *a);

typedef union vec4 {
    float arr[4];
    struct {
        float x;
        float y;
        float z;
        float w;
    };
} __attribute__((__aligned__(16))) vec4;

/*
 * Matrices are stored in column major order
 */
typedef struct mat4 {
    float arr[4][4];
} __attribute__((__aligned__(16))) mat4;

#define mat4_elem(ptr, row, col) (ptr)->arr[col][row]

#define MAT4_IDENTITY (mat4){\
    {{1.0, 0.0, 0.0, 0.0},\
     {0.0, 1.0, 0.0, 0.0},\
     {0.0, 0.0, 1.0, 0.0},\
     {0.0, 0.0, 0.0, 1.0}}\
}

#define MAT4_ZERO (mat4){0.0}

void mat4_add(mat4 *res, const mat4 *a, const mat4 *b);
void mat4_scale(mat4 *res, const mat4 *a, float val);
void mat4_neg(mat4 *res, const mat4 *a);
void mat4_sub(mat4 *res, const mat4 *a, const mat4 *b);
void mat4_mul(mat4 *res, const mat4 *a, const mat4 *b);
void mat4_mul_vec4(vec4 *res, const mat4 *a, const vec4 *b);
void mat4_transpose(mat4 *res, const mat4 *a);
void mat4_inverse(mat4 *res, const mat4 *a);

void mat4_init_scale(mat4 *m, float scale_x, float scale_y, float scale_z); 
void mat4_init_translation(mat4 *m, float tx, float ty, float tz);
void mat4_init_rotate_x(mat4 *m, float angle); 
void mat4_init_rotate_y(mat4 *m, float angle); 
void mat4_init_rotate_z(mat4 *m, float angle); 
void mat4_init_rotate_arbitrary_axis(mat4 *m, const vec3 *axis, float angle); 
void mat4_init_coordinate_transform(mat4 *m, const vec3 *pos, const vec3 *z, const vec3 *y, const vec3 *x);
void mat4_init_look(mat4 *m, const vec3 *pos, const vec3 *dir, const vec3 *up);
void mat4_init_look_at(mat4 *m, const vec3 *pos, const vec3 *target_pos, const vec3 *up);
void mat4_init_ortho(mat4 *m, float left, float right, float bottom, float top, float znear, float zfar); 
void mat4_init_perspective(mat4 *m, float fovy, float aspect, float znear, float zfar);

/*
 * Given a point o and normal vector n, a plane is defined as the set of all points p = (x,y,z) where:
 * n.(p-o) = 0
 * If n = (a,b,c), then the equation can be expanded to:
 * ax+by+cz+d = 0 where d = -n.o 
 */
typedef union plane {
    float arr[4];
    struct {
        float a, b, c, d;
    };
} __attribute__((__aligned__(16))) plane;

void plane_normalize(plane *res, const plane *p);
float plane_distance_to_point(const plane *p, const vec3 *point);

/*
 * Gets frustum planes. Planes are not normalized. Normals point inside
 */
void frustum_extract_planes(plane (*planes)[6], const mat4 *matrix);
bool frustum_vertices_outside(const plane (*frustum)[6], const vec3 (*vertices)[8]);

typedef struct AABB {
    vec3 min, max;
} AABB;

void AABB_init(AABB *a, const vec3 *pa, const vec3 *pb);
void AABB_get_vertices(const AABB *a, vec3 (*vertices)[8]);
void AABB_transform(const AABB *a, vec3 (*vertices)[8], const mat4 *matrix);
bool AABB_outside_frustum(const AABB *a, const plane (*frustum)[6], const mat4 *matrix);

