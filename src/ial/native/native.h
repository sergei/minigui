/*
** $Id: native.h 6566 2006-05-10 01:44:57Z xwyan $
**
** native.h: the head file of native Low Level Input Engine 
**
** Copyright (C) 2003 ~ 2006 Feynman Software.
** Copyright (C) 2000 ~ 2002 Song Lixin and Wei Yongming.
**
** Created by Song Lixin, 2000/10/17
*/

#ifndef GUI_IAL_NATIVE_H
    #define GUI_IAL_NATIVE_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#ifdef _LITE_VERSION

/* vtswitch */
/* signals for VT switching */
#define SIGRELVT        SIGUSR1
#define SIGACQVT        SIGUSR2
extern int switching_blocked;
int init_vtswitch (int kbd_fd);
int done_vtswitch (int kbd_fd);
int vtswitch_try (int vt);
/* end of vtswitch*/

#endif

BOOL InitNativeInput (INPUT* input, const char* mdev, const char* mtype);
void TermNativeInput (void);

/* Interface to Mouse Device Driver*/
typedef struct _mousedevice {
    int  (*Open)(const char* mdev);
    void (*Close)(void);
    int  (*GetButtonInfo)(void);
    void (*GetDefaultAccel)(int *pscale,int *pthresh);
    int  (*Read)(int *dx,int *dy,int *dz,int *bp);
    void (*Suspend) (void);
    int  (*Resume) (void);
} MOUSEDEVICE;

#ifdef _GPM_SUPPORT
  extern MOUSEDEVICE mousedev_GPM;
#endif
#ifdef _PS2_SUPPORT
  extern MOUSEDEVICE mousedev_PS2;
#endif
#ifdef _IMPS2_SUPPORT
  extern MOUSEDEVICE mousedev_IMPS2;
#endif
#ifdef _MS_SUPPORT
  extern MOUSEDEVICE mousedev_MS;
#endif
#ifdef _MS3_SUPPORT
  extern MOUSEDEVICE mousedev_MS3;
#endif

/* Interface to Keyboard Device Driver*/
typedef struct _kbddevice {
    int  (*Open)(void);
    void (*Close)(void);
    void (*GetModifierInfo)(int *modifiers);
    int  (*Read)(unsigned char *buf,int *modifiers);
    void (*Suspend) (void);
    int  (*Resume) (void);
} KBDDEVICE;

extern KBDDEVICE kbddev_tty;

/* Mouse button bits*/
#define WHEEL_UP    0x10
#define WHEEL_DOWN  0x08 

#define BUTTON_L    0x04
#define BUTTON_M    0x02
#define BUTTON_R    0x01

#define MIN_COORD   -32767
#define MAX_COORD   32767

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* GUI_IAL_NATIVE_H*/


