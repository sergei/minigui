/* 
** $Id: bitmap.c 8029 2007-11-01 10:02:38Z xwyan $
**
** Bitmap operations of GDI.
**
** Copyright (C) 2003 ~ 2007 Feynman Software
** Copyright (C) 2001 ~ 2002 Wei Yongming.
**
** Current maintainer: Wei Yongming.
**
** Create date: 2000/06/12, derived from original gdi.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "minigui.h"
#include "gdi.h"
#include "window.h"
#include "cliprect.h"
#include "gal.h"
#include "internals.h"
#include "ctrlclass.h"
#include "dc.h"
#include "pixel_ops.h"
#include "cursor.h"
#include "readbmp.h"
#include "bitmap.h"

/****************************** Bitmap Support *******************************/
void _dc_fillbox_clip (PDC pdc, const GAL_Rect* rect)
{
    PCLIPRECT cliprect;
    RECT eff_rc;

    cliprect = pdc->ecrgn.head;
    if (pdc->rop == ROP_SET) {
        while (cliprect) {
            if (IntersectRect (&eff_rc, &pdc->rc_output, &cliprect->rc)) {
                SET_GAL_CLIPRECT (pdc, eff_rc);
                GAL_FillRect (pdc->surface, rect, pdc->cur_pixel);
            }
            cliprect = cliprect->next;
        }
    }
    else {
        pdc->step = 1;
        while (cliprect) {
            if (IntersectRect (&eff_rc, &pdc->rc_output, &cliprect->rc)) {
                int _w = RECTW(eff_rc), _h = RECTH(eff_rc);
                pdc->move_to (pdc, eff_rc.left, eff_rc.top);
                while (_h--) {
                    pdc->draw_hline (pdc, _w);
                    pdc->cur_dst += pdc->surface->pitch;
                }
            }
            cliprect = cliprect->next;
        }
    }

}

#ifdef _ADV_2DAPI
void _dc_fillbox_clip_nosolid_brush (PDC pdc, const GAL_Rect* rect)
{
    PCLIPRECT cliprect;
    RECT eff_rc;

    cliprect = pdc->ecrgn.head;
    while (cliprect) {
        if (IntersectRect (&eff_rc, &pdc->rc_output, &cliprect->rc)) {
            int _w = RECTW(eff_rc), _h = RECTH(eff_rc);
            int y = eff_rc.top;
            while (_h--) {
                _dc_fill_span_brush_helper (pdc, eff_rc.left, y++, _w);
            }
        }

        cliprect = cliprect->next;
    }
}
#endif

void _dc_fillbox_bmp_clip (PDC pdc, const GAL_Rect* rect, BITMAP* bmp)
{
    PCLIPRECT cliprect;
    RECT eff_rc;

    cliprect = pdc->ecrgn.head;
    if (pdc->rop == ROP_SET) {
        while (cliprect) {
            if (IntersectRect (&eff_rc, &pdc->rc_output, &cliprect->rc)) {
                SET_GAL_CLIPRECT (pdc, eff_rc);
                GAL_PutBox (pdc->surface, rect, bmp);
            }

            cliprect = cliprect->next;
        }
    }
    else {
        BYTE* row;
        int _w, _h;

        pdc->step = 1;
        while (cliprect) {
            if (IntersectRect (&eff_rc, &pdc->rc_output, &cliprect->rc)) {

                pdc->move_to (pdc, eff_rc.left, eff_rc.top);
                row = bmp->bmBits + bmp->bmPitch * (eff_rc.top - rect->y)
                        + bmp->bmBytesPerPixel * (eff_rc.left - rect->x);

                _h = RECTH(eff_rc); _w = RECTW(eff_rc);
                while (_h--) {
                    pdc->put_hline (pdc, row, _w);
                    row += bmp->bmPitch;
                    _dc_step_y (pdc, 1);
                }
            }

            cliprect = cliprect->next;
        }
    }
}

void GUIAPI FillBox (HDC hdc, int x, int y, int w, int h)
{
    PDC pdc;
    GAL_Rect rect;

    if (w <= 0 || h <= 0)
        return;

    if (!(pdc = __mg_check_ecrgn (hdc)))
        return;

    if (w < 0) w = RECTW (pdc->DevRC);
    if (h < 0) h = RECTH (pdc->DevRC);

    /* Transfer logical to device to screen here. */
    w += x; h += y;
    coor_LP2SP (pdc, &x, &y);
    coor_LP2SP (pdc, &w, &h);
    SetRect (&pdc->rc_output, x, y, w, h);
    NormalizeRect (&pdc->rc_output);
    w = RECTW (pdc->rc_output); h = RECTH (pdc->rc_output);
    rect.x = x; rect.y = y; rect.w = w; rect.h = h;

    pdc->cur_pixel = pdc->brushcolor;
    pdc->cur_ban = NULL;
    pdc->step = 1;

    ENTER_DRAWING (pdc);

#ifdef _ADV_2DAPI
    if (pdc->brush_type == BT_SOLID)
        _dc_fillbox_clip (pdc, &rect);
    else
        _dc_fillbox_clip_nosolid_brush (pdc, &rect);
#else
    _dc_fillbox_clip (pdc, &rect);
#endif
    
    LEAVE_DRAWING (pdc);
    UNLOCK_GCRINFO (pdc);
}

BOOL GUIAPI GetBitmapFromDC (HDC hdc, int x, int y, int w, int h, BITMAP* bmp)
{
    PDC pdc;
    GAL_Rect rect;
    int ret;

    pdc = dc_HDC2PDC (hdc);
    if (dc_IsGeneralDC (pdc)) {
        LOCK_GCRINFO (pdc);
        dc_GenerateECRgn (pdc, FALSE);
    }

    w += x; h += y;
    coor_LP2SP (pdc, &x, &y);
    coor_LP2SP (pdc, &w, &h);
    SetRect (&pdc->rc_output, x, y, w, h);
    NormalizeRect (&pdc->rc_output);
    w = RECTW (pdc->rc_output); h = RECTH (pdc->rc_output);
    rect.x = x; rect.y = y; rect.w = w; rect.h = h;

    ENTER_DRAWING_NOCHECK (pdc);

    ret = GAL_GetBox (pdc->surface, &rect, bmp);

    LEAVE_DRAWING_NOCHECK (pdc);
    UNLOCK_GCRINFO (pdc);

    if (ret) return FALSE;
    return TRUE;
}

static void* _get_line_buff_fillbox (void* context, int y)
{
    struct _SCALER_INFO_FILLBMP* info = (struct _SCALER_INFO_FILLBMP*) context;

    return info->line_buff;
}

