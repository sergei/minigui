/* 
** $Id: window.c 8281 2007-11-27 09:18:50Z weiym $
**
** window.c: The Window module.
**
** Copyright (C) 2003 ~ 2007 Feynman Software.
** Copyright (C) 1999 ~ 2002 Wei Yongming.
**
** All rights reserved by Feynman Software.
**
** Current maintainer: Wei Yongming.
**
** Create date: 1999.04.19
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "common.h"
#include "minigui.h"
#include "gdi.h"
#include "window.h"
#include "control.h"
#include "cliprect.h"
#include "gal.h"
#include "internals.h"
#include "menu.h"
#include "ctrlclass.h"
#include "element.h"
#include "dc.h"

#if defined(_LITE_VERSION) && !defined(_STAND_ALONE)
#include "client.h"
#endif

/* function defined in menu module. */
void DrawMenuBarHelper (const MAINWIN *pWin, HDC hdc, const RECT* pClipRect);

/* functions defined in caret module. */
BOOL BlinkCaret (HWND hWnd);
void GetCaretBitmaps (PCARETINFO pCaretInfo);

/* this message will auto-repeat when MSG_IDLE received */
static MSG sg_repeat_msg = {0, 0, 0, 0};

void GUIAPI SetAutoRepeatMessage (HWND hwnd, int msg, 
                WPARAM wParam, LPARAM lParam)
{
    PMAINWIN pWin = (PMAINWIN)hwnd;

    /* if hwnd is zero, disable repeate message */
    if (hwnd == 0) {
        sg_repeat_msg.hwnd = 0;
        sg_repeat_msg.message  = 0;
        sg_repeat_msg.wParam = 0;
        sg_repeat_msg.lParam = 0;
        return;
    }
    else if (hwnd == HWND_INVALID || pWin->DataType != TYPE_HWND)
        return;

    sg_repeat_msg.hwnd = hwnd;
    sg_repeat_msg.message  = msg;
    sg_repeat_msg.wParam = wParam;
    sg_repeat_msg.lParam = lParam;
}

static void RecalcClientArea (HWND hWnd)
{
    PMAINWIN pWin = (PMAINWIN)hWnd;
    RECT rcWin, rcClient;
        
    memcpy (&rcWin, &pWin->left, sizeof (RECT));
    memcpy (&rcClient, &pWin->cl, sizeof (RECT));

    if (SendAsyncMessage (hWnd, MSG_SIZECHANGED, 
            (WPARAM)&rcWin, (LPARAM)&rcClient))
        memcpy (&pWin->cl, &rcClient, sizeof(RECT));

    SendNotifyMessage (hWnd, MSG_CSIZECHANGED, 
		    pWin->cr - pWin->cl, pWin->cb - pWin->ct);
}

static PCONTROL wndMouseInWhichControl (PMAINWIN pWin, int x, int y, 
                        int* UndHitCode)
{
    PCONTROL pCtrl;
    int hitcode;
   
    pCtrl = (PCONTROL)pWin->hPrimitive;
    if (pCtrl) {
        if (pCtrl->primitive) {
            if (UndHitCode)
                *UndHitCode = HT_CLIENT;
            return pCtrl;
        }
        else {
            hitcode = SendAsyncMessage ((HWND)pCtrl, MSG_HITTEST, 
                                        (WPARAM)x, (LPARAM)y);
            if (hitcode != HT_OUT && hitcode != HT_TRANSPARENT) {
                if (UndHitCode)
                    *UndHitCode = hitcode;
                return pCtrl;
            }
        }
    }

    pCtrl = (PCONTROL)(pWin->hFirstChild);
    while (pCtrl) {
        if ((pCtrl->dwStyle & WS_VISIBLE) 
                && PtInRect ((PRECT)(&pCtrl->left), x, y)) {
            hitcode = SendAsyncMessage((HWND)pCtrl, MSG_HITTEST, 
                                        (WPARAM)x, (LPARAM)y);
            if (hitcode != HT_OUT && hitcode != HT_TRANSPARENT) {
                if (UndHitCode) {
                    *UndHitCode = hitcode;
                }
                return pCtrl;
            }
        }

        pCtrl = pCtrl->next;
    }

    return NULL;
}

HWND GUIAPI GetWindowUnderCursor (void)
{
    POINT pt;
    PCONTROL pCtrl, pchild;
    int cx, cy;

    GetCursorPos (&pt);

    pCtrl = (PCONTROL)GetMainWindowPtrUnderPoint (pt.x, pt.y);
    if (!pCtrl) return 0;

    do {
        cx = pt.x; cy = pt.y;
        ScreenToClient ((HWND)pCtrl, &cx, &cy);
        pchild = wndMouseInWhichControl ((PMAINWIN)pCtrl, cx, cy, NULL);
        if (!pchild) break;
        pCtrl = pchild;
    } while (1);

    return (HWND)pCtrl;
}

/* NOTE:
** this function is CONTROL mouse messages handler,
** can automaticly capture mouse depend on HITTEST code.
*/

static PCONTROL __mgs_captured_ctrl = NULL;

void __mg_reset_mainwin_capture_info (PCONTROL ctrl)
{
    if ((ctrl == NULL || ctrl == __mgs_captured_ctrl)
            || (__mgs_captured_ctrl 
                    && __mgs_captured_ctrl->pMainWin == (PMAINWIN)ctrl)) {
        POINT mousePos;

        ReleaseCapture ();
        __mgs_captured_ctrl = NULL;

        GetCursorPos (&mousePos);
        PostMessage (HWND_DESKTOP, MSG_MOUSEMOVE, 
                        0, MAKELONG (mousePos.x, mousePos.y));
    }
}

static int DefaultDTMouseMsgHandler (PMAINWIN pWin, int message, 
                           WPARAM flags, int x, int y)
{
    PCONTROL control;
    int hc_mainwin = HT_UNKNOWN;
    int cx, cy;

    cx = x - pWin->cl;
    cy = y - pWin->ct;

    control = wndMouseInWhichControl (pWin, cx, cy, &hc_mainwin);

    if (control == NULL) {
        hc_mainwin = SendAsyncMessage((HWND)pWin, MSG_HITTEST, 
                    (WPARAM)x, (LPARAM)y);
        control = (PCONTROL)pWin;
    }
    else {
        hc_mainwin = HT_CLIENT;
    }
    
    switch (message) {
        case MSG_DT_MOUSEMOVE:
            if (hc_mainwin == HT_CLIENT)
                PostMessage ((HWND)pWin, MSG_SETCURSOR, 
                            0, MAKELONG (cx, cy));
            else
                PostMessage ((HWND)pWin, MSG_NCSETCURSOR, 
                            hc_mainwin, MAKELONG (cx, cy));
        case MSG_DT_LBUTTONDBLCLK:
        case MSG_DT_RBUTTONDBLCLK:
            if (hc_mainwin == HT_CLIENT) {
                PostMessage((HWND)pWin, 
                                message + (MSG_NCMOUSEOFF - MSG_DT_MOUSEOFF),
                                hc_mainwin, MAKELONG (x, y));
                PostMessage((HWND)pWin, 
                                message - MSG_DT_MOUSEOFF,
                                flags, MAKELONG (cx, cy));
            }
            else {
                PostMessage((HWND)pWin, 
                                message + (MSG_NCMOUSEOFF - MSG_DT_MOUSEOFF),
                                hc_mainwin, MAKELONG (x, y));
            }
            break;

        case MSG_DT_LBUTTONDOWN:
        case MSG_DT_RBUTTONDOWN:
            if (hc_mainwin != HT_CLIENT) {
                PostMessage ((HWND)pWin, 
                                message + (MSG_NCMOUSEOFF - MSG_DT_MOUSEOFF),
                                hc_mainwin, MAKELONG (x, y));
            }
            else {
                PostMessage((HWND)pWin,
                                message - MSG_DT_MOUSEOFF,
                                flags, MAKELONG(cx, cy));
            }
            break;

        case MSG_DT_LBUTTONUP:
        case MSG_DT_RBUTTONUP:
            if (hc_mainwin == HT_CLIENT) {
                PostMessage((HWND)pWin, 
                                message - MSG_DT_MOUSEOFF,
                                flags, MAKELONG(cx, cy));
            }
            else {
                PostMessage ((HWND)pWin, 
                                message + (MSG_NCMOUSEOFF - MSG_DT_MOUSEOFF),
                                hc_mainwin, MAKELONG (x, y));
            }
            break;
        
    }

    return 0;
}

static int DefaultMouseMsgHandler (PMAINWIN pWin, int message, 
                           WPARAM flags, int x, int y)
{
    static PMAINWIN __mgs_captured_win = NULL;
    PCONTROL pUnderPointer;
    int CapHitCode = HT_UNKNOWN;
    int UndHitCode = HT_UNKNOWN;
    int cx = 0, cy = 0;

    if (__mgs_captured_ctrl) {
        /* convert to parent window's client coordinates. */
        ScreenToClient ((HWND)__mgs_captured_win, &x, &y);

        CapHitCode = SendAsyncMessage((HWND)__mgs_captured_ctrl, MSG_HITTEST,
                                        (WPARAM)x, (LPARAM)y);
        
        pUnderPointer = NULL;
    }
    else {
        pUnderPointer = wndMouseInWhichControl (pWin, x, y, &UndHitCode);
        if (pUnderPointer && (pUnderPointer->dwStyle & WS_DISABLED))
            pUnderPointer = NULL;

        if (pUnderPointer) {
            cx = x - pUnderPointer->cl;
            cy = y - pUnderPointer->ct;
        }
    }

    switch (message) {
        case MSG_MOUSEMOVE:
            if (__mgs_captured_ctrl)
                PostMessage((HWND)__mgs_captured_ctrl, MSG_NCMOUSEMOVE, 
                                CapHitCode, MAKELONG (x, y));
            else {
                if (pWin->hOldUnderPointer != (HWND)pUnderPointer) {
                    if (pWin->hOldUnderPointer) {
                        PostMessage ((HWND)pWin->hOldUnderPointer,
                            MSG_MOUSEMOVEIN, FALSE, (LPARAM)pUnderPointer);
                        PostMessage ((HWND)pWin->hOldUnderPointer,
                            MSG_NCMOUSEMOVE, HT_OUT, MAKELONG (x, y));
                    }

                    if (pUnderPointer)
                        PostMessage ((HWND)pUnderPointer, MSG_MOUSEMOVEIN, 
                                        TRUE, (LPARAM)pWin->hOldUnderPointer);

                    pWin->hOldUnderPointer = (HWND)pUnderPointer;
                }

                if (pUnderPointer == NULL) {
                    pWin->hOldUnderPointer = 0;
                    break;
                }

                if (pUnderPointer->dwStyle & WS_DISABLED) {
                    SetCursor (GetSystemCursor (IDC_ARROW));
                    break;
                }

                if (UndHitCode == HT_CLIENT) {
                    PostMessage ((HWND)pUnderPointer,
                                    MSG_SETCURSOR, 0, MAKELONG (cx, cy));

                    PostMessage((HWND)pUnderPointer, MSG_NCMOUSEMOVE, 
                            UndHitCode, MAKELONG (x, y));
                    PostMessage((HWND)pUnderPointer, MSG_MOUSEMOVE, 
                            flags, MAKELONG (cx, cy));
                }
                else
                {
                    PostMessage((HWND)pUnderPointer, MSG_NCSETCURSOR, 
                            UndHitCode, MAKELONG (x, y));
                    PostMessage((HWND)pUnderPointer, MSG_NCMOUSEMOVE, 
                            UndHitCode, MAKELONG (x, y));
                }
            }
        break;

        case MSG_LBUTTONDOWN:
        case MSG_RBUTTONDOWN:
            if (pUnderPointer) {
                if (pUnderPointer->dwStyle & WS_DISABLED) {
                    Ping ();
                    break;
                }

                SendNotifyMessage ((HWND) pUnderPointer, 
                                MSG_MOUSEACTIVE, UndHitCode, 0);

                if (UndHitCode != HT_CLIENT) {
                    if (UndHitCode & HT_NEEDCAPTURE
                                    && message == MSG_LBUTTONDOWN) {
                        SetCapture ((HWND)pWin);
                        __mgs_captured_win = pWin;
                        __mgs_captured_ctrl = pUnderPointer;
                    }
                    else
                        __mgs_captured_ctrl = NULL;

                    PostMessage ((HWND)pUnderPointer, message + MSG_NCMOUSEOFF,
                                    UndHitCode, MAKELONG (x, y));
                }
                else if (__mgs_captured_ctrl == NULL) {
                    PostMessage((HWND)pUnderPointer, message, 
                        flags, MAKELONG(cx, cy));
                }
            }
            else if (pWin->hActiveChild) {
                SendNotifyMessage (pWin->hActiveChild, 
                                MSG_MOUSEACTIVE, HT_OUT, 0);
                PostMessage (pWin->hActiveChild, message + MSG_NCMOUSEOFF, 
                                HT_OUT, MAKELONG(x, y));
            }
        break;

        case MSG_RBUTTONUP:
        case MSG_LBUTTONUP:
            if (__mgs_captured_ctrl && message == MSG_LBUTTONUP) {
                PostMessage ((HWND)__mgs_captured_ctrl, 
                                message + MSG_NCMOUSEOFF, 
                                CapHitCode, MAKELONG (x, y));
                ReleaseCapture ();
                __mgs_captured_win = NULL;
                __mgs_captured_ctrl = NULL;
            }
            else if (pUnderPointer) {
                if (pUnderPointer->dwStyle & WS_DISABLED) {
                    break;
                }

                if (UndHitCode == HT_CLIENT)
                    PostMessage((HWND)pUnderPointer, message, 
                            flags, MAKELONG (cx, cy));
                else
                    PostMessage((HWND)pUnderPointer, message + MSG_NCMOUSEOFF, 
                            UndHitCode, MAKELONG (x, y));
            }
            else if (pWin->hActiveChild) {
                PostMessage(pWin->hActiveChild, message + MSG_NCMOUSEOFF, 
                        HT_OUT, MAKELONG(x, y));
            }
        break;
        
        case MSG_LBUTTONDBLCLK:
        case MSG_RBUTTONDBLCLK:
            if (pUnderPointer) {
                if (pUnderPointer->dwStyle & WS_DISABLED) {
                    Ping ();
                    break;
                }

                if (UndHitCode == HT_CLIENT)
                    PostMessage((HWND)pUnderPointer, message, 
                        flags, MAKELONG(cx, cy));
                else
                    PostMessage((HWND)pUnderPointer, message + MSG_NCMOUSEOFF, 
                        UndHitCode, MAKELONG (x, y));
            }
            else {
                if (pWin->hActiveChild) {
                    PostMessage(pWin->hActiveChild, message + MSG_NCMOUSEOFF, 
                        HT_OUT, MAKELONG(x, y));
                }
            }
        break;

    }

    return 0;
}

static inline int wndGetBorder (const MAINWIN* pWin)
{
    if (pWin->dwStyle & WS_BORDER) 
        return GetMainWinMetrics(MWM_BORDER);
    else if (pWin->dwStyle & WS_THICKFRAME)
        return GetMainWinMetrics(MWM_THICKFRAME);
    else if (pWin->dwStyle & WS_THINFRAME)
        return GetMainWinMetrics (MWM_THINFRAME);

    return 0;
}


static BOOL wndGetVScrollBarRect (const MAINWIN* pWin, 
                RECT* rcVBar)
{
    if (pWin->dwStyle & WS_VSCROLL) {
        int iBorder = wndGetBorder (pWin);

        rcVBar->left = pWin->right - GetMainWinMetrics (MWM_CXVSCROLL) 
                        - iBorder;
        rcVBar->right = pWin->right - iBorder;

        rcVBar->top  = pWin->ct;
        rcVBar->bottom = pWin->bottom - iBorder;

        if (pWin->dwStyle & WS_HSCROLL && !(pWin->hscroll.status & SBS_HIDE))
            rcVBar->bottom -= GetMainWinMetrics (MWM_CYHSCROLL);
        
        return TRUE;
    }
    
    return FALSE;
}

static BOOL wndGetHScrollBarRect (const MAINWIN* pWin, 
                RECT* rcHBar)
{
    if (pWin->dwStyle & WS_HSCROLL) {
        int iBorder = wndGetBorder (pWin);

        rcHBar->top = pWin->bottom - GetMainWinMetrics (MWM_CYHSCROLL)
                        - iBorder;
        rcHBar->bottom = pWin->bottom - iBorder;

        rcHBar->left  = pWin->cl;
        rcHBar->right = pWin->right - iBorder;

        if (pWin->dwStyle & WS_VSCROLL && !(pWin->vscroll.status & SBS_HIDE))
            rcHBar->right -= GetMainWinMetrics (MWM_CXVSCROLL);

        return TRUE;
    }
    
    return FALSE;
    
}

static int wndGetHScrollBarPos (PMAINWIN pWin, int x, int y)
{
    RECT rcBar;
    RECT rcArea;

    if (pWin->hscroll.status & SBS_DISABLED)
        return SBPOS_UNKNOWN;

    wndGetHScrollBarRect (pWin, &rcBar);

    if (!PtInRect (&rcBar, x, y))
        return SBPOS_UNKNOWN;

    rcArea.top  = rcBar.top;
    rcArea.bottom = rcBar.bottom;

    /* Left arrow area */
    rcArea.left = rcBar.left;
    rcArea.right = rcArea.left + GetMainWinMetrics (MWM_CXHSCROLL);

    if (PtInRect (&rcArea, x, y))
        return SBPOS_LEFTARROW;

    /* Right arrow area */
    rcArea.left = rcBar.right - GetMainWinMetrics (MWM_CXHSCROLL);
    rcArea.right = rcBar.right;

    if (PtInRect (&rcArea, x, y))
        return SBPOS_RIGHTARROW;


    if (x < (rcBar.left + pWin->hscroll.barStart 
            + GetMainWinMetrics (MWM_CXHSCROLL)))
        return SBPOS_LEFTSPACE;

    if (x > (rcBar.left + pWin->hscroll.barStart + pWin->hscroll.barLen
            + GetMainWinMetrics (MWM_CXHSCROLL)))
        return SBPOS_RIGHTSPACE;

    return SBPOS_THUMB;
}

static int wndGetVScrollBarPos (PMAINWIN pWin, int x, int y)
{
    RECT rcBar;
    RECT rcArea;

    if (pWin->vscroll.status & SBS_DISABLED)
        return SBPOS_UNKNOWN;

    wndGetVScrollBarRect (pWin, &rcBar);

    if (!PtInRect (&rcBar, x, y))
        return SBPOS_UNKNOWN;

    rcArea.left  = rcBar.left;
    rcArea.right = rcBar.right;

    /* Left arrow area */
    rcArea.top = rcBar.top;
    rcArea.bottom = rcArea.top + GetMainWinMetrics (MWM_CYVSCROLL);

    if (PtInRect (&rcArea, x, y))
        return SBPOS_UPARROW;

    /* Right arrow area */
    rcArea.top = rcBar.bottom - GetMainWinMetrics (MWM_CYVSCROLL);
    rcArea.bottom = rcBar.bottom;

    if (PtInRect (&rcArea, x, y))
        return SBPOS_DOWNARROW;

    if (y < (rcBar.top + pWin->vscroll.barStart
            + GetMainWinMetrics (MWM_CYVSCROLL)))
        return SBPOS_UPSPACE;

    if (y > (rcBar.top + pWin->vscroll.barStart + pWin->vscroll.barLen
            + GetMainWinMetrics (MWM_CYVSCROLL)))
        return SBPOS_DOWNSPACE;

    return SBPOS_THUMB;
}

#ifdef _PC3D_WINDOW_STYLE
static BOOL sbGetSBarArrowPos (PMAINWIN pWin, int location, 
                            int* x, int* y, int* w, int* h)
{
    RECT rcBar;

    if (location < SBPOS_UPARROW) 
        wndGetHScrollBarRect (pWin, &rcBar);
    else
        wndGetVScrollBarRect (pWin, &rcBar);

    *w = GetMainWinMetrics (MWM_CXHSCROLL);
    *h = GetMainWinMetrics (MWM_CYHSCROLL);
    switch (location) {
        case SBPOS_LEFTARROW:
            *x = rcBar.left;
            *y = rcBar.top;
        break;
        
        case SBPOS_RIGHTARROW:
            *x = rcBar.right - GetMainWinMetrics (MWM_CXHSCROLL) ;
            *y = rcBar.top;
        break;

        case SBPOS_UPARROW:
            *x = rcBar.left;
            *y = rcBar.top;
        break;

        case SBPOS_DOWNARROW:
            *x = rcBar.left;
            *y = rcBar.bottom - GetMainWinMetrics (MWM_CYVSCROLL);
        break;

        default:
        return FALSE;
    }

    *x -= pWin->left;
    *y -= pWin->top;

    return TRUE;
}

static BOOL sbGetButtonPos (PMAINWIN pWin, int location, 
                            int* x, int* y, int* w, int* h)
{
    RECT rc;
    int iBorder;
    int iCaption = 0, bCaption;
    int iIconX = 0;
    int iIconY = 0;

    /* scroll bar position */
    if (location & SBPOS_MASK)
        return sbGetSBarArrowPos (pWin, location, x, y, w, h);

    iBorder = wndGetBorder (pWin);

    if (!(pWin->dwStyle & WS_CAPTION))
         return FALSE;

    if (pWin->hIcon)
    {
        iIconX = GetMainWinMetrics(MWM_ICONX);
        iIconY = GetMainWinMetrics(MWM_ICONY);
    }

    iCaption = GetMainWinMetrics(MWM_CAPTIONY);
    bCaption = iBorder + iCaption - 1;

    rc.left = iBorder;
    rc.top = iBorder;
    rc.right = pWin->right - pWin->left - iBorder;
    rc.bottom = iBorder + iCaption;
                    
    /* close button */
    *x = rc.right - GetMainWinMetrics (MWM_SB_WIDTH) - GetMainWinMetrics (MWM_SB_INTERX);
    *y = GetMainWinMetrics (MWM_SB_HEIGHT);
    *w = GetMainWinMetrics (MWM_SB_WIDTH);
    if (*y < bCaption) {
        *y = iBorder + ((bCaption - *y)>>1);
        *h = GetMainWinMetrics (MWM_SB_HEIGHT);
    }
    else {
        *y = iBorder;
        *h = iIconY;
    }

    if (!(pWin->dwExStyle & WS_EX_NOCLOSEBOX)) {
        if (location == HT_CLOSEBUTTON)
            return TRUE;

        *x -= GetMainWinMetrics (MWM_SB_WIDTH);
        *x -= GetMainWinMetrics (MWM_SB_INTERX)<<1;
    }

    if (pWin->dwStyle & WS_MAXIMIZEBOX) {
        /* restore/maximize button */
        if (location == HT_MAXBUTTON)
            return TRUE;

        *x -= GetMainWinMetrics (MWM_SB_WIDTH);
        *x -= GetMainWinMetrics (MWM_SB_INTERX);
    }

    if (pWin->dwStyle & WS_MINIMIZEBOX) {
        /* minimize button. */
        if (location == HT_MINBUTTON)
            return TRUE;
    }

    return FALSE;
}

