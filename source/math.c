#include <math.h>
#include <stdlib.h>
#include <float.h>

#include "math.h"
#include "objects.h"
#include "game.h"
#include "player.h"
#include "main.h"
#include "easing.h"
#include "font_stuff.h"
#include <stdio.h>

#include "../libraries/color.h"

Vec2D normalize(Vec2D v) {
    float len = sqrtf(v.x*v.x + v.y*v.y);
    return (Vec2D){ v.x / len, v.y / len };
}

float dot_vec(Vec2D a, Vec2D b) {
    return a.x * b.x + a.y * b.y;
}

Vec2D perpendicular(Vec2D v) {
    return (Vec2D){ -v.y, v.x };
}

float positive_fmod(float n, float divisor) {
    float value = fmod(n, divisor);
    return value + (value < 0 ? divisor : 0);
}

Color color_lerp(Color color1, Color color2, float fraction) {
    u8 r1 = color1.r;
    u8 r2 = color2.r;
    u8 g1 = color1.g;
    u8 g2 = color2.g;
    u8 b1 = color1.b;
    u8 b2 = color2.b;

    Color returned;
    returned.r = (r2 - r1) * fraction + r1;
    returned.g = (g2 - g1) * fraction + g1;
    returned.b = (b2 - b1) * fraction + b1;

    return returned;
}

Vec2D rotate(Vec2D point, float angle, Vec2D origin) {
    float s = sinf(angle);
    float c = cosf(angle);
    point.x -= origin.x;
    point.y -= origin.y;
    Vec2D rotated = {
        point.x * c - point.y * s + origin.x,
        point.x * s + point.y * c + origin.y
    };
    return rotated;
}

float maxf(float a, float b) {
    return a > b ? a : b;
}

float approachf(float current, float target, float speed, float smoothing) {
    float diff = target - current;
    float step = diff * smoothing; // smoothing in [0,1], e.g. 0.1 for gentle, 0.5 for fast
    if (fabsf(diff) < speed)
        return target;
    return current + step + (diff > 0 ? speed : -speed);
}

// This one only works for drawing
void rotate_point_around_center_gfx(
    float x, float y,               // anchor position on screen
    float offset_x, float offset_y, // object position (pre-rotation)
    float center_x, float center_y, // pivot center in the object
    float width, float height,      // size of the object
    float rotation_deg,             // rotation angle in degrees
    float *output_x, float *output_y // output screen position
) {
    float rot = DegToRad(rotation_deg);
    float sin_theta = sinf(rot);
    float cos_theta = cosf(rot);

    // Relative position to rotation center
    float relative_x = offset_x - center_x + (width / 2.0f);
    float relative_y = offset_y - center_y + (height / 2.0f);

    // Apply rotation
    float rotated_x = (relative_x * cos_theta) - (relative_y * sin_theta);
    float rotated_y = (relative_y * cos_theta) + (relative_x * sin_theta);

    // Final screen position
    *output_x = x + (center_x - (width / 2.0f)) + rotated_x;
    *output_y = y + (center_y - (height / 2.0f)) + rotated_y;
}

void rotate_point_around_center(float cx, float cy, float angle, float x, float y, float *out_x, float *out_y) {
    float s = sinf(DegToRad(-angle));
    float c = cosf(DegToRad(-angle));

    x -= cx;
    y -= cy;

    float xnew = x * c - y * s;
    float ynew = x * s + y * c;

    *out_x = xnew + cx;
    *out_y = ynew + cy;
}

float randomf() {
    return (float)rand() / RAND_MAX;
}

float random_float(float min, float max) {
    return ((max - min) * ((float)rand() / RAND_MAX)) + min;
}

int random_int(int min, int max) {
    return rand() % (max + 1 - min) + min;
}

float map_range(float val, float min1, float max1, float min2, float max2) {
    return min2 + ((max2 - min2) / (max1 - min1)) * (val - min1);
}

float adjust_angle(float angle, int flipX, int flipY) {
    // Normalize to [0, 360)
    angle = positive_fmod(angle, 360);

    if (flipX && flipY) {
        angle = fmodf(angle + 180.0f, 360.0f);
    } else if (flipX) {
        angle = 180.0f - angle;
    } else if (flipY) {
        angle = -angle;
    }

    // Normalize again
    angle = positive_fmod(angle, 360);

    return angle;
}

