/*
** $Id: vt.c 7376 2007-08-16 05:40:27Z xgwang $
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
** Console driver handling, may have a look at Linux kernel source code: 
** /usr/src/linux/drivers/char/console.c
**
** The below code doesn't handle all of the ANSI required ESC stuff
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
#include <sys/types.h>
#include <sys/vt.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <fcntl.h>
#include <signal.h>
#include <termio.h>

#include "common.h"
#include "minigui.h"
#include "gdi.h"
#include "window.h"
#include "control.h"

#include "vcongui.h"
#include "defs.h"
#include "vc.h"
#include "child.h"
#include "terminal.h"
#include "vt.h"

#define CHAR_NUL    '\x00'
#define CHAR_BEL    '\x07'
#define CHAR_BS     '\x08'
#define CHAR_HT     '\x09'
#define CHAR_LF     '\x0A'
#define CHAR_VT     '\x0B'
#define CHAR_FF     '\x0C'
#define CHAR_CR     '\x0D'
#define CHAR_SO     '\x0E'
#define CHAR_SI     '\x0F'
#define CHAR_XON    '\x11'
#define CHAR_XOFF   '\x12'
#define CHAR_CAN    '\x18'
#define CHAR_SUB    '\x1A'
#define CHAR_ESC    '\x1B'
#define CHAR_DEL    '\x7F'
#define CHAR_CSI    '\x9B'
#define CHAR_SS2    '\x8E'

#define LEN_REPORT    9

/*  Call the routines in vc.c
    TextInsertChar/TextDeleteChar
    ScrollUp/ScrollDown
    TextSput/TextWput
    TextMoveUp/TextMoveDown
    TextClearAll/TextClearEol/TextClearEos
*/

/****************************************************************************
 *                         SaveAttr/RestoreAttr                             *
 ****************************************************************************/
static void SaveAttr(CONINFO *con)
{
    struct attrStack *tmp;

    tmp = (struct attrStack *)malloc(sizeof(struct attrStack));
    if (con->saveAttr)
        tmp->prev = con->saveAttr;
    else
        tmp->prev = NULL;
        
    con->saveAttr = tmp;
    con->saveAttr->x = con->x;
    con->saveAttr->y = con->y;
    con->saveAttr->attr = con->attr;
    con->saveAttr->fcol = con->fcol;
    con->saveAttr->bcol = con->bcol;
}

static void RestoreAttr(CONINFO *con)
{
    struct attrStack *tmp;

    if (con->saveAttr) {
        con->x = con->saveAttr->x;
        con->y = con->saveAttr->y;
        con->attr = con->saveAttr->attr;
        con->fcol = con->saveAttr->fcol;
        con->bcol = con->saveAttr->bcol;
        tmp = con->saveAttr;
        con->saveAttr = tmp->prev;
        free(tmp);
    }
}

/**************************************************************************
 *                               EscSetAttr                               *
 **************************************************************************/
