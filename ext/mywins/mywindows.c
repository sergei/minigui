/*
** $Id: mywindows.c 8225 2007-11-26 05:37:49Z xwyan $
**
** mywindows.c: implementation file of libmywins library.
**
** Copyright (C) 2003 ~ 2007 Feynman Software.
** Copyright (C) 2000 ~ 2002 Wei Yongming.
**
** Current maintainer: WEI Yongming.
**
** Create date: 2000.xx.xx
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "common.h"
#include "minigui.h"
#include "gdi.h"
#include "window.h"
#include "control.h"

#include "mywindows.h"

void errorWindow(HWND hwnd, const char* str, const char* title)
{
    char* a = NULL;

    if (!strstr (str, "%s")) {
        a = malloc (strlen (str) + 5);
        strcpy(a, str);
        strcat(a, ": %s");
        str = a;
    }

    myMessageBox (hwnd, MB_OK | MB_ICONSTOP, 
                title?title:GetSysText(IDS_MGST_ERROR), str, strerror (errno));

    free (a);
}

int myMessageBox (HWND hwnd, DWORD dwStyle, const char* title, 
                const char* text, ...)
{
    char * buf = NULL;
    int rc;

    if (strchr (text, '%')) {
        va_list args;
        int size = 0;
        int i = 0;

        va_start(args, text);
        do {
            size += 1000;
            if (buf) free(buf);
            buf = malloc(size);
            i = vsnprintf(buf, size, text, args);
        } while (i == size);
        va_end(args);
    }

    rc = MessageBox (hwnd, buf ? buf : text, title, dwStyle);

    if (buf)
        free (buf);

    return rc;
}

static DLGTEMPLATE DlgButtons =
{
    WS_BORDER | WS_CAPTION, WS_EX_NOCLOSEBOX,
    (640 - 418)/2, (480 - 170)/2, 418, 170,
    NULL, 0, 0, 5, NULL, 0
};

#define IDC_BASE        100
#define IDC_ONE         101
#define IDC_TWO         102
#define IDC_THREE       103

static CTRLDATA CtrlButtons [] =
{ 
    { "static", WS_VISIBLE | SS_ICON,
        10, 10, 32, 32, IDC_STATIC, "LOGO", 0 },
    { "static", WS_VISIBLE | SS_LEFT,
        60, 10, 340, 90, IDC_STATIC, NULL, 0 },
    { "button", WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP | WS_GROUP,
        100, 110, 80, 24, IDC_ONE, NULL, 0 },
    { "button", WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        200, 110, 80, 24, IDC_TWO, NULL, 0 },
    { "button", WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        300, 110, 80, 24, IDC_THREE, NULL, 0 }
};

static int 
ButtonsBoxProc (HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case MSG_INITDIALOG:
        return 1;
    break;
        
    case MSG_COMMAND:
        if (wParam == IDC_ONE || wParam == IDC_TWO
                || wParam == IDC_THREE) {
            EndDialog (hDlg, wParam);
        }
        break;
    }
    
    return DefaultDialogProc (hDlg, message, wParam, lParam);
}

int myWinMessage (HWND hwnd, const char* title, const char* button1, 
                       const char* text, ...)
{
    char* buf = NULL;
    int rc;

    if (strchr (text, '%')) {
        va_list args;
        int size = 0;
        int i = 0;

        va_start(args, text);
        do {
            size += 1000;
            if (buf) free(buf);
            buf = malloc(size);
            i = vsnprintf(buf, size, text, args);
        } while (i == size);
        va_end(args);
    }

    DlgButtons.caption   = title;
    DlgButtons.controls  = CtrlButtons;
    DlgButtons.controlnr = 3;

    CtrlButtons [0].caption = GetSysText(IDS_MGST_LOGO);
    CtrlButtons [1].caption = buf ? buf : text;
    CtrlButtons [2].caption = button1;
    CtrlButtons [2].x       = 300;
    
    CtrlButtons [0].dwAddData = GetLargeSystemIcon (IDI_INFORMATION);
    rc = DialogBoxIndirectParam (&DlgButtons, hwnd,
            ButtonsBoxProc, 0L);

    if (buf)
        free (buf);

    return rc - IDC_BASE;
}

int myWinChoice (HWND hwnd, const char* title, 
                const char* button1, const char* button2,
                const char* text, ...)
{
    char * buf = NULL;
    int rc;

    if (strchr (text, '%')) {
        va_list args;
        int size = 0;
        int i = 0;

        va_start(args, text);
        do {
            size += 1000;
            if (buf) free(buf);
            buf = malloc(size);
            i = vsnprintf(buf, size, text, args);
        } while (i == size);
        va_end(args);
    }

    DlgButtons.caption   = title;
    DlgButtons.controls  = CtrlButtons;
    DlgButtons.controlnr = 4;

    CtrlButtons [1].caption = buf ? buf : text;
    CtrlButtons [2].caption = button1;
    CtrlButtons [3].caption = button2;
    CtrlButtons [2].x       = 200;
    CtrlButtons [3].x       = 300;
    
    CtrlButtons [0].dwAddData = GetLargeSystemIcon (IDI_INFORMATION);
    rc = DialogBoxIndirectParam (&DlgButtons, hwnd,
            ButtonsBoxProc, 0L);

    if (buf)
        free (buf);

    return rc - IDC_BASE;
}

int myWinTernary (HWND hwnd, const char* title, 
                const char* button1, const char* button2, const char* button3,
                const char* text, ...)
{
    char * buf = NULL;
    int rc;

    if (strchr (text, '%')) {
        va_list args;
        int size = 0;
        int i = 0;

        va_start(args, text);
        do {
            size += 1000;
            if (buf) free(buf);
            buf = malloc(size);
            i = vsnprintf(buf, size, text, args);
        } while (i == size);
        va_end(args);
    }

    DlgButtons.caption   = title;
    DlgButtons.controls  = CtrlButtons;
    DlgButtons.controlnr = 5;

    CtrlButtons [1].caption = buf ? buf : text;
    CtrlButtons [2].caption = button1;
    CtrlButtons [3].caption = button2;
    CtrlButtons [4].caption = button3;
    CtrlButtons [2].x       = 100;
    CtrlButtons [3].x       = 200;
    CtrlButtons [4].x       = 300;
    
    CtrlButtons [0].dwAddData = GetLargeSystemIcon (IDI_INFORMATION);
    rc = DialogBoxIndirectParam (&DlgButtons, hwnd,
            ButtonsBoxProc, 0L);

    if (buf)
        free (buf);

    return rc - IDC_BASE;
}

static int StatusWinProc (HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
        case MSG_PAINT:
        {
            HDC hdc;
            char* text;
            RECT rc;
            
            hdc = BeginPaint (hWnd);
 
            text = (char*) GetWindowAdditionalData (hWnd);
            if (text) {
                GetClientRect (hWnd, &rc);
                SetBkMode (hdc, BM_TRANSPARENT);
                TextOut (hdc, 5, (rc.bottom - GetSysCharHeight())>>1, text);
            }
 
            EndPaint (hWnd, hdc);
            return 0;
        }
    }
 
    return DefaultMainWinProc(hWnd, message, wParam, lParam);
}

HWND createStatusWin (HWND hParentWnd, int width, int height, 
            const char* title, const char* text, ...)
{
    HWND hwnd;
    char* buf = NULL;
    MAINWINCREATE CreateInfo;

    if (strchr (text, '%')) {
        va_list args;
        int size = 0;
        int i = 0;

        va_start(args, text);
        do {
            size += 1000;
            if (buf) free(buf);
            buf = malloc(size);
            i = vsnprintf(buf, size, text, args);
        } while (i == size);
        va_end(args);
    }
    
    width = ClientWidthToWindowWidth (WS_CAPTION | WS_BORDER, width);
    height= ClientHeightToWindowHeight (WS_CAPTION | WS_BORDER, height, FALSE);
    
    CreateInfo.dwStyle = WS_ABSSCRPOS | WS_CAPTION | WS_BORDER | WS_VISIBLE;
    CreateInfo.dwExStyle = WS_EX_NOCLOSEBOX;
    CreateInfo.spCaption = title;
    CreateInfo.hMenu = 0;
    CreateInfo.hCursor = GetSystemCursor (IDC_WAIT);
    CreateInfo.hIcon = 0;
    CreateInfo.MainWindowProc = StatusWinProc;
    CreateInfo.lx = (GetGDCapability (HDC_SCREEN, GDCAP_MAXX) - width) >> 1;
    CreateInfo.ty = (GetGDCapability (HDC_SCREEN, GDCAP_MAXY) - height) >> 1;
    CreateInfo.rx = CreateInfo.lx + width;
    CreateInfo.by = CreateInfo.ty + height;
    CreateInfo.iBkColor = GetWindowElementColor (BKC_DIALOG);
    CreateInfo.dwAddData = buf ? (DWORD) buf : (DWORD) text;
    CreateInfo.hHosting = hParentWnd;

    hwnd = CreateMainWindow (&CreateInfo);
    if (hwnd == HWND_INVALID)
        return hwnd;

    SetWindowAdditionalData2 (hwnd, buf ? (DWORD) buf : 0);
    UpdateWindow (hwnd, TRUE);

    return hwnd;
}

void destroyStatusWin (HWND hwnd)
{
    char* buf;

    DestroyMainWindow (hwnd);
    MainWindowThreadCleanup (hwnd);
    
    buf = (char*)GetWindowAdditionalData2 (hwnd);
    if (buf)
        free (buf);
}

#define _MARGIN     2
#define _ID_TIMER   100

static int ToolTipWinProc (HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
        case MSG_CREATE:
        {
            int timeout = (int)GetWindowAdditionalData (hWnd);
            if (timeout >= 10)
                SetTimer (hWnd, _ID_TIMER, timeout / 10);
            break;
        }

        case MSG_TIMER:
            KillTimer (hWnd, _ID_TIMER);
            ShowWindow (hWnd, SW_HIDE);
            break;

        case MSG_PAINT:
        {
            HDC hdc;
            const char* text;
            
            hdc = BeginPaint (hWnd);
 
            text = GetWindowCaption (hWnd);
            SetBkMode (hdc, BM_TRANSPARENT);
            SelectFont (hdc, GetSystemFont (SYSLOGFONT_WCHAR_DEF));
            TabbedTextOut (hdc, _MARGIN, _MARGIN, text);
 
            EndPaint (hWnd, hdc);
            return 0;
        }

        case MSG_DESTROY:
        {
            KillTimer (hWnd, _ID_TIMER);
            return 0;
        }

        case MSG_SETTEXT:
        {
            int timeout = (int)GetWindowAdditionalData (hWnd);
            if (timeout >= 10) {
                KillTimer (hWnd, _ID_TIMER);
                SetTimer (hWnd, _ID_TIMER, timeout / 10);
            }

            break;
        }
    }
 
    return DefaultMainWinProc(hWnd, message, wParam, lParam);
}


HWND createToolTipWin (HWND hParentWnd, int x, int y, int timeout_ms, 
                const char* text, ...)
{
    HWND hwnd;
    char* buf = NULL;
    MAINWINCREATE CreateInfo;
    SIZE text_size;
    int width, height;

    if (strchr (text, '%')) {
        va_list args;
        int size = 0;
        int i = 0;

        va_start(args, text);
        do {
            size += 1000;
            if (buf) free(buf);
            buf = malloc(size);
            i = vsnprintf(buf, size, text, args);
        } while (i == size);
        va_end(args);
    }

    SelectFont (HDC_SCREEN, GetSystemFont (SYSLOGFONT_WCHAR_DEF));
    GetTabbedTextExtent (HDC_SCREEN, buf ? buf : text, -1, &text_size);
    text_size.cx += _MARGIN << 1;
    text_size.cy = GetSysCharHeight () + (_MARGIN << 1);
    width = ClientWidthToWindowWidth (WS_THINFRAME, text_size.cx);
    height= ClientHeightToWindowHeight (WS_THINFRAME, text_size.cy, FALSE);

    if (x + width > g_rcScr.right)
        x = g_rcScr.right - width;
    if (y + height > g_rcScr.bottom)
        y = g_rcScr.bottom - height;

    CreateInfo.dwStyle = WS_ABSSCRPOS | WS_THINFRAME | WS_VISIBLE;
    CreateInfo.dwExStyle = WS_EX_TOPMOST | WS_EX_TOOLWINDOW;
    CreateInfo.spCaption = buf ? buf : text;
    CreateInfo.hMenu = 0;
    CreateInfo.hCursor = GetSystemCursor (IDC_ARROW);
    CreateInfo.hIcon = 0;
    CreateInfo.MainWindowProc = ToolTipWinProc;
    CreateInfo.lx = x;
    CreateInfo.ty = y;
    CreateInfo.rx = CreateInfo.lx + width;
    CreateInfo.by = CreateInfo.ty + height;
    CreateInfo.iBkColor = GetWindowElementColor (BKC_TIP);
    CreateInfo.dwAddData = (DWORD) timeout_ms;
    CreateInfo.hHosting = hParentWnd;

    hwnd = CreateMainWindow (&CreateInfo);

    if (buf)
        free (buf);

    return hwnd;
}

void resetToolTipWin (HWND hwnd, int x, int y, const char* text, ...)
{
    char* buf = NULL;
    SIZE text_size;
    int width, height;

    if (strchr (text, '%')) {
        va_list args;
        int size = 0;
        int i = 0;

        va_start(args, text);
        do {
            size += 1000;
            if (buf) free(buf);
            buf = malloc(size);
            i = vsnprintf(buf, size, text, args);
        } while (i == size);
        va_end(args);
    }

    SelectFont (HDC_SCREEN, GetSystemFont (SYSLOGFONT_WCHAR_DEF));
    GetTabbedTextExtent (HDC_SCREEN, buf ? buf : text, -1, &text_size);

    text_size.cx += _MARGIN << 1;
    text_size.cy = GetSysCharHeight () + (_MARGIN << 1);
    width = ClientWidthToWindowWidth (WS_THINFRAME, text_size.cx);
    height= ClientHeightToWindowHeight (WS_THINFRAME, text_size.cy, FALSE);

    SetWindowCaption (hwnd, buf ? buf : text);
    if (buf) free (buf);

    if (x + width > g_rcScr.right)
        x = g_rcScr.right - width;
    if (y + height > g_rcScr.bottom)
        y = g_rcScr.bottom - height;

    MoveWindow (hwnd, x, y, width, height, TRUE);
    ShowWindow (hwnd, SW_SHOWNORMAL);
}

void destroyToolTipWin (HWND hwnd)
{
    DestroyMainWindow (hwnd);
    MainWindowThreadCleanup (hwnd);
}

#ifdef _CTRL_PROGRESSBAR

HWND createProgressWin (HWND hParentWnd, const char* title, const char* label,
        int id, int range)
{
    HWND hwnd;
    MAINWINCREATE CreateInfo;
    int ww, wh;
    HWND hStatic, hProgBar;

    ww = ClientWidthToWindowWidth (WS_CAPTION | WS_BORDER, 400);
    wh = ClientHeightToWindowHeight (WS_CAPTION | WS_BORDER, 
            (range > 0) ? 70 : 35, FALSE);
    
    CreateInfo.dwStyle = WS_ABSSCRPOS | WS_CAPTION | WS_BORDER | WS_VISIBLE;
    CreateInfo.dwExStyle = WS_EX_NOCLOSEBOX;
    CreateInfo.spCaption = title;
    CreateInfo.hMenu = 0;
    CreateInfo.hCursor = GetSystemCursor(IDC_WAIT);
    CreateInfo.hIcon = 0;
    CreateInfo.MainWindowProc = DefaultMainWinProc;
    CreateInfo.lx = (GetGDCapability (HDC_SCREEN, GDCAP_MAXX) - ww) >> 1;
    CreateInfo.ty = (GetGDCapability (HDC_SCREEN, GDCAP_MAXY) - wh) >> 1;
    CreateInfo.rx = CreateInfo.lx + ww;
    CreateInfo.by = CreateInfo.ty + wh;
    CreateInfo.iBkColor = GetWindowElementColor (BKC_DIALOG);
    CreateInfo.dwAddData = 0L;
    CreateInfo.hHosting = hParentWnd;

    hwnd = CreateMainWindow (&CreateInfo);
    if (hwnd == HWND_INVALID)
        return hwnd;

    hStatic = CreateWindowEx ("static", 
                  label, 
                  WS_VISIBLE | SS_SIMPLE, 
                  WS_EX_USEPARENTCURSOR,
                  IDC_STATIC, 
                  10, 10, 380, 16, hwnd, 0);
    
    if (range > 0) {
        hProgBar = CreateWindowEx ("progressbar", 
                  NULL, 
                  WS_VISIBLE,
                  WS_EX_USEPARENTCURSOR,
                  id,
                  10, 30, 380, 30, hwnd, 0);
        SendDlgItemMessage (hwnd, id, PBM_SETRANGE, 0, range);
    }
    else
        hProgBar = HWND_INVALID;

    UpdateWindow (hwnd, TRUE);

    return hwnd;
}

void destroyProgressWin (HWND hwnd)
{
    DestroyAllControls (hwnd);
    DestroyMainWindow (hwnd);
    ThrowAwayMessages (hwnd);
    MainWindowThreadCleanup (hwnd);
}
#endif

#define INTERW_CTRLS        5
#define INTERH_CTRLS        5

#define LABEL_HEIGHT        16
#define LABEL_WIDTH         100
#define BUTTON_WIDTH        80
#define BUTTON_HEIGHT       24
#define ENTRY_HEIGHT        22

#define MARGIN_TOP          10
#define MARGIN_BOTTOM       10
#define MARGIN_LEFT         50
#define MARGIN_RIGHT        10

static int
WinMenuBoxProc (HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
  switch (message) {
    case MSG_INITDIALOG:
    {
        myWINMENUITEMS* pWinMenuItems = (myWINMENUITEMS*)lParam;
        int i = 0;
        char** items = pWinMenuItems->items;

        while (items[i]) {
            SendDlgItemMessage (hDlg, pWinMenuItems->listboxid, 
                    LB_ADDSTRING, 0, (LPARAM)(items[i]));
            i++;
        }
        
        SendDlgItemMessage (hDlg, pWinMenuItems->listboxid,
                LB_SETCURSEL, (WPARAM)(*(pWinMenuItems->selected)), 0L);

        SetWindowAdditionalData (hDlg, (DWORD)lParam);
        return 1;
    }
    break;

    case MSG_COMMAND:
    {
        myWINMENUITEMS* pWinMenuItems 
            = (myWINMENUITEMS*)GetWindowAdditionalData (hDlg);

        if ( (wParam >= pWinMenuItems->minbuttonid) 
                && (wParam <= pWinMenuItems->maxbuttonid) ) {
            *(pWinMenuItems->selected)
                = SendDlgItemMessage (hDlg, pWinMenuItems->listboxid,
                        LB_GETCURSEL, 0, 0L);
            EndDialog (hDlg, wParam);
        }
    }
        break;
  }

    return DefaultDialogProc (hDlg, message, wParam, lParam);
}

int myWinMenu (HWND hParentWnd, const char* title, const char* label, 
                int width, int listboxheight, char** items, int* listItem, 
                myWINBUTTON* buttons)
{
    DLGTEMPLATE DlgBoxData;
    CTRLDATA* pCtrlData;
    int buttonCount = 0, ctrlCount;
    int i, rc;
    int cw, ch, ww, wh;
    int maxid = 0, minbuttonid, maxbuttonid;
    int buttonx, buttony;
    myWINMENUITEMS WinMenuItems;
    RECT rcText;
    int listboxy;

    minbuttonid = buttons->id;
    maxbuttonid = buttons->id;
    while ((buttons + buttonCount)->text) {
        int id;

        id = (buttons + buttonCount)->id;
        maxid += id;
        if ( id > maxbuttonid)
            maxbuttonid = id;
        else if (id < minbuttonid)
            minbuttonid = id;

        buttonCount ++;
    }
    maxid ++;

    if (buttonCount == 0)
        return 0;

    ctrlCount = buttonCount + 3;
    if ( !(pCtrlData = malloc (sizeof (CTRLDATA) * ctrlCount)) )
        return 0;
    
    DlgBoxData.dwStyle      = WS_ABSSCRPOS | WS_CAPTION | WS_BORDER;
    DlgBoxData.dwExStyle    = WS_EX_NOCLOSEBOX;
    DlgBoxData.caption      = title;
    DlgBoxData.hIcon        = 0;
    DlgBoxData.hMenu        = 0;
    DlgBoxData.controlnr    = ctrlCount;
    DlgBoxData.controls     = pCtrlData;
    DlgBoxData.dwAddData    = 0L;

    cw = BUTTON_WIDTH * buttonCount + INTERW_CTRLS * (buttonCount - 1);
    cw = MAX (cw, width);
    cw += MARGIN_LEFT + MARGIN_RIGHT;

    rcText.left = 0;
    rcText.top  = 0;
    rcText.right = cw - MARGIN_LEFT - MARGIN_RIGHT;
    rcText.bottom = GetSysCharHeight ();
    DrawText (HDC_SCREEN, label, -1, &rcText, 
                DT_LEFT | DT_TOP | DT_WORDBREAK | DT_EXPANDTABS | DT_CALCRECT);

    ch = listboxheight + BUTTON_HEIGHT + INTERH_CTRLS 
            + MARGIN_TOP + MARGIN_BOTTOM + RECTH (rcText) + INTERH_CTRLS;

    ww = ClientWidthToWindowWidth (DlgBoxData.dwStyle, cw);
    wh = ClientHeightToWindowHeight (DlgBoxData.dwStyle, ch, FALSE);

    DlgBoxData.x = (GetGDCapability (HDC_SCREEN, GDCAP_HPIXEL) - ww)/2;
    DlgBoxData.y = (GetGDCapability (HDC_SCREEN, GDCAP_VPIXEL) - wh)/2;
    DlgBoxData.w = ww;
    DlgBoxData.h = wh;
   
    // LOGO:
    pCtrlData->class_name    = "static";
    pCtrlData->dwStyle  = WS_CHILD | WS_VISIBLE | SS_ICON;
    pCtrlData->dwExStyle= 0;
    pCtrlData->x        = 10;
    pCtrlData->y        = MARGIN_TOP;
    pCtrlData->w        = 32;
    pCtrlData->h        = 32;;
    pCtrlData->id       = IDC_STATIC;
    pCtrlData->caption  = GetSysText(IDS_MGST_LOGO);
    pCtrlData->dwAddData = GetLargeSystemIcon (IDI_INFORMATION);
    
    // label:
    (pCtrlData + 1)->class_name    = "static";
    (pCtrlData + 1)->dwStyle  = WS_CHILD | WS_VISIBLE | SS_LEFT;
    (pCtrlData + 1)->dwExStyle= 0;
    (pCtrlData + 1)->x        = MARGIN_LEFT;
    (pCtrlData + 1)->y        = MARGIN_TOP;
    (pCtrlData + 1)->w        = cw - MARGIN_LEFT - MARGIN_RIGHT;
    (pCtrlData + 1)->h        = RECTH (rcText);
    (pCtrlData + 1)->id       = IDC_STATIC;
    (pCtrlData + 1)->caption  = label;
    (pCtrlData + 1)->dwAddData = 0;
    
    // listbox:
    listboxy = MARGIN_TOP + RECTH (rcText) + INTERH_CTRLS;
    (pCtrlData + 2)->class_name      = "listbox";
    (pCtrlData + 2)->dwStyle    = WS_CHILD | WS_VISIBLE | WS_TABSTOP
                                    | WS_VSCROLL | WS_BORDER;
    (pCtrlData + 2)->dwExStyle  = 0;
    (pCtrlData + 2)->x          = MARGIN_LEFT;
    (pCtrlData + 2)->y          = listboxy;
    (pCtrlData + 2)->w          = cw - MARGIN_LEFT - MARGIN_RIGHT;
    (pCtrlData + 2)->h          = listboxheight;
    (pCtrlData + 2)->id         = maxid;
    (pCtrlData + 2)->caption    = NULL;
    (pCtrlData + 2)->dwAddData  = 0;

    buttony =  (pCtrlData + 2)->y + (pCtrlData + 2)->h + INTERH_CTRLS;
    buttonx =  ((cw - MARGIN_LEFT - MARGIN_RIGHT - 
        BUTTON_WIDTH * buttonCount - INTERW_CTRLS * (buttonCount - 1))>>1)
        + MARGIN_LEFT;

    // buttons
    for (i = 3; i < buttonCount + 3; i++) {
        (pCtrlData + i)->class_name      = "button";
        (pCtrlData + i)->dwStyle    = WS_CHILD | WS_VISIBLE | WS_TABSTOP;
        (pCtrlData + i)->dwStyle    |= (buttons + i - 3)->flags;
        (pCtrlData + i)->dwExStyle  = 0;
        (pCtrlData + i)->x          = buttonx;
        (pCtrlData + i)->y          = buttony;
        (pCtrlData + i)->w          = BUTTON_WIDTH;
        (pCtrlData + i)->h          = BUTTON_HEIGHT;
        (pCtrlData + i)->id         = (buttons + i - 3)->id;
        (pCtrlData + i)->caption    = (buttons + i - 3)->text;
        (pCtrlData + i)->dwAddData  = 0;

        buttonx += BUTTON_WIDTH + INTERW_CTRLS;
    }
    
    WinMenuItems.items       = items;
    WinMenuItems.listboxid   = maxid;
    WinMenuItems.selected    = listItem;
    WinMenuItems.minbuttonid = minbuttonid;
    WinMenuItems.maxbuttonid = maxbuttonid;
    rc = DialogBoxIndirectParam (&DlgBoxData, hParentWnd, WinMenuBoxProc, 
                                        (LPARAM)(&WinMenuItems));

    free (pCtrlData);
    return rc;
}

static int
WinEntryBoxProc (HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
  switch (message) {
    case MSG_INITDIALOG:
    {
        int i;
        myWINENTRYITEMS* pWinEntryItems 
            = (myWINENTRYITEMS*)lParam;

        for (i = 0; i < pWinEntryItems->entrycount; i++) {
            if ((pWinEntryItems->entries + i)->maxlen > 0)
                SendDlgItemMessage (hDlg, pWinEntryItems->firstentryid + i,
                        EM_LIMITTEXT,
                        (pWinEntryItems->entries + i)->maxlen,
                        0L);
        }

        SetWindowAdditionalData (hDlg, (DWORD)lParam);
        return 1;
    }
    break;

    case MSG_COMMAND:
    {
        int i;
        myWINENTRYITEMS* pWinEntryItems 
            = (myWINENTRYITEMS*)GetWindowAdditionalData (hDlg);

        if ( (wParam >= pWinEntryItems->minbuttonid) 
                && (wParam <= pWinEntryItems->maxbuttonid) ) {
            for (i = 0; i < pWinEntryItems->entrycount; i++) {
                char** value;
                value = (pWinEntryItems->entries + i)->value;
                *value = GetDlgItemText2 (hDlg, 
                        pWinEntryItems->firstentryid + i, NULL);
            }
            
            EndDialog (hDlg, wParam);
        }
    }
        break;
  }

    return DefaultDialogProc (hDlg, message, wParam, lParam);
}
int myWinEntries (HWND hParentWnd, const char* title, const char* label,
                int width, int editboxwidth, BOOL fIME, myWINENTRY* items, 
                myWINBUTTON* buttons)
{
    DLGTEMPLATE DlgBoxData;
    CTRLDATA* pCtrlData;
    int buttonCount = 0, entryCount = 0, ctrlCount, count;
    int i, rc;
    int cw, ch, ww, wh, labelwidth;
    int maxid = 0, minbuttonid, maxbuttonid;
    int entryx, entryy;
    int buttonx, buttony;
    myWINENTRYITEMS WinEntryItems;
    RECT rcText;

    minbuttonid = buttons->id;
    maxbuttonid = buttons->id;
    while ((buttons + buttonCount)->text) {
        int id;

        id = (buttons + buttonCount)->id;
        maxid += id;
        if ( id > maxbuttonid)
            maxbuttonid = id;
        else if (id < minbuttonid)
            minbuttonid = id;

        buttonCount ++;
    }
    maxid ++;

    while ((items + entryCount)->text) {
        entryCount ++;
    }

    if (buttonCount == 0 || entryCount == 0)
        return 0;

    ctrlCount = buttonCount + (entryCount<<1) + 2;

    if ( !(pCtrlData = malloc (sizeof (CTRLDATA) * ctrlCount)) )
        return 0;
    
    DlgBoxData.dwStyle      = WS_ABSSCRPOS | WS_CAPTION | WS_BORDER;
    DlgBoxData.dwExStyle    = fIME?(WS_EX_IMECOMPOSE | WS_EX_NOCLOSEBOX):WS_EX_NOCLOSEBOX;
    DlgBoxData.caption      = title;
    DlgBoxData.hIcon        = 0;
    DlgBoxData.hMenu        = 0;
    DlgBoxData.controlnr    = ctrlCount;
    DlgBoxData.controls     = pCtrlData;
    DlgBoxData.dwAddData    = 0L;

    cw = BUTTON_WIDTH * buttonCount + INTERW_CTRLS * (buttonCount - 1);
    if (width < editboxwidth + INTERW_CTRLS + LABEL_WIDTH)
        width = editboxwidth + INTERW_CTRLS + LABEL_WIDTH;
    cw = MAX (cw, width);
    labelwidth = cw - editboxwidth - INTERW_CTRLS;
    cw += MARGIN_LEFT + MARGIN_RIGHT;

    rcText.left = 0;
    rcText.top  = 0;
    rcText.right = cw - MARGIN_LEFT - MARGIN_RIGHT;
    rcText.bottom = GetSysCharHeight ();
    DrawText (HDC_SCREEN, label, -1, &rcText, 
                DT_LEFT | DT_TOP | DT_WORDBREAK | DT_EXPANDTABS | DT_CALCRECT);
    
    ch = MARGIN_TOP + RECTH (rcText) + INTERH_CTRLS
                    + (ENTRY_HEIGHT + INTERH_CTRLS) * entryCount 
                    + BUTTON_HEIGHT
                    + MARGIN_BOTTOM;

    ww = ClientWidthToWindowWidth (DlgBoxData.dwStyle, cw);
    wh = ClientHeightToWindowHeight (DlgBoxData.dwStyle, ch, FALSE);

    DlgBoxData.x = (GetGDCapability (HDC_SCREEN, GDCAP_HPIXEL) - ww)/2;
    DlgBoxData.y = (GetGDCapability (HDC_SCREEN, GDCAP_VPIXEL) - wh)/2;
    DlgBoxData.w = ww;
    DlgBoxData.h = wh;
    
    // LOGO:
    pCtrlData->class_name    = "static";
    pCtrlData->dwStyle  = WS_CHILD | WS_VISIBLE | SS_ICON;
    pCtrlData->dwExStyle= 0;
    pCtrlData->x        = 10;
    pCtrlData->y        = MARGIN_TOP;
    pCtrlData->w        = 32;
    pCtrlData->h        = 32;;
    pCtrlData->id       = IDC_STATIC;
    pCtrlData->caption  = GetSysText(IDS_MGST_LOGO);
    pCtrlData->dwAddData = GetLargeSystemIcon (IDI_INFORMATION);
    
    // label:
    (pCtrlData + 1)->class_name    = "static";
    (pCtrlData + 1)->dwStyle  = WS_CHILD | WS_VISIBLE | SS_LEFT;
    (pCtrlData + 1)->dwExStyle= 0;
    (pCtrlData + 1)->x        = MARGIN_LEFT;
    (pCtrlData + 1)->y        = MARGIN_TOP;
    (pCtrlData + 1)->w        = cw - MARGIN_LEFT - MARGIN_RIGHT;
    (pCtrlData + 1)->h        = RECTH (rcText);
    (pCtrlData + 1)->id       = IDC_STATIC;
    (pCtrlData + 1)->caption  = label;
    (pCtrlData + 1)->dwAddData = 0;
    
    // entries:
    entryy = MARGIN_TOP + RECTH (rcText) + INTERH_CTRLS;
    entryx = MARGIN_LEFT + labelwidth + INTERW_CTRLS;
    for (i = 0; i < entryCount; i++) {
        // label
        (pCtrlData + i*2 + 2)->class_name    = "static";
        (pCtrlData + i*2 + 2)->dwStyle  = WS_CHILD | WS_VISIBLE | SS_RIGHT;
        (pCtrlData + i*2 + 2)->dwExStyle= 0;
        (pCtrlData + i*2 + 2)->x        = MARGIN_LEFT;
        (pCtrlData + i*2 + 2)->y        = entryy + 3;
        (pCtrlData + i*2 + 2)->w        = labelwidth;
        (pCtrlData + i*2 + 2)->h        = LABEL_HEIGHT;
        (pCtrlData + i*2 + 2)->id       = IDC_STATIC;
        (pCtrlData + i*2 + 2)->caption  = (items + i)->text;
        (pCtrlData + i*2 + 2)->dwAddData = 0;
        
        // editbox
        (pCtrlData + i*2 + 3)->class_name    = "edit";
        (pCtrlData + i*2 + 3)->dwStyle  = WS_CHILD | WS_VISIBLE | WS_TABSTOP;
        (pCtrlData + i*2 + 3)->dwStyle |= WS_BORDER | (items + i)->flags;
        (pCtrlData + i*2 + 3)->dwExStyle= 0;
        (pCtrlData + i*2 + 3)->x        = entryx;
        (pCtrlData + i*2 + 3)->y        = entryy;
        (pCtrlData + i*2 + 3)->w        = editboxwidth;
        (pCtrlData + i*2 + 3)->h        = ENTRY_HEIGHT;
        (pCtrlData + i*2 + 3)->id       = maxid + i;
        (pCtrlData + i*2 + 3)->caption  = *(items + i)->value;
        (pCtrlData + i*2 + 3)->dwAddData = 0;

        entryy += ENTRY_HEIGHT + INTERH_CTRLS;
    }

    buttony =  entryy;
    buttonx =  ((cw - MARGIN_LEFT - MARGIN_RIGHT - 
        BUTTON_WIDTH*buttonCount - INTERW_CTRLS * (buttonCount - 1))>>1)
        + MARGIN_LEFT;

    // buttons
    count = (entryCount<<1) + 2;
    for (i = count; i < buttonCount + count; i++) {
        (pCtrlData + i)->class_name      = "button";
        (pCtrlData + i)->dwStyle    = WS_CHILD | WS_VISIBLE | WS_TABSTOP;
        (pCtrlData + i)->dwStyle   |= (buttons + i - count)->flags;
        (pCtrlData + i)->dwExStyle  = 0;
        (pCtrlData + i)->x          = buttonx;
        (pCtrlData + i)->y          = buttony;
        (pCtrlData + i)->w          = BUTTON_WIDTH;
        (pCtrlData + i)->h          = BUTTON_HEIGHT;
        (pCtrlData + i)->id         = (buttons + i - count)->id;
        (pCtrlData + i)->caption    = (buttons + i - count)->text;
        (pCtrlData + i)->dwAddData  = 0;

        buttonx += BUTTON_WIDTH + INTERW_CTRLS;
    }
    
    WinEntryItems.entries       = items;
    WinEntryItems.entrycount    = entryCount;
    WinEntryItems.firstentryid  = maxid;
    WinEntryItems.minbuttonid   = minbuttonid;
    WinEntryItems.maxbuttonid   = maxbuttonid;
    rc = DialogBoxIndirectParam (&DlgBoxData, hParentWnd, WinEntryBoxProc, 
                                        (LPARAM)(&WinEntryItems));

    free (pCtrlData);
    return rc;
}

