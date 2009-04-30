/*
** $Id: button.c 8267 2007-11-27 06:27:34Z weiym $
**
** button.c: the Button Control module.
**
** Copyright (C) 2003 ~ 2007 Feynman Software.
** Copyright (C) 1999 ~ 2002 Wei Yongming.
**
** All rights reserved by Feynman Software.
**
** Current maintainer: Wang Xiaogang.
**
** Create date: 1999/8/23
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "minigui.h"
#include "gdi.h"
#include "window.h"
#include "ctrl/ctrlhelper.h"
#include "ctrl/button.h"
#include "cliprect.h"
#include "internals.h"
#include "ctrlclass.h"
#include "fixedmath.h"

#ifdef _CTRL_BUTTON

#include "button_impl.h"
#include "ctrlmisc.h"

#ifdef _FLAT_WINDOW_STYLE 
//#define BTN_WIDTH_BORDER    1
#ifdef _GRAY_SCREEN
#define myDrawButton(hdc, hwnd, x0, y0, x1, y1, flags, fillc) \
                 DrawFlatControlFrameEx(hdc, hwnd, x0, y0, x1, y1, 3, flags, fillc)
#endif

#define FocusRect(hdc, x, y, w, h)

#elif defined (_PHONE_WINDOW_STYLE)

//#define BTN_WIDTH_BORDER    2

#define myDrawButton btnDrawFashionButton

//#define FocusRect(hdc, x, y, w, h) 

#else

//#define BTN_WIDTH_BORDER    4

#define myDrawButton Draw3DControlFrameEx

#endif

#ifdef _PHONE_WINDOW_STYLE
#   define BTN_WIDTH_BMP       16
#   define BTN_HEIGHT_BMP      16
#   define BTN_INTER_BMPTEXT   2
#else
#   define BTN_WIDTH_BMP       14
#   define BTN_HEIGHT_BMP      13
#   define BTN_INTER_BMPTEXT   2
#endif

#define BUTTON_STATUS(pCtrl)   (((PBUTTONDATA)(pCtrl->dwAddData2))->status)
#define BUTTON_DATA(pCtrl)     (((PBUTTONDATA)(pCtrl->dwAddData2))->data)

static const BITMAP* bmp_button;

#define BUTTON_BMP              bmp_button

#define BUTTON_OFFSET      1

static int ButtonCtrlProc (HWND hWnd, int uMsg, WPARAM wParam, LPARAM lParam);

BOOL RegisterButtonControl (void)
{
    WNDCLASS WndClass;

#ifdef _PHONE_WINDOW_STYLE
    if ((bmp_button = GetStockBitmap (STOCKBMP_BUTTON, 0, 0)) == NULL)
    {
        printf("GetStockBitmap failure! STOCKBMP_BUTTON\n");
        return FALSE;
    }
#else
    if ((bmp_button = GetStockBitmap (STOCKBMP_BUTTON, -1, -1)) == NULL)
        return FALSE;
#endif

    WndClass.spClassName = CTRL_BUTTON;
    WndClass.dwStyle     = WS_NONE;
    WndClass.dwExStyle   = WS_EX_NONE;
    WndClass.hCursor     = GetSystemCursor (IDC_ARROW);
    WndClass.iBkColor    = GetWindowElementColor (BKC_CONTROL_DEF); 
    WndClass.WinProc     = ButtonCtrlProc;

    return AddNewControlClass (&WndClass) == ERR_OK;
}

#if 0
void ButtonControlCleanup (void)
{
    // do nothing.
}
#endif

#ifdef _PHONE_WINDOW_STYLE
#if 0
static void fill_corner(HDC hdc, int left, int top,
                 int right, int bottom, int corner, gal_pixel fillc)
{
    SetBrushColor(hdc, fillc);
    FillBox(hdc, left, top, corner, corner);
    FillBox(hdc, right-corner, top, corner, corner);
    FillBox(hdc, right-corner, bottom-corner, corner, corner);
    FillBox(hdc, left, bottom-corner, corner, corner);
}
#endif

static void draw_fashion_border(HDC hdc, int left, int top, 
                                int right, int bottom, int corner)
{
     SetPenColor(hdc, PIXEL_black);
     MoveTo(hdc, left+corner, top);
     LineTo(hdc, right-corner, top);
     LineTo(hdc, right, top+corner);
     LineTo(hdc, right, bottom-corner);
     LineTo(hdc, right-corner, bottom);
     LineTo(hdc, left+corner, bottom);
     LineTo(hdc, left, bottom-corner);
     LineTo(hdc, left, top+corner);
     LineTo(hdc, left+corner, top);

}

typedef struct _fixed_rgb
{
    fixed r;
    fixed g;
    fixed b;

}FIXED_RGB;

static FIXED_RGB calc_drgb(RGB rgb1, RGB rgb2, int dist)
{
    FIXED_RGB drgb;
    fixed d;

    if (dist == 0){
        drgb.r = rgb2.r << 16;
        drgb.g = rgb2.g << 16;
        drgb.b = rgb2.b << 16;

        return drgb;
    }

    d = abs(dist) << 16;

    drgb.r = fixdiv((rgb1.r - rgb2.r) << 16, d);
    drgb.g = fixdiv((rgb1.g - rgb2.g) << 16, d);
    drgb.b = fixdiv((rgb1.b - rgb2.b) << 16, d);

    return drgb;

}

#define INCRGB(crgb, drgb) \
          crgb.r += drgb.r;\
          crgb.g += drgb.g;\
          crgb.b += drgb.b 

#define INITRGB(rgb, rdata, gdata, bdata)\
                           rgb.r = rdata;\
                           rgb.g = gdata;\
                           rgb.b = bdata

#define FixedRGB2Pixel(hdc, r, g, b)\
          RGB2Pixel(hdc, r>>16, g>>16, b>>16)
                    
static void btnDrawFashionButton(HDC hdc, HWND hwnd, int left, int top ,int right,
                               int bottom, DWORD flags, gal_pixel fillc)
{
    int i = 0;
    int l, r, t, b;
    FIXED_RGB drgb, crgb;
    RGB rgb1, rgb2;
    int corner = 2;


    l = left;
    t = top;
    r = right;
    b = bottom;


    if ((flags & DF_3DBOX_STATEMASK) == DF_3DBOX_NORMAL){/* push button color */
        INITRGB(rgb1, 0x55, 0x55, 0x55);
        INITRGB(rgb2, 0xff, 0xff, 0xff);
        INITRGB(crgb, 0xff<<16, 0xff<<16, 0xff<<16);
        drgb = calc_drgb(rgb1, rgb2, bottom - top); 
    }
    else{/* pushed button color */
        INITRGB(rgb1, 0xdd, 0xdd, 0xdd);
        INITRGB(rgb2, 0x98, 0xb4, 0xff);
        INITRGB(crgb, 0x98<<16, 0xb4<<16, 0xff<<16);
        drgb = calc_drgb(rgb1, rgb2, b/2); 
    }

    for (i=b/2; i>=t+corner; i--){
        SetPenColor(hdc, FixedRGB2Pixel(hdc, crgb.r, crgb.g, crgb.b));
        MoveTo(hdc, l, i);
        LineTo(hdc, r, i);

        INCRGB(crgb, drgb);
    }

    SetPenColor(hdc, FixedRGB2Pixel(hdc, crgb.r, crgb.g, crgb.b));
    MoveTo(hdc, l+1, t + 1);
    LineTo(hdc, r-2, t + 1);

    INCRGB(crgb, drgb);
    SetPenColor(hdc, FixedRGB2Pixel(hdc, crgb.r, crgb.g, crgb.b));
    MoveTo(hdc, l+2, t);
    LineTo(hdc, r-3, t);

    if ((flags & DF_3DBOX_STATEMASK) == DF_3DBOX_NORMAL){/* push button color */
        INITRGB(crgb, 0xff<<16, 0xff<<16, 0xff<<16);
    }
    else{/* pushed button color */
        INITRGB(crgb, 0x98<<16, 0xb4<<16, 0xff<<16);
    }

    for (i=b/2+1; i<b-corner; i++){
        SetPenColor(hdc, FixedRGB2Pixel(hdc, crgb.r, crgb.g, crgb.b));
        MoveTo(hdc, l, i);
        LineTo(hdc, r, i);

        INCRGB(crgb, drgb);
    }

    SetPenColor(hdc, FixedRGB2Pixel(hdc, crgb.r, crgb.g, crgb.b));
    MoveTo(hdc, l+1, b-2);
    LineTo(hdc, r-2, b-2);

    INCRGB(crgb, drgb);
    SetPenColor(hdc, FixedRGB2Pixel(hdc, crgb.r, crgb.g, crgb.b));
    MoveTo(hdc, l+2, b-1);
    LineTo(hdc, r-3, b-1);

    draw_fashion_border(hdc, left, top, right-1, bottom-1, corner);
}
#endif

