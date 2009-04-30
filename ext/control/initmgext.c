/*
** $Id: initmgext.c 7451 2007-08-23 13:46:13Z weiym $
** 
** initmgext.c: Initialization and cleanup of mgext library.
** 
** Copyright (C) 2003 ~ 2007 Feynman Software
** Copyright (C) 2001 ~ 2002 Wei Yongming.
**
** Create date: 2001/01/17
*/

#ifdef __MINIGUI_LIB__
    #include "common.h"
    #include "minigui.h"
    #include "gdi.h"
    #include "window.h"
    #include "control.h"
#else
    #include <minigui/common.h>
    #include <minigui/minigui.h>
    #include <minigui/gdi.h>
    #include <minigui/window.h>
    #include <minigui/control.h>
#endif

#include "mgext.h"

#ifdef _EXT_CTRL_MONTHCAL
extern BOOL RegisterMonthCalendarControl (void);
extern void MonthCalendarControlCleanup (void);
#endif

#ifdef _EXT_CTRL_TREEVIEW
extern BOOL RegisterTreeViewControl (void);
extern void TreeViewControlCleanup (void);
#endif

#ifdef _EXT_CTRL_SPINBOX
extern BOOL RegisterSpinControl (void);
extern void SpinControlCleanup (void);
#endif

#ifdef _EXT_CTRL_COOLBAR
extern BOOL RegisterCoolBarControl (void);
extern void CoolBarControlCleanup (void);
#endif

#ifdef _EXT_CTRL_LISTVIEW
extern BOOL RegisterListViewControl (void);
extern void ListViewControlCleanup (void);
#endif

#ifdef _EXT_CTRL_GRIDVIEW
extern BOOL RegisterGridViewControl (void);
extern void GridViewControlCleanup (void);
#endif

#ifdef _EXT_CTRL_ICONVIEW
extern BOOL RegisterIconViewControl (void);
extern void IconViewControlCleanup (void);
#endif

#ifdef _EXT_CTRL_ANIMATION
extern BOOL RegisterAnimationControl (void);
extern void AnimationControlCleanup (void);
#endif

#ifdef _EXT_SKIN
/* defined in the skin module */
BOOL RegisterSkinControl (void);
void SkinControlCleanup (void);
#endif

#if !defined(__NOUNIX__) && defined(_EXT_CTRL_LISTVIEW)
/* defined in the mywins module */
BOOL InitNewOpenFileBox (void);
void NewOpenFileBoxCleanup (void);
#endif

static int init_count = 0;

BOOL InitMiniGUIExt ( void )
{
    if (init_count > 0)
        goto success;

#ifdef _EXT_CTRL_TREEVIEW
    if (!RegisterTreeViewControl ()) {
        _MG_PRINTF ("InitMiniGUIExt error: TreeView.\n");
        return FALSE;
    }
#endif

#ifdef _EXT_CTRL_MONTHCAL
    if (!RegisterMonthCalendarControl ()) {
        _MG_PRINTF ("InitMiniGUIExt error: MonthCalendar.\n");
        return FALSE;
    }
#endif

#ifdef _EXT_CTRL_SPINBOX
    if (!RegisterSpinControl ()) {
        _MG_PRINTF ("InitMiniGUIExt error: Spin.\n");
        return FALSE;
    }
#endif

#ifdef _EXT_CTRL_COOLBAR
    if (!RegisterCoolBarControl ()) {
        _MG_PRINTF ("InitMiniGUIExt error: CoolBar.\n");
        return FALSE;
    }
#endif

#ifdef _EXT_CTRL_LISTVIEW
    if (!RegisterListViewControl ()) {
        _MG_PRINTF ("InitMiniGUIExt error: ListView.\n");
        return FALSE;
    }
#endif

#ifdef _EXT_CTRL_GRIDVIEW
    if (!RegisterGridViewControl ()) {
        _MG_PRINTF ("InitMiniGUIExt error: GridView.\n");
        return FALSE;
    }
#endif

#ifdef _EXT_CTRL_ICONVIEW
    if (!RegisterIconViewControl ()) {
        _MG_PRINTF ("InitMiniGUIExt error: IconView.\n");
        return FALSE;
    }
#endif

#ifdef _EXT_CTRL_ANIMATION
    if (!RegisterAnimationControl ()) {
        _MG_PRINTF ("InitMiniGUIExt error: Animation.\n");
        return FALSE;
    }
#endif

#ifdef _EXT_SKIN
    if (!RegisterSkinControl ()) {
        _MG_PRINTF ("InitMiniGUIExt error: Skin.\n");
        return FALSE;
    }
#endif
#if !defined(__NOUNIX__) && defined(_EXT_CTRL_LISTVIEW)
    if (!InitNewOpenFileBox ()) {
        _MG_PRINTF ("InitMiniGUIExt error: NewOpenFileBox.\n");
        return FALSE;
    }
#endif

success:
    init_count ++;
    return TRUE;
}

void MiniGUIExtCleanUp (void)
{
    if (init_count == 0) /* not inited */
        return;

    init_count --;
    if (init_count != 0)
        return;

#ifdef _EXT_CTRL_TREEVIEW
    TreeViewControlCleanup ();
#endif
#ifdef _EXT_CTRL_MONTHCAL
    MonthCalendarControlCleanup ();
#endif
#ifdef _EXT_CTRL_SPINBOX
    SpinControlCleanup ();
#endif
#ifdef _EXT_CTRL_COOLBAR
    CoolBarControlCleanup ();
#endif
#ifdef _EXT_CTRL_LISTVIEW
    ListViewControlCleanup ();
#endif
#ifdef _EXT_CTRL_GRIDVIEW
    GridViewControlCleanup ();
#endif
#ifdef _EXT_CTRL_ANIMATION
    AnimationControlCleanup ();
#endif
#ifdef _EXT_SKIN
    SkinControlCleanup ();
#endif
#if !defined(__NOUNIX__) && defined(_EXT_CTRL_LISTVIEW)
    NewOpenFileBoxCleanup ();
#endif
}

