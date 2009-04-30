/*
** $Id: drawtext.c 8049 2007-11-06 06:20:31Z wangxuguang $
** 
** drawtext.c: Low level text drawing.
** 
** Copyright (C) 2003 ~ 2007 Feynman Software.
** Copyright (C) 1999 ~ 2002 WEI Yongming.
**
** All right reserved by Feynman Software.
**
** Current maintainer: WEI Yongming.
**
** Create date: 2000/06/15
*/

/*
** TODO:
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "common.h"
#include "minigui.h"
#include "gdi.h"
#include "window.h"
#include "devfont.h"
#include "cliprect.h"
#include "gal.h"
#include "internals.h"
#include "ctrlclass.h"
#include "dc.h"
#include "pixel_ops.h"
#include "cursor.h"
#include "drawtext.h"

#define EDGE_ALPHA      64
#define ALL_WGHT        32
#define ROUND_WGHT      16

#define NEIGHBOR_WGHT    3
#define NEAR_WGHT        1

#ifdef _FT2_SUPPORT
int ft2GetLcdFilter (DEVFONT* devfont);
int ft2IsFreeTypeDevfont (DEVFONT* devfont);
#endif

static BITMAP char_bmp;
static size_t char_bits_size;

BOOL InitTextBitmapBuffer (void)
{
    char_bmp.bmBits = NULL;
    return TRUE;
}

void TermTextBitmapBuffer (void)
{
    free (char_bmp.bmBits);
    char_bmp.bmBits = NULL;

    char_bits_size = 0;
}

static void prepare_bitmap (PDC pdc, int w, int h)
{
    Uint32 size;
    size = GAL_GetBoxSize (pdc->surface, w + 2, h + 3, &char_bmp.bmPitch);
    char_bmp.bmType = 0;
    char_bmp.bmBitsPerPixel = pdc->surface->format->BitsPerPixel;
    char_bmp.bmBytesPerPixel = pdc->surface->format->BytesPerPixel;
    char_bmp.bmWidth = w;
    char_bmp.bmHeight = h;
    char_bmp.bmAlphaPixelFormat = NULL;

    if (size <= char_bits_size) {
        return;
    }
    char_bits_size = ((size + 31) >> 5) << 5;
    
    char_bmp.bmBits = realloc(char_bmp.bmBits, char_bits_size);

    /*
    if (char_bmp.bmBits != NULL)
    {
        free(char_bmp.bmBits);
    }
    char_bmp.bmBits = malloc(char_bits_size);
    */
}

int gdi_get_italic_added_width(LOGFONT* logfont)
{
    DEVFONT* sbc_devfont = logfont->sbc_devfont;
    DEVFONT* mbc_devfont = logfont->mbc_devfont;
    int sbc_height = 0;
    int mbc_height = 0;

    if (logfont->style & FS_SLANT_ITALIC
        && !(sbc_devfont->style & FS_SLANT_ITALIC)) 
    {
        sbc_height = sbc_devfont->font_ops->get_font_height(logfont, sbc_devfont);
        if (mbc_devfont) 
        {
            mbc_height = mbc_devfont->font_ops->get_font_height(logfont, mbc_devfont);
        }

        return (MAX(sbc_height, mbc_height) + 1) >> 1;
    }
    else
    {
        return 0;
    }

}

void gdi_start_new_line (PDC pdc)
{
    DEVFONT* sbc_devfont = pdc->pLogFont->sbc_devfont;
    DEVFONT* mbc_devfont = pdc->pLogFont->mbc_devfont;

    if (mbc_devfont) {
        if (mbc_devfont->font_ops->start_str_output)
        {
            (*mbc_devfont->font_ops->start_str_output) (pdc->pLogFont, 
                                                        mbc_devfont);
        }
    }
    if (sbc_devfont->font_ops->start_str_output)
    {
            (*sbc_devfont->font_ops->start_str_output) (pdc->pLogFont, 
                                                        sbc_devfont);
    }
}


static Uint32 allocated_size = 0;
static BYTE* filtered_bits;

static BYTE* do_low_pass_filtering (PDC pdc)
{
    int x, y;
    int bpp = GAL_BytesPerPixel (pdc->surface);
    BYTE *curr_sline, *prev_sline, *next_sline;
    BYTE *dst_line, *dst_pixel;

    if (allocated_size < char_bits_size) {

        filtered_bits = realloc(filtered_bits, char_bits_size);
        allocated_size = char_bits_size;
        /*
        free(filtered_bits);
        filtered_bits = malloc (char_bits_size);
        allocated_size = char_bits_size;
        */
    }

    if (filtered_bits == NULL)
    {
        return NULL;
    }

    curr_sline = char_bmp.bmBits; 
    next_sline = curr_sline + char_bmp.bmPitch;
    dst_line = filtered_bits;
    dst_pixel = dst_line;;

    /* For the first line, set the background and foreground color directly. */
    for (x = 0; x < char_bmp.bmWidth; x++) {
        int weight = 0;

        if (curr_sline [x]) {
            weight = 16;
        }
        else {
            if (x > 0) {
                if (curr_sline [x - 1]) weight += 3;
                if (next_sline [x - 1]) weight += 1;
            }
            if (x < char_bmp.bmWidth - 1) {
                if (curr_sline [x + 1]) weight += 3;
                if (next_sline [x + 1]) weight += 1;
            }
            if (next_sline [x]) weight += 3;
        }

        dst_pixel = _mem_set_pixel (dst_pixel, 
                        bpp, pdc->filter_pixels [weight]);
    }

    prev_sline = curr_sline;
    curr_sline = curr_sline + char_bmp.bmPitch;
    next_sline = curr_sline + char_bmp.bmPitch;
    dst_line += char_bmp.bmPitch;

    if (char_bmp.bmHeight > 2)
    for (y = 1; y < char_bmp.bmHeight - 1; y++) {
        int weight;

        dst_pixel = dst_line;

        for (x = 0; x < char_bmp.bmWidth; x++) {
            weight = 0;

            /*
             * For the first pixel per line, 
             * set the background and foreground color directly.
             */ 
            if (curr_sline [x]) {
                weight = 16;
            }
            else {
                if (x == 0) {
                    if (curr_sline [x + 1]) weight += 3;

                    if (prev_sline [x]) weight += 3;
                    if (prev_sline [x + 1]) weight += 1;

                    if (next_sline [x]) weight += 3;
                    if (next_sline [x + 1]) weight += 1;
                }
                else if (x == (char_bmp.bmWidth - 1)) {
                    if (curr_sline [x - 1]) weight += 3;

                    if (prev_sline [x - 1]) weight += 1;
                    if (prev_sline [x]) weight += 3;

                    if (next_sline [x - 1]) weight += 1;
                    if (next_sline [x]) weight += 3;
                }
                else {
                    if (curr_sline [x - 1]) weight += 3;
                    if (curr_sline [x + 1]) weight += 3;

                    if (prev_sline [x - 1]) weight += 1;
                    if (prev_sline [x]) weight += 3;
                    if (prev_sline [x + 1]) weight += 1;

                    if (next_sline [x - 1]) weight += 1;
                    if (next_sline [x]) weight += 3;
                    if (next_sline [x + 1]) weight += 1;
                }
            }

            /* set destination pixel according to the weight */
            dst_pixel = _mem_set_pixel (dst_pixel, 
                            bpp, pdc->filter_pixels [weight]);
        }

        prev_sline += char_bmp.bmPitch;
        curr_sline += char_bmp.bmPitch;
        next_sline += char_bmp.bmPitch;
        dst_line += char_bmp.bmPitch;
    }

    dst_pixel = dst_line;
    /* For the last line, set the background and foreground color directly. */
    for (x = 0; x < char_bmp.bmWidth; x++) {
        int weight = 0;

        if (curr_sline [x]) {
            weight = 16;
        }
        else {
            if (x > 0) {
                if (prev_sline [x - 1]) weight += 1;
                if (curr_sline [x - 1]) weight += 3;
            }
            if (x < char_bmp.bmWidth - 1) {
                if (prev_sline [x + 1]) weight += 1;
                if (curr_sline [x + 1]) weight += 3;
            }
            if (prev_sline [x]) weight += 3;
        }

        dst_pixel = _mem_set_pixel (dst_pixel, 
                        bpp, pdc->filter_pixels [weight]);
    }

    return filtered_bits;
}

