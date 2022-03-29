#include "cgmath.h"
#include <pmmintrin.h>

void vec3_add(vec3 *res, const vec3 *a, const vec3 *b) 
{
    __m128 av = _mm_load_ps(a->arr); 
    __m128 bv = _mm_load_ps(b->arr); 
    _mm_store_ps(res->arr, _mm_add_ps(av, bv));
}

void vec3_scale(vec3 *res, const vec3 *a, float val) 
{
    _mm_store_ps(res->arr, _mm_mul_ps(_mm_load_ps(a->arr), _mm_set1_ps(val)));
}

void vec3_neg(vec3 *res, const vec3 *a) 
{
    vec3_scale(res, a, -1); 
}

void vec3_sub(vec3 *res, const vec3 *a, const vec3 *b) 
{
    vec3 tmp;
    vec3_neg(&tmp, b);
    vec3_add(res, a, &tmp);
}

void vec3_cross(vec3 *res, const vec3 *a, const vec3 *b)
{
    #if 0
    float ax = a->x, ay = a->y, az = a->z;
    float bx = b->x, by = b->y, bz = b->z;
    res->x = ay*bz - az*by;
    res->y = az*bx - ax*bz;
    res->z = ax*by - ay*bx;
    #endif
    __m128 av = _mm_load_ps(a->arr);
    __m128 bv = _mm_load_ps(b->arr);

    __m128 v1 = _mm_shuffle_ps(av, av, _MM_SHUFFLE(3, 0, 2, 1));
    __m128 v2 = _mm_shuffle_ps(bv, bv, _MM_SHUFFLE(3, 1, 0, 2));
    __m128 mul1 = _mm_mul_ps(v1, v2);

    v1 = _mm_shuffle_ps(av, av, _MM_SHUFFLE(3, 1, 0, 2));
    v2 = _mm_shuffle_ps(bv, bv, _MM_SHUFFLE(3, 0, 2, 1));
    __m128 mul2 = _mm_mul_ps(v1, v2);

    __m128 result = _mm_sub_ps(mul1, mul2);
    _mm_store_ps(res->arr, result);
}

float vec3_dot(const vec3 *a, const vec3 *b) 
{
    __m128 v = _mm_mul_ps(_mm_load_ps(a->arr), _mm_load_ps(b->arr));
    // horizontal add
    __m128 v1 = _mm_movehdup_ps(v);
    __m128 add = _mm_add_ps(v, v1);
    __m128 v2 = _mm_movehl_ps(v1, add);
    return _mm_cvtss_f32(_mm_add_ss(add, v2));
}

float vec3_len_squared(const vec3 *a) 
{
    return vec3_dot(a, a); 
}

float vec3_len(const vec3 *a) 
{
    #if 0
    return sqrt(vec3_len_squared(a));
    #endif
    // this confuses the compiler and emits useless movs so we manually dotproduct here.
    //return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(vec3_len_squared(a))));
    __m128 va = _mm_load_ps(a->arr);
    __m128 v = _mm_mul_ps(va, va);
    __m128 v1 = _mm_movehdup_ps(v);
    __m128 add = _mm_add_ps(v, v1);
    __m128 v2 = _mm_movehl_ps(v1, add);
    return _mm_cvtss_f32(_mm_sqrt_ss(_mm_add_ss(add, v2)));
}

void vec3_normalize(vec3 *res, const vec3 *a) 
{
    float len = vec3_len(a);
    _mm_store_ps(res->arr, _mm_div_ps(_mm_load_ps(a->arr), _mm_set1_ps(len)));
}

void mat4_add(mat4 *res, const mat4 *a, const mat4 *b) 
{
    for (int col = 0; col < 4; col++) {
        __m128 ac = _mm_load_ps(&mat4_elem(a, 0, col));
        __m128 bc = _mm_load_ps(&mat4_elem(b, 0, col));
        __m128 result = _mm_add_ps(ac, bc);
        _mm_store_ps(&mat4_elem(res, 0, col), result);
    }
}

