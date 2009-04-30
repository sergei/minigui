/*
** $Id: vc.c 7376 2007-08-16 05:40:27Z xgwang $
**
** KON2 - Kanji ON Console -
** Copyright (C) 1992-1996 Takashi MANABE (manabe@papilio.tutics.tut.ac.jp)
** 
** CCE - Console Chinese Environment -
** Copyright (C) 1998-1999 Rui He (herui@cs.duke.edu)
**
** VCOnGUI - Virtual Console On MiniGUI -
** Copyright (c) 1999-2002 Wei Yongming (ymwei@minigui.org)
** Copyright (C) 2003-2007 Feynman Software
**
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/vt.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <fcntl.h>
#include <signal.h>
#include <termio.h>
#include <dirent.h>

#include "common.h"
#include "minigui.h"
#include "gdi.h"
#include "window.h"
#include "control.h"

#include "vcongui.h"
#include "resource.h"
#include "defs.h"
#include "vc.h"
#include "vt.h"
#include "paint.h"

static void TextClearBand (CONINFO *con, int top, int btm);

PCONINFO AllocConInfo (void)
{
    PCONINFO pCONINFO;

    if ( !(pCONINFO = malloc (sizeof (CONINFO))) )
        return NULL;

    memset (pCONINFO, 0, sizeof (CONINFO));

    pCONINFO->cols = VGASTD_NUMBER_COLS;
    pCONINFO->rows = VGASTD_NUMBER_ROWS;
    pCONINFO->termType = IDM_80X25;

    return pCONINFO;
}


void FreeConInfo (PCONINFO pCONINFO)
{
    free (pCONINFO);
}

/*
  con->flagBuff:
  |      7|      6|      5|4||3|2|1|0|
  |CLEAN_S|LATCH_2|LATCH_1| ||<----->|
  |0=latch|  byte2|  byte1| ||   LANG|

  Usage: CLEAN_S         indicate if we need to redraw this char
         LATCH_1/LATCH_2 double byte char (Chinese, Japanese, Korean)
         LANG            language, we can support BIG5/GB same screen
*/

/* blatch clear the bit 7 of array head byte , force to redraw */
#ifdef USE_ASM
static inline void blatch(void *head, int n)
    __asm__("\t clc\n"
        "1:\n"
        "\t andb %%dl, (%%eax)\n"
        "\t incl %%eax\n"
        "\t loop 1b\n"
        :
        : "eax" ((long)head), "dl" (0x7F), "c" (n)
        : "dl", "cx" );
}
#else
static inline void blatch(BYTE *head, int n)
{
    int i;

    for (i = 0; i < n; i++)
       *head++ &= 0x7F;
}
#endif

/* long latch */
#ifdef USE_ASM
static inline void llatch(void *head, int n)
{
    __asm__("\t clc\n"
        "1:\n"
        "\t andl %%ebx, (%%eax)\n"
        "\t addl $4, %%eax\n"
        "\t loop 1b\n"
        :
        : "eax" ((long)head), "ebx" (0x7F7F7F7F), "c" (n>>2)
        : "ebx", "cx" );
}
#else
static inline void llatch (void* head, int n)
{
    unsigned int *u = (unsigned int*) head;
    int i;

    for (i = 0; i < n>>2; i++)
       *u++ &= 0x7F7F7F7F;
}
#endif

static inline u_int TextAddress (CONINFO *con, u_int x, u_int y)
{
    return (con->textHead + x + y * con->dispxmax) % con->textSize;
}

static inline bool IsKanji (CONINFO *con, u_int x, u_int y)
{
    return(*(con->flagBuff + TextAddress (con, x, y)) & CODEIS_1);
}

static inline bool IsKanji2 (CONINFO *con, u_int x, u_int y)
{
    return(*(con->flagBuff + TextAddress (con, x, y)) & CODEIS_2);
}

/****************************************************************************
 *                     TextDeleteChar/TextInsertChar                        *
 ****************************************************************************/
