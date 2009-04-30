/*
** $Id: auto.c 7335 2007-08-16 03:38:27Z xgwang $
**
** auto.c: Automatic Input Engine
** 
** Copyright (C) 2004 ~ 2007 Feynman Software
**
** Created by Wei Yongming, 2004/01/29
*/

#include <stdio.h>
#include <stdlib.h>

#include "common.h"

#ifdef _AUTO_IAL

#include "minigui.h"
#include "misc.h"
#include "ial.h"

#define IDF_MOUSE       0x1000

#define IDF_MLDOWN      IAL_MOUSE_LEFTBUTTON
#define IDF_MRDOWN      IAL_MOUSE_RIGHTBUTTON
#define IDF_MMDOWN      IAL_MOUSE_MIDDLEBUTTON

#define IDF_KEY         0x2000
#define IDF_KEYDOWN     0x0100

static struct _IN_DATA
{
    unsigned short flags;
    unsigned short mouse_x;
    unsigned short mouse_y;
    unsigned short key_code;
} in_data [] = 
{
    {IDF_MOUSE,                 160, 160, 0},
    {IDF_MOUSE | IDF_MLDOWN,    160, 160, 0},
    {IDF_MOUSE,                 160, 160, 0},
    {IDF_MOUSE,                 0, 0, 0},
    {IDF_KEY | IDF_KEYDOWN,     0, 0, SCANCODE_A},
    {IDF_KEY,                   0, 0, SCANCODE_A},
    {IDF_KEY | IDF_KEYDOWN,     0, 0, SCANCODE_B},
    {IDF_KEY,                   0, 0, SCANCODE_B},
    {IDF_KEY | IDF_KEYDOWN,     0, 0, SCANCODE_C},
    {IDF_KEY,                   0, 0, SCANCODE_C},
};

static int cur_index;

/************************  Low Level Input Operations **********************/
/*
 * Mouse operations -- Event
 */
static int mouse_update (void)
{
    return 1;
}

static void mouse_getxy (int *x, int* y)
{
    *x = in_data [cur_index].mouse_x;
    *y = in_data [cur_index].mouse_y;
}

static int mouse_getbutton (void)
{
    int buttons = 0;

    if (in_data [cur_index].flags & IDF_MLDOWN)
        buttons |= IAL_MOUSE_LEFTBUTTON;
    if (in_data [cur_index].flags & IDF_MRDOWN)
        buttons |= IAL_MOUSE_RIGHTBUTTON;
    if (in_data [cur_index].flags & IDF_MMDOWN)
        buttons |= IAL_MOUSE_MIDDLEBUTTON;

    return buttons;
}

static unsigned char kbd_state [NR_KEYS];

static int keyboard_update (void)
{
    if (in_data [cur_index].flags & IDF_KEY) {
        if (in_data [cur_index].flags & IDF_KEYDOWN)
            kbd_state [in_data [cur_index].key_code] = 1;
        else
            kbd_state [in_data [cur_index].key_code] = 0;
    }

    return in_data [cur_index].key_code + 1;
}

static const char* keyboard_getstate (void)
{
    return ( const char*) kbd_state;
}

#ifdef _LITE_VERSION
static int wait_event (int which, int maxfd, fd_set *in, fd_set *out, fd_set *except,
                struct timeval *timeout)
{
    int    retvalue;
    int    e;
    static int index = 0;

    e = select (maxfd + 1, in, out, except, timeout) ;
    if (e < 0) {
        __mg_os_time_delay (300);
        return -1;
    }

    if (in_data [index].flags & IDF_MOUSE) {
        retvalue = IAL_MOUSEEVENT;
    }
    else {
        retvalue = IAL_KEYEVENT;
    }

    cur_index = index;
    index ++;
    if (index == TABLESIZE (in_data))
        index = 0;

    return retvalue;
}

#else
static int wait_event (int which, fd_set *in, fd_set *out, fd_set *except,
                struct timeval *timeout)
{
    int    retvalue;
    static int index = 0;

    __mg_os_time_delay (300);

    if (in_data [index].flags & IDF_MOUSE) {
        retvalue = IAL_MOUSEEVENT;
    }
    else {
        retvalue = IAL_KEYEVENT;
    }

    cur_index = index;
    index ++;
    if (index == TABLESIZE (in_data))
        index = 0;

    return retvalue;
}

#endif

BOOL InitAutoInput (INPUT* input, const char* mdev, const char* mtype)
{
    input->update_mouse = mouse_update;
    input->get_mouse_xy = mouse_getxy;
    input->set_mouse_xy = NULL;
    input->get_mouse_button = mouse_getbutton;
    input->set_mouse_range = NULL;
    input->suspend_mouse= NULL;
    input->resume_mouse = NULL;

    input->update_keyboard = keyboard_update;
    input->get_keyboard_state = keyboard_getstate;
    input->suspend_keyboard = NULL;
    input->resume_keyboard = NULL;
    input->set_leds = NULL;

    input->wait_event = wait_event;

    return TRUE;
}

void TermAutoInput (void)
{
}

#endif /* _AUTO_IAL */