static BYTE* do_inflate_filtering (PDC pdc)
{
    int x, y;
    int bpp = GAL_BytesPerPixel (pdc->surface);
    BYTE *curr_sline, *prev_sline, *next_sline;
    BYTE *dst_line, *dst_pixel;
    gal_pixel pixel;

    if (allocated_size < char_bits_size) {
        free(filtered_bits);
        filtered_bits = malloc (char_bits_size);
        allocated_size = char_bits_size;
    }

    if (filtered_bits == NULL)
    {
        return NULL;
    }

    curr_sline = char_bmp.bmBits;
    next_sline = curr_sline + char_bmp.bmPitch;
    dst_line = filtered_bits;
    dst_pixel = dst_line;;

    for (x = 0; x < char_bmp.bmWidth; x++) {
        if (curr_sline [x]) {
            pixel = pdc->filter_pixels [2];
        }
        else if ((x > 0 && curr_sline [x - 1]) || 
                        (x < char_bmp.bmWidth - 1 && curr_sline [x + 1]) || 
                        next_sline [x])
            pixel = pdc->filter_pixels [1];
        else
            pixel = pdc->filter_pixels [0];

        dst_pixel = _mem_set_pixel (dst_pixel, bpp, pixel);
    }

    prev_sline = curr_sline;
    curr_sline = curr_sline + char_bmp.bmPitch;
    next_sline = curr_sline + char_bmp.bmPitch;
    dst_line += char_bmp.bmPitch;

    for (y = 1; y < char_bmp.bmHeight - 1; y++) {

        dst_pixel = dst_line;
        for (x = 0; x < char_bmp.bmWidth; x++) {
            if (curr_sline [x]) {
                pixel = pdc->filter_pixels [2];
            }
            else if ((x > 0 && curr_sline [x - 1]) || 
                            (x < char_bmp.bmWidth - 1 && curr_sline [x + 1]) ||
                            prev_sline [x] || next_sline [x]) {
                pixel = pdc->filter_pixels [1];
            }
            else
                pixel = pdc->filter_pixels [0];

            dst_pixel = _mem_set_pixel (dst_pixel, bpp, pixel);
        }

        prev_sline += char_bmp.bmPitch;
        curr_sline += char_bmp.bmPitch;
        next_sline += char_bmp.bmPitch;
        dst_line += char_bmp.bmPitch;
    }

    dst_pixel = dst_line;
    for (x = 0; x < char_bmp.bmWidth; x++) {
        if (curr_sline [x]) {
            pixel = pdc->filter_pixels [2];
        }
        else if ((x > 0 && curr_sline [x - 1]) || 
                        (x < char_bmp.bmWidth - 1 && curr_sline [x + 1]) || 
                        prev_sline [x])
            pixel = pdc->filter_pixels [1];
        else
            pixel = pdc->filter_pixels [0];

        dst_pixel = _mem_set_pixel (dst_pixel, bpp, pixel);
    }

    return filtered_bits;
}

static void expand_char_pixmap (PDC pdc, int w, int h, const BYTE* bits, 
            BYTE* expanded, int bold, int italic, int cols, 
            unsigned short scale)
{
    int x, y, w_loop;
    int b = 0;
    BYTE* line;
    unsigned short s_loop;

    w_loop = w / scale;
    line = expanded;
    switch (GAL_BytesPerPixel (pdc->surface)) {
    case 1:
        for (y = 0; y < h; y += scale) {
            Uint8* dst = line;
            memset (dst , pdc->gray_pixels [0], w + bold + italic);

            if (italic)
                dst += (h - y) >> 1;

            for (x = 0; x < w_loop; x++) {

                b = *(bits+x);
                if (b == 255) b = 16;
                else b >>= 4;

                memset (dst, pdc->gray_pixels [b], scale);
                dst += scale;

                if (bold)
                    *dst = pdc->gray_pixels [b];
            }

            for (s_loop = 0; s_loop < (scale - 1); s_loop++) {
                memcpy (line + char_bmp.bmPitch, line, char_bmp.bmPitch);
                line += char_bmp.bmPitch;
            }
            line += char_bmp.bmPitch;

            bits += cols;
        }
    break;

    case 2:
        for (y = 0; y < h; y += scale) {
            Uint16* dst = (Uint16*) line;
            for (x = 0; x < (w + bold + italic); x ++) {
                dst [x] = pdc->gray_pixels [0];
            }
            
            if (italic)
                dst += ((h - y) >> 1);

            for (x = 0; x < w_loop; x++) {


                b = *(bits+x);
                
                if (b == 255) b = 16;
                else b >>= 4;

                for (s_loop = 0; s_loop < scale; s_loop ++) {
                    *dst = pdc->gray_pixels [b];
                    dst ++;
                }

                if (bold)
                    *dst = pdc->gray_pixels [b];
            }

            for (s_loop = 0; s_loop < (scale - 1); s_loop++) {
                memcpy (line + char_bmp.bmPitch, line, char_bmp.bmPitch);
                line += char_bmp.bmPitch;
            }
            line += char_bmp.bmPitch;

            bits += cols;
        }
    break;

    case 3:
        for (y = 0; y < h; y += scale) {
            Uint8* expanded = line;
            for (x = 0; x < (w + bold + italic) * 3; x += 3) {
                *(Uint16 *) (expanded + x) = pdc->gray_pixels [0];
                *(expanded + x + 2) = pdc->gray_pixels [0] >> 16;
            }

            if (italic)
                expanded += ((h - y) >> 1) * 3;

            for (x = 0; x < w_loop; x++) {
                b = *(bits+x);
                if (b == 255) b = 16;
                else b >>= 4;

                for (s_loop = 0; s_loop < scale; s_loop ++) {
                    *(Uint16 *) expanded = pdc->gray_pixels[b];
                    *(expanded + 2) = pdc->gray_pixels[b] >> 16;
                    expanded += 3;
                }

                if (bold) {
                    *(Uint16 *)expanded = pdc->gray_pixels[b];
                    *(expanded + 2) = pdc->gray_pixels[b] >> 16;
                }
                
            }

            for (s_loop = 0; s_loop < (scale - 1); s_loop++) {
                memcpy (line + char_bmp.bmPitch, line, char_bmp.bmPitch);
                line += char_bmp.bmPitch;
            }
            line += char_bmp.bmPitch;

            bits += cols;
        }
    break;

    case 4:
        for (y = 0; y < h; y += scale) {
            Uint32* dst = (Uint32*)line;

            for (x = 0; x < (w + bold + italic); x ++) {
                dst [x] = pdc->gray_pixels [0];
            }

            if (italic)
                dst += ((h - y) >> 1);

            for (x = 0; x < w_loop; x++) {

                b = *(bits+x);
                if (b == 255) b = 16;
                else b >>= 4;

                for (s_loop = 0; s_loop < scale; s_loop ++) {
                    *dst = pdc->gray_pixels [b];
                    dst ++;
                }
                if (bold)
                    *dst = pdc->gray_pixels[b];
            }

            for (s_loop = 0; s_loop < (scale - 1); s_loop++) {
                memcpy (line + char_bmp.bmPitch, line, char_bmp.bmPitch);
                line += char_bmp.bmPitch;
            }
            line += char_bmp.bmPitch;

            bits += cols;
        }
    }
}

static void expand_char_bitmap (int w, int h, 
            const BYTE* bits, int bpp, BYTE* expanded, 
            int bg, int fg, int bold, int italic)
{
    int x, y;
    int b = 0;
    BYTE* line;

    line = expanded;
    switch (bpp) {
    case 1:
        for (y = 0; y < h; y++) {
            expanded = line;
            for (x = 0; x < (w + bold + italic); x++) {
                *(expanded + x) = bg;
            }

            if (italic)
                expanded += (h - y) >> 1;
            for (x = 0; x < w; x++) {
                if (x % 8 == 0)
                    b = *bits++;
                if ((b & (128 >> (x % 8)))) {
                    *expanded = fg;
                    if (bold)
                        *(expanded + 1) = fg;
                }

                expanded++;
            }
            line += char_bmp.bmPitch;
        }
    break;

    case 2:
        for (y = 0; y < h; y++) {
            expanded = line;
            for (x = 0; x < (w + bold + italic) << 1; x += 2) {
                *(Uint16 *) (expanded + x) = bg;
            }
            
            if (italic)
                expanded += ((h - y) >> 1) << 1;
            for (x = 0; x < w; x++) {
                if (x % 8 == 0)
                    b = *bits++;
                if ((b & (128 >> (x % 8)))) {
                    *(Uint16 *) expanded = fg;
                    if (bold)
                        *(Uint16 *)(expanded + 2) = fg;
                }

                expanded += 2;
            }
            line += char_bmp.bmPitch;
        }
    break;

    case 3:
        for (y = 0; y < h; y++) {
            expanded = line;
            for (x = 0; x < (w + bold + italic) * 3; x += 3) {
                *(Uint16 *) (expanded + x) = bg;
                *(expanded + x + 2) = bg >> 16;
            }

            if (italic)
                expanded += 3 * ((h - y) >> 1);

            for (x = 0; x < w; x++) {
                if (x % 8 == 0)
                    b = *bits++;
                if ((b & (128 >> (x % 8)))) {
                    *(Uint16 *) expanded = fg;
                    *(expanded + 2) = fg >> 16;
                    if (bold) {
                        *(Uint16 *)(expanded + 3) = fg;
                        *(expanded + 5) = fg >> 16;
                    }
                }
                
                expanded += 3;
            }
            line += char_bmp.bmPitch;
        }
    break;

    case 4:
        for (y = 0; y < h; y++) {
            expanded = line;
            for (x = 0; x < (w + bold + italic) << 2; x += 4) {
                *(Uint32 *) (expanded + x)= bg;
            }

            if (italic)
                expanded += ((h - y) >> 1) << 2;

            for (x = 0; x < w; x++) {
                if (x % 8 == 0)
                    b = *bits++;
                if ((b & (128 >> (x % 8)))) {
                    *(Uint32 *) expanded = fg;
                    if (bold)
                        *(Uint32 *) (expanded + 4) = fg;
                }

                expanded += 4;
            }
            line += char_bmp.bmPitch;
        }
    }
}