void TextDeleteChar (CONINFO *con, int n)
{
    u_int addr, dx;
    
    addr = TextAddress (con, con->x, con->y);
    dx = con->dispxmax - con->x - n;
    
    memmove (con->textBuff + addr, con->textBuff + addr + n, dx);
    memmove (con->attrBuff + addr, con->attrBuff + addr + n, dx);
    blatch (con->flagBuff + addr, dx);
    
    addr = TextAddress (con, con->dispxmax - n, con->y);

    bzero (con->textBuff + addr, n);
    bzero (con->attrBuff + addr, n);
    bzero (con->flagBuff + addr, n);
}

void TextInsertChar (CONINFO *con, int n)
{
    u_int addr, dx;

#if 0
    addr = TextAddress (con, con->dispxmax, con->y);
    dx = con->dispxmax - con->x - n;
    
    brmove (con->textBuff + addr, con->textBuff + addr - n, dx + 1);
    brmove (con->attrBuff + addr, con->attrBuff + addr - n, dx + 1);
    
    addr = TextAddress (con, con->x, con->y);
    blatch (con->flagBuff + addr + n, dx);
    bzero (con->textBuff + addr, n);
    bzero (con->attrBuff + addr, n);
    bzero (con->flagBuff + addr, n);
#else
    addr = TextAddress (con, con->x, con->y);
    dx = con->dispxmax - con->x - n;
    
    memmove (con->textBuff + addr + n, con->textBuff + addr, dx);
    memmove (con->attrBuff + addr + n, con->attrBuff + addr, dx);
    
    addr = TextAddress (con, con->x, con->y);
    blatch (con->flagBuff + addr + n, dx);
    bzero (con->textBuff + addr, n);
    bzero (con->attrBuff + addr, n);
    bzero (con->flagBuff + addr, n);
#endif
}

/****************************************************************************
 *                     TextScrollUp/TextScrollDown                          *
 *                     TextMoveUp/TextMoveDown                              *
 *                        ScrollUp/ScrollDown                               *
 ****************************************************************************/

#if 0
static void TextScrollUp (CONINFO *con, int line)
{
    int oldhead, len;

    oldhead = con->textHead;
    con->textHead += line * con->dispxmax;

    if (con->textHead > con->textSize) {
    
        con->textHead -= con->textSize;
        len = con->textSize - oldhead;
        
        if (con->textHead) {
            lzero (con->textBuff, con->textHead);
            lzero (con->attrBuff, con->textHead);
            lzero (con->flagBuff, con->textHead);
        }
    }
    else
        len = con->textHead - oldhead;

    lzero (con->textBuff + oldhead, len);
    lzero (con->attrBuff + oldhead, len);
    lzero (con->flagBuff + oldhead, len);
}

static void TextScrollDown (CONINFO *con, int line)
{
    int oldhead, len;

    oldhead = con->textHead;
    con->textHead -= line * con->dispxmax;

    if (con->textHead < 0) {
        con->textHead += con->textSize;
        if (oldhead) {
            lzero(con->textBuff, oldhead);
            lzero(con->attrBuff, oldhead);
            lzero(con->flagBuff, oldhead);
        }
        len = con->textSize - con->textHead;
    }
    else
        len = oldhead - con->textHead;

    lzero (con->textBuff + con->textHead, len);
    lzero (con->attrBuff + con->textHead, len);
    lzero (con->flagBuff + con->textHead, len);
}
#endif

void TextMoveUp (CONINFO *con, int top, int btm, int line)
{
    int n, src, dst;

    if (btm - top - line + 1 <= 0) {
        TextClearBand (con, top, btm);
        return;
    }
    
    for (n = top; n <= btm - line; n ++) {
        dst = TextAddress (con, 0, n);
        src = TextAddress (con, 0, n + line);
        memmove (con->textBuff + dst, con->textBuff + src, con->dispxmax);
        memmove (con->attrBuff + dst, con->attrBuff + src, con->dispxmax);
        memmove (con->flagBuff + dst, con->flagBuff + src, con->dispxmax);
        llatch (con->flagBuff + dst, con->dispxmax);
    }
    
    TextClearBand (con, btm - line + 1, btm);
}

