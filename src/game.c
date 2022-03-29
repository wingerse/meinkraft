#include "game.h"
#include <stdio.h>
#include "util.h"
#include <stdio.h>

void game_init(game *g, GLFWwindow *window)
{
    g->window = window;
    g->current_time = glfwGetTime();
    g->accumulator = 0;
    g->running = true;
    world_init(&g->world);
    model_init(&g->model);
    selector_init(&g->selector);
    mat4 proj_matrix;
    mat4_init_perspective(&proj_matrix, rad_from_deg(100), 1024.0 / 800.0, 0.1, 1000);
    camera_init_custom(&g->camera, &proj_matrix, &(vec3){30, 61, 30}, 0, 0);
    g->mouse_state = (mouse_state){0, 0, true};
    glfwSetCursorPos(g->window, g->mouse_state.x, g->mouse_state.y);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glEnable(GL_DEPTH_TEST);
    glClearColor(89.0/255.0, 219.0/255.0, 1.0, 1.0);

    shader_block_init(&g->shader_block);
    shader_selector_init(&g->shader_selector);
}

void game_process_input(game *g) 
{
    if (glfwWindowShouldClose(g->window)) {
        g->running = false;
        return;
    }
    if (glfwGetKey(g->window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetInputMode(g->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        g->mouse_state.in_game = false;
    }
    if (glfwGetMouseButton(g->window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS) {
        glfwSetInputMode(g->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        g->mouse_state.in_game = true;
        glfwGetCursorPos(g->window, &g->mouse_state.x, &g->mouse_state.y);
    }
    if (!g->mouse_state.in_game) return;
    float speed = 0.2;
    if (glfwGetKey(g->window, GLFW_KEY_W) == GLFW_PRESS) {
        camera_move_forward(&g->camera, speed);
    }
    if (glfwGetKey(g->window, GLFW_KEY_S) == GLFW_PRESS) {
        camera_move_backward(&g->camera, speed);
    }
    if (glfwGetKey(g->window, GLFW_KEY_A) == GLFW_PRESS) {
        camera_move_left(&g->camera, speed);
    }
    if (glfwGetKey(g->window, GLFW_KEY_D) == GLFW_PRESS) {
        camera_move_right(&g->camera, speed);
    }
    if (glfwGetKey(g->window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        camera_move_up(&g->camera, speed);
    }
    if (glfwGetKey(g->window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        camera_move_down(&g->camera, speed);
    }
    double last_m_x = g->mouse_state.x, last_m_y = g->mouse_state.y;
    glfwGetCursorPos(g->window, &g->mouse_state.x, &g->mouse_state.y);
    float x_offset = (g->mouse_state.x - last_m_x);
    float y_offset = -(g->mouse_state.y - last_m_y);
    float sensitivity = 0.25;
    x_offset *= sensitivity;
    y_offset *= sensitivity;
    float pitch = g->camera.pitch, yaw = g->camera.yaw;
    pitch += y_offset;
    yaw += x_offset;
    camera_set_yaw_pitch(&g->camera, yaw, pitch);
    //printf("\r(%f, %f, %f) (%f, %f)", g->camera.pos.x, g->camera.pos.y, g->camera.pos.z, g->camera.yaw, g->camera.pitch);
    camera_update_view_matrix(&g->camera);
}

void game_update(game *g)
{

}

void game_render(game *g, float alpha) 
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    shader_block_use(&g->shader_block);
    world_render(&g->world, &g->camera, &g->shader_block);        
    model_render(&g->model, &(vec3){30, 61, 30}, &g->camera, &g->shader_block, g->current_time);

    block_type b;
    ubpos pos = world_ray_cast(&g->world, &g->camera, 6, &b);
    shader_selector_use(&g->shader_selector);
    if (b != BLOCK_AIR) {
        selector_render(&g->selector, &(vec3){pos.x, pos.y, pos.z}, &g->camera, &g->shader_selector);
    }
    glDisable(GL_DEPTH_TEST);
    selector_render_cursor(&g->selector, &g->shader_selector);
}

void game_run(game *g)
{
    while (g->running)
    {
        glfwPollEvents();

        double new_time = glfwGetTime();
        double delta = new_time - g->current_time;
        g->current_time = new_time;
        g->accumulator += delta;

        game_process_input(g);
        while (g->accumulator >= TIME_PER_TICK) {
            game_update(g);
            g->accumulator -= TIME_PER_TICK;
        }

        float alpha = TIME_PER_TICK / g->accumulator;

        game_render(g, alpha);
        glfwSwapBuffers(g->window);
    }
}

void game_end(game *g)
{
    g->running = false;
}