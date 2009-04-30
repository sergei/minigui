/*
**  $Id: linux_qvfb.c 7427 2007-08-21 06:00:11Z weiym $
**  
**  linux_qvfb.c: A subdriver of shadow NEWGAL engine for Linux QVFB 4bpp/1bpp.
**
**  Copyright (C) 2003 ~ 2007 Feynman Software.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

#ifdef _NEWGAL_ENGINE_SHADOW 

#if defined (__LINUX__) && defined (__TARGET_QVFB__)

#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#include "minigui.h"
#include "newgal.h"
#include "shadow.h"

#define QT_VFB_MOUSE_PIPE   "/tmp/.qtvfb_mouse-%d"
#define QT_VFB_KEYBOARD_PIPE    "/tmp/.qtvfb_keyboard-%d"
#define QTE_PIPE "QtEmbedded"

struct QVFBHeader
{
    int width;
    int height;
    int depth;
    int linestep;
    int dataoffset;
    RECT update;
    BOOL dirty;
    int  numcols;
    unsigned int clut[256];
};

struct QVFBHeader* qvfb_hdr;

static int a_getinfo (struct shadowlcd_info* lcd_info)
{
    lcd_info->width = qvfb_hdr->width;
    lcd_info->height = qvfb_hdr->height;
    lcd_info->bpp = qvfb_hdr->depth;
    if (qvfb_hdr->depth >= 8)
        lcd_info->fb = ((BYTE*)qvfb_hdr) + qvfb_hdr->dataoffset;
    else
        lcd_info->fb = NULL;
    lcd_info->rlen = qvfb_hdr->linestep;
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
            qvfb_hdr->clut [entry/255] = 
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
            qvfb_hdr->clut [entry/16] = 
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
            unsigned int r = colors[i].r & 0xff;
            unsigned int g = colors[i].g & 0xff;
            unsigned int b = colors[i].b & 0xff;
            qvfb_hdr->clut [entry] = (0xff << 24) | (r << 16) | (g << 8) | (b & 0xff);
            set++;
            entry++;
        }
    }

    return set;
}

static int a_init (void)
{
    int x_display;
    char file [50];
    key_t key;
    int shmid;

    if (GetMgEtcIntValue ("qvfb", "display", &x_display) < 0)
        x_display = 0;

    sprintf (file, QT_VFB_MOUSE_PIPE, x_display);
    key = ftok (file, 'b');

    shmid = shmget (key, 0, 0);
    if (shmid == -1)
        return 2;

    qvfb_hdr = (struct QVFBHeader *) shmat (shmid, 0, 0);
    if ((int) qvfb_hdr == -1 || qvfb_hdr == NULL)
        return 3;

    switch (qvfb_hdr->depth) {
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

    SetRect (&qvfb_hdr->update, 0, 0, 0, 0);
    qvfb_hdr->dirty = FALSE;
    return 0;
}

static int a_release (void)
{
    shmdt (qvfb_hdr);
    return 0;
}

static void a_sleep (void)
{
    /* 20ms */
    usleep (20000);
}

static unsigned char pixel_bit [] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};

static void qvfb_update_1bpp (_THIS)
{
    RECT bound = qvfb_hdr->update;
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

    dst_bits = ((BYTE*) qvfb_hdr) + qvfb_hdr->dataoffset;
    dst_bits += bound.top * qvfb_hdr->linestep + (bound.left >> 3);

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
        dst_bits += qvfb_hdr->linestep;
    }
}

static void qvfb_update_4bpp (_THIS)
{
    RECT bound = qvfb_hdr->update;
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

    dst_bits = (BYTE*) qvfb_hdr + qvfb_hdr->dataoffset;
    dst_bits += bound.top * qvfb_hdr->linestep + (bound.left >> 1);

    for (y = 0; y < height; y++) {
        const BYTE* src_line = src_bits;
        BYTE* dst_line = dst_bits;
        for (x = 0; x < width; x+=2) {
            *dst_line = (*src_line >> 4) | (*(++src_line) & 0xF0);
            src_line ++;
            dst_line ++;
        }
        src_bits += this->hidden->pitch;
        dst_bits += qvfb_hdr->linestep;
    }
}

static void a_refresh (_THIS, const RECT* update)
{
    RECT bound;

    bound = qvfb_hdr->update;

    if (bound.right < 0) bound.right = 0;
    if (bound.bottom < 0) bound.bottom = 0;

    GetBoundRect (&bound, &bound, update);

    qvfb_hdr->update = bound;

     /* Copy the bits from Shadow FrameBuffer to QVFB FrameBuffer */
    if (qvfb_hdr->depth == 1)
        qvfb_update_1bpp (this);
    else if (qvfb_hdr->depth == 4)
        qvfb_update_4bpp (this);

    qvfb_hdr->dirty = TRUE;
}

struct shadow_lcd_ops __mg_shadow_lcd_ops = {
    a_init,
    a_getinfo,
    a_release,
    NULL,
    a_sleep,
    a_refresh
};

#endif /* __LINUX__ && __TARGET_QVFB__ */

#endif /* _NEWGAL_ENGINE_SHADOW */