void TextMoveDown (CONINFO *con, int top, int btm, int line)
{
    int n, src, dst;

    if (btm - top - line + 1 <= 0) {
        TextClearBand (con, top, btm);
        return;
    }
    
    for (n = btm; n >= top + line; n --) {
        dst = TextAddress (con, 0, n);
        src = TextAddress (con, 0, n - line);
        memmove (con->textBuff + dst, con->textBuff + src, con->dispxmax);
        memmove (con->attrBuff + dst, con->attrBuff + src, con->dispxmax);
        memmove (con->flagBuff + dst, con->flagBuff + src, con->dispxmax);
        llatch (con->flagBuff + dst, con->dispxmax);
    }

    TextClearBand (con, top, top + line - 1);
}

void ScrollUp (CONINFO *con, int line)
{
#if 1
    TextMoveUp(con, con->ymin, con->ymax, line);
#else
    if (con->soft) {
        TextMoveUp(con, con->ymin, con->ymax, line);
    }
    else {
        TextScrollUp (con, line);
        con->scrollLine += line;
    }
#endif
}

void ScrollDown (CONINFO *con, int line)
{
#if 1
    TextMoveDown (con, con->ymin, con->ymax, line);
#else
    if (con->soft) {
        TextMoveDown(con,con->ymin, con->ymax, line);
    }
    else {
        TextScrollDown (con, line);
        con->scrollLine -= line;
    }
#endif
}

/****************************************************************************
 *                       TextClearAll/TextClearEol                          *
 *                       TextClearEos/TextClearBand                         *
 ****************************************************************************/
void TextClearAll (CONINFO *con)
{
    // lzero (con->textBuff, con->textSize);
    // lzero (con->attrBuff, con->textSize);
    memset(con->textBuff, 0x20, con->textSize);
    memset(con->attrBuff, ((con->bcol << 4) | con->fcol), con->textSize);
    lzero (con->flagBuff, con->textSize);
    con->textClear = true;
}

void TextClearEol (CONINFO *con, u_char mode)
{
    u_int  addr;
    u_char len, x=0;

    switch(mode) {
    case 1:
        len = con->x;
        break;
        
    case 2:
        len = con->dispxmax;
        break;

    default:
        x = con->x;
        len = con->dispxmax - con->x;
        break;
    }
    
    addr = TextAddress (con, x, con->y);
    // bzero (con->textBuff + addr, len);
    // bzero (con->attrBuff + addr, len);
    memset (con->textBuff + addr, 0x20, len);
    memset (con->attrBuff + addr, (con->bcol << 4) | con->fcol, len);
    bzero (con->flagBuff + addr, len);     /* needless to latch */
}

void TextClearEos (CONINFO *con, u_char mode)
{
    int addr, len, y;

    if (mode == 2) {
        TextClearAll (con);
        return;
    }

    switch(mode) {
    case 1:
        for (y = 0; y < con->y; y ++) {
            addr = TextAddress (con, 0, y);

            // lzero (con->textBuff + addr, con->dispxmax);
            // lzero (con->attrBuff + addr, con->dispxmax);
            memset (con->textBuff + addr, 0x20, con->dispxmax);
            memset (con->attrBuff + addr, 
                    (con->bcol << 4) | con->fcol, con->dispxmax);

            lzero (con->flagBuff + addr, con->dispxmax);
            /* needless to latch */
        }
        
        addr = TextAddress (con, 0, con->y);
        // bzero (con->textBuff + addr, con->x);
        // bzero (con->attrBuff + addr, con->x);
        memset (con->textBuff + addr, 0x20, con->x);
        memset (con->attrBuff + addr, (con->bcol << 4) | con->fcol, con->x);
        bzero (con->flagBuff + addr, con->x); /* needless to latch */
        break;
        
    default:
        for (y = con->y + 1; y <= con->dispymax; y ++) {
            addr = TextAddress (con, 0, y);
            // lzero (con->textBuff + addr, con->dispxmax);
            // lzero (con->attrBuff + addr, con->dispxmax);
            memset (con->textBuff + addr, 0x20, con->dispxmax);
            memset (con->attrBuff + addr, 
                    (con->bcol << 4) | con->fcol, con->dispxmax);

            lzero (con->flagBuff + addr, con->dispxmax);
            /* needless to latch */        
        }
        
        addr = TextAddress (con, con->x, con->y);
        len = con->dispxmax - con->x;
        // bzero (con->textBuff + addr, len);
        // bzero (con->attrBuff + addr, len);
        memset (con->textBuff + addr, 0x20, len);
        memset (con->attrBuff + addr, (con->bcol << 4) | con->fcol, len);
        bzero (con->flagBuff + addr, len); /* needless to latch */
        break;
    }
}

