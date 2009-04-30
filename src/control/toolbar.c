/*
** $Id: toolbar.c 7329 2007-08-16 02:59:29Z xgwang $
**
** toolbar.c: the toolbar control module.
**
** Copyright (C) 2003 ~ 2007 Feynman Software.
** Copyright (C) 1999 ~ 2002 Wei Yongming.
**
** All rights reserved by Feynman Software.
** 
** Current maintainer: Yan Xiaowei
**
** Create date: 2000/9/20
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "minigui.h"
#include "gdi.h"
#include "window.h"
#include "ctrl/ctrlhelper.h"
#include "ctrl/toolbar.h"
#include "cliprect.h"
#include "internals.h"
#include "ctrlclass.h"

#ifdef _CTRL_TOOLBAR

#include "ctrlmisc.h"
#include "toolbar_impl.h"

static int ToolbarCtrlProc (HWND hWnd, int message, WPARAM wParam, LPARAM lParam);

BOOL RegisterToolbarControl (void)
{
    WNDCLASS WndClass;
    
    WndClass.spClassName = CTRL_TOOLBAR;
    WndClass.dwStyle     = WS_NONE;
    WndClass.dwExStyle   = WS_EX_NONE;
    WndClass.hCursor     = GetSystemCursor (IDC_ARROW);
    WndClass.iBkColor    = GetWindowElementColor (BKC_CONTROL_DEF);
    WndClass.WinProc     = ToolbarCtrlProc;

    return AddNewControlClass (&WndClass) == ERR_OK;
}

static void DrawToolBox (HDC hdc, PTOOLBARCTRL pdata)
{
    TOOLBARITEMDATA*  tmpdata;

    tmpdata = pdata->head;
  
    while (tmpdata != NULL) {
       FillBoxWithBitmap (hdc, tmpdata->RcTitle.left , tmpdata->RcTitle.top,
                        0, 0, tmpdata->NBmp);
       tmpdata = tmpdata->next;
    }
}

static TOOLBARITEMDATA* GetCurTag(int posx,int posy, PTOOLBARCTRL pdata)
{
    TOOLBARITEMDATA*  tmpdata;
    tmpdata = pdata->head;
    while (tmpdata != NULL) { 
        if (PtInRect (&tmpdata->RcTitle, posx, posy)) {
            return tmpdata;    
        }
        tmpdata = tmpdata->next;
    }

    return NULL;         
}

static void HilightToolBox (HWND hWnd, TOOLBARITEMDATA* pItemdata)
{
    HDC hdc;

    hdc = GetClientDC (hWnd);

    FillBoxWithBitmap (hdc, pItemdata->RcTitle.left, pItemdata->RcTitle.top,
                    0,0, pItemdata->HBmp);

    ReleaseDC (hdc);
}

static void UnhilightToolBox (HWND hWnd, TOOLBARITEMDATA* pItemdata)
{
    HDC hdc;

    hdc = GetClientDC (hWnd);

    FillBoxWithBitmap (hdc, pItemdata->RcTitle.left, pItemdata->RcTitle.top,
                    0, 0, pItemdata->DBmp);

    ReleaseDC (hdc);
}

static int ToolbarCtrlProc (HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{
    HDC         hdc;
    PCONTROL    pCtrl;
    DWORD       dwStyle;
    PTOOLBARCTRL TbarData;
    PTOOLBARITEMDATA pTbid;
    
    pCtrl = Control (hWnd);
    dwStyle = pCtrl->dwStyle;

    switch (message) {   
        case MSG_CREATE:
        {
            DWORD data; 
            data = GetWindowAdditionalData (hWnd);

            TbarData = (TOOLBARCTRL*) calloc (1, sizeof (TOOLBARCTRL));
            TbarData->nCount = 0;
            TbarData->head = TbarData->tail = NULL;
            TbarData->iLBDn = 0;
            TbarData->ItemWidth = HIWORD(data);
            TbarData->ItemHeight = LOWORD(data);
            SetWindowAdditionalData2 (hWnd,(DWORD)TbarData);
        }
        break;

        case MSG_DESTROY:
        { 
            TOOLBARITEMDATA* unloaddata, *tmp;
            TbarData=(PTOOLBARCTRL) GetWindowAdditionalData2(hWnd);
            unloaddata = TbarData->head;

            while (unloaddata != NULL) {
                UnloadBitmap ((PBITMAP)(unloaddata->NBmp));
                free ((PBITMAP)(unloaddata->NBmp));

                if (unloaddata->HBmp != unloaddata->NBmp) {
                    UnloadBitmap ((PBITMAP)(unloaddata->HBmp));
                    free ((PBITMAP)(unloaddata->HBmp));
                }
                if (unloaddata->DBmp != unloaddata->NBmp) {
                    UnloadBitmap ((PBITMAP)(unloaddata->DBmp));
                    free ((PBITMAP)(unloaddata->DBmp));
                }

                tmp = unloaddata->next;
                free (unloaddata);
                unloaddata = tmp;
            }

            free (TbarData);
            break;
        }

        case MSG_PAINT:
        {
            TbarData = (PTOOLBARCTRL)GetWindowAdditionalData2(hWnd);
            hdc = BeginPaint (hWnd);
            DrawToolBox (hdc, TbarData);
            EndPaint (hWnd, hdc);
            return 0;
        }

        case TBM_ADDITEM:
        {  
            TOOLBARITEMINFO* TbarInfo = NULL;
            TOOLBARITEMDATA* ptemp;
            RECT rc;
            TbarData=(PTOOLBARCTRL) GetWindowAdditionalData2(hWnd);
            TbarInfo = (TOOLBARITEMINFO*) lParam;
                
            GetClientRect (hWnd, &rc);
                
            ptemp = (TOOLBARITEMDATA*)malloc (sizeof (TOOLBARITEMDATA));
            ptemp->id = TbarInfo->id;

            if (TbarData->tail == NULL) 
                ptemp->RcTitle.left =  0;
            else
                ptemp->RcTitle.left = TbarData->tail->RcTitle.right;

            ptemp->RcTitle.right = ptemp->RcTitle.left + TbarData->ItemWidth;
            ptemp->RcTitle.top = 0;
            ptemp->RcTitle.bottom = ptemp->RcTitle.top + TbarData->ItemHeight;

            ptemp->NBmp = (BITMAP*)malloc (sizeof (BITMAP));
            LoadBitmap (HDC_SCREEN, ptemp->NBmp, TbarInfo->NBmpPath);
            if (TbarInfo->HBmpPath [0]) {
                ptemp->HBmp = (BITMAP*)malloc (sizeof (BITMAP));
                LoadBitmap (HDC_SCREEN, ptemp->HBmp, TbarInfo->HBmpPath);
            }
            else
                ptemp->HBmp = ptemp->NBmp;
                    
            if (TbarInfo->DBmpPath [0]) {
                ptemp->DBmp = (BITMAP*)malloc (sizeof (BITMAP));
                LoadBitmap (HDC_SCREEN, ptemp->DBmp, TbarInfo->DBmpPath);
            }
            else
                ptemp->DBmp = ptemp->NBmp;

            ptemp->next = NULL;

            if (TbarData->nCount == 0)
                TbarData->head = TbarData->tail = ptemp;
            else if (TbarData->nCount > 0) { 
                TbarData->tail->next = ptemp;
                TbarData->tail = ptemp;
            }
            ptemp->insPos = TbarData->nCount;
            TbarData->nCount++;

            InvalidateRect (hWnd, NULL, FALSE);
            break;
        }

        case MSG_MOUSEMOVEIN:
        {
            TbarData = (PTOOLBARCTRL) GetWindowAdditionalData2(hWnd);
            if (!wParam) {
                hdc = GetClientDC (hWnd);
                DrawToolBox (hdc, TbarData);
                ReleaseDC (hdc);
            }

            break;
        }

        case MSG_LBUTTONDOWN:
        {
            int posx, posy;
            TbarData=(PTOOLBARCTRL) GetWindowAdditionalData2(hWnd);

            posx = LOSWORD (lParam);
            posy = HISWORD (lParam);

            if (GetCapture () == hWnd)
                break;
            SetCapture (hWnd);
                
            if ((pTbid = GetCurTag (posx,posy,TbarData)) == NULL)
                break; 

            TbarData->iLBDn = 1;
            TbarData->iSel = pTbid->insPos;
            UnhilightToolBox (hWnd, GetCurTag (posx, posy, TbarData));
            break;
        }

        case MSG_LBUTTONUP:
        {
            int x, y;
            TbarData=(PTOOLBARCTRL) GetWindowAdditionalData2(hWnd);
            x = LOSWORD(lParam);
            y = HISWORD(lParam);
            
            TbarData->iLBDn = 0;
            
            if (GetCapture() != hWnd)
                break;
            ReleaseCapture ();

            ScreenToClient (hWnd, &x, &y);
            if ((pTbid = GetCurTag(x, y, TbarData)) == NULL) {
                hdc = GetClientDC (hWnd);
                DrawToolBox (hdc, TbarData);
                ReleaseDC (hdc);
                break;
            }
            else 
                 HilightToolBox (hWnd, GetCurTag (x, y, TbarData));
            
            InvalidateRect (hWnd, NULL, FALSE);
            if (TbarData->iSel == pTbid->insPos)
                NotifyParent (hWnd, pCtrl->id, pTbid->id);

            break;
        }

        case MSG_MOUSEMOVE:
        {
            int x, y;
            TbarData = (PTOOLBARCTRL) GetWindowAdditionalData2(hWnd);
            x = LOSWORD(lParam);
            y = HISWORD(lParam);

            if (TbarData->iLBDn == 1)
                ScreenToClient (hWnd, &x, &y);

            if (( pTbid = GetCurTag (x, y, TbarData)) == NULL) {
                hdc = GetClientDC (hWnd);
                DrawToolBox (hdc, TbarData);
                ReleaseDC (hdc);
                break;
            }
            
            if (TbarData->iMvOver != pTbid->insPos) {
                TbarData->iMvOver = pTbid->insPos;
                    
                hdc = GetClientDC (hWnd);
                DrawToolBox (hdc, TbarData);
                ReleaseDC (hdc);
            }
                    
            if (TbarData->iSel == pTbid->insPos && TbarData->iLBDn == 1)
                UnhilightToolBox (hWnd, GetCurTag (x, y, TbarData));
            else if ( TbarData->iLBDn == 0 ) 
                HilightToolBox (hWnd, GetCurTag(x, y, TbarData));

            break;
        }
    }

    return DefaultControlProc (hWnd, message, wParam, lParam);
}

#endif /* _CTRL_TOOLBAR */

