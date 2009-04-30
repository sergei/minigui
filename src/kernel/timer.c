/*
** $Id: timer.c 8028 2007-11-01 09:29:33Z weiym $
**
** timer.c: The Timer module for MiniGUI-Threads.
**
** Copyright (C) 2003 ~ 2007 Feynman Software.
** Copyright (C) 1999 ~ 2002 Wei Yongming.
**
** All rights reserved by Feynman Software.
**
** Current maintainer: Wei Yongming.
**
** Create date: 1999/04/21
*/ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "minigui.h"
#include "gdi.h"
#include "window.h"
#include "cliprect.h"
#include "gal.h"
#include "internals.h"
#include "misc.h"
#include "timer.h"

#if defined(_LITE_VERSION) && !defined(_STAND_ALONE)
#include "client.h"
#include "sharedres.h"
#endif

#ifdef _DEBUG
#define TIMER_ERR_SYS(text)         err_sys (text)
#else
#define TIMER_ERR_SYS(text)
#endif

unsigned int __mg_timer_counter = 0;
static TIMER *timerstr[DEF_NR_TIMERS];

#ifndef _LITE_VERSION
/* lock for protecting timerstr */
static pthread_mutex_t timerLock;
#define TIMER_LOCK()   pthread_mutex_lock(&timerLock)
#define TIMER_UNLOCK() pthread_mutex_unlock(&timerLock)
#else
#define TIMER_LOCK()
#define TIMER_UNLOCK()
#endif

/* timer action for minigui timers */
static void __mg_timer_action (void *data)
{
#if defined(_LITE_VERSION) && !defined(_STAND_ALONE)
    SHAREDRES_TIMER_COUNTER += 1;
#else

#if defined(__uClinux__) && defined(_STAND_ALONE)
    __mg_timer_counter += 10;
#else
    __mg_timer_counter ++;
#endif

#endif

#ifndef _LITE_VERSION
    /* alert desktop */
    AlertDesktopTimerEvent ();
#endif
}

/* timer entry for thread version */
#ifndef _LITE_VERSION

#ifdef __AOS__

#include "os_api.h"

static OS_TIMER_ID __mg_os_timer = 0;

int __mg_timer_init (void)
{
    if (!InitTimer ())
        return -1;

    __mg_os_timer = tp_os_timer_create ("mgtimer", __mg_timer_action, 
                                         NULL, AOS_TIMER_TICKT, 
                                         OS_AUTO_ACTIVATE | OS_AUTO_LOAD);

    return 0;
}

#else /* __AOS__ */

static inline void _os_timer_loop (void)
{
    while (1) {
        __mg_os_time_delay (10);
        __mg_timer_action (NULL);
    }
}

static void* TimerEntry (void* data)
{
    if (!InitTimer ()) {
        fprintf (stderr, "TIMER: Init Timer failure, exit!\n");
#ifndef __NOUNIX__
        exit (1);
#endif
        return NULL;
    }
    sem_post ((sem_t*)data);

    _os_timer_loop ();

    return NULL;
}

int __mg_timer_init (void)
{
    sem_t wait;

    sem_init (&wait, 0, 0);
    pthread_create (&__mg_timer, NULL, TimerEntry, &wait);
    sem_wait (&wait);
    sem_destroy (&wait);

    return 0;
}

#endif /* !__AOS__ */

#else /* _LITE_VERSION: for MiniGUI-Processes and MiniGUI-Standalone */

#ifdef __NOUNIX__

BOOL InstallIntervalTimer (void)
{
    return TRUE;
}

BOOL UninstallIntervalTimer (void)
{
    return TRUE;
}

#else

#include <signal.h>
#include <unistd.h>
#include <sys/time.h>

static struct sigaction old_alarm_handler;
static struct itimerval old_timer;

BOOL InstallIntervalTimer (void)
{
    struct itimerval timerv;
    struct sigaction siga;
    
    sigaction (SIGALRM, NULL, &old_alarm_handler);

    siga = old_alarm_handler;
    siga.sa_handler = (void(*)(int))__mg_timer_action;
#ifndef _STAND_ALONE
    siga.sa_flags = 0;
#else
    siga.sa_flags = SA_RESTART;
#endif
    sigaction (SIGALRM, &siga, NULL);

    timerv.it_interval.tv_sec = 0;
#if defined(__uClinux__) && defined(_STAND_ALONE)
    timerv.it_interval.tv_usec = 100000;        /* 100 ms */
#else
    timerv.it_interval.tv_usec = 10000;         /* 10 ms */
#endif
    timerv.it_value = timerv.it_interval;

    if (setitimer (ITIMER_REAL, &timerv, &old_timer) == -1) {
        TIMER_ERR_SYS ("setitimer call failed!\n");
        return FALSE;
    }

    return TRUE;
}