float adjust_angle_x(float angle, int flipX) {
    angle = positive_fmod(angle, 360);

    if (flipX) {
        angle = 180.0f - angle;
    } 

    angle = positive_fmod(angle, 360);

    return angle;
}

float adjust_angle_y(float angle, int flipY) {
    angle = positive_fmod(angle, 360);

    if (flipY) {
        angle = angle + 180;
    }

    angle = positive_fmod(angle, 360);

    return angle;
}

float square_distance(float xa, float ya, float xb, float yb) {
	return ((xb - xa) * (xb - xa)) + ((yb - ya) * (yb - ya));
}

float clampf(float d, float min, float max) {
    const float t = d < min ? min : d;
    return t > max ? max : t;
}

float repeat(float a, float length) {
	return clampf(a - floorf(a / length) * length, 0.0f, length);
}

float slerp(float a, float b, float ratio) {
	float delta = repeat((b - a), 360.f);
	if (delta > 180.f)
		delta -= 360.f;
	return a + delta * clampf(ratio, 0.f, 1.f);
}

float lerp(float from, float to, float alpha) {
    return from * (1.0f - alpha) + to * alpha;
}

float iLerp(float a, float b, float ratio, float dt) {
	const float rDelta = dt / STEPS_DT;
	const float s	  = 1.f - ratio;

	float iRatio = 1.f - powf(s, rDelta);

	return lerp(a, b, iRatio);
}

float iSlerp(float a, float b, float ratio, float dt) {
	const float rDelta = dt / STEPS_DT;
	const float s	  = 1.f - ratio;

	float iRatio = 1.f - powf(s, rDelta);

	return slerp(a, b, iRatio);
}

// Generic easing out function, not from GD
float ease_out(float current, float target, float smoothing) {
    return current + (target - current) * smoothing;
}

static guVector axis = (guVector){0.0f, 0.0f, 1.0f};


// Custom GRRLIB functions for maximum performance (so it can be batched)

void set_texture(const GRRLIB_texImg *tex) {
    
    if (tex == NULL || tex->data == NULL)
        return;

    GXTexObj  texObj;
    GX_InitTexObj(&texObj, tex->data, tex->w, tex->h,
                  tex->format, GX_CLAMP, GX_CLAMP, GX_FALSE);

    if (GRRLIB_Settings.antialias == false) {
        GX_InitTexObjLOD(&texObj, GX_NEAR, GX_NEAR,
                         0.0f, 0.0f, 0.0f, 0, 0, GX_ANISO_1);
    }

    GX_LoadTexObj(&texObj,      GX_TEXMAP0);
}

void  custom_drawImg (const f32 xpos, const f32 ypos, const GRRLIB_texImg *tex, const f32 degrees, const f32 scaleX, const f32 scaleY, const u32 color) {
    Mtx       m, m1, m2, mv;

    guMtxIdentity  (m1);
    guMtxScaleApply(m1, m1, scaleX, scaleY, 1.0f);
    guMtxRotAxisDeg(m2, &axis, degrees);
    guMtxConcat    (m2, m1, m);

    const f32 width  = tex->w * 0.5f;
    const f32 height = tex->h * 0.5f;

    guMtxTransApply(m, m,
        xpos +width  +tex->handlex
            -tex->offsetx +( scaleX *(-tex->handley *sin(-DegToRad(degrees))
                                      -tex->handlex *cos(-DegToRad(degrees))) ),
        ypos +height +tex->handley
            -tex->offsety +( scaleY *(-tex->handley *cos(-DegToRad(degrees))
                                      +tex->handlex *sin(-DegToRad(degrees))) ),
        0);
    guMtxConcat(GXmodelView2D, m, mv);

    GX_LoadPosMtxImm(mv, GX_PNMTX0);
    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
        GX_Position3f32(-width, -height, 0.0f);
        GX_Color1u32   (color);
        GX_TexCoord2f32(0, 0);

        GX_Position3f32(width, -height, 0.0f);
        GX_Color1u32   (color);
        GX_TexCoord2f32(1, 0);

        GX_Position3f32(width, height, 0.0f);
        GX_Color1u32   (color);
        GX_TexCoord2f32(1, 1);

        GX_Position3f32(-width, height, 0.0f);
        GX_Color1u32   (color);
        GX_TexCoord2f32(0, 1);
    GX_End();
}

