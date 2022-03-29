#pragma once

#include "glad.h"
#include <GLFW/glfw3.h>
#include "world.h"
#include "model.h"
#include "camera.h"
#include <stdbool.h>
#include "cgmath.h"
#include "block.h"
#include "shaders/shader_block.h"
#include "shaders/shader_selector.h"

#define TICKS_PER_SEC 20
#define TIME_PER_TICK (1.0 / TICKS_PER_SEC)

typedef struct mouse_state {
    double x;
    double y;
    bool   in_game;
} mouse_state;

typedef struct game {
    GLFWwindow      *window;
    double          current_time;
    double          accumulator;
    bool            running;
    world           world;
    model           model;
    selector        selector;
    camera          camera;
    mouse_state     mouse_state;

    shader_block    shader_block;
    shader_selector shader_selector;
} game;

void game_init(game *g, GLFWwindow *window);
void game_run(game *g);
// called within gameloop to end
void game_end(game *g);