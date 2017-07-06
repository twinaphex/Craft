#ifndef _map_h_
#define _map_h_

#include <stdint.h>

#define EMPTY_ENTRY(entry) ((entry)->value == 0L)

#define MAP_FOR_EACH(map, ex, ey, ez, ew) \
{ \
   unsigned int i; \
   int ex, ey, ez, ew; \
    for (i = 0; i <= map->mask; i++) { \
        MapEntry *entry = map->data + i; \
        if (EMPTY_ENTRY(entry)) \
            continue; \
        ex = entry->e.x + map->dx; \
        ey = entry->e.y + map->dy; \
        ez = entry->e.z + map->dz; \
        ew = entry->e.w;

#define END_MAP_FOR_EACH } }

typedef union {
    uint64_t value;
    struct {
        uint16_t x;
        uint16_t y;
        uint16_t z;
        int16_t w;
    } e;
} MapEntry;

typedef struct {
    int dx;
    int dy;
    int dz;
    unsigned int mask;
    unsigned int size;
    MapEntry *data;
} Map;

void map_alloc(Map *map, int dx, int dy, int dz, int mask);
void map_free(Map *map);
void map_copy(Map *dst, Map *src);
void map_grow(Map *map);
int map_set(Map *map, int x, int y, int z, int w);
int map_get(Map *map, int x, int y, int z);

#endif