static void expand_char_bitmap_scale (int w, int h, 
            const BYTE* bits, int bpp, BYTE* expanded, 
            int bg, int fg, int bold, int italic, unsigned short scale)
{
    int x, y, w_loop, b = 0;
    unsigned short s_loop;
    BYTE* line;

    w_loop = w / scale;
    line = expanded;
    switch (bpp) {
    case 1:
        for (y = 0; y < h; y+=scale) {
            expanded = line;

            memset (expanded, bg, w + bold + italic);

            if (italic)
                expanded += (h - y) >> 1;

            for (x = 0; x < w_loop; x++) {
                if (x % 8 == 0)
                    b = *bits++;

                if ((b & (128 >> (x % 8)))) {
                    memset (expanded, fg, scale);
                    expanded += scale;

                    if (bold)
                        *expanded = fg;
                }
                else
                    expanded += scale;
            }
            for (s_loop = 0; s_loop < (scale - 1); s_loop++) {
                memcpy (line + char_bmp.bmPitch, line, char_bmp.bmPitch);
                line += char_bmp.bmPitch;
            }
            line += char_bmp.bmPitch;
        }
    break;

    case 2:
        for (y = 0; y < h; y+=scale) {
            expanded = line;

            for (x = 0; x < (w + bold + italic) << 1; x += 2) {
                *(Uint16 *) (expanded + x) = bg;
            }

            if (italic)
                expanded += ((h - y) >> 1) << 1;

            for (x = 0; x < w_loop; x++) {
                if (x % 8 == 0)
                    b = *bits++;
                if ((b & (128 >> (x % 8)))) {
                    for (s_loop = 0; s_loop < scale; s_loop++) {
                        *(Uint16 *) expanded = fg;
                        expanded += 2;
                    }
                    if (bold)
                        *(Uint16 *)expanded = fg;
                }
                else
                    expanded += 2 * scale;
            }

            for (s_loop = 0; s_loop < (scale - 1); s_loop++) {
                memcpy (line + char_bmp.bmPitch, line, char_bmp.bmPitch);
                line += char_bmp.bmPitch;
            }
            line += char_bmp.bmPitch;
        }
    break;

    case 3:
        for (y = 0; y < h; y+=scale) {
            expanded = line;
            for (x = 0; x < (w + bold + italic) * 3; x += 3) {
                *(Uint16 *) (expanded + x) = bg;
                *(expanded + x + 2) = bg >> 16;
            }

            if (italic)
                expanded += 3 * ((h - y) >> 1);

            for (x = 0; x < w_loop; x++) {
                if (x % 8 == 0)
                    b = *bits++;
                if ((b & (128 >> (x % 8)))) {
                    for (s_loop = 0; s_loop < scale; s_loop++) {
                        *(Uint16 *) expanded = fg;
                        *(expanded + 2) = fg >> 16;
                        expanded += 3;
                    }
                    if (bold) {
                        *(Uint16 *)expanded = fg;
                        *(expanded + 2) = fg >> 16;
                    }
                }
                else
                    expanded += 3 * scale;
            }
            for (s_loop = 0; s_loop < (scale - 1); s_loop++) {
                memcpy (line + char_bmp.bmPitch, line, char_bmp.bmPitch);
                line += char_bmp.bmPitch;
            }
            line += char_bmp.bmPitch;
        }
    break;

    case 4:
        for (y = 0; y < h; y+=scale) {
            expanded = line;
            for (x = 0; x < (w + bold + italic) << 2; x += 4) {
                *(Uint32 *) (expanded + x)= bg;
            }

            if (italic)
                expanded += ((h - y) >> 1) << 2;

            for (x = 0; x < w_loop; x++) {

                if (x % 8 == 0)
                    b = *bits++;

                if ((b & (128 >> (x % 8)))) {
                    for (s_loop = 0; s_loop < scale; s_loop++) {
                        *(Uint32 *) expanded = fg;
                        expanded += 4;
                    }

                    if (bold)
                        *(Uint32 *) expanded = fg;
                }
                else {
                    expanded += 4 * scale;
                }
            }

            for (s_loop = 0; s_loop < (scale - 1); s_loop++) {
                memcpy (line + char_bmp.bmPitch, line, char_bmp.bmPitch);
                line += char_bmp.bmPitch;
            }
            line += char_bmp.bmPitch;
        }
    }
}


static void expand_char_bitmap_to_rgba_scale (int w, int h, 
            const BYTE* bits, RGB* expanded, 
            RGB fg, int bold, int italic, unsigned short scale)
{
    int x; 
    int y; 
    int font_w;
    int font_h;
    int font_x;
    int font_y;
    int italic_blank = 0;

    int b;

    RGB* line_start_rgba;
    BYTE* line_head_bits;

    font_w = (w-italic-bold) / scale;
    font_h = h / scale;

    /*expand font_h line*/
    for (font_y = 0; font_y < font_h; font_y++) {
        
        y = font_y * scale;
        line_head_bits = (BYTE*)bits + ((font_w+7) >> 3 ) * font_y;
        
 
        /*expand a font line*/
        for ( ; y < (font_y+1)* scale; y++) {
            if (italic)
            {
                italic_blank = (h - y) >> 1;
            }

            line_start_rgba = expanded + y * w + italic_blank;

            /*expand a font point*/
            for (font_x = 0; font_x < font_w; font_x++) {

                b = line_head_bits[font_x/8];

                if ((b & (128 >> (font_x % 8)))) {
                    /*a font point => scale bmp point*/
                    for (x = font_x*scale; x < (font_x+1)*scale; x++) {
                        line_start_rgba[x] = fg;
                    }

                    if (bold)
                        line_start_rgba[x] = fg;
                }
                else
                {
                }
            }
        }
    }
}