static void EscSetAttr (CONINFO *con, int col)
{
    static const u_char table[] = {0, 4, 2, 6, 1, 5, 3, 7};
    u_char    swp;
    
    switch(col) {
    case 0:    /* off all attributes */
        con->bcol = 0;
        con->fcol = 7;
        con->attr = 0;
        break;
        
    case 1:  // highlight, fcolor
        con->attr |= ATTR_HIGH;
        con->fcol |= 8;
        break;
        
    case 21:  // clear highlight
        con->attr &= ~ATTR_HIGH;
        con->fcol &= ~8;
        break;
        
    case 4:    // underline, bcolor
        con->attr |= ATTR_ULINE;
        con->bcol |= 8;
        break;

    case 24:   // clear underline
        con->attr &= ~ATTR_ULINE;
        con->bcol &= ~8;
        break;
        
    case 7:  // reverse
        if (!(con->attr & ATTR_REVERSE)) {
            con->attr |= ATTR_REVERSE;
            swp = con->fcol & 7;
            
            if (con->attr & ATTR_ULINE)
                swp |= 8;
                
            con->fcol = con->bcol & 7;
            if ((con->attr & ATTR_HIGH) && con->fcol)
                con->fcol |= 8;
            con->bcol = swp;
        }
        break;
        
    case 27:  // clear reverse
        if (con->attr & ATTR_REVERSE) {
            con->attr &= ~ATTR_REVERSE;
            swp = con->fcol & 7;
            if (con->attr & ATTR_ULINE)
                swp |= 8;
            con->fcol = con->bcol & 7;
            if ((con->attr & ATTR_HIGH) && con->fcol)
                con->fcol |= 8;
            con->bcol = swp;
        }
        break;
        
    default:
        if (col >= 30 && col <= 37)  // assign foreground color
        {
            swp = table[col - 30];
            if (con->attr & ATTR_REVERSE) {
                if (con->attr & ATTR_ULINE)
                    swp |= 8;
                con->bcol = swp;
            } 
            else {
                if (con->attr & ATTR_HIGH)
                    swp |= 8;
                con->fcol = swp;
            }
        } 
        else if (col >= 40 && col <= 47)  // assign background color
        {
            swp = table[col - 40];
            if (con->attr & ATTR_REVERSE) {
                if (con->attr & ATTR_HIGH)
                    swp |= 8;
                con->fcol = swp;
            }
            else {
                if (con->attr & ATTR_ULINE)
                    swp |= 8;
                con->bcol = swp;
            }
        }
        break;

    }
}

static void VtSetMode (CONINFO *con, u_char mode, bool sw)
{
    switch(mode) {
    case 4:
        con->ins = sw;
        break;

    case 25:
        con->cursorsw = sw;
        break;
    }
}

static void EscReport (CONINFO *con, u_char mode, u_short arg)
{
    char report[LEN_REPORT];
    
    switch(mode) {
    case 'n':
        if (arg == 6) {
            int x = (con->x < con->xmax) ? con->x : con->xmax;
            int y = (con->y < con->ymax) ? con->y : con->ymax;
            
            sprintf(report, "\x1B[%d;%dR", y + 1, x + 1);
        }
        else if (arg == 5)
            strcpy(report, "\x1B[0n\0");
        break;
    case 'c':
        if (arg == 0)
            strcpy(report, "\x1B[?6c\0");
        break;
    }

    write (con->masterPty, report, strlen (report));
}

/* ESC [ ..... */
static void EscBracket (CONINFO *con, u_char ch)
{
    u_char    n;

    if (ch >= '0' && ch <= '9') {
        con->varg[con->narg] = (con->varg[con->narg] * 10) + (ch - '0');
    }
    else if (ch == ';') {
        if (con->narg < MAX_NARG)
            con->narg ++;
        else
            con->esc = NULL;
    }
    else {
        con->esc = NULL;
        switch (ch) 
        {
        case 'K':
            TextClearEol (con, con->varg[0]);
            break;
            
        case 'J':
            TextClearEos (con, con->varg[0]);
            break;

        // ESC[%dX - I can't find it in /etc/termcap, but dialog-based
        // program used it. - holly
        case 'X':
            TextClearChars (con, con->varg[0]);
            break;
            
        case 'A':
            con->y -= con->varg[0] ? con->varg[0]: 1;
            if (con->y < con->ymin) {
                con->scroll -= con->y - con->ymin;
                con->y = con->ymin;
            }
            break;
            
        case 'B':
            con->y += con->varg[0] ? con->varg[0]: 1;
            if (con->y > con->ymax) {
                con->scroll += con->y - con->ymin;
                con->y = con->ymax;
            }
            break;
            
        case 'C':
            con->x += con->varg[0] ? con->varg[0]: 1;
            con->wrap = false;
            break;
            
        case 'D':
            con->x -= con->varg[0] ? con->varg[0]: 1;
            con->wrap = false;
            break;
            
        case 'G':
            con->x = con->varg[0] ? con->varg[0] - 1: 0;
            con->wrap = false;
            break;
            
        case 'P':
            TextDeleteChar (con, con->varg[0] ? con->varg[0]: 1);
            break;
            
        case '@':
            TextInsertChar (con, con->varg[0] ? con->varg[0]: 1);
            break;
            
        case 'L':
            TextMoveDown (con, con->y, con->ymax, con->varg[0]?con->varg[0]:1);
            break;
            
        case 'M':
            TextMoveUp (con, con->y, con->ymax, con->varg[0]?con->varg[0]:1);
            break;
            
        case 'H':
        case 'f':
            if (con->varg[1])
                con->x = con->varg[1] - 1;
            else 
                con->x = 0;
            con->wrap = false;
            
        case 'd':
            con->y = con->varg[0] ? con->varg[0] - 1: 0;
            break;
            
        case 'm':
            for (n = 0; n <= con->narg; n ++)
                EscSetAttr (con, con->varg[n]);
            break;
            
        case 'r':
            con->ymin = con->varg[0]? (con->varg[0] - 1): 0;
            con->ymax = con->varg[1]? (con->varg[1] - 1): (con->dispymax - 1);
            con->x = 0;
            con->y = con->ymin;
            con->wrap = false;
#if 1
            if (con->ymin || (con->ymax != con->dispymax - 1))
                con->soft = true;
            else
                con->soft = false;
#endif
            break;
            
        case 'l':
            for (n = 0; n <= con->narg; n ++)
                VtSetMode (con, con->varg[n], false);
            break;
            
        case 'h':
            for (n = 0; n <= con->narg; n ++)
                VtSetMode (con, con->varg[n], true);
            break;
            
        case '?':
            con->question = true;
            con->esc = EscBracket;
            break;
            
        case 's':
            SaveAttr (con);
            break;
            
        case 'u':
            RestoreAttr (con);
            break;
            
        case 'n':
        case 'c':
            if (con->question != true)
                EscReport (con, ch, con->varg[0]);
            break;
            
        case 'R':
            break;
        }

        if (con->esc == NULL) {
            con->question = false;
            con->narg = 0;
            for ( n = 0; n < MAX_NARG; n++ )
                con->varg[n] = 0;
        }
    }
}

