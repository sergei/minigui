/* 
** $Id: desktop-ths.c 7697 2007-09-27 03:03:13Z weiym $
**
** desktop-ths.c: The desktop for MiniGUI-Threads.
**
** Copyright (C) 2003 ~ 2007 Feynman Software.
** Copyright (C) 1999 ~ 2002 Wei Yongming.
**
** Current maintainer: Wei Yongming.
**
** Create date: 1999/04/19
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

#ifndef _LITE_VERSION

#include "minigui.h"
#include "gdi.h"
#include "window.h"
#include "cliprect.h"
#include "gal.h"
#include "internals.h"
#include "ctrlclass.h"
#include "menu.h"
#include "timer.h"
#include "misc.h"


/******************************* global data *********************************/
RECT g_rcScr;

/* system threads */
pthread_t __mg_desktop, __mg_parsor, __mg_timer;

/* current capture window */
HWND __mg_capture_wnd;

/* input method window */
HWND __mg_ime_wnd;

/* current active main window handle */
PMAINWIN __mg_active_mainwnd;

/* pointer to desktop message queue */
PMSGQUEUE __mg_dsk_msg_queue;

/* Track information of menu */
PTRACKMENUINFO __mg_ptmi;

/********************* Window management support *****************************/
static BLOCKHEAP sg_FreeInvRectList;
static BLOCKHEAP sg_FreeClipRectList;
static ZORDERINFO sg_MainWinZOrder;
static ZORDERINFO sg_TopMostWinZOrder;

static HWND sg_hCaretWnd;
static UINT sg_uCaretBTime;

static GCRINFO sg_ScrGCRInfo;

static BOOL InitWndManagementInfo (void)
{
    if (!InitMainWinMetrics())
        return FALSE;

    __mg_capture_wnd = 0;
    __mg_active_mainwnd = NULL;

    __mg_ptmi = NULL;

    __mg_ime_wnd = 0;
    sg_hCaretWnd = 0;

    g_rcScr.left = g_rcScr.top = 0;
    g_rcScr.right = GetGDCapability (HDC_SCREEN, GDCAP_MAXX) + 1;
    g_rcScr.bottom = GetGDCapability (HDC_SCREEN, GDCAP_MAXY) + 1;

    InitClipRgn (&sg_ScrGCRInfo.crgn, &sg_FreeClipRectList);
    SetClipRgn (&sg_ScrGCRInfo.crgn, &g_rcScr);
    pthread_mutex_init (&sg_ScrGCRInfo.lock, NULL);
    sg_ScrGCRInfo.age = 0;

    return TRUE;
}

static void InitZOrderInfo (PZORDERINFO pZOrderInfo, HWND hHost);

BOOL InitDesktop (void)
{
    /*
     * Init ZOrderInfo here.
     */
    InitZOrderInfo (&sg_MainWinZOrder, HWND_DESKTOP);
    InitZOrderInfo (&sg_TopMostWinZOrder, HWND_DESKTOP);
    
    /*
     * Init heap of clipping rects.
     */
    InitFreeClipRectList (&sg_FreeClipRectList, SIZE_CLIPRECTHEAP);

    /*
     * Init heap of invalid rects.
     */
    InitFreeClipRectList (&sg_FreeInvRectList, SIZE_INVRECTHEAP);

    /*
     * Load system resource here.
     */
    if (!InitSystemRes ()) {
        fprintf (stderr, "DESKTOP: Can not initialize system resource!\n");
        return FALSE;
    }

    // Init Window Management information.
    if (!InitWndManagementInfo()) {
        fprintf (stderr, 
            "DESKTOP: Can not initialize window management information!\n");
        return FALSE;
    }

    return TRUE;
}

#include "desktop-comm.c"

void* DesktopMain (void* data)
{
    MSG Msg;

    /* init message queue of desktop thread */
    if (!(__mg_dsk_msg_queue = InitMsgQueueThisThread ()) ) {
        fprintf (stderr, "DESKTOP: InitMsgQueueThisThread failure!\n");
        return NULL;
    }

    /* init desktop window */
    init_desktop_win();
    __mg_dsk_win->th = pthread_self ();
    __mg_dsk_msg_queue->pRootMainWin = __mg_dsk_win;

    DesktopWinProc (HWND_DESKTOP, MSG_STARTSESSION, 0, 0);
    PostMessage (HWND_DESKTOP, MSG_ERASEDESKTOP, 0, 0);

    /* desktop thread is ready now */
    sem_post ((sem_t*)data);

    /* process messages of desktop thread */
    while (GetMessage(&Msg, HWND_DESKTOP)) {
        int iRet = 0;

#ifdef _TRACE_MSG
        fprintf (stderr, "Message, %s: hWnd: %#x, wP: %#x, lP: %#lx. %s\n",
            Message2Str (Msg.message),
            Msg.hwnd,
            Msg.wParam,
            Msg.lParam,
            Msg.pAdd?"Sync":"Normal");
#endif

        iRet = DesktopWinProc (HWND_DESKTOP, 
                        Msg.message, Msg.wParam, Msg.lParam);

        if (Msg.pAdd) /* this is a sync message. */
        {
            PSYNCMSG pSyncMsg = (PSYNCMSG)(Msg.pAdd);
            pSyncMsg->retval = iRet;
            sem_post(pSyncMsg->sem_handle);
        }

#ifdef _TRACE_MSG
        fprintf (stderr, "Message, %s done, return value: %#x\n",
            Message2Str (Msg.message), iRet);
#endif
    }

    return NULL;
}

pthread_t GUIAPI GetMainWinThread(HWND hMainWnd)
{ 
#ifdef WIN32
    pthread_t ret;
    memset(&ret, 0, sizeof(pthread_t));
    MG_CHECK_RET (MG_IS_WINDOW(hMainWnd), ret);
#else
    MG_CHECK_RET (MG_IS_WINDOW(hMainWnd), 0);
#endif
    
    return ((PMAINWIN)hMainWnd)->th;
}

#endif /* !_LITE_VERSION */

