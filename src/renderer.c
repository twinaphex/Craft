#include <stdlib.h>

#include <glsm/glsmsym.h>

#include "renderer.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

enum shader_program_type
{
   SHADER_PROGRAM_NONE = 0,
   SHADER_PROGRAM_BLOCK,
   SHADER_PROGRAM_LINE,
   SHADER_PROGRAM_TEXT,
   SHADER_PROGRAM_SKY,
   SHADER_PROGRAM_WATER
};

#if defined(HAVE_OPENGLES)
#define GLSL_VERSION "100"
#else
#define GLSL_VERSION "120"
#endif

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
static const char *text_fragment_shader[] = {
   "#version " GLSL_VERSION "\n"
#if defined(HAVE_OPENGLES)
    "precision lowp float; \n"
#endif
   "uniform sampler2D sampler;\n",
   "uniform bool is_sign;\n",
   "varying vec2 fragment_uv;\n",
   "void main() {\n",
   "  vec4 color = texture2D(sampler, fragment_uv);\n"
   "  if (is_sign) {\n",
   "    if (color == vec4(1.0)) {\n",
   "      discard;\n",
   "    }\n",
   "  }\n",
   "  else {\n",
   "    color.a = max(color.a, 0.4);\n",
   "  }\n",
   "  gl_FragColor = color;\n",
   "}",
};

static const char *text_vertex_shader[] = {
   "#version " GLSL_VERSION "\n"
   "uniform mat4 matrix;\n",
   "attribute vec4 position;\n",
   "attribute vec2 uv;\n",
   "varying vec2 fragment_uv;\n",
   "void main() {\n",
   "  gl_Position = matrix * position;\n",
   "  fragment_uv = uv;\n",
   "}\n",
};

static const char *line_vertex_shader[] = {
   "#version " GLSL_VERSION "\n"
   "uniform mat4 matrix;\n",
   "attribute vec4 position;\n",
   "void main() {\n",
   "  gl_Position = matrix * position;\n",
   "}\n",
};

static const char *line_fragment_shader[] = {
   "#version " GLSL_VERSION "\n"
   "void main() {\n",
   "  gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);\n",
   "}\n",
};

static const char *sky_fragment_shader[] = {
   "#version " GLSL_VERSION "\n"
#if defined(HAVE_OPENGLES)
    "precision lowp float; \n"
#endif
   "uniform sampler2D sampler;\n",
   "uniform float timer;\n",
   "varying vec2 fragment_uv;\n",
   "void main() {\n",
   "  vec2 uv = vec2(timer, fragment_uv.t);\n",
   "  gl_FragColor = texture2D(sampler, uv);\n",
   "}\n",
};

static const char *sky_vertex_shader[] = {
   "#version " GLSL_VERSION "\n"
   "uniform mat4 matrix;\n",
   "attribute vec4 position;\n",
   "attribute vec3 normal;\n",
   "attribute vec2 uv;\n",
   "varying vec2 fragment_uv;\n",
   "void main() {\n",
   "  gl_Position = matrix * position;\n"
   "  fragment_uv = uv;\n",
   "}\n",
};

static const char *water_vertex_shader[] = {
   "#version " GLSL_VERSION "\n"
   "uniform mat4 matrix;\n",
   "attribute vec4 position;\n",
   "attribute vec3 normal;\n",
   "attribute vec2 uv;\n",
   "varying vec4 point;\n",
   "void main() {\n",
   "  gl_Position = matrix * position;\n"
   "  point = position;\n",
   "}\n",
};

static const char *water_fragment_shader[] = {
   "#version " GLSL_VERSION "\n"
#if defined(HAVE_OPENGLES)
    "precision lowp float; \n"
#endif
   "uniform sampler2D sky_sampler;\n",
   "uniform float timer;\n",
   "uniform float daylight;\n",
   "uniform float fog_distance;\n",
   "uniform vec3 camera;\n",
   "varying vec4 point;\n",
   "const float pi = 3.14159265;\n",
   "void main() {\n",
   "  vec3 color = vec3(0.00, 0.33, 0.58);\n",
   "  vec3 ambient = vec3(daylight * 0.6 + 0.2);\n",
   "  color = min(color * ambient, vec3(1.0));\n",
   "  float camera_distance = distance(camera, vec3(point));\n",
   "  float fog_factor = pow(clamp(camera_distance / fog_distance, 0.0, 1.0), 4.0);\n",
   "  float dy = point.y - camera.y;\n",
   "  float dx = distance(point.xz, camera.xz);\n",
   "  float fog_height = (atan(dy, dx) + pi / 2.0) / pi;\n",
   "  vec3 sky_color = vec3(texture2D(sky_sampler, vec2(timer, fog_height)));\n",
   "  color = mix(color, sky_color, fog_factor);\n",
   "  gl_FragColor = vec4(color, 0.7);\n",
   "}\n",
};

