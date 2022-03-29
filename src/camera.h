#pragma once

#include "cgmath.h"

typedef struct camera {
    vec3  pos;
    vec3  dir;
    float pitch;
    float yaw;
    mat4  proj_matrix;
    mat4  view_matrix;
    mat4  vp_matrix;
    plane frustum_planes[6];
} camera;

void camera_init_custom(camera *c, mat4 *proj_matrix, vec3 *pos, float pitch, float yaw);
void camera_init(camera *c, mat4 *proj_matrix);
void camera_move_forward(camera *c, float val);
void camera_move_backward(camera *c, float val);
void camera_move_right(camera *c, float val);
void camera_move_left(camera *c, float val);
void camera_move_up(camera *c, float val);
void camera_move_down(camera *c, float val);
void camera_set_yaw_pitch(camera *c, float yaw, float pitch);
void camera_set_proj_matrix(camera *c, mat4 *proj_matrix);
void camera_update_view_matrix(camera *c);