#ifdef ADJUST_SUBPEXIL_WIEGHT
static void do_subpixel_line(RGB* dist_buf, RGB* pre_line, 
        RGB* cur_line, RGB* next_line, int w, RGB rgba_fg)
{
    int x;
    int weight;
    Uint32 sub_val;

    if (cur_line[0].a == 0)
    {
        weight = 0;
        if (cur_line[1].a != 0)
            weight += NEIGHBOR_WGHT;
        if (pre_line[0].a != 0)
            weight += NEIGHBOR_WGHT;
        if (next_line[0].a != 0)
            weight += NEIGHBOR_WGHT;

        if (pre_line[1].a != 0)
            weight += NEAR_WGHT;
        if (next_line[1].a != 0)
            weight += NEAR_WGHT;

            sub_val = (cur_line[0].r * (ALL_WGHT-weight) + rgba_fg.r * weight)/ALL_WGHT;
            dist_buf[0].r = sub_val;

            sub_val = (cur_line[0].g * (ALL_WGHT-weight) + rgba_fg.g * weight)/ALL_WGHT;
            dist_buf[0].g = sub_val;

            sub_val = (cur_line[0].b * (ALL_WGHT-weight) + rgba_fg.b * weight)/ALL_WGHT;
            dist_buf[0].b = sub_val;

            dist_buf[0].a = 255;

    }
    else
    {
        sub_val = cur_line[0].r * (ALL_WGHT-ROUND_WGHT+ NEIGHBOR_WGHT + 2*NEAR_WGHT);
        sub_val += cur_line[1].r * NEIGHBOR_WGHT;

        sub_val += pre_line[0].r * NEIGHBOR_WGHT;
        sub_val += next_line[0].r * NEIGHBOR_WGHT;

        sub_val += pre_line[1].r * NEAR_WGHT;
        sub_val += next_line[1].r * NEAR_WGHT;

        dist_buf[0].r = sub_val/ALL_WGHT;


        sub_val = cur_line[0].g * (ALL_WGHT-ROUND_WGHT+ NEIGHBOR_WGHT + 2*NEAR_WGHT);
        sub_val += cur_line[1].g * NEIGHBOR_WGHT;
        sub_val += pre_line[0].g * NEIGHBOR_WGHT;
        sub_val += next_line[0].g * NEIGHBOR_WGHT;

        sub_val += pre_line[1].g * NEAR_WGHT;
        sub_val += next_line[1].g * NEAR_WGHT;

        dist_buf[0].g = sub_val/ALL_WGHT;

        sub_val = cur_line[0].b * (ALL_WGHT-ROUND_WGHT+ NEIGHBOR_WGHT + 2*NEAR_WGHT);
        sub_val += cur_line[1].b * NEIGHBOR_WGHT;
        sub_val += pre_line[0].b * NEIGHBOR_WGHT;
        sub_val += next_line[0].b * NEIGHBOR_WGHT;

        sub_val += pre_line[1].b * NEAR_WGHT;
        sub_val += next_line[1].b * NEAR_WGHT;

        dist_buf[0].b = sub_val/ALL_WGHT;

        dist_buf[0].a = 255;
    }

    for (x=1; x<w-1; x++)
    {
        weight = 0;
        if (cur_line[x].a == 0) 
        {
            if (cur_line[x-1].a != 0)
                weight += NEIGHBOR_WGHT;
            if (cur_line[x+1].a != 0)
                weight += NEIGHBOR_WGHT;
            if (pre_line[ x ].a != 0)
                weight += NEIGHBOR_WGHT;
            if (next_line[ x ].a != 0)
                weight += NEIGHBOR_WGHT;

            if (pre_line[x-1].a != 0)
                weight += NEAR_WGHT;
            if (pre_line[x+1].a != 0)
                weight += NEAR_WGHT;
            if (next_line[x-1].a != 0)
                weight += NEAR_WGHT;
            if (next_line[x+1].a != 0)
                weight += NEAR_WGHT;

                sub_val = (cur_line[x].r * (ALL_WGHT-weight) + rgba_fg.r * weight)/ALL_WGHT;
                dist_buf[x].r = sub_val;

                sub_val = (cur_line[x].g * (ALL_WGHT-weight) + rgba_fg.g * weight)/ALL_WGHT;
                dist_buf[x].g = sub_val;

                sub_val = (cur_line[x].b * (ALL_WGHT-weight) + rgba_fg.b * weight)/ALL_WGHT;
                dist_buf[x].b = sub_val;

                dist_buf[x].a = 255;

        }
        else
        {
            sub_val = cur_line[ x ].r * (ALL_WGHT-ROUND_WGHT);
            
            sub_val += cur_line[x-1].r * NEIGHBOR_WGHT;
            sub_val += cur_line[x+1].r * NEIGHBOR_WGHT;
            sub_val += pre_line[ x ].r * NEIGHBOR_WGHT;
            sub_val += next_line[ x ].r * NEIGHBOR_WGHT;


            sub_val += pre_line[x-1].r * NEAR_WGHT;
            sub_val += next_line[x-1].r * NEAR_WGHT;
            sub_val += pre_line[x+1].r * NEAR_WGHT;
            sub_val += next_line[x+1].r * NEAR_WGHT;

            dist_buf[x].r = sub_val/ALL_WGHT;


            sub_val = cur_line[ x ].g * (ALL_WGHT-ROUND_WGHT);
            
            sub_val += cur_line[x-1].g * NEIGHBOR_WGHT;
            sub_val += cur_line[x+1].g * NEIGHBOR_WGHT;
            sub_val += pre_line[ x ].g * NEIGHBOR_WGHT;
            sub_val += next_line[ x ].g * NEIGHBOR_WGHT;


            sub_val += pre_line[x-1].g * NEAR_WGHT;
            sub_val += next_line[x-1].g * NEAR_WGHT;
            sub_val += pre_line[x+1].g * NEAR_WGHT;
            sub_val += next_line[x+1].g * NEAR_WGHT;

            dist_buf[x].g = sub_val/ALL_WGHT;


            sub_val = cur_line[ x ].b * (ALL_WGHT-ROUND_WGHT);
            
            sub_val += cur_line[x-1].b * NEIGHBOR_WGHT;
            sub_val += cur_line[x+1].b * NEIGHBOR_WGHT;
            sub_val += pre_line[ x ].b * NEIGHBOR_WGHT;
            sub_val += next_line[ x ].b * NEIGHBOR_WGHT;


            sub_val += pre_line[x-1].b * NEAR_WGHT;
            sub_val += next_line[x-1].b * NEAR_WGHT;
            sub_val += pre_line[x+1].b * NEAR_WGHT;
            sub_val += next_line[x+1].b * NEAR_WGHT;

            dist_buf[x].b = sub_val/ALL_WGHT;



            dist_buf[x].a = 255;
        }
    }


    weight = 0;
    if (cur_line[x].a == 0) 
    {
        if (cur_line[x-1].a != 0)
            weight += NEIGHBOR_WGHT;
        if (pre_line[ x ].a != 0)
            weight += NEIGHBOR_WGHT;
        if (next_line[ x ].a != 0)
            weight += NEIGHBOR_WGHT;

        if (pre_line[x-1].a != 0)
            weight += NEAR_WGHT;
        if (next_line[x-1].a != 0)
            weight += NEAR_WGHT;

            sub_val = (cur_line[x].r * (ALL_WGHT-weight) + rgba_fg.r * weight)/ALL_WGHT;
            dist_buf[x].r = sub_val;

            sub_val = (cur_line[x].g * (ALL_WGHT-weight) + rgba_fg.g * weight)/ALL_WGHT;
            dist_buf[x].g = sub_val;

            sub_val = (cur_line[x].b * (ALL_WGHT-weight) + rgba_fg.b * weight)/ALL_WGHT;
            dist_buf[x].b = sub_val;

            dist_buf[x].a = 255;

    }
    else
    {
        sub_val = cur_line[x].r * (ALL_WGHT-ROUND_WGHT+ NEIGHBOR_WGHT + 2*NEAR_WGHT);
        sub_val += cur_line[x-1].r * NEIGHBOR_WGHT;
        sub_val += pre_line[x].r * NEIGHBOR_WGHT;
        sub_val += next_line[x].r * NEIGHBOR_WGHT;

        sub_val += pre_line[x-1].r * NEAR_WGHT;
        sub_val += next_line[x-1].r * NEAR_WGHT;

        dist_buf[x].r = sub_val/ALL_WGHT;
        
        sub_val = cur_line[x].g * (ALL_WGHT-ROUND_WGHT+ NEIGHBOR_WGHT + 2*NEAR_WGHT);
        sub_val += cur_line[x-1].g * NEIGHBOR_WGHT;
        sub_val += pre_line[x].g * NEIGHBOR_WGHT;
        sub_val += next_line[x].g * NEIGHBOR_WGHT;

        sub_val += pre_line[x-1].g * NEAR_WGHT;
        sub_val += next_line[x-1].g * NEAR_WGHT;

        dist_buf[x].g = sub_val/ALL_WGHT;
        
        sub_val = cur_line[x].b * (ALL_WGHT-ROUND_WGHT+ NEIGHBOR_WGHT + 2*NEAR_WGHT);
        sub_val += cur_line[x-1].b * NEIGHBOR_WGHT;
        sub_val += pre_line[x].b * NEIGHBOR_WGHT;
        sub_val += next_line[x].b * NEIGHBOR_WGHT;

        sub_val += pre_line[x-1].b * NEAR_WGHT;
        sub_val += next_line[x-1].b * NEAR_WGHT;

        dist_buf[x].b = sub_val/ALL_WGHT;

        dist_buf[x].a = 255;
    }

}

#else /* ADJUST_SUBPEXIL_WIEGHT */

static void do_subpixel_line(RGB* dist_buf, RGB* pre_line, 
        RGB* cur_line, RGB* next_line, int w, RGB rgba_fg)
{
    int x;
    int weight;
    Uint32 sub_val;

    if (cur_line[0].a == 0)
    {
        weight = 0;
        if (cur_line[1].a != 0)
            weight += 3;
        if (pre_line[0].a != 0)
            weight += 3;
        if (next_line[0].a != 0)
            weight += 3;

        if (pre_line[1].a != 0)
            weight++;
        if (next_line[1].a != 0)
            weight++;

            sub_val = (cur_line[0].r * (32-weight) + rgba_fg.r * weight) >> 5;
            dist_buf[0].r = sub_val;

            sub_val = (cur_line[0].g * (32-weight) + rgba_fg.g * weight) >> 5;
            dist_buf[0].g = sub_val;

            sub_val = (cur_line[0].b * (32-weight) + rgba_fg.b * weight) >> 5;
            dist_buf[0].b = sub_val;

            dist_buf[0].a = 255;

    }
    else
    {
        sub_val = cur_line[0].r * 21; /* (32-16+ 3 + 2*1); */
        sub_val += cur_line[1].r * 3;

        sub_val += pre_line[0].r * 3;
        sub_val += next_line[0].r * 3;

        sub_val += pre_line[1].r;
        sub_val += next_line[1].r;

        dist_buf[0].r = sub_val >> 5;


        sub_val = cur_line[0].g * 21;/* (32-16+ 3 + 2*1); */
        sub_val += cur_line[1].g * 3;
        sub_val += pre_line[0].g * 3;
        sub_val += next_line[0].g * 3;

        sub_val += pre_line[1].g;
        sub_val += next_line[1].g;

        dist_buf[0].g = sub_val >> 5;

        sub_val = cur_line[0].b * 21; /* (32-16+ 3 + 2*1); */
        sub_val += cur_line[1].b * 3;
        sub_val += pre_line[0].b * 3;
        sub_val += next_line[0].b * 3;

        sub_val += pre_line[1].b;
        sub_val += next_line[1].b;

        dist_buf[0].b = sub_val >> 5;

        dist_buf[0].a = 255;
    }

    for (x=1; x<w-1; x++)
    {
        weight = 0;
        if (cur_line[x].a == 0) 
        {
            if (cur_line[x-1].a != 0)
                weight += 3;
            if (cur_line[x+1].a != 0)
                weight += 3;
            if (pre_line[ x ].a != 0)
                weight += 3;
            if (next_line[ x ].a != 0)
                weight += 3;

            if (pre_line[x-1].a != 0)
                weight ++;
            if (pre_line[x+1].a != 0)
                weight ++;
            if (next_line[x-1].a != 0)
                weight ++;
            if (next_line[x+1].a != 0)
                weight ++;

                sub_val = (cur_line[x].r * (32-weight) + rgba_fg.r * weight) >> 5;
                dist_buf[x].r = sub_val;

                sub_val = (cur_line[x].g * (32-weight) + rgba_fg.g * weight) >> 5;
                dist_buf[x].g = sub_val;

                sub_val = (cur_line[x].b * (32-weight) + rgba_fg.b * weight) >> 5;
                dist_buf[x].b = sub_val;

                dist_buf[x].a = 255;

        }
        else
        {
            sub_val = cur_line[ x ].r << 4;
            
            sub_val += cur_line[x-1].r * 3;
            sub_val += cur_line[x+1].r * 3;
            sub_val += pre_line[ x ].r * 3;
            sub_val += next_line[ x ].r * 3;


            sub_val += pre_line[x-1].r;
            sub_val += next_line[x-1].r;
            sub_val += pre_line[x+1].r;
            sub_val += next_line[x+1].r;

            dist_buf[x].r = sub_val >> 5;


            sub_val = cur_line[ x ].g << 4 ;
            
            sub_val += cur_line[x-1].g * 3;
            sub_val += cur_line[x+1].g * 3;
            sub_val += pre_line[ x ].g * 3;
            sub_val += next_line[ x ].g * 3;


            sub_val += pre_line[x-1].g;
            sub_val += next_line[x-1].g;
            sub_val += pre_line[x+1].g;
            sub_val += next_line[x+1].g;

            dist_buf[x].g = sub_val >> 5;


            sub_val = cur_line[ x ].b << 4;
            
            sub_val += cur_line[x-1].b * 3;
            sub_val += cur_line[x+1].b * 3;
            sub_val += pre_line[ x ].b * 3;
            sub_val += next_line[ x ].b * 3;


            sub_val += pre_line[x-1].b;
            sub_val += next_line[x-1].b;
            sub_val += pre_line[x+1].b;
            sub_val += next_line[x+1].b;

            dist_buf[x].b = sub_val >> 5;



            dist_buf[x].a = 255;
        }
    }


    weight = 0;
    if (cur_line[x].a == 0) 
    {
        if (cur_line[x-1].a != 0)
            weight += 3;
        if (pre_line[ x ].a != 0)
            weight += 3;
        if (next_line[ x ].a != 0)
            weight += 3;

        if (pre_line[x-1].a != 0)
            weight ++;
        if (next_line[x-1].a != 0)
            weight ++;

            sub_val = (cur_line[x].r * (32-weight) + rgba_fg.r * weight) >> 5;
            dist_buf[x].r = sub_val;

            sub_val = (cur_line[x].g * (32-weight) + rgba_fg.g * weight) >> 5;
            dist_buf[x].g = sub_val;

            sub_val = (cur_line[x].b * (32-weight) + rgba_fg.b * weight) >> 5;
            dist_buf[x].b = sub_val;

            dist_buf[x].a = 255;

    }
    else
    {
        sub_val = cur_line[x].r * 21; /* (32-16+ 3 + 2*1); */
        sub_val += cur_line[x-1].r * 3;
        sub_val += pre_line[x].r * 3;
        sub_val += next_line[x].r * 3;

        sub_val += pre_line[x-1].r;
        sub_val += next_line[x-1].r;

        dist_buf[x].r = sub_val >> 5;
        
        sub_val = cur_line[x].g * 21;
        sub_val += cur_line[x-1].g * 3;
        sub_val += pre_line[x].g * 3;
        sub_val += next_line[x].g * 3;

        sub_val += pre_line[x-1].g;
        sub_val += next_line[x-1].g;

        dist_buf[x].g = sub_val >> 5;
        
        sub_val = cur_line[x].b * 21;
        sub_val += cur_line[x-1].b * 3;
        sub_val += pre_line[x].b * 3;
        sub_val += next_line[x].b * 3;

        sub_val += pre_line[x-1].b;
        sub_val += next_line[x-1].b;

        dist_buf[x].b = sub_val >> 5;

        dist_buf[x].a = 255;
    }

}
#endif /* !ADJUST_SUBPEXIL_WIEGHT */

