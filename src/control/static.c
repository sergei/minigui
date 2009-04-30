/*
** $Id: static.c 7329 2007-08-16 02:59:29Z xgwang $
**
** static.c: the Static Control module.
**
** Copyright (C) 2003 ~ 2007 Feynman Software.
** Copyright (C) 1999 ~ 2002 Wei Yongming.
**
** All rights reserved by Feynman Software.
** 
** Current maitainer: Cui Wei
**
** Create date: 1999/5/22
*/

#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "minigui.h"
#include "gdi.h"
#include "window.h"
#include "ctrl/ctrlhelper.h"
#include "ctrl/static.h"
#include "cliprect.h"
#include "internals.h"
#include "ctrlclass.h"

#ifdef _CTRL_STATIC

#include "ctrlmisc.h"
#include "static_impl.h"

static int StaticControlProc (HWND hwnd, int message, WPARAM wParam, LPARAM lParam);

BOOL RegisterStaticControl (void)
{
    WNDCLASS WndClass;

    WndClass.spClassName = CTRL_STATIC;
    WndClass.dwStyle     = WS_NONE;
    WndClass.dwExStyle   = WS_EX_NONE;
    WndClass.hCursor     = GetSystemCursor (IDC_ARROW);
    WndClass.iBkColor    = GetWindowElementColor (BKC_CONTROL_DEF);
    WndClass.WinProc     = StaticControlProc;

    return AddNewControlClass (&WndClass) == ERR_OK;
}

#if 0
void StaticControlCleanup (void)
{
    // do nothing.
    return;
}
#endif