void  custom_gxengine (const guVector v[], const u32 color[], const u16 n,
                       const u8 fmt, const float lineWidth) {
    // Set line width if using line primitives
    if (fmt == GX_LINESTRIP || fmt == GX_LINES) {
        GX_SetLineWidth(lineWidth * 8, fmt);
    }
    
    GX_Begin(fmt, GX_VTXFMT0, n);
    for (u16 i = 0; i < n; i++) {
        GX_Position3f32(v[i].x, v[i].y, v[i].z);
        GX_Color1u32(color[i]);
    }
    GX_End();
}

void custom_ellipse(const float x, const float y, const float radiusX,
                    const float radiusY, const u32 color, const u8 filled,
                    const float lineWidth) {
    int segments = (int)(MAX(radiusX, radiusY) * 0.75f); // tweak factor for smoothness
    segments = CLAMP(segments, 12, 256);                 // minimum and maximum
    guVector v[segments + 1];  // +1 to close the loop
    u32 ncolor[segments + 1];

    for (int i = 0; i < segments; i++) {
        float angle = i * 2 * M_PI / segments;
        v[i].x = cosf(angle) * radiusX + x;
        v[i].y = sinf(angle) * radiusY + y;
        v[i].z = 0;
        ncolor[i] = color;
        
        if (!filled) {
            Vec2D dir = { x - v[i].x, y - v[i].y };
            dir = normalize(dir);
            
            v[i].x += dir.x * (lineWidth - 1);
            v[i].y += dir.y * (lineWidth - 1);
        }
    }

    // Close the loop for line mode
    v[segments] = v[0];
    ncolor[segments] = color;
    
    if (filled == false) {
        custom_gxengine(v, ncolor, segments + 1, GX_LINESTRIP, lineWidth);
    } else {
        custom_gxengine(v, ncolor, segments, GX_TRIANGLEFAN, 0);
    }
}

void  custom_circle (const f32 x, const f32 y, const f32 radius,
                     const u32 color) {
    custom_ellipse(x, y, radius, radius, color, TRUE, 1);
}

void  custom_circunference (const f32 x, const f32 y, const f32 radius,
                     const u32 color, const f32 lineWidth) {
    custom_ellipse(x, y, radius, radius, color, FALSE, lineWidth);
}

void  custom_drawPart (const f32 xpos, const f32 ypos, const f32 partx, const f32 party, const f32 partw, const f32 parth, const GRRLIB_texImg *tex, const f32 degrees, const f32 scaleX, const f32 scaleY, const u32 color) {
    Mtx       m, m1, m2, mv;

    if (tex == NULL || tex->data == NULL)
        return;

    // The 0.001f/x is the frame correction formula by spiffen
    const f32 s1 = (partx / tex->w) + (0.001f / tex->w);
    const f32 s2 = ((partx + partw) / tex->w) - (0.001f / tex->w);
    const f32 t1 = (party / tex->h) + (0.001f / tex->h);
    const f32 t2 = ((party + parth) / tex->h) - (0.001f / tex->h);

    const f32 width  = partw * 0.5f;
    const f32 height = parth * 0.5f;

    guMtxIdentity  (m1);
    guMtxScaleApply(m1, m1, scaleX, scaleY, 1.0f);
    guMtxRotAxisDeg(m2, &axis, degrees);
    guMtxConcat    (m2, m1, m);

    guMtxTransApply(m, m,
        xpos +width  +tex->handlex
            -tex->offsetx +( scaleX *(-tex->handley *sin(-DegToRad(degrees))
                                      -tex->handlex *cos(-DegToRad(degrees))) ),
        ypos +height +tex->handley
            -tex->offsety +( scaleY *(-tex->handley *cos(-DegToRad(degrees))
                                      +tex->handlex *sin(-DegToRad(degrees))) ),
        0);

    guMtxConcat(GXmodelView2D, m, mv);

    GX_LoadPosMtxImm(mv, GX_PNMTX0);
    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
        GX_Position3f32(-width, -height, 0.0f);
        GX_Color1u32   (color);
        GX_TexCoord2f32(s1, t1);

        GX_Position3f32(width, -height,  0.0f);
        GX_Color1u32   (color);
        GX_TexCoord2f32(s2, t1);

        GX_Position3f32(width, height,  0.0f);
        GX_Color1u32   (color);
        GX_TexCoord2f32(s2, t2);

        GX_Position3f32(-width, height,  0.0f);
        GX_Color1u32   (color);
        GX_TexCoord2f32(s1, t2);
    GX_End();
    GX_LoadPosMtxImm(GXmodelView2D, GX_PNMTX0);
}