static BOOL sbDownButton (PMAINWIN pWin, int downCode)
{
    HDC hdc;
    int x, y, w, h;
    
    if (!sbGetButtonPos (pWin, downCode, &x, &y, &w, &h)) {
        return FALSE;
    }
    
    hdc = GetDC ((HWND)pWin);

    w += x - 1;
    h += y - 1;
#if 0
    Draw3DDownFrame (hdc, x, y, w, h, 0);
#else
    SetPenColor (hdc, GetWindowElementColorEx ((HWND)pWin, WEC_FLAT_BORDER));
    Rectangle (hdc, x, y, w, h);
    SetPenColor (hdc, GetWindowElementColorEx ((HWND)pWin, BKC_CONTROL_DEF));
    Rectangle (hdc, x + 1, y + 1, w - 1, h - 1);
#endif
    ReleaseDC (hdc);

    return TRUE;
}

static BOOL sbUpButton (PMAINWIN pWin, int downCode)
{
    HDC hdc;
    int x, y, w, h;
    PBITMAP bmp = NULL;
    int xo = 0, yo = 0, bw = 0, bh = 0, r = 0, xos = 0, yos = 0;
    float scaleX = 0.0f, scaleY = 0.0f;
    
    if (!sbGetButtonPos (pWin, downCode, &x, &y, &w, &h))
        return FALSE;

    hdc = GetDC ((HWND)pWin);
    switch (downCode) {
        case HT_MAXBUTTON:
            bmp = GetSystemBitmap (SYSBMP_CAPBTNS);
            bw = bmp->bmWidth >> 2;
            bh = bmp->bmHeight;
            xo = yo = 0;
            break;
        case HT_MINBUTTON:
            bmp = GetSystemBitmap (SYSBMP_CAPBTNS);
            bw = bmp->bmWidth >> 2;
            bh = bmp->bmHeight;
            xo = bw; yo = 0;
            break;
        case HT_CLOSEBUTTON:
            bmp = GetSystemBitmap (SYSBMP_CAPBTNS);
            bw = bmp->bmWidth >> 2;
            bh = bmp->bmHeight;
            xo = (bw << 1) + bw; yo = 0;
            break;

        case SBPOS_UPARROW:
            if ((r = GetMainWinMetrics (MWM_CYVSCROLL)) != 0) {
                bmp = GetSystemBitmap (SYSBMP_ARROWS);
                bw = bmp->bmWidth >> 2;
                bh = bmp->bmHeight >> 1;
                xo = yo = 0;
            }
            break;
        case SBPOS_DOWNARROW:
            if ((r = GetMainWinMetrics (MWM_CYVSCROLL)) != 0) {
                bmp = GetSystemBitmap (SYSBMP_ARROWS);
                bw = bmp->bmWidth >> 2;
                bh = bmp->bmHeight >> 1;
                xo = bw; yo = 0;
            }
            break;
        case SBPOS_LEFTARROW:
            if ((r = GetMainWinMetrics (MWM_CXHSCROLL)) != 0) {
                bmp = GetSystemBitmap (SYSBMP_ARROWS);
                bw = bmp->bmWidth >> 2;
                bh = bmp->bmHeight >> 1;
                xo = bw << 1; yo = 0;
            }
            break;
        case SBPOS_RIGHTARROW:
            if ((r = GetMainWinMetrics (MWM_CXHSCROLL)) != 0) {
                bmp = GetSystemBitmap (SYSBMP_ARROWS);
                bw = bmp->bmWidth >> 2;
                bh = bmp->bmHeight >> 1;
                xo = (bw << 1) + bw; yo = 0;
            }
            break;
    }



    if (bmp) {
        scaleX = (float)r/(float)bw;
        scaleY = (float)r/(float)bh;
        w = (int)ceil(bmp->bmWidth*scaleX);
        h = (int)ceil(bmp->bmHeight*scaleY);
        xos = (int)ceil(xo*scaleX); 
        yos = (int)ceil(yo*scaleY); 
        FillBoxWithBitmapPart (hdc, x, y, r, r, w, h, bmp, xos, yos);
    }
    ReleaseDC (hdc);

    return TRUE;
}

#else

#define sbDownButton(pWin, downCode)
#define sbUpButton(pWin, downCode)

#endif

int MenuBarHitTest (HWND hwnd, int x, int y);

static void wndTrackMenuBarOnMouseMove(PMAINWIN pWin, int message, 
                                    int location, int x, int y)
{
    PMENUBAR pMenuBar;
    int oldBarItem;
    int barItem;

    pMenuBar = (PMENUBAR)(pWin->hMenu);
    if (!pMenuBar) return;

    oldBarItem = pMenuBar->hilitedItem;
    
    if (location == HT_OUT) {
        if (oldBarItem >= 0)
            HiliteMenuBarItem ((HWND)pWin, oldBarItem, HMF_DEFAULT);
        return;
    }

    barItem = MenuBarHitTest ((HWND)pWin, x, y);

    if (barItem != oldBarItem) {

        if (oldBarItem >= 0)
            HiliteMenuBarItem ((HWND)pWin, oldBarItem, HMF_DEFAULT);

        if (barItem >= 0) {
            HiliteMenuBarItem ((HWND)pWin, barItem, HMF_UPITEM);
            pMenuBar->hilitedItem = barItem;
            pMenuBar->hiliteFlag = HMF_UPITEM;
        }
        else
            pMenuBar->hilitedItem = -1;
    }
}

static int wndHandleHScrollBar (PMAINWIN pWin, int message, int x, int y)
{
    static int downPos = SBPOS_UNKNOWN;
    static int movePos = SBPOS_UNKNOWN;
    static int sbCode;
    static int oldBarStart;
    static int oldThumbPos;
    static int oldx;

    int curPos;
    RECT rcBar;

    wndGetHScrollBarRect (pWin, &rcBar);
    rcBar.left += GetMainWinMetrics (MWM_CXHSCROLL);
    rcBar.right -= GetMainWinMetrics (MWM_CXHSCROLL);

    curPos = wndGetHScrollBarPos (pWin, x, y);
    if (curPos == SBPOS_UNKNOWN && downPos == SBPOS_UNKNOWN)
        return 0;
    
    switch( message )
    {
        case MSG_NCLBUTTONDOWN:
            oldBarStart = pWin->hscroll.barStart;
            oldThumbPos = pWin->hscroll.curPos;
            oldx = x;

            downPos = curPos;
            movePos = curPos;
            if (curPos == SBPOS_LEFTARROW) {
                sbDownButton (pWin, curPos);
                if (pWin->hscroll.curPos == pWin->hscroll.minPos)
                    break;

                sbCode = SB_LINELEFT;
            }
            else if (curPos == SBPOS_RIGHTARROW) {
                sbDownButton (pWin, curPos);
                if (pWin->hscroll.curPos == pWin->hscroll.maxPos)
                    break;
                
                sbCode = SB_LINERIGHT;
            }
            else if (curPos == SBPOS_LEFTSPACE) {
                if (pWin->hscroll.curPos == pWin->hscroll.minPos)
                    break;

                sbCode = SB_PAGELEFT;
            }
            else if (curPos == SBPOS_RIGHTSPACE) {
                if (pWin->hscroll.curPos == pWin->hscroll.maxPos)
                    break;
                
                sbCode = SB_PAGERIGHT;
            }
            else if (curPos == SBPOS_THUMB) {
                sbCode = SB_THUMBTRACK;
                break;
            }
            SendNotifyMessage ((HWND)pWin, MSG_HSCROLL, sbCode, x);
            SetAutoRepeatMessage ((HWND)pWin, MSG_HSCROLL, sbCode, x);
            break;

        case MSG_NCLBUTTONUP:
            if (sbCode == SB_THUMBTRACK && downPos == SBPOS_THUMB) {
                int newBarStart = oldBarStart + x - oldx;
                int newThumbPos = newBarStart
                    * (pWin->hscroll.maxPos - pWin->hscroll.minPos + 1) 
                    / (rcBar.right - rcBar.left) + pWin->hscroll.minPos;
                    
                if (newThumbPos < pWin->hscroll.minPos)
                    newThumbPos = pWin->hscroll.minPos;
                if (newThumbPos > pWin->hscroll.maxPos)
                    newThumbPos = pWin->hscroll.maxPos;
                if (newBarStart != oldBarStart)
                    SendNotifyMessage ((HWND)pWin,
                        MSG_HSCROLL, SB_THUMBPOSITION, newThumbPos);

                downPos = SBPOS_UNKNOWN;
                movePos = SBPOS_UNKNOWN;
                return -1;
            }
            
            if (downPos != SBPOS_UNKNOWN) {
                sbUpButton (pWin, downPos);
                SendNotifyMessage ((HWND)pWin, MSG_HSCROLL, SB_ENDSCROLL, 0);
                /* cancel repeat message */
                SetAutoRepeatMessage (0, 0, 0, 0);
            }

            downPos = SBPOS_UNKNOWN;
            movePos = SBPOS_UNKNOWN;
            return -1;
    
        case MSG_NCMOUSEMOVE:
            if (sbCode == SB_THUMBTRACK && downPos == SBPOS_THUMB) {
                int newBarStart = oldBarStart + x - oldx;
                int newThumbPos = newBarStart
                    * (pWin->hscroll.maxPos - pWin->hscroll.minPos + 1) 
                    / (rcBar.right - rcBar.left) + pWin->hscroll.minPos;

                
                if (newThumbPos < pWin->hscroll.minPos)
                    newThumbPos = pWin->hscroll.minPos;
                if (newThumbPos > pWin->hscroll.maxPos)
                    newThumbPos = pWin->hscroll.maxPos;
                if (newThumbPos != oldThumbPos) {
                    SendNotifyMessage ((HWND)pWin,
                        MSG_HSCROLL, SB_THUMBTRACK, newThumbPos);
                    oldThumbPos = newThumbPos;
                }
                movePos = curPos;
                break;
            }

            if (movePos == downPos && curPos != downPos)
                sbUpButton (pWin, downPos);
            else if (movePos != downPos && curPos == downPos)
                sbDownButton (pWin, downPos);
            movePos = curPos;
            break;
    }

    return 1;
}

static int wndHandleVScrollBar (PMAINWIN pWin, int message, int x, int y)
{
    static int downPos = SBPOS_UNKNOWN;
    static int movePos = SBPOS_UNKNOWN;
    static int sbCode;
    static int oldBarStart;
    static int oldThumbPos;
    static int oldy;
    int curPos;
    RECT rcBar;
    int newBarStart;
    int newThumbPos;

    wndGetVScrollBarRect (pWin, &rcBar);
    rcBar.top += GetMainWinMetrics (MWM_CYVSCROLL);
    rcBar.bottom -= GetMainWinMetrics (MWM_CYVSCROLL);

    curPos = wndGetVScrollBarPos (pWin, x, y);

    if (curPos == SBPOS_UNKNOWN && downPos == SBPOS_UNKNOWN)
        return 0;
    
    switch (message)
    {
        case MSG_NCLBUTTONDOWN:
            oldBarStart = pWin->vscroll.barStart;
            oldThumbPos = pWin->vscroll.curPos;
            oldy = y;
            downPos = curPos;
            movePos = curPos;
            if (curPos == SBPOS_UPARROW) {
                sbDownButton (pWin, curPos);
                if (pWin->vscroll.curPos == pWin->vscroll.minPos)
                    break;

                sbCode = SB_LINEUP;
            }
            else if (curPos == SBPOS_DOWNARROW) {
                sbDownButton (pWin, curPos);
                if (pWin->vscroll.curPos == pWin->vscroll.maxPos)
                    break;

                sbCode = SB_LINEDOWN;
            }
            else if (curPos == SBPOS_UPSPACE) {
                if (pWin->vscroll.curPos == pWin->vscroll.minPos)
                    break;

                sbCode = SB_PAGEUP;
            }
            else if (curPos == SBPOS_DOWNSPACE) {
                if (pWin->vscroll.curPos == pWin->vscroll.maxPos)
                    break;

                sbCode = SB_PAGEDOWN;
            }
            else if (curPos == SBPOS_THUMB) {
                sbCode = SB_THUMBTRACK;
                break;
            }
            SendNotifyMessage ((HWND)pWin, MSG_VSCROLL, sbCode, y);
            SetAutoRepeatMessage ((HWND)pWin, MSG_VSCROLL, sbCode, y);
            break;

        case MSG_NCLBUTTONUP:
            if (sbCode == SB_THUMBTRACK && downPos == SBPOS_THUMB) {
                newBarStart = oldBarStart + y - oldy;
                newThumbPos = newBarStart
                    * (pWin->vscroll.maxPos - pWin->vscroll.minPos + 1) 
                    / (rcBar.bottom - rcBar.top) + pWin->vscroll.minPos;

                if (newThumbPos < pWin->vscroll.minPos)
                    newThumbPos = pWin->vscroll.minPos;
                if (newThumbPos > pWin->vscroll.maxPos)
                    newThumbPos = pWin->vscroll.maxPos;
                if (newBarStart != oldBarStart)
                    SendNotifyMessage ((HWND)pWin,
                        MSG_VSCROLL, SB_THUMBPOSITION, newThumbPos);
                        
                downPos = SBPOS_UNKNOWN;
                movePos = SBPOS_UNKNOWN;
                return -1;
            }

            if (downPos != SBPOS_UNKNOWN) {
                sbUpButton (pWin, downPos);
                SendNotifyMessage ((HWND)pWin, MSG_VSCROLL, SB_ENDSCROLL, 0);
                /* cancel repeat message */
                SetAutoRepeatMessage (0, 0, 0, 0);
            }

            downPos = SBPOS_UNKNOWN;
            movePos = SBPOS_UNKNOWN;
            return -1;
    
        case MSG_NCMOUSEMOVE:
            if (sbCode == SB_THUMBTRACK && downPos == SBPOS_THUMB) {
                newBarStart = oldBarStart + y - oldy;
                newThumbPos = newBarStart
                        * (pWin->vscroll.maxPos - pWin->vscroll.minPos + 1) 
                        / (rcBar.bottom - rcBar.top) + pWin->vscroll.minPos;
                    
                if (newThumbPos < pWin->vscroll.minPos)
                    newThumbPos = pWin->vscroll.minPos;
                if (newThumbPos > pWin->vscroll.maxPos)
                    newThumbPos = pWin->vscroll.maxPos;
                if (newThumbPos != oldThumbPos) {
                    SendNotifyMessage ((HWND)pWin,
                        MSG_VSCROLL, SB_THUMBTRACK, newThumbPos);
                    oldThumbPos = newThumbPos;
                }
                movePos = curPos;
                break;
            }
            
            if (movePos == downPos && curPos != downPos)
                sbUpButton (pWin, downPos);
            else if (movePos != downPos && curPos == downPos)
                sbDownButton (pWin, downPos);
            movePos = curPos;
            break;
    }

    return 1;
}

/* this function is CONTROL safe. */
static int DefaultNCMouseMsgHandler(PMAINWIN pWin, int message, 
                           int location, int x, int y)
{
    static PMAINWIN downWin  = NULL;
    static int downCode = HT_UNKNOWN;
    static int moveCode = HT_UNKNOWN;
    int barItem;
    
    if (pWin->WinType == TYPE_MAINWIN && message == MSG_NCMOUSEMOVE)
        wndTrackMenuBarOnMouseMove (pWin, message, location, x, y);

    if (pWin->dwStyle & WS_HSCROLL) {
        switch (wndHandleHScrollBar (pWin, message, x, y)) {
            case 1:
                return 0;
            case -1:
                downCode = HT_UNKNOWN;
                moveCode = HT_UNKNOWN;
                return 0;
        }
    }
    
    if (pWin->dwStyle & WS_VSCROLL) {
        switch (wndHandleVScrollBar (pWin, message, x, y)) {
            case 1:
                return 0;
            case -1:
                downCode = HT_UNKNOWN;
                moveCode = HT_UNKNOWN;
                return 0;
        }
    }

    switch (message)
    {
        case MSG_NCLBUTTONDOWN:
            
            if (location == HT_MENUBAR) {
                barItem = MenuBarHitTest ((HWND)pWin, x, y);
                if (barItem >= 0)
                    TrackMenuBar ((HWND)pWin, barItem);

                return 0;
            }
            else if (location & HT_DRAGGABLE) {
#ifdef _MOVE_WINDOW_BY_MOUSE
                DRAGINFO drag_info;

                drag_info.location = location;
                drag_info.init_x = x;
                drag_info.init_y = y;
                SendMessage (HWND_DESKTOP, MSG_STARTDRAGWIN, 
                                (WPARAM)pWin, (LPARAM)&drag_info);
#endif
            }
            else {
                downCode = location;
                moveCode = location;
                downWin  = pWin;
                
                sbDownButton (pWin, downCode);
            }
            break;

        case MSG_NCMOUSEMOVE:
            if (pWin->hOldUnderPointer && location == HT_OUT) {
                PostMessage (pWin->hOldUnderPointer,
                            MSG_MOUSEMOVEIN, FALSE, 0);
                PostMessage (pWin->hOldUnderPointer,
                            MSG_NCMOUSEMOVE, HT_OUT, MAKELONG (x, y));
                pWin->hOldUnderPointer = 0;
            }

            if (downCode != HT_UNKNOWN) { 
                if (moveCode == downCode && location != downCode) {
                    sbUpButton (pWin, downCode);
                    moveCode = location;
                }
                else if (moveCode != downCode && location == downCode) {
                    sbDownButton (pWin, downCode);
                    moveCode = location;
                }
            }

            if (location != HT_CLIENT && downCode == HT_UNKNOWN)
                PostMessage((HWND)pWin, MSG_NCSETCURSOR, 
                            location, MAKELONG (x, y));
            break;

        case MSG_NCLBUTTONUP:
            if (downCode & HT_DRAGGABLE) {
#ifdef _MOVE_WINDOW_BY_MOUSE
                SendMessage (HWND_DESKTOP, MSG_CANCELDRAGWIN, (WPARAM)pWin, 0L);
#endif
            }
            else if (location == downCode) {
                sbUpButton (pWin, downCode);
                switch (location) {
                    case HT_CLOSEBUTTON:
                        SendNotifyMessage ((HWND)pWin, MSG_CLOSE, 0, 0);
                    break;

                    case HT_MAXBUTTON:
                        SendNotifyMessage ((HWND)pWin, MSG_MAXIMIZE, 0, 0);
                    break;

                    case HT_MINBUTTON:
                        SendNotifyMessage ((HWND)pWin, MSG_MINIMIZE, 0, 0);
                    break;

                    case HT_ICON:
                        if (pWin->hSysMenu)
                            TrackPopupMenu (pWin->hSysMenu, 
                                TPM_SYSCMD, x, y, (HWND)pWin);
                    break;

                    case HT_CAPTION:
                    break;

                }
            }
            downCode = HT_UNKNOWN;
            moveCode = HT_UNKNOWN;
            downWin  = NULL;
            break;
            
        case MSG_NCRBUTTONUP:
            if (location == HT_CAPTION && pWin->hSysMenu)
                TrackPopupMenu (pWin->hSysMenu, TPM_SYSCMD, x, y, (HWND)pWin);
            break;
            
        case MSG_NCLBUTTONDBLCLK:
            if (location == HT_ICON)
                SendNotifyMessage ((HWND)pWin, MSG_CLOSE, 0, 0);
/*            else if (location == HT_CAPTION) */
/*                SendNotifyMessage ((HWND)pWin, MSG_MAXIMIZE, 0, 0); */
            break;

/*        case MSG_NCRBUTTONDOWN: */
/*        case MSG_NCRBUTTONDBLCLK: */
/*            break; */

    }

    return 0;
}

static int DefaultKeyMsgHandler (PMAINWIN pWin, int message, 
                           WPARAM wParam, LPARAM lParam)
{
/*
** NOTE:
** Currently only handle one message.
** Should handle fowllowing messages:
**
** MSG_KEYDOWN,
** MSG_KEYUP,
** MSG_CHAR,
** MSG_SYSKEYDOWN,
** MSG_SYSCHAR.
*/
    if (message == MSG_KEYDOWN 
                    || message == MSG_KEYUP 
                    || message == MSG_KEYLONGPRESS 
                    || message == MSG_KEYALWAYSPRESS 
                    || message == MSG_CHAR) {
        if (pWin->hActiveChild
                && !(pWin->dwStyle & WS_DISABLED)) {
            SendMessage (pWin->hActiveChild, message, wParam, lParam);
        }
    }
    else if (message == MSG_SYSKEYUP) {
       if (pWin->WinType == TYPE_MAINWIN
                && !(pWin->dwStyle & WS_DISABLED))
           TrackMenuBar ((HWND)pWin, 0);
    }

    return 0;
}

static int DefaultCreateMsgHandler(PMAINWIN pWin, int message, 
                           WPARAM wParam, LPARAM lParam)
{

/*
** NOTE:
** Currently does nothing.
** Should handle fowllowing messages:
**
** MSG_NCCREATE,
** MSG_CREATE,
** MSG_INITPANES,
** MSG_DESTROYPANES,
** MSG_DESTROY,
** MSG_NCDESTROY.
*/
    return 0;
}

static void wndScrollBarPos (PMAINWIN pWin, BOOL bIsHBar, RECT* rcBar)
{
    UINT moveRange;
    PSCROLLBARINFO pSBar;

    if (bIsHBar)
        pSBar = &pWin->hscroll;
    else
        pSBar = &pWin->vscroll;

    if (pSBar->minPos == pSBar->maxPos) {
        pSBar->status |= SBS_HIDE;
        return;
    }

    if (bIsHBar)
        moveRange = RECTWP (rcBar) - (GetMainWinMetrics (MWM_CXHSCROLL)<<1);
    else
        moveRange = RECTHP (rcBar) - (GetMainWinMetrics (MWM_CYVSCROLL)<<1);
    if (pSBar->pageStep == 0) {
        pSBar->barLen = GetMainWinMetrics (MWM_DEFBARLEN);

        if (pSBar->barLen > moveRange)
            pSBar->barLen = GetMainWinMetrics (MWM_MINBARLEN);
    }
    else {
#ifndef _USE_FIXED_SB_BARLEN
        pSBar->barLen = (int) (moveRange*pSBar->pageStep * 1.0f/
                               (pSBar->maxPos - pSBar->minPos + 1) + 0.5);
#else
        pSBar->barLen = GetMainWinMetrics (MWM_DEFBARLEN);
#endif
        if (pSBar->barLen < GetMainWinMetrics (MWM_MINBARLEN))
            pSBar->barLen = GetMainWinMetrics (MWM_MINBARLEN);
    }

#ifndef _USE_FIXED_SB_BARLEN
    pSBar->barStart = (int) (moveRange*(pSBar->curPos - pSBar->minPos) * 1.0f/
                               (pSBar->maxPos - pSBar->minPos + 1) + 0.5);
#else
    pSBar->barStart = (int) ( (moveRange - pSBar->barLen) * (pSBar->curPos - pSBar->minPos) * 1.0f /
                               (pSBar->maxPos - pSBar->minPos + 1 - pSBar->pageStep) + 0.5 );
#endif

    if (pSBar->barStart + pSBar->barLen > moveRange)
        pSBar->barStart = moveRange - pSBar->barLen;
    if (pSBar->barStart < 0)
        pSBar->barStart = 0;
}