static void _line_scaled_fillbox (void* context, const void* line, int y)
{
    int top;
    CLIPRECT* cliprect;
    RECT rc_output, eff_rc;
    struct _SCALER_INFO_FILLBMP* info = (struct _SCALER_INFO_FILLBMP*) context;

    if (!_dc_which_region_ban (info->pdc, info->dst_rc.top + y))
        return;

    if (info->pdc->rop == ROP_SET) {
        BITMAP tmp_bmp = *info->bmp;
        GAL_Rect rect;
        rect.x = info->dst_rc.left;
        rect.y = info->dst_rc.top + y;
        rect.w = RECTW (info->dst_rc);
        rect.h = 1;

        rc_output.left = info->dst_rc.left;
        rc_output.right = info->dst_rc.right;
        rc_output.top = info->dst_rc.top + y;
        rc_output.bottom = rc_output.top + 1;

        tmp_bmp.bmWidth = rect.w;
        tmp_bmp.bmHeight = 1;
        tmp_bmp.bmPitch = GAL_BytesPerPixel (info->pdc->surface) * rect.w;
        tmp_bmp.bmBits = (Uint8*)line;

        cliprect = info->pdc->cur_ban;
        top = cliprect->rc.top;
        while (cliprect && cliprect->rc.top == top) {
            if (IntersectRect (&eff_rc, &rc_output, &cliprect->rc)) {
                SET_GAL_CLIPRECT (info->pdc, eff_rc);
                GAL_PutBox (info->pdc->surface, &rect, &tmp_bmp);
            }

            cliprect = cliprect->next;
        }
    }
    else {
        rc_output.left = info->dst_rc.left;
        rc_output.right = info->dst_rc.right;
        rc_output.top = info->dst_rc.top + y;
        rc_output.bottom = rc_output.top + 1;

        /* draw in this ban */
        cliprect = info->pdc->cur_ban;
        top = cliprect->rc.top;
        while (cliprect && cliprect->rc.top == top) {
            if (IntersectRect (&eff_rc, &rc_output, &cliprect->rc)) {
                BYTE* row = (BYTE*)line;
                row += GAL_BytesPerPixel (info->pdc->surface) * 
                        (eff_rc.left - rc_output.left);
                info->pdc->move_to (info->pdc, eff_rc.left, eff_rc.top);
                info->pdc->put_hline (info->pdc, row, RECTW(eff_rc));
            }
            cliprect = cliprect->next;
        }
    }
}

static int _bmp_decode_rle (const BITMAP* bmp, const BYTE* encoded,
                            BYTE* decoded, int* skip, int* run)
{
    int i, consumed = 0;

    *skip = 0;
    *run = 0;

    if (bmp->bmBytesPerPixel > 3) {
        *skip = MGUI_ReadLE16Mem (&encoded);
        *run = MGUI_ReadLE16Mem (&encoded);
        consumed += 4;
        encoded += 4;
    }
    else {
        *skip = encoded [0];
        *run = encoded [1];
        consumed += 2;
        encoded += 2;
    }

    for (i = 0; i < *run; i++) {
        decoded = _mem_set_pixel (decoded, bmp->bmBytesPerPixel, 
                    *((gal_pixel*)encoded));
    }

    if (*run > 0) {
        consumed += bmp->bmBytesPerPixel;
    }

    return consumed;
}

PDC _begin_fill_bitmap (HDC hdc, int x, int y, int w, int h, 
                             const BITMAP* bmp, FILLINFO* fill_info)
{
    PDC pdc;
    GAL_Rect rect;
    int sw, sh;
    struct _SCALER_INFO_FILLBMP *info;

    if (!bmp || bmp->bmWidth == 0 || bmp->bmHeight == 0 || bmp->bmBits == NULL)
        return NULL;

    if (!(pdc = __mg_check_ecrgn (hdc)))
        return NULL;

    info = &fill_info->scaler_info;
    info->line_buff = NULL;
    fill_info->decoded_buff = NULL;

    sw = bmp->bmWidth;
    sh = bmp->bmHeight;

    if (bmp->bmType & BMP_TYPE_RLE) {
        fill_info->decoded_buff = malloc (bmp->bmBytesPerPixel * sw);
        if (fill_info->decoded_buff == NULL)
            goto fail;
        fill_info->encoded_bits = bmp->bmBits;

        /* disable scale for the RLE bitmap */
        w = sw; h = sh;
    }
    else {
        fill_info->decoded_buff = NULL;

        if (w <= 0) w = sw;
        if (h <= 0) h = sh;
    }

    /* Transfer logical to device to screen here. */
    w += x; h += y;
    coor_LP2SP (pdc, &x, &y);
    coor_LP2SP (pdc, &w, &h);
    SetRect (&pdc->rc_output, x, y, w, h);
    NormalizeRect (&pdc->rc_output);
    w = RECTW (pdc->rc_output); h = RECTH (pdc->rc_output);
    rect.x = x; rect.y = y; rect.w = w; rect.h = h;

    if (w != sw || h != sh) {
        info->pdc = pdc;
        info->bmp = bmp;
        SetRect (&info->dst_rc, rect.x, rect.y, 
                        rect.x + rect.w, rect.y + rect.h);
        info->line_buff = malloc (GAL_BytesPerPixel (pdc->surface) * w);
        if (info->line_buff == NULL) {
            goto fail;
        }
    }
    else
        info->line_buff = NULL;

    pdc->step = 1;
    pdc->cur_ban = NULL;
    pdc->cur_pixel = pdc->brushcolor;
    pdc->skip_pixel = bmp->bmColorKey;
    fill_info->old_bkmode = pdc->bkmode;
    if (bmp->bmType & BMP_TYPE_COLORKEY) {
        pdc->bkmode = BM_TRANSPARENT;
    }

    if(__mg_enter_drawing (pdc) < 0) {
        goto fail;
    }

    fill_info->dst_rect = rect;

    return pdc;

fail:
    UNLOCK_GCRINFO (pdc);
    if (fill_info->decoded_buff)
        free (fill_info->decoded_buff);
    if (info->line_buff)
        free (info->line_buff);
    return NULL;
}

void _end_fill_bitmap (PDC pdc, const BITMAP* bmp, FILLINFO *fill_info)
{
    pdc->bkmode = fill_info->old_bkmode;

    __mg_leave_drawing (pdc);

    if (fill_info->decoded_buff)
        free (fill_info->decoded_buff);

    if (fill_info->scaler_info.line_buff)
        free (fill_info->scaler_info.line_buff);

    UNLOCK_GCRINFO (pdc);
}

static void _fill_bitmap (PDC pdc, const BITMAP *bmp, FILLINFO *fill_info)
{
    int sw = bmp->bmWidth;
    int sh = bmp->bmHeight;
    int w = fill_info->dst_rect.w;
    int h = fill_info->dst_rect.h;

    if (w != sw || h != sh) {
        BitmapDDAScaler (&fill_info->scaler_info, bmp, w, h,
            _get_line_buff_fillbox, _line_scaled_fillbox);
    }
    else {
        _dc_fillbox_bmp_clip (pdc, &fill_info->dst_rect, (BITMAP*)bmp);
    }
}

void _fill_bitmap_scanline (PDC pdc, const BITMAP* bmp, 
                FILLINFO* fill_info, int y)
{
    GAL_Rect dst_rect;
    dst_rect.x = fill_info->dst_rect.x;
    dst_rect.y = fill_info->dst_rect.y + y;
    dst_rect.w = fill_info->dst_rect.w;
    dst_rect.h = 1;

    _dc_fillbox_bmp_clip (pdc, &dst_rect, (BITMAP*)bmp);
}