void  custom_rectangle (const f32 x,      const f32 y,
                        const f32 width,  const f32 height,
                        const u32 color, const bool filled) {
    const f32 x2 = x + width;
    const f32 y2 = y + height;

    if (filled == true) {
        GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
            GX_Position3f32(x, y, 0.0f);
            GX_Color1u32(color);
            GX_Position3f32(x2, y, 0.0f);
            GX_Color1u32(color);
            GX_Position3f32(x2, y2, 0.0f);
            GX_Color1u32(color);
            GX_Position3f32(x, y2, 0.0f);
            GX_Color1u32(color);
        GX_End();
    }
    else {
        GX_Begin(GX_LINESTRIP, GX_VTXFMT0, 5);
            GX_Position3f32(x, y, 0.0f);
            GX_Color1u32(color);
            GX_Position3f32(x2, y, 0.0f);
            GX_Color1u32(color);
            GX_Position3f32(x2, y2, 0.0f);
            GX_Color1u32(color);
            GX_Position3f32(x, y2, 0.0f);
            GX_Color1u32(color);
            GX_Position3f32(x, y, 0.0f);
            GX_Color1u32(color);
        GX_End();
    }
}

void custom_rounded_rectangle(float x, float y,
                              float width, float height,
                              float radius,
                              u32 color) {
    // Clamp radius
    if (radius > width * 0.5f) radius = width * 0.5f;
    if (radius > height * 0.5f) radius = height * 0.5f;

    const int cornerSegments = 8; // number of points per corner
    guVector v[cornerSegments + 2]; // +1 for corner center, +1 for closing the fan
    u32 ncolor[cornerSegments + 2];

    float cornerCentersX[4] = { x + radius, x + width - radius, x + width - radius, x + radius };
    float cornerCentersY[4] = { y + radius, y + radius, y + height - radius, y + height - radius };
    float startAngles[4]   = { M_PI, -M_PI/2, 0, M_PI/2 }; // TL, TR, BR, BL

    for (int corner = 0; corner < 4; corner++) {
        guVector center = { cornerCentersX[corner], cornerCentersY[corner], 0.0f };
        for (int i = 0; i <= cornerSegments; i++) {
            float t = (float)i / cornerSegments;
            float angle = startAngles[corner] + t * (M_PI/2);
            v[i].x = cosf(angle) * radius + center.x;
            v[i].y = sinf(angle) * radius + center.y;
            v[i].z = 0.0f;
            ncolor[i] = color;
        }

        // Draw triangle fan for this corner
        v[cornerSegments + 1] = center; // corner center at the end
        ncolor[cornerSegments + 1] = color;

        GX_Begin(GX_TRIANGLEFAN, GX_VTXFMT0, cornerSegments + 2);
        for (int k = 0; k <= cornerSegments + 1; k++) {
            GX_Position3f32(v[k].x, v[k].y, v[k].z);
            GX_Color1u32(ncolor[k]);
        }
        GX_End();
    }

    float left   = x;
    float right  = x + width;
    float top    = y;
    float bottom = y + height;

    // Top edge
    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
        GX_Position3f32(left + radius, top, 0.0f);
        GX_Color1u32(color);
        GX_Position3f32(right - radius, top, 0.0f);
        GX_Color1u32(color);
        GX_Position3f32(right - radius, top + radius, 0.0f);
        GX_Color1u32(color);
        GX_Position3f32(left + radius, top + radius, 0.0f);
        GX_Color1u32(color);
    GX_End();

    // Bottom edge
    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
        GX_Position3f32(left + radius, bottom - radius, 0.0f);
        GX_Color1u32(color);
        GX_Position3f32(right - radius, bottom - radius, 0.0f);
        GX_Color1u32(color);
        GX_Position3f32(right - radius, bottom, 0.0f);
        GX_Color1u32(color);
        GX_Position3f32(left + radius, bottom, 0.0f);
        GX_Color1u32(color);
    GX_End();

    // Left edge
    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
        GX_Position3f32(left, top + radius, 0.0f);
        GX_Color1u32(color);
        GX_Position3f32(left + radius, top + radius, 0.0f);
        GX_Color1u32(color);
        GX_Position3f32(left + radius, bottom - radius, 0.0f);
        GX_Color1u32(color);
        GX_Position3f32(left, bottom - radius, 0.0f);
        GX_Color1u32(color);
    GX_End();

    // Right edge
    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
        GX_Position3f32(right - radius, top + radius, 0.0f);
        GX_Color1u32(color);
        GX_Position3f32(right, top + radius, 0.0f);
        GX_Color1u32(color);
        GX_Position3f32(right, bottom - radius, 0.0f);
        GX_Color1u32(color);
        GX_Position3f32(right - radius, bottom - radius, 0.0f);
        GX_Color1u32(color);
    GX_End();

    // Center rectangle
    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
        GX_Position3f32(left + radius, top + radius, 0.0f);
        GX_Color1u32(color);
        GX_Position3f32(right - radius, top + radius, 0.0f);
        GX_Color1u32(color);
        GX_Position3f32(right - radius, bottom - radius, 0.0f);
        GX_Color1u32(color);
        GX_Position3f32(left + radius, bottom - radius, 0.0f);
        GX_Color1u32(color);
    GX_End();
}