CODINGINFO SCodingInfo [] = {
    /* latin1(French, Spanish, ...) */
    {"ISO8859-1", 'B', 'A'},
    /* latin2 */
    {"ISO8859-2", 'B', 'B'},
    /* latin3 */
    {"ISO8859-3", 'B', 'C'},
    /* latin4 */
    {"ISO8859-4", 'B', 'D'},
    /* Russian */
    {"ISO8859-5", 'B', 'L'},
    /* Arabic */
    {"ISO8859-6", 'B', 'G'},
    /* Greek */
    {"ISO8859-7", 'B', 'F'},
    /* Hebrew */
    {"ISO8859-8", 'B', 'H'},
    /* latin5 */
    {"ISO8859-9", 'B', 'M'},
    /* Japanese */
    {"JISX0201.1976-0", 'J', 'I'},
    /* DUMB */
    {NULL, 0, 0}
};

CODINGINFO DCodingInfo [] = {
    /* DF_GB2312 */
    {"GB2312.1980-0", 'A', 0},
    /* DF_JISX0208 */
    {"JISX0208.1983-0", 'B', 0},
    /* DF_KSC5601 */
    { "KSC5601.1987-0", 'C', 0},
    /* DF_JISX0212 */
    {"JISX0212", 'D', 0, },
    /* DF_BIG5_0 */
    {"BIG5.HKU-0", '0', 0},
    /* DF_BIG5_1 */
    {"BIG5.HKU-0", '1', 0},
    /* DUMB */
    {NULL, 0, 0}
};

static void EscSetDCodeG0 (CONINFO *con, u_char ch)
{
    int i = 0;
    
    switch(ch) {
    case '(': /* EscSetDCodeG0 */
    case ')': /* EscSetDCodeG1 */
        return;
        
    case '@':
        ch = 'B';
        
    default:
    	while (DCodingInfo [i].sign0) {
	        if (DCodingInfo[i].sign0 == ch) {
		        con->db = (u_char)i|LATCH_1;
		        break;
	        }
	        i ++;
	    }
	    con->trans = CS_DBCS;
	    break;
    }
    con->esc = NULL;
}

