/*
**  $Id: win_wvfb.c 7427 2007-08-21 06:00:11Z weiym $
**  
**  win_wvfb.c: A subdriver of shadow NEWGAL engine for Windows WVFB 4bpp/1bpp.
**
**  Copyring (C) 2003 ~ 2007 Feynman Software.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mgconfig.h"

#ifdef _NEWGAL_ENGINE_SHADOW 

#include "common.h"
#include "minigui.h"
#include "newgal.h"
#include "shadow.h"

#if defined (WIN32) && defined (__TARGET_WVFB__)

#include <stdio.h>
#include <stdlib.h>

extern void *wvfb_shadow_init (void);
extern void wvfb_shadow_close (void);

struct WVFBHeader
{
    int width;
    int height;
    int depth;
    int linestep;
    int dataoffset;
    RECT update;
    BYTE dirty;
    int  numcols;
    unsigned int clut[256];
};

struct WVFBHeader* wvfb_hdr;

static int a_getinfo (struct shadowlcd_info* lcd_info)
{
    lcd_info->width = wvfb_hdr->width;
    lcd_info->height = wvfb_hdr->height;
    lcd_info->bpp = wvfb_hdr->depth;
    if (wvfb_hdr->depth >= 8)
        lcd_info->fb = ((char*)wvfb_hdr) + wvfb_hdr->dataoffset;
    else
        lcd_info->fb = NULL;
    lcd_info->rlen = wvfb_hdr->linestep;
    lcd_info->type = 0;
    
    return 0;
}

static int a_setclut_1bpp (int first, int ncolors, GAL_Color *colors)
{
    int i, entry = first;
    int set = 0;

    for (i = 0; i < ncolors; i++) {
        if (entry == 0 || entry == 255) {
            set++;
            wvfb_hdr->clut [entry/255] = 
                (0xff << 24) | ((colors[i].r & 0xff) << 16) | ((colors[i].g & 0xff) << 8) | (colors[i].b & 0xff);
        }
        entry ++;
    }

    return set;
}

static int a_setclut_4bpp (int first, int ncolors, GAL_Color *colors)
{
    int i, entry = first;
    int set = 0;

    for (i = 0; i < ncolors; i++) {
        if (entry == 0 || ((entry + 1) % 16 == 0)) {
            set++;
            wvfb_hdr->clut [entry/16] = 
                (0xff << 24) | ((colors[i].r & 0xff) << 16) | ((colors[i].g & 0xff) << 8) | (colors[i].b & 0xff);
        }
        entry ++;
    }

    return set;
}

static int a_setclut_8bpp (int first, int ncolors, GAL_Color *colors)
{
    int i, entry = first;
    int set = 0;

    for (i = 0; i < ncolors; i++) {
        if (entry < 256) {
            set++;
            wvfb_hdr->clut [entry++] = 
                (0xff << 24) | ((colors[i].r & 0xff) << 16) | ((colors[i].g & 0xff) << 8) | (colors[i].b & 0xff);
        }
    }

    return set;
}

static int a_init (void)
{
    wvfb_hdr = (struct WVFBHeader *) wvfb_shadow_init ();
	if ((int) wvfb_hdr == -1 || wvfb_hdr == NULL)
        return 3;
    switch (wvfb_hdr->depth) {
        case 1:
            __mg_shadow_lcd_ops.setclut = a_setclut_1bpp;
            break;
        case 4:
            __mg_shadow_lcd_ops.setclut = a_setclut_4bpp;
            break;
        case 8:
            __mg_shadow_lcd_ops.setclut = a_setclut_8bpp;
            break;
    }

    SetRect (&wvfb_hdr->update, 0, 0, 0, 0);
    wvfb_hdr->dirty = FALSE;
    return 0;
}

static int a_release (void)
{
    wvfb_shadow_close ();
	return 0;
}

static void a_sleep (void)
{
    /* 20ms */
    _sleep (20);
}

static unsigned char pixel_bit [] = {0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};

static void wvfb_update_1bpp (_THIS)
{
    RECT bound = wvfb_hdr->update;
    const BYTE* src_bits;
    BYTE* dst_bits;
    int width, height;
    int x, y, i;

    /* round x to be 8-aligned */
    bound.left = bound.left & ~0x07;
    bound.right = (bound.right + 7) & ~0x07;

    width = RECTW (bound);
    height = RECTH (bound);

    if (width <= 0 || height <= 0)
        return;

    src_bits = this->hidden->fb;
    src_bits += bound.top * this->hidden->pitch + bound.left;

    dst_bits = ((BYTE*) wvfb_hdr) + wvfb_hdr->dataoffset;
    dst_bits += bound.top * wvfb_hdr->linestep + (bound.left >> 3);

    for (y = 0; y < height; y++) {
        const BYTE* src_line = src_bits;
        BYTE* dst_line = dst_bits;
        for (x = 0; x < width; x+=8) {
            *dst_line = 0;
            for (i = 0; i < 8; i++) {
                if (*src_line < 128)
                    *dst_line |= pixel_bit [i];
                src_line ++;
            }
            dst_line ++;
		
        }
        src_bits += this->hidden->pitch;
        dst_bits += wvfb_hdr->linestep;
    }
}

static void wvfb_update_4bpp (_THIS)
{
    RECT bound = wvfb_hdr->update;
    const BYTE* src_bits;
    BYTE* dst_bits;
    int width, height;
    int x, y;

    /* round x to be 2-aligned */
    bound.left = bound.left & ~0x01;
    bound.right = (bound.right + 1) & ~0x01;

    width = RECTW (bound);
    height = RECTH (bound);

    if (width <= 0 || height <= 0)
        return;

    src_bits = this->hidden->fb;
    src_bits += bound.top * this->hidden->pitch + bound.left;

    dst_bits = (BYTE*) wvfb_hdr + wvfb_hdr->dataoffset;
    dst_bits += bound.top * wvfb_hdr->linestep + (bound.left >> 1);

    for (y = 0; y < height; y++) {
        const BYTE* src_line = src_bits;
        BYTE* dst_line = dst_bits;
        for (x = 0; x < width; x+=2) {
            *dst_line = (*src_line & 0xF0) | (*(++src_line) >> 4);
			src_line ++;
            dst_line ++;
        }
        src_bits += this->hidden->pitch;
        dst_bits += wvfb_hdr->linestep;
    }
}

static void a_refresh (_THIS, const RECT* update)
{
    RECT bound;
    wvfb_shadow_lock ();
    bound = wvfb_hdr->update;

    if (bound.right < 0) bound.right = 0;
    if (bound.bottom < 0) bound.bottom = 0;

    GetBoundRect (&bound, &bound, update);

    wvfb_hdr->update = bound;

     /* Copy the bits from Shadow FrameBuffer to WVFB FrameBuffer */
    if (wvfb_hdr->depth == 1)
        wvfb_update_1bpp (this);
    else if (wvfb_hdr->depth == 4)
        wvfb_update_4bpp (this);

    wvfb_hdr->dirty = TRUE;
	wvfb_shadow_unlock ();
}

struct shadow_lcd_ops __mg_shadow_lcd_ops = {
    a_init,
    a_getinfo,
    a_release,
    NULL,
    a_sleep,
    a_refresh
};

#endif /* __WIN32__ && __TARGET_WVFB__ */

#endif /* _NEWGAL_ENGINE_SHADOW */