static void do_subpixel_filter (RGB* rgba_buf, int w, int h, RGB rgba_fg)
{
    RGB* cur_dist_buf = rgba_buf+ w * h;
    RGB* pre_dist_buf = cur_dist_buf + w;
    
    RGB* pre_src_line;
    RGB* cur_src_line;
    RGB* next_src_line;
    RGB* tmp;
    
    int y;

    /* the first line */
    pre_src_line = rgba_buf;
    cur_src_line = rgba_buf;
    next_src_line = rgba_buf + w;

    do_subpixel_line(pre_dist_buf, pre_src_line, cur_src_line, 
        next_src_line, w, rgba_fg);

    /* inner point */
    for (y=1; y<h-1; y++)
    {
        pre_src_line = cur_src_line;
        cur_src_line = next_src_line;
        next_src_line += w;
        
        do_subpixel_line(cur_dist_buf, pre_src_line, cur_src_line, 
            next_src_line, w, rgba_fg);
        
        /*save result of pre_line*/
        memcpy(pre_src_line, pre_dist_buf, w*4);

        /*move roll queue*/
        tmp = cur_dist_buf;
        cur_dist_buf = pre_dist_buf;
        pre_dist_buf = tmp;
    }

    /*cur_line:h-2=>h-1, next_line:h-1=>h-1*/
    pre_src_line = cur_src_line;
    cur_src_line = next_src_line;
    /*next_src_line is itself*/
    do_subpixel_line (cur_dist_buf, pre_src_line, cur_src_line, 
            next_src_line, w, rgba_fg);
    
    memcpy(pre_src_line, pre_dist_buf, w*4);
    memcpy(cur_src_line, cur_dist_buf, w*4);
}

#ifdef _FT2_SUPPORT
static void expand_subpixel_freetype (int w, int h, 
                const BYTE* ft_a_buf, int ft_a_pitch, 
                RGB* rgba_buf, RGB rgba_fg, int bold, int italic) 
{
    int x;
    int y;
    int font_w = w - italic;
    const BYTE* sub_a_cur;
    RGB*  rgba_cur;
    Uint32 sub_val;
    int fg_alpha;

    for (y=0; y<h; y++)
    {
        sub_a_cur = ft_a_buf + y * ft_a_pitch;
        
        rgba_cur = rgba_buf + y * w;
        if (italic)
        {
            rgba_cur += (h - y) / 2;
        }

        for (x=0; x<font_w; x++)
        {
#if 0
            sub_val = rgba_fg.r * *sub_a_cur + rgba_cur->r * (255-*sub_a_cur);
            rgba_cur->r = sub_val /255;
            sub_a_cur++;

            sub_val = rgba_fg.g * *sub_a_cur + rgba_cur->g * (255-*sub_a_cur);
            rgba_cur->g = sub_val /255;
            sub_a_cur++;

            sub_val = rgba_fg.b * *sub_a_cur + rgba_cur->b * (255-*sub_a_cur);
            rgba_cur->b = sub_val /255;
            sub_a_cur++;
#else
            fg_alpha = (*sub_a_cur++) + 1;
            sub_val = rgba_fg.r * fg_alpha + rgba_cur->r * (256-fg_alpha);
            rgba_cur->r = sub_val >> 8;

            fg_alpha = (*sub_a_cur++) + 1;
            sub_val = rgba_fg.g * fg_alpha + rgba_cur->g * (256-fg_alpha);
            rgba_cur->g = sub_val >> 8;

            fg_alpha = (*sub_a_cur++) + 1;
            sub_val = rgba_fg.b * fg_alpha + rgba_cur->b * (256-fg_alpha);
            rgba_cur->b = sub_val >> 8;
#endif

            rgba_cur->a = 255;

            if (bold) {
                /* FIXME FIXME */
            }

            rgba_cur++;
        }
    }
}

#endif /* _FT2_SUPPORT */

static void* expand_bkbmp_to_rgba (PDC pdc, DEVFONT* devfont,  
            BITMAP* bk_bmp, const BYTE* bits, 
            int pitch, int bold, int italic, unsigned short scale)
{
    static RGB* rgba_buf = NULL;
    static int    rgba_buf_size = 0;

    int w = bk_bmp->bmWidth;
    int h = bk_bmp->bmHeight;
    int x;
    int y;
    int bpp = bk_bmp->bmBytesPerPixel;
    int needed_rgba_buf_size = w * h * 4 + w*4*2;

    BYTE* src_pixel;
    RGB* dist_line_rgba;

    RGB* src_line_rgba;
    BYTE* dist_pixel;

    gal_pixel pixel;
    RGB rgba_fg;


    /*for char_bmp, and two line used by filter*/
    if (rgba_buf_size < needed_rgba_buf_size)
    {
        rgba_buf_size = needed_rgba_buf_size;
        rgba_buf = realloc(rgba_buf, rgba_buf_size);
        /*
           if (rgba_buf != NULL)
           free(rgba_buf);

           rgba_buf = malloc(rgba_buf_size);
         */
    }

    if (rgba_buf == NULL)
        return NULL;

    for (y=0; y<h; y++)
    {
        src_pixel = bk_bmp->bmBits + bk_bmp->bmPitch * y;
        dist_line_rgba = rgba_buf + w * y;

        for (x=0; x<w; x++)
        {
            pixel = _mem_get_pixel(src_pixel, bpp);

            GAL_GetRGBA (pixel, pdc->surface->format, &(dist_line_rgba[x].r),
                    &(dist_line_rgba[x].g), &(dist_line_rgba[x].b),
                    &(dist_line_rgba[x].a));

            dist_line_rgba[x].a = 0;

            src_pixel += bpp;
        }
    }

    /* expand char to rgba_buf; */
    GAL_GetRGBA (pdc->textcolor, pdc->surface->format, &(rgba_fg.r), 
            &(rgba_fg.g), &(rgba_fg.b), &(rgba_fg.a));
    rgba_fg.a = 255;

#ifdef _FT2_SUPPORT
    if (ft2IsFreeTypeDevfont (devfont) &&
            ft2GetLcdFilter (devfont) != 0) {
        expand_subpixel_freetype (w, h, bits, pitch, rgba_buf, rgba_fg, 
                bold, italic);
    }
    else 
#endif
    {
        expand_char_bitmap_to_rgba_scale (w, h, bits, rgba_buf, rgba_fg, 
                bold, italic, scale);
        /*draw rgba_fg to old rgba*/
        do_subpixel_filter (rgba_buf, w, h, rgba_fg);
    }
    
    /*change old_rgba_buf to dc format*/
    for (y=0; y<h; y++)
    {
        src_line_rgba = rgba_buf + w * y;
        dist_pixel = bk_bmp->bmBits + bk_bmp->bmPitch * y;

        for (x=0; x<w; x++)
        {
            pixel = GAL_MapRGB (pdc->surface->format, src_line_rgba[x].r,
                    src_line_rgba[x].g, src_line_rgba[x].b);
            dist_pixel = _mem_set_pixel(dist_pixel, bpp, pixel);
        }
    }

    return bk_bmp->bmBits;
}


