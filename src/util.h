#ifndef _util_h_
#define _util_h_

#ifdef __LIBRETRO__
#include <glsm/glsmsym.h>
#else
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#endif
#include "config.h"

#define PI 3.14159265359
#define DEGREES(radians) ((radians) * 180 / PI)
#define RADIANS(degrees) ((degrees) * PI / 180)
#define ABS(x) ((x) < 0 ? (-(x)) : (x))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define SIGN(x) (((x) > 0) - ((x) < 0))

#if DEBUG
    #define LOG(...) printf(__VA_ARGS__)
#else
    #define LOG(...)
#endif

#endif
