/*
** $Id: ctrlmisc.c 8093 2007-11-16 07:37:30Z weiym $
**
** ctrlmisc.c: the help routines for MiniGUI control.
**
** Copyright (C) 2003 ~ 2007 Feynman Software.
** Copyright (C) 1999 ~ 2002 Wei Yongming.
**
** All rights reserved by Feynman Software.
**
** Current maintainer: Yan Xiaowei
**
** Create date: 1999/8/23
*/

#include "stdio.h"

#include "common.h"
#include "minigui.h"
#include "gdi.h"
#include "window.h"
#include "ctrl/ctrlhelper.h"
#include "ctrl/button.h"

#include "ctrlmisc.h"

void GUIAPI NotifyParentEx (HWND hwnd, int id, int code, DWORD add_data)
{
    NOTIFPROC notif_proc = GetNotificationCallback (hwnd);

    if (notif_proc) {
        notif_proc (hwnd, id, code, add_data);
    }
    else {
        SendNotifyMessage (GetParent (hwnd), MSG_COMMAND, 
                                 (WPARAM) MAKELONG (id, code), (LPARAM)hwnd);
    }
}

/****************************** Drawing Helpers *******************************/
void GUIAPI Draw3DThickFrameEx (HDC hdc, HWND hwnd, int l, int t, int r, int b, DWORD flags, gal_pixel fillc)
{
    if ((flags & DF_3DBOX_STATEMASK) == DF_3DBOX_NORMAL) {
        r--;
        b--;

        SetPenColor(hdc, GetWindowElementColor (WEC_3DFRAME_RIGHT_INNER));
        Rectangle(hdc, l, t, r, b);
        SetPenColor(hdc, GetWindowElementColor (WEC_3DFRAME_LEFT_INNER));
        Rectangle(hdc, l, t, r - 1, b - 1);
        SetPenColor(hdc, GetWindowElementColor (WEC_3DFRAME_LEFT_OUTER));
        MoveTo(hdc, l, b);
        LineTo(hdc, r, b);
        LineTo(hdc, r, t);
        SetPenColor(hdc, GetWindowElementColor (WEC_3DFRAME_RIGHT_OUTER));
        MoveTo(hdc, l + 1, b - 1);
        LineTo(hdc, l + 1, t + 1);
        LineTo(hdc, r - 1, t + 1); 
    }
    else {
#if 0
        SetPenColor(hdc, GetWindowElementColor (WEC_3DFRAME_LEFT_OUTER));
        MoveTo(hdc, l, b);
        LineTo(hdc, l, t);
        LineTo(hdc, r, t);
        SetPenColor(hdc, GetWindowElementColor (WEC_3DFRAME_LEFT_INNER));
        MoveTo(hdc, l + 1, b - 1);
        LineTo(hdc, l + 1, t + 1);
        LineTo(hdc, r - 1, t + 1);
        SetPenColor(hdc, GetWindowElementColor (WEC_3DFRAME_RIGHT_INNER));
        MoveTo(hdc, l + 1, b - 1);
        LineTo(hdc, r - 1, b - 1);
        LineTo(hdc, r - 1, t + 1);
        SetPenColor(hdc, GetWindowElementColor (WEC_3DFRAME_RIGHT_OUTER));
        MoveTo(hdc, l, b);
        LineTo(hdc, r, b);
        LineTo(hdc, r, t);
    }
#endif 
        SetPenColor(hdc, GetWindowElementColor (WEC_3DBOX_DARK));
        MoveTo(hdc, l, b);
        LineTo(hdc, l, t);
        LineTo(hdc, r, t);
        SetPenColor(hdc, GetWindowElementColor (WEC_3DBOX_NORMAL));
        MoveTo(hdc, l + 1, b - 1);
        LineTo(hdc, l + 1, t + 1);
        LineTo(hdc, r - 1, t + 1);
        SetPenColor(hdc, GetWindowElementColor (WEC_3DBOX_NORMAL));
        MoveTo(hdc, l + 1, b - 1);
        LineTo(hdc, r - 1, b - 1);
        LineTo(hdc, r - 1, t + 1);
        SetPenColor(hdc, GetWindowElementColor (WEC_3DBOX_LIGHT));
        MoveTo(hdc, l, b);
        LineTo(hdc, r, b);
        LineTo(hdc, r, t);
    }

    if (flags & DF_3DBOX_FILL) {
         SetBrushColor (hdc, fillc);
         FillBox (hdc, l + 2, t + 2, r - l - 3, b - t - 3);
    }
}

