/*
** $Id: helpwin.c 7371 2007-08-16 05:30:47Z xgwang $
**
** helpwin.c: a useful help window.
**
** Copyright (C) 2003 ~ 2007 Feynman Software.
** Copyright (C) 2001 ~ 2002 Wei Yongming.
**
** Current maintainer: Wei Yongming.
**
** Create date: 2001/12/21
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "minigui.h"
#include "gdi.h"
#include "window.h"
#include "control.h"
#include "mywindows.h"
#include "mgext.h"

#if defined (_CTRL_BUTTON) && defined (_EXT_CTRL_SPINBOX)

#define IDC_SPIN    100

static CTRLDATA _help_win_ctrls [] =
{ 
    {
        CTRL_BUTTON,
        WS_TABSTOP | WS_VISIBLE | BS_DEFPUSHBUTTON, 
        0, 0, 0, 0, IDOK, "OK"
    },
    {
        CTRL_SPINBOX,
        WS_CHILD | SPS_AUTOSCROLL | WS_VISIBLE,
        0, 0, 0, 0, IDC_SPIN, "",
    }
};

static DLGTEMPLATE _help_win =
{
    WS_BORDER | WS_CAPTION | WS_VISIBLE,
    WS_EX_NONE,
    0, 0, 0, 0,
    NULL,
    0, 0, 2
};

typedef struct tagHELPMSGINFO {
    const char* msg;
    int nr_lines;

    int vis_lines;
    int start_line;

    RECT rc;
} HELPMSGINFO;

static int _help_win_proc (HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
    HELPMSGINFO* info;
    SPININFO spinfo;

    info = (HELPMSGINFO*) GetWindowAdditionalData (hDlg);

    switch (message) {
    case MSG_INITDIALOG:
        info = (HELPMSGINFO*) lParam;
        spinfo.min = 0;
        spinfo.max = info->nr_lines - info->vis_lines;
        if (spinfo.max < 0) spinfo.max = 0;
        spinfo.cur = 0;

        SendMessage ( GetDlgItem ( hDlg, IDC_SPIN), SPM_SETTARGET, 0, (LPARAM) hDlg);
        SendMessage ( GetDlgItem ( hDlg, IDC_SPIN), SPM_SETINFO, 0, (LPARAM) &spinfo);
        SetWindowAdditionalData (hDlg, (DWORD) lParam);
        return 1;
        
    case MSG_COMMAND:
        if (wParam == IDOK)
            EndDialog (hDlg, IDOK);
        break;

    case MSG_PAINT:
    {
        HDC hdc = BeginPaint (hDlg);
        RECT rc = info->rc;

        rc.top -= info->start_line * GetSysCharHeight ();
        SetBkMode (hdc, BM_TRANSPARENT);
        DrawText (hdc, info->msg, -1, &rc,
            DT_LEFT | DT_TOP | DT_WORDBREAK | DT_EXPANDTABS);

        EndPaint (hDlg, hdc);
        return 0;
    }

    case MSG_KEYDOWN:
        if (wParam == SCANCODE_CURSORBLOCKUP) {
            if (info->start_line > 0) {
                info->start_line--;
                if (info->start_line == 0 && !(lParam & KS_SPINPOST))
                    SendDlgItemMessage ( hDlg, IDC_SPIN, SPM_SETCUR, info->start_line, 0);
                InvalidateRect (hDlg, &info->rc, TRUE);
            }
            return 0;
        } else if ( wParam == SCANCODE_CURSORBLOCKDOWN ) {
            if (info->start_line + info->vis_lines < info->nr_lines) {
                info->start_line++;
                if (info->start_line + info->vis_lines == info->nr_lines
                                && !(lParam & KS_SPINPOST))
                    SendDlgItemMessage ( hDlg, IDC_SPIN, SPM_SETCUR, info->start_line, 0);
                InvalidateRect (hDlg, &info->rc, TRUE);
            }
            return 0;
        } 
        break;

    case MSG_CLOSE:
        EndDialog (hDlg, IDOK);
        return 0;        
    }
    
    return DefaultDialogProc (hDlg, message, wParam, lParam);
}

#define LEFT_MARGIN     12
#define BOTTOM_MARGIN   6

int myWinHelpMessage (HWND hwnd, int width, int height, 
                const char* help_title, const char* help_msg)
{
    int spinbox_w = 0, spinbox_h = 0;
    HELPMSGINFO info;
    RECT rc;

    GetSpinBoxSize (SPS_AUTOSCROLL, &spinbox_w, &spinbox_h);

    rc.top = 0;
    rc.left = LEFT_MARGIN;
    rc.right = width - GetMainWinMetrics(MWM_BORDER) * 2 - LEFT_MARGIN;
    rc.bottom = height;

    _help_win_ctrls[0].x = LEFT_MARGIN;
    _help_win_ctrls[0].y = height - GetMainWinMetrics(MWM_BORDER) * 2 
            - GetMainWinMetrics(MWM_CAPTIONY) - spinbox_h 
            - 2*BTN_WIDTH_BORDER - 2 - BOTTOM_MARGIN;
    _help_win_ctrls[0].w = spinbox_w * 6;
    _help_win_ctrls[0].h = spinbox_h + 2*BTN_WIDTH_BORDER + 2;

    _help_win_ctrls[0].caption = GetSysText (IDS_MGST_OK);

    _help_win_ctrls[1].x = width - GetMainWinMetrics(MWM_BORDER) * 2
            - spinbox_w - LEFT_MARGIN;
    _help_win_ctrls[1].y = _help_win_ctrls[0].y + BTN_WIDTH_BORDER + 1; 
    _help_win_ctrls[1].w = spinbox_w;
    _help_win_ctrls[1].h = spinbox_h;

    _help_win.w = width;
    _help_win.h = height;
    _help_win.caption = help_title;
    _help_win.controls = _help_win_ctrls;

    info.msg = help_msg;

    SelectFont (HDC_SCREEN, GetSystemFont (SYSLOGFONT_WCHAR_DEF));
    DrawText (HDC_SCREEN, info.msg, -1, &rc,
        DT_LEFT | DT_TOP | DT_WORDBREAK | DT_EXPANDTABS | DT_CALCRECT);

    info.nr_lines = RECTH (rc) / GetSysCharHeight (); 

    if (info.nr_lines <= 0) return -1;

    info.rc.top = BOTTOM_MARGIN;
    info.rc.left = LEFT_MARGIN;
    info.rc.right = width - GetMainWinMetrics(MWM_BORDER) * 2 - LEFT_MARGIN;
    info.rc.bottom = _help_win_ctrls[0].y - BOTTOM_MARGIN;

    info.vis_lines = RECTH (info.rc) / GetSysCharHeight ();

    info.rc.bottom = info.rc.top + info.vis_lines * GetSysCharHeight ();

    info.start_line = 0;


    DialogBoxIndirectParam (&_help_win, hwnd, _help_win_proc, (LPARAM)&info);

    return 0;
}

#endif /* defined (_CTRL_BUTTON) && defined (_EXT_CTRL_SPINBOX) */

