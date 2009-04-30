/*
 * $Id: key.h 7376 2007-08-16 05:40:27Z xgwang $
 *
 * key.h: head file of Key handling module.
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

#ifndef KEY_VCONGUI_H
  #define KEY_VCONGUI_H
 
#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define VC_XLATE        0x0000   /* translate keycodes using keymap */
#define VC_MEDIUMRAW    0x0001   /* medium raw (keycode) mode */
#define VC_RAW          0x0002   /* raw (scancode) mode */
#define VC_UNICODE      0x0004   /* Unicode mode */

#define VC_APPLIC       0x0010   /* application key mode */
#define VC_CKMODE       0x0020   /* cursor key mode */
#define VC_REPEAT       0x0040   /* keyboard repeat */
#define VC_CRLF         0x0080   /* 0 - enter sends CR, 1 - enter sends CRLF */
#define VC_META         0x0100   /* 0 - meta, 1 - meta=prefix with ESC */

typedef struct _key_info
{
    DWORD state;
    DWORD oldstate;

    int npadch;
    unsigned char diacr;
	int dead_key_next;
    
    unsigned char* buff;
    int  pos;

    DWORD kbd_mode;
}key_info;

void handle_scancode_on_keydown (int scancode, key_info* kinfo);
void handle_scancode_on_keyup   (int scancode, key_info* kinfo);
void init_key_info  (key_info* kinfo);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* KEY_VCONGUI_H */