BOOL GUIAPI FillBoxWithBitmap (HDC hdc, int x, int y, int w, int h, 
                const BITMAP* bmp)
{
    PDC pdc;
    FILLINFO fill_info;

    pdc = _begin_fill_bitmap (hdc, x, y, w, h, bmp, &fill_info);
    if (pdc == NULL)
        return FALSE;

    if (bmp->bmType & BMP_TYPE_RLE) {
        int y;
        GAL_Rect dst_rect;
        BITMAP my_bmp = *bmp;
        int consumed, skip, run;

        my_bmp.bmHeight = 1;
        my_bmp.bmBits = fill_info.decoded_buff;
        for (y = 0; y < fill_info.dst_rect.h; y ++) {
            dst_rect.x = fill_info.dst_rect.x;
            dst_rect.y = fill_info.dst_rect.y + y;
            dst_rect.h = 1;
            
            do {
                consumed = _bmp_decode_rle (bmp, fill_info.encoded_bits, 
                            fill_info.decoded_buff, &skip, &run);

                if ((skip + run) > 0) {
                    dst_rect.x += skip;
                    dst_rect.w = run;
                    _dc_fillbox_bmp_clip (pdc, &dst_rect, &my_bmp);

                    dst_rect.x = fill_info.dst_rect.x + run + skip;
                    fill_info.encoded_bits += consumed;
                }
            } while (consumed > 0);
        }
    }
    else
        _fill_bitmap (pdc, bmp, &fill_info);

    _end_fill_bitmap (pdc, bmp, &fill_info);

    return TRUE;
}

struct _SCALER_INFO_FILLBMPPART
{
    PDC pdc;
    int dst_x, dst_y;
    int dst_w, dst_h;
    int off_x, off_y;
    BYTE* line_buff;
    const BITMAP* bmp;
};

static void* _get_line_buff_fillboxpart (void* context, int y)
{
    struct _SCALER_INFO_FILLBMPPART* info = 
            (struct _SCALER_INFO_FILLBMPPART*) context;

    return info->line_buff;
}

static void _line_scaled_fillboxpart (void* context, const void* line, int y)
{
    int top;
    RECT rc_output, eff_rc;
    CLIPRECT* cliprect; 

    struct _SCALER_INFO_FILLBMPPART* info = 
            (struct _SCALER_INFO_FILLBMPPART*) context;

    if (y < info->off_y || y >= (info->off_y + info->dst_h))
        return;

    if (!_dc_which_region_ban (info->pdc, info->dst_y + y - info->off_y))
        return;

    if (info->pdc->rop == ROP_SET) {
        BITMAP tmp_bmp = *info->bmp;
        GAL_Rect rect;
        rect.x = info->dst_x;
        rect.y = info->dst_y + y - info->off_y;
        rect.w = info->dst_w;
        rect.h = 1;

        rc_output.left = info->dst_x;
        rc_output.right = info->dst_x + info->dst_w;
        rc_output.top = info->dst_y + y - info->off_y;
        rc_output.bottom = rc_output.top + 1;

        tmp_bmp.bmWidth = info->dst_w;
        tmp_bmp.bmHeight = 1;
        tmp_bmp.bmPitch = GAL_BytesPerPixel (info->pdc->surface) * info->dst_w;
        tmp_bmp.bmBits = (Uint8*)line;
        tmp_bmp.bmBits += GAL_BytesPerPixel (info->pdc->surface) * info->off_x;

        cliprect = info->pdc->cur_ban;
        top = cliprect->rc.top;
        while (cliprect && cliprect->rc.top == top) {
            if (IntersectRect (&eff_rc, &rc_output, &cliprect->rc)) {
                SET_GAL_CLIPRECT (info->pdc, eff_rc);
                GAL_PutBox (info->pdc->surface, &rect, &tmp_bmp);
            }

            cliprect = cliprect->next;
        }
    }
    else {
        rc_output.left = info->dst_x;
        rc_output.right = info->dst_x + info->dst_w;
        rc_output.top = info->dst_y + y - info->off_y;
        rc_output.bottom = rc_output.top + 1;

        /* draw in this ban */
        cliprect = info->pdc->cur_ban;
        top = cliprect->rc.top;
        while (cliprect && cliprect->rc.top == top) {
            if (IntersectRect (&eff_rc, &rc_output, &cliprect->rc)) {
                BYTE* row = (BYTE*)line;
                row += GAL_BytesPerPixel (info->pdc->surface) * 
                        (info->off_x + eff_rc.left - rc_output.left);
                info->pdc->move_to (info->pdc, eff_rc.left, eff_rc.top);
                info->pdc->put_hline (info->pdc, row, RECTW(eff_rc));
            }
            cliprect = cliprect->next;
        }
    }
}

BOOL GUIAPI FillBoxWithBitmapPart (HDC hdc, int x, int y, int w, int h,
                int bw, int bh, const BITMAP* bmp, int xo, int yo)
{
    PDC pdc;
    GAL_Rect rect;
    int old_bkmode;
    struct _SCALER_INFO_FILLBMPPART info;

    if (bmp->bmWidth == 0 || bmp->bmHeight == 0 || bmp->bmBits == NULL)
        return FALSE;

    if (bmp->bmType & BMP_TYPE_RLE)
        return FALSE;

    if (!(pdc = __mg_check_ecrgn (hdc)))
        return TRUE;

    /* Transfer logical to device to screen here. */
    w += x; h += y;
    coor_LP2SP(pdc, &x, &y);
    coor_LP2SP(pdc, &w, &h);
    SetRect (&pdc->rc_output, x, y, w, h);
    NormalizeRect (&pdc->rc_output);
    w = RECTW (pdc->rc_output); h = RECTH (pdc->rc_output);
    rect.x = x; rect.y = y; rect.w = w; rect.h = h;

    if (bw <= 0) bw = bmp->bmWidth;
    if (bh <= 0) bh = bmp->bmHeight;

    if (xo < 0 || yo < 0 || (xo + w) > bw || (yo + h) > bh) {
        UNLOCK_GCRINFO (pdc);
        return FALSE;
    }

    if (bw != bmp->bmWidth || bh != bmp->bmHeight) {
        info.pdc = pdc;
        info.dst_x = x; info.dst_y = y;
        info.dst_w = w; info.dst_h = h;
        info.off_x = xo; info.off_y = yo;
        info.bmp = bmp;
        info.line_buff = malloc (GAL_BytesPerPixel (pdc->surface) * bw);
        if (info.line_buff == NULL) {
            UNLOCK_GCRINFO (pdc);
            return FALSE;
        }
    }
    else
        info.line_buff = NULL;

    pdc->step = 1;
    pdc->cur_ban = NULL;
    pdc->cur_pixel = pdc->brushcolor;
    pdc->skip_pixel = bmp->bmColorKey;
    old_bkmode = pdc->bkmode;
    if (bmp->bmType & BMP_TYPE_COLORKEY) {
        pdc->bkmode = BM_TRANSPARENT;
    }

    ENTER_DRAWING (pdc);

    if (bw != bmp->bmWidth || bh != bmp->bmHeight) {
        BitmapDDAScaler (&info, bmp, bw, bh,
            _get_line_buff_fillboxpart, _line_scaled_fillboxpart);
    }
    else {
        BITMAP part = *bmp;
        if (xo != 0 || yo != 0) {
            part.bmBits += part.bmPitch * yo + 
                    xo * GAL_BytesPerPixel (pdc->surface);
        }
        _dc_fillbox_bmp_clip (pdc, &rect, &part);
    }

    pdc->bkmode = old_bkmode;

    LEAVE_DRAWING (pdc);

    if (info.line_buff)
        free (info.line_buff);

    UNLOCK_GCRINFO (pdc);

    return TRUE;
}

