/*
** $Id: key.c 7376 2007-08-16 05:40:27Z xgwang $
**
** key.c: scancode to key, use Linux's default keymap.
** 
** Some code from Linux Kernel.
**
** VCOnGUI - Virtual Console On MiniGUi -
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
#include <sys/types.h>

#include <linux/types.h>
#include <linux/keyboard.h>
#include <linux/kd.h>

#include "common.h"

#include "key.h"

extern ushort **key_maps;
extern struct kbdiacr *accent_table;
extern unsigned int accent_table_size;
extern char **func_table;

typedef void (*k_hand) (unsigned char value, key_info* kinfo);
typedef void (k_handfn) (unsigned char value, key_info* kinfo);

static k_handfn
    do_self, do_fn, do_spec, do_pad, do_dead, do_cur, do_shift,
    do_meta, do_ascii, do_dead2;

static k_hand key_handler[16] = {
    do_self, do_fn, do_spec, do_pad, do_dead, NULL, do_cur, NULL,
    do_meta, do_ascii, NULL, NULL, NULL, do_dead2,
    NULL, NULL 
};

typedef void (*void_fnp) (key_info* kinfo);
typedef void (void_fn) (key_info* kinfo);

static void_fn enter, compose;

static void_fnp spec_fn_table[] = {
    NULL,    enter,    NULL,       NULL,
    NULL,    NULL,     NULL,       NULL,
    NULL,    NULL,     NULL,       NULL,
    NULL,    NULL,     compose,    NULL,
    NULL,    NULL,     NULL,       NULL
};

static inline void put_queue (char ch, key_info* kinfo)
{
    kinfo->buff [kinfo->pos] = ch;
    kinfo->pos ++;
}

static inline void puts_queue (char* cp, key_info* kinfo)
{
    while (*cp) {
        kinfo->buff [kinfo->pos] = *cp;
        kinfo->pos ++;

        cp++;
    }
}

static void applkey (int key, char mode, key_info* kinfo)
{
    static char buf[] = { 0x1b, 'O', 0x00, 0x00 };

    buf[1] = (mode ? 'O' : '[');
    buf[2] = key;
    puts_queue (buf, kinfo);
}

/*
 * Many other routines do put_queue, but I think either
 * they produce ASCII, or they produce some user-assigned
 * string, and in both cases we might assume that it is
 * in utf-8 already.
 */
void to_utf8 (ushort c, key_info* kinfo)
{
    if (c < 0x80)
        put_queue(c, kinfo);                /* 0*******  */
    else if (c < 0x800) {
        put_queue(0xc0 | (c >> 6), kinfo);  /*  110***** 10******  */
        put_queue(0x80 | (c & 0x3f), kinfo);
    } else {
        put_queue(0xe0 | (c >> 12), kinfo); /*  1110**** 10****** 10******  */
        put_queue(0x80 | ((c >> 6) & 0x3f), kinfo);
        put_queue(0x80 | (c & 0x3f), kinfo);
    }
    
    /* UTF-8 is defined for words of up to 31 bits,
       but we need only 16 bits here */
}

void init_key_info  (key_info* kinfo)
{
    kinfo->state = 0;
    kinfo->oldstate = 0;
    kinfo->npadch = -1;
    kinfo->diacr = 0;
    kinfo->dead_key_next = 0;

    kinfo->kbd_mode = VC_XLATE;
}

static int compute_shiftstate (DWORD shiftstate)
{
    int shift_final = 0;

    if (key_maps [1]) {
        if (shiftstate & KS_SHIFT) {
            shift_final += 1 << KG_SHIFT;
        }
    }
    else {
        if (shiftstate & KS_LEFTSHIFT)
            shift_final += 1 << KG_SHIFTL;
        if (shiftstate & KS_RIGHTSHIFT)
            shift_final += 1 << KG_SHIFTR;
    }

    if (key_maps [4]) {
        if (shiftstate & KS_CTRL) {
            shift_final += 1 << KG_CTRL;
        }
    }
    else {
        if (shiftstate & KS_LEFTCTRL)
            shift_final += 1 << KG_CTRLL;
        if (shiftstate & KS_RIGHTCTRL)
            shift_final += 1 << KG_CTRLR;
    }

    if (shiftstate & KS_LEFTALT)
        shift_final += 1 << KG_ALT;
    if (shiftstate & KS_RIGHTALT)
        shift_final += 1 << KG_ALTGR;

    return shift_final;
}