#define FS_WEIGHT_BOOK_LIGHT    (FS_WEIGHT_BOOK | FS_WEIGHT_LIGHT)

/* return width of output */
static void put_one_char (PDC pdc, LOGFONT* logfont, DEVFONT* devfont,
                int* px, int* py, int h, int ascent, const char* mchar, int len)
{
    const BYTE* bits = NULL;
    int bbox_x = *px, bbox_y = *py;
    int bbox_w =0, bbox_h=0, bold = 0, italic = 0;
    int bpp, pitch = 0;
    GAL_Rect fg_rect;
    RECT rcOutput, rcTemp;
    gal_pixel bgcolor;
    unsigned short scale;
    BOOL pixmap = FALSE;
    BYTE* old_bits;
    gal_pixel fgcolor;

    if (devfont->font_ops->get_char_bbox) 
    {
        (*devfont->font_ops->get_char_bbox) (logfont, devfont,
                (const unsigned char*)mchar, len, 
                &bbox_x, &bbox_y, &bbox_w, &bbox_h);
    }
    else 
    {
        bbox_w = (*devfont->font_ops->get_char_width) (logfont, devfont, 
                (const unsigned char*)mchar, len);
        bbox_h = h;
        bbox_y -= ascent;
    }

    if (logfont->style & FS_WEIGHT_BOLD 
            && !(devfont->style & FS_WEIGHT_BOLD)) {
        bold = 1;
    }
    if (logfont->style & FS_SLANT_ITALIC
        && !(devfont->style & FS_SLANT_ITALIC)) {
        italic = (bbox_h - 1) >> 1;
    }

    if (devfont->font_ops->get_char_advance) {
        devfont->font_ops->get_char_advance (logfont, 
                devfont, (const unsigned char*)mchar, len, px, py);
        *px += bold;
    }
    else { 
        *px += bbox_w + bold;
    }

    /* FIXME SUB_PIXEL SMOOTH */
    if (logfont->style & FS_WEIGHT_SUBPIXEL) {
#ifdef _FT2_SUPPORT
        if (ft2IsFreeTypeDevfont (devfont) &&
                ft2GetLcdFilter (devfont) &&
                *devfont->font_ops->get_char_pixmap) {
            /* the returned bits will be the subpixled pixmap */
            bits = (*devfont->font_ops->get_char_pixmap) (logfont, devfont, 
                        (const unsigned char*)mchar, len, &pitch, &scale);
        }
#endif

        if (bits == NULL)
            bits = (*devfont->font_ops->get_char_bitmap) (logfont, devfont, 
                    (const unsigned char*)mchar, len, &scale);

        if (bits == NULL)
            return;

        pixmap = FALSE;
    }
    else {
        if (logfont->style & FS_WEIGHT_BOOK 
                && devfont->font_ops->get_char_pixmap) {
            bits = (*devfont->font_ops->get_char_pixmap) (logfont, devfont, 
                    (const unsigned char*)mchar, len, &pitch, &scale);

            if (bits)
                pixmap = TRUE;
        }

        if (!pixmap) {
            bits = (*devfont->font_ops->get_char_bitmap) (logfont, devfont, 
                    (const unsigned char*)mchar, len, &scale);
        }

        if (bits == NULL)
            return;
    }

    /*fg_rect: front ground rect (bounding box in back ground rect,
      have kerning(fg_rect_x))*/
    fg_rect.x = bbox_x;
    fg_rect.y = bbox_y;
    fg_rect.w = (bbox_w + bold + italic); 
    fg_rect.h = bbox_h;

    bpp = GAL_BytesPerPixel (pdc->surface);

    /*set char_bmp with pdc's attribute, 
      and relloc memory(to save bitmap image) */
    prepare_bitmap (pdc, (bbox_w + bold + italic), bbox_h);

    rcOutput.left = fg_rect.x;
    rcOutput.top = fg_rect.y;
    rcOutput.right = fg_rect.x + fg_rect.w;
    rcOutput.bottom = fg_rect.y + fg_rect.h;

    if (!IntersectRect (&rcOutput, &rcOutput, &pdc->rc_output))
        return;

    /* FIXME ADD SUB_PIXEL get bkground bmp */
    if (logfont->style & FS_WEIGHT_SUBPIXEL)
    {
        int offset = 0;
        if (logfont->style & FS_FLIP_VERT) {
            offset = -h/2 - fg_rect.h + (*py - bbox_y) * 2;
            fg_rect.y += offset;
        }

        rcTemp = pdc->rc_output;
        if (dc_IsMemDC (pdc)) {
            GetBitmapFromDC ((HDC)pdc, 
                fg_rect.x, fg_rect.y, fg_rect.w, fg_rect.h, &char_bmp);
        }
        else {
            GetBitmapFromDC (HDC_SCREEN, 
                fg_rect.x, fg_rect.y, fg_rect.w, fg_rect.h, &char_bmp);
        }
        pdc->rc_output = rcTemp;

        if (offset)
            fg_rect.y -= offset; 

        if (logfont->style & FS_FLIP_HORZ) {
            HFlipBitmap (&char_bmp, char_bmp.bmBits + 
                    char_bmp.bmPitch * char_bmp.bmHeight);
        }
        if (logfont->style & FS_FLIP_VERT) {
            VFlipBitmap (&char_bmp, char_bmp.bmBits + 
                    char_bmp.bmPitch * char_bmp.bmHeight);
        }
    }

    rcTemp = pdc->rc_output;
    pdc->rc_output = rcOutput;
    ENTER_DRAWING (pdc);

    pdc->step = 1;
    pdc->cur_ban = NULL;
    bgcolor = pdc->bkcolor;

    if (pixmap) {
        if (pdc->bkmode == BM_TRANSPARENT) {
            if (pdc->alpha_pixel_format && pdc->rop == ROP_SET) {
                char_bmp.bmType = BMP_TYPE_PRIV_PIXEL | BMP_TYPE_ALPHA;
                char_bmp.bmAlphaPixelFormat = pdc->alpha_pixel_format;
            }
            else {
                char_bmp.bmType = BMP_TYPE_COLORKEY;
                char_bmp.bmColorKey = pdc->gray_pixels [0];
                bgcolor = pdc->gray_pixels [0];
            }
        }
        else  
        {
            char_bmp.bmType = BMP_TYPE_COLORKEY;
            char_bmp.bmColorKey = bgcolor;
        }

        /* draw bits to char_bmp.bmBits (pdc->gray_pixels[0] is background) */
        if (logfont->style & FS_WEIGHT_SUBPIXEL) {

        }
        else {
            expand_char_pixmap (pdc, bbox_w, bbox_h, bits, char_bmp.bmBits, 
                    bold, italic, pitch, scale);
        }
    }
    else {  
        if ((((logfont->style & FS_WEIGHT_BOOK)
                        && pdc->alpha_pixel_format)
                    || ((logfont->style & FS_WEIGHT_LIGHT)
                        && pdc->textcolor != pdc->bkcolor))) 
        {   
            unsigned char* ex_bits = char_bmp.bmBits;

            if (logfont->style & FS_WEIGHT_BOOK) 
            {
                char_bmp.bmType = BMP_TYPE_PRIV_PIXEL | BMP_TYPE_ALPHA;
                char_bmp.bmAlphaPixelFormat = pdc->alpha_pixel_format;
            }
            else 
            {
                char_bmp.bmType = BMP_TYPE_COLORKEY;
                char_bmp.bmColorKey = pdc->filter_pixels [0];

                /* special handle for light style */
                memset (char_bmp.bmBits, 0, char_bits_size);
                ex_bits += char_bmp.bmPitch + 1;
            }

            if (scale < 2)
            {
                expand_char_bitmap (bbox_w, bbox_h, bits, 1,
                    ex_bits, 0, 1, bold, italic);
            }
            else
            {
                expand_char_bitmap_scale (bbox_w, bbox_h, bits, 1,
                    ex_bits, 0, 1, bold, italic, scale);
            }
        }
        else if (logfont->style & FS_WEIGHT_SUBPIXEL) {

        }
        else
        { 
            char_bmp.bmType = BMP_TYPE_COLORKEY;
            
            if (bgcolor == pdc->textcolor) 
            {
                bgcolor ^= 1;
            }
            char_bmp.bmColorKey = bgcolor;

            fgcolor = pdc->textcolor;

            if (scale < 2) {
                expand_char_bitmap (bbox_w, bbox_h, bits, bpp,
                        char_bmp.bmBits, bgcolor, fgcolor, bold, italic);
            }
            else {
                expand_char_bitmap_scale (bbox_w, bbox_h, bits, bpp,
                        char_bmp.bmBits, bgcolor, fgcolor, 
                        bold, italic, scale);
            }

            pixmap = TRUE;
        }
    }

    pdc->cur_pixel = pdc->brushcolor;
    pdc->skip_pixel = bgcolor;
    pdc->cur_ban = NULL;

    old_bits = char_bmp.bmBits;

    if (!pixmap) { 

        if (logfont->style & FS_WEIGHT_BOOK) {
            char_bmp.bmBits = do_low_pass_filtering (pdc);
        }
        else if (logfont->style & FS_WEIGHT_SUBPIXEL) {
            expand_bkbmp_to_rgba (pdc, devfont, &char_bmp, (const BYTE*)bits, 
                    pitch, bold, italic, scale);

        }
        else { 

            /* special handle for light style */
            InflateRect (&pdc->rc_output, 1, 1);
            fg_rect.x --; fg_rect.y --;
            fg_rect.w += 2; fg_rect.h += 2;

            char_bmp.bmWidth += 2;
            char_bmp.bmHeight += 2;
            char_bmp.bmBits = do_inflate_filtering (pdc);
        }
    }

    if (char_bmp.bmBits) {
        /* 
         * To optimize:
         * the flip should do when expand bitmap/pixmap.
         */
        if (logfont->style & FS_FLIP_HORZ) {
            HFlipBitmap (&char_bmp, char_bmp.bmBits + 
                    char_bmp.bmPitch * char_bmp.bmHeight);
        }
        if (logfont->style & FS_FLIP_VERT) {
            int offset = -h/2 - fg_rect.h + (*py - bbox_y) * 2;
            fg_rect.y += offset;
            OffsetRect (&pdc->rc_output, 0, offset);
            VFlipBitmap (&char_bmp, char_bmp.bmBits + 
                    char_bmp.bmPitch * char_bmp.bmHeight);
        }

        _dc_fillbox_bmp_clip (pdc, &fg_rect, &char_bmp);
        char_bmp.bmBits = old_bits;
    }

    LEAVE_DRAWING (pdc);
    pdc->rc_output = rcTemp;
}

