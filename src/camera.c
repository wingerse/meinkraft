#include "camera.h"
#include <stdio.h>

void camera_init_custom(camera *c, mat4 *proj_matrix, vec3 *pos, float pitch, float yaw)
{
    camera_set_proj_matrix(c, proj_matrix);
    camera_set_yaw_pitch(c, pitch, yaw);
    c->pos = *pos;
    camera_update_view_matrix(c);
}

void camera_init(camera *c, mat4 *proj_matrix)
{
    camera_init_custom(c, proj_matrix, &VEC3_ZERO, 0, 0);
}

void camera_move_forward(camera *c, float val)
{
    vec3 dir = {c->dir.x, 0, c->dir.z};
    vec3_normalize(&dir, &dir);
    vec3_scale(&dir, &dir, val);
    vec3_add(&c->pos, &c->pos, &dir);
}

void camera_move_backward(camera *c, float val)
{
    camera_move_forward(c, -val);
}

void camera_move_right(camera *c, float val)
{
    vec3 dir;
    vec3_neg(&dir, &c->dir);
    vec3 right;
    vec3_cross(&right, &VEC3_UNIT_Y, &dir);
    vec3_normalize(&right, &right);

    vec3_scale(&right, &right, val);
    vec3_add(&c->pos, &c->pos, &right);
}

void camera_move_left(camera *c, float val)
{
    camera_move_right(c, -val);
}

void camera_move_up(camera *c, float val)
{
    c->pos.y += val;
}

void camera_move_down(camera *c, float val)
{
    camera_move_up(c, -val);
}

void camera_set_yaw_pitch(camera *c, float yaw, float pitch)
{
    if (pitch > 89.0)  pitch = 89.0;
    if (pitch < -89.0) pitch = -89.0;
    if (yaw > 180)     yaw   -= 180 * 2;
    if (yaw < -180)    yaw   += 180 * 2;

    c->pitch   = pitch;
    c->yaw     = yaw;
    pitch      = rad_from_deg(c->pitch);
    yaw        = rad_from_deg(c->yaw);
    float hlen = cos(pitch);
    c->dir.x   = hlen * -sin(yaw);
    c->dir.y   = sin(pitch);
    c->dir.z   = hlen * cos(yaw);
    vec3_normalize(&c->dir, &c->dir);
}

void camera_set_proj_matrix(camera *c, mat4 *proj_matrix)
{
    c->proj_matrix = *proj_matrix;
    frustum_extract_planes(&c->frustum_planes, &c->proj_matrix);
}

void camera_update_view_matrix(camera *c)
{
    mat4_init_look(&c->view_matrix, &c->pos, &c->dir, &VEC3_UNIT_Y);
    mat4_mul(&c->vp_matrix, &c->proj_matrix, &c->view_matrix);
}