BOOL UninstallIntervalTimer (void)
{
    if (setitimer (ITIMER_REAL, &old_timer, 0) == -1) {
        TIMER_ERR_SYS ("setitimer call failed!\n");
        return FALSE;
    }

    if (sigaction (SIGALRM, &old_alarm_handler, NULL) == -1) {
        TIMER_ERR_SYS ("sigaction call failed!\n");
        return FALSE;
    }

    return TRUE;
}

#endif /* __NOUNIX__ */
#endif /* _LITE_VERSION */

BOOL InitTimer (void)
{
#ifdef _LITE_VERSION
    InstallIntervalTimer ();
#endif

#ifndef _LITE_VERSION
    pthread_mutex_init (&timerLock, NULL);
    __mg_timer_counter = 0;
#endif

    return TRUE;
}

void TerminateTimer (void)
{
    int i;

#ifdef __AOS__
    tp_os_timer_delete (__mg_os_timer);
#endif

#ifdef _LITE_VERSION
    UninstallIntervalTimer ();
#else
#   ifdef __WINBOND_SWLINUX__
    pthread_detach (__mg_timer);
#   else
    pthread_cancel (__mg_timer);
#   endif
#endif

    for (i = 0; i < DEF_NR_TIMERS; i++) {
        if (timerstr[i] != NULL)
            free ( timerstr[i] );
        timerstr[i] = NULL;
    }

#ifndef _LITE_VERSION
    pthread_mutex_destroy (&timerLock);
#endif
}

/************************* Functions run in desktop thread *******************/
void DispatchTimerMessage (unsigned int inter)
{
    int i;

    TIMER_LOCK ();

    for (i=0; i<DEF_NR_TIMERS; i++) {
        if (timerstr[i] && timerstr[i]->msg_queue) {
            timerstr[i]->count += inter;
            if (timerstr[i]->count >= timerstr[i]->speed) {
                if (timerstr[i]->tick_count == 0)
#if defined(_LITE_VERSION) && !defined(_STAND_ALONE)
                    timerstr[i]->tick_count = SHAREDRES_TIMER_COUNTER;
#else
                    timerstr[i]->tick_count = __mg_timer_counter;
#endif
                /* setting timer flag is simple, we do not need to lock msgq,
                   or else we may encounter dead lock here */ 
                SetMsgQueueTimerFlag (timerstr[i]->msg_queue, i);
                
                timerstr[i]->count -= timerstr[i]->speed;
            }
        }
    }

    TIMER_UNLOCK ();
}

/* thread safe, no mutex */
TIMER* __mg_get_timer (int slot)
{
    if (slot < 0 || slot >= DEF_NR_TIMERS)
        return NULL;

    return timerstr[slot];
}

#if 0
int __mg_get_timer_slot (HWND hWnd, int id)
{
    int i;
    int slot = -1;

    TIMER_LOCK ();
    for (i = 0; i < DEF_NR_TIMERS; i++) {
        if (timerstr[i] != NULL) {
            if (timerstr[i]->hWnd == hWnd && timerstr[i]->id == id) {
                slot = i;
                break;
            }
        }
    }

    TIMER_UNLOCK ();
    return slot;
}

void __mg_move_timer_last (TIMER* timer, int slot)
{
    if (slot < 0 || slot >= DEF_NR_TIMERS)
        return;

    TIMER_LOCK ();

    if (timer && timer->msg_queue) {
        /* The following code is already called in message.c...
         * timer->tick_count = 0;
         * timer->msg_queue->TimerMask &= ~(0x01 << slot);
         */

        if (slot != (DEF_NR_TIMERS - 1)) {
            TIMER* t;

            if (timer->msg_queue->TimerMask & (0x01 << (DEF_NR_TIMERS -1))) {
                timer->msg_queue->TimerMask |= (0x01 << slot);
                timer->msg_queue->TimerMask &= ~(0x01 << (DEF_NR_TIMERS -1));
            }

            t = timerstr [DEF_NR_TIMERS - 1];
            timerstr [DEF_NR_TIMERS - 1] = timerstr [slot];
            timerstr [slot] = t;
        }
    }

    TIMER_UNLOCK ();
    return;
}
#endif

BOOL GUIAPI SetTimerEx (HWND hWnd, int id, unsigned int speed, 
                TIMERPROC timer_proc)
{
    int i;
    PMSGQUEUE pMsgQueue;
    int slot = -1;

#ifndef _LITE_VERSION
    if (!(pMsgQueue = GetMsgQueueThisThread ()))
        return FALSE;
#else
    pMsgQueue = __mg_dsk_msg_queue;
#endif

    TIMER_LOCK ();

    /* Is there an empty timer slot? */
    for (i=0; i<DEF_NR_TIMERS; i++) {
        if (timerstr[i] == NULL) {
            if (slot < 0)
                slot = i;
        }
        else if (timerstr[i]->hWnd == hWnd && timerstr[i]->id == id) {
            goto badret;
        }
    }

    if (slot < 0 || slot == DEF_NR_TIMERS)
        goto badret ;

    timerstr[slot] = malloc (sizeof (TIMER));

    timerstr[slot]->speed = speed;
    timerstr[slot]->hWnd = hWnd;
    timerstr[slot]->id = id;
    timerstr[slot]->count = 0;
    timerstr[slot]->proc = timer_proc;
    timerstr[slot]->tick_count = 0;
    timerstr[slot]->msg_queue = pMsgQueue;

#if defined(_LITE_VERSION) && !defined(_STAND_ALONE)
    if (!mgIsServer)
        __mg_set_select_timeout (USEC_10MS * speed);
#endif

    TIMER_UNLOCK ();
    return TRUE;
    
badret:
    TIMER_UNLOCK ();
    return FALSE;
}