void GUIAPI BitBlt (HDC hsdc, int sx, int sy, int sw, int sh,
                   HDC hddc, int dx, int dy, DWORD dwRop)
{
    PCLIPRECT cliprect;
    PDC psdc, pddc;
    RECT srcOutput, dstOutput;
    GAL_Rect dst, src;
    RECT eff_rc;

    psdc = dc_HDC2PDC (hsdc);
    if (!(pddc = __mg_check_ecrgn (hddc)))
        return;

    /* The coordinates should be in device space. */
#if 0
    sw += sx; sh += sy;
    coor_LP2SP (psdc, &sx, &sy);
    coor_LP2SP (psdc, &sw, &sh);
    SetRect (&srcOutput, sx, sy, sw, sh);
    NormalizeRect (&srcOutput);
    (sw > sx) ? (sw -= sx) : (sw = sx - sw);
    (sh > sy) ? (sh -= sy) : (sh = sy - sh);
    coor_LP2SP (pddc, &dx, &dy);
    SetRect (&dstOutput, dx, dy, dx + sw, dy + sh);
    NormalizeRect (&dstOutput);
#else
    if (sw <= 0) sw = RECTW (psdc->DevRC);
    if (sh <= 0) sh = RECTH (psdc->DevRC);

    coor_DP2SP (psdc, &sx, &sy);
    SetRect (&srcOutput, sx, sy, sx + sw, sy + sh);

    coor_DP2SP (pddc, &dx, &dy);
    SetRect (&dstOutput, dx, dy, dx + sw, dy + sh);
#endif

    if (pddc->surface ==  psdc->surface)
        GetBoundRect (&pddc->rc_output, &srcOutput, &dstOutput);
    else
        pddc->rc_output = dstOutput;

    if (pddc->surface !=  psdc->surface && (psdc->surface == __gal_screen))
        psdc->rc_output = srcOutput;

    ENTER_DRAWING (pddc);

    if (pddc->surface !=  psdc->surface && (psdc->surface == __gal_screen))
        ShowCursorForGDI (FALSE, psdc);

    if (pddc->surface == psdc->surface && dy > sy)
        cliprect = pddc->ecrgn.tail;
    else
        cliprect = pddc->ecrgn.head;

    while (cliprect) {
        if (IntersectRect (&eff_rc, &pddc->rc_output, &cliprect->rc)) {
            SET_GAL_CLIPRECT (pddc, eff_rc);

            src.x = sx; src.y = sy; src.w = sw; src.h = sh;
            dst.x = dx; dst.y = dy; dst.w = sw; dst.h = sh;
            GAL_BlitSurface (psdc->surface, &src, pddc->surface, &dst);
        }

        if (pddc->surface == psdc->surface && dy > sy)
            cliprect = cliprect->prev;
        else
            cliprect = cliprect->next;
    }

    if (pddc->surface !=  psdc->surface && (psdc->surface == __gal_screen))
        ShowCursorForGDI (TRUE, psdc);

    LEAVE_DRAWING (pddc);

    UNLOCK_GCRINFO (pddc);
}

struct _SCALER_INFO_STRETCHBLT
{
    PDC pdc;
    int dst_x, dst_y;
    int dst_w, dst_h;
    BYTE* line_buff;
    BOOL bottom2top;
};

static void* _get_line_buff_stretchblt (void* context, int y)
{
    struct _SCALER_INFO_STRETCHBLT* info = 
            (struct _SCALER_INFO_STRETCHBLT*) context;

    return info->line_buff;
}

static void _line_scaled_stretchblt (void* context, const void* line, int y)
{
    struct _SCALER_INFO_STRETCHBLT* info = 
            (struct _SCALER_INFO_STRETCHBLT*) context;

    if (info->bottom2top)
        info->pdc->cur_ban = NULL;

    if (_dc_which_region_ban (info->pdc, info->dst_y + y)) {
        int top;
        CLIPRECT* cliprect;
        RECT rc_output, eff_rc;

        rc_output.left = info->dst_x;
        rc_output.right = info->dst_x + info->dst_w;
        rc_output.top = info->dst_y + y;
        rc_output.bottom = rc_output.top + 1;

        /* draw in this ban */
        cliprect = info->pdc->cur_ban;
        top = cliprect->rc.top;
        while (cliprect && cliprect->rc.top == top) {
            if (IntersectRect (&eff_rc, &rc_output, &cliprect->rc)) {
                BYTE* row = (BYTE*)line;
                row += GAL_BytesPerPixel (info->pdc->surface) * 
                        (eff_rc.left - rc_output.left);
                info->pdc->move_to (info->pdc, eff_rc.left, eff_rc.top);
                info->pdc->put_hline (info->pdc, row, RECTW(eff_rc));
            }
            cliprect = cliprect->next;
        }
    }
}