static int StaticControlProc (HWND hwnd, int message, WPARAM wParam, LPARAM lParam)
{
    RECT        rcClient;
    HDC         hdc;
    const char* spCaption;
    PCONTROL    pCtrl;
    UINT        uFormat;
    DWORD       dwStyle;         
    
    pCtrl = Control (hwnd);                        
    switch (message) {
        case MSG_CREATE:
            pCtrl->dwAddData2 = pCtrl->dwAddData;
            switch (pCtrl->dwStyle & SS_TYPEMASK) {
                case SS_GRAYRECT:
                    pCtrl->iBkColor = PIXEL_lightgray;
                break;
                case SS_BLACKRECT:
                    pCtrl->iBkColor = PIXEL_black;
                break;
                case SS_WHITERECT:
                    pCtrl->iBkColor = PIXEL_lightwhite;
                break;
            }
            return 0;
            
        case STM_GETIMAGE:
            return (int)(pCtrl->dwAddData2); 
        
        case STM_SETIMAGE:
        {
            int pOldValue;
            
            pOldValue  = (int)(pCtrl->dwAddData2);
            pCtrl->dwAddData2 = (DWORD)wParam;
            InvalidateRect (hwnd, NULL, (GetWindowStyle (hwnd) & SS_TYPEMASK) == SS_ICON);
            return pOldValue;
        }
           
        case MSG_GETDLGCODE:
            return DLGC_STATIC;

        case MSG_PAINT:
            hdc = BeginPaint (hwnd);
            GetClientRect (hwnd, &rcClient);
            dwStyle = GetWindowStyle (hwnd);
            if (dwStyle & WS_DISABLED)
                SetTextColor (hdc, GetWindowElementColorEx (hwnd, FGC_CONTROL_DISABLED));
            else
                SetTextColor (hdc, GetWindowElementColorEx (hwnd, FGC_CONTROL_NORMAL));

            switch (dwStyle & SS_TYPEMASK)
            {
                case SS_GRAYRECT:
                    SetBrushColor (hdc, PIXEL_lightgray);
                    FillBox (hdc, 0, 0, rcClient.right, rcClient.bottom);
                break;
                case SS_BLACKRECT:
                    SetBrushColor (hdc, PIXEL_black);
                    FillBox (hdc, 0, 0, rcClient.right, rcClient.bottom);
                break;
                case SS_WHITERECT:
                    SetBrushColor (hdc, PIXEL_lightwhite);
                    FillBox (hdc, 0, 0, rcClient.right, rcClient.bottom);
                break;
                
                case SS_BLACKFRAME:
                    SetPenColor (hdc, PIXEL_black);
                    Rectangle (hdc, 0, 0, rcClient.right - 1, rcClient.bottom - 1); 
                break;
                
                case SS_GRAYFRAME:
                    SetPenColor (hdc, PIXEL_lightgray);
                    Rectangle (hdc, 0, 0, rcClient.right - 1, rcClient.bottom - 1); 
                break;
                
                case SS_WHITEFRAME:
                    SetPenColor (hdc, PIXEL_lightwhite);
                    Rectangle (hdc, 0, 0, rcClient.right - 1, rcClient.bottom - 1); 
                break;
                
                case SS_BITMAP:
                    if (pCtrl->dwAddData2) {
                        int x = 0, y = 0, w, h;
                        PBITMAP bmp = (PBITMAP)(pCtrl->dwAddData2);

                        if (dwStyle & SS_REALSIZEIMAGE) {
                            w = bmp->bmWidth;
                            h = bmp->bmHeight;
                            if (dwStyle & SS_CENTERIMAGE) {
                                x = (rcClient.right - w) >> 1;
                                y = (rcClient.bottom - h) >> 1;
                            }
                        }
                        else {
                            x = y = 0;
                            w = RECTW (rcClient);
                            h = RECTH (rcClient);
                        }

                        FillBoxWithBitmap(hdc, x, y, w, h, bmp);
                    }
                break;
                
                case SS_ICON:
                    if (pCtrl->dwAddData2) {
                        int x = 0, y = 0, w, h;
                        HICON hIcon = (HICON)(pCtrl->dwAddData2);

                        if (dwStyle & SS_REALSIZEIMAGE) {
                            GetIconSize (hIcon, &w, &h);
                            if (dwStyle & SS_CENTERIMAGE) {
                                x = (rcClient.right - w) >> 1;
                                y = (rcClient.bottom - h) >> 1;
                            }
                        }
                        else {
                            x = y = 0;
                            w = RECTW (rcClient);
                            h = RECTH (rcClient);
                        }

                        DrawIcon (hdc, x, y, w, h, hIcon);
                    }
                break;
      
                case SS_SIMPLE:
                    SetBkMode (hdc, BM_TRANSPARENT);
                    SetBkColor (hdc, GetWindowBkColor (hwnd));
                    spCaption = GetWindowCaption (hwnd);
                    if (spCaption)
                        TextOut (hdc, 0, 0, spCaption); 
                break; 

                case SS_LEFT:
                case SS_CENTER:
                case SS_RIGHT:
                case SS_LEFTNOWORDWRAP:
                    uFormat = DT_TOP;
                    if ( (dwStyle & SS_TYPEMASK) == SS_LEFT)
                        uFormat |= DT_LEFT | DT_WORDBREAK;
                    else if ( (dwStyle & SS_TYPEMASK) == SS_CENTER)
                        uFormat |= DT_CENTER | DT_WORDBREAK;
                    else if ( (dwStyle & SS_TYPEMASK) == SS_RIGHT)
                        uFormat |= DT_RIGHT | DT_WORDBREAK;
                    else if ( (dwStyle & SS_TYPEMASK) == SS_LEFTNOWORDWRAP)
                        uFormat |= DT_LEFT | DT_SINGLELINE | DT_EXPANDTABS;

                    SetBkMode (hdc, BM_TRANSPARENT);
                    SetBkColor (hdc, GetWindowBkColor (hwnd));
                    spCaption = GetWindowCaption (hwnd);
                    if (dwStyle & SS_NOPREFIX)
                        uFormat |= DT_NOPREFIX;
                        
                    if (spCaption)
                        DrawText (hdc, spCaption, -1, &rcClient, uFormat);
                break;

                case SS_GROUPBOX:
                    spCaption = GetWindowCaption (hwnd);
                    if (spCaption)
                    {
                        SIZE size;
                        RECT rect;
                        if (GetWindowExStyle (hwnd) & WS_EX_TRANSPARENT)
                            SetBkMode (hdc, BM_TRANSPARENT);
                        else
                            SetBkMode (hdc, BM_OPAQUE);
                        SetBkColor(hdc, GetWindowBkColor (hwnd));
                        TextOut (hdc, pCtrl->pLogFont->size, 2, spCaption);

                        GetTextExtent (hdc, spCaption, -1, &size);
                        rect.left = pCtrl->pLogFont->size - 1;
                        rect.right = rect.left + size.cx + 2;
                        rect.top = 0;
                        rect.bottom = size.cy;
                        ExcludeClipRect (hdc, &rect);
                    }

#ifdef _FLAT_WINDOW_STYLE
                    DrawFlatControlFrameEx (hdc, hwnd, rcClient.left, 
                                    rcClient.top + (pCtrl->pLogFont->size >> 1),
                                    rcClient.right - 1,
                                    rcClient.bottom - 1, 2, 
                                    DF_3DBOX_NORMAL | DF_3DBOX_NOTFILL, 0);
#else
                    Draw3DBorderEx (hdc, hwnd, rcClient.left, 
                                    rcClient.top + (pCtrl->pLogFont->size >> 1),
                                    rcClient.right,
                                    rcClient.bottom);
#endif
                    
                break;
            }
            EndPaint (hwnd, hdc);
            return 0;

        case MSG_LBUTTONDBLCLK:
            if (GetWindowStyle (hwnd) & SS_NOTIFY)
                NotifyParent (hwnd, pCtrl->id, STN_DBLCLK);
            break;

        case MSG_LBUTTONDOWN:
            if (GetWindowStyle (hwnd) & SS_NOTIFY)
                NotifyParent (hwnd, pCtrl->id, STN_CLICKED);
            break;

        case MSG_NCLBUTTONDBLCLK:
            break;

        case MSG_NCLBUTTONDOWN:
            break;

        case MSG_HITTEST:
            dwStyle = GetWindowStyle (hwnd);
            if ((dwStyle & SS_TYPEMASK) == SS_GROUPBOX)
                return HT_TRANSPARENT;

            if (GetWindowStyle (hwnd) & SS_NOTIFY)
                return HT_CLIENT;
            else
                return HT_OUT;
        break;

        case MSG_FONTCHANGED:
            InvalidateRect (hwnd, NULL, TRUE);
            return 0;
            
        case MSG_SETTEXT:
            SetWindowCaption (hwnd, (char*)lParam);
            InvalidateRect (hwnd, NULL, TRUE);
            break;
            
        default:
            break;
    }

    return DefaultControlProc (hwnd, message, wParam, lParam);
}

#endif /* _CTRL_STATIC */