void mat4_scale(mat4 *res, const mat4 *a, float val) 
{
    for (int col = 0; col < 4; col++) {
        __m128 ac = _mm_load_ps(&mat4_elem(a, 0, col));
        __m128 result = _mm_mul_ps(ac, _mm_set1_ps(val));
        _mm_store_ps(&mat4_elem(res, 0, col), result);
    }
}

void mat4_neg(mat4 *res, const mat4 *a) 
{
    mat4_scale(res, a, -1);
}

void mat4_sub(mat4 *res, const mat4 *a, const mat4 *b) 
{
    for (int col = 0; col < 4; col++) {
        __m128 ac = _mm_load_ps(&mat4_elem(a, 0, col));
        __m128 bc = _mm_load_ps(&mat4_elem(b, 0, col));
        __m128 result = _mm_sub_ps(ac, bc);
        _mm_store_ps(&mat4_elem(res, 0, col), result);
    }
}

void mat4_mul(mat4 *res, const mat4 *a, const mat4 *b)
{
    /*
     * The scalar algorithm does the dot product of a_rows and b_columns for every element (16 elements).
     * This can be done in simd by loading a_row by `set`s and b_col by `load`s and then doing dot product by 
     * vertical mul and horizontal add afterwards. 
     * But a faster way is processing one column of res a time instead of one element. To do this we do 4 dot 
     * products at a time. If we have data arranged in a SOA form, we can use the normal scalar formula for dot products
     * but using simd vectors instead. Fortunately, we can get the SOA form by loading a_cols instead of a_rows. 
     * Then the SOA form of b_col can be created by doing a broadcast of every element in b_col. 
     * Illustration:
     *   ax  *   bx  +   ay  *   by
     * |a00|   |b00|   |a01|   |b10|
     * |a10| * |b00| + |a11| * |b10| + ... = 
     * |a20|   |b00|   |a12|   |b10|
     * |a30|   |b00|   |a13|   |b10|
     *  ^        ^
     * a col 0  broadcasted b00
     */

    // cols of a
    __m128 ac0 = _mm_load_ps(&mat4_elem(a, 0, 0));
    __m128 ac1 = _mm_load_ps(&mat4_elem(a, 0, 1));
    __m128 ac2 = _mm_load_ps(&mat4_elem(a, 0, 2));
    __m128 ac3 = _mm_load_ps(&mat4_elem(a, 0, 3));

    for (int bcol = 0; bcol < 4; bcol++) {
        // do 4 dot products for the column at the same time
        // broadcasted elements of b column
        __m128 xxxx = _mm_set1_ps(mat4_elem(b, 0, bcol));
        __m128 yyyy = _mm_set1_ps(mat4_elem(b, 1, bcol));
        __m128 zzzz = _mm_set1_ps(mat4_elem(b, 2, bcol));
        __m128 wwww = _mm_set1_ps(mat4_elem(b, 3, bcol));

        __m128 fourdots = _mm_add_ps(
            _mm_add_ps(_mm_mul_ps(ac0, xxxx), _mm_mul_ps(ac1, yyyy)),
            _mm_add_ps(_mm_mul_ps(ac2, zzzz), _mm_mul_ps(ac3, wwww))
        );

        _mm_store_ps(&mat4_elem(res, 0, bcol), fourdots);
    }
}

void mat4_mul_vec4(vec4 *res, const mat4 *a, const vec4 *b)
{
    // this is just mat4 * mat4 except the last "matrix" is just one col, so no loop needed.
    // cols of a
    __m128 ac0 = _mm_load_ps(&mat4_elem(a, 0, 0));
    __m128 ac1 = _mm_load_ps(&mat4_elem(a, 0, 1));
    __m128 ac2 = _mm_load_ps(&mat4_elem(a, 0, 2));
    __m128 ac3 = _mm_load_ps(&mat4_elem(a, 0, 3));

    __m128 xxxx = _mm_set1_ps(b->x);
    __m128 yyyy = _mm_set1_ps(b->y);
    __m128 zzzz = _mm_set1_ps(b->z);
    __m128 wwww = _mm_set1_ps(b->w);

    __m128 fourdots = _mm_add_ps(
        _mm_add_ps(_mm_mul_ps(ac0, xxxx), _mm_mul_ps(ac1, yyyy)),
        _mm_add_ps(_mm_mul_ps(ac2, zzzz), _mm_mul_ps(ac3, wwww))
    );

    _mm_store_ps(res->arr, fourdots);
}