void GUIAPI StretchBlt (HDC hsdc, int sx, int sy, int sw, int sh,
                       HDC hddc, int dx, int dy, int dw, int dh, DWORD dwRop)
{
    PDC psdc, pddc;
    RECT srcOutput, dstOutput;
    BITMAP bmp;
    struct _SCALER_INFO_STRETCHBLT info;

    psdc = dc_HDC2PDC (hsdc);
    if (!(pddc = __mg_check_ecrgn (hddc)))
        return;

    if (GAL_BitsPerPixel (psdc->surface) != GAL_BitsPerPixel (pddc->surface)) {
        goto error_ret;
    }

    if (sx < 0) sx = 0;
    if (sy < 0) sy = 0;
    if (dx < 0) dx = 0;
    if (dy < 0) dy = 0;

    if (sx >= RECTW(psdc->DevRC))
        goto error_ret;
    if (sy >= RECTH(psdc->DevRC))
        goto error_ret;
    if (dx >= RECTW(pddc->DevRC))
        goto error_ret;
    if (dy >= RECTH(pddc->DevRC))
        goto error_ret;

    if (sw <= 0 || sw > RECTW (psdc->DevRC) - sx) sw = RECTW (psdc->DevRC) - sx;
    if (sh <= 0 || sh > RECTH (psdc->DevRC) - sy) sh = RECTH (psdc->DevRC) - sy;
    if (dw <= 0 || dw > RECTW (pddc->DevRC) - dx) dw = RECTW (pddc->DevRC) - dx;
    if (dh <= 0 || dh > RECTH (pddc->DevRC) - dy) dh = RECTH (pddc->DevRC) - dy;


    /* The coordinates should be in device space. */
#if 0
    // Transfer logical to device to screen here.
    sw += sx; sh += sy;
    coor_LP2SP(psdc, &sx, &sy);
    coor_LP2SP(psdc, &sw, &sh);
    SetRect (&srcOutput, sx, sy, sw, sh);
    NormalizeRect (&srcOutput);
    sw = RECTW (srcOutput); sh = RECTH (srcOutput);

    dw += dx; dh += dy;
    coor_LP2SP (pddc, &dx, &dy);
    coor_LP2SP (pddc, &dw, &dh);
    SetRect (&dstOutput, dx, dy, dw, dh);
    NormalizeRect (&dstOutput);
    dw = RECTW (dstOutput); dh = RECTH (dstOutput);
#else
    coor_DP2SP (psdc, &sx, &sy);
    SetRect (&srcOutput, sx, sy, sx + sw, sy + sh);
/*
    if (!IsCovered (&srcOutput, &psdc->DevRC))
        goto error_ret;
*/

    coor_DP2SP (pddc, &dx, &dy);
    SetRect (&dstOutput, dx, dy, dx + dw, dy + dh);
#endif

    info.pdc = pddc;
    info.dst_x = dx; info.dst_y = dy;
    info.dst_w = dw; info.dst_h = dh;
    info.line_buff = malloc (GAL_BytesPerPixel (pddc->surface) * dw);

    if (info.line_buff == NULL) {
        goto error_ret;
    }

    memset (&bmp, 0, sizeof (bmp));
    bmp.bmType = BMP_TYPE_NORMAL;
    bmp.bmBitsPerPixel = GAL_BitsPerPixel (psdc->surface);
    bmp.bmBytesPerPixel = GAL_BytesPerPixel (psdc->surface);
    bmp.bmWidth = sw;
    bmp.bmHeight = sh;
    bmp.bmPitch = psdc->surface->pitch;
    bmp.bmBits = (unsigned char*)psdc->surface->pixels 
            + sy * psdc->surface->pitch 
            + sx * GAL_BytesPerPixel (psdc->surface);

    if (pddc->surface ==  psdc->surface)
        GetBoundRect (&pddc->rc_output, &srcOutput, &dstOutput);
    else
        pddc->rc_output = dstOutput;

    if (pddc->surface !=  psdc->surface && (psdc->surface == __gal_screen))
        psdc->rc_output = srcOutput;

    ENTER_DRAWING (pddc);

    if (pddc->surface !=  psdc->surface && (psdc->surface == __gal_screen))
        ShowCursorForGDI (FALSE, psdc);

#if 0
    PCLIPRECT cliprect;
    GAL_Rect src, dst;
    RECT eff_rc;

    if (pddc->surface == psdc->surface && dy > sy)
        cliprect = pddc->ecrgn.tail;
    else
        cliprect = pddc->ecrgn.head;
    while(cliprect) {
        if (IntersectRect (&eff_rc, &pddc->rc_output, &cliprect->rc)) {
            SET_GAL_CLIPRECT (pddc, eff_rc);

            src.x = sx; src.y = sy; src.w = sw; src.h = sh;
            dst.x = dx; dst.y = dy; dst.w = dw; dst.h = dh;
            GAL_SoftStretch (psdc->surface, &src, pddc->surface, &dst);
        }

        if (pddc->surface == psdc->surface && dy > sy)
            cliprect = cliprect->prev;
        else
            cliprect = cliprect->next;
    }
#else
    pddc->step = 1;
    pddc->cur_ban = NULL;
    if (pddc->surface == psdc->surface 
                    && DoesIntersect (&srcOutput, &dstOutput)) {
        info.bottom2top = TRUE;
        BitmapDDAScaler2 (&info, &bmp, dw, dh,
                _get_line_buff_stretchblt, _line_scaled_stretchblt);
    }
    else {
        info.bottom2top = FALSE;
        BitmapDDAScaler (&info, &bmp, dw, dh,
                _get_line_buff_stretchblt, _line_scaled_stretchblt);
    }
#endif

    if (pddc->surface !=  psdc->surface && (psdc->surface == __gal_screen))
        ShowCursorForGDI (TRUE, psdc);

    LEAVE_DRAWING (pddc);

    free (info.line_buff);

error_ret:
    UNLOCK_GCRINFO (pddc);
}

/* 
 * This function performs a fast box scaling.
 * This is a DDA-based algorithm; Iteration over target bitmap.
 *
 * This function comes from SVGALib, Copyright 1993 Harm Hanemaayer
 */

/* We use the 32-bit to 64-bit multiply and 64-bit to 32-bit divide of the
 * 386 (which gcc doesn't know well enough) to efficiently perform integer
 * scaling without having to worry about overflows.
 */

#if defined(__GNUC__) && defined(i386)

static inline int muldiv64(int m1, int m2, int d)
{
    /* int32 * int32 -> int64 / int32 -> int32 */
    int result;
    int dummy;
    __asm__ __volatile__ (
               "imull %%edx\n\t"
               "idivl %4\n\t"
  :            "=a"(result), "=d"(dummy)    /* out */
  :            "0"(m1), "1"(m2), "g"(d)     /* in */
        );
    return result;
}

#else

static inline int muldiv64 (int m1, int m2, int d)
{
    Sint64 mul = (Sint64) m1 * m2;

    return (int) (mul / d);
}

#endif