/*
 * Translation of escaped scancodes to keycodes.
 */
void handle_scancode_on_keydown (int scancode, key_info* kinfo)
{
    u_short keysym;
    u_char type;
    int shift_final;
    ushort *key_map;
    
    shift_final = compute_shiftstate (kinfo->state);

    key_map = key_maps [shift_final];
    if (key_map != NULL) {
        keysym = key_map [scancode];
        type = HIBYTE (keysym);

        if (type >= 0xf0) {
            type -= 0xf0;
            if (type == KT_LETTER) {
                type = KT_LATIN;
                if (kinfo->state & KS_CAPSLOCK) {
                    key_map = key_maps [shift_final ^ (1<<KG_SHIFT)];
                    if (key_map)
                      keysym = key_map [scancode];
                }
            }
            
            if (key_handler [type])
                (*key_handler [type]) (keysym & 0xff, kinfo);
        }
        else {
            to_utf8 (keysym, kinfo);
        }
    }
}

void handle_scancode_on_keyup (int scancode, key_info* kinfo)
{
    u_short keysym;
    u_char type;
    int shift_final;
    ushort *key_map;
    
    shift_final = compute_shiftstate (kinfo->state);

    key_map = key_maps [shift_final];
    if (key_map != NULL) {
        keysym = key_map [scancode];
        type = HIBYTE (keysym);

        if (type >= 0xf0) {
            type -= 0xf0;
            if (type == KT_SHIFT)
                do_shift (keysym & 0xff, kinfo);
        }
    }
}

#define A_GRAVE  '`'
#define A_ACUTE  '\''
#define A_CFLEX  '^'
#define A_TILDE  '~'
#define A_DIAER  '"'
#define A_CEDIL  ','
static unsigned char ret_diacr[NR_DEAD] =
    {A_GRAVE, A_ACUTE, A_CFLEX, A_TILDE, A_DIAER, A_CEDIL };

/*
 * We have a combining character DIACR here, followed by the character CH.
 * If the combination occurs in the table, return the corresponding value.
 * Otherwise, if CH is a space or equals DIACR, return DIACR.
 * Otherwise, conclude that DIACR was not combining after all,
 * queue it and return CH.
 */
unsigned char handle_diacr (unsigned char ch, key_info* kinfo)
{
    int d = kinfo->diacr;
    int i;

    kinfo->diacr = 0;

    for (i = 0; i < accent_table_size; i++) {
        if (accent_table[i].diacr == d && accent_table[i].base == ch)
            return accent_table[i].result;
    }

    if (ch == ' ' || ch == d)
        return d;

    put_queue (d, kinfo);
    return ch;
}

static void do_dead (unsigned char value, key_info* kinfo)
{
    value = ret_diacr [value];
    do_dead2 (value, kinfo);
}

/*
 * Handle dead key. Note that we now may have several
 * dead keys modifying the same character. Very useful
 * for Vietnamese.
 */
static void do_dead2 (unsigned char value, key_info* kinfo)
{
    kinfo->diacr = (kinfo->diacr ? handle_diacr (value, kinfo) : value);
}

static void do_self (unsigned char value, key_info* kinfo)
{
    if (kinfo->diacr)
        value = handle_diacr (value, kinfo);

    if (kinfo->dead_key_next) {
        kinfo->dead_key_next = 0;
        kinfo->diacr = value;
        return;
    }

    put_queue (value, kinfo);
}

#define SIZE(x) (sizeof(x)/sizeof((x)[0]))

static void do_fn (unsigned char value, key_info* kinfo)
{
    if (value < SIZE (func_table)) {
        if (func_table [value])
            puts_queue (func_table [value], kinfo);
    }
}

