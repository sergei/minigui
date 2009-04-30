/*
** $Id: trackbar.c 7329 2007-08-16 02:59:29Z xgwang $
**
** trackbar.c: the TrackBar Control module.
**
** Copyright (C) 2003 ~ 2007 Feynman Software.
** Copyright (C) 1999 ~ 2002 Wei Yongming.
**
** All rights reserved by Feynman Software.
** 
** Current maintainer: Yan Xiaowei
**
** NOTE: Originally by Zheng Yiran
**
** Create date: 2000/12/02
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "common.h"
#include "minigui.h"
#include "gdi.h"
#include "window.h"
#include "ctrl/ctrlhelper.h"
#include "ctrl/trackbar.h"
#include "cliprect.h"
#include "internals.h"
#include "ctrlclass.h"

#ifdef _CTRL_TRACKBAR

#include "ctrlmisc.h"
#include "trackbar_impl.h"

#ifdef _PHONE_WINDOW_STYLE

static const BITMAP *bmp_hbg, *bmp_hslider;
static const BITMAP *bmp_vbg, *bmp_vslider;

#define WIDTH_HORZ_SLIDER       (bmp_hslider->bmWidth)
#define HEIGHT_VERT_SLIDER      (bmp_vslider->bmHeight)

#else   /* not _PHONE_WINDOW_STYLE */

/* use these definition when drawing trackbar */
#define TB_BORDER               2

#define WIDTH_HORZ_SLIDER       24
#define HEIGHT_HORZ_SLIDER      12

#define WIDTH_VERT_SLIDER       12
#define HEIGHT_VERT_SLIDER      24

#define WIDTH_VERT_RULER        6
#define HEIGHT_HORZ_RULER       6

/* please keep it even for good appearance */
#define LEN_TICK                4
#define GAP_TIP_SLIDER          12
#define GAP_TICK_SLIDER         6

#endif  /* not _PHONE_WINDOW_STYLE */

static int TrackBarCtrlProc (HWND hwnd, int message, WPARAM wParam, LPARAM lParam);

BOOL RegisterTrackBarControl (void)
{
    WNDCLASS WndClass;

#ifdef _PHONE_WINDOW_STYLE
    if ((bmp_hbg = GetStockBitmap (STOCKBMP_TRACKBAR_HBG, 0, 0)) == NULL)
        return FALSE;

    if ((bmp_hslider = GetStockBitmap (STOCKBMP_TRACKBAR_HSLIDER, 0, 0)) == NULL)
        return FALSE;

    if ((bmp_vbg = GetStockBitmap (STOCKBMP_TRACKBAR_VBG, 0, 0)) == NULL)
        return FALSE;

    if ((bmp_vslider = GetStockBitmap (STOCKBMP_TRACKBAR_VSLIDER, 0, 0)) == NULL)
        return FALSE;
#endif

    WndClass.spClassName = "trackbar";
    WndClass.dwStyle     = WS_NONE;
    WndClass.dwExStyle   = WS_EX_NONE;
    WndClass.hCursor     = GetSystemCursor (0);
    WndClass.iBkColor    = GetWindowElementColor (BKC_CONTROL_DEF);
    WndClass.WinProc     = TrackBarCtrlProc;

    return AddNewControlClass (&WndClass) == ERR_OK;
}

#if 0
void TrackBarControlCleanup (void)
{
    return;
}
#endif

#ifdef _PHONE_WINDOW_STYLE

static void GetTrackbarRects (TRACKBARDATA* pData, DWORD dwStyle, const RECT* rc_client, RECT* rc_ruler, RECT* rc_slider)
{
    int     l, t, w, h;
    int     pos, min, max;
    int     sliderx, slidery, sliderw, sliderh;

    /* get data of trackbar. */
    w = RECTWP (rc_client);
    h = RECTHP (rc_client);
    pos = pData->nPos;
    max = pData->nMax;
    min = pData->nMin;

    /* get width/height of slider. */
    if (dwStyle & TBS_VERTICAL) {
        sliderw = bmp_vslider->bmWidth;
        sliderh = bmp_vslider->bmHeight;
        h -= sliderh;
        l = 0; t = sliderh>>1;
    }
    else {
        sliderw = bmp_hslider->bmWidth;
        sliderh = bmp_hslider->bmHeight;
        w -= sliderw;
        l = sliderw>>1; t = 0;
    }

    /* get the rectangle of the ruler. */
    if (rc_ruler) {
        SetRect (rc_ruler, l, t, l + w, t + h);
    }
    
    /* get the rectangle of the slider according to position. */
    if (dwStyle & TBS_VERTICAL) {

        sliderx = (w - sliderw)>>1; 
        slidery = t + (int)((max - pos) * h / (float)(max - min)) - (sliderh>>1);
    }
    else {
        slidery = (h - sliderh)>>1; 
        sliderx = l + (int)((pos - min) * w / (float)(max - min)) - (sliderw>>1);
    }

    SetRect (rc_slider, sliderx, slidery, sliderx + sliderw, slidery + sliderh);
}