void mat4_transpose(mat4 *res, const mat4 *a) 
{
    __m128 c0 = _mm_load_ps(&mat4_elem(a, 0, 0));
    __m128 c1 = _mm_load_ps(&mat4_elem(a, 0, 1));
    __m128 c2 = _mm_load_ps(&mat4_elem(a, 0, 2));
    __m128 c3 = _mm_load_ps(&mat4_elem(a, 0, 3));
    _MM_TRANSPOSE4_PS(c0, c1, c2, c3);
    _mm_store_ps(&mat4_elem(res, 0, 0), c0);
    _mm_store_ps(&mat4_elem(res, 0, 1), c1);
    _mm_store_ps(&mat4_elem(res, 0, 2), c2);
    _mm_store_ps(&mat4_elem(res, 0, 3), c3);
}

void mat4_inverse(mat4 *res, const mat4 *a)
{
    // http://cfs9.tistory.com/upload_control/download.blog?fhandle=YmxvZzEzMjUyMkBmczkudGlzdG9yeS5jb206L2F0dGFjaC8wLzAucGRm&filename=InverseMatrix.pdf
    float *dst = (float *)res->arr;
    float *src = (float *)a->arr;

    __m128 minor0, minor1, minor2, minor3;
    __m128 row0, row1 = _mm_setzero_ps(), row2, row3 = _mm_setzero_ps();
    __m128 det, tmp1 = _mm_setzero_ps();

    tmp1 = _mm_loadh_pi(_mm_loadl_pi(tmp1, (__m64*)(src)), (__m64*)(src+ 4));
    row1 = _mm_loadh_pi(_mm_loadl_pi(row1, (__m64*)(src+8)), (__m64*)(src+12));

    row0 = _mm_shuffle_ps(tmp1, row1, 0x88);
    row1 = _mm_shuffle_ps(row1, tmp1, 0xDD);

    tmp1 = _mm_loadh_pi(_mm_loadl_pi(tmp1, (__m64*)(src+ 2)), (__m64*)(src+ 6));
    row3 = _mm_loadh_pi(_mm_loadl_pi(row3, (__m64*)(src+10)), (__m64*)(src+14));

    row2 = _mm_shuffle_ps(tmp1, row3, 0x88);
    row3 = _mm_shuffle_ps(row3, tmp1, 0xDD);

    tmp1 = _mm_mul_ps(row2, row3);
    tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);

    minor0 = _mm_mul_ps(row1, tmp1);
    minor1 = _mm_mul_ps(row0, tmp1);

    tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);

    minor0 = _mm_sub_ps(_mm_mul_ps(row1, tmp1), minor0);
    minor1 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor1);
    minor1 = _mm_shuffle_ps(minor1, minor1, 0x4E);

    tmp1 = _mm_mul_ps(row1, row2);
    tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);

    minor0 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor0);
    minor3 = _mm_mul_ps(row0, tmp1);

    tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);

    minor0 = _mm_sub_ps(minor0, _mm_mul_ps(row3, tmp1));
    minor3 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor3);
    minor3 = _mm_shuffle_ps(minor3, minor3, 0x4E);

    tmp1 = _mm_mul_ps(_mm_shuffle_ps(row1, row1, 0x4E), row3);
    tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
    row2 = _mm_shuffle_ps(row2, row2, 0x4E);

    minor0 = _mm_add_ps(_mm_mul_ps(row2, tmp1), minor0);
    minor2 = _mm_mul_ps(row0, tmp1);

    tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);

    minor0 = _mm_sub_ps(minor0, _mm_mul_ps(row2, tmp1));
    minor2 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor2);
    minor2 = _mm_shuffle_ps(minor2, minor2, 0x4E);

    tmp1 = _mm_mul_ps(row0, row1);
    tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);

    minor2 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor2);
    minor3 = _mm_sub_ps(_mm_mul_ps(row2, tmp1), minor3);

    tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);

    minor2 = _mm_sub_ps(_mm_mul_ps(row3, tmp1), minor2);
    minor3 = _mm_sub_ps(minor3, _mm_mul_ps(row2, tmp1));

    tmp1 = _mm_mul_ps(row0, row3);
    tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);

    minor1 = _mm_sub_ps(minor1, _mm_mul_ps(row2, tmp1));
    minor2 = _mm_add_ps(_mm_mul_ps(row1, tmp1), minor2);

    tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);

    minor1 = _mm_add_ps(_mm_mul_ps(row2, tmp1), minor1);
    minor2 = _mm_sub_ps(minor2, _mm_mul_ps(row1, tmp1));

    tmp1 = _mm_mul_ps(row0, row2);
    tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);

    minor1 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor1);
    minor3 = _mm_sub_ps(minor3, _mm_mul_ps(row1, tmp1));

    tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);

    minor1 = _mm_sub_ps(minor1, _mm_mul_ps(row3, tmp1));
    minor3 = _mm_add_ps(_mm_mul_ps(row1, tmp1), minor3);

    det = _mm_mul_ps(row0, minor0);
    det = _mm_add_ps(_mm_shuffle_ps(det, det, 0x4E), det);
    det = _mm_add_ss(_mm_shuffle_ps(det, det, 0xB1), det);

    tmp1 = _mm_rcp_ss(det);

    det = _mm_sub_ss(_mm_add_ss(tmp1, tmp1), _mm_mul_ss(det, _mm_mul_ss(tmp1, tmp1)));
    det = _mm_shuffle_ps(det, det, 0x00);

    minor0 = _mm_mul_ps(det, minor0);
    _mm_storel_pi((__m64*)(dst), minor0);
    _mm_storeh_pi((__m64*)(dst+2), minor0);

    minor1 = _mm_mul_ps(det, minor1);
    _mm_storel_pi((__m64*)(dst+4), minor1);
    _mm_storeh_pi((__m64*)(dst+6), minor1);

    minor2 = _mm_mul_ps(det, minor2);
    _mm_storel_pi((__m64*)(dst+ 8), minor2);
    _mm_storeh_pi((__m64*)(dst+10), minor2);

    minor3 = _mm_mul_ps(det, minor3);
    _mm_storel_pi((__m64*)(dst+12), minor3);
    _mm_storeh_pi((__m64*)(dst+14), minor3);
}