/* This function is CONTROL safe. */
static void OnChangeSize (PMAINWIN pWin, PRECT pDestRect, PRECT pResultRect)
{
    int iBorder = 0;
    int iCaptionY = 0;
    int iIconX = 0;
    int iIconY = 0;
    int iMenuY = 0;

    iBorder = wndGetBorder (pWin);

    if( pWin->dwStyle & WS_CAPTION )
    {
        iCaptionY = GetMainWinMetrics(MWM_CAPTIONY);

        if (pWin->WinType == TYPE_MAINWIN && pWin->hIcon) {
            iIconX = GetMainWinMetrics(MWM_ICONX);
            iIconY = GetMainWinMetrics(MWM_ICONY);
        }
    }

    if (pWin->WinType == TYPE_MAINWIN && pWin->hMenu) {
        iMenuY = GetMainWinMetrics (MWM_MENUBARY);
        iMenuY += GetMainWinMetrics (MWM_MENUBAROFFY)<<1;
    }

    if (pDestRect) {
        int minWidth = 0, minHeight = 0;

        memcpy(&pWin->left, pDestRect, sizeof(RECT));

        minHeight = iMenuY + (iCaptionY<<1);
        if (pWin->dwStyle & WS_VSCROLL) {
            minWidth += GetMainWinMetrics (MWM_CXVSCROLL);
            minHeight += (GetMainWinMetrics (MWM_CYVSCROLL)<<1) +
                         (GetMainWinMetrics (MWM_MINBARLEN)<<1);
        }
        
        if (pWin->WinType == TYPE_MAINWIN)
            minWidth += GetMainWinMetrics (MWM_MINWIDTH);

        if (pWin->dwStyle & WS_HSCROLL) {
            minHeight += GetMainWinMetrics (MWM_CYHSCROLL);
            minWidth += (GetMainWinMetrics (MWM_CXHSCROLL)<<1) +
                        (GetMainWinMetrics (MWM_MINBARLEN)<<1);
        }

        if ((minHeight > (pWin->bottom - pWin->top)) && (pWin->bottom != pWin->top))
            pWin->bottom = pWin->top + minHeight;

        if ((pWin->right < (pWin->left + minWidth)) && (pWin->right != pWin->left))
            pWin->right = pWin->left + minWidth;

        if (pResultRect)
             memcpy (pResultRect, &pWin->left, sizeof(RECT));
    }

    memcpy (&pWin->cl, &pWin->left, sizeof(RECT));

    pWin->cl += iBorder;
    pWin->ct += iBorder;
    pWin->cr -= iBorder;
    pWin->cb -= iBorder;
    pWin->ct += iCaptionY;
    pWin->ct += iMenuY;
    
    if (pWin->dwStyle & WS_HSCROLL && !(pWin->hscroll.status & SBS_HIDE)) {
    
        RECT rcBar;
        wndGetHScrollBarRect (pWin, &rcBar);
        wndScrollBarPos (pWin, TRUE, &rcBar);

        pWin->cb -= GetMainWinMetrics (MWM_CYHSCROLL);
    }
        
    if (pWin->dwStyle & WS_VSCROLL && !(pWin->vscroll.status & SBS_HIDE)) {
    
        RECT rcBar;
        wndGetVScrollBarRect (pWin, &rcBar);
        wndScrollBarPos (pWin, FALSE, &rcBar);

        pWin->cr -= GetMainWinMetrics (MWM_CXVSCROLL);
    }
}

static void OnQueryNCRect(PMAINWIN pWin, PRECT pRC)
{
    memcpy(pRC, &pWin->left, sizeof(RECT));
}

static void OnQueryClientArea(PMAINWIN pWin, PRECT pRC)
{
    memcpy(pRC, &pWin->cl, sizeof(RECT));
}

int ClientWidthToWindowWidth (DWORD dwStyle, int cw)
{
    int iBorder = 0;
    int iScroll = 0;

    if (dwStyle & WS_BORDER) 
        iBorder = GetMainWinMetrics(MWM_BORDER);
    else if (dwStyle & WS_THICKFRAME)
        iBorder = GetMainWinMetrics(MWM_THICKFRAME);
    else if (dwStyle & WS_THINFRAME)
        iBorder = GetMainWinMetrics (MWM_THINFRAME);

    if (dwStyle & WS_VSCROLL)
        iScroll = GetMainWinMetrics (MWM_CXVSCROLL);
        
    return cw + (iBorder<<1) + iScroll;
}

int ClientHeightToWindowHeight (DWORD dwStyle, int ch, BOOL hasMenu)
{
    int iBorder  = 0;
    int iCaption = 0;
    int iScroll  = 0;
    int iMenu    = 0;

    if (dwStyle & WS_BORDER) 
        iBorder = GetMainWinMetrics(MWM_BORDER);
    else if (dwStyle & WS_THICKFRAME)
        iBorder = GetMainWinMetrics(MWM_THICKFRAME);
    else if (dwStyle & WS_THINFRAME)
        iBorder = GetMainWinMetrics (MWM_THINFRAME);

    if (dwStyle & WS_HSCROLL)
        iScroll = GetMainWinMetrics (MWM_CYHSCROLL);
        
    if (dwStyle & WS_CAPTION)
        iCaption = GetMainWinMetrics(MWM_CAPTIONY);

    if (hasMenu) {
        iMenu = GetMainWinMetrics (MWM_MENUBARY);
        iMenu += GetMainWinMetrics (MWM_MENUBAROFFY)<<1;
    }
    
    return ch + (iBorder<<1) + iCaption + iScroll + iMenu;
}

/* this function is CONTROL safe. */
static int HittestOnNClient (PMAINWIN pWin, int x, int y)
{
    RECT rcCaption, rcIcon, rcButton, rcMenuBar;
    int iBorder = wndGetBorder (pWin);
    int iCaption = 0, bCaption;
    int iIconX = 0;
    int iIconY = 0;

    if (pWin->dwStyle & WS_HSCROLL && !(pWin->hscroll.status & SBS_HIDE)) {
    
        RECT rcBar;
        wndGetHScrollBarRect (pWin, &rcBar);

        if (PtInRect (&rcBar, x, y))
            return HT_HSCROLL;
    }
        
    if (pWin->dwStyle & WS_VSCROLL && !(pWin->vscroll.status & SBS_HIDE)) {
    
        RECT rcBar;
        wndGetVScrollBarRect (pWin, &rcBar);

        if (PtInRect (&rcBar, x, y)) {
            return HT_VSCROLL;
        }
    }
    
    if (pWin->dwStyle & WS_CAPTION) {
        
        iCaption = GetMainWinMetrics(MWM_CAPTIONY);
        bCaption = iBorder + iCaption - 1;

        if (pWin->WinType == TYPE_MAINWIN && pWin->hIcon) {
            iIconX = GetMainWinMetrics(MWM_ICONX);
            iIconY = GetMainWinMetrics(MWM_ICONY);
        }

        /* Caption rect; */
        rcCaption.left = pWin->left + iBorder;
        rcCaption.top = pWin->top + iBorder;
        rcCaption.right = pWin->right - iBorder;
        rcCaption.bottom = rcCaption.top + iCaption;
        
        if (pWin->WinType == TYPE_MAINWIN && pWin->hIcon)
        { 
            rcIcon.left = rcCaption.left;
            rcIcon.top = pWin->top + iBorder + (bCaption - iIconY) / 2;
            rcIcon.right = rcIcon.left + iIconX;
            rcIcon.bottom = rcIcon.top + iIconY;
                       
            if (PtInRect (&rcIcon, x, y)) 
                return HT_ICON;
            
        }
        
        rcButton.left = rcCaption.right - GetMainWinMetrics (MWM_SB_WIDTH) - GetMainWinMetrics (MWM_SB_INTERX);
        rcButton.top = GetMainWinMetrics (MWM_SB_HEIGHT);
        if (rcButton.top < bCaption) {
            rcButton.top = iBorder + ((bCaption - GetMainWinMetrics (MWM_SB_HEIGHT))>>1);
        } else {
            rcButton.top = iBorder;
        }
        
        rcButton.top = pWin->top + rcButton.top; 
        rcButton.bottom = rcButton.top + GetMainWinMetrics (MWM_SB_HEIGHT);
        rcButton.right = rcCaption.right - GetMainWinMetrics (MWM_SB_INTERX);

        if (!(pWin->dwExStyle & WS_EX_NOCLOSEBOX)) {
            
            if (PtInRect (&rcButton, x, y))
                return HT_CLOSEBUTTON;

            rcButton.left -= GetMainWinMetrics (MWM_SB_WIDTH);
            rcButton.left -= GetMainWinMetrics (MWM_SB_INTERX)<<1;
        }
    
        if (pWin->dwStyle & WS_MAXIMIZEBOX) {
            rcButton.right = rcButton.left + GetMainWinMetrics (MWM_SB_WIDTH);
            if (PtInRect (&rcButton, x, y))
                return HT_MAXBUTTON;
    
            rcButton.left -= GetMainWinMetrics (MWM_SB_WIDTH);
            rcButton.left -= GetMainWinMetrics (MWM_SB_INTERX);
        }
    
        if (pWin->dwStyle & WS_MINIMIZEBOX) {
            rcButton.right = rcButton.left + GetMainWinMetrics (MWM_SB_WIDTH);
            if (PtInRect (&rcButton, x, y))
                return HT_MINBUTTON;
        }
    
        if (pWin->WinType == TYPE_MAINWIN && pWin->hMenu) {
            rcMenuBar.left = rcCaption.left;
            rcMenuBar.top = rcCaption.bottom + 1;
            rcMenuBar.right = rcCaption.right;
            rcMenuBar.bottom = rcMenuBar.top + GetMainWinMetrics (MWM_MENUBARY);
            rcMenuBar.bottom += GetMainWinMetrics (MWM_MENUBAROFFY)<<1;
                    
            if (PtInRect (&rcMenuBar, x, y))
                return HT_MENUBAR;
        }

        if (PtInRect (&rcCaption, x, y))
            return HT_CAPTION;

        if (pWin->dwStyle & WS_DLGFRAME)
            return HT_BORDER;

        if (x <= pWin->left + iCaption) {
            if (y <= pWin->top + iCaption)
                return HT_CORNER_TL;
            else if (y >= pWin->bottom - iCaption)
                return HT_CORNER_BL;
            else
                return HT_BORDER_LEFT;
        }
        else if (x >= pWin->right - iCaption) {
            if (y <= pWin->top + iCaption)
                return HT_CORNER_TR;
            else if (y >= pWin->bottom - iCaption)
                return HT_CORNER_BR;
            else
                return HT_BORDER_RIGHT;
        }
        else if (y <= pWin->top + iCaption) {
            return HT_BORDER_TOP;
        }
        else
            return HT_BORDER_BOTTOM;
    }
    else {
        /* Caption rect; */
        rcCaption.left = pWin->left + iBorder;
        rcCaption.top = pWin->top + iBorder;
        rcCaption.right = pWin->right - iBorder;
        rcCaption.bottom = rcCaption.top;

        if (pWin->WinType == TYPE_MAINWIN && pWin->hMenu) {
            rcMenuBar.left = rcCaption.left;
            rcMenuBar.top = rcCaption.bottom + 1;
            rcMenuBar.right = rcCaption.right;
            rcMenuBar.bottom = rcMenuBar.top + GetMainWinMetrics (MWM_MENUBARY);
            rcMenuBar.bottom += GetMainWinMetrics (MWM_MENUBAROFFY)<<1;
                    
            if (PtInRect (&rcMenuBar, x, y))
                return HT_MENUBAR;
        }
    }

    return HT_BORDER;
}

extern HWND __mg_ime_wnd;
static void open_ime_window (PMAINWIN pWin, BOOL open_close, HWND rev_hwnd)
{
#if !defined(_LITE_VERSION) || defined(_STAND_ALONE)
    if (__mg_ime_wnd && pWin) {
        BOOL is_edit;

        if (pWin->pMainWin && ((pWin->pMainWin->dwExStyle & WS_EX_TOOLWINDOW)
                           || ((HWND)(pWin->pMainWin) == __mg_ime_wnd)))
            return;

        is_edit = SendAsyncMessage ((HWND)pWin, MSG_DOESNEEDIME, 0, 0L);
        if (is_edit && open_close)
            SendNotifyMessage (__mg_ime_wnd, MSG_IME_OPEN, 0, 0);
        else
            SendNotifyMessage (__mg_ime_wnd, MSG_IME_CLOSE, 0, 0);
#if 0
        }
        else if (!open_close) {
            BOOL other_is_edit 
            = rev_hwnd && SendAsyncMessage (rev_hwnd, MSG_DOESNEEDIME, 0, 0L);
            if (!other_is_edit)
                SendNotifyMessage (__mg_ime_wnd, MSG_IME_CLOSE, 0, 0);
        }
#endif
    }
#else
    if (!mgIsServer && pWin) {
        BOOL is_edit;
        REQUEST req;
        BOOL open = FALSE;

        if (pWin->pMainWin && (pWin->pMainWin->dwExStyle & WS_EX_TOOLWINDOW))
            return;

        is_edit = SendAsyncMessage ((HWND)pWin, MSG_DOESNEEDIME, 0, 0L);

        req.id = REQID_OPENIMEWND;
        req.data = &open;
        req.len_data = sizeof (BOOL);
        if (open_close && is_edit) {
                open = TRUE;
            ClientRequest (&req, NULL, 0);
        }
#if 0
        else if (!open_close && is_edit) {
            BOOL other_is_edit 
                = MG_IS_NORMAL_WINDOW(rev_hwnd) && SendAsyncMessage (rev_hwnd, MSG_DOESNEEDIME, 0, 0L);
            if (!other_is_edit) {
                ClientRequest (&req, NULL, 0);
            }
        }
#endif
    }
#endif
}

static int DefaultPostMsgHandler(PMAINWIN pWin, int message,
                           WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case MSG_SETCURSOR:
/*
** NOTE:
** this message is only implemented for main window.
** for CONTROL, must handle this message and should NOT 
** call default window procedure
** when handle MSG_SETCURSOR.
*/
            if (wndMouseInWhichControl (pWin, LOSWORD(lParam), HISWORD(lParam), 
                    NULL))
                break;

            if (pWin->hCursor)
                SetCursor(pWin->hCursor);
        break;

        case MSG_NCSETCURSOR:
/*
** NOTE:
** this message is only implemented for main window.
** for CONTROL, must handle this message and should NOT 
** call default window procedure
** when handle MSG_SETCURSOR.
*/
            switch (wParam) {
                case HT_BORDER_TOP:
                case HT_BORDER_BOTTOM:
                    SetCursor (GetSystemCursor (IDC_SIZENS));
                    break;
                case HT_BORDER_LEFT:
                case HT_BORDER_RIGHT:
                    SetCursor (GetSystemCursor (IDC_SIZEWE));
                    break;
                case HT_CORNER_TL:
                case HT_CORNER_BR:
                    SetCursor (GetSystemCursor (IDC_SIZENWSE));
                    break;
                case HT_CORNER_BL:
                case HT_CORNER_TR:
                    SetCursor (GetSystemCursor (IDC_SIZENESW));
                    break;
                default:
                    SetCursor (GetSystemCursor (IDC_ARROW));
                break;
            }
        break;
        
        case MSG_HITTEST:
            if (PtInRect((PRECT)(&(pWin->cl)), (int)wParam, (int)lParam) )
                return HT_CLIENT;
            else
                return HittestOnNClient (pWin, 
                            (int)wParam, (int)lParam);
        break;

        case MSG_CHANGESIZE:
            OnChangeSize (pWin, (PRECT)wParam, (PRECT)lParam);
            RecalcClientArea ((HWND)pWin);
        break;

        case MSG_SIZECHANGING:
            memcpy ((PRECT)lParam, (PRECT)wParam, sizeof (RECT));
        break;
        
        case MSG_QUERYNCRECT:
            OnQueryNCRect(pWin, (PRECT)lParam);
        break;

        case MSG_QUERYCLIENTAREA:
            OnQueryClientArea(pWin, (PRECT)lParam);
        break;

        case MSG_SETFOCUS:
        case MSG_KILLFOCUS:
            if (pWin->hActiveChild)
                SendNotifyMessage (pWin->hActiveChild, message, 0, lParam);
            if (!pWin->hActiveChild && lParam == 0) {
                open_ime_window (pWin, message == MSG_SETFOCUS, (HWND)wParam);
            }
        break;
        
        case MSG_MOUSEACTIVE:
            if (pWin->WinType == TYPE_CONTROL 
                    && !(pWin->dwStyle & WS_DISABLED)) {

                if (wParam != HT_OUT) {
                    PCONTROL pCtrl = (PCONTROL)pWin;
                    PCONTROL old_active = pCtrl->pParent->active;
 
                    if (old_active != (PCONTROL)pWin) {
                        if (old_active) {
                            SendNotifyMessage ((HWND)old_active, 
                                            MSG_ACTIVE, FALSE, wParam);
                            SendNotifyMessage ((HWND)old_active, 
                                            MSG_KILLFOCUS, (WPARAM)pWin, 1);
                        }

                        pCtrl->pParent->active = (PCONTROL)pWin;

                        SendNotifyMessage ((HWND)pWin, MSG_ACTIVE, TRUE, 0);
                        SendNotifyMessage ((HWND)pWin, 
                                        MSG_SETFOCUS, (WPARAM)old_active, 1);
                    }

                    if (!pWin->hActiveChild && wParam == HT_CLIENT) {
                        open_ime_window (pWin, TRUE, 0);
                    }
                    else {
                        open_ime_window (pWin, FALSE, 0);
                    }
                }
            }
        break;

        case MSG_WINDOWDROPPED:
        {
            RECT rc;
            rc.left = LOSWORD (wParam);
            rc.top = HISWORD (wParam);
            rc.right = LOSWORD (lParam);
            rc.bottom = HISWORD (lParam);

            MoveWindow ((HWND)pWin, rc.left, rc.top, 
                            RECTW(rc), RECTH(rc), FALSE);
        }
        break;
    }

    return 0;
}

/* this function is CONTROL safe. */
static void wndDrawNCArea (const MAINWIN* pWin, HDC hdc)
{
    /* Draw window frame */
    if (pWin->dwStyle & WS_BORDER) {
#if defined(_FLAT_WINDOW_STYLE) 
        int i, iBorder;

        iBorder = GetMainWinMetrics (MWM_BORDER);
        SetPenColor (hdc, GetWindowElementColorEx ((HWND)pWin, WEC_FLAT_BORDER));
        for (i = 0; i < iBorder; i++)
            Rectangle (hdc, i, i, 
                      pWin->right - pWin->left - i - 1, 
                      pWin->bottom - pWin->top - i - 1);
#elif defined _PHONE_WINDOW_STYLE
        int i, iBorder;

        iBorder = GetMainWinMetrics (MWM_BORDER);
        SetPenColor (hdc, GetWindowElementColorEx ((HWND)pWin, WEC_FLAT_BORDER));
        for (i = 0; i < iBorder; i++){
            Rectangle (hdc, i, i, 
                    pWin->right - pWin->left - i - 1, 
                    pWin->bottom - pWin->top - i - 1);
            SetPenColor (hdc,RGB2Pixel(hdc, 0x9c, 0xa1, 0xac));/* set inner border color */ 
        }
#else

        if (pWin->dwStyle & WS_CHILD)
            Draw3DThickFrameEx (hdc, (HWND)pWin,
                   0, 0, 
                   pWin->right - pWin->left - 1, 
                   pWin->bottom - pWin->top - 1, 
                   DF_3DBOX_PRESSED | DF_3DBOX_NOTFILL, 0);
        else
            Draw3DThickFrameEx (hdc, (HWND)pWin,
                   0, 0, 
                   pWin->right - pWin->left, 
                   pWin->bottom - pWin->top, 
                   DF_3DBOX_NORMAL | DF_3DBOX_NOTFILL, 0);
#endif
    }
    else if ((pWin->dwStyle & WS_THICKFRAME) ||
            (pWin->dwStyle & WS_THINFRAME))
    {
       SetPenColor(hdc, GetWindowElementColorEx ((HWND)pWin, WEC_FRAME_NORMAL));
       Rectangle(hdc, 0, 0, 
                      pWin->right - pWin->left - 1, 
                      pWin->bottom - pWin->top - 1);
    }

}

