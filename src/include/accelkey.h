/*
** $Id: accelkey.h 7459 2007-08-24 02:13:16Z weiym $
**
** acclekey.h: the head file of accelkey.c.
**
** Copyright (C) 2003 ~ 2007 Feynman Software.
** Copyright (C) 1999 ~ 2002 Wei Yongming.
**
** NOTE: Originally by Kang Xiaoning.
**
** Create date: 1999/8/28
*/

#ifndef GUI_ACCEL_H
    #define GUI_ACCEL_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

typedef struct _ACCELITEM
{
    int                 key;            // lower 8 bit is key c
    DWORD               keymask;
    WPARAM              wParam;         // command id
    LPARAM              lParam;         // lParam to be sent to
    struct _ACCELITEM*  next;           // next item                           
} ACCELITEM;
typedef ACCELITEM* PACCELITEM;

typedef struct _ACCELTABLE
{
    short               category;       // class of data.
    HWND                hwnd;           // owner.
    PACCELITEM          head;           // head of menu item list
} ACCELTABLE;
typedef ACCELTABLE* PACCELTABLE;

#if defined (__NOUNIX__) || defined (__uClinux__)
    #define SIZE_AC_HEAP   1
    #define SIZE_AI_HEAP   4
#else
  #ifdef _LITE_VERSION
    #define SIZE_AC_HEAP   1
    #define SIZE_AI_HEAP   4
  #else
    #define SIZE_AC_HEAP   64
    #define SIZE_AI_HEAP   512
  #endif
#endif

BOOL InitAccel (void);
void TerminateAccel (void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif // GUI_ACCEL_H