#if defined(_LITE_VERSION) && !defined(_STAND_ALONE)
static void reset_select_timeout (void)
{
    int i;

    unsigned int speed = 0;
    for (i=0; i<DEF_NR_TIMERS; i++) {
        if (timerstr[i]) {
            if (speed == 0 || timerstr[i]->speed < speed)
                speed = timerstr[i]->speed;
        }
    }
    __mg_set_select_timeout (USEC_10MS * speed);
}

#endif

void __mg_remove_timer (TIMER* timer, int slot)
{
    if (slot < 0 || slot >= DEF_NR_TIMERS)
        return;

    TIMER_LOCK ();

    if (timer && timer->msg_queue && timerstr[slot] == timer) {
        /* The following code is already called in message.c...
         * timer->msg_queue->TimerMask &= ~(0x01 << slot);
         */
        free (timerstr[slot]);
        timerstr [slot] = NULL;

#if defined(_LITE_VERSION) && !defined(_STAND_ALONE)
        if (!mgIsServer)
            reset_select_timeout ();
#endif
    }


    TIMER_UNLOCK ();
    return;
}

/* If id <= 0, clear all timers of the window */
int GUIAPI KillTimer (HWND hWnd, int id)
{
    int i;
    PMSGQUEUE pMsgQueue;
    int killed = 0;

#ifndef _LITE_VERSION
    if (!(pMsgQueue = GetMsgQueueThisThread ()))
        return 0;
#else
    pMsgQueue = __mg_dsk_msg_queue;
#endif

    TIMER_LOCK ();

    for (i = 0; i < DEF_NR_TIMERS; i++) {
        if ((timerstr [i] && timerstr [i]->hWnd == hWnd) && 
                        (id <= 0 || timerstr [i]->id == id)) {
            RemoveMsgQueueTimerFlag (pMsgQueue, i);
            free (timerstr[i]);
            timerstr [i] = NULL;
            killed ++;

            if (id > 0) break;
        }
    }

#if defined(_LITE_VERSION) && !defined(_STAND_ALONE)
    if (!mgIsServer && killed)
        reset_select_timeout ();
#endif

    TIMER_UNLOCK ();
    return killed;
}

BOOL GUIAPI ResetTimerEx (HWND hWnd, int id, unsigned int speed, 
                TIMERPROC timer_proc)
{
    int i;
    PMSGQUEUE pMsgQueue;

#ifndef _LITE_VERSION
    if (!(pMsgQueue = GetMsgQueueThisThread ()))
        return FALSE;
#else
    pMsgQueue = __mg_dsk_msg_queue;
#endif

    TIMER_LOCK ();

    for (i=0; i<DEF_NR_TIMERS; i++) {
      if (timerstr[i] && timerstr[i]->hWnd == hWnd && timerstr[i]->id == id) {
        /* Should clear old timer flags */
        RemoveMsgQueueTimerFlag (pMsgQueue, i);
        timerstr[i]->speed = speed;
        timerstr[i]->count = 0;
        if (timer_proc != (TIMERPROC)0xFFFFFFFF)
            timerstr[i]->proc = timer_proc;
        timerstr[i]->tick_count = 0;

        TIMER_UNLOCK ();
        return TRUE;
      }
    }

    TIMER_UNLOCK ();
    return FALSE;
}

BOOL GUIAPI IsTimerInstalled (HWND hWnd, int id)
{
    int i;

    TIMER_LOCK ();

    for (i=0; i<DEF_NR_TIMERS; i++) {
        if ( timerstr[i] != NULL ) {
            if ( timerstr[i]->hWnd == hWnd && timerstr[i]->id == id) {
                TIMER_UNLOCK ();
                return TRUE;
            }
        }
    }

    TIMER_UNLOCK ();
    return FALSE;
}

/* no lock is ok */
BOOL GUIAPI HaveFreeTimer (void)
{
    int i;

    for (i=0; i<DEF_NR_TIMERS; i++) {
        if (timerstr[i] == NULL)
            return TRUE;
    }

    return FALSE;
}

unsigned int GUIAPI GetTickCount (void)
{
#if defined(_LITE_VERSION) && !defined(_STAND_ALONE)
    return SHAREDRES_TIMER_COUNTER;
#else
    return __mg_timer_counter;
#endif
}