static const char *block_fragment_shader[] = {
    "#version " GLSL_VERSION "\n"
#if defined(HAVE_OPENGLES)
    "precision lowp float; \n"
#endif
    "uniform sampler2D sampler;\n",
    "uniform sampler2D sky_sampler;\n",
    "uniform float timer;\n",
    "uniform float daylight;\n",
    "uniform int ortho;\n",
    "varying vec2 fragment_uv;\n",
    "varying float fragment_ao;\n",
    "varying float fragment_light;\n",
    "varying float fog_factor;\n",
    "varying float fog_height;\n",
    "varying float diffuse;\n",
    "const float pi = 3.14159265;\n",
    "void main() {\n",
    "  vec3 color = vec3(texture2D(sampler, fragment_uv));\n",
    "  if (color == vec3(1.0, 0.0, 1.0)) {\n",
    "    discard;\n",
    "  }\n",
    "  bool cloud = color == vec3(1.0, 1.0, 1.0);\n",
    "  if (cloud && bool(ortho)) {\n",
    "    discard;\n",
    "  }\n",
    "  float df = cloud ? 1.0 - diffuse * 0.2 : diffuse;\n",
    "  float ao = cloud ? 1.0 - (1.0 - fragment_ao) * 0.2 : fragment_ao;\n",
    "  ao = min(1.0, ao + fragment_light);\n",
    "  df = min(1.0, df + fragment_light);\n",
    "  float value = min(1.0, daylight + fragment_light);\n",
    "  vec3 light_color = vec3(value * 0.3 + 0.2);\n",
    "  vec3 ambient = vec3(value * 0.3 + 0.2) + \n"
    "      vec3(sin(pi*daylight)/2.0, sin(pi*daylight)/4.0, 0.0);\n",
    "  vec3 light = ambient + light_color * df;\n",
    "  color = clamp(color * light * ao, vec3(0.0), vec3(1.0));\n",
    "  vec3 sky_color = vec3(texture2D(sky_sampler, vec2(timer, fog_height)));\n",
    "  color = mix(color, sky_color, fog_factor);\n",
    "  gl_FragColor = vec4(color, 1.0);\n",
    "}\n",
};

static const char *block_vertex_shader[] = {
   "#version " GLSL_VERSION "\n"
   "uniform mat4 matrix;\n",
   "uniform vec3 camera;\n",
   "uniform float fog_distance;\n",
   "uniform int ortho;\n",
   "attribute vec4 position;\n",
   "attribute vec3 normal;\n",
   "attribute vec4 uv;\n",
   "varying vec2 fragment_uv;\n",
   "varying float fragment_ao;\n",
   "varying float fragment_light;\n",
   "varying float fog_factor;\n",
   "varying float fog_height;\n",
   "varying float diffuse;\n",
   "const float pi = 3.14159265;\n",
   "const vec3 light_direction = normalize(vec3(-1.0, 1.0, -1.0));\n",
   "void main() {\n",
   "  gl_Position = matrix * position;\n",
   "  fragment_uv = uv.xy;\n",
   "  fragment_ao = 0.3 + (1.0 - uv.z) * 0.7;\n",
   "  fragment_light = uv.w;\n",
   "  diffuse = max(0.0, dot(normal, light_direction));\n",
   "  if (bool(ortho)) {\n",
   "    fog_factor = 0.0;\n",
   "    fog_height = 0.0;\n",
   "  }\n",
   "  else {\n",
   "    float camera_distance = distance(camera, vec3(position));\n",
   "    fog_factor = pow(clamp(camera_distance / fog_distance, 0.0, 1.0), 4.0);\n",
   "    float dy = position.y - camera.y;\n",
   "    float dx = distance(position.xz, camera.xz);\n",
   "    fog_height = (atan(dy, dx) + pi / 2.0) / pi;\n",
   "  }\n",
   "}\n",
};
#endif