static void TrackBarOnDraw (HWND hwnd, HDC hdc, TRACKBARDATA* pData, DWORD dwStyle)
{
    RECT rc_client, rc_ruler, rc_slider;

    GetClientRect (hwnd, &rc_client);

    GetTrackbarRects (pData, dwStyle, &rc_client, &rc_ruler, &rc_slider);

    /* draw the ruler. */
    if (dwStyle & TBS_VERTICAL)
        DrawBoxFromBitmap (hdc, &rc_ruler, bmp_vbg, FALSE, FALSE);
    else
        DrawBoxFromBitmap (hdc, &rc_ruler, bmp_hbg, TRUE, FALSE);
    
    /* draw the slider. */
    FillBoxWithBitmap (hdc, rc_slider.left, rc_slider.top, 0, 0, 
            (dwStyle & TBS_VERTICAL) ? bmp_vslider : bmp_hslider);
}

#else /* not _PHONE_WINDOW_STYLE */

static void GetTrackbarRects (TRACKBARDATA* pData, DWORD dwStyle, const RECT* rc_client, RECT* rc_ruler, RECT* rc_slider)
{
    int x, y, w, h;
    int pos, min, max;
    int sliderx, slidery, sliderw, sliderh;

    /* get data of trackbar. */
    x = rc_client->left;
    y = rc_client->top;
    w = RECTWP (rc_client);
    h = RECTHP (rc_client);
    pos = pData->nPos;
    max = pData->nMax;
    min = pData->nMin;

    if (dwStyle & TBS_BORDER) {
        x += TB_BORDER;
        y += TB_BORDER;
        w -= TB_BORDER << 1;
        h -= TB_BORDER << 1;
    }

    /* get the rectangle of the ruler. */
    if (rc_ruler) {
        if (dwStyle & TBS_VERTICAL) {
            rc_ruler->left = x + ((w - WIDTH_VERT_RULER)>>1);
            rc_ruler->top = y + (HEIGHT_VERT_SLIDER >> 1);
            rc_ruler->right = x + ((w + WIDTH_VERT_RULER)>>1);
            rc_ruler->bottom = y + h - (HEIGHT_VERT_SLIDER >> 1);
        }
        else {
            rc_ruler->left = x + (WIDTH_HORZ_SLIDER >> 1);
            rc_ruler->top = y + ((h - HEIGHT_HORZ_RULER)>>1);
            rc_ruler->right = x + w - (WIDTH_HORZ_SLIDER >> 1);
            rc_ruler->bottom = y + ((h + HEIGHT_HORZ_RULER)>>1);
        }
    }

    /* get width/height of the slider. */
    if (dwStyle & TBS_VERTICAL) {
        sliderw = WIDTH_VERT_SLIDER;
        sliderh = HEIGHT_VERT_SLIDER;
    }
    else {
        sliderw = WIDTH_HORZ_SLIDER;
        sliderh = HEIGHT_HORZ_SLIDER;
    }

    if (dwStyle & TBS_VERTICAL) {
        sliderx = x + ((w - sliderw) >> 1); 
        slidery = y + (HEIGHT_VERT_SLIDER>>1)+ (int)((max - pos) * 
                (h - HEIGHT_VERT_SLIDER) / (float)(max - min)) - (sliderh>>1);
    }
    else {
        slidery = y + ((h - sliderh) >> 1); 
        sliderx = x + (WIDTH_HORZ_SLIDER >> 1) + (int)((pos - min) * 
                (w - WIDTH_HORZ_SLIDER) / (float)(max - min)) - (sliderw>>1);
    }

    SetRect (rc_slider, sliderx, slidery, sliderx + sliderw, slidery + sliderh);
}

