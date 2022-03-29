#include "shader_selector.h"
#include "../glad.h"
#include "../util.h"

static const char *vertex = "\
#version 330 core\n\
\
layout (location = 0) in vec3 pos;\
\
uniform mat4 mvp_matrix;\
\
void main()\
{\
    gl_Position = mvp_matrix * vec4(pos, 1.0);\
}";

static const char *fragment = "\
#version 330 core\n\
\
out vec4 FragColor;\
\
void main()\
{\
    FragColor = vec4(0.0, 0.0, 0.0, 1.0);\
}";

void shader_selector_init(shader_selector *s)
{
    s->program = create_linked_program(vertex, fragment);
    s->mvp_matrix_location = glGetUniformLocation(s->program, "mvp_matrix");
}

void shader_selector_use(shader_selector *s)
{
    glUseProgram(s->program);
}

void shader_selector_set_up_attributes(void)
{
    GLsizei stride = sizeof(shader_selector_vertex);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(shader_selector_vertex, pos_x));
    glEnableVertexAttribArray(0);
}