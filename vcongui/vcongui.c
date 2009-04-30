/*
** $Id: vcongui.c 7376 2007-08-16 05:40:27Z xgwang $
**
** VCOnGUI - Virtual Console On MiniGUi -
** Copyright (c) 1999-2002 Wei Yongming (ymwei@minigui.org)
** Copyright (C) 2003-2007 Feynman Software
**
** Some idea and source come from CCE (Console Chinese Environment) 
** Thank He Rui and Takashi MANABE for their great work and good license.
*/

/*
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
** The copyright statement of CCE and KON2:
**
** KON2 - Kanji ON Console -
** Copyright (C) 1992-1996 Takashi MANABE (manabe@papilio.tutics.tut.ac.jp)
**
** CCE - Console Chinese Environment -
** Copyright (C) 1998-1999 Rui He (herui@cs.duke.edu)
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
** EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED.  IN NO EVENT SHALL THE TERRENCE R. LAMBERT BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
** OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
** HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
** SUCH DAMAGE.
** 
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/poll.h>

#include "common.h"
#include "minigui.h"
#include "gdi.h"
#include "window.h"
#include "control.h"
#include "mywindows.h"

#include "resource.h"
#include "defs.h"
#include "vcongui.h"
#include "vc.h"
#include "child.h"
#include "terminal.h"
#include "vt.h"

static HMENU createpmenuabout (void)
{
    HMENU hmnu;
    MENUITEMINFO mii;
    memset (&mii, 0, sizeof(MENUITEMINFO));
    mii.type        = MFT_STRING;
    mii.id          = 0;
    mii.typedata    = (DWORD)"About...";
    hmnu = CreatePopupMenu (&mii);
    
    memset (&mii, 0, sizeof(MENUITEMINFO));
    mii.type        = MFT_STRING ;
    mii.state       = 0;
    mii.id          = IDM_ABOUT_THIS;
    mii.typedata    = (DWORD)"About VC on MiniGUI...";
    InsertMenuItem(hmnu, 0, TRUE, &mii);

    memset (&mii, 0, sizeof(MENUITEMINFO));
    mii.type        = MFT_STRING ;
    mii.state       = 0;
    mii.id          = IDM_ABOUT;
    mii.typedata    = (DWORD)"About MiniGUI...";
    InsertMenuItem(hmnu, 1, TRUE, &mii);

    return hmnu;
}

static HMENU createpmenufile (void)
{
    HMENU hmnu;
    MENUITEMINFO mii;
    memset (&mii, 0, sizeof(MENUITEMINFO));
    mii.type        = MFT_STRING;
    mii.id          = 0;
    mii.typedata    = (DWORD)"File";
    hmnu = CreatePopupMenu (&mii);
    
    memset (&mii, 0, sizeof(MENUITEMINFO));
    mii.type        = MFT_STRING;
    mii.state       = 0;
    mii.id          = IDM_NEW;
    mii.typedata    = (DWORD)"New";
    InsertMenuItem(hmnu, 0, TRUE, &mii);
    
    mii.type        = MFT_STRING;
    mii.state       = 0;
    mii.id          = IDM_OPEN;
    mii.typedata    = (DWORD)"Open...";
    InsertMenuItem(hmnu, 1, TRUE, &mii);
    
    mii.type        = MFT_STRING;
    mii.state       = 0;
    mii.id          = IDM_SAVE;
    mii.typedata    = (DWORD)"Save";
    InsertMenuItem(hmnu, 2, TRUE, &mii);
    
    mii.type        = MFT_STRING;
    mii.state       = 0;
    mii.id          = IDM_SAVEAS;
    mii.typedata    = (DWORD)"Save As...";
    InsertMenuItem(hmnu, 3, TRUE, &mii);

    mii.type        = MFT_STRING;
    mii.state       = 0;
    mii.id          = IDM_CLOSE;
    mii.typedata    = (DWORD)"Close";
    InsertMenuItem(hmnu, 4, TRUE, &mii);
    
    mii.type        = MFT_SEPARATOR;
    mii.state       = 0;
    mii.id          = 0;
    mii.typedata    = 0;
    InsertMenuItem(hmnu, 5, TRUE, &mii);

    mii.type        = MFT_STRING;
    mii.state       = 0;
    mii.id          = IDM_EXIT;
    mii.typedata    = (DWORD)"Exit";
    InsertMenuItem(hmnu, 6, TRUE, &mii);

    return hmnu;
}

static HMENU createpmenuedit (void)
{
    HMENU hmnu;
    MENUITEMINFO mii;
    memset (&mii, 0, sizeof(MENUITEMINFO));
    mii.type        = MFT_STRING;
    mii.id          = 0;
    mii.typedata    = (DWORD)"Edit";
    hmnu = CreatePopupMenu (&mii);
    
    mii.type        = MFT_STRING ;
    mii.state       = 0;
    mii.id          = IDM_COPY;
    mii.typedata    = (DWORD)"Copy Screen";
    InsertMenuItem(hmnu, 0, TRUE, &mii);
     
    mii.type        = MFT_STRING;
    mii.state       = 0;
    mii.id          = IDM_PASTE;
    mii.typedata    = (DWORD)"Paste";
    InsertMenuItem(hmnu, 1, TRUE, &mii);  
    
    return hmnu;
}

static HMENU createpmenuterminal (void)
{
    HMENU hmnu;
    MENUITEMINFO mii;
    memset (&mii, 0, sizeof(MENUITEMINFO));
    mii.type        = MFT_STRING;
    mii.id          = 0;
    mii.typedata    = (DWORD)"Terminal";
    hmnu = CreatePopupMenu (&mii);
    
    mii.type        = MFT_STRING;
    mii.state       = 0;
    mii.id          = IDM_40X15;
    mii.typedata    = (DWORD)"40x15 (small)";
    InsertMenuItem(hmnu, 0, TRUE, &mii);
     
    mii.type        = MFT_STRING;
    mii.state       = 0;
    mii.id          = IDM_80X24;
    mii.typedata    = (DWORD)"80x24 (vt100)";
    InsertMenuItem(hmnu, 1, TRUE, &mii);
     
    mii.type        = MFT_STRING;
    mii.state       = MF_CHECKED;
    mii.id          = IDM_80X25;
    mii.typedata    = (DWORD)"80x25 (ibmpc)";
    InsertMenuItem(hmnu, 2, TRUE, &mii);
     
    mii.type        = MFT_STRING;
    mii.state       = 0;
    mii.id          = IDM_80X40;
    mii.typedata    = (DWORD)"80x40 (xterm)";
    InsertMenuItem(hmnu, 3, TRUE, &mii);
     
    mii.type        = MFT_STRING;
    mii.state       = 0;
    mii.id          = IDM_80X52;
    mii.typedata    = (DWORD)"80x52 (ibmvga)";
    InsertMenuItem(hmnu, 4, TRUE, &mii);

    mii.type        = MFT_STRING;
    mii.state       = 0;
    mii.id          = IDM_96X25;
    mii.typedata    = (DWORD)"96x25 (wide)";
    InsertMenuItem(hmnu, 5, TRUE, &mii);

    mii.type        = MFT_STRING;
    mii.state       = 0;
    mii.id          = IDM_96X40;
    mii.typedata    = (DWORD)"96x40 (My favorite)";
    InsertMenuItem(hmnu, 6, TRUE, &mii);

    mii.type        = MFT_STRING;
    mii.state       = 0;
    mii.id          = IDM_96X52;
    mii.typedata    = (DWORD)"96x52 (large)";
    InsertMenuItem(hmnu, 7, TRUE, &mii);

    mii.type        = MFT_STRING;
    mii.state       = 0;
    mii.id          = IDM_CUSTOMIZE;
    mii.typedata    = (DWORD)"Customize...";
    InsertMenuItem(hmnu, 8, TRUE, &mii);
    
    mii.type        = MFT_SEPARATOR;
    mii.state       = 0;
    mii.id          = 0;
    mii.typedata    = 0;
    InsertMenuItem(hmnu, 9, TRUE, &mii);

    mii.type        = MFT_STRING;
    mii.state       = 0;
    mii.id          = IDM_DEFAULT;
    mii.typedata    = (DWORD)"Default Charset";
    InsertMenuItem(hmnu, 10, TRUE, &mii);

    mii.type        = MFT_STRING;
    mii.state       = 0;
    mii.id          = IDM_ANSI;
    mii.typedata    = (DWORD)"ANSI";
    InsertMenuItem(hmnu, 11, TRUE, &mii);

    mii.type        = MFT_STRING;
    mii.state       = 0;
    mii.id          = IDM_GB2312;
    mii.typedata    = (DWORD)"GB2312";
    InsertMenuItem(hmnu, 12, TRUE, &mii);

    mii.type        = MFT_STRING;
    mii.state       = 0;
    mii.id          = IDM_BIG5;
    mii.typedata    = (DWORD)"BIG5";
    InsertMenuItem(hmnu, 13, TRUE, &mii);

    return hmnu;
}

static HMENU createmenu (void)
{
    HMENU hmnu;
    MENUITEMINFO mii;

    hmnu = CreateMenu();

    memset (&mii, 0, sizeof(MENUITEMINFO));
    mii.type        = MFT_STRING;
    mii.id          = 100;
    mii.typedata    = (DWORD)"File";
    mii.hsubmenu    = createpmenufile ();

    InsertMenuItem(hmnu, 0, TRUE, &mii);

    mii.type        = MFT_STRING;
    mii.id          = 110;
    mii.typedata    = (DWORD)"Edit";
    mii.hsubmenu    = createpmenuedit ();
    InsertMenuItem(hmnu, 1, TRUE, &mii);
    
    mii.type        = MFT_STRING;
    mii.id          = 120;
    mii.typedata    = (DWORD)"Terminal";
    mii.hsubmenu    = createpmenuterminal ();
    InsertMenuItem(hmnu, 2, TRUE, &mii);
    
    mii.type        = MFT_STRING;
    mii.id          = 130;
    mii.typedata    = (DWORD)"About";
    mii.hsubmenu    = createpmenuabout ();
    InsertMenuItem(hmnu, 3, TRUE, &mii);
                   
    return hmnu;
}

static void SetTerminalWindowSize (PCONINFO pConInfo, WPARAM cmd_id)
{
    int col, row;
    RECT new_win_rc;
    struct winsize twinsz;
    
    switch (cmd_id) {
    case IDM_40X15:
        col = 40;
        row = 15;
        break;
    case IDM_80X24:
        col = 80;
        row = 24;
        break;
    case IDM_80X25:
        col = 80;
        row = 25;
        break;
    case IDM_80X40:
        col = 80;
        row = 40;
        break;
    case IDM_80X52:
        col = 80;
        row = 52;
        break;
    case IDM_96X25:
        col = 96;
        row = 25;
        break;
    case IDM_96X40:
        col = 96;
        row = 40;
        break;
    case IDM_96X52:
        col = 96;
        row = 52;
        break;
    case IDM_CUSTOMIZE:
    {
        char cols [10];
        char rows [10];
        char* newcols = cols;
        char* newrows = rows;
        myWINENTRY entries [] = {
            { "New number of columns:", &newcols, 0, 0 },
            { "New number of rows   :", &newrows, 0, 0 },
            { NULL, NULL, 0, 0 }
        };
        myWINBUTTON buttons[] = {
            { "OK", IDOK, BS_DEFPUSHBUTTON },
            { "Cancel", IDCANCEL, 0 },
            { NULL, 0, 0}
        };

        sprintf (cols, "%d", pConInfo->cols);
        sprintf (rows, "%d", pConInfo->rows);
        if (myWinEntries (pConInfo->hWnd,
                "New Terminal Window Size",
                "Please specify the new terminal window size",
                340, 100, FALSE, entries, buttons) != IDOK)
            return;

        col = newcols ? atoi (newcols) : 0;
        row = newrows ? atoi (newrows) : 0;
        free (newcols);
        free (newrows);

        if (col < MIN_COLS || col > MAX_COLS 
                || row < MIN_ROWS || row > MAX_ROWS) {
            MessageBox (pConInfo->hWnd, 
                    "Please specify a valid terminal window size.",
                    "Virtual Console on MiniGUI",
                    MB_OK | MB_ICONINFORMATION | MB_BASEDONPARENT);
            return;
        }
        break;
    }
    default:
        return;
    }

    pConInfo->termType = cmd_id;

    if (!VtChangeWindowSize (pConInfo, row, col))
        return;

    GetWindowRect (pConInfo->hWnd, &new_win_rc);
    MoveWindow (pConInfo->hWnd, new_win_rc.left, new_win_rc.top,
                ClientWidthToWindowWidth (WS_CAPTION | WS_BORDER, 
                    col * GetCharWidth ()),
                ClientHeightToWindowHeight (WS_CAPTION | WS_BORDER,
                    row * GetCharHeight (), 
                    GetMenu (pConInfo->hWnd)), TRUE);

    // Set new terminal window size
    twinsz.ws_row = row;
    twinsz.ws_col = col;
    ioctl (pConInfo->masterPty, TIOCSWINSZ, &twinsz);
}

static BOOL SetTerminalCharset (PCONINFO pConInfo, WPARAM cmd_id)
{
    LOGFONT log_font;
    PLOGFONT sys_font;
    char* charset;
    HDC hdc;

    if (pConInfo->termCharset == cmd_id)
        return FALSE;

    switch (cmd_id) {
    case IDM_ANSI:
        charset = "ISO8859-1";
        break;

    case IDM_GB2312:
        charset = "GB2312-80";
        break;

    case IDM_BIG5:
        charset = "BIG5";
        break;

    case IDM_DEFAULT:
        charset = NULL;
        break;

    default:
        return FALSE;
    }

    if (cmd_id == IDM_DEFAULT)
        pConInfo->log_font = NULL;
    else {
        sys_font = GetSystemFont (SYSLOGFONT_DEFAULT);
        memcpy (&log_font, sys_font, sizeof (LOGFONT));

        if (pConInfo->log_font)
            DestroyLogFont (pConInfo->log_font);

        strcpy (log_font.charset, charset);
        pConInfo->log_font = CreateLogFontIndirect (&log_font);
    }

    pConInfo->termCharset = cmd_id;

    hdc = GetPrivateClientDC (pConInfo->hWnd);
    SelectFont (hdc, pConInfo->log_font);
    return TRUE;
}

static int VCOnGUIMainWinProc (HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{
    PCONINFO pConInfo;

    pConInfo = (PCONINFO)GetWindowAdditionalData (hWnd);
    switch (message) {
        case MSG_SETFOCUS:
            ActiveCaret (hWnd);
            ShowCaret (hWnd);

            init_key_info (&pConInfo->kinfo);
        break;

        case MSG_KILLFOCUS:
            HideCaret (hWnd);

            init_key_info (&pConInfo->kinfo);
        break;
        
        case MSG_DOESNEEDIME:
            return TRUE;

        case MSG_KEYUP:
            HandleInputKeyUp (pConInfo, wParam, lParam);
        break;

        case MSG_KEYDOWN:
            HandleInputKeyDown (pConInfo, wParam, lParam);
        break;

        case MSG_CHAR:
            HandleInputChar (pConInfo, wParam, lParam);
        break;

#ifdef _LITE_VERSION
        case MSG_FDEVENT:
            ReadMasterPty (pConInfo);
        break;
#endif

        case MSG_PAINT:
        {
            HDC hdc;
            
            hdc = BeginPaint (hWnd);
            SelectFont (hdc, pConInfo->log_font);
            TextRepaintAll (pConInfo);
            EndPaint (hWnd, hdc);
            return 0;
        }

        case MSG_LBUTTONDOWN:
            if (GetShiftKeyStatus() & KS_RIGHTBUTTON)
                HandleMouseBothDown (pConInfo, 
                    LOWORD (lParam), HIWORD (lParam), wParam);
            else {
                SetCapture (hWnd);
                HandleMouseLeftDownWhenCaptured (pConInfo, 
                        LOWORD (lParam), HIWORD (lParam), wParam);
                pConInfo->m_captured = true;
           }
        break;

        case MSG_MOUSEMOVE:
            if (pConInfo->m_captured == true) {
                int x = LOWORD (lParam);
                int y = HIWORD (lParam);
                
                if (wParam & KS_CAPTURED)
                    ScreenToClient (hWnd, &x, &y);
                HandleMouseMoveWhenCaptured (pConInfo, x, y, wParam);
            }
        break;

        case MSG_LBUTTONUP:
            ReleaseCapture ();
            if (pConInfo->m_captured == true) {
                int x = LOWORD (lParam);
                int y = HIWORD (lParam);
                
                if (wParam & KS_CAPTURED)
                    ScreenToClient (hWnd, &x, &y);
                HandleMouseLeftUpWhenCaptured (pConInfo, x, y, wParam);

                pConInfo->m_captured = false;
            }
        break;

        case MSG_RBUTTONDOWN:
            if (GetShiftKeyStatus() & KS_LEFTBUTTON)
                HandleMouseBothDown (pConInfo, 
                    LOWORD (lParam), HIWORD (lParam), wParam);
        break;
        
        case MSG_RBUTTONUP:
            HandleMouseRightUp (pConInfo, 
                    LOWORD (lParam), HIWORD (lParam), wParam);
        break;

        case MSG_ACTIVEMENU:
            if (wParam == 2) {
                CheckMenuRadioItem ((HMENU)lParam, 
                    IDM_40X15, IDM_CUSTOMIZE, 
                    pConInfo->termType, MF_BYCOMMAND);
                CheckMenuRadioItem ((HMENU)lParam, 
                    IDM_DEFAULT, IDM_BIG5, 
                    pConInfo->termCharset, MF_BYCOMMAND);
            }
        break;
        
        case MSG_COMMAND:
        switch (wParam) 
        {
            case IDM_NEW:
            case IDM_OPEN:
            case IDM_SAVE:
            case IDM_SAVEAS:
            case IDM_CLOSE:
            break;

            case IDM_COPY:
                TextCopy (pConInfo, 0, 0, pConInfo->dispxmax - 1, 
                                          pConInfo->dispymax - 1);
            break;

            case IDM_PASTE:
                TextPaste (pConInfo);
            break;
            
            case IDM_EXIT:
                SendMessage (hWnd, MSG_CLOSE, 0, 0L);
            break;

            case IDM_40X15 ... IDM_CUSTOMIZE:
                SetTerminalWindowSize (pConInfo, wParam);
            break;
            
            case IDM_DEFAULT ... IDM_BIG5:
                if (SetTerminalCharset (pConInfo, wParam))
                    InvalidateRect (hWnd, NULL, FALSE);
            break;
            
            case IDM_ABOUT_THIS:
                MessageBox (hWnd, 
                    "VCOnGUI - Virtual Console On MiniGUI " VCONGUI_VERSION "\n"
                    "Copyright (C) 1999-2001 Wei Yongming (ymwei@minigui.org).\n"
                    "Some idea comes from CCE by He Rui and others.\n"
                    "    CCE: Copyright (C) 1998-1999 He Rui and others.\n",
                    "About VC On MiniGUI",
                    MB_OK | MB_ICONINFORMATION | MB_BASEDONPARENT);
            break;
            
            case IDM_ABOUT:
#ifdef _MISC_ABOUTDLG
#ifdef _LITE_VERSION
                OpenAboutDialog (hWnd);
#else
                OpenAboutDialog ();
#endif
#endif
            break;
        }
        break;

        case MSG_CLOSE:
            if (MessageBox (hWnd, 
                    "Please type \"exit\" at shell prompt "
                    "or terminate the current program to quit if possible.\n\n"
                    "Or you can choose \"Cancel\" to try kill the process.",
                    "Virtual Console on MiniGUI", 
                    MB_OKCANCEL | MB_ICONQUESTION | MB_BASEDONPARENT) == IDCANCEL)
                kill (pConInfo->childPid, SIGKILL);
        return 0;

        case MSG_DESTROY:
        return 0;
    }
    
    if (pConInfo->DefWinProc)
        return (*pConInfo->DefWinProc)(hWnd, message, wParam, lParam);
    else
        return DefaultMainWinProc (hWnd, message, wParam, lParam);
}

static void InitCreateInfo (PMAINWINCREATE pCreateInfo, 
                PCONINFO pConInfo, int x, int y, bool fMenu)
{
    pCreateInfo->dwStyle = WS_CAPTION | WS_BORDER;
    pCreateInfo->dwExStyle = WS_EX_IMECOMPOSE | WS_EX_USEPRIVATECDC;
    pCreateInfo->spCaption = "Virtual Console on MiniGUI";
    pCreateInfo->hMenu = fMenu?createmenu ():0;
    pCreateInfo->hCursor = GetSystemCursor (IDC_IBEAM);
    pCreateInfo->hIcon = 0;
    pCreateInfo->MainWindowProc = VCOnGUIMainWinProc;
    pCreateInfo->lx = x; 
    pCreateInfo->ty = y;
    pCreateInfo->rx 
        = x + ClientWidthToWindowWidth (WS_CAPTION | WS_BORDER, 
                pConInfo->cols * GetCharWidth ());
    pCreateInfo->by
        = y + ClientHeightToWindowHeight (WS_CAPTION | WS_BORDER,
                pConInfo->rows * GetCharHeight (), fMenu);
    pCreateInfo->iBkColor = COLOR_black;
    pCreateInfo->dwAddData = (DWORD)pConInfo;
    pCreateInfo->hHosting = HWND_DESKTOP;
}

void* VCOnMiniGUI (void* data)
{
    MSG Msg;
    MAINWINCREATE CreateInfo;
    HWND hMainWnd;
    PCONINFO pConInfo;
    CHILDINFO ChildInfo;

    if (!data) {
        ChildInfo.startupMessage = true;
        ChildInfo.startupStr = NULL;
        ChildInfo.execProg = NULL;
        
        ChildInfo.DefWinProc = NULL;
        ChildInfo.fMenu = true;
        ChildInfo.left = 0;
        ChildInfo.top  = 0;
        ChildInfo.rows = 0;
        ChildInfo.cols = 0;
    }
    else
        ChildInfo = *((PCHILDINFO)data);

    if ( !(pConInfo = AllocConInfo ()) ) {
        MessageBox (HWND_DESKTOP, "Init Virtual Console information error!", 
                                  "Error", MB_OK | MB_ICONSTOP);
        return NULL;
    }
    if (ChildInfo.rows > 0) {
        pConInfo->rows = ChildInfo.rows;
        pConInfo->termType = IDM_CUSTOMIZE;
    }
    if (ChildInfo.cols > 0) {
        pConInfo->cols = ChildInfo.cols;
        pConInfo->termType = IDM_CUSTOMIZE;
    }

    pConInfo->termCharset = IDM_DEFAULT;
    pConInfo->log_font = NULL;

    init_key_info (&pConInfo->kinfo);
    
    InitCreateInfo (&CreateInfo, pConInfo, 
        ChildInfo.left, ChildInfo.top, ChildInfo.fMenu);
    pConInfo->DefWinProc = ChildInfo.DefWinProc;
    
    hMainWnd = CreateMainWindow (&CreateInfo);
    if (hMainWnd == HWND_INVALID) {
        FreeConInfo (pConInfo);
        MessageBox (HWND_DESKTOP, "Create Virtual Console window error!", 
                                  "Error", MB_OK | MB_ICONSTOP);
        return NULL;
    }

    if (!CreateCaret (hMainWnd, NULL, GetCCharWidth (), GetCharHeight ()))
        fprintf (stderr, "Create caret error!\n");
    
    ShowWindow (hMainWnd, SW_SHOWNORMAL);
    ShowCaret (hMainWnd);

    pConInfo->hWnd = hMainWnd;
    if (!TerminalStart (pConInfo, &ChildInfo)) {
        MessageBox (hMainWnd, "Create Terminal error!", 
                                  "Error", MB_OK | MB_ICONSTOP);
        goto error;
    }

    // Enter message loop.
#ifdef _LITE_VERSION
    RegisterListenFD (pConInfo->masterPty, POLLIN, hMainWnd, 0);

    while (!pConInfo->terminate && GetMessage (&Msg, hMainWnd)) {
        DispatchMessage (&Msg);
    }
    UnregisterListenFD (pConInfo->masterPty);
#else
    do {
        // It is time to read from master pty, and output.
        ReadMasterPty (pConInfo);

        if (pConInfo->terminate)
            break;

        while (HavePendingMessage (hMainWnd)) {
            if (!GetMessage (&Msg, hMainWnd))
                break;
            DispatchMessage (&Msg);
        }
    } while (TRUE);
#endif

    TerminalCleanup (pConInfo);

error:
    DestroyCaret (hMainWnd);
    DestroyMainWindow (hMainWnd);
    FreeConInfo (pConInfo);
#ifdef _LITE_VERSION
    EmptyMessageQueue (hMainWnd);
#endif
    MainWindowThreadCleanup (hMainWnd);

    return NULL;
}

#ifndef _LITE_VERSION
void* NewVirtualConsole (PCHILDINFO pChildInfo)
{
    pthread_t th;
    
    CreateThreadForMainWindow (&th, NULL, VCOnMiniGUI, pChildInfo);

    return NULL;
}
#endif

