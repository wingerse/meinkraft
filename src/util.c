#include "util.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "stb_image.h"

void compile_shader(GLuint shader)
{
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success)
        return;

    GLint info_len;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_len);
    char *info = malloc(info_len);
    glGetShaderInfoLog(shader, info_len, NULL, info);
    panic("%s", info);
}

void link_program(GLuint program)
{
    glLinkProgram(program);
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success)
        return;

    GLint info_len;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &info_len);
    char *info = malloc(info_len);
    glGetProgramInfoLog(program, info_len, NULL, info);
    panic("%s", info);
}

GLuint create_linked_program(const char *vertex_shader_src, const char *fragment_shader_src)
{
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_src, NULL);
    compile_shader(vertex_shader);

    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_src, NULL);
    compile_shader(fragment_shader);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    
    link_program(program);

    glDeleteShader(vertex_shader);
    glDetachShader(program, vertex_shader);
    glDeleteShader(fragment_shader);
    glDetachShader(program, fragment_shader);
    return program;
}

GLuint create_texture(unsigned char *buffer, size_t buffer_len, GLint mag_filter, GLint *max_level)
{
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mag_filter);
    if (max_level != NULL) glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, *max_level);

    int width, height, nChannels;
    unsigned char *texture_data = stbi_load_from_memory(buffer, buffer_len, &width, &height, &nChannels, STBI_rgb_alpha);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(texture_data);
    return tex;
}

noreturn void panic_(const char *s, ...)
{
    va_list va;
    va_start(va, s);
    vfprintf(stderr, s, va);
    va_end(va);
    fflush(stdout);
    exit(EXIT_FAILURE);
}

void check_gl_errors_(const char *file, int line)
{
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        const char *err_str;
        switch (err) {
        case GL_INVALID_ENUM:                  err_str = stringify(GL_INVALID_ENUM); break;
        case GL_INVALID_VALUE:                 err_str = stringify(GL_INVALID_VALUE); break;
        case GL_INVALID_OPERATION:             err_str = stringify(GL_INVALID_OPERATION); break;
        case GL_OUT_OF_MEMORY:                 err_str = stringify(GL_OUT_OF_MEMORY); break;
        case GL_INVALID_FRAMEBUFFER_OPERATION: err_str = stringify(GL_INVALID_FRAMEBUFFER_OPERATION); break;
        default: err_str = "Unknown error"; break;
        }
        panic_("OpenGL error at %s:%d: %s\n", file, line, err_str);
    }
}