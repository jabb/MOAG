
#ifndef COMMON_H
#define COMMON_H


#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <enet/enet.h>
#include <netinet/in.h>

#define M_PI            3.14159

#define PORT            8080
#define MAX_CLIENTS     8
#define NUM_CHANNELS    2

#define MAX_BULLETS     256

#define LAND_WIDTH      800
#define LAND_HEIGHT     600

enum {LAND_CHUNK, TANK_CHUNK, BULLET_CHUNK, MSG_CHUNK, CRATE_CHUNK};

struct tank {
    int x, y, lastx, lasty;
    int angle, power;
    int spawntimer;
    int ladder;
    bool active;
    char name[16];
    char bullet;
    bool facingLeft;
    bool kleft, kright, kup, kdown, kfire;
};

struct bullet {
    int x, y;
    float fx, fy, vx, vy;
    char active;
    char type;
};

struct crate {
    int x, y;
    char type;
};

inline char get_land_at(char *land, int x, int y)
{
    if (x < 0 || x >= LAND_WIDTH || y < 0 || y >= LAND_HEIGHT)
        return -1;
    return land[y * LAND_WIDTH + x];
}

inline void set_land_at(char *land, int x, int y, char to)
{
    if (x < 0 || x >= LAND_WIDTH || y < 0 || y >= LAND_HEIGHT)
        return;
    land[y * LAND_WIDTH + x] = to;
}

static inline void write8(char *buf, size_t *pos, uint8_t val)
{
    *(char *)(&buf[*pos]) = val;
    (*pos) += 1;
}

static inline void write16(char *buf, size_t *pos, uint16_t val)
{
    *(uint16_t *)(&buf[*pos]) = htons(val);
    (*pos) += 2;
}

static inline void write32(char *buf, size_t *pos, uint32_t val)
{
    *(uint32_t *)(&buf[*pos]) = htonl(val);
    (*pos) += 4;
}

static inline uint8_t read8(char *buf, size_t *pos)
{
    uint8_t val = *(char *)(&buf[*pos]);
    (*pos) += 1;
    return val;
}

static inline uint16_t read16(char *buf, size_t *pos)
{
    uint16_t val = ntohs(*(uint16_t *)(&buf[*pos]));
    (*pos) += 2;
    return val;
}

static inline uint32_t read32(char *buf, size_t *pos)
{
    uint32_t val = ntohl(*(uint32_t *)(&buf[*pos]));
    (*pos) += 4;
    return val;
}

static inline void build_chunk(void *buf, size_t *pos)
{
    write8(buf, pos, 'M');
    write8(buf, pos, 'O');
    write8(buf, pos, 'A');
    write8(buf, pos, 'G');
}

static inline void build_land_chunk(void *buf, size_t *pos)
{
    build_chunk(buf, pos);
    write8(buf, pos, LAND_CHUNK);
}

int die(const char *fmt, ...);

#endif