static void btnGetRects (HWND hWnd, DWORD dwStyle,
                                    RECT* prcClient, 
                                    RECT* prcText, 
                                    RECT* prcBitmap)
{
    GetClientRect (hWnd, prcClient);
    
#ifndef _PHONE_WINDOW_STYLE
    prcClient->right --;
    prcClient->bottom --;
#endif

    SetRect (prcText, (prcClient->left   + BTN_WIDTH_BORDER),
                      (prcClient->top    + BTN_WIDTH_BORDER),
                      (prcClient->right  - BTN_WIDTH_BORDER),
                      (prcClient->bottom - BTN_WIDTH_BORDER));

    SetRectEmpty (prcBitmap);

    if ( ((dwStyle & BS_TYPEMASK) < BS_CHECKBOX)
        || (dwStyle & BS_PUSHLIKE))
        return;

    if (dwStyle & BS_LEFTTEXT) {
        SetRect (prcText, prcClient->left + 1,
                          prcClient->top + 1,
                          prcClient->right - BTN_WIDTH_BMP - BTN_INTER_BMPTEXT,
                          prcClient->bottom - 1);
        SetRect (prcBitmap, prcClient->right - BTN_WIDTH_BMP,
                            prcClient->top + 1,
                            prcClient->right - 1,
                            prcClient->bottom - 1);
    }
    else {
        SetRect (prcText, prcClient->left + BTN_WIDTH_BMP + BTN_INTER_BMPTEXT,
                          prcClient->top + 1,
                          prcClient->right - 1,
                          prcClient->bottom - 1);
        SetRect (prcBitmap, prcClient->left + 1,
                            prcClient->top + 1,
                            prcClient->left + BTN_WIDTH_BMP,
                            prcClient->bottom - 1);
    }

}

static void btnGetTextBoundsRect (PCONTROL pCtrl, HDC hdc, DWORD dwStyle, 
                const RECT* prcText, RECT* prcBounds)
{
    UINT uFormat;
    RECT tmp_rc;

    tmp_rc.top = prcText->top - BUTTON_OFFSET;
    tmp_rc.left = prcText->left;
    tmp_rc.right = prcText->right;
    tmp_rc.bottom = prcText->bottom;
  
    *prcBounds = *prcText;

    if (dwStyle & BS_MULTLINE)
        uFormat = DT_WORDBREAK;
    else
        uFormat = DT_SINGLELINE;

    if ((dwStyle & BS_TYPEMASK) == BS_PUSHBUTTON
            || (dwStyle & BS_TYPEMASK) == BS_DEFPUSHBUTTON
            || (dwStyle & BS_PUSHLIKE))
        uFormat |= DT_CENTER | DT_VCENTER;
    else {
        if ((dwStyle & BS_ALIGNMASK) == BS_RIGHT)
            uFormat = DT_WORDBREAK | DT_RIGHT;
        else if ((dwStyle & BS_ALIGNMASK) == BS_CENTER)
            uFormat = DT_WORDBREAK | DT_CENTER;
        else uFormat = DT_WORDBREAK | DT_LEFT;
            
        if ((dwStyle & BS_ALIGNMASK) == BS_TOP)
            uFormat |= DT_SINGLELINE | DT_TOP;
        else if ((dwStyle & BS_ALIGNMASK) == BS_BOTTOM)
            uFormat |= DT_SINGLELINE | DT_BOTTOM;
        else uFormat |= DT_SINGLELINE | DT_VCENTER;
    }

    uFormat |= DT_CALCRECT;
    
    DrawText (hdc, pCtrl->spCaption, -1, &tmp_rc/*prcBounds*/, uFormat);
    *prcBounds = tmp_rc;
}

static void draw_bitmap_button(PCONTROL pCtrl, HDC hdc, DWORD dwStyle, 
                          RECT *prcText)
{
    if (BUTTON_DATA(pCtrl)) {
        int x = prcText->left;
        int y = prcText->top;
        int w = RECTWP (prcText);
        int h = RECTHP (prcText);
        PBITMAP bmp = (PBITMAP)(BUTTON_DATA(pCtrl));
        RECT prcClient;

        GetClientRect ((HWND)pCtrl, &prcClient);
        if (dwStyle & BS_REALSIZEIMAGE) {
            x += (w - bmp->bmWidth) >> 1;
            y += (h - bmp->bmHeight) >> 1;
            w = h = 0;

            if (bmp->bmWidth > RECTW(prcClient)){
                x = prcClient.top + BTN_WIDTH_BORDER;
                y = prcClient.left + BTN_WIDTH_BORDER;
                w = RECTW(prcClient) - 2*BTN_WIDTH_BORDER-1;
            }

            if (bmp->bmHeight > RECTH(prcClient)){
                x = prcClient.top + BTN_WIDTH_BORDER;
                y = prcClient.left + BTN_WIDTH_BORDER;
                h = RECTH(prcClient) - 2*BTN_WIDTH_BORDER-1;
            }

        }
        else {
            x = prcClient.top + BTN_WIDTH_BORDER;
            y = prcClient.left + BTN_WIDTH_BORDER;
            w = RECTW (prcClient) - 2*BTN_WIDTH_BORDER-1;
            h = RECTH (prcClient) - 2*BTN_WIDTH_BORDER-1;
        }
        FillBoxWithBitmap (hdc, x, y, w, h, bmp);
    }
}