BOOL GUIAPI BitmapDDAScaler (void* context, const BITMAP* src_bmp, int dst_w, int dst_h,
            CB_GET_LINE_BUFF cb_line_buff, CB_LINE_SCALED cb_line_scaled)
{
    BYTE *dp1 = src_bmp->bmBits;
    BYTE *dp2;
    int xfactor;
    int yfactor;

    if (dst_w <= 0 || dst_h <= 0)
        return FALSE;

    xfactor = muldiv64 (src_bmp->bmWidth, 65536, dst_w);    /* scaled by 65536 */
    yfactor = muldiv64 (src_bmp->bmHeight, 65536, dst_h);   /* scaled by 65536 */

    switch (src_bmp->bmBytesPerPixel) {
    case 1:
        {
            int y, sy;
            sy = 0;
            for (y = 0; y < dst_h;) {
                int sx = 0;
                int x;

                x = 0;
                dp2 = cb_line_buff (context, y);
                while (x < dst_w) {
                    *(dp2 + x) = *(dp1 + (sx >> 16));
                    sx += xfactor;
                    x++;
                }
                cb_line_scaled (context, dp2, y);
                y++;
                while (y < dst_h) {
                    int syint = sy >> 16;
                    sy += yfactor;
                    if ((sy >> 16) != syint)
                        break;
                    /* Copy identical lines. */
                    cb_line_scaled (context, dp2, y);
                    y++;
                }
                dp1 = src_bmp->bmBits + (sy >> 16) * src_bmp->bmPitch;
            }
        }
    break;
    case 2:
        {
            int y, sy;
            sy = 0;
            for (y = 0; y < dst_h;) {
                int sx = 0;
                int x;

                x = 0;
                dp2 = cb_line_buff (context, y);
                /* This can be greatly optimized with loop */
                /* unrolling; omitted to save space. */
                while (x < dst_w) {
                    *(unsigned short *) (dp2 + x * 2) =
                        *(unsigned short *) (dp1 + (sx >> 16) * 2);
                    sx += xfactor;
                    x++;
                }
                cb_line_scaled (context, dp2, y);
                y++;
                while (y < dst_h) {
                    int syint = sy >> 16;
                    sy += yfactor;
                    if ((sy >> 16) != syint)
                        break;
                    /* Copy identical lines. */
                    cb_line_scaled (context, dp2, y);
                    y++;
                }
                dp1 = src_bmp->bmBits + (sy >> 16) * src_bmp->bmPitch;
            }
        }
    break;
    case 3:
        {
            int y, sy;
            sy = 0;
            for (y = 0; y < dst_h;) {
                int sx = 0;
                int x;

                x = 0;
                dp2 = cb_line_buff (context, y);
                /* This can be greatly optimized with loop */
                /* unrolling; omitted to save space. */
                while (x < dst_w) {
                    *(unsigned short *) (dp2 + x * 3) =
                        *(unsigned short *) (dp1 + (sx >> 16) * 3);
                    *(unsigned char *) (dp2 + x * 3 + 2) =
                        *(unsigned char *) (dp1 + (sx >> 16) * 3 + 2);
                    sx += xfactor;
                    x++;
                }
                cb_line_scaled (context, dp2, y);
                y++;
                while (y < dst_h) {
                    int syint = sy >> 16;
                    sy += yfactor;
                    if ((sy >> 16) != syint)
                        break;
                    /* Copy identical lines. */
                    cb_line_scaled (context, dp2, y);
                    y++;
                }
                dp1 = src_bmp->bmBits + (sy >> 16) * src_bmp->bmPitch;
            }
        }
    break;
    case 4:
        {
            int y, sy;
            sy = 0;
            for (y = 0; y < dst_h;) {
                int sx = 0;
                int x;

                x = 0;
                dp2 = cb_line_buff (context, y);
                /* This can be greatly optimized with loop */
                /* unrolling; omitted to save space. */
                while (x < dst_w) {
                    *(unsigned *) (dp2 + x * 4) =
                        *(unsigned *) (dp1 + (sx >> 16) * 4);
                    sx += xfactor;
                    x++;
                }
                cb_line_scaled (context, dp2, y);
                y++;
                while (y < dst_h) {
                    int syint = sy >> 16;
                    sy += yfactor;
                    if ((sy >> 16) != syint)
                        break;
                    /* Copy identical lines. */
                    cb_line_scaled (context, dp2, y);
                    y++;
                }
                dp1 = src_bmp->bmBits + (sy >> 16) * src_bmp->bmPitch;
            }
        }
    break;
    }

    return TRUE;
    
}

BOOL GUIAPI BitmapDDAScaler2 (void* context, const BITMAP* src_bmp, int dst_w, int dst_h,
            CB_GET_LINE_BUFF cb_line_buff, CB_LINE_SCALED cb_line_scaled)
{
    BYTE *src_bits = src_bmp->bmBits + src_bmp->bmPitch * (src_bmp->bmHeight - 1);
    BYTE *dp1 = src_bits;
    BYTE *dp2;
    int xfactor;
    int yfactor;

    if (dst_w == 0 || dst_h == 0)
        return TRUE;

    xfactor = muldiv64 (src_bmp->bmWidth, 65536, dst_w);    /* scaled by 65536 */
    yfactor = muldiv64 (src_bmp->bmHeight, 65536, dst_h);   /* scaled by 65536 */

    switch (src_bmp->bmBytesPerPixel) {
    case 1:
        {
            int y, sy;
            sy = 0;
            for (y = dst_h - 1; y >= 0;) {
                int sx = 0;
                int x;

                x = 0;
                dp2 = cb_line_buff (context, y);
                while (x < dst_w) {
                    *(dp2 + x) = *(dp1 + (sx >> 16));
                    sx += xfactor;
                    x++;
                }
                cb_line_scaled (context, dp2, y);
                y--;
                while (y >= 0) {
                    int syint = sy >> 16;
                    sy += yfactor;
                    if ((sy >> 16) != syint)
                        break;
                    /* Copy identical lines. */
                    cb_line_scaled (context, dp2, y);
                    y--;
                }
                dp1 = src_bits - (sy >> 16) * src_bmp->bmPitch;
            }
        }
    break;
    case 2:
        {
            int y, sy;
            sy = 0;
            for (y = dst_h - 1; y >= 0;) {
                int sx = 0;
                int x;

                x = 0;
                dp2 = cb_line_buff (context, y);
                /* This can be greatly optimized with loop */
                /* unrolling; omitted to save space. */
                while (x < dst_w) {
                    *(unsigned short *) (dp2 + x * 2) =
                        *(unsigned short *) (dp1 + (sx >> 16) * 2);
                    sx += xfactor;
                    x++;
                }
                cb_line_scaled (context, dp2, y);
                y--;
                while (y >= 0) {
                    int syint = sy >> 16;
                    sy += yfactor;
                    if ((sy >> 16) != syint)
                        break;
                    /* Copy identical lines. */
                    cb_line_scaled (context, dp2, y);
                    y--;
                }
                dp1 = src_bits - (sy >> 16) * src_bmp->bmPitch;
            }
        }
    break;
    case 3:
        {
            int y, sy;
            sy = 0;
            for (y = dst_h - 1; y >= 0;) {
                int sx = 0;
                int x;

                x = 0;
                dp2 = cb_line_buff (context, y);
                /* This can be greatly optimized with loop */
                /* unrolling; omitted to save space. */
                while (x < dst_w) {
                    *(unsigned short *) (dp2 + x * 3) =
                        *(unsigned short *) (dp1 + (sx >> 16) * 3);
                    *(unsigned char *) (dp2 + x * 3 + 2) =
                        *(unsigned char *) (dp1 + (sx >> 16) * 3 + 2);
                    sx += xfactor;
                    x++;
                }
                cb_line_scaled (context, dp2, y);
                y--;
                while (y >= 0) {
                    int syint = sy >> 16;
                    sy += yfactor;
                    if ((sy >> 16) != syint)
                        break;
                    /* Copy identical lines. */
                    cb_line_scaled (context, dp2, y);
                    y--;
                }
                dp1 = src_bits - (sy >> 16) * src_bmp->bmPitch;
            }
        }
    break;
    case 4:
        {
            int y, sy;
            sy = 0;
            for (y = dst_h - 1; y >= 0;) {
                int sx = 0;
                int x;

                x = 0;
                dp2 = cb_line_buff (context, y);
                /* This can be greatly optimized with loop */
                /* unrolling; omitted to save space. */
                while (x < dst_w) {
                    *(unsigned *) (dp2 + x * 4) =
                        *(unsigned *) (dp1 + (sx >> 16) * 4);
                    sx += xfactor;
                    x++;
                }
                cb_line_scaled (context, dp2, y);
                y--;
                while (y >= 0) {
                    int syint = sy >> 16;
                    sy += yfactor;
                    if ((sy >> 16) != syint)
                        break;
                    /* Copy identical lines. */
                    cb_line_scaled (context, dp2, y);
                    y--;
                }
                dp1 = src_bits - (sy >> 16) * src_bmp->bmPitch;
            }
        }
    break;
    }

    return TRUE;
    
}