static void EscSetSCodeG0 (CONINFO *con, u_char ch)
{
    int i = 0;
    
    switch(ch) {
    case 'U':
        con->g[0] = CS_GRAPH;
        break;
        
    default:
    	while (SCodingInfo [i].sign0) {
	        if (SCodingInfo [i].sign0 == ch) {
		        con->sb = (u_char)i;
		        con->g[0] = CS_LEFT;
		        break;
	        }
            else if (SCodingInfo [i].sign1 == ch) {
		        con->sb = (u_char)i;
		        con->g[0] = CS_RIGHT;
		        break;
	        }
	        i ++;
	    }
    }

    con->trans = con->g[0];
    con->esc = NULL;
}

static void EscSetSCodeG1 (CONINFO *con, u_char ch)
{
    switch(ch) {
    case 'U':
        con->g[1] = CS_LEFT;
        break;
        
    case '0':
        con->g[1] = CS_GRAPH;
        break;
        
    case 'A':
    case 'J':
    case 'B':
        break;
    }
    
    con->trans = con->g[1];
    con->esc = NULL;
}

/* Escape code handling */
static void EscStart (CONINFO *con, u_char ch)
{
    con->esc = NULL;
    
    switch (ch) {
    case '[':
        con->esc = EscBracket;
    break;
    
    case '$':/* Set Double Byte Code */
        con->esc = EscSetDCodeG0;
    break;
    
    case '(':/* Set 94 to G0 */
    case ',':/* Set 96 to G0 */
        con->esc = EscSetSCodeG0;
    break;
    
    case ')':/* Set G1 */
        con->esc = EscSetSCodeG1;
    break;
    
    case 'E':
        con->x = 0;
        con->wrap = false;
    case 'D':
        if (con->y == con->ymax)
            con->scroll ++;
        else
            con->y ++;
        break;
        
    case 'M':
        if (con->y == con->ymin)
            con->scroll --;
        else
            con->y --;
        break;

    case 'c':  /* reset terminal */
        con->fcol = 7;
        con->attr = 0;
#ifdef USECCEWAY
        con->knj1 = 0;
#endif
        con->bcol = 0;
        con->wrap = false;
        con->trans = CS_LEFT;
        con->sb = 0;
        con->db = LATCH_1;
        
    case '*':
        con->x = con->y = 0;
        con->wrap = false;
        TextClearAll (con);
        break;
    
    case '7':
        SaveAttr (con);
        break;
        
    case '8':
        RestoreAttr (con);
        con->wrap = false;
        break;
    }
}

#ifdef USECCEWAY
static inline bool iskanji_byte1 (PCONINFO con, u_char c)
{
    switch(con->sysCoding) {
    case CODE_SJIS:
        return (c >=0x81 && c<=0x9F) || (c >=0xE0 && c <=0xFC);
    case CODE_GB:
        return (c >= 0xA1 && c <= 0xFE);
    case CODE_GBK:
        return (c >= 0x81 && c <= 0xFE);
    case CODE_BIG5:
       return (c >=0xa1 && c<=0xfe);  //first byte
    default:
        return (c & 0x80);
    }
}

static inline bool iskanji_byte2 (PCONINFO con, u_char c)
{
    switch(con->sysCoding) {
    case CODE_SJIS:
        return (c >=0x81 && c<=0x9F) || (c >=0xE0 && c <=0xFC);
    case CODE_GB:
        return (c >= 0xA1 && c <= 0xFE);
    case CODE_GBK:
        return (c >= 0x40 && c <= 0xFE);
    case CODE_BIG5:
       return ((c >=0x40 && c<=0x7e) || (c >=0xa1 && c<=0xfe)); // second byte
    default:
        return (c & 0x80);
    }
}
#endif

static inline bool iskanji (PCONINFO con, u_char c, u_char c1)
{
    switch(con->sysCoding) {
    case CODE_SJIS:
        return (c >= 0x81 && c<= 0x9F && c >= 0xE0 && c <= 0xFC);
    case CODE_GB:
        return (c >= 0xA1 && c <= 0xFE && c1 >= 0xA1 && c1 <= 0xFE);
    case CODE_GBK:
        return (c >= 0x81 && c <= 0xFE && c1 >= 0x40 && c1 <= 0xFE);
    case CODE_BIG5:
       return ((c >=0xa1 && c<=0xfe) 
                && ((c >=0x40 && c<=0x7e) || (c >=0xa1 && c<=0xfe)));
    default:
        return (c & 0x80);
    }
}