static void draw_text_lines (PDC pdc, PLOGFONT logfont, int x1, int y1, 
                int x2, int y2)
{
    int h = logfont->size;
    RECT eff_rc, rcOutput, rcTemp;
    CLIPRECT* cliprect;
    int sbc_descent, mbc_descent;
    int orig_y1 = y1, orig_y2 = y2;

    if (x1 == x2 && y1 == y2)
    {
        return;
    }

    sbc_descent = logfont->sbc_devfont->font_ops->get_font_descent 
                    (logfont, logfont->sbc_devfont);
    if (logfont->mbc_devfont) {
        mbc_descent = logfont->mbc_devfont->font_ops->get_font_descent 
                    (logfont, logfont->mbc_devfont);
    }
    else
        mbc_descent = 0;

    SetRect (&rcOutput, x1, y1, x2, y2);
    NormalizeRect (&rcOutput);
    rcOutput.left -= 1;
    rcOutput.top -= h;
    rcOutput.right += 1;
    rcOutput.bottom += 1;

    rcTemp = pdc->rc_output;
    pdc->rc_output = rcOutput;

    ENTER_DRAWING (pdc);

    pdc->cur_pixel = pdc->textcolor;
    if (logfont->style & FS_UNDERLINE_LINE) {
        if (logfont->style & FS_FLIP_VERT) {
            y1 += MAX (sbc_descent, mbc_descent) * 2 + 1;
            y2 += MAX (sbc_descent, mbc_descent) * 2 + 1;
            y1 -= h;
            y2 -= h;
        }

        cliprect = pdc->ecrgn.head;
        while (cliprect) {
            if (IntersectRect (&eff_rc, &pdc->rc_output, &cliprect->rc)
                        && LineClipper (&eff_rc, &x1, &y1, &x2, &y2)) {
                pdc->move_to (pdc, x1, y1);
                LineGenerator (pdc, x1, y1, x2, y2, _dc_set_pixel_noclip);
            }
            cliprect = cliprect->next;
       }
    } 

    if (logfont->style & FS_STRUCKOUT_LINE) {
        y1 = orig_y1 + MAX (sbc_descent, mbc_descent);
        y2 = orig_y2 + MAX (sbc_descent, mbc_descent);
        y1 -= h >> 1;
        y2 -= h >> 1;
        if (logfont->style & FS_FLIP_VERT) {
            y1 ++; y2 ++;
        }

        cliprect = pdc->ecrgn.head;
        while (cliprect) {
            if (IntersectRect (&eff_rc, &pdc->rc_output, &cliprect->rc)
                        && LineClipper (&eff_rc, &x1, &y1, &x2, &y2)) {
                pdc->move_to (pdc, x1, y1);
                LineGenerator (pdc, x1, y1, x2, y2, _dc_set_pixel_noclip);
            }
            cliprect = cliprect->next;
        }
    }

    LEAVE_DRAWING (pdc);
    pdc->rc_output = rcTemp;
}

/* return width of output */
int gdi_strnwrite (PDC pdc, int x, int y, const char* text, int len)
{

    DEVFONT* sbc_devfont = pdc->pLogFont->sbc_devfont;
    DEVFONT* mbc_devfont = pdc->pLogFont->mbc_devfont;
    int len_cur_char;
    int left_bytes = len;
    int origx;
    int origy;
    int sbc_height = (*sbc_devfont->font_ops->get_font_height) 
            (pdc->pLogFont, sbc_devfont);
    int sbc_ascent = (*sbc_devfont->font_ops->get_font_ascent) 
            (pdc->pLogFont, sbc_devfont);
    int mbc_height = 0;
    int mbc_ascent = 0;
    SIZE size; 
    RECT rc_temp; 
    GAL_Rect gal_rc_bg; 


    if (mbc_devfont) {
        mbc_height = (*mbc_devfont->font_ops->get_font_height) 
                (pdc->pLogFont, mbc_devfont);
        mbc_ascent = (*mbc_devfont->font_ops->get_font_ascent) 
                (pdc->pLogFont, mbc_devfont);
    }
    

    /* draw bkcolor of text */
    gdi_get_TextOut_extent (pdc, pdc->pLogFont, text, len, &size);
    gal_rc_bg.x = x;
    gal_rc_bg.y = y;
    gal_rc_bg.w = size.cx;
    gal_rc_bg.h = MAX(sbc_height, mbc_height);

    rc_temp = pdc->rc_output;
    ENTER_DRAWING(pdc);
    pdc->step = 1;
    pdc->cur_ban = NULL;
    if (pdc->bkmode != BM_TRANSPARENT) 
    {
        pdc->cur_pixel = pdc->bkcolor;
        _dc_fillbox_clip (pdc, &gal_rc_bg);
    }
    LEAVE_DRAWING(pdc);
    pdc->rc_output = rc_temp;

    y += MAX (sbc_ascent, mbc_ascent); 
    y += pdc->alExtra;

    origx = x;
    origy = y;

    gdi_start_new_line (pdc);
    while (left_bytes > 0) {
        if (mbc_devfont != NULL) {
            len_cur_char = (*mbc_devfont->charset_ops->len_first_char) 
                    ((const unsigned char*)text, left_bytes);
            if (len_cur_char > 0) {
                put_one_char (pdc, pdc->pLogFont, mbc_devfont, &x, &y, 
                                    mbc_height, mbc_ascent, text, len_cur_char);
                
                left_bytes -= len_cur_char;
                text += len_cur_char;
                x += pdc->cExtra;
                continue;
            }
        }

        len_cur_char = (*sbc_devfont->charset_ops->len_first_char) 
                ((const unsigned char*)text, left_bytes);
        if (len_cur_char > 0)
            put_one_char (pdc, pdc->pLogFont, sbc_devfont, &x, &y, 
                                    sbc_height, sbc_ascent, text, len_cur_char);
        else
            break;

        left_bytes -= len_cur_char;
        text += len_cur_char;
        x += pdc->cExtra;
    }

    draw_text_lines (pdc, pdc->pLogFont, origx, origy, x, y);

    return x - origx;
}

void gdi_draw_tabbed_text_oneline_back(PDC pdc, int x, int y, const char* text, int len)
{
    SIZE size; 
    int oneline_len = len;
    RECT rc_temp; 
    char* p_end;
    GAL_Rect gal_rc_bg;
    int sbc_height = 0;
    int mbc_height = 0;
    int sbc_ascent = 0;
    int mbc_ascent = 0;
    DEVFONT* sbc_devfont;
    DEVFONT* mbc_devfont;

    int tab_width;
    
    p_end = memchr(text, '\n', len);
    if (p_end)
    {
        oneline_len = p_end - text;
    }

    sbc_devfont = pdc->pLogFont->sbc_devfont;
    mbc_devfont = pdc->pLogFont->mbc_devfont;
    
    sbc_height = (*sbc_devfont->font_ops->get_font_height) 
        (pdc->pLogFont, sbc_devfont);
    sbc_ascent = (*sbc_devfont->font_ops->get_font_ascent) 
        (pdc->pLogFont, sbc_devfont);

    if (mbc_devfont) {
        mbc_height = (*mbc_devfont->font_ops->get_font_height) 
            (pdc->pLogFont, mbc_devfont);
        mbc_ascent = (*mbc_devfont->font_ops->get_font_ascent) 
            (pdc->pLogFont, mbc_devfont);
    }

    tab_width = (*sbc_devfont->font_ops->get_ave_width) (pdc->pLogFont, 
            sbc_devfont) * pdc->tabstop;

    /* draw bkcolor of text */
    gdi_get_TabbedTextOut_extent(pdc, pdc->pLogFont, tab_width, text, oneline_len, &size, NULL);
    gal_rc_bg.x = x;
    gal_rc_bg.y = y - MAX(sbc_ascent, mbc_ascent);
    gal_rc_bg.w = size.cx;
    gal_rc_bg.h = MAX(sbc_height, mbc_height);

    rc_temp = pdc->rc_output;
    ENTER_DRAWING(pdc);
    pdc->step = 1;
    pdc->cur_ban = NULL;
    if (pdc->bkmode != BM_TRANSPARENT) 
    {
        pdc->cur_pixel = pdc->bkcolor;
        _dc_fillbox_clip (pdc, &gal_rc_bg);
    }
    LEAVE_DRAWING(pdc);
    pdc->rc_output = rc_temp;

}