void GUIAPI Draw3DThinFrameEx (HDC hdc, HWND hwnd, int l, int t, int r, int b, DWORD flags, gal_pixel fillc)
{
    if ((flags & DF_3DBOX_STATEMASK) == DF_3DBOX_NORMAL) {
        SetPenColor(hdc, GetWindowElementColorEx (hwnd, WEC_3DBOX_DARK));
        MoveTo(hdc, l, b);
        LineTo(hdc, r, b);
        LineTo(hdc, r, t);
        SetPenColor(hdc, GetWindowElementColorEx (hwnd, WEC_3DBOX_LIGHT));
        MoveTo(hdc, l, b);
        LineTo(hdc, l, t);
        LineTo(hdc, r, t); 
    }
    else {
        SetPenColor(hdc, GetWindowElementColorEx (hwnd, WEC_3DBOX_LIGHT));
        MoveTo(hdc, l, b);
        LineTo(hdc, r, b);
        LineTo(hdc, r, t);
        SetPenColor(hdc, GetWindowElementColorEx (hwnd, WEC_3DBOX_DARK));
        MoveTo(hdc, l, b);
        LineTo(hdc, l, t);
        LineTo(hdc, r, t);
    }

    if (flags & DF_3DBOX_FILL) {
         SetBrushColor (hdc, fillc);
         FillBox(hdc, l + 1, t + 1, r - l - 2, b - t - 2);
    }
}

void GUIAPI Draw3DBorderEx (HDC hdc, HWND hwnd, int l, int t, int r, int b)
{
    SetPenColor(hdc, GetWindowElementColorEx (hwnd, WEC_3DBOX_LIGHT));
    Rectangle (hdc, l + 1, t + 1, r - 1, b - 1);

    SetPenColor(hdc, GetWindowElementColorEx (hwnd, WEC_3DBOX_DARK));
    Rectangle (hdc, l, t, r - 2, b - 2);
}

void GUIAPI DisabledTextOutEx (HDC hdc, HWND hwnd, int x, int y, const char* szText)
{
    SetBkMode (hdc, BM_TRANSPARENT);
    SetBkColor (hdc, GetWindowBkColor (hwnd));
    SetTextColor (hdc, GetWindowElementColorEx (hwnd, WEC_3DBOX_LIGHT));
    TextOut (hdc, x + 1, y + 1, szText);
    SetTextColor (hdc, GetWindowElementColorEx (hwnd, WEC_3DBOX_DARK));
    TextOut (hdc, x, y, szText);
}

#if 0
void Draw3DControlFrameEx (HDC hdc, HWND hwnd, int x0, int y0, int x1, int y1, 
            DWORD flags, gal_pixel fillc)
{
    int left, top, right, bottom;
    left = MIN (x0, x1);
    top  = MIN (y0, y1);
    right = MAX (x0, x1);
    bottom = MAX (y0, y1);
    
    if (flags & DF_3DBOX_FILL) {
        SetBrushColor (hdc, fillc);
        FillBox (hdc, left + 1, top + 1, right - left - 1 , bottom - top - 1); 
    }

    if ((flags & DF_3DBOX_STATEMASK) == DF_3DBOX_PRESSED) {
        SetPenColor (hdc, GetWindowElementColorEx (hwnd, WEC_3DBOX_DARK));
        MoveTo (hdc, left, bottom);
        LineTo (hdc, left, top);
        LineTo (hdc, right, top);
        SetPenColor (hdc, GetWindowElementColorEx (hwnd, WEC_3DBOX_LIGHT));
        MoveTo (hdc, left, bottom);
        LineTo (hdc, right, bottom);
        LineTo (hdc, right, top);

        left++; top++; right--; bottom--;
        SetPenColor (hdc, GetWindowElementColorEx (hwnd, WED_3DBOX_REVERSE));
        MoveTo (hdc, left, bottom);
        LineTo (hdc, left, top);
        LineTo (hdc, right, top);
        SetPenColor (hdc, GetWindowElementColorEx (hwnd, WEC_3DBOX_DARK));
        MoveTo (hdc, left, bottom);
        LineTo (hdc, right, bottom);
        LineTo (hdc, right, top);
    }
    else {
        SetPenColor (hdc, GetWindowElementColorEx (hwnd, WEC_3DBOX_LIGHT));
        MoveTo (hdc, left, bottom);
        LineTo (hdc, left, top);
        LineTo (hdc, right, top);
        SetPenColor (hdc, GetWindowElementColorEx (hwnd, WED_3DBOX_REVERSE));
        LineTo (hdc, right, bottom);
        LineTo (hdc, left, bottom);

        left++; top++; right--; bottom--;
        SetPenColor (hdc, GetWindowElementColorEx (hwnd, WEC_3DBOX_DARK));
        MoveTo (hdc, left, bottom);
        LineTo (hdc, left, top);
        LineTo (hdc, right, top);
        SetPenColor (hdc, GetWindowElementColorEx (hwnd, WED_3DBOX_REVERSE));
        LineTo (hdc, right, bottom);
        LineTo (hdc, left, bottom);
    }
}
#endif 

