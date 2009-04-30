/* 
** $Id: init.c 9227 2008-01-30 03:08:12Z xwyan $
**
** init.c: The Initialization/Termination routines for MiniGUI-Threads.
**
** Copyright (C) 2003 ~ 2007 Feynman Software.
** Copyright (C) 1999 ~ 2002 Wei Yongming.
**
** All rights reserved by Feynman Software.
**
** Current maintainer: Wei Yongming.
**
** Create date: 2000/11/05
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "minigui.h"

#ifndef __NOUNIX__
#include <unistd.h>
#include <signal.h>
#include <sys/termios.h>
#endif

#include "minigui.h"
#include "gdi.h"
#include "window.h"
#include "cliprect.h"
#include "gal.h"
#include "ial.h"
#include "internals.h"
#include "ctrlclass.h"
#include "cursor.h"
#include "event.h"
#include "misc.h"
#include "menu.h"
#include "timer.h"
#include "accelkey.h"
#include "clipboard.h"

#ifndef _LITE_VERSION

/******************************* extern data *********************************/
extern void* DesktopMain (void* data);

/************************* Entry of the thread of parsor *********************/
static void ParseEvent (PLWEVENT lwe)
{
    PMOUSEEVENT me;
    PKEYEVENT ke;
    MSG Msg;
    static int mouse_x = 0, mouse_y = 0;

    ke = &(lwe->data.ke);
    me = &(lwe->data.me);
    Msg.hwnd = HWND_DESKTOP;
    Msg.wParam = 0;
    Msg.lParam = 0;

    Msg.time = __mg_timer_counter;

    if (lwe->type == LWETYPE_TIMEOUT) {
        Msg.message = MSG_TIMEOUT;
        Msg.wParam = (WPARAM)lwe->count;
        Msg.lParam = 0;
        QueueDeskMessage (&Msg);
    }
    else if (lwe->type == LWETYPE_KEY) {
        Msg.wParam = ke->scancode;
        Msg.lParam = ke->status;
        if(ke->event == KE_KEYDOWN){
            Msg.message = MSG_KEYDOWN;
        }
        else if (ke->event == KE_KEYUP) {
            Msg.message = MSG_KEYUP;
        }
        else if (ke->event == KE_KEYLONGPRESS) {
            Msg.message = MSG_KEYLONGPRESS;
        }
        else if (ke->event == KE_KEYALWAYSPRESS) {
            Msg.message = MSG_KEYALWAYSPRESS;
        }
        QueueDeskMessage (&Msg);
    }
    else if(lwe->type == LWETYPE_MOUSE) {
        Msg.wParam = me->status;
        Msg.lParam = MAKELONG (me->x, me->y);

        switch (me->event) {
        case ME_MOVED:
            Msg.message = MSG_MOUSEMOVE;
            break;
        case ME_LEFTDOWN:
            Msg.message = MSG_LBUTTONDOWN;
            break;
        case ME_LEFTUP:
            Msg.message = MSG_LBUTTONUP;
            break;
        case ME_LEFTDBLCLICK:
            Msg.message = MSG_LBUTTONDBLCLK;
            break;
        case ME_RIGHTDOWN:
            Msg.message = MSG_RBUTTONDOWN;
            break;
        case ME_RIGHTUP:
            Msg.message = MSG_RBUTTONUP;
            break;
        case ME_RIGHTDBLCLICK:
            Msg.message = MSG_RBUTTONDBLCLK;
            break;
        }

        if (me->event != ME_MOVED && (mouse_x != me->x || mouse_y != me->y)) {
            int old = Msg.message;

            Msg.message = MSG_MOUSEMOVE;
            QueueDeskMessage (&Msg);
            Msg.message = old;

            mouse_x = me->x; mouse_y = me->y;
        }

        QueueDeskMessage (&Msg);
    }
}


extern struct timeval __mg_event_timeout;

static void* EventLoop (void* data)
{
    LWEVENT lwe;
    int event;
	
    lwe.data.me.x = 0; lwe.data.me.y = 0;
	
    sem_post ((sem_t*)data);

    while (TRUE) {
        event = IAL_WaitEvent (IAL_MOUSEEVENT | IAL_KEYEVENT, 
                        NULL, NULL, NULL, (void*)&__mg_event_timeout);
        if (event < 0)
            continue;

        lwe.status = 0L;

        if (event & IAL_MOUSEEVENT && GetLWEvent (IAL_MOUSEEVENT, &lwe))
            ParseEvent (&lwe);

        lwe.status = 0L;
        if (event & IAL_KEYEVENT && GetLWEvent (IAL_KEYEVENT, &lwe))
            ParseEvent (&lwe);

        if (event == 0 && GetLWEvent (0, &lwe))
            ParseEvent (&lwe);
    }

    return NULL;
}

/************************** Thread Information  ******************************/

pthread_key_t __mg_threadinfo_key;