float normalize_angle(float angle) {
    angle = fmodf(angle, 360.0f);
    if (angle < 0) angle += 360.0f;
    return angle;
}

bool is_effectively_integer(float x) {
    return fabsf(x - roundf(x)) < FLT_EPSILON;
}

float ip1_ceilf(float x) {
    float c = ceilf(x);
    if (is_effectively_integer(x)) {
        return c + 1.0f;
    }
    return c;
}

void custom_line (const f32 x1, const f32 y1,
                   const f32 x2, const f32 y2, const u32 color) {
    GX_Begin(GX_LINES, GX_VTXFMT0, 2);
        GX_Position3f32(x1, y1, 0.0f);
        GX_Color1u32(color);
        GX_Position3f32(x2, y2, 0.0f);
        GX_Color1u32(color);
    GX_End();
}
void draw_thick_line(const float x1, const float y1, const float x2, const float y2, const float thickness, const u32 color) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float length = sqrtf(dx * dx + dy * dy);

    // Normalize perpendicular vector
    float px = -dy / length;
    float py =  dx / length;

    // Half-width offset
    float hw = thickness / 2.0f;

    // Four points of the quad
    float x1a = x1 + px * hw;
    float y1a = y1 + py * hw;
    float x1b = x1 - px * hw;
    float y1b = y1 - py * hw;

    float x2a = x2 + px * hw;
    float y2a = y2 + py * hw;
    float x2b = x2 - px * hw;
    float y2b = y2 - py * hw;

    // Draw as two triangles (quad)
    GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 6);

    GX_Position3f32(x1a, y1a, 0.0f);
    GX_Color1u32(color);
    GX_Position3f32(x2a, y2a, 0.0f);
    GX_Color1u32(color);
    GX_Position3f32(x2b, y2b, 0.0f);
    GX_Color1u32(color);

    GX_Position3f32(x2b, y2b, 0.0f);
    GX_Color1u32(color);
    GX_Position3f32(x1b, y1b, 0.0f);
    GX_Color1u32(color);
    GX_Position3f32(x1a, y1a, 0.0f);
    GX_Color1u32(color);

    GX_End();
}

// Returns true if vertices are counter-clockwise
bool is_ccw(Vec2D *poly, int n) {
    float sum = 0.0f;
    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n;
        sum += (poly[j].x - poly[i].x) * (poly[j].y + poly[i].y);
    }
    return sum < 0.0f; // CCW if sum < 0
}


