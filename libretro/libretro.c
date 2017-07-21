#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include <retro_miscellaneous.h>

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
#include <glsm/glsm.h>
#endif

#include "libretro.h"
#include "../src/util.h"

static struct retro_log_callback logging;

static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
retro_environment_t environ_cb;
static retro_input_poll_t input_poll_cb;
retro_input_state_t input_state_cb;
static retro_log_printf_t log_cb;

unsigned game_width  = 640;
unsigned game_height = 480;

static void fallback_log(enum retro_log_level level, const char *fmt, ...)
{
   va_list va;

   (void)level;

   va_start(va, fmt);
   vfprintf(stderr, fmt, va);
   va_end(va);
}

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
static bool fb_ready = false;
static bool init_program_now = false;

static void context_reset(void)
{
   printf("context_reset.\n");
   glsm_ctl(GLSM_CTL_STATE_CONTEXT_RESET, NULL);

   if (!glsm_ctl(GLSM_CTL_STATE_SETUP, NULL))
      return;

   fb_ready = true;
   init_program_now = true;
}

static void context_destroy(void)
{
   glsm_ctl(GLSM_CTL_STATE_CONTEXT_DESTROY, NULL);
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
   main_init();
}

void retro_deinit(void)
{
   main_deinit();
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
   #ifdef GIT_VERSION
       info->library_version = GIT_VERSION;
   #else   
      info->library_version = "1.0";
   #endif
   info->need_fullpath    = false;
   info->valid_extensions = NULL; /* Anything is fine, we don't care. */
}


void retro_get_system_av_info(struct retro_system_av_info *info)
{
   info->geometry.base_width  = game_width;
   info->geometry.base_height = game_height;
   info->geometry.max_width   = game_width;
   info->geometry.max_height  = game_height;
   info->geometry.aspect_ratio = 16.0 / 9.0;
   info->timing.fps            = 60.0;
   info->timing.sample_rate    = 48000.0;
}

static struct retro_rumble_interface rumble;