void mat4_init_scale(mat4 *m, float scale_x, float scale_y, float scale_z) 
{
    *m = (mat4) 
    {{
        {scale_x, 0.0,     0.0,     0.0},
        {0.0,     scale_y, 0.0,     0.0},
        {0.0,     0.0,     scale_z, 0.0},
        {0.0,     0.0,     0.0,     1.0}
    }};
} 

void mat4_init_translation(mat4 *m, float tx, float ty, float tz) 
{
    *m = (mat4) 
    {{
        {1.0, 0.0, 0.0, 0.0},
        {0.0, 1.0, 0.0, 0.0},
        {0.0, 0.0, 1.0, 0.0},
        {tx,  ty,  tz,  1.0}
    }};
}

void mat4_init_rotate_x(mat4 *m, float angle) 
{
    float cos_a = cos(angle);
    float sin_a = sin(angle);
    *m = (mat4) 
    {{
        {1.0,  0.0,   0.0,   0.0},
        {0.0,  cos_a, sin_a, 0.0},
        {0.0, -sin_a, cos_a, 0.0},
        {0.0,  0.0,   0.0,   1.0}
    }};
}

void mat4_init_rotate_y(mat4 *m, float angle) 
{
    float cos_a = cos(angle), sin_a = sin(angle);
    *m = (mat4) 
    {{
        { cos_a, 0.0, -sin_a, 0.0},
        { 0.0,   1.0,  0.0,   0.0},
        { sin_a, 0.0,  cos_a, 0.0},
        { 0.0,   0.0,  0.0,   1.0}
    }};
}

