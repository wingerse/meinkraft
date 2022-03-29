#include "glad.h"
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include "util.h"
#include "game.h"
#include "stb_image.h"

void error_callback(int error, const char *desc)
{
    panic("%s", desc);
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

int main() 
{
    glfwSetErrorCallback(error_callback);
    glfwInit();
    stbi_set_flip_vertically_on_load(true);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow *window = glfwCreateWindow(1024, 800, "Meincraft", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        panic("%s", "GLAD initialization failure");

    glViewport(0, 0, 1024, 800);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    game game;
    game_init(&game, window);
    game_run(&game);

    glfwTerminate();
    return 0;
}