static void TrackBarOnDraw (HWND hwnd, HDC hdc, TRACKBARDATA* pData, DWORD dwStyle)
{
    RECT    rc_client, rc_ruler, rc_slider;
    int     x, y, w, h;
    int     max, min;
    int     TickFreq, EndTipLen;
    int     sliderx, slidery, sliderw, sliderh;
    int     TickStart, TickEnd;
    float   TickGap, Tick;
    char    sPos[10];

    GetClientRect (hwnd, &rc_client);
    GetTrackbarRects (pData, dwStyle, &rc_client, &rc_ruler, &rc_slider);

    x = rc_client.left;
    y = rc_client.top;
    w = RECTW (rc_client);
    h = RECTH (rc_client);
    if (dwStyle & TBS_BORDER) {
        x += TB_BORDER;
        y += TB_BORDER;
        w -= TB_BORDER << 1;
        h -= TB_BORDER << 1;
    }

    /* get data of trackbar. */
    TickFreq = pData->nTickFreq;

    /* draw the border according to trackbar style. */
#ifdef _FLAT_WINDOW_STYLE
    if (dwStyle & TBS_BORDER) {
        DrawFlatControlFrameEx (hdc,  hwnd,
            rc_client.left, rc_client.top,
            rc_client.right, rc_client.bottom,
            0, DF_3DBOX_PRESSED | DF_3DBOX_NOTFILL, 0);
    }
#else
    if (dwStyle & TBS_BORDER) {
        Draw3DControlFrameEx (hdc, hwnd,
            rc_client.left, rc_client.top,
            rc_client.right - 1, rc_client.bottom - 1,
            DF_3DBOX_PRESSED | DF_3DBOX_NOTFILL, 0);
    }
#endif

    /* draw the rulder in middle of trackbar. */
#ifdef _FLAT_WINDOW_STYLE
    DrawFlatControlFrameEx (hdc, hwnd,
            rc_ruler.left, rc_ruler.top,
            rc_ruler.right - 1, rc_ruler.bottom - 1,
            0, DF_3DBOX_PRESSED | DF_3DBOX_NOTFILL, 0);    
    
#else
    Draw3DControlFrameEx (hdc, hwnd,
            rc_ruler.left, rc_ruler.top,
            rc_ruler.right - 1, rc_ruler.bottom - 1,
            DF_3DBOX_PRESSED | DF_3DBOX_NOTFILL, 0); 
#endif
    
    max = pData->nMax;
    min = pData->nMin;
    sliderx = rc_slider.left;
    slidery = rc_slider.top;
    sliderw = RECTW(rc_slider);
    sliderh = RECTH(rc_slider);

    /* draw the tick of trackbar. */
    //if (!(dwStyle & TBS_NOTICK) && TickFreq <= (max - min)) {
    if (!(dwStyle & TBS_NOTICK)) {
        SetPenColor (hdc, PIXEL_black);
        if (dwStyle & TBS_VERTICAL) {
            TickStart = y + (HEIGHT_VERT_SLIDER >> 1); 
            TickGap = (h - HEIGHT_VERT_SLIDER) / (float)(max - min) * TickFreq;
            TickEnd = y + h - (HEIGHT_VERT_SLIDER >> 1);
            for (Tick = TickStart; (int)Tick <= TickEnd; Tick += TickGap) { 
                MoveTo (hdc, x + (w>>1) + (sliderw>>1) + GAP_TICK_SLIDER, (int) Tick);
                LineTo (hdc, x + (w>>1) + (sliderw>>1) + GAP_TICK_SLIDER + LEN_TICK, (int) Tick);
            }
            if ((int)(Tick - TickGap + 0.9) < TickEnd) {
                MoveTo (hdc, x + (w>>1) + (sliderw>>1) + GAP_TICK_SLIDER, TickEnd);
                LineTo (hdc, x + (w>>1) + (sliderw>>1) + GAP_TICK_SLIDER + LEN_TICK, TickEnd);
            }
        } else {
            TickStart = x + (WIDTH_HORZ_SLIDER >> 1); 
            TickGap = (w - WIDTH_HORZ_SLIDER) / (float)(max - min) * TickFreq;
            TickEnd = x + w - (WIDTH_HORZ_SLIDER >> 1);
            for (Tick = TickStart; (int)Tick <= TickEnd; Tick += TickGap) { 
                MoveTo (hdc, (int)Tick, y + (h>>1) + (sliderh>>1) + GAP_TICK_SLIDER);
                LineTo (hdc, (int)Tick, y + (h>>1) + (sliderh>>1) + GAP_TICK_SLIDER + LEN_TICK);
            }
            if ((int)(Tick - TickGap + 0.9) < TickEnd) {
                MoveTo (hdc, TickEnd, y + (h>>1) + (sliderh>>1) + GAP_TICK_SLIDER);
                LineTo (hdc, TickEnd, y + (h>>1) + (sliderh>>1) + GAP_TICK_SLIDER + LEN_TICK);
            }
        }
    }

    /* draw the slider of trackbar according to pos. */
#ifdef _FLAT_WINDOW_STYLE
    DrawFlatControlFrameEx (hdc, hwnd,
            rc_slider.left, rc_slider.top,
            rc_slider.right - 1, rc_slider.bottom - 1, 0, 
            DF_3DBOX_NORMAL | DF_3DBOX_FILL, GetWindowBkColor (hwnd));
#else
    Draw3DControlFrameEx (hdc, hwnd,
            rc_slider.left, rc_slider.top,
            rc_slider.right - 1, rc_slider.bottom - 1,
            DF_3DBOX_NORMAL | DF_3DBOX_FILL, GetWindowBkColor (hwnd));
#endif

    SetPenColor (hdc, GetWindowElementColorEx (hwnd, FGC_CONTROL_NORMAL));
    if (dwStyle & TBS_VERTICAL) {
        MoveTo (hdc, sliderx + (sliderw >> 1) - 3, slidery + (sliderh >> 1));
        LineTo (hdc, sliderx + (sliderw >> 1) + 3, slidery + (sliderh >> 1));
        MoveTo (hdc, sliderx + (sliderw >> 1) - 2, slidery + (sliderh >> 1) - 2);
        LineTo (hdc, sliderx + (sliderw >> 1) + 2, slidery + (sliderh >> 1) - 2);
        MoveTo (hdc, sliderx + (sliderw >> 1) - 2, slidery + (sliderh >> 1) + 2);
        LineTo (hdc, sliderx + (sliderw >> 1) + 2, slidery + (sliderh >> 1) + 2);
    }
    else {
        MoveTo (hdc, sliderx + (sliderw >> 1), slidery + (sliderh >> 1) - 3);
        LineTo (hdc, sliderx + (sliderw >> 1), slidery + (sliderh >> 1) + 3);
        MoveTo (hdc, sliderx + (sliderw >> 1) - 2, slidery + (sliderh >> 1) - 2);
        LineTo (hdc, sliderx + (sliderw >> 1) - 2, slidery + (sliderh >> 1) + 2);
        MoveTo (hdc, sliderx + (sliderw >> 1) + 2, slidery + (sliderh >> 1) - 2);
        LineTo (hdc, sliderx + (sliderw >> 1) + 2, slidery + (sliderh >> 1) + 2);
    }

    /* draw the tip of trackbar. */
    if ((dwStyle & TBS_TIP) && !(dwStyle & TBS_VERTICAL)) {
        SIZE text_ext;

        SetBkMode (hdc, BM_TRANSPARENT);
        SetBkColor (hdc, GetWindowBkColor (hwnd));
        SetTextColor (hdc, GetWindowElementColorEx (hwnd, FGC_CONTROL_NORMAL));
        TextOut (hdc, x + 1, y + (h>>1) - (sliderh>>1) - GAP_TIP_SLIDER, 
                            pData->sStartTip);

        GetTextExtent (hdc, pData->sEndTip, -1, &text_ext);
        EndTipLen = text_ext.cx + 4;
        TextOut (hdc, (EndTipLen > (w>>1) - 20 ? x + (w>>1) + 20 : x + w -EndTipLen), 
                        y + (h>>1) - (sliderh>>1) - GAP_TIP_SLIDER, pData->sEndTip); 
        sprintf (sPos, "%d", pData->nPos);
        GetTextExtent (hdc, sPos, -1, &text_ext);
        TextOut (hdc, x + ((w - text_ext.cx) >> 1), 
                        y + (h>>1) -(sliderh>>1) - GAP_TIP_SLIDER, sPos);
    }
    
    /* draw the focus frame. */
#ifdef _PC3D_WINDOW_STYLE
    if (dwStyle & TBS_FOCUS) {
        SetPenColor (hdc, PIXEL_lightwhite);
        FocusRect (hdc, x, y, x + w - 1, y + h - 1);
    }
#endif
}