static inline BOOL createThreadInfoKey (void)
{
    if (pthread_key_create (&__mg_threadinfo_key, NULL))
        return FALSE;
    return TRUE;
}

static inline void deleteThreadInfoKey (void)
{
    pthread_key_delete (__mg_threadinfo_key);
}

MSGQUEUE* InitMsgQueueThisThread (void)
{
    MSGQUEUE* pMsgQueue;
    int ret;
    
    if (!(pMsgQueue = malloc(sizeof(MSGQUEUE))) ) {
        return NULL;
    }

    if (!InitMsgQueue(pMsgQueue, 0)) {
        free (pMsgQueue);
        return NULL;
    }

    ret = pthread_setspecific (__mg_threadinfo_key, pMsgQueue);
    return pMsgQueue;
}

void FreeMsgQueueThisThread (void)
{
    MSGQUEUE* pMsgQueue;

    pMsgQueue = pthread_getspecific (__mg_threadinfo_key);
#ifdef __VXWORKS__
    if (pMsgQueue != (void *)0 && pMsgQueue != (void *)-1) {
#else
    if (pMsgQueue) {
#endif
        DestroyMsgQueue (pMsgQueue);
        free (pMsgQueue);
#ifdef __VXWORKS__
        pthread_setspecific (__mg_threadinfo_key, (void*)-1);
#else
        pthread_setspecific (__mg_threadinfo_key, NULL);
#endif
    }
}

/*
The following function is moved to src/include/internals.h as an inline 
function.
MSGQUEUE* GetMsgQueueThisThread (void)
{
    return (MSGQUEUE*) pthread_getspecific (__mg_threadinfo_key);
}
*/

/************************** System Initialization ****************************/

int __mg_timer_init (void);

static BOOL SystemThreads(void)
{
    sem_t wait;

    sem_init (&wait, 0, 0);

    // this is the thread for desktop window.
    // this thread should have a normal priority same as
    // other main window threads. 
    // if there is no main window can receive the low level events,
    // this desktop window is the only one can receive the events.
    // to close a MiniGUI application, we should close the desktop 
    // window.
#ifdef __NOUNIX__
    {
        pthread_attr_t new_attr;
        pthread_attr_init (&new_attr);
        pthread_attr_setstacksize (&new_attr, 16 * 1024);
        pthread_create (&__mg_desktop, &new_attr, DesktopMain, &wait);
        pthread_attr_destroy (&new_attr);
    }
#else
    pthread_create (&__mg_desktop, NULL, DesktopMain, &wait);
#endif

    sem_wait (&wait);

    __mg_timer_init ();

    // this thread collect low level event from outside,
    // if there is no event, this thread will suspend to wait a event.
    // the events maybe mouse event, keyboard event, or timeout event.
    //
    // this thread also parse low level event and translate it to message,
    // then post the message to the approriate message queue.
    // this thread should also have a higher priority.
    pthread_create (&__mg_parsor, NULL, EventLoop, &wait);
    sem_wait (&wait);

    sem_destroy (&wait);
    return TRUE;
}

BOOL GUIAPI ReinitDesktopEx (BOOL init_sys_text)
{
    return SendMessage (HWND_DESKTOP, MSG_REINITSESSION, init_sys_text, 0) == 0;
}

#ifndef __NOUNIX__
static struct termios savedtermio;

void* GUIAPI GetOriginalTermIO (void)
{
    return &savedtermio;
}

static void segvsig_handler (int v)
{
    TerminateLWEvent ();
    TerminateGAL ();

    if (v == SIGSEGV)
        kill (getpid(), SIGABRT); /* cause core dump */
    else
        _exit (v);
}

static BOOL InstallSEGVHandler (void)
{
    struct sigaction siga;
    
    siga.sa_handler = segvsig_handler;
    siga.sa_flags = 0;
    
    memset (&siga.sa_mask, 0, sizeof (sigset_t));
    sigaction (SIGSEGV, &siga, NULL);
    sigaction (SIGTERM, &siga, NULL);
    sigaction (SIGINT, &siga, NULL);

    return TRUE;
}
#endif

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

int GUIAPI InitGUI (int args, const char *agr[])
{
    int step = 0;

#ifdef HAVE_SETLOCALE
    setlocale (LC_ALL, "C");
#endif

#if defined (_USE_MUTEX_ON_SYSVIPC) || defined (_USE_SEM_ON_SYSVIPC)
    step++;
    if (_sysvipc_mutex_sem_init ())
        return step;
#endif

#ifndef __NOUNIX__
    // Save original termio
    tcgetattr (0, &savedtermio);
#endif

    /*initialize default window process*/
    __mg_def_proc[0] = PreDefMainWinProc;
    __mg_def_proc[1] = PreDefDialogProc;
    __mg_def_proc[2] = PreDefControlProc;

    step++;
    if (!InitFixStr ()) {
        fprintf (stderr, "InitGUI: Init Fixed String module failure!\n");
        return step;
    }
    
    step++;
    /* Init miscelleous*/
    if (!InitMisc ()) {
        fprintf (stderr, "InitGUI: Initialization of misc things failure!\n");
        return step;
    }

    step++;
    switch (InitGAL ()) {
    case ERR_CONFIG_FILE:
        fprintf (stderr, 
            "InitGUI: Reading configuration failure!\n");
        return step;

    case ERR_NO_ENGINE:
        fprintf (stderr, 
            "InitGUI: No graphics engine defined!\n");
        return step;

    case ERR_NO_MATCH:
        fprintf (stderr, 
            "InitGUI: Can not get graphics engine information!\n");
        return step;

    case ERR_GFX_ENGINE:
        fprintf (stderr, 
            "InitGUI: Can not initialize graphics engine!\n");
        return step;
    }

#ifndef __NOUNIX__
    InstallSEGVHandler ();
#endif

    /* Init GDI. */

    step++;
    if(!InitGDI()) {
        fprintf (stderr, "InitGUI: Initialization of GDI failure!\n");
        goto failure1;
    }

    /* Init Master Screen DC here */
    step++;
    if (!InitScreenDC (__gal_screen)) {
        fprintf (stderr, "InitGUI: Can not initialize screen DC!\n");
        goto failure1;
    }

    step++;
    if (!InitWindowElementColors ()) {
        fprintf (stderr, "InitGUI: Can not initialize colors of window element!\n");
        goto failure1;
    }

    /* Init low level event */
    step++;
    if(!InitLWEvent()) {
        fprintf(stderr, "InitGUI: Low level event initialization failure!\n");
        goto failure1;
    }

    /* Init mouse cursor. */
    step++;
    if( !InitCursor() ) {
        fprintf (stderr, "InitGUI: Count not init mouse cursor support!\n");
        goto failure;
    }

    /* Init menu */
    step++;
    if (!InitMenu ()) {
        fprintf (stderr, "InitGUI: Init Menu module failure!\n");
        goto failure;
    }

    /* Init control class */
    step++;
    if(!InitControlClass()) {
        fprintf(stderr, "InitGUI: Init Control Class failure!\n");
        goto failure;
    }


    /* Init accelerator */
    step++;
    if(!InitAccel()) {
        fprintf(stderr, "InitGUI: Init Accelerator failure!\n");
        goto failure;
    }

    step++;
    if (!InitDesktop ()) {
        fprintf (stderr, "InitGUI: Init Desktop failure!\n");
        goto failure;
    }
   
    step++;
    if (!InitFreeQMSGList ()) {
        fprintf (stderr, "InitGUI: Init free QMSG list failure!\n");
        goto failure;
    }

    step++;
    if (!createThreadInfoKey ()) {
        fprintf (stderr, "InitGUI: Init thread hash table failure!\n");
        goto failure;
    }

    step++;
    if (!SystemThreads()) {
        fprintf (stderr, "InitGUI: Init system threads failure!\n");
        goto failure;
    }

    SetKeyboardLayout ("default");

    SetCursor (GetSystemCursor (IDC_ARROW));

    SetCursorPos (g_rcScr.right >> 1, g_rcScr.bottom >> 1);

    TerminateMgEtc ();
    return 0;

failure:
    TerminateLWEvent ();
failure1:
    TerminateGAL ();
#ifdef _INCORE_RES
    fprintf (stderr, "InitGUI failure when using incore resource.\n");
#else
    fprintf (stderr, "InitGUI failure when using %s as cfg file.\n", ETCFILEPATH);
#endif /* _INCORE_RES */
    return step;
}

void GUIAPI TerminateGUI (int rcByGUI)
{
    if (rcByGUI >= 0) {
        pthread_join (__mg_desktop, NULL);
    }

    deleteThreadInfoKey ();

    TerminateTimer ();
    TerminateDesktop ();

    DestroyFreeQMSGList ();

    TerminateAccel ();
    TerminateControlClass ();
    TerminateMenu ();
    TerminateMisc ();
    TerminateFixStr ();

#ifdef _CURSOR_SUPPORT
    TerminateCursor ();
#endif
    TerminateLWEvent ();
    TerminateScreenDC ();
    TerminateGDI ();
    TerminateGAL ();

    /* 
     * Restore original termio
     *tcsetattr (0, TCSAFLUSH, &savedtermio);
     */

#if defined (_USE_MUTEX_ON_SYSVIPC) || defined (_USE_SEM_ON_SYSVIPC)
    _sysvipc_mutex_sem_term ();
#endif
}

void GUIAPI ExitGUISafely (int exitcode)
{
    TerminateGUI ((exitcode > 0) ? -exitcode : exitcode);
#ifndef __NOUNIX__
    exit (2);
#endif
}

#endif /* !_LITE_VERSION */

