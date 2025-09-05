
#include <grrlib.h>
typedef struct {
    float x, y;
} Vec2D;

typedef struct {
    u8 r;
    u8 b;
    u8 g;
} Color;

typedef struct {
    float h;
    float s;
    float v;
    bool sChecked;
    bool vChecked;
} HSV;

typedef struct {
    float r;
    float g;
    float b;
    float a;
} ColorAlpha;