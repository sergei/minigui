/*
 * $Id: vc.h 7376 2007-08-16 05:40:27Z xgwang $
 *
 * vc.h: head file of Virtual Console module.
 * 
 * KON2 - Kanji ON Console -
 * Copyright (C) 1992-1996 Takashi MANABE (manabe@papilio.tutics.tut.ac.jp)
 * 
 * CCE - Console Chinese Environment -
 * Copyright (C) 1998-1999 Rui He (herui@cs.duke.edu)
 *
 * VCOnGUI - Virtual Console On MiniGUi -
** Copyright (c) 1999-2002 Wei Yongming (ymwei@minigui.org)
** Copyright (C) 2003-2007 Feynman Software
 *
 */

/*
**  This library is free software; you can redistribute it and/or
**  modify it under the terms of the GNU Library General Public
**  License as published by the Free Software Foundation; either
**  version 2 of the License, or (at your option) any later version.
**
**  This software is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
**  Library General Public License for more details.
**
**  You should have received a copy of the GNU Library General Public
**  License along with this library; if not, write to the Free
**  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
**  MA 02111-1307, USA
*/

#ifndef VC_VCONGUI_H
  #define VC_VCONGUI_H

#include "key.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define VGASTD_NUMBER_ROWS  25
#define VGASTD_NUMBER_COLS  80

#define VGAEXT_NUMBER_ROWS  52
#define VGAEXT_NUMBER_COLS  80

#define WIDTH_TABSTOP       8

#define MAX_TTYNAME         10

struct attrStack
{
    struct  attrStack *prev;
    short   x, y;
    int     attr;
    int     bcol, fcol;
};

#define     MAX_NARG    8
// #define     USECCEWAY

typedef struct _CONINFO
{
    HWND    hWnd;

    short   dispxmax, dispymax;

    int     termType;

    int     termCharset;
    PLOGFONT log_font;
    
    short   cols, rows;
    short   x, y;
    short   xmax;
    short   ymin, ymax;
    short   tab;

    char    ptyName [MAX_TTYNAME + 1];
    int     masterPty; //, slavePty;
    pid_t   childPid;

    int     terminate;

    struct attrStack *saveAttr;

    u_char* textBuff;
    u_char* attrBuff;
    u_char* flagBuff;
    
    bool    textClear;
    int     textHead, scrollLine;
    int     currentScroll;
    int     scroll;
    int     textSize; 
    
    u_char  fcol;       /* foreground color */
    u_char  bcol;       /* background color */
    u_char  attr;       /* attribute */
    u_char  sb;         /* 1 byte code */
    u_char  db;         /* 2 byte code */

#ifdef USECCEWAY
    u_char  knj1;       /* 1 byte, first half of the double-byte char */
    short   knj1x, knj1y; /* position of first half of the double-byte char */
    short   knj2x, knj2y; /* position of second half of the double-byte char */
#endif
    
    bool    cursorsw;
    bool    soft;

    void (*esc)(struct _CONINFO* con, u_char);

    enum {CS_LEFT, CS_RIGHT, CS_GRAPH, CS_DBCS} trans, g[2]; 

    bool    ins, wrap;

    u_char  sysCoding;
    u_short varg [MAX_NARG], narg;
    bool question;

    key_info    kinfo;

    bool m_captured;
    int  m_origx, m_origy;
    int  m_oldx, m_oldy;

    WNDPROC DefWinProc;
}CONINFO;
typedef CONINFO* PCONINFO;

#define ATTR_ULINE      0x80    /* under line */
#define ATTR_REVERSE    0x40    /* reverse */
#define ATTR_HIGH       0x20    /* high */

#define LATCH_S         0x00    /* single byte char */
#define LATCH_1         0x20    /* double byte char 1st byte */
#define LATCH_2         0x40    /* double byte char 2nd byte */

#define CLEAN_S         0x80

#define CODEIS_1        LATCH_1
#define CODEIS_2        LATCH_2
#define CODEIS_S        LATCH_S
#define LANG_CODE       0x0F

PCONINFO AllocConInfo (void);
void FreeConInfo (PCONINFO con);

void ConsoleInit (void);
void ConsoleStart (void);
void ConsoleCleanup (void);

void TextClearAll (CONINFO *con);
void TextClearEol (CONINFO *con, u_char);
void TextClearEos (CONINFO *con, u_char);
void TextClearChars (CONINFO* con, int len);

void TextDeleteChar (CONINFO *con, int);
void TextInsertChar (CONINFO *con, int);
void TextMoveDown (CONINFO *con, int top, int btm, int line);
void TextMoveUp (CONINFO *con, int top, int btm, int line);
void TextMode (void);
void GraphMode (void);
void ScrollUp (CONINFO *con, int);
void ScrollDown (CONINFO *con, int);
void TextWput (CONINFO *con, u_char ch1, u_char ch2);
void TextWput1 (CONINFO *con, u_char ch);
void TextWput2 (CONINFO *con, u_char ch);
void TextSput (CONINFO *con, u_char ch);
void TextReverse (CONINFO *con, int* ofx, int* ofy, int* otx, int* oty);
void TextRefresh (CONINFO *con, bool bHideCaret);
void TextCopy (CONINFO *con, int fx, int fy, int tx, int ty);
void TextPaste (CONINFO *con);
void PollCursor (bool wakeup); 

void TextRepaintAll (CONINFO *con);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif // VC_VCONGUI_H 

