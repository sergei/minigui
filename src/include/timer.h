/*
** $Id: timer.h 7814 2007-10-11 06:30:44Z weiym $
**
** tiemr.h: the head file of Timer module.
**
** Copyright (C) 2003 ~ 2007 Feynman Software.
** Copyright (C) 1999 ~ 2002 Wei Yongming.
**
** Create date: 1999/4/21
*/

#ifndef GUI_TIMER_H
    #define GUI_TIMER_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define USEC_1S         1000000
#define USEC_10MS       10000

typedef struct _timer {
    HWND    hWnd;
    int     id;
    unsigned int speed;
    unsigned int count;

    TIMERPROC proc;
    unsigned int tick_count;
    PMSGQUEUE msg_queue;
} TIMER;
typedef TIMER* PTIMER;

BOOL InitTimer (void);
void TerminateTimer (void);
void DispatchTimerMessage (unsigned int inter);

TIMER* __mg_get_timer (int slot);
void __mg_remove_timer (TIMER* timer, int slot);

static inline HWND __mg_get_timer_hwnd (int slot)
{
    TIMER* ptimer = __mg_get_timer (slot);
    if (ptimer)
        return ptimer->hWnd;

    return HWND_NULL;
}

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* GUI_TIMER_H */