#endif /* not _PHONE_WINDOW_STYLE end*/

static void TrackBarNormalizeParams (const CONTROL* pCtrl, TRACKBARDATA* pData, BOOL fNotify)
{
    if (pData->nPos >= pData->nMax) {
        if (fNotify) {
            NotifyParent ((HWND)pCtrl, pCtrl->id, TBN_CHANGE);
            NotifyParent ((HWND)pCtrl, pCtrl->id, TBN_REACHMAX);
        }
        pData->nPos = pData->nMax;
    }
    else if (pData->nPos <= pData->nMin) {
        if (fNotify) {
            NotifyParent ((HWND)pCtrl, pCtrl->id, TBN_CHANGE);
            NotifyParent ((HWND)pCtrl, pCtrl->id, TBN_REACHMIN);
        }
        pData->nPos = pData->nMin;
    }
    else if (pData->nPos < pData->nMax && pData->nPos > pData->nMin) {
        if (fNotify)
            NotifyParent ((HWND)pCtrl, pCtrl->id, TBN_CHANGE);
    }
}

static void SetSliderPos (const CONTROL* pCtrl, int new_pos)
{
    TRACKBARDATA* pData = (TRACKBARDATA *)pCtrl->dwAddData2;
    RECT rc_client, old_slider, new_slider;

    if (new_pos > pData->nMax)
        new_pos = pData->nMax; 

    if (new_pos < pData->nMin)
        new_pos = pData->nMin; 

    if (pData->nPos == new_pos) 
        return;

    GetClientRect ((HWND)pCtrl, &rc_client);

    GetTrackbarRects (pData, pCtrl->dwStyle, &rc_client, NULL, &old_slider);

    pData->nPos = new_pos;
    TrackBarNormalizeParams (pCtrl, pData, pCtrl->dwStyle & TBS_NOTIFY);

    GetTrackbarRects (pData, pCtrl->dwStyle, &rc_client, NULL, &new_slider);

    if (pCtrl->dwStyle & TBS_TIP) {
        InvalidateRect ((HWND)pCtrl, NULL, TRUE);
    }
    else if (!EqualRect (&old_slider, &new_slider)) {
        InvalidateRect ((HWND)pCtrl, &old_slider, TRUE);
        InvalidateRect ((HWND)pCtrl, &new_slider, TRUE);
    }

}