void retro_set_environment(retro_environment_t cb)
{
   bool no_content = true;
   static const struct retro_variable vars[] = {
      { "craft_resolution",
         "Resolution (restart); 640x480|320x200|640x400|960x600|1280x800|1600x1000|1920x1200|2240x1400|2560x1600|2880x1800|3200x2000|3520x2200|3840x2400|7680x4320|15360x8640|16000x9000|320x240|320x480|360x200|360x240|360x400|360x480|400x224|480x272|512x224|512x240|512x384|512x512|640x224|640x240|640x448|720x576|800x480|800x600|960x720|1024x768|1280x720|1366x768|1600x900|1920x1080|2048x2048|4096x4096" },
      { "craft_show_info_text",
         "Show info text; disabled|enabled" },
      { "craft_jumping_flash_mode",
         "Jumping Flash mode; disabled|enabled" },
      { "craft_field_of_view",
         "Field of view; 65|70|75|80|85|90|95|100|105|110|115|120|125|130|135|140|145|150" },
      { "craft_draw_distance",
         "Draw distance; 10|11|12|13|14|15|16|17|18|19|20|21|22|23|24|25|26|27|28|29|30|31|32|9|8|7|6|5|4|3|2|1" },
      { "craft_inverted_aim",
         "Inverted aim; disabled|enabled" },
      { "craft_analog_sensitivity",
         "Right analog sensitivity; 0.0150|0.0175|0.0200|0.0225|0.0250|0.0275|0.0300|0.0325|0.0350|0.0375|0.0400|0.0425|0.0450|0.0475|0.0500" },
      { "craft_deadzone_radius",
         "Analog deadzone size; 0.010|0.015|0.020|0.025|0.030|0.035|0.040|0.045|0.050|0.055|0.060|0.065|0.070|0.075|0.080|0.085|0.090|0.095|0.100|0.110|0.115|0.120|0.125|0.130|0.135|0.140|0.145|0.150|0.155|0.160|0.165|0.170|0.175|0.180|0.185|0.190|0.195|0.200" },
      { NULL, NULL },
   };

   environ_cb = cb;


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

extern unsigned SHOW_INFO_TEXT;

static void check_variables(bool first_time_startup)
{
   struct retro_variable var = {0};
   var.key = "craft_resolution";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value && 
         first_time_startup)
   {
      char *pch;
      char str[100];
      snprintf(str, sizeof(str), "%s", var.value);

      pch = strtok(str, "x");
      if (pch)
         game_width = strtoul(pch, NULL, 0);
      pch = strtok(NULL, "x");
      if (pch)
         game_height = strtoul(pch, NULL, 0);

      if (log_cb)
         log_cb(RETRO_LOG_INFO, "Got size: %u x %u.\n", game_width, game_height);
   }

   var.key = "craft_show_info_text";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "disabled"))
         SHOW_INFO_TEXT = 0;
      else if (!strcmp(var.value, "enabled"))
         SHOW_INFO_TEXT = 1;
   }

   var.key = "craft_jumping_flash_mode";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "disabled"))
         JUMPING_FLASH_MODE = 0;
      else if (!strcmp(var.value, "enabled"))
         JUMPING_FLASH_MODE  = 1;
   }

   var.key = "craft_field_of_view";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      FIELD_OF_VIEW = atoi(var.value);
   }

   var.key = "craft_draw_distance";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      RENDER_CHUNK_RADIUS = atoi(var.value);
   }

   var.key = "craft_inverted_aim";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "disabled"))
         INVERTED_AIM = 0;
      else if (!strcmp(var.value, "enabled"))
         INVERTED_AIM = 1;
   }

   var.key = "craft_analog_sensitivity";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      ANALOG_SENSITIVITY = atof(var.value);
   }

   var.key = "craft_deadzone_radius";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      DEADZONE_RADIUS = atof(var.value);
   }
}

static unsigned logic_frames        = 0;
static unsigned amount_frames       = 0;

extern void on_key(void);

void retro_run(void)
{
   static unsigned timestep = 0;
   static double libretro_on_key_delay = 0.0f;
   bool updated = false;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
      check_variables(false);

   if (!fb_ready)
   {
      video_cb(NULL, game_width, game_height, 0);
      return;
   }
   if (init_program_now)
   {
      main_load_game(0, NULL);
      init_program_now = false;
      video_cb(NULL, game_width, game_height, 0);
      return;
   }

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
   glsm_ctl(GLSM_CTL_STATE_BIND, NULL);
#endif

   input_poll_cb();

   if (libretro_on_key_delay > logic_frames) { }
   else
   {
      libretro_on_key_delay = 0;
      libretro_on_key_delay = logic_frames + (15);
      on_key();
   }

   if (main_run() != 1)
   {
      /* Do shutdown or something similar. */
   }

   timestep += 1;
   logic_frames++;

   if (timestep >= 60)
   {
      amount_frames++;
      timestep = 0;
   }

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
   glsm_ctl(GLSM_CTL_STATE_UNBIND, NULL);
#endif

   video_cb(RETRO_HW_FRAME_BUFFER_VALID, game_width, game_height, 0);
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
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
   glsm_ctx_params_t params = {0};
#endif

   if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
   {
      log_cb(RETRO_LOG_INFO, "XRGB8888 is not supported.\n");
      return false;
   }

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
   params.context_reset         = context_reset;
   params.context_destroy       = context_destroy;
   params.environ_cb            = environ_cb;
   params.stencil               = false;
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

   check_variables(true);

   (void)info;
   return true;
}

void retro_unload_game(void)
{
   main_unload_game();
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

void handle_mouse_input()
{
}

void glfwSetTime(double time)
{
   amount_frames += time;
}

double glfwGetTime(void)
{
   double val = amount_frames;
   return val;
}