/* this function is CONTROL safe. */
static void wndDrawScrollBar (MAINWIN* pWin, HDC hdc)
{
    int start = 0;
    RECT rcHBar, rcVBar, rcSlider;
    PBITMAP bmp;
    int xo, yo, bw, bh, w , h , xos, yos, r;
    float scaleX = 0.0f, scaleY = 0.0f;
    
    wndGetVScrollBarRect (pWin, &rcVBar);
    rcVBar.left -= pWin->left;
    rcVBar.top  -= pWin->top;
    rcVBar.right -= pWin->left;
    rcVBar.bottom -= pWin->top;
    wndGetHScrollBarRect (pWin, &rcHBar);
    rcHBar.left -= pWin->left;
    rcHBar.top  -= pWin->top;
    rcHBar.right -= pWin->left;
    rcHBar.bottom -= pWin->top;

    bmp = GetSystemBitmap (SYSBMP_ARROWS);
    bw = bmp->bmWidth >> 2;
    bh = bmp->bmHeight >> 1;

    if ((pWin->dwStyle & WS_HSCROLL) && (pWin->dwStyle & WS_VSCROLL)
            && !(pWin->hscroll.status & SBS_HIDE 
                && pWin->vscroll.status & SBS_HIDE)) {
        /* Always erase the bottom-right corner */
#ifdef _FLAT_WINDOW_STYLE
        SetBrushColor (hdc, PIXEL_lightwhite);
#else
        SetBrushColor (hdc, GetWindowElementColorEx ((HWND)pWin, BKC_CONTROL_DEF));
#endif
        FillBox (hdc, rcVBar.left, rcHBar.top, RECTW (rcVBar), RECTH (rcHBar));
    }

    if (pWin->dwStyle & WS_HSCROLL 
                && !(pWin->hscroll.status & SBS_HIDE) && RectVisible (hdc, &rcHBar)) {

        r = GetMainWinMetrics (MWM_CXHSCROLL);
        scaleX = (float)r/(float)bw;
        scaleY = (float)r/(float)bh;
        w = (int)ceil(bmp->bmWidth*scaleX);
        h = (int)ceil(bmp->bmHeight*scaleY);

        /* draw left and right buttons. */
        if (pWin->hscroll.status & SBS_DISABLED) {
            xo = bw << 1; yo = bh;
        }
        else {
            xo = bw << 1; yo = 0;
        }
       
        if (GetMainWinMetrics (MWM_CXHSCROLL) != 0) {
            xos = (int)ceil(xo*scaleX); 
            yos = (int)ceil(yo*scaleY); 
            FillBoxWithBitmapPart (hdc, rcHBar.left, rcHBar.top, 
                                r, r, w, h, bmp, xos, yos);
        }

        if (pWin->hscroll.status & SBS_DISABLED) {
            xo = (bw << 1) + bw; yo = bh;
        }
        else {
            xo = (bw << 1) + bw; yo = 0;
        }

        if (GetMainWinMetrics (MWM_CXHSCROLL) != 0) { 
            xos = (int)ceil(xo*scaleX); 
            yos = (int)ceil(yo*scaleY); 
            FillBoxWithBitmapPart (hdc, 
                        rcHBar.right - GetMainWinMetrics (MWM_CXHSCROLL), rcHBar.top,
                        r, r, w, h, bmp, xos, yos);
        }

        /* draw slider. */
        start = rcHBar.left +
                    GetMainWinMetrics (MWM_CXHSCROLL) +
                    pWin->hscroll.barStart;

        if (start + pWin->hscroll.barLen > rcHBar.right)
            start = rcHBar.right - pWin->hscroll.barLen;

        rcSlider.left = start;
        rcSlider.top = rcHBar.top;
        rcSlider.right = start + pWin->hscroll.barLen;
        rcSlider.bottom = rcHBar.bottom;

#ifdef _FLAT_WINDOW_STYLE
        SetBrushColor (hdc, GetWindowElementColorEx ((HWND)pWin, BKC_CONTROL_DEF));
        FillBox (hdc, start + 1, rcHBar.top + 1, 
                pWin->hscroll.barLen - 2, RECTH (rcHBar) - 1);
        SetPenColor (hdc, PIXEL_black);
        Rectangle (hdc, rcSlider.left, rcSlider.top, rcSlider.right - 1, rcSlider.bottom);

        xo = (rcSlider.left + rcSlider.right) >> 1;
        MoveTo (hdc, xo, rcSlider.top + 2);
        LineTo (hdc, xo, rcSlider.bottom - 1);
        MoveTo (hdc, xo - 2, rcSlider.top + 3);
        LineTo (hdc, xo - 2, rcSlider.bottom - 2);
        MoveTo (hdc, xo + 2, rcSlider.top + 3);
        LineTo (hdc, xo + 2, rcSlider.bottom - 2);

        SetPenColor (hdc, PIXEL_black);
        Rectangle (hdc, rcHBar.left - 1, rcHBar.top, rcHBar.right, rcHBar.bottom);
#elif defined(_PHONE_WINDOW_STYLE)
        DrawBoxFromBitmap (hdc, &rcSlider, GetSystemBitmap (SYSBMP_SCROLLBAR_HSLIDER), TRUE, FALSE);
#else
        Draw3DThickFrameEx (hdc, (HWND)pWin, 
                            rcSlider.left, rcHBar.top,
                            rcSlider.right, rcHBar.bottom,
                            DF_3DBOX_NORMAL | DF_3DBOX_FILL, 
                            GetWindowElementColorEx ((HWND)pWin, BKC_CONTROL_DEF));
#endif

        rcHBar.left += GetMainWinMetrics (MWM_CXHSCROLL);
        rcHBar.right -= GetMainWinMetrics (MWM_CXHSCROLL);

        /* Exclude the rect of slider */
        ExcludeClipRect (hdc, &rcSlider);

        /* Draw back bar */
#ifdef _FLAT_WINDOW_STYLE
        SetBrushColor (hdc, PIXEL_lightwhite);
        FillBox (hdc, rcHBar.left + 1, rcHBar.top + 1, RECTW(rcHBar) - 1, RECTH (rcHBar) - 1);
#elif defined(_PHONE_WINDOW_STYLE)
        DrawBoxFromBitmap (hdc, &rcHBar, GetSystemBitmap (SYSBMP_SCROLLBAR_HBG), TRUE, FALSE);
#else
        SetBrushColor (hdc, GetWindowElementColorEx ((HWND)pWin, BKC_CONTROL_DEF));
        FillBox (hdc, rcHBar.left, rcHBar.top, RECTW(rcHBar), RECTH (rcHBar));
#endif
    }

    if (pWin->dwStyle & WS_VSCROLL 
                && !(pWin->vscroll.status & SBS_HIDE) && RectVisible (hdc, &rcVBar)) {

        r = GetMainWinMetrics (MWM_CYVSCROLL);
        scaleX = (float)r/(float)bw;
        scaleY = (float)r/(float)bh;
        w = (int)ceil(bmp->bmWidth*scaleX);
        h = (int)ceil(bmp->bmHeight*scaleY);

        /* draw top and bottom arrow buttons. */
        if (pWin->vscroll.status & SBS_DISABLED) {
            xo = 0; yo = bh;
        }
        else {
            xo = 0; yo = 0;
        }

        if (GetMainWinMetrics (MWM_CYVSCROLL) != 0) { 
            xos = (int)ceil(xo*scaleX); 
            yos = (int)ceil(yo*scaleY); 
            FillBoxWithBitmapPart (hdc, rcVBar.left, rcVBar.top, 
                        r, r, w, h, bmp, xos, yos);
        }

        if (pWin->vscroll.status & SBS_DISABLED) {
            xo = bw; yo = bh;
        }
        else {
            xo = bw; yo = 0;
        }
        
        if (GetMainWinMetrics (MWM_CYVSCROLL) != 0) {
            xos = (int)ceil(xo*scaleX); 
            yos = (int)ceil(yo*scaleY); 
            FillBoxWithBitmapPart (hdc, 
                        rcVBar.left, rcVBar.bottom - GetMainWinMetrics (MWM_CYVSCROLL),
                        r, r, w, h, bmp, xos, yos);
        }

        /* draw slider */
        start = rcVBar.top +
                    GetMainWinMetrics (MWM_CYVSCROLL) +
                    pWin->vscroll.barStart;
                    
        if (start + pWin->vscroll.barLen > rcVBar.bottom)
            start = rcVBar.bottom - pWin->vscroll.barLen;

        rcSlider.left = rcVBar.left;
        rcSlider.top = start;
        rcSlider.right = rcVBar.right;
        rcSlider.bottom = start + pWin->vscroll.barLen;

#ifdef _FLAT_WINDOW_STYLE
        SetBrushColor (hdc, GetWindowElementColorEx ((HWND)pWin, BKC_CONTROL_DEF));
        FillBox (hdc, rcVBar.left + 1, start + 1, 
                RECTW (rcVBar) - 1, pWin->vscroll.barLen - 2);
        SetPenColor (hdc, PIXEL_black);
        Rectangle (hdc, rcSlider.left, rcSlider.top, rcSlider.right, rcSlider.bottom - 1);

        yo = (rcSlider.top + rcSlider.bottom) >> 1;
        MoveTo (hdc, rcSlider.left + 2, yo);
        LineTo (hdc, rcSlider.right - 1, yo);
        MoveTo (hdc, rcSlider.left + 3, yo - 2);
        LineTo (hdc, rcSlider.right - 2, yo - 2);
        MoveTo (hdc, rcSlider.left + 3, yo + 2);
        LineTo (hdc, rcSlider.right - 2, yo + 2);

        SetPenColor (hdc, PIXEL_black);
        Rectangle (hdc, rcVBar.left, rcVBar.top - 1, rcVBar.right, rcVBar.bottom);
#elif defined(_PHONE_WINDOW_STYLE)
        DrawBoxFromBitmap (hdc, &rcSlider, GetSystemBitmap (SYSBMP_SCROLLBAR_VSLIDER), FALSE, FALSE);
#else
        Draw3DThickFrameEx (hdc, (HWND)pWin, 
                            rcVBar.left, rcSlider.top, rcVBar.right, rcSlider.bottom,
                            DF_3DBOX_NORMAL | DF_3DBOX_FILL,
                            GetWindowElementColorEx ((HWND)pWin, BKC_CONTROL_DEF));
#endif

        rcVBar.top += GetMainWinMetrics (MWM_CYVSCROLL);
        rcVBar.bottom -= GetMainWinMetrics (MWM_CYVSCROLL);

        /* Exclude the rect of slider */
        ExcludeClipRect (hdc, &rcSlider);

        /* draw back bar */
#ifdef _FLAT_WINDOW_STYLE
        SetBrushColor (hdc, PIXEL_lightwhite);
        FillBox (hdc, rcVBar.left + 1, rcVBar.top + 1, RECTW (rcVBar) - 1, RECTH (rcVBar) - 1);
#elif defined(_PHONE_WINDOW_STYLE)
        DrawBoxFromBitmap (hdc, &rcVBar, GetSystemBitmap (SYSBMP_SCROLLBAR_VBG), FALSE, FALSE);
#else
        SetBrushColor (hdc, GetWindowElementColorEx ((HWND)pWin, BKC_CONTROL_DEF));
        FillBox (hdc, rcVBar.left, rcVBar.top, RECTW (rcVBar), RECTH (rcVBar));
#endif
    }
}

/* this function is CONTROL safe. */
static void wndDrawCaption(const MAINWIN* pWin, HDC hdc, BOOL bFocus)
{
    int i;
    RECT rc;
    int iBorder = 0;
    int iCaption = 0, bCaption;
    int iIconX = 0;
    int iIconY = 0;
    int x, y, w, h;
#if defined(_FLAT_WINDOW_STYLE) && defined(_GRAY_SCREEN) || defined(_PHONE_WINDOW_STYLE)
    SIZE text_ext;
#endif
    PBITMAP bmp;
#ifdef _PHONE_WINDOW_STYLE
    const BITMAP* bmp_caption;
#endif
    int bw, bh;

    bmp = GetSystemBitmap (SYSBMP_CAPBTNS);
    bw = bmp->bmWidth >> 2;
    bh = bmp->bmHeight;

    if (pWin->dwStyle & WS_BORDER) 
        iBorder = GetMainWinMetrics(MWM_BORDER);
    else if( pWin->dwStyle & WS_THICKFRAME ) {
        iBorder = GetMainWinMetrics(MWM_THICKFRAME);
        
        SetPenColor (hdc, bFocus
                            ? GetWindowElementColorEx ((HWND)pWin, WEC_FRAME_ACTIVED)
                            : GetWindowElementColorEx ((HWND)pWin, WEC_FRAME_NORMAL));
        for (i=1; i<iBorder; i++)
            Rectangle(hdc, i, i, 
                      pWin->right - pWin->left - i - 1, 
                      pWin->bottom - pWin->top - i - 1);

    }
    else if (pWin->dwStyle & WS_THINFRAME)
        iBorder = GetMainWinMetrics (MWM_THINFRAME);

    if (!(pWin->dwStyle & WS_CAPTION))
        return;

    if (pWin->hIcon ) {
        iIconX = GetMainWinMetrics(MWM_ICONX);
        iIconY = GetMainWinMetrics(MWM_ICONY);
    }


    iCaption = GetMainWinMetrics(MWM_CAPTIONY);
    bCaption = iBorder + iCaption - 1;

    /* draw Caption */
    rc.left = iBorder;
    rc.top = iBorder;
    rc.right = pWin->right - pWin->left - iBorder;
    rc.bottom = iBorder + iCaption;
    ClipRectIntersect (hdc, &rc);

    SelectFont (hdc, GetSystemFont (SYSLOGFONT_CAPTION));

#ifdef _FLAT_WINDOW_STYLE
#ifdef _GRAY_SCREEN
    GetTextExtent (hdc, pWin->spCaption, -1, &text_ext);
    if (pWin->hIcon) text_ext.cx += iIconX + 2;

    SetBrushColor (hdc, bFocus
                        ? GetWindowElementColorEx ((HWND)pWin, BKC_CAPTION_ACTIVED)
                        : GetWindowElementColorEx ((HWND)pWin, BKC_CAPTION_NORMAL));
    FillBox (hdc, iBorder, iBorder, text_ext.cx + 4, bCaption);

    SetBrushColor(hdc, GetWindowElementColorEx ((HWND)pWin, FGC_CAPTION_ACTIVED));
    FillBox (hdc, iBorder + text_ext.cx + 4, iBorder,
               pWin->right - pWin->left, bCaption);
#if 0
    SetPenColor(hdc, bFocus
                        ? GetWindowElementColorEx ((HWND)pWin, BKC_CAPTION_ACTIVED)
                        : GetWindowElementColorEx ((HWND)pWin, BKC_CAPTION_NORMAL));
#endif 
    if ((pWin->dwStyle & WS_BORDER) || (pWin->dwStyle & WS_THICKFRAME) || (pWin->dwStyle & WS_THINFRAME)){
        SetPenColor(hdc, GetWindowElementColorEx((HWND)pWin, BKC_CAPTION_ACTIVED));
    }
    else{
        SetPenColor(hdc, GetWindowElementColorEx((HWND)pWin, BKC_CAPTION_NORMAL));
    }
    MoveTo (hdc, iBorder, bCaption);
    LineTo (hdc, pWin->right - pWin->left, bCaption);

    SetPenColor(hdc, GetWindowElementColorEx ((HWND)pWin, FGC_CAPTION_ACTIVED));

    MoveTo (hdc, iBorder + text_ext.cx + 4, bCaption - 3);
    LineTo (hdc, iBorder + text_ext.cx + 2, bCaption - 1);
    MoveTo (hdc, iBorder + text_ext.cx + 4, bCaption - 2);
    LineTo (hdc, iBorder + text_ext.cx + 3, bCaption - 1);

    MoveTo (hdc, iBorder + text_ext.cx + 2, iBorder);
    LineTo (hdc, iBorder + text_ext.cx + 4, iBorder + 2);
    MoveTo (hdc, iBorder + text_ext.cx + 3, iBorder);
    LineTo (hdc, iBorder + text_ext.cx + 4, iBorder + 1);
#endif
#elif _PHONE_WINDOW_STYLE
    bmp_caption = GetStockBitmap (STOCKBMP_CAPTION, 0, 0);

    if (pWin->dwStyle & WS_BORDER || pWin->dwStyle & WS_THICKFRAME){
        FillBoxWithBitmap (hdc, iBorder, iBorder - 2,
                pWin->right - pWin->left - iBorder,
                bCaption + 1 , bmp_caption);
    }
    else if (pWin->dwStyle & WS_THINFRAME){
        FillBoxWithBitmap (hdc, iBorder, iBorder - 2,
                pWin->right - pWin->left - iBorder,
                bCaption + 1 + 1, bmp_caption);
    }
    else{
        FillBoxWithBitmap (hdc, iBorder, iBorder - 2,
                pWin->right - pWin->left - iBorder,
                bCaption + 1 + 2, bmp_caption);
    }
#else
    SetBrushColor (hdc, bFocus
                        ? GetWindowElementColorEx ((HWND)pWin, BKC_CAPTION_ACTIVED)
                        : GetWindowElementColorEx ((HWND)pWin, BKC_CAPTION_NORMAL));
    FillBox(hdc, iBorder, iBorder,
                       pWin->right - pWin->left - iBorder,
                       bCaption + 1);
#endif

    if (pWin->hIcon)
        DrawIcon (hdc, iBorder, iBorder + (bCaption - iIconY) / 2, 
                        iIconX, iIconY, pWin->hIcon);

#if _PHONE_WINDOW_STYLE
    SetTextColor(hdc, bFocus
                        ? PIXEL_black
                        : GetWindowElementColorEx ((HWND)pWin, FGC_CAPTION_NORMAL));
#else
    SetTextColor(hdc, bFocus
                        ? GetWindowElementColorEx ((HWND)pWin, FGC_CAPTION_ACTIVED)
                        : GetWindowElementColorEx ((HWND)pWin, FGC_CAPTION_NORMAL));
#endif
    SetBkColor(hdc, bFocus
                        ? GetWindowElementColorEx ((HWND)pWin, BKC_CAPTION_ACTIVED)
                        : GetWindowElementColorEx ((HWND)pWin, BKC_CAPTION_NORMAL));
    SetBkMode(hdc, BM_TRANSPARENT);
#if _PHONE_WINDOW_STYLE
    GetTextExtent (hdc, pWin->spCaption, -1, &text_ext);
    TextOut(hdc, (pWin->right - pWin->left - iBorder - text_ext.cx)/2 + iIconX, 
            iBorder + 3, pWin->spCaption);
#else
    TextOut(hdc, iBorder + iIconX + 2, iBorder + 3,
                pWin->spCaption);
#endif

    /* draw system button */
    w = GetMainWinMetrics (MWM_SB_WIDTH);
    x = rc.right - w - GetMainWinMetrics (MWM_SB_INTERX);
    y = GetMainWinMetrics (MWM_SB_HEIGHT);
    if (y < bCaption) {
        y = iBorder + ((bCaption - y)>>1);
        h = GetMainWinMetrics (MWM_SB_HEIGHT);
    }
    else {
        y = iBorder;
        h = iIconY;
    }
    
    if (!(pWin->dwExStyle & WS_EX_NOCLOSEBOX)) {
        /* close box */
        FillBoxWithBitmapPart (hdc, x, y, bw, bh, 0, 0, bmp, (bw << 1) + bw, 0);

        x -= GetMainWinMetrics (MWM_SB_WIDTH);
        x -= GetMainWinMetrics (MWM_SB_INTERX) << 1;
    }

    if (pWin->dwStyle & WS_MAXIMIZEBOX) {
        /* restore/maximize/question box */
        if (pWin->dwStyle & WS_MAXIMIZE)
            FillBoxWithBitmapPart (hdc, x, y, bw, bh, 0, 0, bmp, bw << 1, 0);
        else
            FillBoxWithBitmapPart (hdc, x, y, bw, bh, 0, 0, bmp, 0, 0);
        x -= GetMainWinMetrics (MWM_SB_WIDTH);
        x -= GetMainWinMetrics (MWM_SB_INTERX);
    }

    if (pWin->dwStyle & WS_MINIMIZEBOX) {
        /* minimize/ok box */
        FillBoxWithBitmapPart (hdc, x, y, bw, bh, 0, 0, bmp, bw, 0);
    }
}

static void wndEraseBackground(const MAINWIN* pWin, 
            HDC hdc, const RECT* pClipRect)
{
    RECT rcTemp;
    BOOL fGetDC = FALSE, fCaret;

    fCaret = HideCaret ((HWND)pWin);

    if (hdc == 0) {
        hdc = GetClientDC ((HWND)pWin);
        fGetDC = TRUE;
    }

    if (pClipRect) {
        rcTemp = *pClipRect;
        if (pWin->WinType == TYPE_MAINWIN) {
            ScreenToClient ((HWND)pWin, &rcTemp.left, &rcTemp.top);
            ScreenToClient ((HWND)pWin, &rcTemp.right, &rcTemp.bottom);
        }
    }
    else {
        rcTemp.left = rcTemp.top = 0;
        rcTemp.right = pWin->cr - pWin->cl;
        rcTemp.bottom = pWin->cb - pWin->ct;
    }
    
    SetBrushColor(hdc, pWin->iBkColor);

    FillBox(hdc, rcTemp.left, rcTemp.top, 
                 RECTW (rcTemp), RECTH (rcTemp));

    if (fGetDC)
        ReleaseDC (hdc);

    if (fCaret)
        ShowCaret ((HWND)pWin);
}

/* this function is CONTROL safe. */
static void wndDrawNCFrame(MAINWIN* pWin, HDC hdc, const RECT* prcInvalid)
{
    BOOL fGetDC = FALSE;
    
    /* to avoid some unexpected event... */
    if (!MG_IS_WINDOW ((HWND)pWin))
        return;

    if (hdc == 0) {
        hdc = GetDC ((HWND)pWin);
        fGetDC = TRUE;
    }
        
    if (prcInvalid)
        ClipRectIntersect (hdc, prcInvalid);

    wndDrawNCArea (pWin, hdc);

    wndDrawScrollBar (pWin, hdc);

    if (pWin->WinType == TYPE_MAINWIN) {
        wndDrawCaption (pWin, hdc, !(pWin->dwStyle & WS_DISABLED) 
            && (GetActiveWindow() == (HWND)pWin));
        DrawMenuBarHelper (pWin, hdc, prcInvalid);
    }
    else {
        wndDrawCaption (pWin, hdc, !(pWin->dwStyle & WS_DISABLED) && 
                ((PCONTROL)pWin)->pParent->active == (PCONTROL)pWin);
    }

    if (fGetDC)
        ReleaseDC (hdc);
}

/* this function is CONTROL safe. */
static void wndActiveMainWindow (PMAINWIN pWin, BOOL fActive)
{
    HDC hdc;

    hdc = GetDC ((HWND)pWin);

    wndDrawCaption (pWin, hdc, fActive);
        
    ReleaseDC (hdc);
}

#if 0
static void OnShowWindow (PMAINWIN pWin, int iShowCmd)
{
    PCONTROL pCtrl;
    
    if (iShowCmd != SW_HIDE) {
        
        pCtrl = (PCONTROL)(pWin->hFirstChild);

        while (pCtrl) {
            ShowWindow ((HWND)pCtrl, iShowCmd);

            pCtrl = pCtrl->next;
        }
    }
}
#endif

static int DefaultPaintMsgHandler(PMAINWIN pWin, int message,
                           WPARAM wParam, LPARAM lParam)
{
    switch( message )
    {
#if 0
        case MSG_SHOWWINDOW:
            OnShowWindow (pWin, (int)wParam);
        break;
#endif
        
        case MSG_NCPAINT:
            wndDrawNCFrame (pWin, (HDC)wParam, (const RECT*)lParam);
        break;

        case MSG_ERASEBKGND:
            if (pWin->WinType == TYPE_CONTROL &&
                            pWin->dwExStyle & WS_EX_TRANSPARENT)
                break;
            wndEraseBackground (pWin, (HDC)wParam, (const RECT*)lParam);
        break;

        case MSG_NCACTIVATE:
            wndActiveMainWindow (pWin, (BOOL)wParam);
        break;

        case MSG_SYNCPAINT:
            wndActiveMainWindow (pWin, (BOOL)wParam);
            PostMessage ((HWND)pWin, MSG_NCPAINT, 0, 0);
            PostMessage ((HWND)pWin, MSG_ERASEBKGND, 0, 0);
        break;

        case MSG_PAINT:
        {
            PINVRGN pInvRgn;

            pInvRgn = &pWin->InvRgn;

#ifndef _LITE_VERSION
            pthread_mutex_lock (&pInvRgn->lock);
#endif
            EmptyClipRgn (&pInvRgn->rgn);
#ifndef _LITE_VERSION
            pthread_mutex_unlock (&pInvRgn->lock);
#endif
        }

        break;

    }

    return 0;
}

