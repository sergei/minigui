/*
 * $Id: terminal.h 7376 2007-08-16 05:40:27Z xgwang $
 *
 * terminal.h: head file of Terminal module.
 * 
 * KON2 - Kanji ON Console -
 * Copyright (C) 1992-1996 Takashi MANABE (manabe@papilio.tutics.tut.ac.jp)
 * 
 * CCE - Console Chinese Environment -
 * Copyright (C) 1998-1999 Rui He (herui@cs.duke.edu)
 *
 * VCOnGUI - Virtual Console On MiniGUi -
 * Copyright (c) 1999-2002 Wei yongming (ymwei@minigui.org)
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

#ifndef TERM_VCONGUI_H
  #define TERM_VCONGUI_H
 
#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

BOOL TerminalStart (PCONINFO con, PCHILDINFO pChildInfo);
void TerminalCleanup (PCONINFO con);

void ReadMasterPty (PCONINFO con);
void HandleInputChar (PCONINFO con, WPARAM wParam, LPARAM lParam);
void HandleInputKeyDown (PCONINFO con, WPARAM wParam, LPARAM lParam);
void HandleInputKeyUp (PCONINFO con, WPARAM wParam, LPARAM lParam);

void HandleMouseLeftDownWhenCaptured (PCONINFO con,int x,int y, WPARAM wParam);
void HandleMouseMoveWhenCaptured (PCONINFO con, int x, int y, WPARAM wParam);
void HandleMouseLeftUpWhenCaptured (PCONINFO con, int x, int y, WPARAM wParam);
void HandleMouseBothDown (PCONINFO con, int x, int y, WPARAM wParam);
void HandleMouseRightUp (PCONINFO con, int x, int y, WPARAM wParam);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif // TERM_VCONGUI_H 