static void renderer_load_shader(craft_info_t *info, size_t len, size_t len2,
      const char **string, const char **string2)
{
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
   GLuint vert, frag;

   info->program               = glCreateProgram();
   vert                 = glCreateShader(GL_VERTEX_SHADER);
   frag                 = glCreateShader(GL_FRAGMENT_SHADER);

   glShaderSource(vert, (GLsizei)len,  (const GLchar**)string, 0);
   glShaderSource(frag, (GLsizei)len2, (const GLchar**)string2, 0);
   glCompileShader(vert);
   glCompileShader(frag);

   glAttachShader(info->program, vert);
   glAttachShader(info->program, frag);
   glLinkProgram(info->program);
   glDeleteShader(vert);
   glDeleteShader(frag);
#endif
}

void renderer_upload_texture_data(const unsigned char *in_data, size_t in_size,
      uintptr_t *tex, unsigned num)
{
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
   GLenum texture_num   = 0;
   GLenum linear_filter = GL_NEAREST;

   switch (num)
   {
      case 0:
         texture_num = GL_TEXTURE0;
         break;
      case 1:
         texture_num = GL_TEXTURE1;
         break;
      case 2:
         texture_num = GL_TEXTURE2;
         break;
      case 3:
         texture_num = GL_TEXTURE3;
         break;
   }

   glGenTextures(1, (GLuint*)tex);
   glActiveTexture(texture_num);
   glBindTexture(GL_TEXTURE_2D, (GLuint)*tex);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, linear_filter);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, linear_filter);
#endif
}

static void renderer_load_shader_type(craft_info_t *info, enum shader_program_type type)
{
   switch (type)
   {
      case SHADER_PROGRAM_BLOCK:
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
         renderer_load_shader(info, ARRAY_SIZE(block_vertex_shader), ARRAY_SIZE(block_fragment_shader),
               block_vertex_shader, block_fragment_shader);

         info->block_attrib.program  = info->program;
         info->block_attrib.position = glGetAttribLocation(info->program, "position");
         info->block_attrib.normal   = glGetAttribLocation(info->program, "normal");
         info->block_attrib.uv       = glGetAttribLocation(info->program, "uv");
         info->block_attrib.matrix   = glGetUniformLocation(info->program, "matrix");
         info->block_attrib.sampler  = glGetUniformLocation(info->program, "sampler");
         info->block_attrib.extra1   = glGetUniformLocation(info->program, "sky_sampler");
         info->block_attrib.extra2   = glGetUniformLocation(info->program, "daylight");
         info->block_attrib.extra3   = glGetUniformLocation(info->program, "fog_distance");
         info->block_attrib.extra4   = glGetUniformLocation(info->program, "ortho");
         info->block_attrib.camera   = glGetUniformLocation(info->program, "camera");
         info->block_attrib.timer    = glGetUniformLocation(info->program, "timer");
#endif
         break;
      case SHADER_PROGRAM_LINE:
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
         renderer_load_shader(info, ARRAY_SIZE(line_vertex_shader), ARRAY_SIZE(line_fragment_shader),
               line_vertex_shader, line_fragment_shader);

         info->line_attrib.program  = info->program;
         info->line_attrib.position = glGetAttribLocation(info->program, "position");
         info->line_attrib.matrix   = glGetUniformLocation(info->program, "matrix");
#endif
         break;
      case SHADER_PROGRAM_TEXT:
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
         renderer_load_shader(info, ARRAY_SIZE(text_vertex_shader), ARRAY_SIZE(text_fragment_shader),
               text_vertex_shader, text_fragment_shader);

         info->text_attrib.program  = info->program;
         info->text_attrib.position = glGetAttribLocation(info->program, "position");
         info->text_attrib.uv       = glGetAttribLocation(info->program, "uv");
         info->text_attrib.matrix   = glGetUniformLocation(info->program, "matrix");
         info->text_attrib.sampler  = glGetUniformLocation(info->program, "sampler");
         info->text_attrib.extra1   = glGetUniformLocation(info->program, "is_sign");
#endif
         break;
      case SHADER_PROGRAM_SKY:
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
         renderer_load_shader(info, ARRAY_SIZE(sky_vertex_shader), ARRAY_SIZE(sky_fragment_shader),
               sky_vertex_shader, sky_fragment_shader);

         info->sky_attrib.program  = info->program;
         info->sky_attrib.position = glGetAttribLocation(info->program, "position");
         info->sky_attrib.normal   = glGetAttribLocation(info->program, "normal");
         info->sky_attrib.uv       = glGetAttribLocation(info->program, "uv");
         info->sky_attrib.matrix   = glGetUniformLocation(info->program, "matrix");
         info->sky_attrib.sampler  = glGetUniformLocation(info->program, "sampler");
         info->sky_attrib.timer    = glGetUniformLocation(info->program, "timer");
#endif
         break;
      case SHADER_PROGRAM_WATER:
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
         renderer_load_shader(info, ARRAY_SIZE(water_vertex_shader), ARRAY_SIZE(water_fragment_shader),
               water_vertex_shader, water_fragment_shader);
         info->water_attrib.program      = info->program;
         info->water_attrib.position     = glGetAttribLocation(info->program, "position");
         info->water_attrib.normal       = glGetAttribLocation(info->program, "normal");
         info->water_attrib.uv           = glGetAttribLocation(info->program, "uv");
         info->water_attrib.matrix       = glGetUniformLocation(info->program, "matrix");
         info->water_attrib.extra1       = glGetUniformLocation(info->program, "sky_sampler");
         info->water_attrib.extra2       = glGetUniformLocation(info->program, "daylight");
         info->water_attrib.extra3       = glGetUniformLocation(info->program, "fog_distance");
         info->water_attrib.extra4       = glGetUniformLocation(info->program, "ortho");
         info->water_attrib.camera       = glGetUniformLocation(info->program, "camera");
         info->water_attrib.timer        = glGetUniformLocation(info->program, "timer");
#endif
         break;
      case SHADER_PROGRAM_NONE:
         break;
   }
}

