/*
** $Id: colorspace.c 7371 2007-08-16 05:30:47Z xgwang $
** 
** colorspace.c: ColorSpace converters.
** 
** Copyright (C) 2004 ~ 2007 Feynman Software.
**
** Author: Linxs (linxs@minigui.net)
**
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "common.h"

#ifdef _USE_NEWGAL

#include "minigui.h"
#include "colorspace.h"

#define LIMIT(a, b, c ) do {a=((a<b)?b:a);a=(a>c)?c:a;} while (0)

// Needed by rgb2hsv()
static float maxrgb (float r, float g, float b)
{
    float max;
  
    if (r > g)
        max = r;
    else
        max = g;
    if (b > max)
        max = b;

    return max;
}


// Needed by rgb2hsv()
static float minrgb (float r, float g, float b)
{
    float min;
  
    if (r < g)
        min = r;
    else
        min = g;
    if (b < min)
        min = b;

    return min;
}

/* 
   Assumes (r,g,b) range from 0.0 to 1.0
   Sets h in degrees: 0.0 to 360. s,v in [0.,1.]
*/
static void rgb2hsva (float r, float g, float b, float *hout, float *sout, float *vout)
{
    float h=0, s=1.0, v=1.0;
    float max_v, min_v, diff, r_dist, g_dist, b_dist;
    float undefined = 0.0;

    max_v = maxrgb (r,g,b);
    min_v = minrgb (r,g,b);
    diff = max_v - min_v;
    v = max_v;

    if (max_v != 0)
        s = diff/max_v;
    else
        s = 0.0;
    if (s == 0)
        h = undefined;
    else {
        r_dist = (max_v - r)/diff;
        g_dist = (max_v - g)/diff;
        b_dist = (max_v - b)/diff;
        if (r == max_v) 
            h = b_dist - g_dist;
        else
        if (g == max_v)
            h = 2 + r_dist - b_dist;
        else {
            if (b == max_v)
                h = 4 + g_dist - r_dist;
            else
                printf("rgb2hsv::How did I get here?\n");
        }

        h *= 60;
        if (h < 0)
            h += 360.0;
    }

    *hout = h;
    *sout = s;
    *vout = v;
}

/* 
   Assumes (r,g,b) range from 0 to 255
   Sets h in degrees: 0 to 360. s,v in [0,255]
*/
void RGB2HSV (Uint8 r, Uint8 g, Uint8 b, Uint16 *hout, Uint8 *sout, Uint8 *vout)
{
    float h, s, v;

    rgb2hsva (r/255., g/255., b/255., &h, &s, &v);

    *hout = h;
    *sout = 255 * s;
    *vout = 255 * v;
}

void HSV2RGB (Uint16 hin, Uint8 sin, Uint8 vin, Uint8 *rout, Uint8 *gout, Uint8 *bout)
{
    float h, s, v;
    float r = 0, g = 0, b = 0;
    float f, p, q, t;
    int i;

    h = hin;
    s = sin/255.;
    v = vin/255.;

    if (sin == 0 ) {
        r = v;
        g = v;
        b = v;
    }
    else {
        if (hin == 360)
            h = 0.0;
        h /= 60.;
        i = (int) h;
        f = h - i;
        p = v*(1-s);
        q = v*(1-(s*f));
        t = v*(1-s*(1-f));

        switch (i) {
        case 0:
            r = v;
            g = t;
            b = p;
            break;
        case 1:
            r = q;
            g = v;
            b = p;
            break;
        case 2:
            r = p;
            g = v;
            b = t;
            break;
        case 3:
            r = p;
            g = q;
            b = v;
            break;
        case 4:
            r = t;
            g = p;
            b = v;
            break;
        case 5:
            r = v;
            g = p;
            b = q;
            break;
        default:
            r = 1.0;
            b = 1.0;
            b = 1.0;
            break;
        }
    }

    *rout = 255*r;
    *gout = 255*g;
    *bout = 255*b;
}

void YUV2RGB (int y, int u, int v, Uint8 *r, Uint8 *g, Uint8 *b)
{
    double rr, gg ,bb;
    
    y = y - 16;
    u = u - 128;
    v = v - 128;
    rr = 0.00456621 * y + 0.00625893 * v;
    gg = 0.00456621 * y - 0.00153632 * u - 0.00318811 * v;
    bb = 0.00456621 * y + .00791071 * u;
    
    *r = 255 * rr;
    *g = 255 * gg;
    *b = 255 * bb;
}

void RGB2YUV (Uint8 r, Uint8 g, Uint8 b, int *y, int *u, int *v)
{
#if 1 /* these are the two formulas that I found on the FourCC site... */
    *y = 0.299*r + 0.587*g + 0.114*b;
    *u = (b-*y)*0.565 + 128;
    *v = (r-*y)*0.713 + 128;
#else
    *y = (0.257 * r) + (0.504 * g) + (0.098 * b) + 16;
    *u = 128 - (0.148 * r) - (0.291 * g) + (0.439 * b);
    *v = 128 + (0.439 * r) - (0.368 * g) - (0.071 * b);
#endif
}

#endif /* _USE_NEWGAL */