static void TextClearBand (CONINFO *con, int top, int btm)
{
    int y, addr;

    for (y = top; y <= btm; y ++) {
        addr = TextAddress (con, 0, y);
        // lzero (con->textBuff + addr, con->dispxmax);
        // lzero (con->attrBuff + addr, con->dispxmax);
        memset (con->textBuff + addr, 0x20, con->dispxmax);
        memset (con->attrBuff + addr, 
                (con->bcol << 4) | con->fcol, con->dispxmax);
        lzero (con->flagBuff + addr, con->dispxmax);/* needless to latch */
    }
}

// --------------------------------------------------------------
// TextClearChars - Added by Holly Lee
//    ConInfo * con
//    int row
//    int fromColumn
//    int columns
// --------------------------------------------------------------
void TextClearChars (CONINFO* con, int len)
{
     int addr;
     
     // Must be in one line
     if ( con->x + len > con->dispxmax )
        len = con->dispxmax - con->x;

     addr = TextAddress (con, con->x, con->y);
     memset (con->textBuff + addr, 0x20, len);
     memset (con->attrBuff + addr, (con->bcol << 4) | con->fcol, len);
     bzero (con->flagBuff + addr, len);  /* needless to latch */
}

/****************************************************************************
 *                   TextCopy/TextPaste/TextReverse                         *
 ****************************************************************************/
static inline void KanjiAdjust (CONINFO *con, int *x, int *y)
{
    if (IsKanji2 (con, *x, *y))
        --*x;
}

#define COPY_FILE   "/tmp/.vcongui.copy"

void TextCopy (CONINFO *con, int fx, int fy, int tx, int ty)
{
    int fd;
    int from, to, y, swp, xx, x;
    u_char ch, ch2;
    char szTmpFileName [MAX_PATH];

    strcpy (szTmpFileName, COPY_FILE);
    strcat (szTmpFileName, ".");
    strcat (szTmpFileName, getpwuid (getuid ())->pw_name);

    unlink (szTmpFileName);
    if ((fd = open (szTmpFileName, O_WRONLY|O_CREAT, 0600)) < 0)
        return;

    KanjiAdjust (con, &fx, &fy);
    KanjiAdjust (con, &tx, &ty);
    
    if (fy > ty) {
        swp = fy; fy = ty; ty = swp;
        swp = fx; fx = tx; tx = swp;
    }
    else if (fy == ty && fx > tx) {
        swp = fx; fx = tx; tx = swp;
    }

    for (xx = con->dispxmax - 1, y = fy; y <= ty; y ++) {
    
        if (y == ty)
            xx = tx;
            
        from = TextAddress (con, fx, y);
        
        if (con->flagBuff[from] & CODEIS_2)
            /* 2nd byte of kanji */
            from--;
        to = TextAddress (con, xx, y);
        
        for (x = to; x >= from; x --)
            if (con->textBuff[x] > ' ')
                break;

        to = x;

        for (x = from; x <= to; x ++)
        {
            ch = con->textBuff[x];
            if (!ch)
                ch = ' ';
            if (con->flagBuff[x] & CODEIS_1) {
                x ++;
                ch2 = con->textBuff[x];
                
                switch (con->sysCoding) {
                case CODE_EUC:
                    ch2 |= 0x80;
                    ch |= 0x80;
                    break;
                case CODE_SJIS:
                    jistosjis(ch2, ch);
                    break;
                }
                
                write (fd, &ch, 1);
                write (fd, &ch2, 1);
            }
            else 
                write (fd, &ch, 1);
        }

        if (y < ty) {
            ch = '\n';
            write(fd, &ch, 1);
        }
        
        fx = 0;
    }
    
    close(fd);
}

