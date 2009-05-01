/***************************************************************************
 *            ial_tomtom_ts.h
 *
 *  Fri Jan  4 20:11:12 2008
 *  Copyright  2008  nullpointer
 *  Email
 ****************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#ifndef GUI_IAL_TOMTOM_TS_H
#define GUI_IAL_TOMTOM_TS_H

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

BOOL InitTomtomTSInput (INPUT* input, const char* mdev, const char* mtype);
void TermTomtomTSInput (void);

#ifdef __cplusplus
}
#endif	/* __cplusplus */

#endif	/* GUI_IAL_TOMTOM_TS_H */