void renderer_load_shaders(craft_info_t *info)
{
   renderer_load_shader_type(info, SHADER_PROGRAM_BLOCK);
   renderer_load_shader_type(info, SHADER_PROGRAM_LINE);
   renderer_load_shader_type(info, SHADER_PROGRAM_TEXT);
   renderer_load_shader_type(info, SHADER_PROGRAM_SKY);
   renderer_load_shader_type(info, SHADER_PROGRAM_WATER);
}

void renderer_preinit(void)
{
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
   glEnable(GL_CULL_FACE);
   glEnable(GL_DEPTH_TEST);
#endif
#if defined(HAVE_OPENGL)
   glLogicOp(GL_INVERT);
#endif
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
   glClearColor(0, 0, 0, 1);
#endif
}

void renderer_set_viewport(int x, int y, int width, int height)
{
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
   glViewport(x, y, width, height);
#endif
}

void renderer_del_buffer(uintptr_t buffer)
{
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
    glDeleteBuffers(1, (GLuint*)&buffer);
#endif
}

uintptr_t renderer_gen_buffer(size_t size, float *data)
{
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, (GLsizei)size, data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return buffer;
#endif
}

void render_shader_program(struct shader_program_info *info)
{
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
   if (info->program.enable)
      glUseProgram(info->attrib->program);

   if (info->linewidth.enable)
      glLineWidth(info->linewidth.data);

   if (info->matrix.enable)
      glUniformMatrix4fv(info->attrib->matrix, 1, GL_FALSE, info->matrix.data);

   if (info->camera.enable)
      glUniform3f(info->attrib->camera, info->camera.x, info->camera.y, info->camera.z);

   if (info->sampler.enable)
      glUniform1i(info->attrib->sampler, info->sampler.data);

   if (info->extra1.enable)
      glUniform1i(info->attrib->extra1,   info->extra1.data);

   if (info->extra2.enable)
      glUniform1f(info->attrib->extra2,   info->extra2.data);

   if (info->extra3.enable)
      glUniform1f(info->attrib->extra3,   info->extra3.data);

   if (info->extra4.enable)
      glUniform1i(info->attrib->extra4,   info->extra4.data);

   if (info->timer.enable)
      glUniform1f(info->attrib->timer,    info->timer.data);
#endif
}

void renderer_enable_color_logic_op(void)
{
#ifndef HAVE_OPENGLES
   glEnable(GL_COLOR_LOGIC_OP);
#endif
}