static int DefaultControlMsgHandler(PMAINWIN pWin, int message,
                           WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    int len;

    switch( message )
    {
        case MSG_ENABLE:
            if ( (!(pWin->dwStyle & WS_DISABLED) && !wParam)
                    || ((pWin->dwStyle & WS_DISABLED) && wParam) ) {
                if (wParam)
                    pWin->dwStyle &= ~WS_DISABLED;
                else
                    pWin->dwStyle |=  WS_DISABLED;
            }
        break;
        
        case MSG_SYSCOMMAND:
            if (wParam == SC_CLOSE)
                SendNotifyMessage ((HWND)pWin, MSG_CLOSE, 0, 0);
        break;

        case MSG_GETTEXTLENGTH:
            if (pWin->spCaption)
                return strlen (pWin->spCaption);
            else
                return 0;

        case MSG_GETTEXT:
            if (pWin->spCaption) {
                char* buffer = (char*)lParam;

                len = MIN (strlen (pWin->spCaption), wParam);
                memcpy (buffer, pWin->spCaption, len);
                buffer [len] = '\0';
                return len;
            }
            else
                return 0;
        break;

        case MSG_FONTCHANGED:
            UpdateWindow ((HWND)pWin, TRUE);
            break;

        case MSG_SETTEXT:
/*
** NOTE:
** this message is only implemented for main window.
** for CONTROL, must handle this message and should NOT 
** call default window procedure
** when handle MSG_SETTEXT.
*/
            if (pWin->WinType == TYPE_CONTROL)
                return 0;

            FreeFixStr (pWin->spCaption);
            len = strlen ((char*)lParam);
            pWin->spCaption = FixStrAlloc (len);
            if (len > 0)
                strcpy (pWin->spCaption, (char*)lParam);

#ifdef _MGRM_PROCESSES
            SendMessage (HWND_DESKTOP, MSG_CHANGECAPTION, (WPARAM) pWin, 0L);
#endif

            hdc = GetDC ((HWND)pWin);
            wndDrawCaption(pWin, hdc, GetForegroundWindow () == (HWND)pWin);
            ReleaseDC (hdc);
        break;
    }

    return 0;
}

static int DefaultSessionMsgHandler(PMAINWIN pWin, int message,
                           WPARAM wParam, LPARAM lParam)
{

/*
** NOTE:
** Currently does nothing, should handle fowllowing messages:
**
** MSG_QUERYENDSESSION:
** MSG_ENDSESSION:
*/
    
    return 0;
}

static int DefaultSystemMsgHandler(PMAINWIN pWin, int message, 
                           WPARAM wParam, LPARAM lParam)
{

/*
** NOTE:
** Currently does nothing, should handle following messages:
**
** MSG_IDLE, MSG_CARETBLINK:
*/

    if (message == MSG_IDLE) {
        if (pWin == GetMainWindowPtrOfControl (sg_repeat_msg.hwnd)) {
            SendNotifyMessage (sg_repeat_msg.hwnd, 
                    sg_repeat_msg.message, 
                    sg_repeat_msg.wParam, sg_repeat_msg.lParam);
        }
    }
    else if (message == MSG_CARETBLINK && pWin->dwStyle & WS_VISIBLE) {
        BlinkCaret ((HWND)pWin);
    }
    
    return 0;
}

/*
** NOTE:
** This default main window call-back procedure
** also implemented for control.
*/
int PreDefMainWinProc (HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{
    PMAINWIN pWin = (PMAINWIN)hWnd;

    if (message > MSG_DT_MOUSEOFF && message <= MSG_DT_RBUTTONDBLCLK)
        return DefaultDTMouseMsgHandler(pWin, message, 
            wParam, LOSWORD (lParam), HISWORD (lParam));
    else if (message >= MSG_FIRSTMOUSEMSG && message <= MSG_NCMOUSEOFF)
        return DefaultMouseMsgHandler(pWin, message, 
            wParam, LOSWORD (lParam), HISWORD (lParam));
    else if (message > MSG_NCMOUSEOFF && message <= MSG_LASTMOUSEMSG)
        return DefaultNCMouseMsgHandler(pWin, message, 
            (int)wParam, LOSWORD (lParam), HISWORD (lParam));
    else if (message >= MSG_FIRSTKEYMSG && message <= MSG_LASTKEYMSG)
        return DefaultKeyMsgHandler(pWin, message, wParam, lParam);
    else if (message >= MSG_FIRSTPOSTMSG && message <= MSG_LASTPOSTMSG)
        return DefaultPostMsgHandler(pWin, message, wParam, lParam);
    else if (message >= MSG_FIRSTCREATEMSG && message <= MSG_LASTCREATEMSG) 
        return DefaultCreateMsgHandler(pWin, message, wParam, lParam);
    else if (message >= MSG_FIRSTPAINTMSG && message <= MSG_LASTPAINTMSG) 
        return DefaultPaintMsgHandler(pWin, message, wParam, lParam);
    else if (message >= MSG_FIRSTSESSIONMSG && message <= MSG_LASTSESSIONMSG) 
        return DefaultSessionMsgHandler(pWin, message, wParam, lParam);
    else if (message >= MSG_FIRSTCONTROLMSG && message <= MSG_LASTCONTROLMSG) 
        return DefaultControlMsgHandler(pWin, message, wParam, lParam);
    else if (message >= MSG_FIRSTSYSTEMMSG && message <= MSG_LASTSYSTEMMSG) 
        return DefaultSystemMsgHandler(pWin, message, wParam, lParam);

    return 0;
}

int PreDefControlProc (HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{
    if (message == MSG_SETTEXT)
        return 0;
    else if (message == MSG_SETCURSOR) {
        if (GetWindowExStyle (hWnd) & WS_EX_USEPARENTCURSOR)
            return 0;

        SetCursor (GetWindowCursor (hWnd));
        return 0;
    }
    else if (message == MSG_NCSETCURSOR) {
        SetCursor (GetSystemCursor (IDC_ARROW));
        return 0;
    }

    return DefaultMainWinProc (hWnd, message, wParam, lParam);
}

int DefaultWindowProc (HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{
    if (IsMainWindow(hWnd)) {
        return DefaultMainWinProc (hWnd, message, wParam, lParam);
    }
    else if (IsControl(hWnd)) {
        return DefaultControlProc (hWnd, message, wParam, lParam);
    }
    else if (IsDialog(hWnd)) {
        return DefaultDialogProc (hWnd, message, wParam, lParam);
    }
    return 0;
}

/************************* GUI calls support ********************************/

static HWND set_focus_helper (HWND hWnd)
{
    PMAINWIN pWin;
    PMAINWIN old_active;
    
    if (IsMainWindow (hWnd))
        return HWND_INVALID;

    pWin = (PMAINWIN) GetParent (hWnd);
    old_active = (PMAINWIN)pWin->hActiveChild;
    if (old_active != (PMAINWIN)hWnd) {

        if (old_active)
            SendNotifyMessage ((HWND)old_active, MSG_KILLFOCUS, (WPARAM)hWnd, 0);
                
        pWin->hActiveChild = hWnd;
        SendNotifyMessage (hWnd, MSG_SETFOCUS, (WPARAM)old_active, 0);
    }
    
    return pWin->hActiveChild;
}

HWND GUIAPI SetFocusChild (HWND hWnd)
{
    HWND hOldActive;

    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), HWND_INVALID);

    if ((hOldActive = set_focus_helper (hWnd)) != HWND_INVALID) {
        do {
            hWnd = GetParent (hWnd);
        } while (set_focus_helper (hWnd) != HWND_INVALID);
    }

    return hOldActive;
}

HWND GUIAPI GetFocusChild (HWND hParent)
{
    PMAINWIN pWin;
    
    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hParent), HWND_INVALID);
    pWin = MG_GET_WINDOW_PTR (hParent);
    return pWin->hActiveChild;
}

HWND GUIAPI SetNullFocus (HWND hParent)
{
    PMAINWIN pWin;
    HWND hOldActive;

    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hParent), HWND_INVALID);
    pWin = MG_GET_WINDOW_PTR (hParent);

    hOldActive = pWin->hActiveChild;
    if (hOldActive)
        SendNotifyMessage (hOldActive, MSG_KILLFOCUS, 0, 0);
    pWin->hActiveChild = 0;

    return hOldActive;
}

/* NOTE: this function support ONLY main window. */
HWND GUIAPI SetActiveWindow (HWND hMainWnd)
{
    if (!MG_IS_NORMAL_WINDOW(hMainWnd) || !IsMainWindow (hMainWnd))
        return HWND_INVALID;

    return SendMessage (HWND_DESKTOP, 
                    MSG_SETACTIVEMAIN, (WPARAM)hMainWnd, 0);
}

/* NOTE: this function support ONLY main window. */
HWND GUIAPI GetActiveWindow (void)
{
    return (HWND)SendMessage (HWND_DESKTOP, MSG_GETACTIVEMAIN, 0, 0);
}

HWND GUIAPI SetCapture (HWND hWnd)
{
    PMAINWIN pWin = (PMAINWIN)hWnd;

    if (hWnd == HWND_INVALID || (pWin && pWin->DataType != TYPE_HWND))
        return HWND_INVALID;

    /* if hWnd is HWND_DESKTOP or zero, release capture */
    if (hWnd == HWND_DESKTOP) hWnd = 0;

    return (HWND) SendMessage (HWND_DESKTOP, MSG_SETCAPTURE, (WPARAM)hWnd, 0);
}


HWND GUIAPI GetCapture (void)
{
    return (HWND) SendMessage (HWND_DESKTOP, MSG_GETCAPTURE, 0, 0);
}

void GUIAPI ReleaseCapture (void)
{
    SendMessage (HWND_DESKTOP, MSG_SETCAPTURE, 0, 0);
}

/*************************** Main window and thread **************************/

/* get main window pointer from a handle */
PMAINWIN CheckAndGetMainWindowPtr (HWND hWnd)
{
    MG_CHECK_RET (MG_IS_NORMAL_MAIN_WINDOW(hWnd), NULL);
    return MG_GET_WINDOW_PTR (hWnd);
}

PMAINWIN GetMainWindowPtrOfControl (HWND hWnd)
{
    PMAINWIN pWin;

    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), NULL);
    pWin = MG_GET_WINDOW_PTR (hWnd);

    return pWin->pMainWin;
}

BOOL GUIAPI IsWindow (HWND hWnd)
{
    return MG_IS_NORMAL_WINDOW (hWnd);
}

BOOL GUIAPI IsMainWindow (HWND hWnd)
{
    PMAINWIN pWin;

    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), FALSE);

    pWin = MG_GET_WINDOW_PTR(hWnd);

    return (pWin->WinType == TYPE_MAINWIN);
}

BOOL GUIAPI IsControl (HWND hWnd)
{
    PMAINWIN pWin;

    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), FALSE);

    pWin = MG_GET_WINDOW_PTR(hWnd);

    return (pWin->WinType == TYPE_CONTROL);
}

BOOL GUIAPI IsDialog (HWND hWnd)
{
    return (BOOL)SendAsyncMessage (hWnd, MSG_ISDIALOG, 0, 0);
}

HWND GUIAPI GetMainWindowHandle (HWND hWnd)
{
    PMAINWIN pWin;

    MG_CHECK_RET (MG_IS_WINDOW(hWnd), HWND_INVALID);
    pWin = MG_GET_WINDOW_PTR (hWnd);

    return (HWND)pWin->pMainWin;
}

HWND GUIAPI GetParent (HWND hWnd)
{
    PCONTROL pChildWin = (PCONTROL)hWnd;

    MG_CHECK_RET (MG_IS_WINDOW(hWnd), HWND_INVALID);
    
    return (HWND)pChildWin->pParent;
}

HWND GUIAPI GetHosting (HWND hWnd)
{
    PMAINWIN pWin;

    if (!(pWin = CheckAndGetMainWindowPtr (hWnd)))
        return HWND_INVALID;

    if (pWin->pHosting == NULL)
        return HWND_DESKTOP;

    return (HWND)(pWin->pHosting);
}

HWND GUIAPI GetFirstHosted (HWND hWnd)
{
    PMAINWIN pWin;

    if (!(pWin = CheckAndGetMainWindowPtr (hWnd)))
        return HWND_INVALID;

    return (HWND)(pWin->pFirstHosted);
}

HWND GUIAPI GetNextHosted (HWND hHosting, HWND hHosted)
{
    PMAINWIN pWin;
    PMAINWIN pHosted;
    
    if (!(pWin = CheckAndGetMainWindowPtr (hHosting)))
        return HWND_INVALID;

    if (hHosted == 0) {
        return GetFirstHosted(hHosting);
    }

    if (!(pHosted = CheckAndGetMainWindowPtr (hHosted)))
        return HWND_INVALID;
        
    if (pHosted->pHosting != pWin)
        return HWND_INVALID;
        
    return (HWND)(pHosted->pNextHosted);
}

HWND GUIAPI GetNextChild (HWND hWnd, HWND hChild)
{
    PCONTROL pControl, pChild;

    if (hChild == HWND_INVALID) return HWND_INVALID;

    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), HWND_INVALID);
    pControl = MG_GET_CONTROL_PTR(hWnd);

    pChild = (PCONTROL)hChild;

    if (pChild == NULL) {
        return (HWND)pControl->children;
    }
    else if (pControl != pChild->pParent) {
        return HWND_INVALID;
    }

    return (HWND)pChild->next;
}

HWND GUIAPI GetNextMainWindow (HWND hMainWnd)
{
    PMAINWIN pMainWin;

    if (hMainWnd == HWND_DESKTOP || hMainWnd == 0)
        pMainWin = NULL;
    else if (!(pMainWin = CheckAndGetMainWindowPtr (hMainWnd)))
        return HWND_INVALID;

    return (HWND) SendMessage (HWND_DESKTOP, 
                    MSG_GETNEXTMAINWIN, (WPARAM)pMainWin, 0L);
}

/* NOTE: this function is CONTROL safe */
void GUIAPI ScrollWindow (HWND hWnd, int iOffx, int iOffy, 
                    const RECT* rc1, const RECT* rc2)
{
    SCROLLWINDOWINFO swi;
    PCONTROL child;
    RECT rcClient, rcScroll;

    if ((iOffx == 0 && iOffy == 0))
        return;

    MG_CHECK (MG_IS_NORMAL_WINDOW(hWnd));

    GetClientRect (hWnd, &rcClient);
    if (rc1)
        IntersectRect (&rcScroll, rc1, &rcClient);
    else
        rcScroll = rcClient;

    swi.iOffx = iOffx;
    swi.iOffy = iOffy;
    swi.rc1   = &rcScroll;
    swi.rc2   = rc2;
 
    /* hide caret */
    HideCaret (hWnd);

    /* Modify the children's position before scrolling */
    child = ((PCONTROL)hWnd)->children;
    while (child) {

        if ((rc2 == NULL) || IsCovered ((const RECT*)&child->left, rc2)) {
           child->left += iOffx; child->top += iOffy;
           child->right += iOffx; child->bottom += iOffy;
           child->cl += iOffx; child->ct += iOffy;
           child->cr += iOffx; child->cb += iOffy;
        }

        child = child->next;
    }

    SendMessage (HWND_DESKTOP, MSG_SCROLLMAINWIN, (WPARAM)hWnd, (LPARAM)(&swi));

    /* show caret */
    ShowCaret (hWnd);
}

static PSCROLLBARINFO wndGetScrollBar (MAINWIN* pWin, int iSBar)
{
    if (iSBar == SB_HORZ) {
        if (pWin->dwStyle & WS_HSCROLL)
            return &pWin->hscroll;
    }
    else if (iSBar == SB_VERT) {
        if (pWin->dwStyle & WS_VSCROLL)
            return &pWin->vscroll;
    }

    return NULL;
}

BOOL GUIAPI EnableScrollBar (HWND hWnd, int iSBar, BOOL bEnable)
{
    PSCROLLBARINFO pSBar;
    PMAINWIN pWin;
    BOOL bPrevState;
    RECT rcBar;
    
    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), FALSE);
    pWin = MG_GET_WINDOW_PTR (hWnd);

    if ( !(pSBar = wndGetScrollBar (pWin, iSBar)) )
        return FALSE;

    bPrevState = !(pSBar->status & SBS_DISABLED);

    if (bEnable && !bPrevState)
        pSBar->status &= ~SBS_DISABLED;
    else if (!bEnable && bPrevState)
        pSBar->status |= SBS_DISABLED;
    else
        return FALSE;

    if (iSBar == SB_VERT)
        wndGetVScrollBarRect (pWin, &rcBar);
    else
        wndGetHScrollBarRect (pWin, &rcBar);
        
    rcBar.left -= pWin->left;
    rcBar.top  -= pWin->top;
    rcBar.right -= pWin->left;
    rcBar.bottom -= pWin->top;

    SendAsyncMessage (hWnd, MSG_NCPAINT, 0, (LPARAM)(&rcBar));

    return TRUE;
}

BOOL GUIAPI GetScrollPos (HWND hWnd, int iSBar, int* pPos)
{
    PSCROLLBARINFO pSBar;
    PMAINWIN pWin;
    
    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), FALSE);
    pWin = MG_GET_WINDOW_PTR (hWnd);
    
    if ( !(pSBar = wndGetScrollBar (pWin, iSBar)) )
        return FALSE;

    *pPos = pSBar->curPos;
    return TRUE;
}

BOOL GUIAPI GetScrollRange (HWND hWnd, int iSBar, int* pMinPos, int* pMaxPos)
{
    PSCROLLBARINFO pSBar;
    PMAINWIN pWin;
    
    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), FALSE);
    pWin = MG_GET_WINDOW_PTR (hWnd);

    if ( !(pSBar = wndGetScrollBar (pWin, iSBar)) )
        return FALSE;

    *pMinPos = pSBar->minPos;
    *pMaxPos = pSBar->maxPos;
    return TRUE;
}

static int vs_get_absstart (PMAINWIN pWin, const RECT* rcVBar)
{
    int start, limit = rcVBar->bottom - GetMainWinMetrics (MWM_CYVSCROLL);

    start = rcVBar->top + GetMainWinMetrics (MWM_CYVSCROLL)
                       + pWin->vscroll.barStart;
                    
    if (start + pWin->vscroll.barLen > limit)
            start = limit - pWin->vscroll.barLen;

    return start;
}

static int hs_get_absstart (PMAINWIN pWin, const RECT* rcHBar)
{
    int start, limit = rcHBar->right - GetMainWinMetrics (MWM_CXHSCROLL);

    start = rcHBar->left + GetMainWinMetrics (MWM_CXHSCROLL)
                        + pWin->hscroll.barStart;

    if (start + pWin->hscroll.barLen > limit)
            start = limit - pWin->hscroll.barLen;

    return start;
}

BOOL GUIAPI SetScrollPos (HWND hWnd, int iSBar, int iNewPos)
{
    PSCROLLBARINFO pSBar;
    PMAINWIN pWin;
    RECT rcBar;
    int old_start, new_start;
    HDC hdc;
    
    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), FALSE);
    pWin = MG_GET_WINDOW_PTR (hWnd);

    if ( !(pSBar = wndGetScrollBar (pWin, iSBar)) )
        return FALSE;

    if (iNewPos == pSBar->curPos)
        return TRUE;

    if (iNewPos < pSBar->minPos)
        pSBar->curPos = pSBar->minPos;
    else
        pSBar->curPos = iNewPos;

    {
        int max = pSBar->maxPos;
        max -= ((pSBar->pageStep - 1) > 0)?(pSBar->pageStep - 1):0;

        if (pSBar->curPos > max)
            pSBar->curPos = max;
    }
    
    if (iSBar == SB_VERT)
        wndGetVScrollBarRect (pWin, &rcBar);
    else
        wndGetHScrollBarRect (pWin, &rcBar);

    rcBar.left -= pWin->left;
    rcBar.top  -= pWin->top;
    rcBar.right -= pWin->left;
    rcBar.bottom -= pWin->top;

    if (iSBar == SB_VERT)
        old_start = vs_get_absstart (pWin, &rcBar);
    else
        old_start = hs_get_absstart (pWin, &rcBar);

    wndScrollBarPos (pWin, iSBar == SB_HORZ, &rcBar);

    hdc = GetDC (hWnd);
    if (iSBar == SB_VERT) {
        new_start = vs_get_absstart (pWin, &rcBar);
        if (new_start == old_start)
            goto no_update;
#if 0
        {
            BitBlt (hdc, rcBar.left, old_start, 
                    RECTW (rcBar), pWin->vscroll.barLen, 
                    hdc, rcBar.left, new_start, 0);
        }
        else
#endif

        if (new_start > old_start) {
            rcBar.top = old_start;
            rcBar.bottom = new_start + pWin->vscroll.barLen;
        }
        else {
            rcBar.top = new_start;
            rcBar.bottom = old_start + pWin->vscroll.barLen;
        }
    }
    else {
        new_start = hs_get_absstart (pWin, &rcBar);
        if (new_start == old_start)
            goto no_update;
#if 0
        {
            BitBlt (hdc, old_start, rcBar.top, 
                    pWin->hscroll.barLen, RECTH (rcBar), 
                    hdc, new_start, rcBar.top, 0);
        }
        else
#endif

        if (new_start > old_start) {
            rcBar.left = old_start;
            rcBar.right = new_start + pWin->hscroll.barLen;
        }
        else {
            rcBar.left = new_start;
            rcBar.right = old_start + pWin->hscroll.barLen;
        }
    }

    ClipRectIntersect (hdc, &rcBar);
    wndDrawScrollBar (pWin, hdc);

no_update:
    ReleaseDC (hdc);

    return TRUE;
}

BOOL GUIAPI SetScrollRange (HWND hWnd, int iSBar, int iMinPos, int iMaxPos)
{
    PSCROLLBARINFO pSBar;
    PMAINWIN pWin;
    RECT rcBar;
    HDC hdc;
    
    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), FALSE);
    pWin = MG_GET_WINDOW_PTR (hWnd);

    if ( !(pSBar = wndGetScrollBar (pWin, iSBar)) )
        return FALSE;

    if (iMinPos == pSBar->minPos && iMaxPos == pSBar->maxPos)
        return TRUE;

    pSBar->minPos = (iMinPos < iMaxPos)?iMinPos:iMaxPos;
    pSBar->maxPos = (iMinPos > iMaxPos)?iMinPos:iMaxPos;
    
    /* validate parameters. */
    if (pSBar->curPos < pSBar->minPos)
        pSBar->curPos = pSBar->minPos;

    if (pSBar->pageStep <= 0)
        pSBar->pageStep = 0;
    else if (pSBar->pageStep > (pSBar->maxPos - pSBar->minPos + 1))
        pSBar->pageStep = pSBar->maxPos - pSBar->minPos + 1;
    
    {
        int max = pSBar->maxPos;
        max -= ((pSBar->pageStep - 1) > 0)?(pSBar->pageStep - 1):0;

        if (pSBar->curPos > max)
            pSBar->curPos = max;
    }

    if (iSBar == SB_VERT)
        wndGetVScrollBarRect (pWin, &rcBar);
    else
        wndGetHScrollBarRect (pWin, &rcBar);

    rcBar.left -= pWin->left;
    rcBar.top  -= pWin->top;
    rcBar.right -= pWin->left;
    rcBar.bottom -= pWin->top;

    wndScrollBarPos (pWin, iSBar == SB_HORZ, &rcBar);

    if (iSBar == SB_VERT) {
        rcBar.top += GetMainWinMetrics (MWM_CYVSCROLL);
        rcBar.bottom -= GetMainWinMetrics (MWM_CYVSCROLL);
    }
    else {
        rcBar.left += GetMainWinMetrics (MWM_CXHSCROLL);
        rcBar.right -= GetMainWinMetrics (MWM_CXHSCROLL);
    }

    hdc = GetDC (hWnd);
    ClipRectIntersect (hdc, &rcBar);
    wndDrawScrollBar (pWin, hdc);
    ReleaseDC (hdc);

    return TRUE;
}