void draw_hitbox_line_inward(Vec2D rect[4], 
                             const float x1, const float y1,
                             const float x2, const float y2,
                             const float thickness,
                             const float cx, const float cy,
                             const u32 color) {

    float ex = x2 - x1;
    float ey = y2 - y1;
    float length = sqrtf(ex * ex + ey * ey);

    // Inward normal depends on winding
    float nx, ny;
    if (is_ccw(rect, 4)) {
        nx = -ey / length;
        ny =  ex / length;
    } else {
        nx =  ey / length;
        ny = -ex / length;
    }

    // Flip it to point inward if not mirror
    if (state.mirror_factor < 0.5f) {
        nx = -nx;
        ny = -ny;
    }

    // Offset inward
    float ox = nx * thickness;
    float oy = ny * thickness;

    // Build quad
    float x1a = x1;
    float y1a = y1;
    float x2a = x2;
    float y2a = y2;

    float x1b = x1 + ox;
    float y1b = y1 + oy;
    float x2b = x2 + ox;
    float y2b = y2 + oy;

    GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 6);

    GX_Position3f32(x1a, y1a, 0.0f);
    GX_Color1u32(color);
    GX_Position3f32(x2a, y2a, 0.0f);
    GX_Color1u32(color);
    GX_Position3f32(x2b, y2b, 0.0f);
    GX_Color1u32(color);

    GX_Position3f32(x2b, y2b, 0.0f);
    GX_Color1u32(color);
    GX_Position3f32(x1b, y1b, 0.0f);
    GX_Color1u32(color);
    GX_Position3f32(x1a, y1a, 0.0f);
    GX_Color1u32(color);

    GX_End();
}

void compute_mitered_offsets(Vec2D *poly, Vec2D *offsets, int n, float thickness, bool ccw) {
    for (int i = 0; i < n; i++) {
        int prev = (i + n - 1) % n;
        int next = (i + 1) % n;

        // Edge vectors
        Vec2D e_prev = { poly[i].x - poly[prev].x, poly[i].y - poly[prev].y };
        Vec2D e_next = { poly[next].x - poly[i].x, poly[next].y - poly[i].y };

        // Normals
        Vec2D n_prev = normalize(perpendicular(e_prev));
        Vec2D n_next = normalize(perpendicular(e_next));

        // Flip to point inward if mirror
        if (state.mirror_factor < 0.5f) { n_prev.x = -n_prev.x; n_prev.y = -n_prev.y; n_next.x = -n_next.x; n_next.y = -n_next.y; }

        // Miter = normalize(n_prev + n_next)
        Vec2D miter = normalize((Vec2D){ n_prev.x + n_next.x, n_prev.y + n_next.y });

        // Compute miter length to preserve thickness
        float cos_half_angle = dot_vec(miter, n_next); // cos(theta/2)
        float miter_length = thickness / cos_half_angle;

        offsets[i].x = poly[i].x + miter.x * miter_length;
        offsets[i].y = poly[i].y + miter.y * miter_length;
    }
}

void draw_polygon_inward_mitered(Vec2D *poly, int n, float thickness, u32 color) {
    bool ccw = is_ccw(poly, n);
    Vec2D offsets[n];
    compute_mitered_offsets(poly, offsets, n, thickness, ccw);

    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n;

        GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 6);

        // Original vertices
        GX_Position3f32(poly[i].x, poly[i].y, 0.0f);
        GX_Color1u32(color);
        GX_Position3f32(poly[j].x, poly[j].y, 0.0f);
        GX_Color1u32(color);

        // Offset vertices (inward)
        GX_Position3f32(offsets[j].x, offsets[j].y, 0.0f);
        GX_Color1u32(color);

        GX_Position3f32(offsets[j].x, offsets[j].y, 0.0f);
        GX_Color1u32(color);
        GX_Position3f32(offsets[i].x, offsets[i].y, 0.0f);
        GX_Color1u32(color);
        GX_Position3f32(poly[i].x, poly[i].y, 0.0f);
        GX_Color1u32(color);

        GX_End();
    }
}

float opacity = 0;

void draw_fade() {
    GRRLIB_FillScreen(RGBA(0,0,0, (int) opacity));
    GRRLIB_Render();
}


void fade_out() {
    GRRLIB_texImg *framebuffer = GRRLIB_CreateEmptyTextureFmt(rmode->fbWidth, rmode->efbHeight, GX_TF_RGBA8);
    GRRLIB_Screen2Texture(0, 0, framebuffer, FALSE);

    u64 prevTicks = gettime();
    while (opacity < 255) {
        start_frame = gettime();
        float frameTime = ticks_to_secs_float(start_frame - prevTicks);
        dt = frameTime;
        prevTicks = start_frame;

        opacity += FADE_SPEED * dt;
        if (opacity > 255) opacity = 255;
        GRRLIB_DrawImg(0, 0, framebuffer, 0, 1 * screen_factor_x, 1, 0xffffffff);
        draw_fade();
    }
    GRRLIB_FreeTexture(framebuffer);
}