static inline u_int text_offset (CONINFO *con, u_int x, u_int y)
{
    return (con->textHead + x + y * con->dispxmax) % con->textSize;
}

int FirstOrSecondByte (CONINFO* con, u_char ch)
{
    u_int prev_addr = -1;
    
    if (con->x == 0) {
        if (con->y > con->ymin) {
            prev_addr = text_offset (con, con->xmax, con->y - 1);
        }
    }
    else
        prev_addr = text_offset (con, con->x - 1, con->y);

    if (prev_addr != -1) {
        if (con->flagBuff [prev_addr] & CODEIS_1)
            return CODEIS_2;
        else
            return CODEIS_1;
    }

    return CODEIS_1;
}

void RearrangeLineWhenInsSB (CONINFO* con)
{
    u_int addr;
    int i;
    int prev = CODEIS_S;
    u_char ch, ch1;
    
    addr = text_offset (con, con->x, con->y) + 1;

    for (i = 0; i < con->xmax - con->x - 1; i++) {
        ch = con->textBuff [addr + i];
        ch1 = con->textBuff [addr + i + 1];
        if (iskanji (con, ch, ch1)) {
            switch (prev) {
            case CODEIS_S:
            case CODEIS_2:
                con->flagBuff [addr + i] = con->db | LATCH_1;
                prev = CODEIS_1;
                break;
            case CODEIS_1:
                con->flagBuff [addr + i] = con->db | LATCH_2;
                prev = CODEIS_1;
                break;
            }
        }
        else if (ch) {
            con->flagBuff [addr + i] = con->sb | LATCH_S;
            prev = CODEIS_S;
        }
        else
            prev = CODEIS_S;
    }
}

/****************************************************************************
 *               very important interface, output the buff                  *
 ****************************************************************************/