static void draw_icon_button(PCONTROL pCtrl, HDC hdc, DWORD dwStyle, RECT *prcText)
{
    if (BUTTON_DATA(pCtrl)) {
        int x = prcText->left;
        int y = prcText->top;
        int w = RECTWP (prcText);
        int h = RECTHP (prcText);
        HICON icon = (HICON)(BUTTON_DATA(pCtrl));
        RECT prcClient;

        if (dwStyle & BS_REALSIZEIMAGE) {
            int iconw, iconh;

            GetIconSize (icon, &iconw, &iconh);
            x += (w - iconw) >> 1;
            y += (h - iconh) >> 1;
            w = h = 0;
        }
        else {
            GetClientRect ((HWND)pCtrl, &prcClient);
            x = prcClient.top;
            y = prcClient.left;
            w = RECTW (prcClient);
            h = RECTH (prcClient);
        }

        DrawIcon (hdc, x, y, w, h, icon);
    }

}

static void btnPaintContent (PCONTROL pCtrl, HDC hdc, DWORD dwStyle, 
                             RECT* prcText)
{
    BOOL pushbutton = FALSE;
    RECT tmp_rc;

    tmp_rc.top = prcText->top - BUTTON_OFFSET;
    tmp_rc.left = prcText->left;
    tmp_rc.right = prcText->right;
    tmp_rc.bottom = prcText->bottom;

    switch (dwStyle & BS_CONTENTMASK)
    {
        
        case BS_BITMAP:
            draw_bitmap_button(pCtrl, hdc, dwStyle, prcText);
        break;
         
        case BS_ICON:
            draw_icon_button(pCtrl, hdc, dwStyle, prcText);
        break;

        case BS_TEXT:
        case BS_LEFTTEXT:
        {
            UINT uFormat;
            
            if (dwStyle & BS_MULTLINE)
                uFormat = DT_WORDBREAK;
            else
                uFormat = DT_SINGLELINE;

            if ((dwStyle & BS_TYPEMASK) == BS_PUSHBUTTON
                    || (dwStyle & BS_TYPEMASK) == BS_DEFPUSHBUTTON
                    || (dwStyle & BS_PUSHLIKE)) {
                pushbutton = TRUE;
                uFormat |= DT_CENTER | DT_VCENTER;
            }
            else {

                if ((dwStyle & BS_ALIGNMASK) == BS_RIGHT)
                    uFormat = DT_WORDBREAK | DT_RIGHT;
                else if ((dwStyle & BS_ALIGNMASK) == BS_CENTER)
                    uFormat = DT_WORDBREAK | DT_CENTER;
                else uFormat = DT_WORDBREAK | DT_LEFT;
            
                if ((dwStyle & BS_ALIGNMASK) == BS_TOP)
                    uFormat |= DT_SINGLELINE | DT_TOP;
                else if ((dwStyle & BS_ALIGNMASK) == BS_BOTTOM)
                    uFormat |= DT_SINGLELINE | DT_BOTTOM;
                else uFormat |= DT_SINGLELINE | DT_VCENTER;
            }
            
            SetBkColor (hdc, GetWindowBkColor ((HWND)pCtrl));
            if (dwStyle & WS_DISABLED) {
                /* RECT rc = *prcText; */
                
                SetBkMode (hdc, BM_TRANSPARENT);
                SetTextColor (hdc, GetWindowElementColorEx ((HWND)pCtrl, FGC_CONTROL_DISABLED));
                DrawText (hdc, pCtrl->spCaption, -1, &tmp_rc/*&rc*/, uFormat);
            }
            else {
                SetBkMode (hdc, BM_TRANSPARENT);
                if (pushbutton && (BUTTON_STATUS(pCtrl) & BST_PUSHED
                            || BUTTON_STATUS(pCtrl) & BST_CHECKED)) {
#ifdef _FLAT_WINDOW_STYLE 
                    if (dwStyle & BS_PUSHLIKE){    
                        SetTextColor (hdc, PIXEL_lightwhite);
                    }
                    else{
                        SetTextColor (hdc, GetWindowElementColorEx ((HWND)pCtrl, WEC_3DBOX_DARK));
                    }
#else
                    SetTextColor (hdc, GetWindowElementColorEx ((HWND)pCtrl, FGC_BUTTON_PUSHED));
#endif 
                }
                else {
                    SetTextColor (hdc, GetWindowElementColorEx ((HWND)pCtrl, FGC_BUTTON_NORMAL));
                }
                /*OffsetRect (prcText, 1, 1);*/

                DrawText (hdc, pCtrl->spCaption, -1, &tmp_rc/*prcText*/, uFormat);
            }
        }
        break;
    }
}

static void btnPaintNormalButton (PCONTROL pCtrl, HDC hdc, DWORD dwStyle,
                RECT* prcClient, RECT* prcText, RECT* prcBitmap)
{
    int bmp_left = prcBitmap->left;
    int bmp_top = prcBitmap->top + (prcBitmap->bottom - BTN_HEIGHT_BMP) / 2;

    switch (dwStyle & BS_TYPEMASK)
    {
        case BS_DEFPUSHBUTTON:
#ifndef _FLAT_WINDOW_STYLE
            myDrawButton (hdc, (HWND)pCtrl, prcClient->left, prcClient->top,
                                prcClient->right, prcClient->bottom, 
                                DF_3DBOX_NORMAL | DF_3DBOX_FILL, 
                                GetWindowBkColor ((HWND)pCtrl));
#else
            SetPenColor (hdc, GetWindowElementColorEx ((HWND)pCtrl, FGC_CONTROL_NORMAL));
            Rectangle (hdc, prcClient->left,
                            prcClient->top,
                            prcClient->right,
                            prcClient->bottom);
            myDrawButton (hdc, (HWND)pCtrl, prcClient->left + 1, prcClient->top + 1,
                                prcClient->right - 1, prcClient->bottom - 1, 
                                DF_3DBOX_NORMAL | DF_3DBOX_FILL, 
                                GetWindowBkColor ((HWND)pCtrl));
#endif
        break;

        case BS_PUSHBUTTON:
            myDrawButton (hdc, (HWND)pCtrl, prcClient->left, prcClient->top,
                                prcClient->right, prcClient->bottom, 
                                DF_3DBOX_NORMAL | DF_3DBOX_FILL,
                                GetWindowBkColor ((HWND)pCtrl));
        break;

        case BS_AUTORADIOBUTTON:
        case BS_RADIOBUTTON:
            if (dwStyle & BS_PUSHLIKE) {
                myDrawButton (hdc, (HWND)pCtrl, prcClient->left, prcClient->top,
                                prcClient->right, prcClient->bottom, 
                                DF_3DBOX_NORMAL | DF_3DBOX_FILL, 
                                GetWindowBkColor ((HWND)pCtrl));
                break;
            }

            if (dwStyle & WS_DISABLED) {
                FillBoxWithBitmapPart (hdc, bmp_left, bmp_top,
                                       BTN_WIDTH_BMP, BTN_HEIGHT_BMP,
                                       0, 0,
                                       BUTTON_BMP,
                                       (BTN_WIDTH_BMP * 2), BTN_HEIGHT_BMP);
            }
            else {
                FillBoxWithBitmapPart (hdc, bmp_left, bmp_top,
                                       BTN_WIDTH_BMP, BTN_HEIGHT_BMP,
                                       0, 0,
                                       BUTTON_BMP,
                                       0, BTN_HEIGHT_BMP);
            }
        break;

        case BS_3STATE:
        case BS_AUTO3STATE:
        case BS_AUTOCHECKBOX:
        case BS_CHECKBOX:
            if (dwStyle & BS_PUSHLIKE) {
                myDrawButton (hdc, (HWND)pCtrl, prcClient->left, prcClient->top,
                                prcClient->right, prcClient->bottom, 
                                DF_3DBOX_NORMAL | DF_3DBOX_FILL, 
                                GetWindowBkColor ((HWND)pCtrl));
                break;
            }

            if (dwStyle & WS_DISABLED) {
                FillBoxWithBitmapPart (hdc, bmp_left, bmp_top,
                                       BTN_WIDTH_BMP, BTN_HEIGHT_BMP,
                                       0, 0,
                                       BUTTON_BMP,
                                       BTN_WIDTH_BMP * 2, 0);
            }
            else {
                FillBoxWithBitmapPart (hdc, bmp_left, bmp_top,
                                       BTN_WIDTH_BMP, BTN_HEIGHT_BMP,
                                       0, 0,
                                       BUTTON_BMP,
                                       0, 0);
            }
        break;
/*
        case BS_OWNERDRAW:
        break;
*/
        default:
        break;
    }
}