int wndScrollBarSliderStartPos (MAINWIN *pWin, int iSBar)
{
    int start = 0; 
    
    if (iSBar == SB_HORZ) {
        RECT rcHBar;

        wndGetHScrollBarRect (pWin, &rcHBar);
        start = rcHBar.left +
                    pWin->hscroll.barStart + GetMainWinMetrics (MWM_CXHSCROLL) ;
    }
    else if (iSBar == SB_VERT) {
        RECT rcVBar;

        wndGetVScrollBarRect (pWin, &rcVBar);
        start = rcVBar.top + 
                    pWin->vscroll.barStart + GetMainWinMetrics (MWM_CYVSCROLL);

        if (start + pWin->vscroll.barLen > rcVBar.bottom)
            start = rcVBar.bottom - pWin->vscroll.barLen;
    }
    
    return start;
}

BOOL GUIAPI SetScrollInfo (HWND hWnd, int iSBar, 
                const SCROLLINFO* lpsi, BOOL fRedraw)
{
    PSCROLLBARINFO pSBar;
    PMAINWIN pWin;
    RECT rcBar;
    DWORD changed_mask = 0;
    
    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), FALSE);
    pWin = MG_GET_WINDOW_PTR (hWnd);

    if ( !(pSBar = wndGetScrollBar (pWin, iSBar)) )
        return FALSE;
        
    if (lpsi->fMask & SIF_RANGE) {
        if (pSBar->minPos != lpsi->nMin || pSBar->maxPos != lpsi->nMax) {
            changed_mask |= SIF_RANGE;
        }
    }
    
    if (lpsi->fMask & SIF_POS && pSBar->curPos != lpsi->nPos) {
        changed_mask |= SIF_POS;
    }
    
    if (lpsi->fMask & SIF_PAGE && pSBar->pageStep != lpsi->nPage) {
        changed_mask |= SIF_PAGE;
    }

    if (changed_mask == 0)
        return TRUE;
    else if (changed_mask == SIF_POS)
        return SetScrollPos (hWnd, iSBar, lpsi->nPos);

    if (changed_mask & SIF_RANGE) {
        pSBar->minPos = (lpsi->nMin < lpsi->nMax)?lpsi->nMin:lpsi->nMax;
        pSBar->maxPos = (lpsi->nMin < lpsi->nMax)?lpsi->nMax:lpsi->nMin;
    }

    if (changed_mask & SIF_POS)
        pSBar->curPos = lpsi->nPos;

    if (changed_mask & SIF_PAGE)
        pSBar->pageStep = lpsi->nPage;

    /* validate parameters. */
    if (pSBar->curPos < pSBar->minPos)
        pSBar->curPos = pSBar->minPos;

    if (pSBar->pageStep <= 0)
        pSBar->pageStep = 0;
    else if (pSBar->pageStep > (pSBar->maxPos - pSBar->minPos + 1))
        pSBar->pageStep = pSBar->maxPos - pSBar->minPos + 1;
    
    {
        int max = pSBar->maxPos;
        max -= ((pSBar->pageStep - 1) > 0)?(pSBar->pageStep - 1):0;

        if (pSBar->curPos > max)
            pSBar->curPos = max;
    }

    if (fRedraw) {
        HDC hdc;

        if (iSBar == SB_VERT)
            wndGetVScrollBarRect (pWin, &rcBar);
        else
            wndGetHScrollBarRect (pWin, &rcBar);
        
        rcBar.left -= pWin->left;
        rcBar.top  -= pWin->top;
        rcBar.right -= pWin->left;
        rcBar.bottom -= pWin->top;
    
        wndScrollBarPos (pWin, iSBar == SB_HORZ, &rcBar);

        if (iSBar == SB_VERT) {
            rcBar.top += GetMainWinMetrics (MWM_CYVSCROLL);
            rcBar.bottom -= GetMainWinMetrics (MWM_CYVSCROLL);
        }
        else {
            rcBar.left += GetMainWinMetrics (MWM_CXHSCROLL);
            rcBar.right -= GetMainWinMetrics (MWM_CXHSCROLL);
        }

        hdc = GetDC (hWnd);
        ClipRectIntersect (hdc, &rcBar);
        wndDrawScrollBar (pWin, hdc);
        ReleaseDC (hdc);
    }
    
    return TRUE;
}

BOOL GUIAPI GetScrollInfo (HWND hWnd, int iSBar, PSCROLLINFO lpsi)
{
    PSCROLLBARINFO pSBar;
    PMAINWIN pWin;
    
    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), FALSE);
    pWin = MG_GET_WINDOW_PTR (hWnd);

    if (!(pSBar = wndGetScrollBar (pWin, iSBar)))
        return FALSE;
        
    if (lpsi->fMask & SIF_RANGE) {
        lpsi->nMin = pSBar->minPos;
        lpsi->nMax = pSBar->maxPos;
    }
    
    if (lpsi->fMask & SIF_POS) {
        lpsi->nPos = pSBar->curPos;
    }
    
    if (lpsi->fMask & SIF_PAGE)
        lpsi->nPage = pSBar->pageStep;
    
    return TRUE;
}

BOOL GUIAPI ShowScrollBar (HWND hWnd, int iSBar, BOOL bShow)
{
    PSCROLLBARINFO pSBar;
    PMAINWIN pWin;
    BOOL bPrevState;
    RECT rcBar;
    
    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), FALSE);
    pWin = MG_GET_WINDOW_PTR (hWnd);

    if ( !(pSBar = wndGetScrollBar (pWin, iSBar)) )
        return FALSE;

    bPrevState = !(pSBar->status & SBS_HIDE);

    if (bShow && !bPrevState)
        pSBar->status &= ~SBS_HIDE;
    else if (!bShow && bPrevState)
        pSBar->status |= SBS_HIDE;
    else
        return FALSE;

    SendAsyncMessage (hWnd, MSG_CHANGESIZE, 0, 0);

    if (iSBar == SB_VERT)
        wndGetVScrollBarRect (pWin, &rcBar);
    else
        wndGetHScrollBarRect (pWin, &rcBar);

    InflateRect (&rcBar, 1, 1);

    if (bShow) {
        SendAsyncMessage (hWnd, MSG_NCPAINT, 0, 0);
    }
    else {
        rcBar.left -= pWin->cl;
        rcBar.top  -= pWin->ct;
        rcBar.right -= pWin->cl;
        rcBar.bottom -= pWin->ct;
        SendAsyncMessage (hWnd, MSG_NCPAINT, 0, 0);
        InvalidateRect (hWnd, &rcBar, TRUE);
    }

    return TRUE;
}

/*************************** Main window creation ****************************/
#ifndef _LITE_VERSION
int GUIAPI CreateThreadForMainWindow (pthread_t* thread,
                                     pthread_attr_t* attr,
                                     void * (*start_routine)(void *),
                                     void * arg)
{
    pthread_attr_t new_attr;
    int ret;

    pthread_attr_init (&new_attr);
    if (attr == NULL) {
        attr = &new_attr;
        pthread_attr_setdetachstate (attr, PTHREAD_CREATE_DETACHED);
#ifdef __NOUNIX__
        /* hope 16KB is enough for MiniGUI */
        pthread_attr_setstacksize (attr, 16 * 1024);
#endif
    }

    ret = pthread_create(thread, attr, start_routine, arg);

    pthread_attr_destroy (&new_attr);

    return ret;
}

int GUIAPI WaitMainWindowClose (HWND hWnd, void** returnval)
{
    pthread_t th;

    if (!(CheckAndGetMainWindowPtr(hWnd)))
        return -1;

    th = GetMainWinThread (hWnd);

    return pthread_join (th, returnval);
}
#endif

void GUIAPI MainWindowThreadCleanup (HWND hMainWnd)
{
    PMAINWIN pWin = (PMAINWIN)hMainWnd;

    if (!MG_IS_DESTROYED_WINDOW (hMainWnd)) {
#ifdef _DEBUG
        fprintf (stderr, "MiniGUI Warning: Unexpected calling of "
                        "(MainWindowThreadCleanup); Window (%x) "
                        "not destroyed yet!\n",
                        hMainWnd);
#endif
        return;
    }

#ifndef _LITE_VERSION
    if (pWin->pHosting == NULL) {
        FreeMsgQueueThisThread ();
    }
#endif

#ifdef __THREADX__ 
    /* to avoid threadx keep pWin's value,which will lead to wrong way */
    memset (pWin, 0xcc, sizeof(MAINWIN));
#endif

    free (pWin);
}

#ifdef __TARGET_MGDESKTOP__
/* To handle main window offset */
int __mg_mainwin_offset_x;
int __mg_mainwin_offset_y;
#endif

HWND GUIAPI CreateMainWindow (PMAINWINCREATE pCreateInfo)
{
    PMAINWIN pWin;

    if (pCreateInfo == NULL)
        return HWND_INVALID;

    if (!(pWin = calloc(1, sizeof(MAINWIN))))
        return HWND_INVALID;

#ifndef _LITE_VERSION
    if (pCreateInfo->hHosting == HWND_DESKTOP || pCreateInfo->hHosting == 0) {
        /*
        ** Create thread infomation and message queue for this new main window.
        */
        if ((pWin->pMessages = GetMsgQueueThisThread ()) == NULL) {
            if (!(pWin->pMessages = InitMsgQueueThisThread ()) ) {
                free (pWin);
                return HWND_INVALID;
            }
            pWin->pMessages->pRootMainWin = pWin;
        }
        else {
            /* Already have a top level main window, in case of user have set 
               a wrong hosting window */
            pWin->pHosting = pWin->pMessages->pRootMainWin;
        }
    }
    else {
        pWin->pMessages = GetMsgQueueThisThread ();
        if (pWin->pMessages != GetMsgQueue (pCreateInfo->hHosting) ||
                pWin->pMessages == NULL) {
            free (pWin);
            return HWND_INVALID;
        }
    }

    if (pWin->pHosting == NULL)
        pWin->pHosting = GetMainWindowPtrOfControl (pCreateInfo->hHosting);
    /* leave the pHosting is NULL for the first window of this thread. */
#else
    pWin->pHosting = GetMainWindowPtrOfControl (pCreateInfo->hHosting);
    if (pWin->pHosting == NULL)
        pWin->pHosting = __mg_dsk_win;

    pWin->pMessages = __mg_dsk_msg_queue;
#endif

    pWin->pMainWin      = pWin;
    pWin->hParent       = 0;
    pWin->pFirstHosted  = NULL;
    pWin->pNextHosted   = NULL;
    pWin->DataType      = TYPE_HWND;
    pWin->WinType       = TYPE_MAINWIN;

#ifndef _LITE_VERSION
    pWin->th            = pthread_self();
#endif

    pWin->hFirstChild   = 0;
    pWin->hActiveChild  = 0;
    pWin->hOldUnderPointer = 0;
    pWin->hPrimitive    = 0;

    pWin->NotifProc     = NULL;

    pWin->dwStyle       = pCreateInfo->dwStyle;
    pWin->dwExStyle     = pCreateInfo->dwExStyle;

    pWin->hMenu         = pCreateInfo->hMenu;
    pWin->hCursor       = pCreateInfo->hCursor;
    pWin->hIcon         = pCreateInfo->hIcon;
    if ((pWin->dwStyle & WS_CAPTION) && (pWin->dwStyle & WS_SYSMENU))
        pWin->hSysMenu= CreateSystemMenu ((HWND)pWin, pWin->dwStyle);
    else
        pWin->hSysMenu = 0;

    pWin->pLogFont     = GetSystemFont (SYSLOGFONT_WCHAR_DEF);

    pWin->spCaption    = FixStrAlloc (strlen (pCreateInfo->spCaption));
    if (pCreateInfo->spCaption [0])
        strcpy (pWin->spCaption, pCreateInfo->spCaption);

    pWin->MainWindowProc = pCreateInfo->MainWindowProc;
    pWin->iBkColor    = pCreateInfo->iBkColor;

    pWin->pCaretInfo = NULL;

    pWin->dwAddData = pCreateInfo->dwAddData;
    pWin->dwAddData2 = 0;

#if !defined (_LITE_VERSION) || defined (_STAND_ALONE)
    if ( !( pWin->pZOrderNode = malloc (sizeof(ZORDERNODE))) )
        goto err;
#endif

    /* Scroll bar */
    if (pWin->dwStyle & WS_VSCROLL) {
        pWin->vscroll.minPos = 0;
        pWin->vscroll.maxPos = 100;
        pWin->vscroll.curPos = 0;
        pWin->vscroll.pageStep = 0;
        pWin->vscroll.barStart = 0;
        pWin->vscroll.barLen = 10;
        pWin->vscroll.status = SBS_NORMAL;
    }
    else
        pWin->vscroll.status = SBS_HIDE | SBS_DISABLED;
        
    if (pWin->dwStyle & WS_HSCROLL) {
        pWin->hscroll.minPos = 0;
        pWin->hscroll.maxPos = 100;
        pWin->hscroll.curPos = 0;
        pWin->hscroll.pageStep = 0;
        pWin->hscroll.barStart = 0;
        pWin->hscroll.barLen = 10;
        pWin->hscroll.status = SBS_NORMAL;
    }
    else
        pWin->hscroll.status = SBS_HIDE | SBS_DISABLED;

    if (SendMessage ((HWND)pWin, MSG_NCCREATE, 0, (LPARAM)pCreateInfo))
        goto err;

#ifdef __TARGET_MGDESKTOP__
    pCreateInfo->lx += __mg_mainwin_offset_x;
    pCreateInfo->rx += __mg_mainwin_offset_x;
    pCreateInfo->ty += __mg_mainwin_offset_y;
    pCreateInfo->by += __mg_mainwin_offset_y;
#endif

    SendMessage ((HWND)pWin, MSG_SIZECHANGING, 
                    (WPARAM)&pCreateInfo->lx, (LPARAM)&pWin->left);
    SendMessage ((HWND)pWin, MSG_CHANGESIZE, (WPARAM)&pWin->left, 0);

    pWin->pGCRInfo = &pWin->GCRInfo;
#if defined (_LITE_VERSION) && !defined (_STAND_ALONE)
    if (SendMessage (HWND_DESKTOP, MSG_ADDNEWMAINWIN, (WPARAM) pWin, 0) < 0)
        goto err;
#else
    SendMessage (HWND_DESKTOP, MSG_ADDNEWMAINWIN, (WPARAM) pWin, 
                    (LPARAM) pWin->pZOrderNode);
#endif

    /* There was a very large bug. 
     * We should add the new main window in system and then
     * SendMessage MSG_CREATE for application to create
     * child windows.
     */
    if (SendMessage ((HWND)pWin, MSG_CREATE, 0, (LPARAM)pCreateInfo)) {
        SendMessage(HWND_DESKTOP, MSG_REMOVEMAINWIN, (WPARAM)pWin, 0);
        goto err;
    }

    /* Create private client dc. */
    if (pWin->dwExStyle & WS_EX_USEPRIVATECDC)
        pWin->privCDC = CreatePrivateClientDC ((HWND)pWin);
    else
        pWin->privCDC = 0;

    return (HWND)pWin;

err:
#ifndef _LITE_VERSION
    if (pWin->pMessages && pWin->pHosting == NULL) {
        FreeMsgQueueThisThread ();
    }
#endif

#if !defined (_LITE_VERSION) || defined (_STAND_ALONE)
    if (pWin->pZOrderNode) free (pWin->pZOrderNode);
#endif
    if (pWin->privCDC) DeletePrivateDC (pWin->privCDC);
    free (pWin);

    return HWND_INVALID;
}

BOOL GUIAPI DestroyMainWindow (HWND hWnd)
{
    PMAINWIN pWin;
    PMAINWIN head, next;    /* hosted window list. */

    if (!(pWin = CheckAndGetMainWindowPtr (hWnd))) return FALSE;

    if (SendMessage (hWnd, MSG_DESTROY, 0, 0))
        return FALSE;

    /* destroy all controls of this window */
    DestroyAllControls (hWnd);

    /* destroy all hosted main windows and dialogs here. */
    head = pWin->pFirstHosted;
    while (head) {
        next = head->pNextHosted;

        if (IsDialog((HWND)head)) {
            EndDialog ((HWND)head, IDCANCEL);
        }
        else {
            if (DestroyMainWindow ((HWND)head))
                MainWindowCleanup ((HWND)head);
        }

        head = next;
    }

    /* kill all timers of this window */
    KillTimer (hWnd, 0);

    SendMessage(HWND_DESKTOP, MSG_REMOVEMAINWIN, (WPARAM)hWnd, 0);

    if (sg_repeat_msg.hwnd == hWnd)
        sg_repeat_msg.hwnd = 0;

    /* make the window to be invalid for PeekMessageEx, PostMessage etc */
    pWin->DataType = TYPE_WINTODEL;

    ThrowAwayMessages (hWnd);

#if !defined (_LITE_VERSION) || defined (_STAND_ALONE)
    if (pWin->pZOrderNode) {
        free (pWin->pZOrderNode);
        pWin->pZOrderNode = NULL;
    }
#endif

    if (pWin->privCDC) {
        DeletePrivateDC (pWin->privCDC);
        pWin->privCDC = 0;
    }

    if (pWin->spCaption) {
        FreeFixStr (pWin->spCaption);
        pWin->spCaption = NULL;
    }

    if (pWin->hMenu) {
        DestroyMenu (pWin->hMenu);
        pWin->hMenu = 0;
    }

    if (pWin->hSysMenu) {
        DestroyMenu (pWin->hSysMenu);
        pWin->hSysMenu = 0;
    }

    EmptyClipRgn (&pWin->pGCRInfo->crgn);
    EmptyClipRgn (&pWin->InvRgn.rgn);

    free_window_element_data (hWnd);

#ifndef _LITE_VERSION
    pthread_mutex_destroy (&pWin->pGCRInfo->lock);
    pthread_mutex_destroy (&pWin->InvRgn.lock);
#endif

    return TRUE;
}

/*************************** Main window creation ****************************/
void GUIAPI UpdateWindow (HWND hWnd, BOOL fErase)
{
    MG_CHECK (MG_IS_NORMAL_WINDOW(hWnd));

    if (fErase)
        SendAsyncMessage (hWnd, MSG_CHANGESIZE, 0, 0);

    SendAsyncMessage (hWnd, MSG_NCPAINT, 0, 0);
    if (fErase)
        InvalidateRect (hWnd, NULL, TRUE);
    else
        InvalidateRect (hWnd, NULL, FALSE);

    if (hWnd != HWND_DESKTOP) {
        PMAINWIN pWin;
    
        pWin = (PMAINWIN) hWnd;
        SendMessage (hWnd, MSG_PAINT, 0, (LPARAM)(&pWin->InvRgn.rgn));
    }
    else
        SendMessage (hWnd, MSG_PAINT, 0, 0);
}

/*
** this function show window in behavious by specified iCmdShow.
** if the window was previously visible, the return value is nonzero.
** if the window was previously hiddedn, the return value is zero.
*/
BOOL GUIAPI ShowWindow (HWND hWnd, int iCmdShow)
{
    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), FALSE);

    if (IsMainWindow (hWnd)) {
        switch (iCmdShow)
        {
            case SW_SHOWNORMAL:
                SendMessage (HWND_DESKTOP, 
                    MSG_MOVETOTOPMOST, (WPARAM)hWnd, 0);
            break;
            
            case SW_SHOW:
                SendMessage (HWND_DESKTOP, 
                    MSG_SHOWMAINWIN, (WPARAM)hWnd, 0);
            break;

            case SW_HIDE:
                SendMessage (HWND_DESKTOP, 
                    MSG_HIDEMAINWIN, (WPARAM)hWnd, 0);
            break;
        }
    }
    else {
        PCONTROL pControl;

        pControl = (PCONTROL)hWnd;
        
        if (pControl->dwExStyle & WS_EX_CTRLASMAINWIN) {
            if (iCmdShow == SW_SHOW)
                SendMessage (HWND_DESKTOP, MSG_SHOWGLOBALCTRL, (WPARAM)hWnd, iCmdShow);
            else if (iCmdShow == SW_HIDE)
                SendMessage (HWND_DESKTOP, MSG_HIDEGLOBALCTRL, (WPARAM)hWnd, iCmdShow);
            else
                return FALSE;
        }
        else {
            switch (iCmdShow) {
            case SW_SHOWNORMAL:
            case SW_SHOW:
                if (!(pControl->dwStyle & WS_VISIBLE)) {
                    pControl->dwStyle |= WS_VISIBLE;

                    SendAsyncMessage (hWnd, MSG_NCPAINT, 0, 0);
                    InvalidateRect (hWnd, NULL, TRUE);
                }
            break;
            
            case SW_HIDE:
                if (pControl->dwStyle & WS_VISIBLE) {
                
                    pControl->dwStyle &= ~WS_VISIBLE;
                    InvalidateRect ((HWND)(pControl->pParent), 
                        (RECT*)(&pControl->left), TRUE);
                }
            break;
            }
        }

        if (iCmdShow == SW_HIDE && pControl->pParent->active == pControl) {
            SendNotifyMessage (hWnd, MSG_KILLFOCUS, 0, 0);
            pControl->pParent->active = NULL;
        }
    }
    
    SendNotifyMessage (hWnd, MSG_SHOWWINDOW, (WPARAM)iCmdShow, 0);
    return TRUE;
}

BOOL GUIAPI EnableWindow (HWND hWnd, BOOL fEnable)
{
    BOOL fOldStatus;
    
    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), FALSE);

    if (IsMainWindow (hWnd)) {
        fOldStatus = SendMessage (HWND_DESKTOP, MSG_ENABLEMAINWIN,
                        (WPARAM)hWnd, (LPARAM)fEnable);
    }
    else {
        PCONTROL pControl;

        pControl = (PCONTROL)hWnd;
        
        fOldStatus = !(pControl->dwStyle & WS_DISABLED);
    }
    
    SendNotifyMessage (hWnd, MSG_ENABLE, fEnable, 0);

    return fOldStatus;
}

BOOL GUIAPI IsWindowEnabled (HWND hWnd)
{
    PMAINWIN pWin = (PMAINWIN)hWnd;

    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), FALSE);

    return !(pWin->dwStyle & WS_DISABLED);
}

void GUIAPI ScreenToClient (HWND hWnd, int* x, int* y)
{
    PCONTROL pParent;
    PCONTROL pCtrl;

    MG_CHECK (MG_IS_NORMAL_WINDOW(hWnd));
    pParent = pCtrl = MG_GET_CONTROL_PTR (hWnd);

    *x -= pCtrl->cl;
    *y -= pCtrl->ct;
    while ((pParent = pParent->pParent)) {
        *x -= pParent->cl;
        *y -= pParent->ct;
    }
}

void GUIAPI ClientToScreen(HWND hWnd, int* x, int* y)
{
    PCONTROL pParent;
    PCONTROL pCtrl;

    MG_CHECK (MG_IS_NORMAL_WINDOW(hWnd));
    pParent = pCtrl = MG_GET_CONTROL_PTR (hWnd);

    *x += pCtrl->cl;
    *y += pCtrl->ct;
    while ((pParent = pParent->pParent)) {
        *x += pParent->cl;
        *y += pParent->ct;
    }
}