void mat4_init_rotate_z(mat4 *m, float angle) 
{
    float cos_a = cos(angle), sin_a = sin(angle);
    *m = (mat4) 
    {{
        { cos_a, sin_a, 0.0, 0.0},
        {-sin_a, cos_a, 0.0, 0.0},
        { 0.0,   0.0,   1.0, 0.0},
        { 0.0,   0.0,   0.0, 1.0}
    }};
}

void mat4_init_rotate_arbitrary_axis(mat4 *m, const vec3 *axis, float angle) 
{
    float cos_a = cos(angle);
    float sin_a = sin(angle);
    float omcos = 1 - cos_a;
    float x = axis->x, y = axis->y, z = axis->z;
    float xsq = x * x;
    float ysq = y * y;
    float zsq = z * z;
    float xy = x * y;
    float xz = x * z;
    float yz = y * z;
    float xsin = x * sin_a;
    float ysin = y * sin_a;
    float zsin = z * sin_a;
    *m = (mat4)
    {{
        { xsq * omcos + cos_a, xy * omcos + zsin,   xz * omcos - ysin,   0 },
        { xy * omcos - zsin,   ysq * omcos + cos_a, yz * omcos + xsin,   0 },
        { xz * omcos + ysin,   yz * omcos - xsin,   zsq * omcos + cos_a, 0 },
        { 0,                   0,                   0,                   1 }
    }};
}

void mat4_init_coordinate_transform(mat4 *m, const vec3 *pos, const vec3 *z, const vec3 *y, const vec3 *x) 
{
    mat4 translate;
    mat4_init_translation(&translate, -pos->x, -pos->y, -pos->z);
    mat4 transform = {{
        { x->x, y->x, z->x, 0.0 },
        { x->y, y->y, z->y, 0.0 },
        { x->z, y->z, z->z, 0.0 },
        { 0.0,  0.0,  0.0,  1.0 }
    }};
    mat4_mul(m, &transform, &translate);
}

/*
 * up does not have to be perpendicular to dir
 */
void mat4_init_look(mat4 *m, const vec3 *pos, const vec3 *dir, const vec3 *up) 
{
    vec3 z;
    vec3_neg(&z, dir);
    vec3 right;
    vec3_cross(&right, up, &z);
    vec3_normalize(&right, &right);
    vec3 u;
    vec3_cross(&u, &z, &right);
    mat4_init_coordinate_transform(m, pos, &z, &u, &right);
}

void mat4_init_look_at(mat4 *m, const vec3 *pos, const vec3 *target_pos, const vec3 *up)
{
    vec3 dir;
    vec3_sub(&dir, target_pos, pos);
    vec3_normalize(&dir, &dir);
    mat4_init_look(m, pos, &dir, up);
}

void mat4_init_ortho(mat4 *m, float left, float right, float bottom, float top, float znear, float zfar) 
{
    *m = (mat4) 
    {{
        { 2 / (right - left),                  0.0,                                0.0,                               0.0},
        { 0.0,                                 2 / (top - bottom),                 0.0,                               0.0},
        { 0.0,                                 0.0,                               -(2 / (zfar - znear)),              0.0},
        { -((right + left) / (right - left)), -((top + bottom) / (top - bottom)), -((zfar + znear) / (zfar - znear)), 1.0}
    }};
}

void mat4_init_perspective(mat4 *m, float fovy, float aspect, float znear, float zfar)
{
    float f = 1 / tan(fovy / 2);
    *m = (mat4) 
    {{
        { f/aspect, 0.0, 0.0,                                  0.0 },
        { 0.0,      f,   0.0,                                  0.0 },
        { 0.0,      0.0, (zfar + znear) / (znear - zfar),     -1.0 },
        { 0.0,      0.0, (2 * zfar * znear) / (znear - zfar),  0.0 }
    }};
}

void plane_normalize(plane *res, const plane *p)
{
    vec3 normal = {p->a, p->b, p->c};
    float len = vec3_len(&normal);
    _mm_store_ps(res->arr, _mm_div_ps(_mm_load_ps(p->arr), _mm_set1_ps(len)));
}

float plane_distance_to_point(const plane *p, const vec3 *point)
{
    return p->a*point->x + p->b*point->y + p->c*point->z + p->d;
}