#ifdef _FLAT_WINDOW_STYLE
static void draw_pressed_pushlike_button (HDC hdc, HWND hwnd, int x0, int y0, int x1, int y1, 
                                          int corner, DWORD flags, gal_pixel fillc);//corner = 3
#endif

static void btnPaintCheckedButton (PCONTROL pCtrl, HDC hdc, DWORD dwStyle,
                RECT* prcClient, RECT* prcText, RECT* prcBitmap)
{
    int bmp_left = prcBitmap->left;
    int bmp_top = prcBitmap->top + (prcBitmap->bottom - BTN_HEIGHT_BMP) / 2;

    switch (dwStyle & BS_TYPEMASK)
    {
        case BS_AUTORADIOBUTTON:
        case BS_RADIOBUTTON:
            if (dwStyle & BS_PUSHLIKE) {
#ifdef _FLAT_WINDOW_STYLE
                draw_pressed_pushlike_button (hdc, (HWND)pCtrl, prcClient->left, prcClient->top,
                                prcClient->right, prcClient->bottom, 3, 
                                DF_3DBOX_PRESSED | DF_3DBOX_FILL, 
                                GetWindowElementColorEx ((HWND)pCtrl, WEC_3DBOX_DARK));

#else
                myDrawButton (hdc, (HWND)pCtrl, prcClient->left, prcClient->top,
                                prcClient->right, prcClient->bottom, 
                                DF_3DBOX_PRESSED | DF_3DBOX_FILL, 
                                GetWindowElementColorEx ((HWND)pCtrl, WEC_3DBOX_DARK));
#endif
                break;
            }

            if (dwStyle & WS_DISABLED) {
                FillBoxWithBitmapPart (hdc, bmp_left, bmp_top, 
                                       BTN_WIDTH_BMP, BTN_HEIGHT_BMP,
                                       0, 0,
                                       BUTTON_BMP,
                                       (BTN_WIDTH_BMP << 2), BTN_HEIGHT_BMP);
            }
            else {
                FillBoxWithBitmapPart (hdc, bmp_left, bmp_top, 
                                       BTN_WIDTH_BMP, BTN_HEIGHT_BMP,
                                       0, 0,
                                       BUTTON_BMP,
                                       BTN_WIDTH_BMP, BTN_HEIGHT_BMP);
            }
        break;

        case BS_3STATE:
        case BS_AUTO3STATE:
        case BS_AUTOCHECKBOX:
        case BS_CHECKBOX:
            if (dwStyle & BS_PUSHLIKE) {
#ifdef _FLAT_WINDOW_STYLE
                draw_pressed_pushlike_button (hdc, (HWND)pCtrl, prcClient->left, prcClient->top,
                                prcClient->right, prcClient->bottom, 3, 
                                DF_3DBOX_PRESSED | DF_3DBOX_FILL, 
                                GetWindowElementColorEx ((HWND)pCtrl, WEC_3DBOX_DARK));

#else
                myDrawButton (hdc, (HWND)pCtrl, prcClient->left, prcClient->top,
                                prcClient->right, prcClient->bottom, 
                                DF_3DBOX_PRESSED | DF_3DBOX_FILL, 
                                GetWindowElementColorEx ((HWND)pCtrl, WEC_3DBOX_DARK));
#endif 
                break;
            }

            if (dwStyle & WS_DISABLED) {
                FillBoxWithBitmapPart (hdc, bmp_left, bmp_top, 
                                       BTN_WIDTH_BMP, BTN_HEIGHT_BMP,
                                       0, 0,
                                       BUTTON_BMP,
                                       BTN_WIDTH_BMP << 2, 0);
            }
            else {
                FillBoxWithBitmapPart (hdc, bmp_left, bmp_top, 
                                       BTN_WIDTH_BMP, BTN_HEIGHT_BMP,
                                       0, 0,
                                       BUTTON_BMP,
                                       BTN_WIDTH_BMP, 0);
            }
        break;
/*
        case BS_DEFPUSHBUTTON:
        case BS_PUSHBUTTON:
        case BS_OWNERDRAW:
        break;
*/
        default:
        break;
   }
}

static void btnPaintInterminateButton (PCONTROL pCtrl, HDC hdc, DWORD dwStyle,
                RECT* prcClient, RECT* prcText, RECT* prcBitmap)
{
    int bmp_left = prcBitmap->left;
    int bmp_top = prcBitmap->top + (prcBitmap->bottom - BTN_HEIGHT_BMP) / 2;

    switch (dwStyle & BS_TYPEMASK)
    {
        case BS_3STATE:
        case BS_AUTO3STATE:
            if (dwStyle & BS_PUSHLIKE) {
                break;
            }

            FillBoxWithBitmapPart (hdc, bmp_left, bmp_top,
                                   BTN_WIDTH_BMP, BTN_HEIGHT_BMP,
                                   0, 0,
                                   BUTTON_BMP,
                                   BTN_WIDTH_BMP << 1, 0);
        break;

/*
        case BS_AUTORADIOBUTTON:
        case BS_RADIOBUTTON:
        case BS_AUTOCHECKBOX:
        case BS_CHECKBOX:
        case BS_DEFPUSHBUTTON:
        case BS_PUSHBUTTON:
        case BS_OWNERDRAW:
        break;
*/
        default:
        break;
    }
}