void TextPaste (CONINFO *con)
{
    u_char ch;
    int    fd;
    char szTmpFileName [MAX_PATH];

    strcpy (szTmpFileName, COPY_FILE);
    strcat (szTmpFileName, ".");
    strcat (szTmpFileName, getpwuid (getuid ())->pw_name);

    if ((fd = open(szTmpFileName, O_RDONLY)) < 0)
        return;
        
    while (read (fd, &ch, 1) == 1)
        write (con->masterPty, &ch, 1);
    
    close(fd);
}

void TextReverse (CONINFO *con, int* ofx, int* ofy, int* otx, int* oty)
{
    int fx, fy, tx, ty;
    int from, to, y, swp, xx, x;
    u_char fc, bc, fc2, bc2;

    KanjiAdjust (con, ofx, ofy);
    KanjiAdjust (con, otx, oty);

    fx = *ofx; fy = *ofy;
    tx = *otx; ty = *oty;

    if (fy > ty) {
        swp = fy; fy = ty; ty = swp;
        swp = fx; fx = tx; tx = swp;
    }
    else if (fy == ty && fx > tx) {
        swp = fx; fx = tx; tx = swp;
    }

    for (xx = con->dispxmax - 1, y = fy; y <= ty; y ++) {
        if (y == ty)
            xx = tx;
            
        from = TextAddress (con, fx, y);
        to = TextAddress (con, xx, y);
        
        if (con->flagBuff[from] & CODEIS_2)
            /* 2nd byte of kanji */
            from--;

        for (x = from; x <= to; x ++) {
            if (!con->textBuff[x])
                continue;
            fc = con->attrBuff[x];
            bc = fc >> 4;
            bc2 = (bc & 8) | (fc & 7);
            fc2 = (fc & 8) | (bc & 7);
            con->attrBuff[x] = fc2 | (bc2 << 4);
            con->flagBuff[x] &= ~CLEAN_S;
        }

        fx = 0;
    }
}

/****************************************************************************
 *                TextWput1/TextWput2/TextWput/TextSput                     *
 ****************************************************************************/
void TextWput1 (CONINFO *con, u_char ch)
{
    u_int addr;

    addr = TextAddress (con, con->x, con->y);
    con->attrBuff[addr] = con->fcol | (con->bcol << 4);
    con->textBuff[addr] = ch;
    con->flagBuff[addr] = con->db;
}

void TextWput2 (CONINFO *con, u_char ch)
{
    u_int addr;

    addr = TextAddress (con, con->x, con->y);
    con->attrBuff[addr] = con->fcol | (con->bcol << 4);
    con->textBuff[addr] = ch;
    con->flagBuff[addr] = LATCH_2;

    // clear first byte.
    con->flagBuff[addr - 1] &= ~CLEAN_S;
}

void TextWput (CONINFO *con, u_char ch1, u_char ch2)
{
    u_int addr;

    addr = TextAddress (con, con->x, con->y);
    con->attrBuff[addr] = con->fcol | (con->bcol << 4);
    con->attrBuff[addr+1] = con->fcol | (con->bcol << 4);
    con->textBuff[addr] = ch2;
    con->textBuff[addr+1] = ch1;
    con->flagBuff[addr] = con->db | LATCH_1;
    con->flagBuff[addr+1] = LATCH_2;
}

void TextSput (CONINFO *con, u_char ch)
{
    u_int addr;

    addr = TextAddress (con, con->x, con->y);
    con->flagBuff[addr] = LATCH_S | con->sb;
    con->attrBuff[addr] = con->fcol | (con->bcol << 4);
    con->textBuff[addr] = ch;
}

/****************************************************************************
 *                         TextRefresh/HardScroll                           *
 ****************************************************************************/

