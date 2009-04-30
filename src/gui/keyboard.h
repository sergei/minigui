/*
 * $Id: keyboard.h 1989 2003-04-11 09:41:19Z weiym $
 *
 * key.h: head file of Key handling module.
 * 
 * Copyright (C) 2003 Feynman Software.
 *
 */

#ifndef GUI_KEYBOARD_H
  #define GUI_KEYBOARD_H
 
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
    DWORD kbd_mode;
    DWORD shiftstate;
    DWORD oldstate;

    int npadch;
    unsigned char diacr;
    int dead_key_next;
    
    unsigned char type;
    unsigned char buff[50];
    int  pos;
} key_info;

typedef void (* INIT_KBD_LAYOUT) (ushort*** key_maps_p, struct kbdiacr** accent_table_p,
                unsigned int* accent_table_size_p, char*** func_table_p);

typedef struct kbd_layout_info 
{
    char* name;
    INIT_KBD_LAYOUT init;
} kbd_layout_info;

void init_default_kbd_layout (ushort*** key_maps_p, struct kbdiacr** accent_table_p, 
                unsigned int* accent_table_size_p, char*** func_table_p);

#ifdef _KBD_LAYOUT_FRPC
void init_frpc_kbd_layout (ushort*** key_maps_p, struct kbdiacr** accent_table_p, 
                unsigned int* accent_table_size_p, char*** func_table_p);
#endif

#ifdef _KBD_LAYOUT_FR
void init_fr_kbd_layout (ushort*** key_maps_p, struct kbdiacr** accent_table_p,
                unsigned int* accent_table_size_p, char*** func_table_p);
#endif

#ifdef _KBD_LAYOUT_DE
void init_de_kbd_layout (ushort*** key_maps_p, struct kbdiacr** accent_table_p,
                unsigned int* accent_table_size_p, char*** func_table_p);
#endif

#ifdef _KBD_LAYOUT_DELATIN1
void init_delatin1_kbd_layout (ushort*** key_maps_p, struct kbdiacr** accent_table_p,
                unsigned int* accent_table_size_p, char*** func_table_p);
#endif

#ifdef _KBD_LAYOUT_IT
void init_it_kbd_layout (ushort*** key_maps_p, struct kbdiacr** accent_table_p,
                unsigned int* accent_table_size_p, char*** func_table_p);
#endif

#ifdef _KBD_LAYOUT_ES
void init_es_kbd_layout (ushort*** key_maps_p, struct kbdiacr** accent_table_p,
                unsigned int* accent_table_size_p, char*** func_table_p);
#endif

#ifdef _KBD_LAYOUT_ESCP850
void init_escp850_kbd_layout (ushort*** key_maps_p, struct kbdiacr** accent_table_p,
                unsigned int* accent_table_size_p, char*** func_table_p);
#endif

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* GUI_KEYBOARD_H */

