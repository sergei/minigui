/*
 * $Id: vt.h 7376 2007-08-16 05:40:27Z xgwang $
 *
 * vt.h: VT emulator
 * 
 * KON2 - Kanji ON Console -
 * Copyright (C) 1992-1996 Takashi MANABE (manabe@papilio.tutics.tut.ac.jp)
 * 
 * CCE - Console Chinese Environment -
 * Copyright (C) 1998-1999 Rui He (herui@cs.duke.edu)
 *
 * VCOnGUI - Virtual Console On MiniGUi -
 * Copyright (c) 1999-2002 Wei Yongming (ymwei@minigui.org)
 * Copyright (C) 2003-2007 Feynman Software
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

#ifndef VT_VCONGUI_H
#define VT_VCONGUI_H

#define CODE_2022   0   /* 2022 のみに従う*/
#define CODE_EUC    1   /* EUC にも従う */
#define CODE_SJIS   2   /* SJIS にも従う */
#define CODE_BIG5   3
#define CODE_GB     4
#define CODE_GBK    5

#define G0_SET      0
#define G1_SET      0x80

typedef struct _CodingInfo 
{
    char *name;
    u_char sign0, sign1;
}CODINGINFO;

void VtInit (void);
bool VtStart (PCONINFO con);
void VtWrite (PCONINFO con, const char*, int nchars);
void VtCleanup (PCONINFO con);
bool VtChangeWindowSize (PCONINFO con, int new_row, int new_col);

#define	sjistojis(ch, cl)\
{\
    ch -= (ch > 0x9F) ? 0xB1: 0x71;\
    ch = ch * 2 + 1;\
    if (cl > 0x9E) {\
	cl = cl - 0x7E;\
	ch ++;\
    } else {\
	if (cl > 0x7E) cl --;\
	cl -= 0x1F;\
    }\
}

#define	jistosjis(ch, cl)\
{\
    if (ch & 1) cl = cl + (cl > 0x5F ? 0x20:0x1F);\
    else cl += 0x7E;\
    ch = ((ch - 0x21) >> 1) + 0x81;\
    if (ch > 0x9F) ch += 0x40;\
}

/*
  derived from Mule:codeconv.c to support "ESC $(0" sequence
  thanks to K.Handa <handa@etl.go.jp>
  */

#define muletobig5(type, m1, m2)\
{\
    unsigned code = (m1 - 0x21) * 94 + (m2 - 0x21);\
\
    if (type == DF_BIG5_1) code += 0x16F0;\
    m1 = code / 157 + 0xA1;\
    m2 = code % 157;\
    m2 += m2 < 0x3F ? 64 : 98;\
}

enum {
    DF_GB2312,
    DF_JISX0208,
    DF_KSC5601,
    DF_JISX0212,
    DF_BIG5_0,
    DF_BIG5_1
};

#endif