static void do_pad(unsigned char value, key_info* kinfo)
{
    static const char *pad_chars = "0123456789+-*/\015,.?()";
    static const char *app_map = "pqrstuvwxylSRQMnnmPQ";

    /* kludge... shift forces cursor/number keys */
    if ( (kinfo->kbd_mode & VC_APPLIC) && !(kinfo->state & KS_SHIFT)) {
        applkey (app_map[value], 1, kinfo);
        return;
    }

    if (!(kinfo->state & KS_NUMLOCK))
        switch (value) {
            case KVAL(K_PCOMMA):
            case KVAL(K_PDOT):
                do_fn (KVAL(K_REMOVE), kinfo);
                return;
            case KVAL(K_P0):
                do_fn (KVAL(K_INSERT), kinfo);
                return;
            case KVAL(K_P1):
                do_fn (KVAL(K_SELECT), kinfo);
                return;
            case KVAL(K_P2):
                do_cur (KVAL(K_DOWN), kinfo);
                return;
            case KVAL(K_P3):
                do_fn (KVAL(K_PGDN), kinfo);
                return;
            case KVAL(K_P4):
                do_cur (KVAL(K_LEFT), kinfo);
                return;
            case KVAL(K_P6):
                do_cur (KVAL(K_RIGHT), kinfo);
                return;
            case KVAL(K_P7):
                do_fn (KVAL(K_FIND), kinfo);
                return;
            case KVAL(K_P8):
                do_cur (KVAL(K_UP), kinfo);
                return;
            case KVAL(K_P9):
                do_fn (KVAL(K_PGUP), kinfo);
                return;
            case KVAL(K_P5):
                applkey ('G', kinfo->kbd_mode & VC_APPLIC, kinfo);
                return;
        }

    put_queue (pad_chars [value], kinfo);
    if (value == KVAL(K_PENTER) && (kinfo->kbd_mode & VC_CRLF))
        put_queue (10, kinfo);
}

static void do_cur (unsigned char value, key_info* kinfo)
{
    static const char *cur_chars = "BDCA";

    applkey (cur_chars [value], kinfo->kbd_mode & VC_CKMODE, kinfo);
}

static void do_meta(unsigned char value, key_info* kinfo)
{
    if (kinfo->kbd_mode & VC_META) {
        put_queue ('\033', kinfo);
        put_queue (value, kinfo);
    } else
        put_queue (value | 0x80, kinfo);
}

static void do_ascii(unsigned char value, key_info* kinfo)
{
    int base;

    if (value < 10)    /* decimal input of code, while Alt depressed */
        base = 10;
    else {       /* hexadecimal input of code, while AltGr depressed */
        value -= 10;
        base = 16;
    }

    if (kinfo->npadch == -1)
        kinfo->npadch = value;
    else
        kinfo->npadch = kinfo->npadch * base + value;
}

static void do_shift (unsigned char value, key_info* kinfo)
{
    /* kludge */
    if ((kinfo->state != kinfo->oldstate) && (kinfo->npadch != -1)) {
        if (kinfo->kbd_mode == VC_UNICODE)
            to_utf8 (kinfo->npadch & 0xffff, kinfo);
        else
            put_queue (kinfo->npadch & 0xff, kinfo);
        kinfo->npadch = -1;
    }
}

static void do_spec(unsigned char value, key_info* kinfo)
{
    if (value >= SIZE(spec_fn_table))
        return;

    if (spec_fn_table [value])
        spec_fn_table [value] (kinfo);
}

static void enter (key_info* kinfo)
{
    if (kinfo->diacr) {
        put_queue (kinfo->diacr, kinfo);
        kinfo->diacr = 0;
    }
    put_queue (13, kinfo);
    
    if (kinfo->kbd_mode & VC_CRLF)
        put_queue (10, kinfo);
}

static void compose (key_info* kinfo)
{
    kinfo->dead_key_next = 1;
}

#if 0
static void send_intr (key_info* kinfo)
{
//  tty_insert_flip_char(tty, 0, TTY_BREAK);
}

static void scroll_forw (key_info* kinfo)
{
    // do nothing.
//  scrollfront(0);
}

static void scroll_back (key_info* kinfo)
{
    // do nothing.
//  scrollback(0);
}

static void boot_it (key_info* kinfo)
{
    // do nothing.
//  ctrl_alt_del();
}

#endif