struct _SCALER_INFO
{
    BITMAP* dst;
    int last_y;
};

static void* _get_line_buff_scalebitmap (void* context, int y)
{
    BYTE* line;
    struct _SCALER_INFO* info = (struct _SCALER_INFO*) context;

    line = info->dst->bmBits;
    line += info->dst->bmPitch * y;
    info->last_y = y;

    return line;
}

static void _line_scaled_scalebitmap (void* context, const void* line, int y)
{
    BYTE* dst_line;
    struct _SCALER_INFO* info = (struct _SCALER_INFO*) context;

    if (y != info->last_y) {
        dst_line = info->dst->bmBits + info->dst->bmPitch * y;
        memcpy (dst_line, line, info->dst->bmPitch);
    }
}

BOOL ScaleBitmap (BITMAP *dst, const BITMAP *src)
{
    struct _SCALER_INFO info;
    
    info.dst = dst;
    info.last_y = 0;

    if (dst->bmWidth == 0 || dst->bmHeight == 0)
        return TRUE;

    if (dst->bmBytesPerPixel != src->bmBytesPerPixel)
        return FALSE;

    BitmapDDAScaler (&info, src, dst->bmWidth, dst->bmHeight,
            _get_line_buff_scalebitmap, _line_scaled_scalebitmap);

    return TRUE;
}

gal_pixel GUIAPI GetPixelInBitmap (const BITMAP* bmp, int x, int y)
{
    BYTE* dst;

    if (x < 0 || y < 0 || x >= bmp->bmWidth || y >= bmp->bmHeight)
        return 0;

    dst = bmp->bmBits + y * bmp->bmPitch + x * bmp->bmBytesPerPixel;
    return _mem_get_pixel (dst, bmp->bmBytesPerPixel);
}

BOOL GUIAPI SetPixelInBitmap (const BITMAP* bmp, int x, int y, gal_pixel pixel)
{
    BYTE* dst;

    if (x < 0 || y < 0 || x >= bmp->bmWidth || y >= bmp->bmHeight)
        return FALSE;

    dst = bmp->bmBits + y * bmp->bmPitch + x * bmp->bmBytesPerPixel;
    _mem_set_pixel (dst, bmp->bmBytesPerPixel, pixel);
    return TRUE;
}

/* This function expand monochorate bitmap. */
void GUIAPI ExpandMonoBitmap (HDC hdc, BYTE* bits, Uint32 pitch, 
                const BYTE* my_bits, Uint32 my_pitch,
                Uint32 w, Uint32 h, DWORD flags, Uint32 bg, Uint32 fg)
{
    Uint32 x, y;
    BYTE *dst, *dst_line;
    const BYTE *src, *src_line;
    Uint32 pixel;
    int bpp = GAL_BytesPerPixel (dc_HDC2PDC (hdc)->surface);
    BYTE byte = 0;

    dst_line = bits;
    if (flags & MYBMP_FLOW_UP)
        src_line = my_bits + my_pitch * (h - 1);
    else
        src_line = my_bits;

    for (y = 0; y < h; y++) {
        src = src_line;
        dst = dst_line;

        for (x = 0; x < w; x++) {
            if (x % 8 == 0)
                byte = *src++;

            if ((byte & (128 >> (x % 8))))   /* pixel */
                pixel = fg;
            else
                pixel = bg;

            dst = _mem_set_pixel (dst, bpp, pixel);
        }
        
        if (flags & MYBMP_FLOW_UP)
            src_line -= my_pitch;
        else
            src_line += my_pitch;

        dst_line += pitch;
    }
}

static const RGB WindowsStdColor [] = {
    {0x00, 0x00, 0x00},     // black         --0
    {0x80, 0x00, 0x00},     // dark red      --1
    {0x00, 0x80, 0x00},     // dark green    --2
    {0x80, 0x80, 0x00},     // dark yellow   --3
    {0x00, 0x00, 0x80},     // dark blue     --4
    {0x80, 0x00, 0x80},     // dark magenta  --5
    {0x00, 0x80, 0x80},     // dark cyan     --6
    {0xC0, 0xC0, 0xC0},     // light gray    --7
    {0x80, 0x80, 0x80},     // dark gray     --8
    {0xFF, 0x00, 0x00},     // red           --9
    {0x00, 0xFF, 0x00},     // green         --10
    {0xFF, 0xFF, 0x00},     // yellow        --11
    {0x00, 0x00, 0xFF},     // blue          --12
    {0xFF, 0x00, 0xFF},     // magenta       --13
    {0x00, 0xFF, 0xFF},     // cyan          --14
    {0xFF, 0xFF, 0xFF},     // light white   --15
};

/* This function expand 16-color bitmap. */
void GUIAPI Expand16CBitmap (HDC hdc, BYTE* bits, Uint32 pitch, const BYTE* my_bits, Uint32 my_pitch,
                Uint32 w, Uint32 h, DWORD flags, const RGB* pal)
{
    PDC pdc;
    Uint32 x, y;
    BYTE *dst, *dst_line;
    const BYTE *src, *src_line;
    int index, bpp;
    Uint32 pixel;
    BYTE byte = 0;

    pdc = dc_HDC2PDC(hdc);
    bpp = GAL_BytesPerPixel (pdc->surface);

    dst_line = bits;
    if (flags & MYBMP_FLOW_UP)
        src_line = my_bits + my_pitch * (h - 1);
    else
        src_line = my_bits;

    for (y = 0; y < h; y++) {
        src = src_line;
        dst = dst_line;
        for (x = 0; x < w; x++) {
            if (x % 2 == 0) {
                byte = *src++;
                index = (byte >> 4) & 0x0f;
            }
            else
                index = byte & 0x0f;

            if (pal)
                pixel = GAL_MapRGB (pdc->surface->format, 
                                pal[index].r, pal[index].g, pal[index].b);
            else
                pixel = GAL_MapRGB (pdc->surface->format, 
                                WindowsStdColor[index].r, 
                                WindowsStdColor[index].g, 
                                WindowsStdColor[index].b);

            dst = _mem_set_pixel (dst, bpp, pixel);
        }

        if (flags & MYBMP_FLOW_UP)
            src_line -= my_pitch;
        else
            src_line += my_pitch;

        dst_line += pitch;
    }
}

