/*
 * $Id: error.h 7376 2007-08-16 05:40:27Z xgwang $
 *
 * error.h: head file of Error module.
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
**  This library is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
**  Library General Public License for more details.
**
**  You should have received a copy of the GNU Library General Public
**  License along with this library; if not, write to the Free
**  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
**  MA 02111-1307, USA
*/

#ifndef ERROR_VCONGUI_H
  #define ERROR_VCONGUI_H
 
#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

void Perror (const char *message);       /* perror (message) */
void PerrorExit (const char *message);   /* perror (message) and exit */
void myFatal (const char *format, ...);    /* print error message and exit */
void myError (const char *format, ...);    /* print error message */
void myWarn (const char *format, ...);     /* print warning message */
void myMessage (const char *format, ...);  /* print message */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif // ERROR_VCONGUI_H 