void fade_in() {
    GRRLIB_texImg *framebuffer = GRRLIB_CreateEmptyTextureFmt(rmode->fbWidth, rmode->efbHeight, GX_TF_RGBA8);
    GRRLIB_Screen2Texture(0, 0, framebuffer, FALSE);

    u64 prevTicks = gettime();
    while (opacity > 0) {
        start_frame = gettime();
        float frameTime = ticks_to_secs_float(start_frame - prevTicks);
        dt = frameTime;
        prevTicks = start_frame;
        opacity -= FADE_SPEED * dt;
        if (opacity < 0) opacity = 0;
        GRRLIB_DrawImg(0, 0, framebuffer, 0, 1 * screen_factor_x, 1, 0xffffffff);
        draw_fade();
    }
    GRRLIB_FreeTexture(framebuffer);
}

void fade_in_level() {
    u64 prevTicks = gettime();
    double accumulator = 0.0;
    while (opacity > 0) {
        start_frame = gettime();
        float frameTime = ticks_to_secs_float(start_frame - prevTicks);
        dt = frameTime;
        
        accumulator += frameTime;

        prevTicks = start_frame;
        opacity -= FADE_SPEED * dt;
        if (opacity < 0) opacity = 0;
        
        while (accumulator >= STEPS_DT) {
            update_percentage();
            handle_objects();
            update_particles();
            accumulator -= STEPS_DT;
        }
        
        draw_game();
        draw_fade();
    }
}

void wait_initial_time() {
    u64 prevTicks = gettime();
    double accumulator = 0.0;
    float time_elapsed = 0.f;
    while (time_elapsed < 0.5f) {
        start_frame = gettime();
        float frameTime = ticks_to_secs_float(start_frame - prevTicks);
        dt = frameTime;
        
        accumulator += frameTime;
        time_elapsed += frameTime;

        prevTicks = start_frame;
        opacity -= FADE_SPEED * dt;
        if (opacity < 0) opacity = 0;

        while (accumulator >= STEPS_DT) {
            update_particles();
            update_beat();
            accumulator -= STEPS_DT;
        }
        
        draw_game();
        draw_fade();
    }
}

void  draw_glyph (const f32 xpos, const f32 ypos, const f32 partx, const f32 party, const f32 partw, const f32 parth, const GRRLIB_texImg *tex, const f32 degrees, const f32 scaleX, const f32 scaleY, const u32 color) {
    GXTexObj  texObj;
    Mtx       m, m1, m2, mv;

    if (tex == NULL || tex->data == NULL)
        return;

    // The 0.001f/x is the frame correction formula by spiffen
    const f32 s1 = (partx / tex->w) + (0.001f / tex->w);
    const f32 s2 = ((partx + partw) / tex->w) - (0.001f / tex->w);
    const f32 t1 = (party / tex->h) + (0.001f / tex->h);
    const f32 t2 = ((party + parth) / tex->h) - (0.001f / tex->h);

    GX_InitTexObj(&texObj, tex->data,
                  tex->w, tex->h,
                  tex->format, GX_CLAMP, GX_CLAMP, GX_FALSE);

    if (GRRLIB_Settings.antialias == false) {
        GX_InitTexObjLOD(&texObj, GX_NEAR, GX_NEAR,
                         0.0f, 0.0f, 0.0f, 0, 0, GX_ANISO_1);
        GX_SetCopyFilter(GX_FALSE, rmode->sample_pattern, GX_FALSE, rmode->vfilter);
    }
    else {
        GX_SetCopyFilter(rmode->aa, rmode->sample_pattern, GX_TRUE, rmode->vfilter);
    }

    GX_LoadTexObj(&texObj,      GX_TEXMAP0);
    GX_SetTevOp  (GX_TEVSTAGE0, GX_MODULATE);
    GX_SetVtxDesc(GX_VA_TEX0,   GX_DIRECT);

    const f32 width  = partw * 0.5f;
    const f32 height = parth * 0.5f;

    guMtxIdentity  (m1);
    guMtxScaleApply(m1, m1, scaleX, scaleY, 1.0f);
    guMtxRotAxisDeg(m2, &axis, degrees);
    guMtxConcat    (m2, m1, m);

    guMtxTransApply(m, m,
        xpos +width*scaleX  +tex->handlex
            -tex->offsetx +( scaleX *(-tex->handley *sin(-DegToRad(degrees))
                                      -tex->handlex *cos(-DegToRad(degrees))) ),
        ypos +height*scaleY +tex->handley
            -tex->offsety +( scaleY *(-tex->handley *cos(-DegToRad(degrees))
                                      +tex->handlex *sin(-DegToRad(degrees))) ),
        0);

    guMtxConcat(GXmodelView2D, m, mv);

    GX_LoadPosMtxImm(mv, GX_PNMTX0);
    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
        GX_Position3f32(-width, -height, 0.0f);
        GX_Color1u32   (color);
        GX_TexCoord2f32(s1, t1);

        GX_Position3f32(width, -height,  0.0f);
        GX_Color1u32   (color);
        GX_TexCoord2f32(s2, t1);

        GX_Position3f32(width, height,  0.0f);
        GX_Color1u32   (color);
        GX_TexCoord2f32(s2, t2);

        GX_Position3f32(-width, height,  0.0f);
        GX_Color1u32   (color);
        GX_TexCoord2f32(s1, t2);
    GX_End();
    GX_LoadPosMtxImm(GXmodelView2D, GX_PNMTX0);

    GX_SetTevOp  (GX_TEVSTAGE0, GX_PASSCLR);
    GX_SetVtxDesc(GX_VA_TEX0,   GX_NONE);
}