void GUIAPI ScreenToWindow(HWND hWnd, int* x, int* y)
{
    PCONTROL pParent;
    PCONTROL pCtrl;

    MG_CHECK (MG_IS_NORMAL_WINDOW(hWnd));
    pParent = pCtrl = MG_GET_CONTROL_PTR (hWnd);

    *x -= pCtrl->left;
    *y -= pCtrl->top;
    while ((pParent = pParent->pParent)) {
        *x -= pParent->left;
        *y -= pParent->top;
    }
}

void GUIAPI WindowToScreen(HWND hWnd, int* x, int* y)
{
    PCONTROL pParent;
    PCONTROL pCtrl;

    MG_CHECK (MG_IS_NORMAL_WINDOW(hWnd));
    pParent = pCtrl = MG_GET_CONTROL_PTR (hWnd);

    *x += pCtrl->left;
    *y += pCtrl->top;
    while ((pParent = pParent->pParent)) {
        *x += pParent->left;
        *y += pParent->top;
    }
}

BOOL GUIAPI GetClientRect (HWND hWnd, PRECT prc)
{
    PMAINWIN pWin = (PMAINWIN)hWnd;

    if (hWnd == HWND_DESKTOP) {
        *prc = g_rcScr;
        return TRUE;
    }
    else if (hWnd == HWND_INVALID || pWin->DataType != TYPE_HWND)
        return FALSE;

    prc->left = prc->top = 0;
    prc->right = pWin->cr - pWin->cl;
    prc->bottom = pWin->cb - pWin->ct;
    return TRUE;
}

/******************** main window and control styles support *****************/
int GUIAPI GetWindowBkColor (HWND hWnd)
{
    PMAINWIN pWin = (PMAINWIN)hWnd;

    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), PIXEL_invalid);

    return pWin->iBkColor;
}

int GUIAPI SetWindowBkColor (HWND hWnd, int new_bkcolor)
{
    int old_bkcolor;
    PMAINWIN pWin;

    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), PIXEL_invalid);
    pWin = MG_GET_WINDOW_PTR (hWnd);

    old_bkcolor = pWin->iBkColor;
    pWin->iBkColor = new_bkcolor;
    return old_bkcolor;
}

PLOGFONT GUIAPI GetWindowFont (HWND hWnd)
{
    PMAINWIN pWin;

    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), NULL);
    pWin = MG_GET_WINDOW_PTR (hWnd);

    return pWin->pLogFont;
}

PLOGFONT GUIAPI SetWindowFont (HWND hWnd, PLOGFONT pLogFont)
{
    PLOGFONT old_logfont;
    PMAINWIN pWin;

    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), NULL);
    pWin = MG_GET_WINDOW_PTR (hWnd);

    if (pLogFont == NULL)
        pLogFont = GetSystemFont (SYSLOGFONT_WCHAR_DEF);

    if (SendMessage (hWnd, MSG_FONTCHANGING, 0, (LPARAM)pLogFont))
        return NULL;

    old_logfont = pWin->pLogFont;
    pWin->pLogFont = pLogFont;
    SendNotifyMessage (hWnd, MSG_FONTCHANGED, 0, 0);

    return old_logfont;
}

HCURSOR GUIAPI GetWindowCursor (HWND hWnd)
{
    PMAINWIN pWin;

    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), 0);
    pWin = MG_GET_WINDOW_PTR (hWnd);

    return pWin->hCursor;
}

HCURSOR GUIAPI SetWindowCursor (HWND hWnd, HCURSOR hNewCursor)
{
    PMAINWIN pWin;

    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), 0);
    pWin = MG_GET_WINDOW_PTR (hWnd);

    if (pWin->WinType == TYPE_MAINWIN)
        return SendMessage (HWND_DESKTOP, 
            MSG_SETWINCURSOR, (WPARAM)hWnd, (LPARAM)hNewCursor);
    else if (pWin->WinType == TYPE_CONTROL) {
        HCURSOR old = pWin->hCursor;
        pWin->hCursor = hNewCursor;
        return old;
    }
        
    return 0;
}

DWORD GetWindowStyle (HWND hWnd)
{
    PMAINWIN pWin;

    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), 0);
    pWin = MG_GET_WINDOW_PTR (hWnd);

    return pWin->dwStyle;
}

BOOL GUIAPI ExcludeWindowStyle (HWND hWnd, DWORD dwStyle)
{
    PMAINWIN pWin;

    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), FALSE);
    pWin = MG_GET_WINDOW_PTR (hWnd);

    pWin->dwStyle &= ~dwStyle;
    return TRUE;
}

BOOL GUIAPI IncludeWindowStyle (HWND hWnd, DWORD dwStyle)
{
    PMAINWIN pWin;

    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), FALSE);
    pWin = MG_GET_WINDOW_PTR (hWnd);

    pWin->dwStyle |= dwStyle;
    return TRUE;
}

DWORD GetWindowExStyle (HWND hWnd)
{
    PMAINWIN pWin;

    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), 0);
    pWin = MG_GET_WINDOW_PTR (hWnd);

    return pWin->dwExStyle;
}

BOOL GUIAPI ExcludeWindowExStyle (HWND hWnd, DWORD dwStyle)
{
    PMAINWIN pWin;

    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), FALSE);
    pWin = MG_GET_WINDOW_PTR (hWnd);

    pWin->dwExStyle &= ~dwStyle;
    return TRUE;
}

BOOL GUIAPI IncludeWindowExStyle (HWND hWnd, DWORD dwStyle)
{
    PMAINWIN pWin;

    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), FALSE);
    pWin = MG_GET_WINDOW_PTR (hWnd);

    pWin->dwExStyle |= dwStyle;
    return TRUE;
}

DWORD GUIAPI GetWindowAdditionalData (HWND hWnd)
{
    PMAINWIN pWin;

    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), 0);
    pWin = MG_GET_WINDOW_PTR (hWnd);

    return pWin->dwAddData;
}

DWORD GUIAPI SetWindowAdditionalData (HWND hWnd, DWORD newData)
{
    DWORD    oldOne;
    PMAINWIN pWin;

    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), 0L);
    pWin = MG_GET_WINDOW_PTR (hWnd);

    oldOne = pWin->dwAddData;
    pWin->dwAddData = newData;
    return oldOne;
}

DWORD GUIAPI GetWindowAdditionalData2 (HWND hWnd)
{
    PMAINWIN pWin;

    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), 0L);
    pWin = MG_GET_WINDOW_PTR (hWnd);

    return pWin->dwAddData2;
}

DWORD GUIAPI SetWindowAdditionalData2 (HWND hWnd, DWORD newData)
{
    DWORD    oldOne;
    PMAINWIN pWin;

    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), 0L);
    pWin = MG_GET_WINDOW_PTR (hWnd);

    oldOne = pWin->dwAddData2;
    pWin->dwAddData2 = newData;
    return oldOne;
}

DWORD GUIAPI GetWindowClassAdditionalData (HWND hWnd)
{
    PMAINWIN pWin;
    PCONTROL pCtrl;

    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), 0);
    pWin = MG_GET_WINDOW_PTR (hWnd);

    if (pWin->WinType == TYPE_CONTROL) {
        pCtrl = MG_GET_CONTROL_PTR(hWnd);
        return pCtrl->pcci->dwAddData;
    }

    return 0;
}

DWORD GUIAPI SetWindowClassAdditionalData (HWND hWnd, DWORD newData)
{
    PMAINWIN pWin;
    PCONTROL pCtrl;

    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), 0);
    pWin = MG_GET_WINDOW_PTR (hWnd);

    if (pWin->WinType == TYPE_CONTROL) {
        DWORD oldOne;

        pCtrl = MG_GET_CONTROL_PTR(hWnd);
        oldOne = pCtrl->pcci->dwAddData;
        pCtrl->pcci->dwAddData = newData;
        return oldOne;
    }
    
    return 0L;
}

const char* GUIAPI GetClassName (HWND hWnd)
{
    PMAINWIN pWin;
    PCONTROL pCtrl;

    pWin = (PMAINWIN)hWnd;

    if (!MG_IS_WINDOW(hWnd))
        return NULL;
    else if (hWnd == HWND_DESKTOP)
        return ROOTWINCLASSNAME;
    else if (pWin->WinType == TYPE_MAINWIN)
        return MAINWINCLASSNAME;
    else if (pWin->WinType == TYPE_CONTROL) {
        pCtrl = (PCONTROL)hWnd;
        return pCtrl->pcci->name;
    }

    return NULL;
}

BOOL GUIAPI IsWindowVisible (HWND hWnd)
{
    PMAINWIN pMainWin;
    PCONTROL pCtrl;
    
    if ((pMainWin = CheckAndGetMainWindowPtr (hWnd))) {
        return pMainWin->dwStyle & WS_VISIBLE;
    }
    else if (IsControl (hWnd)) {
        pCtrl = (PCONTROL)hWnd;
        while (pCtrl) {
            if (!(pCtrl->dwStyle & WS_VISIBLE))
                return FALSE;

            pCtrl = pCtrl->pParent;
        }
    }

    return TRUE;
}

BOOL GUIAPI GetWindowRect (HWND hWnd, PRECT prc)
{
    PMAINWIN pWin;

    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), FALSE);
    pWin = MG_GET_WINDOW_PTR (hWnd);

    prc->left = pWin->left;
    prc->top = pWin->top;
    prc->right = pWin->right;
    prc->bottom = pWin->bottom;
    return TRUE;
}

WNDPROC GUIAPI GetWindowCallbackProc (HWND hWnd)
{
    PMAINWIN pWin;

    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), NULL);
    pWin = MG_GET_WINDOW_PTR (hWnd);

    return pWin->MainWindowProc;
}

WNDPROC GUIAPI SetWindowCallbackProc (HWND hWnd, WNDPROC newProc)
{
    PMAINWIN pWin;
    WNDPROC old_proc;

    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), NULL);
    pWin = MG_GET_WINDOW_PTR (hWnd);

    old_proc = pWin->MainWindowProc;
    if (newProc)
        pWin->MainWindowProc = newProc;
    return old_proc;
}

const char* GUIAPI GetWindowCaption (HWND hWnd)
{
    PMAINWIN pWin;

    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), NULL);
    pWin = MG_GET_WINDOW_PTR (hWnd);

    return pWin->spCaption;
}

BOOL GUIAPI SetWindowCaption (HWND hWnd, const char* spCaption)
{
    PMAINWIN pWin;

    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), FALSE);
    pWin = MG_GET_WINDOW_PTR (hWnd);

    if (pWin->WinType == TYPE_MAINWIN) {
        return SetWindowText (hWnd, spCaption);
    }
    else if (pWin->WinType == TYPE_CONTROL) {
        PCONTROL pCtrl;
        pCtrl = (PCONTROL)hWnd;
        if (pCtrl->spCaption) {
            FreeFixStr (pCtrl->spCaption);
            pCtrl->spCaption = NULL;
        }

        if (spCaption) {
            pCtrl->spCaption = FixStrAlloc (strlen (spCaption));
            if (spCaption [0])
                strcpy (pCtrl->spCaption, spCaption);
        }

        return TRUE;
    }

    return FALSE;
}

int GUIAPI GetWindowTextLength (HWND hWnd)
{
    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), -1);

    return SendMessage (hWnd, MSG_GETTEXTLENGTH, 0, 0);
}

int GUIAPI GetWindowText (HWND hWnd, char* spString, int nMaxLen)
{
    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), -1);

    return SendMessage (hWnd, MSG_GETTEXT, (WPARAM)nMaxLen, (LPARAM)spString);
}

BOOL GUIAPI SetWindowText (HWND hWnd, const char* spString)
{
    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), FALSE);

    return (SendMessage (hWnd, MSG_SETTEXT, 0, (LPARAM)spString) == 0);
}

static void reset_private_dc_on_move (PCONTROL pCtrl)
{
    PCONTROL child;
    PDC pdc;

    if ((pCtrl->dwExStyle & WS_EX_USEPRIVATECDC) && pCtrl->privCDC) {
        pdc = (PDC)pCtrl->privCDC;
        pdc->oldage = 0;
    }

    child = pCtrl->children;
    while (child) {
        reset_private_dc_on_move (child);
        child = child->next;
    }
}

/* NOTE: This function is control safe */
BOOL GUIAPI MoveWindow (HWND hWnd, int x, int y, int w, int h, BOOL fPaint)
{
    RECT rcWindow;
    RECT rcExpect, rcResult;
    PMAINWIN pWin;

    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), FALSE);
    pWin = MG_GET_WINDOW_PTR (hWnd);

    SetRect (&rcExpect, x, y, x + w, y + h);
    
    GetWindowRect (hWnd, &rcWindow);
    SendMessage (hWnd, MSG_SIZECHANGING, 
            (WPARAM)(&rcExpect), (LPARAM)(&rcResult));

    if (EqualRect (&rcWindow, &rcResult))
        return FALSE;

    if (IsMainWindow (hWnd) || (pWin->dwExStyle & WS_EX_CTRLASMAINWIN)) {
        SendMessage (HWND_DESKTOP, MSG_MOVEMAINWIN, 
            (WPARAM)hWnd, (LPARAM)(&rcResult));

        if (RECTH (rcWindow) != RECTH (rcResult)
                        || RECTW (rcWindow) != RECTW (rcResult))
            fPaint = TRUE;
    }
    else {
        PCONTROL pParent;
        PCONTROL pCtrl;
        HDC hdc;

        pCtrl = (PCONTROL)hWnd;
        pParent = pCtrl->pParent;

        rcExpect = rcResult;
        SendMessage (hWnd, MSG_CHANGESIZE, 
                        (WPARAM)(&rcExpect), (LPARAM)(&rcResult));

        if (IsWindowVisible (hWnd)
                && (pParent->InvRgn.frozen == 0)) {
            hdc = GetClientDC ((HWND)pParent);
            BitBlt (hdc, pCtrl->left, pCtrl->top, 
                     pCtrl->right - pCtrl->left,
                     pCtrl->bottom - pCtrl->top,
                     hdc, rcResult.left, rcResult.top, 0);
            ReleaseDC (hdc);

            /* set to invisible temporarily. */
            /* FIXME: need more optimization. */
            pCtrl->dwStyle &= ~WS_VISIBLE;
            InvalidateRect ((HWND)pParent, &rcWindow, TRUE);
            rcExpect.left = rcResult.left;
            rcExpect.top = rcResult.top;
            rcExpect.right = rcResult.left + RECTW (rcWindow);
            rcExpect.bottom = rcResult.top + RECTH (rcWindow);
            InvalidateRect ((HWND)pParent, &rcExpect, TRUE);
            pCtrl->dwStyle |= WS_VISIBLE;
        }
    }

    /* reset children's private DCs recursively here */
    reset_private_dc_on_move ((PCONTROL)pWin);

    SendAsyncMessage (hWnd, MSG_NCPAINT, 0, 0);
    if (fPaint) {
        InvalidateRect (hWnd, NULL, TRUE);
    }

    return TRUE;
}

/*************************** Paint support ***********************************/
void wndUpdateInvalidRegion (HWND hWnd, 
        const RECT* prc, const RECT* prcClient)
{
    PMAINWIN pWin;
    PCONTROL pCtrl;
    PINVRGN pInvRgn;
    RECT rcInter;
    RECT rcTemp;

    pWin = (PMAINWIN)hWnd;

    if (pWin->WinType == TYPE_MAINWIN)
        pInvRgn = &pWin->InvRgn;
    else if (pWin->WinType == TYPE_CONTROL) {
        pCtrl = (PCONTROL)hWnd;
        pInvRgn = &pCtrl->InvRgn;
    }
    else
        return;

    if (pInvRgn->frozen)
        return;

#ifndef _LITE_VERSION
    pthread_mutex_lock (&pInvRgn->lock);
#endif
    if (prc) {
        rcTemp = *prc;
        NormalizeRect (&rcTemp);
        if (IsCovered (prcClient, &rcTemp)) {
            SetClipRgn (&pInvRgn->rgn, prcClient);
        }
        else {
            if (IntersectRect (&rcInter, &rcTemp, prcClient))
                AddClipRect (&pInvRgn->rgn, &rcInter);
        }
    }
    else {
        SetClipRgn (&pInvRgn->rgn, prcClient);
    }
#ifndef _LITE_VERSION
    pthread_mutex_unlock (&pInvRgn->lock);
#endif
}

static void wndCascadeInvalidateChildren (PCONTROL pCtrl, const RECT* prc, BOOL bEraseBkgnd)
{
    RECT rcInter;
    RECT rcTemp;
    RECT rcCtrlClient;

    while (pCtrl) {

        if (!(pCtrl->dwStyle & WS_VISIBLE) 
                        || pCtrl->dwExStyle & WS_EX_CTRLASMAINWIN) {
             pCtrl = pCtrl->next;
             continue;
        }
            
        rcCtrlClient.left = rcCtrlClient.top = 0;
        rcCtrlClient.right = pCtrl->cr - pCtrl->cl;
        rcCtrlClient.bottom = pCtrl->cb - pCtrl->ct;
        if (prc) {
            
            RECT rcCtrl;
                
            rcCtrl.left = rcCtrl.top = 0;
            rcCtrl.right = pCtrl->right - pCtrl->left;
            rcCtrl.bottom = pCtrl->bottom - pCtrl->top;
            rcTemp.left = prc->left - pCtrl->left;
            rcTemp.top  = prc->top  - pCtrl->top;
            rcTemp.right = prc->right - pCtrl->left;
            rcTemp.bottom = prc->bottom - pCtrl->top;

            if (IntersectRect (&rcInter, &rcTemp, &rcCtrl))
                SendAsyncMessage ((HWND)pCtrl,
                            MSG_NCPAINT, 0, (LPARAM)(&rcInter));

            rcTemp.left = prc->left - pCtrl->cl;
            rcTemp.top  = prc->top  - pCtrl->ct;
            rcTemp.right = prc->right - pCtrl->cl;
            rcTemp.bottom = prc->bottom - pCtrl->ct;

            if (IntersectRect (&rcInter, &rcTemp, &rcCtrlClient)) {
                wndUpdateInvalidRegion ((HWND)pCtrl, &rcInter, &rcCtrlClient);
                SendAsyncMessage ((HWND)pCtrl, 
                        MSG_ERASEBKGND, 0, (LPARAM)(&rcInter));
            }

            if (pCtrl->children)
                wndCascadeInvalidateChildren (pCtrl->children, &rcTemp, bEraseBkgnd);
        }
        else {
            SendAsyncMessage ((HWND)pCtrl, MSG_NCPAINT, 0, 0);
            wndUpdateInvalidRegion ((HWND)pCtrl, NULL, &rcCtrlClient);
            SendAsyncMessage ((HWND)pCtrl, MSG_ERASEBKGND, 0, 0);
            if (pCtrl->children)
                wndCascadeInvalidateChildren (pCtrl->children, NULL, bEraseBkgnd);
        }

        pCtrl = pCtrl->next;
    }
}

BOOL wndInvalidateRect (HWND hWnd, const RECT* prc, BOOL bEraseBkgnd)
{
    PMAINWIN pWin;
    PCONTROL pCtrl = NULL;
    RECT rcInter;
    RECT rcInvalid;
    RECT rcClient;
    PINVRGN pInvRgn;

    pWin = (PMAINWIN)hWnd;
    pCtrl = (PCONTROL)hWnd;

    if (pWin->WinType == TYPE_MAINWIN) {

        rcClient.left = rcClient.top = 0;
        rcClient.right = pWin->cr - pWin->cl;
        rcClient.bottom = pWin->cb - pWin->ct;

        pInvRgn = &pWin->InvRgn;

        if (pInvRgn->frozen)
            return FALSE;

#ifndef _LITE_VERSION
        pthread_mutex_lock (&pInvRgn->lock);
#endif
        if (prc) {
            rcInvalid = *prc;
            NormalizeRect (&rcInvalid);
            if (IsCovered (&rcClient, &rcInvalid)) {
                SetClipRgn (&pInvRgn->rgn, &rcClient);
            }
            else {
                if (IntersectRect (&rcInter, &rcInvalid, &rcClient)) {
                    AddClipRect (&pInvRgn->rgn, &rcInter);
                }
            }
        }
        else {
            SetClipRgn (&pInvRgn->rgn, &rcClient);
            rcInvalid = rcClient;
        }
#ifndef _LITE_VERSION
        pthread_mutex_unlock (&pInvRgn->lock);
#endif
        
        /* erase background here. */
        if (bEraseBkgnd) {
#if 1
            ClientToScreen (hWnd, &rcInvalid.left, &rcInvalid.top);
            ClientToScreen (hWnd, &rcInvalid.right, &rcInvalid.bottom);
            SendAsyncMessage (hWnd, MSG_ERASEBKGND, 0, (LPARAM)&rcInvalid);
#else
            HDC hdc;

            hdc = GetClientDC (hWnd);
            SetBrushColor(hdc, pWin->iBkColor);
            FillBox(hdc, rcInvalid.left, rcInvalid.top, 
                         RECTW (rcInvalid), RECTH (rcInvalid));
            ReleaseDC (hdc);
#endif
        }
    }
    else if (pWin->WinType == TYPE_CONTROL) {
        
        rcClient.left = rcClient.top = 0;
        rcClient.right = pCtrl->cr - pCtrl->cl;
        rcClient.bottom = pCtrl->cb - pCtrl->ct;

        if (bEraseBkgnd) {
            if (prc && IntersectRect (&rcInter, prc, &rcClient)) {
                SendAsyncMessage ((HWND)pCtrl, 
                        MSG_ERASEBKGND, 0, (LPARAM)(&rcInter));
            }
            else if (!prc)
                SendAsyncMessage ((HWND)pCtrl, MSG_ERASEBKGND, 0, 0);
        }

        wndUpdateInvalidRegion (hWnd, prc, &rcClient);
    }
    else
        return FALSE;

    if (pCtrl->children)
        wndCascadeInvalidateChildren (pCtrl->children, prc, bEraseBkgnd);    

    return TRUE;
}

BOOL GUIAPI InvalidateRect (HWND hWnd, const RECT* prc, BOOL bEraseBkgnd)
{
    BOOL retval;
    PCONTROL pCtrl = (PCONTROL) hWnd;
    RECT rcInvalidParent;
    PMAINWIN pWin;

    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), FALSE);
    pWin = MG_GET_WINDOW_PTR (hWnd);

    if (pCtrl && pCtrl->WinType == TYPE_CONTROL 
                    && pCtrl->dwExStyle & WS_EX_TRANSPARENT) {
        RECT rcInParent;

        rcInParent.left = pCtrl->cl;
        rcInParent.top = pCtrl->ct;
        rcInParent.right = pCtrl->cr;
        rcInParent.bottom = pCtrl->cb;

        if (prc) {
            RECT rcInvalid = *prc;
            OffsetRect (&rcInvalid, pCtrl->cl, pCtrl->ct);
            if (!IntersectRect (&rcInvalidParent, &rcInParent, &rcInvalid))
                return FALSE;
        }
        else {
            rcInvalidParent = rcInParent;
        }

        hWnd = (HWND) pCtrl->pParent;
        prc = &rcInvalidParent;
        bEraseBkgnd = TRUE;
    }

    retval = wndInvalidateRect (hWnd, prc, bEraseBkgnd);
    PostMessage (hWnd, MSG_PAINT, 0, 0);
    return retval;
}

