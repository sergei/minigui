/*
** $Id: colordlg.c 7371 2007-08-16 05:30:47Z xgwang $
** 
** colordlg.c: Color Selection Dialog Box.
** 
** Copyright (C) 2004 ~ 2007 Feynman Software.
**
** Original author: Linxs (linxs@minigui.net)
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "common.h"

#ifdef _USE_NEWGAL

#include "minigui.h"
#include "gdi.h"
#include "window.h"
#include "control.h"
#include "colordlg.h"
#include "colorspace.h"

#define LIMIT(a, b, c ) do {a=((a<b)?b:a);a=(a>c)?c:a;} while (0)

//#define IDC_STATIC  1000
#define IDC_SPACE   1001
#define IDC_YSPACE  1002
#define IDC_COLOR   1003
#define IDC_VALUE_Y 1005
#define IDC_VALUE_U 1006
#define IDC_VALUE_V 1007
#define IDC_VALUE_R 1008
#define IDC_VALUE_G 1009
#define IDC_VALUE_B 1010
#define IDC_OK      1011
#define IDC_CANCEL  1012    

#define SP_X        10
#define SP_Y        10
#define SP_W        240
#define SP_H        240
#define YSP_X       255
#define YSP_Y       10
#define YSP_W       30
#define YSP_H       240
#define PX          120
#define PY          120
#define PZ          120
#define CLRH        180
#define CLRS        128
#define CLRV        128

typedef struct p_struct {
    Uint16 sp_x;
    Uint16 sp_y;
    Uint16 sp_w;
    Uint16 sp_h;
    Uint16 ysp_x;
    Uint16 ysp_y;
    Uint16 ysp_w;
    Uint16 ysp_h;
    Uint16 selsp_x;
    Uint16 selsp_y;
    Uint16 selsp_w;
    Uint16 selsp_h;
} PALLETTE;

typedef struct yuv_struct {
    Uint16 clrh;
    Uint8 clrs;
    Uint8 clrv;
} YUV;

typedef struct scale_struct {
    float scale_w;
    float scale_h;
    float rc_scale_h;
} SCALE;

typedef struct indication {
    int px;
    int py;
    int pz;
} INDICAT;

typedef struct scd_struct {
    HDC SpaceDC;
    PCOLORDATA pclr;

    INDICAT indicat;

    RECT rcSpace;
    RECT rcYSpace;

    PALLETTE pal;
    YUV yuv;     
    SCALE scale;  
} SCOLORDIA, *PSCOLORDIA;

#define px          indicat.px
#define py          indicat.py
#define pz          indicat.pz

#define sp_x        pal.sp_x
#define sp_y        pal.sp_y
#define sp_w        pal.sp_w
#define sp_h        pal.sp_h
#define ysp_x       pal.ysp_x
#define ysp_y       pal.ysp_y
#define ysp_w       pal.ysp_w
#define ysp_h       pal.ysp_h
#define selsp_x     pal.selsp_x
#define selsp_y     pal.selsp_y
#define selsp_w     pal.selsp_w
#define selsp_h     pal.selsp_h

#define clrh        yuv.clrh
#define clrs        yuv.clrs
#define clrv        yuv.clrv

#define SCALE_W     scale.scale_w
#define SCALE_H     scale.scale_h
#define RC_SCALE_H  scale.rc_scale_h

#define H           pclr->h
#define S           pclr->s
#define V           pclr->v
#define R           pclr->r
#define G           pclr->g
#define B           pclr->b
#define PIXEL       pclr->pixel

static int DrawIndication (HDC hdc, PSCOLORDIA scld)
{
    scld->px = scld->clrh * scld->sp_w/360;
    scld->py = scld->sp_h - scld->clrs * scld->sp_h/255;
    SetBrushColor (hdc, RGB2Pixel (hdc, 0, 0, 0));
    Circle(hdc, scld->px, scld->py, 5);
    return 0;
}

static int DrawColorSpace (HDC hdc, int x, int y, int w, int h, PSCOLORDIA scld)
{
    HDC mdc;

    mdc = CreateCompatibleDCEx (hdc, scld->sp_w, scld->sp_h);
    BitBlt (scld->SpaceDC, 0,0, scld->sp_w, scld->sp_h, mdc, 0 , 0, 0);
    DrawIndication (mdc, scld);
    BitBlt (mdc, 0, 0, scld->sp_w, scld->sp_h, hdc, x, y, 0);
    DeleteMemDC (mdc);
    return 0;
}

static int DrawYSpace ( HDC hdc, int x, int y, int w, int h, PSCOLORDIA scld)
{
    int i;
    Uint8 r, g, b;
    HDC mdc;
    
    mdc = CreateCompatibleDCEx (hdc, scld->ysp_w, 256);
    for (i = 0; i < 256; i ++) {
        HSV2RGB (scld->clrh, scld->clrs, i, &r, &g, &b );
        SetPenColor (mdc, RGB2Pixel(mdc, r, g, b));
        MoveTo (mdc, 0, i);
        LineTo (mdc, 20 * scld->SCALE_W, i);
    }
    SetBrushColor (mdc, PIXEL_lightgray);
    FillBox (mdc, 21 * scld->SCALE_W, 0, scld->ysp_w * scld->SCALE_W, 256);
    scld->pz = scld->clrv ;
    SetPenColor (mdc, RGB2Pixel(mdc, 0, 0, 0));
    MoveTo ( mdc, 21 * scld->SCALE_W, scld->pz);
    LineTo ( mdc, 28 * scld->SCALE_W, scld->pz-3);
    MoveTo ( mdc, 21 * scld->SCALE_W, scld->pz);
    LineTo ( mdc, 28 * scld->SCALE_W, scld->pz+3);
    StretchBlt (mdc, 0, 0, scld->ysp_w, 256, hdc, x, y, w, h, 0);
    DeleteMemDC (mdc);
    return 0;
}

static int DrawSelSpace (HDC hdc, int x, int y, int w, int h, PSCOLORDIA scld)
{
    Uint8 r, g, b;
    HDC mdc;
    
    mdc = CreateCompatibleDCEx (hdc, w, h);
    HSV2RGB (scld->clrh, scld->clrs, scld->clrv, &r, &g, &b);
    SetBrushColor (mdc, RGB2Pixel(mdc, r, g, b));
    FillBox (mdc, 0, 0, w, h);
    BitBlt (mdc, 0, 0, w, h, hdc, x, y, 0);
    DeleteMemDC (mdc);
    return 0;
}

static int DrawAllSpace(HDC dc, PSCOLORDIA scld)
{
    DrawColorSpace (dc, scld->sp_x, scld->sp_y, scld->sp_w, scld->sp_h, scld);
    DrawYSpace (dc, scld->ysp_x, scld->ysp_y, scld->ysp_w, scld->ysp_h, scld);
    DrawSelSpace (dc, scld->selsp_x, scld->selsp_y, scld->selsp_w, scld->selsp_h, scld);
    return 0;
}

static void SetValue (HWND hDlg, PSCOLORDIA scld)
{
    char str[8];
    Uint8 r, g, b;

    sprintf (str, "%d", scld->clrh);
    SetDlgItemText (hDlg, IDC_VALUE_Y, str);
    sprintf (str, "%d", scld->clrs);
    SetDlgItemText (hDlg, IDC_VALUE_U, str);
    sprintf (str, "%d", scld->clrv);
    SetDlgItemText (hDlg, IDC_VALUE_V, str);

    HSV2RGB (scld->clrh, scld->clrs, scld->clrv, &r, &g, &b);
    sprintf (str, "%d", r);
    SetDlgItemText (hDlg, IDC_VALUE_R, str);
    sprintf (str, "%d", g);
    SetDlgItemText (hDlg, IDC_VALUE_G, str);
    sprintf (str, "%d", b);
    SetDlgItemText (hDlg, IDC_VALUE_B, str);
}

static void UpdateValue (HWND hDlg, int id, PSCOLORDIA scld)
{
    char str[8];
    HDC dc = GetClientDC (hDlg);
    Uint8 r, g, b;
    
    HSV2RGB (scld->clrh, scld->clrs, scld->clrv, &r, &g, &b);
    switch (id) {
    case IDC_VALUE_Y:  
        GetDlgItemText (hDlg, id, str, 8);
        scld->clrh = atoi (str);
        scld->clrh = MIN (359, scld->clrh);
        SetValue (hDlg, scld);
        break;
    case IDC_VALUE_U:  
        GetDlgItemText (hDlg, id, str, 8);
        scld->clrs = atoi (str);
        SetValue (hDlg, scld);
        break;
    case IDC_VALUE_V:  
        GetDlgItemText (hDlg, id, str, 8);
        scld->clrv = atoi (str);
        SetValue (hDlg, scld);
        break;
    case IDC_VALUE_R:  
        GetDlgItemText (hDlg, id, str, 8);
        r = atoi (str);
        RGB2HSV (r, g, b, &scld->clrh, &scld->clrs, &scld->clrv);
        SetValue (hDlg, scld);
        break;
    case IDC_VALUE_G:  
        GetDlgItemText (hDlg, id, str, 8);
        g = atoi (str);
        RGB2HSV (r, g, b, &scld->clrh, &scld->clrs, &scld->clrv);
        SetValue (hDlg, scld);
        break;
    case IDC_VALUE_B:  
        GetDlgItemText (hDlg, id, str, 8);
        b = atoi (str);
        RGB2HSV (r, g, b, &scld->clrh, &scld->clrs, &scld->clrv);
        SetValue (hDlg, scld);
        break;
    }
    DrawAllSpace (dc, scld);
    ReleaseDC (dc);
}

static int ColorDlgProc (HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
    PSCOLORDIA scld;

    switch (message) {
    case MSG_INITDIALOG:
        {
            int i, j;
            Uint8 r, g, b;
            HDC hdc, mdc;
            
            scld = (PSCOLORDIA)lParam;
            SetWindowAdditionalData (hDlg, (LPARAM)scld);

            hdc = GetClientDC (hDlg);
            scld->SpaceDC = CreateCompatibleDCEx(hdc, scld->sp_w, scld->sp_h);
            mdc = CreateCompatibleDCEx (hdc, 360, 256);
            for (i =  0; i < 360; i ++) {
                for (j = 0; j < 256; j ++) {
                    HSV2RGB (i, j, 200, &r, &g, &b);
                    SetPixelRGB(mdc, i, 256-j, r, g, b);
                }
            }
            
            StretchBlt(mdc, 0, 0, 360, 256, scld->SpaceDC, 0, 0, scld->sp_w, scld->sp_h, 0);
            DeleteMemDC(mdc);
            ReleaseDC(hdc);
        }        
        break;

    case MSG_CLOSE:
        EndDialog (hDlg, SELCOLOR_CANCEL);
        break;

    case MSG_DESTROY:
        scld = (PSCOLORDIA)GetWindowAdditionalData (hDlg);
        DeleteMemDC (scld->SpaceDC);
        break;

    case MSG_PAINT:
    {
        HDC hdc;

        scld = (PSCOLORDIA)GetWindowAdditionalData (hDlg);
        hdc = BeginPaint (hDlg);
        DrawAllSpace (hdc, scld);
        EndPaint (hDlg, hdc);
        return 0;
    }

    case MSG_COMMAND:
    {
        int msg = HIWORD(wParam);
        int id = LOWORD(wParam);

        scld = (PSCOLORDIA)GetWindowAdditionalData (hDlg);

        if (msg == EN_CONTCHANGED) {
            UpdateValue (hDlg, id, scld);
        }

        switch(id) {
        case IDC_CANCEL:
            EndDialog (hDlg, SELCOLOR_CANCEL);
            break;
        case IDC_OK:
            scld->H = scld->clrh;
            scld->S = scld->clrs;
            scld->V = scld->clrv;
            HSV2RGB (scld->clrh, scld->clrs, scld->clrv, &scld->R, &scld->G, &scld->B);
            scld->PIXEL = RGB2Pixel (HDC_SCREEN, scld->R, scld->G, scld->B);
            EndDialog (hDlg, SELCOLOR_OK);
            break;
        }
        break;
    }

    case MSG_LBUTTONDOWN:
    {
        int x = LOSWORD (lParam);
        int y = HISWORD (lParam);

        scld = (PSCOLORDIA)GetWindowAdditionalData (hDlg);

        if (PtInRect (&scld->rcSpace, x, y)) {
            HDC dc = GetClientDC (hDlg);
            scld->clrh = (x-scld->rcSpace.left)*360/RECTW(scld->rcSpace);
            scld->clrs = 256-(y-scld->rcSpace.top)*256/RECTH(scld->rcSpace);
            DrawAllSpace (dc, scld);
            SetValue (hDlg, scld);
            ReleaseDC (dc);
        }

        if (PtInRect(&scld->rcYSpace, x,y)) {
            HDC dc = GetClientDC (hDlg);
            scld->clrv = (y-scld->rcYSpace.top)*256 / RECTH(scld->rcYSpace);
            DrawAllSpace (dc, scld);
            SetValue (hDlg, scld);
            ReleaseDC (dc);
        }
    }
    }
    return DefaultDialogProc (hDlg, message, wParam, lParam);
}

static void InitColorCtrl (PCTRLDATA ctrl, int x, int y, int w, int h, PSCOLORDIA scld)
{
    int value_w;
    float scale_w, scale_h, rc_scale_h;

    if(w < 180)
        w = 180;
    if(h < 180)
        h = 180;
        
    value_w = 25;
    scale_w = scld->SCALE_W = (float)w/300;
    scale_h = scld->SCALE_H = (float)h/350;
    rc_scale_h = scld->RC_SCALE_H = scale_h;

    if(h - scale_h*240 < 80) {
        rc_scale_h = scld->RC_SCALE_H = (h - 80)/(scale_h*240) * scale_h;
        scale_h = scld->SCALE_H = 0.8;  
        value_w = 22;
    } 
 
    ctrl[0].x = 10 * scale_w;
    ctrl[0].y = 10 * rc_scale_h;
    ctrl[0].w = 240 * scale_w;
    ctrl[0].h = 240 * rc_scale_h;

    ctrl[1].x = 255 * scale_w;
    ctrl[1].y = 10 * rc_scale_h;
    ctrl[1].w = 30 * scale_w;
    ctrl[1].h = 240 * rc_scale_h; 

    ctrl[2].x = 10 * scale_w;
    ctrl[2].y = ctrl[1].y + ctrl[1].h + 5 * scale_h;
    ctrl[2].w = 35 * scale_w;
    ctrl[2].h = 40 * scale_h;

    ctrl[3].x = 50 * scale_w;
    ctrl[3].y = ctrl[2].y;
    ctrl[3].w = 20 * scale_w;
    ctrl[3].h = 15 * scale_h;

    ctrl[4].x = 50 * scale_w;
    ctrl[4].y = ctrl[3].y + ctrl[3].h + 5 * scale_h;
    ctrl[4].w = 20 * scale_w;
    ctrl[4].h = 15 * scale_h;

    ctrl[5].x = 50 * scale_w;
    ctrl[5].y = ctrl[4].y + ctrl[4].h + 5 * scale_h;
    ctrl[5].w = 20 * scale_w;
    ctrl[5].h = 15 * scale_h;

    ctrl[6].x = 120 * scale_w;
    ctrl[6].y = ctrl[3].y;
    ctrl[6].w = 20 * scale_w;
    ctrl[6].h = 15 * scale_h; 

    ctrl[7].x = 120 * scale_w;
    ctrl[7].y = ctrl[4].y;
    ctrl[7].w = 20 * scale_w;
    ctrl[7].h = 15 * scale_h;

    ctrl[8].x = 120 * scale_w;
    ctrl[8].y = ctrl[5].y;
    ctrl[8].w = 20 * scale_w;
    ctrl[8].h = 15 * scale_h;

    ctrl[9].x = 80 * scale_w;
    ctrl[9].y = ctrl[3].y;
    ctrl[9].w = value_w;
    ctrl[9].h = 15 * scale_h;

    ctrl[10].x = 80 * scale_w;
    ctrl[10].y = ctrl[4].y;
    ctrl[10].w = value_w;
    ctrl[10].h = 15 * scale_h;

    ctrl[11].x = 80 * scale_w;
    ctrl[11].y = ctrl[5].y;
    ctrl[11].w = value_w;
    ctrl[11].h = 15 * scale_h; 

    ctrl[12].x = 150 * scale_w;
    ctrl[12].y = ctrl[3].y;
    ctrl[12].w = value_w;
    ctrl[12].h = 15 * scale_h;

    ctrl[13].x = 150 * scale_w;
    ctrl[13].y = ctrl[4].y;
    ctrl[13].w = value_w;
    ctrl[13].h = 15 * scale_h;

    ctrl[14].x = 150 * scale_w;
    ctrl[14].y = ctrl[5].y;
    ctrl[14].w = value_w;
    ctrl[14].h = 15 * scale_h;

    ctrl[15].x = 200 * scale_w;
    ctrl[15].y = ctrl[3].y;
    ctrl[15].w = 80 * scale_w;
    ctrl[15].h = 30 * scale_h;

    ctrl[16].x = 200 * scale_w;
    ctrl[16].y = ctrl[15].y + ctrl[15].h; 
    ctrl[16].w = 80 * scale_w;
    ctrl[16].h = 30 * scale_h; 

    scld->rcSpace.left = SP_X * scale_w;
    scld->rcSpace.top = SP_Y * rc_scale_h;
    scld->rcSpace.right = scld->rcSpace.left + SP_W * scale_w;
    scld->rcSpace.bottom = scld->rcSpace.top + SP_H * rc_scale_h;

    scld->rcYSpace.left = YSP_X * scale_w;
    scld->rcYSpace.top = YSP_Y * rc_scale_h;
    scld->rcYSpace.right = scld->rcYSpace.left + YSP_W * scale_w;
    scld->rcYSpace.bottom = scld->rcYSpace.top + YSP_H * rc_scale_h;

    scld->sp_x = SP_X * scale_w;
    scld->sp_y = SP_Y * rc_scale_h;
    scld->sp_w = SP_W * scale_w;
    scld->sp_h = SP_H * rc_scale_h;

    scld->ysp_x = YSP_X * scale_w;
    scld->ysp_y = YSP_Y * rc_scale_h;
    scld->ysp_w = YSP_W * scale_w;
    scld->ysp_h = YSP_H * rc_scale_h;

    scld->selsp_x = ctrl[2].x;
    scld->selsp_y = ctrl[2].y;
    scld->selsp_w = ctrl[2].w;
    scld->selsp_h = ctrl[2].h;

    scld->px = PX * scale_w;
    scld->py = PY * rc_scale_h;
    scld->pz = PZ * rc_scale_h;

    scld->clrh = CLRH;
    scld->clrs = CLRS;
    scld->clrv = CLRV;
} 

int ColorSelDialog (HWND hWnd, int x, int y, int w, int h, PCOLORDATA pClrData)
{
    SCOLORDIA scldata;

    CTRLDATA ColorCtrl [] = { 
        { "static", WS_CHILD | WS_VISIBLE | SS_GRAYFRAME, 
            10, 10, 240, 240, 
           IDC_SPACE, "", 0},
        { "static", WS_CHILD | WS_VISIBLE | SS_GRAYFRAME, 
            255, 10, 30, 240,
            IDC_YSPACE, "", 0},
        { "static", WS_CHILD | WS_VISIBLE | SS_GRAYFRAME,
           10, 255, 35, 40,
           IDC_COLOR, "", 0},
        
        { "static", WS_CHILD | WS_VISIBLE | SS_SIMPLE,
           35, 255, 20, 15,
           IDC_STATIC , "Y :", 0},
        { "static", WS_CHILD | WS_VISIBLE | SS_SIMPLE,
           35, 275, 20, 15,
           IDC_STATIC , "U :", 0},
        { "static", WS_CHILD | WS_VISIBLE | SS_SIMPLE,
           35, 295, 20, 15,
           IDC_STATIC , "V :", 0},
           
        { "static", WS_CHILD | WS_VISIBLE | SS_SIMPLE,
           105, 255, 20, 15,
           IDC_STATIC , "R :", 0},
        { "static", WS_CHILD | WS_VISIBLE | SS_SIMPLE,
           105, 275, 20, 15,
           IDC_STATIC , "G :", 0},
        { "static", WS_CHILD | WS_VISIBLE | SS_SIMPLE,
           105, 295, 20, 15,
           IDC_STATIC , "B :", 0},
           
        { "sledit", WS_CHILD | WS_VISIBLE,
           95, 255, 25, 15,
           IDC_VALUE_Y , "180", 0},
        { "sledit", WS_CHILD | WS_VISIBLE,
           95, 275, 25, 15,
           IDC_VALUE_U , "128", 0},
        { "sledit", WS_CHILD | WS_VISIBLE,
           95, 295, 25, 15,
           IDC_VALUE_V , "128", 0},
           
        { "sledit", WS_CHILD | WS_VISIBLE,
           165, 255, 25, 15,
           IDC_VALUE_R , "64", 0},
        { "sledit", WS_CHILD | WS_VISIBLE,
           165, 275, 25, 15,
           IDC_VALUE_G , "128", 0},
        { "sledit", WS_CHILD | WS_VISIBLE,
           165, 295, 25, 15,
           IDC_VALUE_B , "128", 0},

        { "button", WS_CHILD | WS_VISIBLE,
           200, 255, 80, 30,
           IDC_OK, NULL, 0, WS_EX_TRANSPARENT},
        { "button", WS_CHILD | WS_VISIBLE ,
           200, 290, 80, 30,
           IDC_CANCEL, NULL, 0 ,WS_EX_TRANSPARENT}
    };

    
    DLGTEMPLATE ColorDlg = {
#ifdef _FLAT_WINDOW_STYLE
        WS_BORDER | WS_CAPTION | WS_MINIMIZEBOX, WS_EX_NONE,
#else
        WS_BORDER | WS_CAPTION, WS_EX_NONE,
#endif
        0, 0, 0, 0, NULL, 0, 0, TABLESIZE(ColorCtrl), NULL };
        
    ColorDlg.x = x;
    ColorDlg.y = y;
    ColorDlg.w = w;
    ColorDlg.h = h;

    scldata.pclr = pClrData;
    InitColorCtrl (ColorCtrl, x, y, w, h, &scldata);

    ColorCtrl [15].caption = GetSysText(IDS_MGST_OK);
    ColorCtrl [16].caption = GetSysText(IDS_MGST_CANCEL);
    ColorDlg.caption = GetSysText(IDS_MGST_COLORSEL);
    ColorDlg.controls = ColorCtrl;

    return DialogBoxIndirectParam (&ColorDlg, hWnd, ColorDlgProc, 
                    (LPARAM)(&scldata));
}

#endif /* _USE_NEWGAL */

