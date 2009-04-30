/*
** $Id: spinbox.c 7367 2007-08-16 05:25:35Z xgwang $
**
** spinbox.c: the SpinBox control
**
** Copyright (C) 2003 ~ 2007 Feynman Software
** Copyright (C) 2001 ~ 2002, Zhang Yunfan, Wei Yongming
** 
** Create date: 2001/3/27
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __MINIGUI_LIB__
    #include "common.h"
    #include "minigui.h"
    #include "gdi.h"
    #include "window.h"
    #include "control.h"
#else
    #include <minigui/common.h>
    #include <minigui/minigui.h>
    #include <minigui/gdi.h>
    #include <minigui/window.h>
    #include <minigui/control.h>
#endif

#ifdef _EXT_CTRL_SPINBOX

#include "ext/spinbox.h"
#include "spinbox_impl.h"

static const BITMAP* bmp_spinbox_v;
static const BITMAP* bmp_spinbox_h;

#define SPINBOX_VERT_WIDTH           (bmp_spinbox_v->bmWidth)
#define SPINBOX_VERT_HEIGHT          (bmp_spinbox_v->bmHeight>>2)

#define SPINBOX_HORZ_WIDTH           (bmp_spinbox_h->bmWidth>>2)
#define SPINBOX_HORZ_HEIGHT          (bmp_spinbox_h->bmHeight)

typedef struct SpinData {
    SPININFO spinfo;
    HWND hTarget;
    int canup, candown;
} SPINDATA;
typedef SPINDATA* PSPINDATA;

static int SpinCtrlProc (HWND hWnd, int uMsg, WPARAM wParam, LPARAM lParam);

BOOL RegisterSpinControl (void)
{
    WNDCLASS WndClass;

#ifdef _PHONE_WINDOW_STYLE
    if ((bmp_spinbox_v = GetStockBitmap (STOCKBMP_SPINBOX_VERT, 0, 0)) == NULL)
        return FALSE;

    if ((bmp_spinbox_h = GetStockBitmap (STOCKBMP_SPINBOX_HORZ, 0, 0)) == NULL)
        return FALSE;
#else
    if ((bmp_spinbox_v = GetStockBitmap (STOCKBMP_SPINBOX_VERT, -1, -1)) == NULL)
        return FALSE;

    if ((bmp_spinbox_h = GetStockBitmap (STOCKBMP_SPINBOX_HORZ, -1, -1)) == NULL)
        return FALSE;
#endif
    WndClass.spClassName = CTRL_SPINBOX;
    WndClass.dwStyle     = WS_NONE;
    WndClass.dwExStyle   = WS_EX_NONE;
    WndClass.hCursor     = GetSystemCursor (IDC_ARROW);
    WndClass.iBkColor    = GetWindowElementColor (BKC_CONTROL_DEF);
    WndClass.WinProc     = SpinCtrlProc;

    return RegisterWindowClass (&WndClass);
}

void SpinControlCleanup (void)
{
    UnregisterWindowClass (CTRL_SPINBOX);
}

void GetSpinBoxSize (DWORD dwStyle, int* w, int* h)
{
    if (dwStyle & SPS_HORIZONTAL) {
        if (h) *h = SPINBOX_HORZ_HEIGHT;
        if (w) {
            if ((dwStyle & SPS_TYPE_MASK) != SPS_TYPE_NORMAL)
                *w = SPINBOX_HORZ_WIDTH;
            else
                *w = SPINBOX_HORZ_WIDTH*2 + 2;
        }
    }
    else {
        if (w) *w = SPINBOX_VERT_WIDTH;

        if (h) {
            if ((dwStyle & SPS_TYPE_MASK) != SPS_TYPE_NORMAL)
                *h = SPINBOX_VERT_HEIGHT;
            else
                *h = SPINBOX_VERT_HEIGHT*2 + 2;
        }
    }
}

static void DrawSpinBoxVert (HDC hdc, SPININFO* spinfo , SPINDATA* psd, DWORD dwStyle)
{
    BOOL flag = !(dwStyle & SPS_AUTOSCROLL);
    int y = 0;

    if ((dwStyle & SPS_TYPE_MASK) == SPS_TYPE_NORMAL
            || (dwStyle & SPS_TYPE_MASK) == SPS_TYPE_UPARROW) {
        if (spinfo->cur > spinfo->min || (flag && psd->canup)) {
            FillBoxWithBitmapPart (hdc, 0, 0, SPINBOX_VERT_WIDTH, SPINBOX_VERT_HEIGHT, 0, 0,
                    bmp_spinbox_v, 0, 0);
        }
        else {
            FillBoxWithBitmapPart (hdc, 0, 0, SPINBOX_VERT_WIDTH, SPINBOX_VERT_HEIGHT, 0, 0,
                    bmp_spinbox_v, 0, SPINBOX_VERT_HEIGHT);
        }

        y = SPINBOX_VERT_HEIGHT + 2;
    }

    if ((dwStyle & SPS_TYPE_MASK) == SPS_TYPE_NORMAL
            || (dwStyle & SPS_TYPE_MASK) == SPS_TYPE_DOWNARROW) {
        if (spinfo->cur < spinfo->max || (flag && psd->candown)) {
            FillBoxWithBitmapPart (hdc, 0, y, SPINBOX_VERT_WIDTH, SPINBOX_VERT_HEIGHT, 0, 0,
                    bmp_spinbox_v, 0, SPINBOX_VERT_HEIGHT*2);
        }
        else {
            FillBoxWithBitmapPart (hdc, 0, y, SPINBOX_VERT_WIDTH, SPINBOX_VERT_HEIGHT, 0, 0,
                    bmp_spinbox_v, 0, SPINBOX_VERT_HEIGHT*3);
        }
    }
}

static void DrawSpinBoxHorz (HDC hdc, SPININFO* spinfo , SPINDATA* psd, DWORD dwStyle)
{
    BOOL flag = !(dwStyle & SPS_AUTOSCROLL);
    int x = 0;

    if ((dwStyle & SPS_TYPE_MASK) == SPS_TYPE_NORMAL
            || (dwStyle & SPS_TYPE_MASK) == SPS_TYPE_UPARROW) {
        if (spinfo->cur > spinfo->min || (flag && psd->canup)) {
            FillBoxWithBitmapPart (hdc, 0, 0, SPINBOX_HORZ_WIDTH, SPINBOX_HORZ_HEIGHT, 0, 0,
                    bmp_spinbox_h, 0, 0);
        }
        else {
            FillBoxWithBitmapPart (hdc, 0, 0, SPINBOX_HORZ_WIDTH, SPINBOX_HORZ_HEIGHT, 0, 0,
                    bmp_spinbox_h, SPINBOX_HORZ_WIDTH, 0);
        }

        x = SPINBOX_HORZ_WIDTH + 2;
    }

    if ((dwStyle & SPS_TYPE_MASK) == SPS_TYPE_NORMAL
            || (dwStyle & SPS_TYPE_MASK) == SPS_TYPE_DOWNARROW) {
        if (spinfo->cur < spinfo->max || (flag && psd->candown)) {
            FillBoxWithBitmapPart (hdc, x, 0, SPINBOX_HORZ_WIDTH, SPINBOX_HORZ_HEIGHT, 0, 0,
                    bmp_spinbox_h, SPINBOX_HORZ_WIDTH*2, 0);
        }
        else {
            FillBoxWithBitmapPart (hdc, x, 0, SPINBOX_HORZ_WIDTH, SPINBOX_HORZ_HEIGHT, 0, 0,
                    bmp_spinbox_h, SPINBOX_HORZ_WIDTH*3, 0);
        }
    }
}

static void OnUpArrow (HWND hWnd, PSPINDATA pData, int id, DWORD dwStyle, DWORD wParam)
{
    if ((dwStyle & SPS_AUTOSCROLL)) {
        if (pData->spinfo.cur > pData->spinfo.min ) {
            pData->spinfo.cur --;
            PostMessage ( pData->hTarget, MSG_KEYDOWN, SCANCODE_CURSORBLOCKUP, (wParam|KS_SPINPOST));
            PostMessage ( pData->hTarget, MSG_KEYUP, SCANCODE_CURSORBLOCKUP, (wParam|KS_SPINPOST));
            if ( pData->spinfo.cur <= pData->spinfo.min ) {
                NotifyParent ( hWnd, id, SPN_REACHMIN);
            }
        }
    } else {
        if (pData->canup) {
            pData->spinfo.cur --;
            PostMessage ( pData->hTarget, MSG_KEYDOWN, SCANCODE_CURSORBLOCKUP, wParam|KS_SPINPOST);
            PostMessage ( pData->hTarget, MSG_KEYUP, SCANCODE_CURSORBLOCKUP, wParam|KS_SPINPOST);
            if ( pData->spinfo.cur <= pData->spinfo.min ) {
                NotifyParent ( hWnd, id, SPN_REACHMIN);
            }
        }
    }
}

static void OnDownArrow (HWND hWnd, PSPINDATA pData, int id, DWORD dwStyle, DWORD wParam)
{
    if ((dwStyle & SPS_AUTOSCROLL)) {
        if (pData->spinfo.cur < pData->spinfo.max) {
            pData->spinfo.cur ++;
            PostMessage ( pData->hTarget, MSG_KEYDOWN, SCANCODE_CURSORBLOCKDOWN, wParam|KS_SPINPOST);
            PostMessage ( pData->hTarget, MSG_KEYUP, SCANCODE_CURSORBLOCKDOWN, wParam|KS_SPINPOST);
            if ( pData->spinfo.cur >= pData->spinfo.max) {
                NotifyParent ( hWnd, id, SPN_REACHMAX);
            }
        }
    } else {
        if (pData->candown) {
            pData->spinfo.cur ++;
            PostMessage ( pData->hTarget, MSG_KEYDOWN, SCANCODE_CURSORBLOCKDOWN, wParam|KS_SPINPOST);
            PostMessage ( pData->hTarget, MSG_KEYUP, SCANCODE_CURSORBLOCKDOWN, wParam|KS_SPINPOST);
            if ( pData->spinfo.cur >= pData->spinfo.max) {
                NotifyParent ( hWnd, id, SPN_REACHMAX);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////
static int SpinCtrlProc (HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{
    PSPINDATA pData = (PSPINDATA) GetWindowAdditionalData2 (hWnd);

    switch (message) {   
        case MSG_CREATE:
            pData = (PSPINDATA) calloc (1, sizeof (SPINDATA));
            if (!pData) {
                return -1;
            }
            pData->canup = 1;
            pData->candown =  1;
            pData->hTarget = GetParent ( hWnd );
            SetWindowAdditionalData2 (hWnd, (DWORD)pData);
        break;

        case MSG_DESTROY:
            free (pData);
        break;

        case MSG_SIZECHANGING:
        {
            const RECT* rcExpect = (const RECT*)wParam;
            RECT* rcResult = (RECT*)lParam;
            int w, h;

            rcResult->left = rcExpect->left;
            rcResult->top = rcExpect->top;
            GetSpinBoxSize (GetWindowStyle (hWnd), &w, &h);
            rcResult->right = rcExpect->left + w;
            rcResult->bottom = rcExpect->top + h;
            return 0;
        }

        case MSG_SIZECHANGED:
        {
            RECT* rcWin = (RECT*)wParam;
            RECT* rcClient = (RECT*)lParam;
            *rcClient = *rcWin;
            return 1;
        }

        case SPM_SETINFO: 
        {
            PSPININFO spinfo = (PSPININFO) lParam;
            if ( !spinfo )
                return -1;

            if (!(GetWindowStyle (hWnd) & SPS_AUTOSCROLL)) {
                memcpy ( &(pData->spinfo), spinfo, sizeof (SPININFO) );
            } else {
                if ( (spinfo->max >= spinfo->min &&
                        spinfo->max >= spinfo->cur &&
                        spinfo->cur >= spinfo->min )) {
                    memcpy ( &(pData->spinfo), spinfo, sizeof (SPININFO) );
                } else {
                    return -1;
                }
            }
            InvalidateRect ( hWnd, NULL, FALSE );
            return 0;
        }

        case SPM_GETINFO:
        {
            PSPININFO spinfo = (PSPININFO) lParam;
            if ( !spinfo )
                return -1;
            memcpy ( spinfo, &(pData->spinfo), sizeof (SPININFO) );
            return 0;
        }

        case SPM_SETCUR:
        {
            if (!(GetWindowStyle (hWnd) & SPS_AUTOSCROLL)) {
                pData->spinfo.cur = wParam;
            }
            else{
                if ( wParam > pData->spinfo.max || wParam < pData->spinfo.min)
                    return -1;
                pData->spinfo.cur = wParam;
            }
            InvalidateRect (hWnd, NULL, FALSE );
            return 0;
        }

        case SPM_GETCUR:
            return pData->spinfo.cur;

        case SPM_ENABLEUP:
            pData->canup = 1;
            InvalidateRect ( hWnd, NULL, FALSE );
            return 0;

        case SPM_ENABLEDOWN:
            pData->candown = 1;
            InvalidateRect ( hWnd, NULL, FALSE );
            return 0;

        case SPM_DISABLEUP:
            pData->canup = 0;
            InvalidateRect ( hWnd, NULL, FALSE );
            return 0;

        case SPM_DISABLEDOWN:
            pData->candown = 0;
            InvalidateRect ( hWnd, NULL, FALSE );
            return 0;

        case SPM_SETTARGET:
            pData->hTarget = (HWND) lParam;
            return 0;

        case SPM_GETTARGET:
            return pData->hTarget;

        case MSG_NCPAINT:
            return 0;

        case MSG_PAINT:
        {
            DWORD dwStyle = GetWindowStyle (hWnd);
            HDC hdc = BeginPaint (hWnd);
            if (dwStyle & SPS_HORIZONTAL)
                DrawSpinBoxHorz (hdc, &pData->spinfo, pData, dwStyle);
            else
                DrawSpinBoxVert (hdc, &pData->spinfo, pData, dwStyle);
            EndPaint (hWnd, hdc);
            return 0;
        }

        case MSG_LBUTTONDOWN:
        {
            int posx = LOWORD (lParam);
            int posy = HIWORD (lParam);
            int id = GetDlgCtrlID (hWnd);
            DWORD dwStyle = GetWindowStyle (hWnd);

            if ((dwStyle & SPS_TYPE_MASK) == SPS_TYPE_UPARROW) {
                OnUpArrow (hWnd, pData, id, dwStyle, wParam);
            }
            else if ((dwStyle & SPS_TYPE_MASK) == SPS_TYPE_DOWNARROW) {
                OnDownArrow (hWnd, pData, id, dwStyle, wParam);
            }
            else if ( ((dwStyle & SPS_HORIZONTAL) && (posx <= SPINBOX_HORZ_WIDTH))
                        || (!(dwStyle & SPS_HORIZONTAL) && (posy <= SPINBOX_VERT_HEIGHT)) ) {
                OnUpArrow (hWnd, pData, id, dwStyle, wParam);
            }
            else if ( ((dwStyle & SPS_HORIZONTAL) && (posx >= SPINBOX_HORZ_WIDTH + 2))
                        || (!(dwStyle & SPS_HORIZONTAL) && (posy >= SPINBOX_VERT_HEIGHT + 2)) ) {
                OnDownArrow (hWnd, pData, id, dwStyle, wParam);
            }

            InvalidateRect ( hWnd, NULL, FALSE );

            if (GetCapture () == hWnd)
                break;

            SetCapture (hWnd);
            SetAutoRepeatMessage (hWnd, message, wParam, lParam);
            break;
        }
            
        case MSG_LBUTTONUP:
            SetAutoRepeatMessage (0, 0, 0, 0);
            ReleaseCapture ();
            break;

        default:
            break;
    }

    return DefaultControlProc (hWnd, message, wParam, lParam);
}

#endif /* _EXT_CTRL_SPINBOX */