void Draw3DControlFrameEx (HDC hdc, HWND hwnd, int x0, int y0, int x1, int y1, 
            DWORD flags, gal_pixel fillc)
{
    int left, top, right, bottom;
    left = MIN (x0, x1);
    top  = MIN (y0, y1);
    right = MAX (x0, x1);
    bottom = MAX (y0, y1);
    
    if (flags & DF_3DBOX_FILL) {
        SetBrushColor (hdc, fillc);
        FillBox (hdc, left + 1, top + 1, right - left - 1 , bottom - top - 1); 
    }

    if ((flags & DF_3DBOX_STATEMASK) == DF_3DBOX_PRESSED) {
        SetPenColor (hdc, GetWindowElementColorEx (hwnd, WED_3DBOX_REVERSE));
        MoveTo (hdc, left, bottom);
        LineTo (hdc, left, top);
        LineTo (hdc, right, top);
        SetPenColor (hdc, GetWindowElementColorEx (hwnd, WEC_3DBOX_LIGHT));
        MoveTo (hdc, left, bottom);
        LineTo (hdc, right, bottom);
        LineTo (hdc, right, top);

        left++; top++; right--; bottom--;
        SetPenColor (hdc, GetWindowElementColorEx (hwnd, WEC_3DBOX_DARK));
        MoveTo (hdc, left, bottom);
        LineTo (hdc, left, top);
        LineTo (hdc, right, top);
        SetPenColor (hdc, GetWindowElementColorEx (hwnd, WEC_3DBOX_NORMAL));
        MoveTo (hdc, left, bottom);
        LineTo (hdc, right, bottom);
        LineTo (hdc, right, top);
    }
    else {
        SetPenColor (hdc, GetWindowElementColorEx (hwnd, WEC_3DBOX_LIGHT));
        MoveTo (hdc, left, bottom);
        LineTo (hdc, left, top);
        LineTo (hdc, right, top);
        SetPenColor (hdc, GetWindowElementColorEx (hwnd, WED_3DBOX_REVERSE));
        LineTo (hdc, right, bottom);
        LineTo (hdc, left, bottom);

        left++; top++; right--; bottom--;
        MoveTo (hdc, left, bottom);
#if 0
        SetPenColor (hdc, GetWindowElementColorEx (hwnd, WEC_3DBOX_DARK));
        LineTo (hdc, left, top);
        LineTo (hdc, right, top);
#endif 
        SetPenColor (hdc, GetWindowElementColorEx (hwnd, WEC_3DBOX_DARK));
        LineTo (hdc, right, bottom);
        LineTo (hdc, right, top);
    }
}