struct glyph *get_glyph(struct charset font, char character) {
    for (int i = 0; i < font.char_num; i++) {
        if (character == font.chars[i].id) return &font.chars[i];
    }
    return NULL;
}

float get_text_length(struct charset font, const float zoom, const char *text, ...) {
    
    char tmp[1024];

    va_list argp;
    va_start(argp, text);
    const int size = vsnprintf(tmp, sizeof(tmp), text, argp);
    va_end(argp);
    
    float text_length = 0;
    for (int i = 0; i < size; i++) {
        struct glyph *character = get_glyph(font, tmp[i]);

        if (character != NULL) {
            float xadvance = character->xadvance * zoom;

            text_length += xadvance;
        }
    }
    return text_length;
}

void draw_text(struct charset font, GRRLIB_texImg *tex, const float x, const float y, const float zoom, const char *text, ...) {
    if (!text || !tex || !tex->data) {
        return;
    }
    GRRLIB_SetHandle(tex, tex->w / 2, tex->h / 2);

    char tmp[1024];

    va_list argp;
    va_start(argp, text);
    const int size = vsnprintf(tmp, sizeof(tmp), text, argp);
    va_end(argp);

    float offset = 0;
    for (int i = 0; i < size; i++) {
        struct glyph *character = get_glyph(font, tmp[i]);

        if (character != NULL) {
            int tile_x = character->x;
            int tile_y = character->y;
            int width = character->width;
            int height = character->height;
            float xoffset = character->xoffset * zoom;
            float yoffset = character->yoffset * zoom;
            float xadvance = character->xadvance * zoom;

            draw_glyph(x + offset + xoffset, y + yoffset, tile_x, tile_y, width, height, tex, 0, zoom, zoom, 0xffffffff);
            
            offset += xadvance;
        }
    }
}

Color HSV_combine(Color color, HSV hsv) {
    if (hsv.h == 0 && hsv.s == 0 && hsv.v == 0) {
        return color;
    }

    HSV color_hsv;
    convertRGBtoHSV(color.r, color.g, color.b, &color_hsv.h, &color_hsv.s, &color_hsv.v);

    hsv.h += color_hsv.h;

    if (hsv.sChecked) {
        hsv.s += color_hsv.s;
    } else {
        hsv.s *= color_hsv.s;
    }
    
    if (hsv.vChecked) {
        hsv.v += color_hsv.v;
    } else {
        hsv.v *= color_hsv.v;
    }

    if (hsv.h < 0) hsv.h += 360;
    if (hsv.h >= 360) hsv.h -= 360;
    if (hsv.s > 1) hsv.s = 1;
    if (hsv.v > 1) hsv.v = 1;

    Color returned_color;
    convertHSVtoRGB(hsv.h, hsv.s, hsv.v, &returned_color.r, &returned_color.g, &returned_color.b);

    return returned_color;
}