/* This function expands 256-color bitmap. */
void GUIAPI Expand256CBitmap (HDC hdc, BYTE* bits, Uint32 pitch, 
                const BYTE* my_bits, Uint32 my_pitch,
                Uint32 w, Uint32 h, DWORD flags, const RGB* pal)
{
    PDC pdc;
    Uint32 x, y;
    int bpp;
    BYTE *dst, *dst_line;
    const BYTE *src, *src_line;
    Uint32 pixel;
    BYTE byte;

    pdc = dc_HDC2PDC(hdc);
    bpp = GAL_BytesPerPixel (pdc->surface);

    dst_line = bits;
    if (flags & MYBMP_FLOW_UP)
        src_line = my_bits + my_pitch * (h - 1);
    else
        src_line = my_bits;

    for (y = 0; y < h; y++) {
        src = src_line;
        dst = dst_line;
        for (x = 0; x < w; x++) {
            byte = *src++;

            if (pal)
                pixel = GAL_MapRGB (pdc->surface->format, 
                                pal[byte].r, pal[byte].g, pal[byte].b);
            else if (bpp == 1)
                /* 
                 * Assume that the palette of the bitmap is the same as 
                 * the palette of the DC.
                 */
                pixel = byte;
            else
                /* 
                 * Treat the bitmap uses the dithered colorful palette.
                 */
                pixel = GAL_MapRGB (pdc->surface->format, 
                                    (byte >> 5) & 0x07,
                                    (byte >> 2) & 0x07,
                                    byte & 0x03);

            dst = _mem_set_pixel (dst, bpp, pixel);
        }

        if (flags & MYBMP_FLOW_UP)
            src_line -= my_pitch;
        else
            src_line += my_pitch;

        dst_line += pitch;
    }
}

/* This function compile a RGBA bitmap. */
void GUIAPI CompileRGBABitmap (HDC hdc, BYTE* bits, Uint32 pitch, 
                const BYTE* my_bits, Uint32 my_pitch,
                Uint32 w, Uint32 h, DWORD flags, void* pixel_format)
{
    PDC pdc;
    Uint32 x, y;
    int bpp;
    BYTE *dst, *dst_line;
    const BYTE *src, *src_line;
    Uint32 pixel;
    GAL_Color rgb;

    pdc = dc_HDC2PDC(hdc);
    bpp = GAL_BytesPerPixel (pdc->surface);

    dst_line = bits;
    if (flags & MYBMP_FLOW_UP)
        src_line = my_bits + my_pitch * (h - 1);
    else
        src_line = my_bits;

    /* expand bits here. */
    for (y = 0; y < h; y++) {
        src = src_line;
        dst = dst_line;
        for (x = 0; x < w; x++) {
            if ((flags & MYBMP_TYPE_MASK) == MYBMP_TYPE_BGR) {
                rgb.b = *src++;
                rgb.g = *src++;
                rgb.r = *src++;
            }
            else {
                rgb.r = *src++;
                rgb.g = *src++;
                rgb.b = *src++;
            }

            if (flags & MYBMP_RGBSIZE_4) {
                if (flags & MYBMP_ALPHA) {
                    rgb.a = *src;
                    pixel = GAL_MapRGBA ((GAL_PixelFormat*) pixel_format, 
                                    rgb.r, rgb.g, rgb.b, rgb.a);
                }
                else {
                    pixel = GAL_MapRGB (pdc->surface->format, 
                                    rgb.r, rgb.g, rgb.b);
                }
                src++;
            }
            else {
                pixel = GAL_MapRGB (pdc->surface->format, rgb.r, rgb.g, rgb.b);
            }
            
            dst = _mem_set_pixel (dst, bpp, pixel);
        }

        if (flags & MYBMP_FLOW_UP)
            src_line -= my_pitch;
        else
            src_line += my_pitch;

        dst_line += pitch;
    }
}

/* This function replaces one color with specified color. */
void GUIAPI ReplaceBitmapColor (HDC hdc, BITMAP* bmp, 
                gal_pixel iOColor, gal_pixel iNColor)
{
    PDC pdc;
    int w, h, i;
    BYTE* line, *bits;
    int bpp;

    pdc = dc_HDC2PDC (hdc);
    bpp = GAL_BytesPerPixel (pdc->surface);

    h = bmp->bmHeight;
    w = bmp->bmWidth;
    line = bmp->bmBits;
    switch (bpp) {
        case 1:
            while (h--) {
                bits = line;
                for (i = 0; i < w; i++) {
                    if (*bits == iOColor)
                        *bits = iNColor;
                    bits++;
                }
                line += bmp->bmPitch;
            }
            break;
        case 2:
            while (h--) {
                bits = line;
                for (i = 0; i < w; i++) {
                    if( *(Uint16 *) bits == iOColor)
                        *(Uint16 *) bits = iNColor;
                    bits += 2;
                }
                line += bmp->bmPitch;
            }
            break;
        case 3: 
            while (h--) {
                bits = line;
                for (i = 0; i < w; i++) {
                    if ((*(Uint16 *) bits == iOColor) 
                           && (*(bits + 2) == (iOColor >> 16)) )
                    {
                        *(Uint16 *) bits = iNColor;
                        *(bits + 2) = iNColor >> 16;
                    }
                    bits += 3;
                }
                line += bmp->bmPitch;
            }
            break;
        case 4:    
            while (h--) {
                bits = line;
                for (i = 0; i < w; i++) {
                    if( *(Uint32 *) bits == iOColor )
                        *(Uint32 *) bits = iNColor;
                    bits += 4;
                }
                line += bmp->bmPitch;
            }
            break;
    }
}

void GUIAPI HFlipBitmap (BITMAP* bmp, unsigned char* inter_buff)
{
    int bpp;
    int x, y;
    unsigned char* sline, *dline, *sp;
    gal_pixel pixel;

    bpp = bmp->bmBytesPerPixel;

    sline = bmp->bmBits;
    for (y = 0; y < bmp->bmHeight; y++) {

        sp = sline + bpp * bmp->bmWidth;
        dline = inter_buff;

        for (x = 0; x < bmp->bmWidth; x++) {
            sp -= bpp;
            pixel = _mem_get_pixel (sp, bpp);
            dline = _mem_set_pixel (dline, bpp, pixel);
        }

        memcpy (sline, inter_buff, bmp->bmPitch);

        sline += bmp->bmPitch;
    }
}

void GUIAPI VFlipBitmap (BITMAP* bmp, unsigned char* inter_buff)
{
    int y;
    int bpp;
    unsigned char* sline, *dline;

    bpp = bmp->bmBytesPerPixel;

    sline = bmp->bmBits;
    dline = bmp->bmBits + bmp->bmPitch * bmp->bmHeight;
    for (y = 0; y < (bmp->bmHeight >> 1); y++) {

        dline -= bmp->bmPitch;

        memcpy (inter_buff, sline, bmp->bmPitch);
        memcpy (sline, dline, bmp->bmPitch);
        memcpy (dline, inter_buff, bmp->bmPitch);

        sline += bmp->bmPitch;
    }
}

