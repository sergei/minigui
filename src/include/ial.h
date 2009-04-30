/*
** $Id: ial.h 7474 2007-08-28 07:40:45Z weiym $
**
** ial.h: the head file of Input Abstract Layer
**
** Copyright (C) 2003 ~ 2007 Feynman Software.
** Copyright (C) 2000 ~ 2002 Wei Yongming.
**
** Create data: 2000/06/13
*/

#ifndef GUI_IAL_H
    #define GUI_IAL_H


#ifndef HAVE_SYS_TIME_H

#define fd_set void

#else

#ifdef __OSE__
#define fd_set void
#endif

#ifndef __NONEED_FD_SET_TYPE
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#endif

#endif

#include "misc.h"
#include "gal.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define IAL_MOUSE_LEFTBUTTON    4
#define IAL_MOUSE_MIDDLEBUTTON  2
#define IAL_MOUSE_RIGHTBUTTON   1
#define IAL_MOUSE_FOURTHBUTTON  8
#define IAL_MOUSE_FIFTHBUTTON   16
#define IAL_MOUSE_SIXTHBUTTON   32
#define IAL_MOUSE_RESETBUTTON   64

#define IAL_MOUSEEVENT          1
#define IAL_KEYEVENT            2

typedef struct tagINPUT
{
    char*   id;

    // Initialization and termination
    BOOL (*init_input) (struct tagINPUT *input, const char* mdev, const char* mtype);
    void (*term_input) (void);

    // Mouse operations
    int  (*update_mouse) (void);
    void (*get_mouse_xy) (int* x, int* y);
    void (*set_mouse_xy) (int x, int y);
    int  (*get_mouse_button) (void);
    void (*set_mouse_range) (int minx, int miny, int maxx, int maxy);
    void (*suspend_mouse) (void);
    int (*resume_mouse) (void);

    // Keyboard operations
    int  (*update_keyboard) (void);
    const char* (*get_keyboard_state) (void);
    void (*suspend_keyboard) (void);
    int (*resume_keyboard) (void);
    void (*set_leds) (unsigned int leds);

    // Event
#ifdef _LITE_VERSION
    int (*wait_event) (int which, int maxfd, fd_set *in, fd_set *out, 
            fd_set *except, struct timeval *timeout);
#else
    int (*wait_event) (int which, fd_set *in, fd_set *out, fd_set *except,
            struct timeval *timeout);
#endif

    char mdev [MAX_PATH + 1];
}INPUT;

extern INPUT* __mg_cur_input;

#ifdef _MISC_MOUSECALIBRATE
typedef void (*DO_MOUSE_CALIBRATE_PROC) (int* x, int* y);
extern DO_MOUSE_CALIBRATE_PROC __mg_do_mouse_calibrate;
extern POINT __mg_mouse_org_pos;
#endif

#define IAL_InitInput           (*__mg_cur_input->init_input)
#define IAL_TermInput           (*__mg_cur_input->term_input)
#define IAL_UpdateMouse         (*__mg_cur_input->update_mouse)

//#define IAL_GetMouseXY          (*__mg_cur_input->get_mouse_xy)
static inline void IAL_GetMouseXY(int *x, int* y) {
#ifdef _COOR_TRANS
    int tmp;
#endif

    if (__mg_cur_input && __mg_cur_input->get_mouse_xy) {
        (*__mg_cur_input->get_mouse_xy) (x, y);
#ifdef _MISC_MOUSECALIBRATE
        __mg_mouse_org_pos.x = *x;
        __mg_mouse_org_pos.y = *y;
#endif
    }

#ifdef _MISC_MOUSECALIBRATE
    if (__mg_do_mouse_calibrate) __mg_do_mouse_calibrate (x, y);
#endif

    if (*x < 0) *x = 0;
    if (*y < 0) *y = 0;

    if (*x > (WIDTHOFPHYSCREEN-1)) *x = (WIDTHOFPHYSCREEN-1);
    if (*y > (HEIGHTOFPHYSCREEN-1)) *y = (HEIGHTOFPHYSCREEN-1);

#ifdef _COOR_TRANS
#if _ROT_DIR_CW
    tmp = *x;
    *x = *y;
    *y = (WIDTHOFPHYSCREEN-1) - tmp;
#else
    tmp = *y;
    *y = *x;
    *x = (HEIGHTOFPHYSCREEN-1) - tmp;
#endif
#endif
}

#define IAL_GetMouseButton      (*__mg_cur_input->get_mouse_button)

//#define IAL_SetMouseXY          if (__mg_cur_input->set_mouse_xy) (*__mg_cur_input->set_mouse_xy)
static inline void IAL_SetMouseXY (int x, int y) {
    if (__mg_cur_input->set_mouse_xy) {
#ifdef _COOR_TRANS
#if _ROT_DIR_CW
        int tmp = x;
        x = (WIDTHOFPHYSCREEN-1) - y;
        y = tmp;
#else
        int tmp = y;
        y = (HEIGHTOFPHYSCREEN-1) - x;
        x = tmp;
#endif
#endif
        (*__mg_cur_input->set_mouse_xy) (x, y);
    }
}

//#define IAL_SetMouseRange       if (__mg_cur_input->set_mouse_range) (*__mg_cur_input->set_mouse_range)
static inline void IAL_SetMouseRange (int minx, int miny, int maxx, int maxy) {
#ifdef _COOR_TRANS
    return;
#else
    if (__mg_cur_input->set_mouse_range) (*__mg_cur_input->set_mouse_range)(minx, miny, maxx, maxy);
#endif
}

#define IAL_SuspendMouse        if (__mg_cur_input->suspend_mouse) (*__mg_cur_input->suspend_mouse)
#define IAL_UpdateKeyboard      (*__mg_cur_input->update_keyboard)
#define IAL_GetKeyboardState    (*__mg_cur_input->get_keyboard_state)
#define IAL_SuspendKeyboard     if (__mg_cur_input->suspend_keyboard) (*__mg_cur_input->suspend_keyboard)
#define IAL_SetLeds(leds)       if (__mg_cur_input->set_leds) (*__mg_cur_input->set_leds) (leds)

static inline int IAL_ResumeMouse (void)
{
    if (__mg_cur_input->resume_mouse)
        return __mg_cur_input->resume_mouse ();
    else
        return -1;
}

static inline int IAL_ResumeKeyboard (void)
{
    if (__mg_cur_input->resume_keyboard)
        return __mg_cur_input->resume_keyboard ();
    else
        return -1;
}

#define IAL_WaitEvent           (*__mg_cur_input->wait_event)

#define IAL_MType               (__mg_cur_input->mtype)
#define IAL_MDev                (__mg_cur_input->mdev)

int InitIAL (void);
void TerminateIAL (void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* GUI_IAL_H */