BOOL GUIAPI GetUpdateRect (HWND hWnd, RECT* update_rect)
{
    PMAINWIN pWin;

    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), FALSE);
    pWin = MG_GET_WINDOW_PTR (hWnd);

    *update_rect = pWin->InvRgn.rgn.rcBound;
    return TRUE;
}

HDC GUIAPI BeginPaint (HWND hWnd)
{
    PMAINWIN pWin;
    PINVRGN pInvRgn;
    HDC hdc;

    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), HDC_INVALID);
    pWin = MG_GET_WINDOW_PTR (hWnd);

    if (pWin->dwExStyle & WS_EX_USEPRIVATECDC)
        hdc = pWin->privCDC;
    else
        hdc = GetClientDC (hWnd);

    pInvRgn = &pWin->InvRgn;

#ifndef _LITE_VERSION
    pthread_mutex_lock (&pInvRgn->lock);
#endif
    pInvRgn->frozen ++;

    /* hide caret */
    if (pWin->pCaretInfo && pWin->pCaretInfo->fBlink) {
        if (pWin->pCaretInfo->fShow && !(pWin->dwExStyle & WS_EX_TRANSPARENT)) {
#ifdef _USE_NEWGAL
            pWin->pCaretInfo->caret_bmp.bmBits = pWin->pCaretInfo->pNormal;
            FillBoxWithBitmap (hdc,
                        pWin->pCaretInfo->x, pWin->pCaretInfo->y, 0, 0, 
                        &pWin->pCaretInfo->caret_bmp);
#else
            PutSavedBoxOnDC (hdc, 
                        pWin->pCaretInfo->x, pWin->pCaretInfo->y,
                        pWin->pCaretInfo->nEffWidth,
                        pWin->pCaretInfo->nEffHeight,
                        pWin->pCaretInfo->pNormal);
#endif /* _USE_NEWGAL */
        }
    }

    SelectClipRegion (hdc, &pInvRgn->rgn);

    /* exclude the children area from the client */
    if (pWin->dwExStyle & WS_EX_CLIPCHILDREN) {
        PCONTROL child = (PCONTROL)pWin->hFirstChild;
        RECT rcClient;

        GetClientRect (hWnd, &rcClient);
        while (child) {
            if ((child->dwStyle & WS_VISIBLE) 
                    && DoesIntersect ((const RECT*)&child->left, &rcClient)) {
                ExcludeClipRect (hdc, (const RECT*)&child->left);
            }
            child = child->next;
        }
    }
    EmptyClipRgn (&pInvRgn->rgn);
    
    pInvRgn->frozen --;

#ifndef _LITE_VERSION
    pthread_mutex_unlock (&pInvRgn->lock);
#endif

    return hdc;
}

void GUIAPI EndPaint (HWND hWnd, HDC hdc)
{
    PMAINWIN pWin;

    MG_CHECK (MG_IS_NORMAL_WINDOW(hWnd));
    pWin = MG_GET_WINDOW_PTR (hWnd);

    /* show caret */
    if (pWin->pCaretInfo) {
        GetCaretBitmaps (pWin->pCaretInfo);
        if (pWin->pCaretInfo->fBlink) {
            pWin->pCaretInfo->fShow = TRUE;
            SetMapMode (hdc, MM_TEXT);
            SelectClipRect (hdc, NULL);
#ifdef _USE_NEWGAL
            pWin->pCaretInfo->caret_bmp.bmBits = pWin->pCaretInfo->pXored;
            FillBoxWithBitmap (hdc,
                        pWin->pCaretInfo->x, pWin->pCaretInfo->y, 0, 0, 
                        &pWin->pCaretInfo->caret_bmp);
#else
            PutSavedBoxOnDC (hdc, 
                        pWin->pCaretInfo->x, pWin->pCaretInfo->y,
                        pWin->pCaretInfo->nEffWidth,
                        pWin->pCaretInfo->nEffHeight,
                        pWin->pCaretInfo->pXored);
#endif /* _USE_NEWGAL */
        }
    }
    
    if (!(pWin->dwExStyle & WS_EX_USEPRIVATECDC))
        ReleaseDC (hdc);
    else if (pWin->privCDC)
        SelectClipRect (pWin->privCDC, NULL);
}

BOOL RegisterWindowClass (PWNDCLASS pWndClass)
{
    if (pWndClass == NULL)
        return FALSE;

    return SendMessage (HWND_DESKTOP, 
            MSG_REGISTERWNDCLASS, 0, (LPARAM)pWndClass) == ERR_OK;
}

BOOL UnregisterWindowClass (const char* szClassName)
{
    if (szClassName == NULL)
        return FALSE;

    return SendMessage (HWND_DESKTOP, 
            MSG_UNREGISTERWNDCLASS, 0, (LPARAM)szClassName) == ERR_OK;
}

BOOL GUIAPI GetWindowClassInfo (PWNDCLASS pWndClass)
{
    if (pWndClass == NULL || pWndClass->spClassName == NULL)
        return FALSE;

    return SendMessage (HWND_DESKTOP, 
            MSG_CTRLCLASSDATAOP, CCDOP_GETCCI, (LPARAM)pWndClass) == ERR_OK;
}

BOOL GUIAPI SetWindowClassInfo (const WNDCLASS* pWndClass)
{
    if (pWndClass == NULL || pWndClass->spClassName == NULL)
        return FALSE;

    return SendMessage (HWND_DESKTOP, 
            MSG_CTRLCLASSDATAOP, CCDOP_SETCCI, (LPARAM)pWndClass) == ERR_OK;
}

HWND GUIAPI CreateWindowEx (const char* spClassName, const char* spCaption,
                  DWORD dwStyle, DWORD dwExStyle, int id, 
                  int x, int y, int w, int h,
                  HWND hParentWnd, DWORD dwAddData)
{
    PMAINWIN pMainWin;
    PCTRLCLASSINFO cci;
    PCONTROL pNewCtrl;
    RECT rcExpect;

    if (!(pMainWin = GetMainWindowPtrOfControl (hParentWnd)))
        return HWND_INVALID;

    cci = (PCTRLCLASSINFO)SendMessage (HWND_DESKTOP, 
                MSG_GETCTRLCLASSINFO, 0, (LPARAM)spClassName);
                
    if (!cci) return HWND_INVALID;

    pNewCtrl = calloc (1, sizeof (CONTROL));

    if (!pNewCtrl) return HWND_INVALID;

    pNewCtrl->DataType = TYPE_HWND;
    pNewCtrl->WinType  = TYPE_CONTROL;

    pNewCtrl->left     = x;
    pNewCtrl->top      = y;
    pNewCtrl->right    = x + w;
    pNewCtrl->bottom   = y + h;

    memcpy (&pNewCtrl->cl, &pNewCtrl->left, sizeof (RECT));
    memcpy (&rcExpect, &pNewCtrl->left, sizeof (RECT));

    if (spCaption) {
        int len = strlen (spCaption);
        
        pNewCtrl->spCaption = FixStrAlloc (len);
        if (len > 0)
            strcpy (pNewCtrl->spCaption, spCaption);
    }
    else
        pNewCtrl->spCaption = "";
        
    pNewCtrl->dwStyle   = dwStyle | WS_CHILD | cci->dwStyle;
    pNewCtrl->dwExStyle = dwExStyle | cci->dwExStyle;

    pNewCtrl->iBkColor  = cci->iBkColor;
    pNewCtrl->hCursor   = cci->hCursor;
    pNewCtrl->hMenu     = 0;
    pNewCtrl->hAccel    = 0;
    pNewCtrl->hIcon     = 0;
    pNewCtrl->hSysMenu  = 0;
    if (pNewCtrl->dwExStyle & WS_EX_USEPARENTFONT)
        pNewCtrl->pLogFont = pMainWin->pLogFont;
    else
        pNewCtrl->pLogFont = GetSystemFont (SYSLOGFONT_CONTROL);

    pNewCtrl->id        = id;

    pNewCtrl->pCaretInfo = NULL;
    
    pNewCtrl->dwAddData = dwAddData;
    pNewCtrl->dwAddData2 = 0;

    pNewCtrl->ControlProc = cci->ControlProc;

    /* Scroll bar */
    if (pNewCtrl->dwStyle & WS_VSCROLL) {
        pNewCtrl->vscroll.minPos = 0;
        pNewCtrl->vscroll.maxPos = 100;
        pNewCtrl->vscroll.curPos = 0;
        pNewCtrl->vscroll.pageStep = 0;
        pNewCtrl->vscroll.barStart = 0;
        pNewCtrl->vscroll.barLen = 10;
        pNewCtrl->vscroll.status = SBS_NORMAL;
    }
    else
        pNewCtrl->vscroll.status = SBS_HIDE | SBS_DISABLED;

    if (pNewCtrl->dwStyle & WS_HSCROLL) {
        pNewCtrl->hscroll.minPos = 0;
        pNewCtrl->hscroll.maxPos = 100;
        pNewCtrl->hscroll.curPos = 0;
        pNewCtrl->hscroll.pageStep = 0;
        pNewCtrl->hscroll.barStart = 0;
        pNewCtrl->hscroll.barLen = 10;
        pNewCtrl->hscroll.status = SBS_NORMAL;
    }
    else
        pNewCtrl->hscroll.status = SBS_HIDE | SBS_DISABLED;

    pNewCtrl->children = NULL;
    pNewCtrl->active   = NULL;
    pNewCtrl->old_under_pointer = NULL;
    pNewCtrl->primitive = NULL;

    pNewCtrl->notif_proc = NULL;

    pNewCtrl->pMainWin = (PMAINWIN)pMainWin;
    pNewCtrl->pParent  = (PCONTROL)hParentWnd;
    pNewCtrl->next     = NULL;

    pNewCtrl->pcci     = cci;

    if (dwExStyle & WS_EX_CTRLASMAINWIN) {
        if ( !(pNewCtrl->pGCRInfo = malloc (sizeof (GCRINFO))) ) {
            goto error;
        }
#if !defined (_LITE_VERSION) || defined (_STAND_ALONE)
        if ( !(pNewCtrl->pZOrderNode = malloc (sizeof(ZORDERNODE))) ) {
            goto error;
        }
#endif
    }
    else {
        pNewCtrl->pGCRInfo = pMainWin->pGCRInfo;
#if defined (_LITE_VERSION) && !defined (_STAND_ALONE)
        pNewCtrl->idx_znode = pMainWin->idx_znode;
#endif
    }

    if (cci->dwStyle & CS_OWNDC) {
        pNewCtrl->privCDC = CreatePrivateClientDC ((HWND)pNewCtrl);
        pNewCtrl->dwExStyle |= WS_EX_USEPRIVATECDC;
    }
    else
        pNewCtrl->privCDC = 0;

    if (SendMessage ((HWND)pNewCtrl, MSG_NCCREATE, 0, (LPARAM)pNewCtrl)) {
        goto error;
    }

    if (SendMessage (HWND_DESKTOP, 
                            MSG_NEWCTRLINSTANCE, 
                            (WPARAM)hParentWnd, (LPARAM)pNewCtrl) < 0)
        goto error;

    if (SendMessage ((HWND)pNewCtrl, MSG_CREATE, 
                            (WPARAM)hParentWnd, (LPARAM)dwAddData)) {
        SendMessage (HWND_DESKTOP, 
                MSG_REMOVECTRLINSTANCE, (WPARAM)hParentWnd, (LPARAM)pNewCtrl);
        goto error;
    }

    SendMessage ((HWND)pNewCtrl, MSG_SIZECHANGING, 
                    (WPARAM)&rcExpect, (LPARAM)&pNewCtrl->left);
    SendMessage ((HWND)pNewCtrl, MSG_CHANGESIZE, 
                    (WPARAM)(&pNewCtrl->left), 0);

    if (pNewCtrl->pParent->dwStyle & WS_VISIBLE 
                    && pNewCtrl->dwStyle & WS_VISIBLE)
        UpdateWindow ((HWND)pNewCtrl, TRUE);

    return (HWND)pNewCtrl;

error:
    if (dwExStyle & WS_EX_CTRLASMAINWIN) {
        if (pNewCtrl->pGCRInfo) free (pNewCtrl->pGCRInfo);
#if !defined (_LITE_VERSION) || defined (_STAND_ALONE)
        if (pNewCtrl->pZOrderNode) free (pNewCtrl->pZOrderNode);
#endif
    }
    if (pNewCtrl->privCDC) DeletePrivateDC (pNewCtrl->privCDC);
    free (pNewCtrl);

    return HWND_INVALID;
}

BOOL GUIAPI DestroyWindow (HWND hWnd)
{
    PCONTROL pCtrl;
    PCONTROL pParent;
    
    if (!IsControl (hWnd)) return FALSE;

    if (SendMessage (hWnd, MSG_DESTROY, 0, 0))
        return FALSE;

    /* destroy all controls of this window */
    DestroyAllControls (hWnd);

    /* kill all timers of this window */
    KillTimer (hWnd, 0);

    pCtrl = (PCONTROL)hWnd;
    pParent = pCtrl->pParent;
    if (pParent->active == (PCONTROL) hWnd)
        pParent->active = NULL;
    if (pParent->old_under_pointer == (PCONTROL) hWnd)
        pParent->old_under_pointer = NULL;
    if (pParent->primitive == (PCONTROL) hWnd)
        pParent->primitive = NULL;

    if (SendMessage (HWND_DESKTOP, 
            MSG_REMOVECTRLINSTANCE, (WPARAM)pParent, (LPARAM)pCtrl))
        return FALSE;

    __mg_reset_mainwin_capture_info (pCtrl);
    
    pCtrl->dwStyle &= ~WS_VISIBLE;
    if (IsWindowVisible ((HWND) pParent))
        InvalidateRect ((HWND) pParent, (PRECT)(&pCtrl->left), TRUE);

    if (pCtrl->privCDC)
        DeletePrivateDC (pCtrl->privCDC);

    if (sg_repeat_msg.hwnd == hWnd)
        sg_repeat_msg.hwnd = 0;
    ThrowAwayMessages (hWnd);

    if (pCtrl->dwExStyle & WS_EX_CTRLASMAINWIN) {
        EmptyClipRgn (&pCtrl->pGCRInfo->crgn);
#ifndef _LITE_VERSION
        pthread_mutex_destroy (&pCtrl->pGCRInfo->lock);
#endif
        free (pCtrl->pGCRInfo);
#if !defined (_LITE_VERSION) || defined (_STAND_ALONE)
        free (pCtrl->pZOrderNode);
#endif
    }
    EmptyClipRgn (&pCtrl->InvRgn.rgn);
#ifndef _LITE_VERSION
    pthread_mutex_destroy (&pCtrl->InvRgn.lock);
#endif

    if (pCtrl->spCaption)
        FreeFixStr (pCtrl->spCaption);

    free_window_element_data (hWnd);

    free (pCtrl);
    return TRUE;
}

NOTIFPROC GUIAPI SetNotificationCallback (HWND hwnd, NOTIFPROC notif_proc)
{
    NOTIFPROC old_proc;
    PCONTROL control = (PCONTROL)hwnd;

    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hwnd), NULL);

    old_proc = control->notif_proc;

    control->notif_proc = notif_proc;

    return old_proc;
}

NOTIFPROC GUIAPI GetNotificationCallback (HWND hwnd)
{
    PCONTROL control = (PCONTROL)hwnd;

    MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hwnd), NULL);

    return control->notif_proc;
}

/****************************** Hooks support ********************************/
#if !defined (_LITE_VERSION) || defined (_STAND_ALONE)
MSGHOOK GUIAPI RegisterKeyMsgHook (void* context, MSGHOOK hook)
{
    return (MSGHOOK)SendMessage (HWND_DESKTOP, 
            MSG_REGISTERKEYHOOK, (WPARAM)context, (LPARAM)hook);
}

MSGHOOK GUIAPI RegisterMouseMsgHook (void* context, MSGHOOK hook)
{
    return (MSGHOOK)SendMessage (HWND_DESKTOP, 
            MSG_REGISTERMOUSEHOOK, (WPARAM)context, (LPARAM)hook);
}

#else

/*
 * REQID_REGKEYHOOK        0x0016
 */
HWND GUIAPI RegisterKeyHookWindow (HWND hwnd, DWORD flag)
{
    HWND old_hwnd = HWND_NULL;

    if (!mgIsServer) {

        REGHOOKINFO info;
        REQUEST req;

        info.id_op = ID_REG_KEY;
        info.hwnd = hwnd;
        info.flag = flag;

        req.id = REQID_REGISTERHOOK;
        req.data = &info;
        req.len_data = sizeof (REGHOOKINFO);
        ClientRequest (&req, &old_hwnd, sizeof (HWND));
    }

    return old_hwnd;
}

HWND GUIAPI RegisterMouseHookWindow (HWND hwnd, DWORD flag)
{
    HWND old_hwnd = HWND_NULL;
    if (!mgIsServer) {

        REGHOOKINFO info;
        REQUEST req;

        info.id_op = ID_REG_MOUSE;
        info.hwnd = hwnd;
        info.flag = flag;

        req.id = REQID_REGISTERHOOK;
        req.data = &info;
        req.len_data = sizeof (REGHOOKINFO);
        ClientRequest (&req, &old_hwnd, sizeof (HWND));
    }

    return old_hwnd;
}
#endif

/**************************** IME support ************************************/
int GUIAPI RegisterIMEWindow (HWND hWnd)
{
#if defined(_LITE_VERSION) && !defined(_STAND_ALONE)
    if (mgIsServer) {
        MG_CHECK_RET (MG_IS_NORMAL_WINDOW(hWnd), -1);

        return SendMessage (HWND_DESKTOP, MSG_IME_REGISTER, (WPARAM)hWnd, 0);
    }else
        return ERR_INV_HWND;
#else
        return SendMessage (HWND_DESKTOP, MSG_IME_REGISTER, (WPARAM)hWnd, 0);
#endif
}

int GUIAPI UnregisterIMEWindow (HWND hWnd)
{
#if defined(_LITE_VERSION) && !defined(_STAND_ALONE)
    if (mgIsServer) {
        return SendMessage (HWND_DESKTOP, MSG_IME_UNREGISTER, (WPARAM)hWnd, 0);
    }else
        return ERR_IME_NOSUCHIMEWND;
#else
        return SendMessage (HWND_DESKTOP, MSG_IME_UNREGISTER, (WPARAM)hWnd, 0);
#endif
}

int GUIAPI GetIMEStatus (int StatusCode)
{
#if defined(_LITE_VERSION) && !defined(_STAND_ALONE)
    if (!mgIsServer) {
        REQUEST req;
        int ret;
    
        req.id = REQID_GETIMESTAT;
        req.data = &StatusCode;
        req.len_data = sizeof(int); 
    
        ClientRequest (&req, &ret, sizeof(int));
        return ret;
    } else
        return SendMessage (HWND_DESKTOP, 
            MSG_IME_GETSTATUS, (WPARAM)StatusCode, 0);
#else
    return SendMessage (HWND_DESKTOP, 
            MSG_IME_GETSTATUS, (WPARAM)StatusCode, 0);
#endif
}

int GUIAPI SetIMEStatus (int StatusCode, int Value)
{
#if defined(_LITE_VERSION) && !defined(_STAND_ALONE)
    if (!mgIsServer) {
        REQUEST req;
        unsigned int que_data;
        int ret;

        que_data = (StatusCode<<4)|Value;
    
        req.id = REQID_SETIMESTAT;
        req.data = &que_data;
        req.len_data = sizeof(int); 
    
        ClientRequest (&req, &ret, sizeof(int));
        return ret;
    } else
        return SendMessage (HWND_DESKTOP, 
            MSG_IME_SETSTATUS, (WPARAM)StatusCode, Value);
#else
    return SendMessage (HWND_DESKTOP, 
            MSG_IME_SETSTATUS, (WPARAM)StatusCode, Value);
#endif
}

int GUIAPI GetWindowElementMetricsEx (HWND hwnd, Uint16 item)
{
    Uint32 data;

    if (item < MWM_ITEM_NUMBER) {
        if (get_window_element_data (hwnd, WET_METRICS, item, &data) == WED_OK)
            return (int)data;

        return WinMainMetrics [item];
    }

    return -1;
}

int GUIAPI SetWindowElementMetricsEx (HWND hwnd, Uint16 item, int new_value)
{
    Uint32 new_data = (Uint32)new_value;
    Uint32 old_data;

    if (item < MWM_ITEM_NUMBER) {
        if (hwnd == HWND_DESKTOP || hwnd == 0) {
            int old_value = WinMainMetrics [item];
            WinMainMetrics [item] = new_value;
            return old_value;
        }

        if (set_window_element_data (hwnd, WET_METRICS, item, 
                                new_data, &old_data) >= 0)
            return (int)old_data;
    }

    return -1;
}

gal_pixel GUIAPI GetWindowElementColorEx (HWND hwnd, Uint16 item)
{
    Uint32 data;

    if (item < WEC_ITEM_NUMBER) {
        if (get_window_element_data (hwnd, WET_COLOR, item, &data) == WED_OK)
            return (gal_pixel)data;

        return WinElementColors [item];
    }

    return 0;
}

gal_pixel GUIAPI SetWindowElementColorEx (HWND hwnd, Uint16 item, 
                gal_pixel new_value)
{
    Uint32 new_data = (Uint32)new_value;
    Uint32 old_data;

    if (item < WEC_ITEM_NUMBER) {
        if (hwnd == HWND_DESKTOP || hwnd == 0) {
            gal_pixel old_value = WinElementColors [item];
            WinElementColors [item] = new_value;
            return old_value;
        }

        if (set_window_element_data (hwnd, WET_COLOR, item, 
                                new_data, &old_data) >= 0)
            return (gal_pixel)old_data;
    }

    return 0;
}

HICON GetWindowIcon (HWND hWnd)
{
    PMAINWIN pWin;
    if (!(pWin = CheckAndGetMainWindowPtr (hWnd)))
        return 0;

    return pWin->hIcon;
}

HICON SetWindowIcon (HWND hWnd, HICON hIcon, BOOL bRedraw)
{
    PMAINWIN pWin;
    HICON hOld;

    if (!(pWin = CheckAndGetMainWindowPtr (hWnd)))
        return 0;

    hOld = pWin->hIcon;
    pWin->hIcon = hIcon;

    if (bRedraw) { /* redraw the whole window */
        UpdateWindow (hWnd, TRUE);
    }
    else {  /* draw caption only */
        HDC hdc = GetDC (hWnd);
        wndDrawCaption (pWin, hdc, 
            !(pWin->dwStyle & WS_DISABLED) && (GetActiveWindow () == hWnd));
        ReleaseDC (hdc);
    }

    return hOld;
}