int gdi_tabbedtextout (PDC pdc, int x, int y, const char* text, int len)
{
    DEVFONT* sbc_devfont = pdc->pLogFont->sbc_devfont;
    DEVFONT* mbc_devfont = pdc->pLogFont->mbc_devfont;
    DEVFONT* devfont;
    int len_cur_char;
    int left_bytes = len;
    int origx, origy;
    int x_start = x, max_x = x;
    int tab_width, line_height;

    int sbc_height = (*sbc_devfont->font_ops->get_font_height) 
        (pdc->pLogFont, sbc_devfont);
    int sbc_ascent = (*sbc_devfont->font_ops->get_font_ascent) 
        (pdc->pLogFont, sbc_devfont);
    int mbc_height = 0;
    int mbc_ascent = 0;

    if (mbc_devfont) {
        mbc_height = (*mbc_devfont->font_ops->get_font_height) 
            (pdc->pLogFont, mbc_devfont);
        mbc_ascent = (*mbc_devfont->font_ops->get_font_ascent) 
            (pdc->pLogFont, mbc_devfont);
    }

    y += MAX (sbc_ascent, mbc_ascent);
    y += pdc->alExtra;

    origx = x; origy = y;

    tab_width = (*sbc_devfont->font_ops->get_ave_width) (pdc->pLogFont, 
            sbc_devfont) * pdc->tabstop;
    line_height = pdc->pLogFont->size + pdc->alExtra + pdc->blExtra;


    gdi_draw_tabbed_text_oneline_back(pdc, x, y, text, len);

    gdi_start_new_line (pdc);
    while (left_bytes > 0) {
        devfont = NULL;
        len_cur_char = 0;

        if (mbc_devfont != NULL)
            len_cur_char = mbc_devfont->charset_ops->len_first_char 
                ((const unsigned char*)text, left_bytes);

        if (len_cur_char > 0)
            devfont = mbc_devfont;
        else {
            len_cur_char = sbc_devfont->charset_ops->len_first_char 
                ((const unsigned char*)text, left_bytes);
            if (len_cur_char > 0)
                devfont = sbc_devfont;
        }

        if (devfont) {
            int ch_type = devfont->charset_ops->char_type 
                ((const unsigned char*)text);
            switch (ch_type) {
                case MCHAR_TYPE_ZEROWIDTH:
                    break;
                case MCHAR_TYPE_LF:
                    y += line_height;
                case MCHAR_TYPE_CR:
                    if (max_x < x) max_x = x;
                    x = x_start;

                    /* draw_text_lines (pdc, pdc->pLogFont, origx, origy, x, y); */
                    origx = x; origy = y;

                    if (*(text+len_cur_char)  != '\0')
                    gdi_draw_tabbed_text_oneline_back(pdc, x, y, text+len_cur_char, len);

                    gdi_start_new_line (pdc);
                    break;

                case MCHAR_TYPE_HT:
                    /*
                     * x = x_start + ((x - x_start)/tab_width + 1) * tab_width;
                     */
                    x += tab_width;
                    gdi_start_new_line (pdc);
                    break;

                default:
                    put_one_char (pdc, pdc->pLogFont, devfont, &x, &y,
                            (devfont == mbc_devfont)?mbc_height:sbc_height,
                            (devfont == mbc_devfont)?mbc_ascent:sbc_ascent,
                            text, len_cur_char);
                    x += pdc->cExtra;
                    break;
            }
        }
        else
            break;

        left_bytes -= len_cur_char;
        text += len_cur_char;
    }

    draw_text_lines (pdc, pdc->pLogFont, origx, origy, x, y);

    if (max_x < x) max_x = x;

    return max_x - x_start;
}

/*width of fg_rect(width of bbox) */
int gdi_width_one_char (LOGFONT* logfont, DEVFONT* devfont, const char* mchar, 
                int len, int* x, int* y)
{
    int w;
    
    if (devfont->font_ops->get_char_bbox) {
        int oldx = *x;
        (*devfont->font_ops->get_char_bbox) (logfont, devfont,
                            (const unsigned char*)mchar, len, 
                            NULL, NULL, NULL, NULL);
        (*devfont->font_ops->get_char_advance) (logfont, devfont, 
                            (const unsigned char*)mchar, len, x, y);
        w = *x - oldx;
    }
    else
        w = (*devfont->font_ops->get_char_width) (logfont, devfont, 
                        (const unsigned char*)mchar, len);
    
    if (logfont->style & FS_WEIGHT_BOLD 
            && !(devfont->style & FS_WEIGHT_BOLD)) {
        w ++;
    }

    return w;
}

void gdi_get_TextOut_extent (PDC pdc, LOGFONT* log_font, const char* text, 
                int len, SIZE* size)
{
    DEVFONT* sbc_devfont = log_font->sbc_devfont;
    DEVFONT* mbc_devfont = log_font->mbc_devfont;
    int len_cur_char;
    int left_bytes = len;
    int x = 0, y = 0;
    int mbc_font_height = 0; 
    int sbc_font_height = 0;    

    gdi_start_new_line (pdc);

    size->cy = log_font->size + pdc->alExtra + pdc->blExtra;
    size->cx = 0;

    while (left_bytes > 0) {
        if (mbc_devfont != NULL) {
            len_cur_char = (*mbc_devfont->charset_ops->len_first_char) 
                    ((const unsigned char*)text, left_bytes);
            if (len_cur_char > 0) {
                size->cx += gdi_width_one_char (log_font, mbc_devfont, 
                    text, len_cur_char, &x, &y); 
                size->cx += pdc->cExtra;
                left_bytes -= len_cur_char;
                text += len_cur_char;
                continue;
            }
        }

        len_cur_char = (*sbc_devfont->charset_ops->len_first_char) 
                ((const unsigned char*)text, left_bytes);
        if (len_cur_char > 0) {
            size->cx += gdi_width_one_char (log_font, sbc_devfont, 
                    text, len_cur_char, &x, &y);
            size->cx += pdc->cExtra;
        }
        else
            break;

        left_bytes -= len_cur_char;
        text += len_cur_char;
    }

    if (mbc_devfont)
    {
        mbc_font_height = mbc_devfont->font_ops->get_font_height(log_font, mbc_devfont);
    }
    if (sbc_devfont)
    {
        sbc_font_height = sbc_devfont->font_ops->get_font_height(log_font, sbc_devfont);
    }

    if (log_font->style & FS_SLANT_ITALIC
        && (!(sbc_devfont->style & FS_SLANT_ITALIC))) {
        size->cx  += (MAX(mbc_font_height, sbc_font_height) + 1)  >> 1;
    }
}

void gdi_get_TabbedTextOut_extent (PDC pdc, LOGFONT* log_font, int tab_width,
                const char* text, int len, SIZE* size, POINT* last_pos)
{
    DEVFONT* sbc_devfont = log_font->sbc_devfont;
    DEVFONT* mbc_devfont = log_font->mbc_devfont;
    DEVFONT* devfont;
    int len_cur_char;
    int left_bytes = len;
    int line_height;
    int last_line_width = 0;
    int x = 0, y = 0;

    int mbc_font_height = 0; 
    int sbc_font_height = 0;     

    line_height = log_font->size + pdc->alExtra + pdc->blExtra;
    size->cx = 0;
    size->cy = line_height;


    gdi_start_new_line (pdc);
    while (left_bytes > 0) {
        devfont = NULL;
        len_cur_char = 0;

        if (mbc_devfont != NULL)
            len_cur_char = mbc_devfont->charset_ops->len_first_char 
                                ((const unsigned char*)text, left_bytes);

        if (len_cur_char > 0)
            devfont = mbc_devfont;
        else {
            len_cur_char = sbc_devfont->charset_ops->len_first_char
                                ((const unsigned char*)text, left_bytes);
            if (len_cur_char > 0)
                devfont = sbc_devfont;
        }

        if (devfont) {
            int ch_type = devfont->charset_ops->char_type 
                                ((const unsigned char*)text);
            switch (ch_type) {
                case MCHAR_TYPE_ZEROWIDTH:
                    break;
                case MCHAR_TYPE_LF:        
                    size->cy += line_height;
                case MCHAR_TYPE_CR:   
                    if (last_line_width > size->cx)
                        size->cx = last_line_width;
                    last_line_width = 0;
                    gdi_start_new_line (pdc);
                    break;

                case MCHAR_TYPE_HT:
                    /*
                     * last_line_width 
                     * = (last_line_width / tab_width + 1) * tab_width;
                     */
                    last_line_width += tab_width;
                    gdi_start_new_line (pdc);
                    break;

                default:
                    last_line_width += gdi_width_one_char (log_font, devfont, 
                                text, len_cur_char, &x, &y);
                    last_line_width += pdc->cExtra;
                    break;
            }
        }
        else
            break;

         left_bytes -= len_cur_char;
         text += len_cur_char;
    }

    if (last_line_width > size->cx)
        size->cx = last_line_width;

    sbc_font_height = sbc_devfont->font_ops->get_font_height(log_font, sbc_devfont);
    
    if (mbc_devfont)
    {
        mbc_font_height = mbc_devfont->font_ops->get_font_height(log_font, mbc_devfont);
    }

    if (log_font->style & FS_SLANT_ITALIC
        && (!(sbc_devfont->style & FS_SLANT_ITALIC))) {
        size->cx  += (MAX(mbc_font_height, sbc_font_height) + 1)  >> 1;
    }
    
    if (last_pos) {
        last_pos->x += last_line_width;
        last_pos->y += size->cy - line_height;
    }
}