static int NormalizeMousePos (HWND hwnd, TRACKBARDATA* pData, int mousepos)
{
    RECT    rcClient;
    int     w, h;
    int     len, pos;

    GetClientRect (hwnd, &rcClient);
    w = RECTW (rcClient);
    h = RECTH (rcClient);

    if (GetWindowStyle (hwnd) & TBS_VERTICAL) {
        int blank = HEIGHT_VERT_SLIDER>>1;

        len = RECTH (rcClient) - HEIGHT_VERT_SLIDER;
        if (mousepos > rcClient.bottom - blank)
            pos = 0;
        else if (mousepos < blank)
            pos = len;
        else
            pos = h - blank - mousepos;
    } else {
        int blank = WIDTH_HORZ_SLIDER>>1;

        len = RECTW (rcClient) - WIDTH_HORZ_SLIDER;
        if (mousepos < blank)
            pos = 0;
        else if (mousepos > rcClient.right - blank)
            pos = len ;
        else
            pos = mousepos - blank;
    }

    return (int)((pData->nMax - pData->nMin) * pos / (float)len + 0.5) + pData->nMin;
}

static int TrackBarCtrlProc (HWND hwnd, int message, WPARAM wParam, LPARAM lParam)
{
    PCONTROL      pCtrl;
    TRACKBARDATA* pData;
    pCtrl = Control (hwnd);
    
    switch (message)
    {
        case MSG_CREATE:
            if (!(pData = malloc (sizeof (TRACKBARDATA)))) {
                fprintf(stderr, "Create trackbar control failure!\n");
                return -1;
            }
            pData->nMax = 10;
            pData->nMin = 0;
            pData->nPos = 0;
            pData->nLineSize = 1;
            pData->nPageSize = 5;
            strcpy (pData->sStartTip, "Start");
            strcpy (pData->sEndTip, "End");
            pData->nTickFreq = 1;
            pCtrl->dwAddData2 = (DWORD)pData;
        break;
    
        case MSG_DESTROY:
            free((void *)(pCtrl->dwAddData2));
        break;

        case MSG_NCPAINT:
            return 0;
       
        case MSG_GETTEXTLENGTH:
        case MSG_GETTEXT:
        case MSG_SETTEXT:
            return -1;

        case MSG_PAINT:
        {
            HDC hdc;

            hdc = BeginPaint (hwnd);
            TrackBarOnDraw (hwnd, hdc, (TRACKBARDATA *)pCtrl->dwAddData2, pCtrl->dwStyle);
            EndPaint (hwnd, hdc);
            return 0;
        }

        case TBM_SETRANGE:
        {
            pData = (TRACKBARDATA *)pCtrl->dwAddData2;

            if (wParam == lParam)
                return -1;

            pData->nMin = MIN ((int)wParam, (int)lParam);
            pData->nMax = MAX ((int)wParam, (int)lParam);

            if (pData->nPos > pData->nMax)
                pData->nPos = pData->nMax;
            if (pData->nPos < pData->nMin)
                pData->nPos = pData->nMin;

            SendMessage (hwnd, TBM_SETPOS, pData->nPos, 0);
            return 0;
        }
        
        case TBM_GETMIN:
            pData = (TRACKBARDATA *)pCtrl->dwAddData2;
            return pData->nMin;
     
        case TBM_GETMAX:    
            pData = (TRACKBARDATA *)pCtrl->dwAddData2;
            return pData->nMax;
    
        case TBM_SETMIN:
            pData = (TRACKBARDATA *)pCtrl->dwAddData2;

            if (wParam == pData->nMin || wParam >= pData->nMax)
                return -1;

            pData->nMin = wParam;
            if (pData->nPos < pData->nMin)
                pData->nPos = pData->nMin;
            SendMessage (hwnd, TBM_SETPOS, pData->nPos, 0);
            return 0;
    
        case TBM_SETMAX:
            pData = (TRACKBARDATA *)pCtrl->dwAddData2;

            if ((int)wParam == pData->nMax || (int)wParam <= pData->nMin)
                return -1;

            pData->nMax = wParam;
            if (pData->nPos > pData->nMax)
                pData->nPos = pData->nMax;
            SendMessage (hwnd, TBM_SETPOS, pData->nPos, 0);
            return 0;
        
        case TBM_SETLINESIZE:
            pData = (TRACKBARDATA *)pCtrl->dwAddData2;
            if (wParam > (pData->nMax - pData->nMin))
                return -1;
            pData->nLineSize = wParam;
            return 0;

        case TBM_GETLINESIZE:
            pData = (TRACKBARDATA *)pCtrl->dwAddData2;
            return pData->nLineSize;
        
        case TBM_SETPAGESIZE:
            pData = (TRACKBARDATA *)pCtrl->dwAddData2;
            if (wParam > (pData->nMax - pData->nMin))
                return -1;
            pData->nPageSize = wParam;
            return 0;
        
        case TBM_GETPAGESIZE:
            pData = (TRACKBARDATA *)pCtrl->dwAddData2;
            return pData->nPageSize;
    
        case TBM_SETPOS:
            SetSliderPos (pCtrl, wParam);
            return 0;
        
        case TBM_GETPOS:
            pData = (TRACKBARDATA *)pCtrl->dwAddData2;
            return pData->nPos;
        
        case TBM_SETTICKFREQ:
            pData = (TRACKBARDATA *)pCtrl->dwAddData2;
            if (wParam > (pData->nMax - pData->nMin))
                wParam = pData->nMax - pData->nMin;
            pData->nTickFreq = wParam > 1 ? wParam : 1;
            InvalidateRect (hwnd, NULL, TRUE);
            return 0;

        case TBM_GETTICKFREQ:
            pData = (TRACKBARDATA *)pCtrl->dwAddData2;
            return pData->nTickFreq;
    
        case TBM_SETTIP:
            pData = (TRACKBARDATA *)pCtrl->dwAddData2;
            if (wParam) 
                strncpy(pData->sStartTip, (char *) wParam, TBLEN_TIP);
            if (lParam)
                strncpy (pData->sEndTip, (char *) lParam, TBLEN_TIP);
            InvalidateRect (hwnd, NULL, TRUE);
            return 0;

        case TBM_GETTIP:
            pData = (TRACKBARDATA *)pCtrl->dwAddData2;
            if (wParam)
                strcpy ((char *) wParam, pData->sStartTip);
            if (lParam)
                strcpy ((char *) lParam, pData->sEndTip);
            return 0;
        
#ifdef _PC3D_WINDOW_STYLE
        case MSG_SETFOCUS:
            if (pCtrl->dwStyle & TBS_FOCUS)
                break;
            pCtrl->dwStyle |= TBS_FOCUS;
            InvalidateRect (hwnd, NULL, TRUE);
            break;
    
        case MSG_KILLFOCUS:
            pCtrl->dwStyle &= ~TBS_FOCUS;
            InvalidateRect (hwnd, NULL, TRUE);
            break;
#endif
    
        case MSG_GETDLGCODE:
            return DLGC_WANTARROWS;

        case MSG_ENABLE:
            if (wParam && (pCtrl->dwStyle & WS_DISABLED))
                pCtrl->dwStyle &= ~WS_DISABLED;
            else if (!wParam && !(pCtrl->dwStyle & WS_DISABLED))
                pCtrl->dwStyle |= WS_DISABLED;
            else
                return 0;
            InvalidateRect (hwnd, NULL, TRUE);

            return 0;

        case MSG_KEYDOWN:
            pData = (TRACKBARDATA *)pCtrl->dwAddData2;

            if (pCtrl->dwStyle & WS_DISABLED)
                return 0;

            switch (LOWORD (wParam)) {
                case SCANCODE_CURSORBLOCKUP:
                case SCANCODE_CURSORBLOCKRIGHT:
                    SetSliderPos (pCtrl, pData->nPos + pData->nLineSize);
                break;

                case SCANCODE_CURSORBLOCKDOWN:
                case SCANCODE_CURSORBLOCKLEFT:
                    SetSliderPos (pCtrl, pData->nPos - pData->nLineSize);
                break;
            
                case SCANCODE_PAGEDOWN:
                    SetSliderPos (pCtrl, pData->nPos - pData->nPageSize);
                break;
            
                case SCANCODE_PAGEUP:
                    SetSliderPos (pCtrl, pData->nPos + pData->nPageSize);
                break;
            
                case SCANCODE_HOME:
                    pData->nPos = pData->nMin;
                    TrackBarNormalizeParams (pCtrl, pData, pCtrl->dwStyle & TBS_NOTIFY);
                    InvalidateRect (hwnd, NULL, TRUE);
                break;
            
                case SCANCODE_END:
                    pData->nPos = pData->nMax;
                    TrackBarNormalizeParams (pCtrl, pData, pCtrl->dwStyle & TBS_NOTIFY);
                    InvalidateRect (hwnd, NULL, TRUE);
                break;
            }
        break;

        case MSG_LBUTTONDOWN:
            if (GetCapture() != hwnd) {
                int mouseX = LOSWORD(lParam);
                int mouseY = HISWORD(lParam);
        
                SetCapture (hwnd);

                NotifyParent ((HWND)pCtrl, pCtrl->id, TBN_STARTDRAG);

                pData = (TRACKBARDATA *)pCtrl->dwAddData2;
                SetSliderPos (pCtrl, NormalizeMousePos (hwnd, pData,
                            (pCtrl->dwStyle & TBS_VERTICAL)?mouseY:mouseX));
            }
            break;
                
        case MSG_MOUSEMOVE:
        {
            int mouseX = LOSWORD(lParam);
            int mouseY = HISWORD(lParam);

            if (GetCapture() == hwnd)
                ScreenToClient (hwnd, &mouseX, &mouseY);
            else
                break;

            pData = (TRACKBARDATA *)pCtrl->dwAddData2;
            SetSliderPos (pCtrl, NormalizeMousePos (hwnd, pData,
                            (pCtrl->dwStyle & TBS_VERTICAL)?mouseY:mouseX));
        }
        break;

        case MSG_LBUTTONUP:
            if (GetCapture() == hwnd) {
                ReleaseCapture ();
                NotifyParent ((HWND)pCtrl, pCtrl->id, TBN_STOPDRAG);
            }
            break;
    
        case MSG_FONTCHANGED:
            InvalidateRect (hwnd, NULL, TRUE);
            return 0;

        default:
            break;    
    }
    
    return DefaultControlProc (hwnd, message, wParam, lParam);
}

#endif /* _CTRL_TRACKBAR */