void VtWrite (CONINFO *con, const char *buff, int nchars)
{
    u_char    ch, ch1;

  /* This is all the esc sequence strings */
#if 0
    {
    FILE *fff;
    int i;

    fff = fopen("/tmp/ccegb-esc.log", "a");
    for(i = 0; i < nchars; i++)
        fprintf (fff, "%c", buff[i]);
        fprintf (fff," (%d, %d)\n", con->x, con->y);
	fclose (fff);
    }
#endif

    while (nchars-- > 0) {
        ch = *buff++;
        ch1 = *buff;
        
        if (!ch)
            continue;

        if (con->esc) 
            con->esc (con, ch);   // in escape mode, parse the parameters
        else {
            switch (ch)
            {
            case CHAR_BEL:  // 0x07
                Ping ();
                break;
                
            case CHAR_DEL:  // 0x7F
                break;
                
            case CHAR_BS:   // 0x08
                if (con->x)
                    con->x--;
                con->wrap = false;
            break;
            
            case CHAR_HT:  //0x09
                con->x += con->tab - (con->x % con->tab);
                con->wrap = false;
                if (con->x > con->xmax)
                    con->x -= con->xmax + 1;
                else
                    break;
                    
            case CHAR_VT:  // 0x0B
            case CHAR_FF:  // 0x0C
#if 1
                con->trans = CS_LEFT;
                con->sb = 0;
                con->db = LATCH_1;
                /* no break? */
#endif

            case CHAR_LF:   // 0x0A
                con->wrap = false;
                if (con->y == con->ymax)
                    con->scroll ++;
                else
                    con->y ++;
                break;
                
            case CHAR_CR:  // 0x0D
                con->x = 0;
                con->wrap = false;
                break;
                
            case CHAR_ESC:  // 0x1B
                con->esc = EscStart;
                continue;
                
            case CHAR_SO:  // 0x0E
                con->trans = con->g[1] | G1_SET;
                continue;
                
            case CHAR_SI:  // 0x0F
                con->trans = con->g[0];
                continue;
            
            default:
                if (con->x == con->xmax + 1) {
                    con->wrap = true;
                    con->x --;
                }

                if (con->wrap) {
                    con->x -= con->xmax;
                    if (con->y == con->ymax)
                        con->scroll ++;
                    else
                        con->y ++;
                    con->wrap = false;
                    buff --;
                    nchars ++;

#ifdef USECCEWAY
                    /* Upate position of second half of the double-byte char */
                    if (con->knj1) {
                        con->knj1x = con->x;
                        con->knj1y = con->y;
                        con->knj2x = con->x + 1;
                        con->knj2y = con->y;
                    }
#endif
                    break;
                }

#ifndef USECCEWAY
                if (iskanji (con, ch, ch1)) {
                    if (con->ins)
                        TextInsertChar (con, 1);
                    
                    switch (FirstOrSecondByte (con, ch)) {
                    case CODEIS_1:
                        TextWput1 (con, ch);
                        break;
                    case CODEIS_2:
                        TextWput2 (con, ch);
                        break;
                    default:
                        TextSput (con, ch);
                        RearrangeLineWhenInsSB (con);
                        break;
                    }

                    con->x ++;
                    continue;
                }
                else {
                    if (con->ins)
                        TextInsertChar (con, 1);
                        
                    TextSput (con, ch);
                    RearrangeLineWhenInsSB (con);

                    con->x ++;
                    continue;
                }
#else
                if ( con->knj1 && iskanji_byte2 (con, ch)
                  && ( ((con->x == con->knj2x) && (con->y == con->knj2y))
                    || ((con->x == con->knj1x) && (con->y == con->knj1y))
                     )
                   ) {
                    short oldx, oldy;
                    
                    /* special handling for Japanese char */
                    if (con->knj1 & 0x80) {
                        switch(con->sysCoding) 
                        {
                        case CODE_EUC:
                            if (con->knj1 == (u_char)CHAR_SS2) {
                                /* handling 'kata-kana' */
                                if (con->ins)
                                    TextInsertChar (con, 1);
                                TextSput (con, ch);
                                con->x ++;
                                con->knj1 = 0;
                                continue;
                            }
                            con->knj1 &= 0x7F;
                            ch &= 0x7F;
                            break;
                            
                        case CODE_SJIS:
                            sjistojis (con->knj1, ch);
                            break;
                        }  // case
                    }
                    else {
                        if (con->db == (DF_BIG5_0|LATCH_1))
                            muletobig5 (con->db, con->knj1, ch);
                    }
                    
                    oldx = con->x; oldy = con->y;
                    con->x = con->knj1x; con->y = con->knj1y;

                    if (con->ins)
                        TextInsertChar (con, 2);
                        
                    TextWput (con, con->knj1, ch);
                    
                    con->x = oldx + 2;
                    con->y = oldy;
                    con->knj1 = 0;
                    continue;
                }
                else if (con->trans == CS_DBCS
                       || (iskanji (con, ch, ch1) && con->trans == CS_LEFT)) {
                    if (con->x == con->xmax)
                        con->wrap = true;
                    else {
                        con->knj1x = con->x;
                        con->knj1y = con->y;

                        con->knj2x = con->x + 1;
                        con->knj2y = con->y;
                    }

                    /* First Half of the double-byte char */
                    con->knj1 = ch;
                    continue;
                }
                else {
                    if (con->ins)
                        TextInsertChar (con, 1);
                    TextSput (con, con->trans == CS_RIGHT ? ch | 0x80: ch);
                    con->x ++;
                    continue;
                }
#endif

            }  /* switch */
        }

        if (con->scroll > 0) 
            ScrollUp (con, con->scroll);
        else if (con->scroll < 0)
            ScrollDown (con, -con->scroll);
        con->scroll = 0;
    }  /* while */
}

