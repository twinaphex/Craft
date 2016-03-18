#ifndef _RENDERER_H
#define _RENDERER_H

#include <stdint.h>
#include <stddef.h>

#include <boolean.h>

#define MAX_NAME_LENGTH 32

typedef struct craft_info craft_info_t;

enum shader_type
{
   SHADER_VERTEX = 0,
   SHADER_FRAGMENT
};

enum draw_prim_type
{
   DRAW_PRIM_TRIANGLES = 0,
   DRAW_PRIM_LINES
};

typedef struct
{
   float x;
   float y;
   float z;
   float rx;
   float ry;
   float t;
} State;

typedef struct
{
   int id;
   char name[MAX_NAME_LENGTH];
   State state;
   State state1;
   State state2;
   uintptr_t buffer;
} Player;

typedef struct
{
   unsigned int fps;
   unsigned int frames;
   double since;
} FPS;

typedef struct
{
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
   GLuint program;
   GLuint position;
   GLuint normal;
   GLuint uv;
   GLuint matrix;
   GLuint sampler;
   GLuint camera;
   GLuint timer;
   GLuint extra1;
   GLuint extra2;
   GLuint extra3;
   GLuint extra4;
#endif
} Attrib;

struct craft_info
{
   Attrib block_attrib;
   Attrib line_attrib;
   Attrib text_attrib;
   Attrib sky_attrib;
   Attrib water_attrib;

   uintptr_t sky_buffer;
   uintptr_t program;
   uintptr_t texture;
   uintptr_t font;
   uintptr_t sky;
   uintptr_t sign;

   State *s;
   Player *me;
   double previous;
   double last_commit;
   double last_update;
   FPS fps;
};


typedef struct shader_program_info
{
   Attrib *attrib;

   struct
   {
      bool enable;
   } program;

   struct
   {
      bool enable;
      unsigned data;
   } linewidth;

   struct
   {
      bool enable;
      unsigned data;
   } sampler;

   struct
   {
      bool enable;
      unsigned data;
   } extra1;

   struct
   {
      bool enable;
      float data;
   } extra2;

   struct
   {
      bool enable;
      float data;
   } extra3;

   struct
   {
      bool enable;
      float data;
   } extra4;

   struct
   {
      bool enable;
      float data;
   } timer;

   struct
   {
      bool enable;
      float x;
      float y;
      float z;
   } camera;

   struct
   {
      bool enable;
      float *data;
   } matrix;
} shader_program_info_t;


uintptr_t make_shader(enum shader_type shader_type, const char *source);

void renderer_load_shaders(craft_info_t *info);

void renderer_upload_texture_data(const unsigned char *in_data, size_t in_size,
      uintptr_t *tex, unsigned num);

void renderer_set_viewport(int x, int y, int width, int height);

void renderer_preinit(void);

uintptr_t renderer_gen_buffer(size_t size, float *data);

void renderer_del_buffer(uintptr_t buffer);

void render_shader_program(struct shader_program_info *info);

void renderer_enable_color_logic_op(void);

void renderer_disable_color_logic_op(void);

void renderer_free_texture(uintptr_t *tex);

void renderer_clear_backbuffer(void);

void renderer_clear_depthbuffer(void);

void renderer_enable_blend(void);

void renderer_disable_blend(void);

uintptr_t renderer_gen_faces(int components, int faces, float *data);

void renderer_bind_array_buffer(Attrib *attrib, uintptr_t buffer,
      unsigned normal, unsigned uv);

void renderer_unbind_array_buffer(Attrib *attrib,
      unsigned normal, unsigned uv);

void renderer_modify_array_buffer(Attrib *attrib,
      unsigned attrib_size,
      unsigned normal, unsigned uv, unsigned mod);

void renderer_enable_polygon_offset_fill(void);

void renderer_disable_polygon_offset_fill(void);

void renderer_draw_triangle_arrays(enum draw_prim_type type, unsigned count);

void renderer_enable_scissor_test(void);

void renderer_disable_scissor_test(void);

void renderer_scissor(int x, int y, int width, int height);

void renderer_upload_image(int width, int height, unsigned char *data);

#endif
