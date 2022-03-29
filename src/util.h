#pragma once

#include "glad.h"
#include <stdnoreturn.h>
#include <stddef.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

void          compile_shader(GLuint shader);
void          link_program(GLuint program);
GLuint        create_linked_program(const char *vertex_shader_src, const char *fragment_shader_src);
GLuint        create_texture(unsigned char *buffer, size_t buffer_len, GLint mag_filter, GLint *max_level);
noreturn void panic_(const char *s, ...);
void          check_gl_errors_(const char *file, int line);

#define stringify(x) #x
#define check_gl_errors() check_gl_errors_(__FILE__, __LINE__)
#define panic(s, ...) panic_("panic at %s:%d: " s, __FILE__, __LINE__, __VA_ARGS__)
#define unreachable() panic("%s", "unreachable")