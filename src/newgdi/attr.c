/* $Id: attr.c 8332 2007-11-29 06:52:58Z xwyan $
**
** Drawing attributes of GDI
**
** Copyright (C) 2003 ~ 2007 Feynman Software
** Copyright (C) 2000 ~ 2002 Wei Yongming.
**
** Current maintainer: Wei Yongming.
**
** Create date: 2000/06/12, derived from original gdi.c
*/

#include "common.h"
#include "minigui.h"
#include "gdi.h"
#include "window.h"
#include "gal.h"
#include "cliprect.h"
#include "internals.h"
#include "ctrlclass.h"
#include "dc.h"

/******************* General drawing attributes *******************************/
Uint32 GUIAPI GetDCAttr (HDC hdc, int attr)
{
    PDC pdc;

    pdc = dc_HDC2PDC (hdc);

#if 1
    if (attr < NR_DC_ATTRS && attr >= 0) {
        Uint32* attrs = (Uint32*) (&pdc->bkcolor);

        return attrs [attr];
    }
#else
    switch (attr) {
    case DC_ATTR_BK_COLOR:
        return (Uint32) pdc->bkcolor;
    case DC_ATTR_BK_MODE:
        return (Uint32) pdc->bkmode;
    case DC_ATTR_TEXT_COLOR:
        return (Uint32) pdc->textcolor;
    case DC_ATTR_TAB_STOP:
        return (Uint32) pdc->tabstop;
    case DC_ATTR_PEN_COLOR:
        return (Uint32) pdc->pencolor;
    case DC_ATTR_BRUSH_COLOR:
        return (Uint32) pdc->brushcolor;
    case DC_ATTR_CHAR_EXTRA:
        return (Uint32) pdc->cExtra;
    case DC_ATTR_ALINE_EXTRA:
        return (Uint32) pdc->alExtra;
    case DC_ATTR_BLINE_EXTRA:
        return (Uint32) pdc->blExtra;
    case DC_ATTR_MAP_MODE:
        return (Uint32) pdc->mapmode;
#ifdef _ADV_2DAPI
    case DC_ATTR_PEN_TYPE:
        return (Uint32) pdc->pen_type;
    case DC_ATTR_PEN_CAP_STYLE:
        return (Uint32) pdc->pen_cap_style;
    case DC_ATTR_PEN_JOIN_STYLE:
        return (Uint32) pdc->pen_joint_style;
    case DC_ATTR_PEN_WIDTH:
        return (Uint32) pdc->pen_width;
    case DC_ATTR_BRUSH_TYPE:
        return (Uint32) pdc->brushtype;
#endif
    }
#endif

    return 0;
}

static int make_alpha_pixel_format (PDC pdc)
{
    Uint32 Rmask, Gmask, Bmask, Amask;

    if (pdc->alpha_pixel_format == NULL) {
        switch (pdc->surface->format->BitsPerPixel) {
        case 15:
        case 16:
            Rmask = 0x0000F000;
            Gmask = 0x00000F00;
            Bmask = 0x000000F0;
            Amask = 0x0000000F;
            break;
        case 24:
            Rmask = 0x00FC0000;
            Gmask = 0x0003F000;
            Bmask = 0x00000FC0;
            Amask = 0x0000003F;
            break;
        case 32:
            Rmask = 0xFF000000;
            Gmask = 0x00FF0000;
            Bmask = 0x0000FF00;
            Amask = 0x000000FF;
            break;
        default:
            return -1;
        }

        pdc->alpha_pixel_format = GAL_AllocFormat (
                        pdc->surface->format->BitsPerPixel, 
                        Rmask, Gmask, Bmask, Amask);
        if (pdc->alpha_pixel_format == NULL)
            return -1;
    }

    return 0;
}

static void make_gray_pixels (PDC pdc)
{
    if (pdc->bkmode == BM_TRANSPARENT
                    && !make_alpha_pixel_format (pdc)) {
        int i;
        Uint8 r, g, b, a;

        GAL_GetRGB (pdc->textcolor, pdc->surface->format, &r, &g, &b);

        a = 0;
        for (i = 0; i < 16; i++) {
            pdc->gray_pixels [i] = GAL_MapRGBA (pdc->alpha_pixel_format, 
                            r, g, b, a);
            a += 16;
        }

        pdc->gray_pixels [16] = GAL_MapRGBA (pdc->alpha_pixel_format, 
                        r, g, b, 255);
    }
    else {
        int i;

        if (pdc->surface->format->BitsPerPixel > 8) {
            int delta_r, delta_g, delta_b;
            RGB pal [17];

            GAL_GetRGB (pdc->bkcolor, pdc->surface->format, 
                            &pal->r, &pal->g, &pal->b);
            GAL_GetRGB (pdc->textcolor, pdc->surface->format, 
                            &pal[16].r, &pal[16].g, &pal[16].b);

            delta_r = ((int)pal[16].r - (int)pal[0].r) / 16;
            delta_g = ((int)pal[16].g - (int)pal[0].g) / 16;
            delta_b = ((int)pal[16].b - (int)pal[0].b) / 16;

            for (i = 1; i < 16; i++) {
                pal[i].r = pal[i - 1].r + delta_r;
                pal[i].g = pal[i - 1].g + delta_g;
                pal[i].b = pal[i - 1].b + delta_b;
                pdc->gray_pixels [i] = GAL_MapRGB (pdc->surface->format, 
                                pal[i].r, pal[i].g, pal[i].b);
            }
            pdc->gray_pixels [0] = pdc->bkcolor;
            pdc->gray_pixels [16] = pdc->textcolor;
        }
        else {
            for (i = 0; i < 16; i++) {
                pdc->gray_pixels [i] = pdc->bkcolor;
            }
            pdc->gray_pixels [16] = pdc->textcolor;
        }
    }
}