#ifdef _FLAT_WINDOW_STYLE
static void draw_pressed_pushlike_button (HDC hdc, HWND hwnd, int x0, int y0,
       int x1, int y1, int corner, DWORD flags, gal_pixel fillc)//corner = 3
{
    int left, top, right, bottom;
    left = MIN (x0, x1);
    top  = MIN (y0, y1);
    right = MAX (x0, x1);
    bottom = MAX (y0, y1);
    
    if (flags & DF_3DBOX_FILL) {
        SetBrushColor (hdc, fillc);
        FillBox (hdc, left + corner, top, 
                      right - left + 1 - 2*corner, bottom - top + 1); 
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

    MoveTo(hdc, left + 1, top + 2);
    LineTo(hdc, left + 1, bottom - 2);
    MoveTo(hdc, left + 2, top + 1);
    LineTo(hdc, left + 2, bottom - 1);

    MoveTo(hdc, right - 1, top + 2);
    LineTo(hdc, right - 1, bottom - 2);
    MoveTo(hdc, right - 2, top + 1);
    LineTo(hdc, right - 2, bottom - 1);
}
#endif

static void btnPaintPushedButton (PCONTROL pCtrl, HDC hdc, DWORD dwStyle,
                RECT* prcClient, RECT* prcText, RECT* prcBitmap)
{
    int bmp_left = prcBitmap->left;
    int bmp_top = prcBitmap->top + (prcBitmap->bottom - BTN_HEIGHT_BMP) / 2;

    switch (dwStyle & BS_TYPEMASK)
    {
        case BS_DEFPUSHBUTTON:
        case BS_PUSHBUTTON:
            /*add*/
#ifdef _FLAT_WINDOW_STYLE
            myDrawButton (hdc, (HWND)pCtrl, prcClient->left + 1, prcClient->top + 1,
                                prcClient->right, prcClient->bottom, 
                                DF_3DBOX_PRESSED | DF_3DBOX_FILL, 
                           GetWindowBkColor ((HWND)pCtrl));
#else
            myDrawButton (hdc, (HWND)pCtrl, prcClient->left, prcClient->top,
                                prcClient->right, prcClient->bottom, 
                                DF_3DBOX_PRESSED | DF_3DBOX_FILL, 
                                GetWindowBkColor ((HWND)pCtrl));
#endif 
            /*add end*/
            OffsetRect (prcText, 1, 1);
        break;

        case BS_AUTORADIOBUTTON:
        case BS_RADIOBUTTON:
            if (dwStyle & BS_PUSHLIKE) {
#ifdef _FLAT_WINDOW_STYLE
                draw_pressed_pushlike_button (hdc, (HWND)pCtrl, prcClient->left, prcClient->top,
                                prcClient->right, prcClient->bottom, 3, 
                                DF_3DBOX_PRESSED | DF_3DBOX_FILL, 
                                GetWindowElementColorEx ((HWND)pCtrl, WEC_3DBOX_DARK));
#else
                myDrawButton (hdc, (HWND)pCtrl, prcClient->left, prcClient->top,
                                prcClient->right, prcClient->bottom, 
                                DF_3DBOX_PRESSED | DF_3DBOX_FILL, 
                                GetWindowElementColorEx ((HWND)pCtrl, WEC_3DBOX_DARK));
#endif 
                OffsetRect (prcText, 1, 1);
                break;
            }

            if (BUTTON_STATUS(pCtrl) & BST_CHECKED) {
                FillBoxWithBitmapPart (hdc, bmp_left, bmp_top, 
                                       BTN_WIDTH_BMP, BTN_HEIGHT_BMP,
                                       0, 0,
                                       BUTTON_BMP,
                                       BTN_WIDTH_BMP * 3, BTN_HEIGHT_BMP);
            }
            else {
                FillBoxWithBitmapPart (hdc, bmp_left, bmp_top, 
                                       BTN_WIDTH_BMP, BTN_HEIGHT_BMP,
                                       0, 0,
                                       BUTTON_BMP,
                                       BTN_WIDTH_BMP << 1, BTN_HEIGHT_BMP);
            }
        break;

        case BS_3STATE:
        case BS_AUTO3STATE:
        case BS_AUTOCHECKBOX:
        case BS_CHECKBOX:
            if (dwStyle & BS_PUSHLIKE) {
#ifdef _FLAT_WINDOW_STYLE
                draw_pressed_pushlike_button (hdc, (HWND)pCtrl, prcClient->left, prcClient->top,
                                prcClient->right, prcClient->bottom, 3, 
                                DF_3DBOX_PRESSED | DF_3DBOX_FILL, 
                                GetWindowElementColorEx ((HWND)pCtrl, WEC_3DBOX_DARK));

#else
                myDrawButton (hdc, (HWND)pCtrl, prcClient->left, prcClient->top,
                                prcClient->right, prcClient->bottom, 
                                DF_3DBOX_PRESSED | DF_3DBOX_FILL, 
                                GetWindowElementColorEx ((HWND)pCtrl, WEC_3DBOX_DARK));
#endif
                OffsetRect (prcText, 1, 1); 
                break;
            }

            if (BUTTON_STATUS (pCtrl) & BST_CHECKED) {
                FillBoxWithBitmapPart (hdc, bmp_left, bmp_top, 
                                       BTN_WIDTH_BMP, BTN_HEIGHT_BMP,
                                       0, 0,
                                       BUTTON_BMP,
                                       BTN_WIDTH_BMP * 3, 0);
            }
            else {
                FillBoxWithBitmapPart (hdc, bmp_left, bmp_top, 
                                       BTN_WIDTH_BMP, BTN_HEIGHT_BMP,
                                       0, 0,
                                       BUTTON_BMP,
                                       BTN_WIDTH_BMP << 1, 0);
            }
        break;
/*
        case BS_OWNERDRAW:
        break;
*/
        default:
        break;
   }
}

static void btnPaintFocusButton (PCONTROL pCtrl, HDC hdc, DWORD dwStyle,
                RECT* prcClient, RECT* prcText, RECT* prcBitmap)
{
    RECT rcBounds;
    RECT tmp_rc;

    tmp_rc.top = prcText->top - BUTTON_OFFSET;
    tmp_rc.left = prcText->left;
    tmp_rc.right = prcText->right;
    tmp_rc.bottom = prcText->bottom;

    SetPenColor (hdc, PIXEL_darkgray);
    switch (dwStyle & BS_TYPEMASK)
    {
        case BS_DEFPUSHBUTTON:
        case BS_PUSHBUTTON:
#ifndef _PHONE_WINDOW_STYLE
            FocusRect (hdc, prcClient->left + BTN_WIDTH_BORDER, 
                            prcClient->top  + BTN_WIDTH_BORDER,
                            prcClient->right - BTN_WIDTH_BORDER,
                            prcClient->bottom - BTN_WIDTH_BORDER);
#else
            FocusRect (hdc, prcClient->left + BTN_WIDTH_BORDER, 
                            prcClient->top  + BTN_WIDTH_BORDER,
                            prcClient->right - BTN_WIDTH_BORDER - 1,
                            prcClient->bottom - BTN_WIDTH_BORDER - 1);
                    
#endif
            break;
        case BS_3STATE:
        case BS_AUTO3STATE:
        case BS_AUTOCHECKBOX:
        case BS_CHECKBOX:
        case BS_AUTORADIOBUTTON:
        case BS_RADIOBUTTON:
            if (dwStyle & BS_PUSHLIKE) {
                FocusRect (hdc, prcClient->left + BTN_WIDTH_BORDER, 
                                prcClient->top  + BTN_WIDTH_BORDER,
                                prcClient->right - BTN_WIDTH_BORDER - 1,
                                prcClient->bottom - BTN_WIDTH_BORDER - 1);
                break;
            }

            btnGetTextBoundsRect (pCtrl, hdc, dwStyle, prcText, &rcBounds);
            FocusRect (hdc, rcBounds.left - 1,
                            rcBounds.top - 1,
                            rcBounds.right + 1,
                            rcBounds.bottom + 1);
            
        break;
/*
        case BS_OWNERDRAW:
        break;
*/
        default:
        break;
   }
}

static int btnUncheckOthers (PCONTROL pCtrl)
{
    PCONTROL pGroupLeader = pCtrl;
    PCONTROL pOthers;
    DWORD    type = pCtrl->dwStyle & BS_TYPEMASK;

    while (pGroupLeader) {
        if (pGroupLeader->dwStyle & WS_GROUP)
            break;

        pGroupLeader = pGroupLeader->prev;
    }

    pOthers = pGroupLeader;
    while (pOthers) {
        
        if ((pOthers->dwStyle & BS_TYPEMASK) != type)
            break;
        
        //if ((pOthers != pCtrl) && (BUTTON_STATUS (pOthers) | BST_CHECKED)) {
        if ((pOthers != pCtrl) && (BUTTON_STATUS (pOthers))) {
            BUTTON_STATUS (pOthers) &= ~BST_CHECKED;
            InvalidateRect ((HWND)pOthers, NULL, TRUE);
        }

        pOthers = pOthers->next;
    }
    return pCtrl->id;
}

static int ButtonCtrlProc (HWND hWnd, int uMsg, WPARAM wParam, LPARAM lParam)
{
    HDC         hdc;
    PCONTROL    pCtrl;
    DWORD       dwStyle;
    RECT        rcClient;
    RECT        rcText;
    RECT        rcBitmap;
    PBUTTONDATA pData;

    pCtrl   = Control(hWnd);
    dwStyle = pCtrl->dwStyle;

    switch(uMsg)
    {
        case MSG_CREATE:
            pData = (BUTTONDATA*) calloc (1, sizeof(BUTTONDATA));
            if (pData == NULL) 
                return -1;

            pData->status = 0;
            pData->data = pCtrl->dwAddData;
            pCtrl->dwAddData2 = (DWORD) pData;

            switch (dwStyle & BS_TYPEMASK) {
                case BS_PUSHBUTTON:
                case BS_DEFPUSHBUTTON:
                    //SetWindowBkColor (hWnd, GetWindowElementColorEx (hWnd, BKC_BUTTON_DEF)); 
                    //SetWindowBkColor (hWnd, GetWindowBkColor ((HWND)pCtrl)); 
                    break;

                default:
                    if (dwStyle & BS_CHECKED)
                        pData->status |= BST_CHECKED;
                    break;
            }

        break;
       
        case MSG_DESTROY:
            free ((void*) pCtrl->dwAddData2);
        break;
        
#ifdef _PHONE_WINDOW_STYLE
		case MSG_SIZECHANGING:
		{
			const RECT* rcExpect = (const RECT*)wParam;
			RECT* rcResult = (RECT*)lParam;

            switch (pCtrl->dwStyle & BS_TYPEMASK)
            {
            case BS_CHECKBOX:
            case BS_RADIOBUTTON:
            case BS_AUTOCHECKBOX:
            case BS_AUTORADIOBUTTON:
            case BS_3STATE:
            case BS_AUTO3STATE:
                if (dwStyle & BS_PUSHLIKE) {
                    rcResult->left = rcExpect->left;
                    rcResult->top = rcExpect->top;
                    rcResult->right = rcExpect->right;
                    rcResult->bottom = rcExpect->bottom;
                    return 0;
                }
            case BS_PUSHBUTTON:
            case BS_DEFPUSHBUTTON:
                rcResult->left = rcExpect->left;
                rcResult->top = rcExpect->top;
                rcResult->right = rcExpect->right;
                rcResult->bottom = rcExpect->bottom; 
                return 0;
            }
            break;
		}

        case MSG_SIZECHANGED:
        {
            RECT* rcWin = (RECT*)wParam;
            RECT* rcClient = (RECT*)lParam;
            switch (pCtrl->dwStyle & BS_TYPEMASK)
            {
                case BS_PUSHBUTTON:
                case BS_DEFPUSHBUTTON:
                    *rcClient = *rcWin;
                    return 1;
            }
            break;
        }
#endif

        case BM_GETCHECK:
            switch (dwStyle & BS_TYPEMASK) {
                case BS_AUTOCHECKBOX:
                case BS_AUTORADIOBUTTON:
                case BS_CHECKBOX:
                case BS_RADIOBUTTON:
                    return (BUTTON_STATUS (pCtrl) & BST_CHECKED);
                
                case BS_3STATE:
                case BS_AUTO3STATE:
                    if (BUTTON_STATUS (pCtrl) & BST_INDETERMINATE)
                        return (BST_INDETERMINATE);
                    return (BUTTON_STATUS (pCtrl) & BST_CHECKED);
                
                default:
                    return 0;
            }
        break;
        
        case BM_GETSTATE:
            return (int)(BUTTON_STATUS (pCtrl));

        case BM_GETIMAGE:
        {
            int* image_type = (int*) wParam;

            if (dwStyle & BS_BITMAP) {
                if (image_type) {
                    *image_type = BM_IMAGE_BITMAP;
                }
                return (int)(BUTTON_DATA (pCtrl));
            }
            else if (dwStyle & BS_ICON) {
                if (image_type) {
                    *image_type = BM_IMAGE_ICON;
                }
                return (int)(BUTTON_DATA (pCtrl));
            }
        }
        return 0;
        
        case BM_SETIMAGE:
        {
            int oldImage = (int)BUTTON_DATA (pCtrl);

            if (wParam == BM_IMAGE_BITMAP) {
                pCtrl->dwStyle &= ~BS_ICON;
                pCtrl->dwStyle |= BS_BITMAP;
            }
            else if (wParam == BM_IMAGE_ICON){
                pCtrl->dwStyle &= ~BS_BITMAP;
                pCtrl->dwStyle |= BS_ICON;
            }

            if (lParam) {
                 BUTTON_DATA (pCtrl) = (DWORD)lParam;
                 InvalidateRect (hWnd, NULL, TRUE);
            }

            return oldImage;
        }
        break;

        case BM_CLICK:
        {
            DWORD dwState;
            
            switch (pCtrl->dwStyle & BS_TYPEMASK)
            {
                case BS_AUTORADIOBUTTON:
                    if (!(BUTTON_STATUS (pCtrl) & BST_CHECKED))
                        SendMessage (hWnd, BM_SETCHECK, BST_CHECKED, 0);
                break;
                    
                case BS_AUTOCHECKBOX:
                    if (BUTTON_STATUS (pCtrl) & BST_CHECKED)
                        dwState = BST_UNCHECKED;
                    else
                        dwState = BST_CHECKED;
                                
                    SendMessage (hWnd, BM_SETCHECK, (WPARAM)dwState, 0);
                break;
                    
                case BS_AUTO3STATE:
                    dwState = (BUTTON_STATUS (pCtrl) & 0x00000003L);
                    dwState = BST_INDETERMINATE - dwState;
                    SendMessage (hWnd, BM_SETCHECK, (WPARAM)dwState, 0);
                break;
    
                case BS_PUSHBUTTON:
                case BS_DEFPUSHBUTTON:
                break;
            }
                
            NotifyParent (hWnd, pCtrl->id, BN_CLICKED);
            InvalidateRect (hWnd, NULL, TRUE);
        }
        break;
        
        case MSG_SETTEXT:
        {
            int len = strlen((char*)lParam);
            FreeFixStr (pCtrl->spCaption);
            pCtrl->spCaption = FixStrAlloc (len);
            if (len > 0)
                strcpy (pCtrl->spCaption, (char*)lParam);
            InvalidateRect (hWnd, NULL, TRUE);
            break;
        }

        case BM_SETCHECK:
        {
            DWORD dwOldState = BUTTON_STATUS (pCtrl);

            switch (dwStyle & BS_TYPEMASK) {
                case BS_CHECKBOX:
                case BS_AUTOCHECKBOX:
                    if (wParam & BST_CHECKED)
                        BUTTON_STATUS(pCtrl) |= BST_CHECKED;
                    else
                        BUTTON_STATUS(pCtrl) &= ~BST_CHECKED;
                break;
            
                case BS_AUTORADIOBUTTON:
                case BS_RADIOBUTTON:
                    if ( ((BUTTON_STATUS(pCtrl) & BST_CHECKED) == 0)
                            && (wParam & BST_CHECKED) ) {
                        BUTTON_STATUS(pCtrl) |= BST_CHECKED;
                        
                        btnUncheckOthers (pCtrl);
                    }
                    else if ( (BUTTON_STATUS(pCtrl) & BST_CHECKED)
                            && (!(wParam & BST_CHECKED)) ) {
                        BUTTON_STATUS(pCtrl) &= ~BST_CHECKED;
                    }
                break;
            
                case BS_3STATE:
                case BS_AUTO3STATE:
                    switch (wParam) { 
                        case BST_INDETERMINATE:
                        case BST_CHECKED:
                        case BST_UNCHECKED:
                            BUTTON_STATUS(pCtrl) &= 0xFFFFFFFC;
                            BUTTON_STATUS(pCtrl) += (wParam & 0x00000003);
                    }
                break;
            }
                
            if (dwOldState != BUTTON_STATUS(pCtrl)) {
                InvalidateRect (hWnd, NULL, TRUE);
            }
            return dwOldState;
        }
        
        case BM_SETSTATE:
        {
            DWORD dwOldState = BUTTON_STATUS(pCtrl) & BST_PUSHED;
            
            if (wParam)
                BUTTON_STATUS(pCtrl) |= BST_PUSHED;
            else
                BUTTON_STATUS(pCtrl) &= ~BST_PUSHED;
            
            if (dwOldState != (BUTTON_STATUS(pCtrl) & BST_PUSHED))
                InvalidateRect (hWnd, NULL, TRUE);

            return dwOldState;
        }
                
        case BM_SETSTYLE:
            pCtrl->dwStyle &= 0xFFFF0000;
            pCtrl->dwStyle |= (DWORD)(wParam & 0x0000FFFF);
            if (LOWORD (lParam))
                InvalidateRect (hWnd, NULL, TRUE);
        break;
        
        case MSG_CHAR:
            if (HIBYTE (wParam) == 0 
                    && ((dwStyle & BS_TYPEMASK) == BS_CHECKBOX
                     || (dwStyle & BS_TYPEMASK) == BS_AUTOCHECKBOX)) {
                DWORD dwOldState = BUTTON_STATUS(pCtrl);
                
                if (LOBYTE(wParam) == '+' || LOBYTE(wParam) == '=')
                    BUTTON_STATUS(pCtrl) |= BST_CHECKED;
                else if (LOBYTE(wParam) == '-')
                    BUTTON_STATUS(pCtrl) &= ~BST_CHECKED;
                    
                if (dwOldState != BUTTON_STATUS(pCtrl))
                    InvalidateRect (hWnd, NULL, TRUE);
            }
        break;
        
        case MSG_ENABLE:
            if (wParam && (dwStyle & WS_DISABLED))
                pCtrl->dwStyle &= ~WS_DISABLED;
            else if (!wParam && !(dwStyle & WS_DISABLED))
                pCtrl->dwStyle |= WS_DISABLED;
            else
                return 0;
            InvalidateRect (hWnd, NULL, TRUE);
        return 0;

        case MSG_GETDLGCODE:
            switch (dwStyle & BS_TYPEMASK) {
                case BS_CHECKBOX:
                case BS_AUTOCHECKBOX:
                return DLGC_WANTCHARS | DLGC_BUTTON;
                
                case BS_RADIOBUTTON:
                case BS_AUTORADIOBUTTON:
                return DLGC_RADIOBUTTON | DLGC_BUTTON;
                
                case BS_DEFPUSHBUTTON:
                return DLGC_DEFPUSHBUTTON;
                
                case BS_PUSHBUTTON:
                return DLGC_PUSHBUTTON;
                
                case BS_3STATE:
                case BS_AUTO3STATE:
                return DLGC_3STATE;

                default:
                return 0;
            }
        break;

        case MSG_NCHITTEST:
        {
            int x, y;
            
            x = (int)wParam;
            y = (int)lParam;
        
            if (PtInRect ((PRECT) &(pCtrl->cl), x, y))
                return HT_CLIENT;
            else  
                return HT_OUT;
        }
        break;
    
        case MSG_KILLFOCUS:
            BUTTON_STATUS(pCtrl) &= (~BST_FOCUS);
            if (GetCapture() == hWnd) {
                ReleaseCapture ();
                BUTTON_STATUS(pCtrl) &= (~BST_PUSHED);
            }

            if (dwStyle & BS_NOTIFY)
                NotifyParent (hWnd, pCtrl->id, BN_KILLFOCUS);

            InvalidateRect (hWnd, NULL, TRUE);
        break;

        case MSG_SETFOCUS:
            BUTTON_STATUS(pCtrl) |= BST_FOCUS;

            if (dwStyle & BS_NOTIFY)
                NotifyParent (hWnd, pCtrl->id, BN_SETFOCUS);

            InvalidateRect (hWnd, NULL, TRUE);
        break;
        
        case MSG_KEYDOWN:
            if (wParam != SCANCODE_SPACE 
                    && wParam != SCANCODE_ENTER
                    && wParam != SCANCODE_KEYPADENTER)
                break;

            if (GetCapture () == hWnd)
                break;
            
            SetCapture (hWnd);

            BUTTON_STATUS(pCtrl) |= BST_PUSHED;
            BUTTON_STATUS(pCtrl) |= BST_FOCUS;

            if (dwStyle & BS_NOTIFY)
                NotifyParent (hWnd, pCtrl->id, BN_PUSHED);

            InvalidateRect (hWnd, NULL, TRUE);
        break;
        
        case MSG_KEYUP:
        {
            DWORD dwState;
            
            if (wParam != SCANCODE_SPACE 
                    && wParam != SCANCODE_ENTER
                    && wParam != SCANCODE_KEYPADENTER)
                break;
                
            if (GetCapture () != hWnd)
                break;

            BUTTON_STATUS(pCtrl) &= ~BST_PUSHED;
            ReleaseCapture ();
            
            InvalidateRect (hWnd, NULL, TRUE);

            switch (pCtrl->dwStyle & BS_TYPEMASK)
            {
                case BS_AUTORADIOBUTTON:
                    if (!(BUTTON_STATUS(pCtrl) & BST_CHECKED))
                        SendMessage (hWnd, BM_SETCHECK, BST_CHECKED, 0);
                break;
                    
                case BS_AUTOCHECKBOX:
                    if (BUTTON_STATUS(pCtrl) & BST_CHECKED)
                        dwState = BST_UNCHECKED;
                    else
                        dwState = BST_CHECKED;
                                
                    SendMessage (hWnd, BM_SETCHECK, (WPARAM)dwState, 0);
                break;
                    
                case BS_AUTO3STATE:
                    dwState = (BUTTON_STATUS(pCtrl) & 0x00000003L);
                    dwState = (dwState + 1) % 3;
                    SendMessage (hWnd, BM_SETCHECK, (WPARAM)dwState, 0);
                break;
    
                case BS_PUSHBUTTON:
                case BS_DEFPUSHBUTTON:
                break;
            }

            NotifyParent (hWnd, pCtrl->id, BN_CLICKED);
            if (dwStyle & BS_NOTIFY)
                NotifyParent (hWnd, pCtrl->id, BN_UNPUSHED);
        }
        break;
      
        case MSG_LBUTTONDBLCLK:
            if (dwStyle & BS_NOTIFY)
                NotifyParent (hWnd, pCtrl->id, BN_DBLCLK);
        break;
        
        case MSG_LBUTTONDOWN:
            if (GetCapture () == hWnd)
                break;
            SetCapture (hWnd);
                
            BUTTON_STATUS(pCtrl) |= BST_PUSHED;
            BUTTON_STATUS(pCtrl) |= BST_FOCUS;

            if (dwStyle & BS_NOTIFY)
                NotifyParent (hWnd, pCtrl->id, BN_PUSHED);

            InvalidateRect (hWnd, NULL, TRUE);
        break;
    
        case MSG_LBUTTONUP:
        {
            int x, y;
            DWORD dwState;

            if (GetCapture() != hWnd)
                break;

            ReleaseCapture ();

            x = LOSWORD(lParam);
            y = HISWORD(lParam);
            ScreenToClient (GetParent (hWnd), &x, &y);
            
            if (PtInRect ((PRECT) &(pCtrl->cl), x, y))
            {
                switch (pCtrl->dwStyle & BS_TYPEMASK)
                {
                    case BS_AUTORADIOBUTTON:
                        if (!(BUTTON_STATUS(pCtrl) & BST_CHECKED))
                            SendMessage (hWnd, BM_SETCHECK, BST_CHECKED, 0);
                    break;
                    
                    case BS_AUTOCHECKBOX:
                        if (BUTTON_STATUS(pCtrl) & BST_CHECKED)
                            dwState = BST_UNCHECKED;
                        else
                            dwState = BST_CHECKED;
                                
                        SendMessage (hWnd, BM_SETCHECK, (WPARAM)dwState, 0);
                    break;
                    
                    case BS_AUTO3STATE:
                        dwState = (BUTTON_STATUS(pCtrl) & 0x00000003L);
                        dwState = (dwState + 1) % 3;
                        SendMessage (hWnd, BM_SETCHECK, (WPARAM)dwState, 0);
                    break;
    
                    case BS_PUSHBUTTON:
                    case BS_DEFPUSHBUTTON:
                    break;
                }
                
                BUTTON_STATUS(pCtrl) &= ~BST_PUSHED;

                if (dwStyle & BS_NOTIFY)
                    NotifyParent (hWnd, pCtrl->id, BN_UNPUSHED);
                NotifyParent (hWnd, pCtrl->id, BN_CLICKED);

                InvalidateRect (hWnd, NULL, TRUE);
            }
            else if (BUTTON_STATUS(pCtrl) & BST_PUSHED) {
                BUTTON_STATUS(pCtrl) &= ~BST_PUSHED;

                if (dwStyle & BS_NOTIFY)
                    NotifyParent (hWnd, pCtrl->id, BN_UNPUSHED);

                InvalidateRect (hWnd, NULL, TRUE);
            }
        }
        return 0;
                
        case MSG_MOUSEMOVE:
        {
            int x, y;

            if (GetCapture() != hWnd)
                break;

            x = LOSWORD(lParam);
            y = HISWORD(lParam);
            ScreenToClient (GetParent (hWnd), &x, &y);
            
            if (PtInRect ((PRECT) &(pCtrl->cl), x, y))
            {
                if ( !(BUTTON_STATUS(pCtrl) & BST_PUSHED) ) {
                    BUTTON_STATUS(pCtrl) |= BST_PUSHED;
                    
                    if (dwStyle & BS_NOTIFY)
                        NotifyParent (hWnd, pCtrl->id, BN_PUSHED);
                    InvalidateRect (hWnd, NULL, TRUE);
                }
            }
            else if (BUTTON_STATUS(pCtrl) & BST_PUSHED) {
                BUTTON_STATUS(pCtrl) &= ~BST_PUSHED;

                if (dwStyle & BS_NOTIFY)
                    NotifyParent (hWnd, pCtrl->id, BN_UNPUSHED);
                InvalidateRect (hWnd, NULL, TRUE);
            }
        }
        break;
    
        case MSG_PAINT:
        {
            hdc = BeginPaint (hWnd);
            
            btnGetRects (hWnd, dwStyle, &rcClient, &rcText, &rcBitmap);
            
            if (BUTTON_STATUS(pCtrl) & BST_PUSHED)
                btnPaintPushedButton (pCtrl, hdc, dwStyle,
                    &rcClient, &rcText, &rcBitmap);
            else if (BUTTON_STATUS(pCtrl) & BST_CHECKED)
                btnPaintCheckedButton (pCtrl, hdc, dwStyle, 
                    &rcClient, &rcText, &rcBitmap);
            else if (BUTTON_STATUS(pCtrl) & BST_INDETERMINATE)
                btnPaintInterminateButton (pCtrl, hdc, dwStyle,
                    &rcClient, &rcText, &rcBitmap);
            else
                btnPaintNormalButton (pCtrl, hdc, dwStyle,
                    &rcClient, &rcText, &rcBitmap);
            
            btnPaintContent (pCtrl, hdc, dwStyle, &rcText);

            if (BUTTON_STATUS(pCtrl) & BST_FOCUS)
                btnPaintFocusButton (pCtrl, hdc, dwStyle,
                    &rcClient, &rcText, &rcBitmap);
                
            EndPaint (hWnd, hdc);
        }
        return 0;

        default:
        break;
    }
    
    return DefaultControlProc (hWnd, uMsg, wParam, lParam);
}

#endif /* _CTRL_BUTTON */