void frustum_extract_planes(plane (*planes)[6], const mat4 *matrix)
{
    // method described here simdized:
    // Fast Extraction of Viewing Frustum Planes from the World-View-Projection Matrix
    // http://www.cs.otago.ac.nz/postgrads/alexis/planeExtraction.pdf

    __m128 row0 = _mm_setr_ps(mat4_elem(matrix, 0, 0), mat4_elem(matrix, 0, 1), mat4_elem(matrix, 0, 2), mat4_elem(matrix, 0, 3));
    __m128 row1 = _mm_setr_ps(mat4_elem(matrix, 1, 0), mat4_elem(matrix, 1, 1), mat4_elem(matrix, 1, 2), mat4_elem(matrix, 1, 3));
    __m128 row2 = _mm_setr_ps(mat4_elem(matrix, 2, 0), mat4_elem(matrix, 2, 1), mat4_elem(matrix, 2, 2), mat4_elem(matrix, 2, 3));
    __m128 row3 = _mm_setr_ps(mat4_elem(matrix, 3, 0), mat4_elem(matrix, 3, 1), mat4_elem(matrix, 3, 2), mat4_elem(matrix, 3, 3));
    // far
    _mm_store_ps((*planes)[0].arr, _mm_sub_ps(row3, row2));
    // near
    _mm_store_ps((*planes)[1].arr, _mm_add_ps(row3, row2));
    // right
    _mm_store_ps((*planes)[2].arr, _mm_sub_ps(row3, row0));
    // left
    _mm_store_ps((*planes)[3].arr, _mm_add_ps(row3, row0));
    // up
    _mm_store_ps((*planes)[4].arr, _mm_sub_ps(row3, row1));
    // down
    _mm_store_ps((*planes)[5].arr, _mm_add_ps(row3, row1));
}

bool frustum_vertices_outside(const plane (*frustum)[6], const vec3 (*vertices)[8])
{
    for (int i = 0; i < 6; i++) {
        bool all_outside = true;
        for (int j = 0; j < 8; j++) {
            float distance = plane_distance_to_point(&(*frustum)[i], &(*vertices)[j]);
            if (distance >= 0) {
                all_outside = false;
                break;
            } 
        }
        if (all_outside) return true;
    }
    return false;
}

void AABB_init(AABB *a, const vec3 *pa, const vec3 *pb)
{
    a->min = (vec3){fmin(pa->x, pb->x), fmin(pa->y, pb->y), fmin(pa->z, pb->z)};
    a->max = (vec3){fmax(pa->x, pb->x), fmax(pa->y, pb->y), fmax(pa->z, pb->z)};
}

void AABB_get_vertices(const AABB *a, vec3 (*vertices)[8])
{
    (*vertices)[0] = (vec3){a->min.x, a->min.y, a->min.z};
    (*vertices)[1] = (vec3){a->max.x, a->min.y, a->min.z};
    (*vertices)[2] = (vec3){a->max.x, a->min.y, a->max.z};
    (*vertices)[3] = (vec3){a->min.x, a->min.y, a->max.z};
    (*vertices)[4] = (vec3){a->min.x, a->max.y, a->min.z};
    (*vertices)[5] = (vec3){a->max.x, a->max.y, a->min.z};
    (*vertices)[6] = (vec3){a->max.x, a->max.y, a->max.z};
    (*vertices)[7] = (vec3){a->min.x, a->max.y, a->max.z};
}

void AABB_transform(const AABB *a, vec3 (*vertices)[8], const mat4 *matrix)
{
    vec3 initial_vertices[8];
    AABB_get_vertices(a, &initial_vertices);
    for (int i = 0; i < 8; i++) {
        vec3 v = initial_vertices[i];
        vec4 v4 = {v.x, v.y, v.z, 1};
        mat4_mul_vec4(&v4, matrix, &v4);
        (*vertices)[i] = (vec3){v4.x, v4.y, v4.z};
    }
}

bool AABB_outside_frustum(const AABB *a, const plane (*frustum)[6], const mat4 *matrix)
{
    vec3 vertices[8];
    AABB_transform(a, &vertices, matrix);
    return frustum_vertices_outside(frustum, (const vec3 (*)[8])&vertices);
}