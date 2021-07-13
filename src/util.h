#ifndef _util_h_
#define _util_h_

#include "config.h"
#include <stdio.h>

#define PI 3.14159265359
#define DEGREES(radians) ((radians) * 180 / PI)
#define RADIANS(degrees) ((degrees) * PI / 180)
#define ABS(x) ((x) < 0 ? (-(x)) : (x))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define SIGN(x) (((x) > 0) - ((x) < 0))

#ifdef __LIBRETRO__
#include "libretro.h"
extern retro_log_printf_t log_cb;
#define LOG(...) log_cb(RETRO_LOG_INFO, __VA_ARGS__)
#define LOG_ERROR(...) log_cb(RETRO_LOG_ERROR, __VA_ARGS__)
#else
#if DEBUG
    #define LOG(...) printf(__VA_ARGS__)
#else
    #define LOG(...)
#endif
#define LOG_ERROR(...) fprintf(stderr, __VA_ARGS__)
#endif

int main_load_graphics(void);

int main_unload_graphics(void);

int main_load_game(int argc, char **argv);

void main_unload_game(void);

int main_init(void);

void main_deinit(void);

int main_run(void);

#endif