void renderer_disable_color_logic_op(void)
{
#ifndef HAVE_OPENGLES
   glDisable(GL_COLOR_LOGIC_OP);
#endif
}

void renderer_free_texture(uintptr_t *tex)
{
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
   glDeleteTextures(1, (const GLuint*)tex);
#endif
}

uintptr_t renderer_gen_faces(int components, int faces, float *data)
{
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
    GLuint buffer = (GLuint)renderer_gen_buffer(
        sizeof(GLfloat) * 6 * components * faces, data);
    free(data);
    return buffer;
#endif
}

void renderer_clear_backbuffer(void)
{
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
   glClear(GL_COLOR_BUFFER_BIT);
#endif
}

void renderer_clear_depthbuffer(void)
{
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
   glClear(GL_DEPTH_BUFFER_BIT);
#endif
}

void renderer_enable_blend(void)
{
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
}

void renderer_disable_blend(void)
{
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
   glDisable(GL_BLEND);
#endif
}

void renderer_bind_array_buffer(Attrib *attrib, uintptr_t buffer,
      unsigned normal, unsigned uv)
{
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
   glBindBuffer(GL_ARRAY_BUFFER, (GLuint)buffer);
   if (attrib->position != -1)
      glEnableVertexAttribArray(attrib->position);
   if (normal && attrib->normal != -1)
      glEnableVertexAttribArray(attrib->normal);
   if (uv && attrib->uv != -1)
      glEnableVertexAttribArray(attrib->uv);
#endif
}

void renderer_unbind_array_buffer(Attrib *attrib,
      unsigned normal, unsigned uv)
{
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
   if (attrib->position != -1)
      glDisableVertexAttribArray(attrib->position);
   if (normal && attrib->normal != -1)
      glDisableVertexAttribArray(attrib->normal);
   if (uv && attrib->uv != -1)
      glDisableVertexAttribArray(attrib->uv);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
#endif
}

void renderer_modify_array_buffer(Attrib *attrib,
      unsigned attrib_size,
      unsigned normal, unsigned uv, unsigned mod)
{
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
   if (attrib->position != -1)
      glVertexAttribPointer(attrib->position, attrib_size, GL_FLOAT, GL_FALSE,
            sizeof(GLfloat) * mod, 0);

   if (normal)
   {
      if (attrib->normal != -1)
         glVertexAttribPointer(attrib->normal, 3, GL_FLOAT, GL_FALSE,
               sizeof(GLfloat) * mod, (GLvoid *)(sizeof(GLfloat) * 3));
   }
   if (uv)
   {
      if (normal)
      {
         if (attrib->uv != -1)
            glVertexAttribPointer(attrib->uv, 4, GL_FLOAT, GL_FALSE,
                  sizeof(GLfloat) * mod, (GLvoid *)(sizeof(GLfloat) * 6));
      }
      else
      {
         if (attrib->uv != -1)
            glVertexAttribPointer(attrib->uv, 2, GL_FLOAT, GL_FALSE,
                  sizeof(GLfloat) * mod, (GLvoid *)(sizeof(GLfloat) * attrib_size));
      }
   }
#endif
}

void renderer_enable_polygon_offset_fill(void)
{
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-8, -1024);
#endif
}

void renderer_disable_polygon_offset_fill(void)
{
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
    glDisable(GL_POLYGON_OFFSET_FILL);
#endif
}

void renderer_draw_triangle_arrays(enum draw_prim_type type, unsigned count)
{
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
   GLenum gl_prim_type;

   switch (type)
   {
      case DRAW_PRIM_TRIANGLES:
         gl_prim_type = GL_TRIANGLES;
         break;
      case DRAW_PRIM_LINES:
         gl_prim_type = GL_LINES;
         break;
   }
   glDrawArrays(gl_prim_type, 0, count);
#endif
}

void renderer_enable_scissor_test(void)
{
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
   glEnable(GL_SCISSOR_TEST);
#endif
}

void renderer_scissor(int x, int y, int width, int height)
{
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
   glScissor(x, y, width, height);
#endif
}

void renderer_disable_scissor_test(void)
{
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
   glDisable(GL_SCISSOR_TEST);
#endif
}

void renderer_upload_image(int width, int height, unsigned char *data)
{
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
         GL_UNSIGNED_BYTE, data);
#endif
}