bool VtStart (PCONINFO con)
{
    const char* charset;

   /* A serious and strange bug here, at first ymin and knj1 is not
      initialized to 0, it works fine in Linux (maybe the malloc in
      Linux will automatically zero the memory), but doesn't work in
      FreeBSD. FreeBSD's calloc zero the memory, but malloc not!!!
      So I need to initialize all the member here */

    con->x = con->y = 0;
    con->dispxmax = con->cols;
    con->dispymax = con->rows;
    con->xmax = con->cols - 1;
    con->ymax = con->rows - 1;
    con->ymin = 0;
    con->tab = 8;
    con->fcol = 7;
    con->bcol = 0;
    con->attr = 0;
    con->esc = NULL;
    con->g[0] = con->g[1] = CS_LEFT;
    con->trans = CS_LEFT;
    con->soft = con->ins = con->wrap = false;
    con->sb = 0;
    con->db = LATCH_1;

#ifdef USECCEWAY
    con->knj1 = 0;
    con->knj1x = con->knj1y = 0;
#endif

    con->cursorsw = true;

    con->textBuff = (u_char *)calloc (con->cols, con->rows);
    con->attrBuff = (u_char *)calloc (con->cols, con->rows);
    con->flagBuff = (u_char *)calloc (con->cols, con->rows);
    if (con->textBuff == NULL || con->attrBuff == NULL ||
        con->flagBuff == NULL)
        return false;
    
    con->textSize = con->cols * (con->rows);

    con->currentScroll = con->scrollLine = 0;
    con->scroll = 0;
    con->textHead = 0;

    con->textClear = false;
    con->terminate = 0;

    if (con->log_font == NULL) {
        PLOGFONT sys_font = GetSystemFont (SYSLOGFONT_WCHAR_DEF);
        if (sys_font->mbc_devfont)
            charset = sys_font->mbc_devfont->charset_ops->name;
        else 
            charset = sys_font->sbc_devfont->charset_ops->name;
    }
    else {
        if (con->log_font->mbc_devfont)
            charset = con->log_font->mbc_devfont->charset_ops->name;
        else 
            charset = con->log_font->sbc_devfont->charset_ops->name;
    }

    con->sysCoding = CODE_GB;
    if (strcmp (charset, FONT_CHARSET_GBK) == 0)
        con->sysCoding = CODE_GBK;
    else if (strcmp (charset, FONT_CHARSET_BIG5) == 0)
        con->sysCoding = CODE_BIG5;

    memset (con->varg, MAX_NARG, 0);
    con->narg = 0;
    con->question = 0;

    con->m_captured = false;
    con->m_oldx = con->m_oldy = 0;
    con->m_origx = con->m_origy = -1;

    return true;
}

bool VtChangeWindowSize (PCONINFO con, int new_row, int new_col)
{
    int i, j;
    int text_size = new_row * new_col;
    char* textBuff, *attrBuff, *flagBuff;

    textBuff = (u_char *)calloc (text_size, 1);
    attrBuff = (u_char *)calloc (text_size, 1);
    flagBuff = (u_char *)calloc (text_size, 1);
    
    if (textBuff == NULL || attrBuff == NULL || flagBuff == NULL)
        return false;
    
    for (i = 0; i < MIN (new_row, con->rows); i++)
        for (j = 0; j < MIN (new_col, con->cols); j++) {
            textBuff [i*new_col + j] = con->textBuff [i*con->cols +j];
            attrBuff [i*new_col + j] = con->attrBuff [i*con->cols +j];
            flagBuff [i*new_col + j] = con->flagBuff [i*con->cols +j];
        }
    
    free (con->textBuff);
    free (con->attrBuff);
    free (con->flagBuff);

    con->rows = new_row;
    con->cols = new_col;
    con->textSize = text_size;
    con->textBuff = textBuff;
    con->attrBuff = attrBuff;
    con->flagBuff = flagBuff;
    con->dispxmax = con->cols;
    con->dispymax = con->rows;
    con->xmax = con->cols - 1;
    con->ymax = con->rows - 1;

    if (con->x > con->xmax) con->x = con->xmax;
    if (con->y > con->ymax) con->y = con->ymax;

    return true;
}

void VtCleanup (PCONINFO con)
{
    if (con == NULL)
        return;

    con->scrollLine = con->textHead = con->currentScroll = 0;
    con->scroll = 0;
    
    SafeFree ((void **)&(con->textBuff));
    SafeFree ((void **)&(con->attrBuff));
    SafeFree ((void **)&(con->flagBuff));
}