void DrawFlatControlFrameEx (HDC hdc, HWND hwnd, int x0, int y0, int x1, int y1, 
            int corner, DWORD flags, gal_pixel fillc)
{
    int left, top, right, bottom;
    left = MIN (x0, x1);
    top  = MIN (y0, y1);
    right = MAX (x0, x1);
    bottom = MAX (y0, y1);
    
    if (flags & DF_3DBOX_FILL) {
        SetBrushColor (hdc, fillc);
        FillBox (hdc, left, top, right - left + 1, bottom - top + 1); 
    }

    SetPenColor (hdc, GetWindowElementColor (WEC_FLAT_BORDER));

    if (corner < 1) {
        Rectangle (hdc, left, top, right, bottom);
        return;
    }

    if ((flags & DF_3DBOX_STATEMASK) == DF_3DBOX_NORMAL) {
        right --; bottom --;
    }

    MoveTo (hdc, left + corner, top);
    LineTo (hdc, right - corner, top);
    LineTo (hdc, right, top + corner);
    LineTo (hdc, right, bottom - corner);
    LineTo (hdc, right - corner, bottom);
    LineTo (hdc, left + corner, bottom);
    LineTo (hdc, left, bottom - corner);
    LineTo (hdc, left, top + corner);
    LineTo (hdc, left + corner, top);

#if 0
    corner++;
    if ((flags & DF_3DBOX_STATEMASK) == DF_3DBOX_NORMAL) {
        MoveTo (hdc, right + 1, top + corner);
        LineTo (hdc, right + 1, bottom - corner);
        MoveTo (hdc, left + corner, bottom + 1);
        LineTo (hdc, right - corner + 1, bottom + 1);
    }
    else {
        MoveTo (hdc, left + corner, top + 1);
        LineTo (hdc, right - corner, top + 1);
        MoveTo (hdc, left + 1, top + corner);
        LineTo (hdc, left + 1, bottom - corner);
    }
#endif 
}

void GUIAPI DrawBoxFromBitmap (HDC hdc, const RECT* box, const BITMAP* bmp, BOOL h_v, BOOL do_clip)
{
    int bmp_w, bmp_h, bmp_x, bmp_y, x, y;

    if (do_clip)
        ClipRectIntersect (hdc, box);

    if (h_v) {
        bmp_w = bmp->bmWidth/3;
        bmp_h = bmp->bmHeight;
        bmp_y = (box->bottom + box->top - bmp_h)>>1;

        FillBoxWithBitmapPart (hdc, box->left, bmp_y,
                        bmp_w, bmp_h, 0, 0, bmp, 0, 0);
        for (x = box->left + bmp_w; x < box->right - bmp_w; x += bmp_w)
            FillBoxWithBitmapPart (hdc, x, bmp_y,
                        bmp_w, bmp_h, 0, 0, bmp, bmp_w, 0);
        FillBoxWithBitmapPart (hdc, box->right - bmp_w, bmp_y, 
                        bmp_w, bmp_h, 0, 0, bmp, bmp_w*2, 0);
    }
    else {
        bmp_w = bmp->bmWidth;
        bmp_h = bmp->bmHeight/3;
        bmp_x = (box->right + box->left - bmp_w)>>1;

        FillBoxWithBitmapPart (hdc, bmp_x, box->top,
                        bmp_w, bmp_h, 0, 0, bmp, 0, 0);
        for (y = box->top + bmp_h; y < box->bottom - bmp_h; y += bmp_h)
            FillBoxWithBitmapPart (hdc, bmp_x, y,
                        bmp_w, bmp_h, 0, 0, bmp, 0, bmp_h);
        FillBoxWithBitmapPart (hdc, bmp_x, box->bottom - bmp_h,
                        bmp_w, bmp_h, 0, 0, bmp, 0, bmp_h*2);
    }
}

int EditOnEraseBackground (HWND hWnd, HDC hdc, const RECT* pClipRect)
{
    RECT rcTemp;
    BOOL fGetDC = FALSE;
    BOOL hidden;

    if (GetWindowExStyle (hWnd) & WS_EX_TRANSPARENT)
        return 0;

    hidden = HideCaret (hWnd);

    if (hdc == 0) {
        hdc = GetClientDC (hWnd);
        fGetDC = TRUE;
    }

    GetClientRect (hWnd, &rcTemp);

    if (pClipRect)
        ClipRectIntersect (hdc, pClipRect);

    if (GetWindowStyle (hWnd) & WS_DISABLED)
        SetBrushColor (hdc, GetWindowElementColor (BKC_EDIT_DISABLED));
    else
        SetBrushColor (hdc, GetWindowBkColor (hWnd));

    FillBox (hdc, rcTemp.left, rcTemp.top, RECTW (rcTemp), RECTH (rcTemp));

    if (fGetDC)
        ReleaseDC (hdc);

    if (hidden)
        ShowCaret (hWnd);

    return 0;
}