void HardScroll (CONINFO *con)
{
    int scroll, color;
     
    scroll = con->currentScroll + con->scrollLine;
    color = ((con->attr & ATTR_REVERSE) ? con->fcol : con->bcol) & 7;

    /* con->currentScroll should be in 0-18  0-42 18+24=42 */
    if (con->scrollLine > 0) {
        if (scroll < 19)
            WindowScrollUp (con, con->scrollLine, color);
        else {
            WindowScrollDown (con, con->currentScroll, color);
            llatch(con->flagBuff, con->textSize); /* force to redraw */
            scroll = 0;
       }
    }
    else if (con->scrollLine < 0) {
        if (scroll > 0)
            WindowScrollDown (con, con->currentScroll, color);
        else if (scroll < 0) {
            if (con->currentScroll > 0)
                WindowScrollDown (con, con->currentScroll, color);
                llatch (con->flagBuff, con->textSize); /* force to redraw */
                scroll = 0;
           }
      }

      con->currentScroll = scroll;
      con->scrollLine = 0;
}

#define OUTPUTCHAR                                  \
                con->flagBuff[i] |= CLEAN_S;        \
                if (flag & CODEIS_1) {              \
                    i ++;                           \
                    con->flagBuff[i] |= CLEAN_S;    \
                    ch2 = con->textBuff[i];         \
                    if (con->ins)                   \
                        TextInsertChar (con, 2);    \
                    line [count++] = ch;            \
                    line [count++] = ch2;           \
                }                                   \
                else {                              \
                    if (con->ins)                   \
                        TextInsertChar (con, 1);    \
                    if (ch)                         \
                        line [count++] = ch;        \
                    else                            \
                        line [count++] = ' ';       \
                }

void TextRefresh (CONINFO *con, bool bHideCaret)
{
    int     x, y, i, j;
    u_char  ch, ch2, fc, bc, flag;
    int     color;
    u_char  line [MAX_COLS + 1];
    int     count, startx;

    color = ((con->attr & ATTR_REVERSE) ? con->fcol : con->bcol) & 7; 

    if (bHideCaret)
        WindowHideCaret (con->hWnd);

    if (con->textClear)
        WindowClearAll (con, color);
        
    if (bHideCaret)
        HardScroll (con);

    con->textClear = false;

    /* con->textSize = con->dispxmax * con->dispymax = 80 * 25 = 2000 */
    j = 0;
    fc = 0; bc = 0;
    for (y = 0; y < con->dispymax; y ++) {
        count = 0;
        startx = 0;
        for (x = 0; x < con->dispxmax; x ++) {
            u_char newfc, newbc;
                
            i = (con->textHead + j ) % con->textSize;

            newfc = *(con->attrBuff + i);
            newbc = *(con->attrBuff + i) >> 4;
            ch    = *(con->textBuff + i);
            flag  = *(con->flagBuff + i);
            
            if (flag & CLEAN_S) {
                if (count != 0) {
                    line [count] = '\0';
                    WindowStringPut (con, line, fc, bc, startx, y);

                    count = 0;
                }
            }
            else if (count == 0) {
                startx = x;
                fc = newfc; bc = newbc;

                OUTPUTCHAR
            }
            else {
                if ((fc != newfc) || (bc != newbc)) {
                    line [count] = '\0';
                    WindowStringPut (con, line, fc, bc, startx, y);

                    count = 0; startx = x;
                    fc = newfc; bc = newbc;
                    
                    OUTPUTCHAR
                }
                else {
                    OUTPUTCHAR
                }
            }

            j ++;
        }
        
        if (count != 0) {
            line [count] = '\0';
            WindowStringPut (con, line, fc, bc, startx, y);
        }
    }

    if (bHideCaret) {
        WindowSetCursor (con, con->x, con->y, IsKanji (con, con->x, con->y));
        WindowShowCaret (con->hWnd);
    }
}

void TextRepaintAll (CONINFO *con)
{
    /* force to redraw all the chars */
    llatch (con->flagBuff, con->textSize);
    con->currentScroll = con->scrollLine = 0;
    TextRefresh (con, false);
}


