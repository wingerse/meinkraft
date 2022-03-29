#include "shader_block.h"
#include "../util.h"

static const char *vertex = "\
#version 330 core\n\
\
layout (location = 0) in vec3 pos;\
layout (location = 1) in vec2 tex_coord;\
layout (location = 2) in vec3 normal;\
layout (location = 3) in float brightness;\
\
out vec2 extern_tex_coord;\
out vec3 extern_normal;\
out float extern_brightness;\
\
uniform mat4 mvp_matrix;\
\
void main()\
{\
    gl_Position = mvp_matrix * vec4(pos, 1.0);\
    extern_tex_coord = tex_coord;\
    extern_normal = normal;\
    extern_brightness = brightness;\
}";

static const char *fragment = "\
#version 330 core\n\
\
in vec2 extern_tex_coord;\
in vec3 extern_normal;\
in float extern_brightness;\
\
out vec4 FragColor;\
\
uniform sampler2D atlas;\
\
void main()\
{\
    vec4 pixel = texture(atlas, extern_tex_coord);\
    FragColor = vec4(vec3(pixel) * extern_brightness, pixel.a);\
}";

void shader_block_init(shader_block *s)
{
    s->program = create_linked_program(vertex, fragment);
    s->mvp_matrix_location = glGetUniformLocation(s->program, "mvp_matrix");
}

void shader_block_use(shader_block *s)
{
    glUseProgram(s->program);
}

void shader_block_set_up_attributes(void)
{
    GLsizei stride = sizeof(shader_block_vertex);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(shader_block_vertex, pos_x));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(shader_block_vertex, uv_s));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(shader_block_vertex, normal_x));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(shader_block_vertex, brightness));
    glEnableVertexAttribArray(3);
}