int GUIAPI DefaultPageProc (HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{
    HWND hCurFocus;

    switch (message) {
    case MSG_DLG_GETDEFID:
    {
        HWND hDef;

        hDef = GetDlgDefPushButton (hWnd);
        if (hDef)
            return GetDlgCtrlID (hDef);
        return 0;
    }
    
    case MSG_DESTROY:
        DestroyAllControls (hWnd);
        break;

    case MSG_DLG_SETDEFID:
    {
        HWND hOldDef;
        HWND hNewDef;

        hNewDef = GetDlgItem (hWnd, wParam);
        if (SendMessage (hNewDef, MSG_GETDLGCODE, 0, 0L) & DLGC_PUSHBUTTON) {
            hOldDef = GetDlgDefPushButton (hWnd);
            if (hOldDef) {
                ExcludeWindowStyle (hOldDef, BS_DEFPUSHBUTTON);
                InvalidateRect (hOldDef, NULL, TRUE);
            }
            IncludeWindowStyle (hNewDef, BS_DEFPUSHBUTTON);
            InvalidateRect (hNewDef, NULL, TRUE);

            return (int)hOldDef;
        }
        break;
    }
        
    case MSG_KEYDOWN:
        if ((hCurFocus = GetFocusChild (hWnd))) {
            if (SendMessage (hCurFocus, MSG_GETDLGCODE, 0, 0L) 
                        & DLGC_WANTALLKEYS)
                break;
        }

        switch (LOWORD (wParam)) {
        case SCANCODE_KEYPADENTER:
        case SCANCODE_ENTER:
        {
            HWND hDef;

            if (SendMessage (hCurFocus, MSG_GETDLGCODE, 0, 0L) & DLGC_PUSHBUTTON)
                break;

            hDef = GetDlgDefPushButton (hWnd);
            if (hDef) {
                SendMessage (hWnd, MSG_COMMAND, GetDlgCtrlID (hDef), 0L);
                return 0;
            }
            break;
        }

        case SCANCODE_ESCAPE:
            SendMessage (hWnd, MSG_COMMAND, IDCANCEL, 0L);
            return 0;

        case SCANCODE_TAB:
        {
            HWND hNewFocus;

            if (hCurFocus) {
                if (SendMessage (hCurFocus, MSG_GETDLGCODE, 0, 0L) 
                        & DLGC_WANTTAB)
                    break;
            }

            if (lParam & KS_SHIFT)
                hNewFocus = GetNextDlgTabItem (hWnd, hCurFocus, TRUE);
            else
                hNewFocus = GetNextDlgTabItem (hWnd, hCurFocus, FALSE);

            if (hNewFocus != hCurFocus) {
                SetFocus (hNewFocus);
//                SendMessage (hWnd, MSG_DLG_SETDEFID, GetDlgCtrlID (hNewFocus), 0L);
            }

            return 0;
        }

        case SCANCODE_CURSORBLOCKDOWN:
        case SCANCODE_CURSORBLOCKRIGHT:
        case SCANCODE_CURSORBLOCKUP:
        case SCANCODE_CURSORBLOCKLEFT:
        {
            HWND hNewFocus;
                
            if (hCurFocus) {
                if (SendMessage (hCurFocus, MSG_GETDLGCODE, 0, 0L) 
                        & DLGC_WANTARROWS)
                    break;
            }

            if (LOWORD (wParam) == SCANCODE_CURSORBLOCKDOWN
                    || LOWORD (wParam) == SCANCODE_CURSORBLOCKRIGHT)
                hNewFocus = GetNextDlgGroupItem (hWnd, hCurFocus, FALSE);
            else
                hNewFocus = GetNextDlgGroupItem (hWnd, hCurFocus, TRUE);
            
            if (hNewFocus != hCurFocus) {
                if (SendMessage (hCurFocus, MSG_GETDLGCODE, 0, 0L) 
                        & DLGC_STATIC)
                    return 0;

                SetFocus (hNewFocus);
//                SendMessage (hWnd, MSG_DLG_SETDEFID, GetDlgCtrlID (hNewFocus), 0L);

                if (SendMessage (hNewFocus, MSG_GETDLGCODE, 0, 0L)
                        & DLGC_RADIOBUTTON) {
                    SendMessage (hNewFocus, BM_CLICK, 0, 0L);
                    ExcludeWindowStyle (hCurFocus, WS_TABSTOP);
                    IncludeWindowStyle (hNewFocus, WS_TABSTOP);
                }
            }

            return 0;
        }
        break;
        }
        break;
    }
    
    return DefaultControlProc (hWnd, message, wParam, lParam);
}