static void make_filter_pixels (PDC pdc)
{
    if (pdc->pLogFont->style & FS_WEIGHT_BOOK) {
        int i;
        Uint8 r, g, b, a;

        if (make_alpha_pixel_format (pdc))
            return;

        GAL_GetRGB (pdc->textcolor, pdc->surface->format, &r, &g, &b);

        a = 0;
        for (i = 0; i < 16; i++) {
            pdc->filter_pixels [i] = GAL_MapRGBA (pdc->alpha_pixel_format, 
                            r, g, b, a);
            a += 16;
        }

        pdc->filter_pixels [16] = GAL_MapRGBA (pdc->alpha_pixel_format, 
                        r, g, b, 255);

    }
    else if (pdc->pLogFont->style & FS_WEIGHT_LIGHT) {
        gal_pixel trans = pdc->bkcolor ^ 1;
        if (trans == pdc->textcolor)
            trans = pdc->bkcolor ^ 3;

        pdc->filter_pixels [0] = trans;
        pdc->filter_pixels [1] = pdc->bkcolor;
        pdc->filter_pixels [2] = pdc->textcolor;
    }
}

Uint32 GUIAPI SetDCAttr (HDC hdc, int attr, Uint32 value)
{
    Uint32 old_value = 0;
    PDC pdc;

    pdc = dc_HDC2PDC (hdc);

#if 1
    if (attr < NR_DC_ATTRS && attr >= 0) {
        Uint32* attrs = (Uint32*) (&pdc->bkcolor);
        old_value = attrs [attr];
        attrs [attr] = value;

        if (attr == DC_ATTR_TEXT_COLOR
                || attr == DC_ATTR_BK_COLOR
                || attr == DC_ATTR_BK_MODE) {
            make_gray_pixels (pdc);
            make_filter_pixels (pdc);
        }
        else if (attr == DC_ATTR_PEN_TYPE && !pdc->dash_list) {
            pdc->dash_offset = 0;
            pdc->dash_list = (const unsigned char*) "\xa\xa";
            pdc->dash_list_len = 2;
        }

        return old_value;
    }
#else
    switch (attr) {
    case DC_ATTR_BK_COLOR:
        old_value = (Uint32) pdc->bkcolor;
        pdc->bkcolor = (gal_pixel)value;
        return old_value;
    case DC_ATTR_BK_MODE:
        old_value = (Uint32) pdc->bkmode;
        pdc->bkmode = (int)value;
        return old_value;
    case DC_ATTR_TEXT_COLOR:
        old_value = (Uint32) pdc->textcolor;
        pdc->textcolor = (gal_pixel)value;
        return old_value;
    case DC_ATTR_TAB_STOP:
        old_value = (Uint32) pdc->tabstop;
        pdc->tabstop = (int)value;
        return old_value;
    case DC_ATTR_PEN_COLOR:
        old_value = (Uint32) pdc->pencolor;
        pdc->pencolor = (gal_pixel)value;
        return old_value;
    case DC_ATTR_BRUSH_COLOR:
        old_value = (Uint32) pdc->brushcolor;
        pdc->brushcolor = (gal_pixel)value;
        return old_value;
    case DC_ATTR_CHAR_EXTRA:
        old_value = (Uint32) pdc->cExtra;
        pdc->cExtra = (int)value;
        return old_value;
    case DC_ATTR_ALINE_EXTRA:
        old_value = (Uint32) pdc->alExtra;
        pdc->alExtra = (int)value;
        return old_value;
    case DC_ATTR_BLINE_EXTRA:
        old_value = (Uint32) pdc->blExtra;
        pdc->blExtra = (int)value;
        return old_value;
    case DC_ATTR_MAP_MODE:
        old_value = (Uint32) pdc->mapmode;
        pdc->mapmode = (int)value;
        return old_value;
#ifdef _ADV_2DAPI
    case DC_ATTR_PEN_TYPE:
        old_value = (Uint32) pdc->pen_type;
        pdc->pen_type = (int)value;
        return old_value;
    case DC_ATTR_PEN_CAP_STYLE:
        old_value = (Uint32) pdc->pen_cap_style;
        pdc->pen_cap_style = (int)value;
        return old_value;
    case DC_ATTR_PEN_JOIN_STYLE:
        old_value = (Uint32) pdc->pen_joint_style;
        pdc->pen_joint_style = (int)value;
        return old_value;
    case DC_ATTR_PEN_WIDTH:
        old_value = (Uint32) pdc->pen_width;
        pdc->pen_width = (int)value;
        return old_value;
    case DC_ATTR_BRUSH_TYPE:
        old_value = (Uint32) pdc->brush_type;
        pdc->brush_type = (int)value;
        return old_value;
#endif
    }
#endif

    return 0;
}

PLOGFONT GUIAPI GetCurFont (HDC hdc)
{
    PDC pdc;

    pdc = dc_HDC2PDC(hdc);
    return pdc->pLogFont;
}

PLOGFONT GUIAPI SelectFont (HDC hdc, PLOGFONT log_font)
{
    PLOGFONT old;
    PDC pdc;

    pdc = dc_HDC2PDC(hdc);
    old = pdc->pLogFont;
    if (log_font == INV_LOGFONT)
        pdc->pLogFont = g_SysLogFont [1] ? g_SysLogFont [1] : g_SysLogFont [0];
    else
        pdc->pLogFont = log_font;
    
    make_gray_pixels (pdc);
    make_filter_pixels (pdc);
    return old;
}

