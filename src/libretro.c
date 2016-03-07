#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
#include <glsm/glsm.h>
#endif

#include "libretro.h"

static struct retro_log_callback logging;

static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_environment_t environ_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;
static retro_log_printf_t log_cb;

static void fallback_log(enum retro_log_level level, const char *fmt, ...)
{
   (void)level;
   va_list va;
   va_start(va, fmt);
   vfprintf(stderr, fmt, va);
   va_end(va);
}

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
static bool fb_ready = false;

static void context_reset(void)
{
   printf("context_reset.\n");
   glsm_ctl(GLSM_CTL_STATE_CONTEXT_RESET, NULL);

   if (!glsm_ctl(GLSM_CTL_STATE_SETUP, NULL))
      return;

   fb_ready = true;
}

static void context_destroy(void)
{
}

static bool context_framebuffer_lock(void *data)
{
   if (fb_ready)
      return false;
   return true;
}
#endif

void retro_init(void)
{
}

void retro_deinit(void)
{
}

unsigned retro_api_version(void)
{
   return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
}

void retro_get_system_info(struct retro_system_info *info)
{
   memset(info, 0, sizeof(*info));
   info->library_name     = "Craft";
   info->library_version  = "v1";
   info->need_fullpath    = false;
   info->valid_extensions = NULL; /* Anything is fine, we don't care. */
}


void retro_get_system_av_info(struct retro_system_av_info *info)
{
   info->geometry.base_width  = 640;
   info->geometry.base_height = 480;
   info->geometry.max_width   = 1024;
   info->geometry.max_height  = 1024;
   info->geometry.aspect_ratio = 4.0 / 3.0;
   info->timing.fps            = 60.0;
   info->timing.sample_rate    = 48000.0;
}

static struct retro_rumble_interface rumble;

void retro_set_environment(retro_environment_t cb)
{
   bool no_content = true;

   environ_cb = cb;

   static const struct retro_variable vars[] = {
      { "craft_test", "Test; disabled|enabled" },
      { NULL, NULL },
   };

   cb(RETRO_ENVIRONMENT_SET_VARIABLES, (void*)vars);

   cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_content);

   if (cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging))
      log_cb = logging.log;
   else
      log_cb = fallback_log;
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
   audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
   audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
   input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
   input_state_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
   video_cb = cb;
}

void retro_reset(void)
{
}

static void check_variables(void)
{
   struct retro_variable var = {0};
   var.key = "craft_test";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      log_cb(RETRO_LOG_INFO, "Key -> Val: %s -> %s.\n", var.key, var.value);
   }
}

void retro_run(void)
{
   bool updated = false;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
      check_variables();

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
   glsm_ctl(GLSM_CTL_STATE_BIND, NULL);
#endif

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
   glsm_ctl(GLSM_CTL_STATE_UNBIND, NULL);
#endif
}

static void keyboard_cb(bool down, unsigned keycode,
      uint32_t character, uint16_t mod)
{
   log_cb(RETRO_LOG_INFO, "Down: %s, Code: %d, Char: %u, Mod: %u.\n",
         down ? "yes" : "no", keycode, character, mod);
}

bool retro_load_game(const struct retro_game_info *info)
{
   enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
   struct retro_keyboard_callback cb = { keyboard_cb };
   struct retro_input_descriptor desc[] = {
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right" },
      { 0 },
   };
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
   glsm_ctx_params_t params = {0};
#endif

   environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);

   if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
   {
      log_cb(RETRO_LOG_INFO, "XRGB8888 is not supported.\n");
      return false;
   }

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
   params.context_reset         = context_reset;
   params.context_destroy       = context_destroy;
   params.environ_cb            = environ_cb;
   params.stencil               = true;
   params.imm_vbo_draw          = NULL;
   params.imm_vbo_disable       = NULL;
   params.framebuffer_lock      = context_framebuffer_lock;

   if (!glsm_ctl(GLSM_CTL_STATE_CONTEXT_INIT, &params))
      return false;   
#endif

   environ_cb(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &cb);
   if (environ_cb(RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE, &rumble))
      log_cb(RETRO_LOG_INFO, "Rumble environment supported.\n");
   else
      log_cb(RETRO_LOG_INFO, "Rumble environment not supported.\n");

   check_variables();

   (void)info;
   return true;
}

void retro_unload_game(void)
{
}

unsigned retro_get_region(void)
{
   return RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num)
{
   return false;
}

size_t retro_serialize_size(void)
{
   return 0;
}

bool retro_serialize(void *data_, size_t size)
{
   return false;
}

bool retro_unserialize(const void *data_, size_t size)
{
   return false;
}

void *retro_get_memory_data(unsigned id)
{
   return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
   return 0;
}

void retro_cheat_reset(void)
{}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
   (void)index;
   (void)enabled;
   (void)code;
}

