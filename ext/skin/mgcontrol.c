/*
** $Id: mgcontrol.c 7372 2007-08-16 05:33:55Z xgwang $
**
** mgcontrol.c: skin MiniGUI control item implementation.
**
** Copyright (C) 2003 ~ 2007 Feynman Software.
**
** Current maintainer: Allen
**
** Create date: 2003-10-21 by Allen
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __MINIGUI_LIB__
    #include "common.h"
    #include "minigui.h"
    #include "gdi.h"
    #include "window.h"
    #include "control.h"
    #include "mgext.h"
#else
    #include <minigui/common.h>
    #include <minigui/minigui.h>
    #include <minigui/gdi.h>
    #include <minigui/window.h>
    #include <minigui/control.h>
    #include <minigui/mgext.h>
#endif

#ifdef _EXT_SKIN

#include "skin.h"

static int control_init (skin_head_t* skin, skin_item_t* item)
{
    return 0;
}

static int control_on_create (skin_item_t* item)
{
    PCTRLDATA    pCtrlData = item->type_data;

    item->bmp_index = 
    CreateWindowEx (pCtrlData->class_name,
                    pCtrlData->caption,
                    pCtrlData->dwStyle | WS_CHILD,
                    pCtrlData->dwExStyle,
                    pCtrlData->id,
                    pCtrlData->x,
                    pCtrlData->y,
                    pCtrlData->w,
                    pCtrlData->h,
                    item->hostskin->hwnd,
                    pCtrlData->dwAddData);
    return 1;                
}

static int control_on_destroy (skin_item_t* item)
{
    DestroyWindow ((HWND)item->bmp_index);
    return 1;                
}

static DWORD control_get_hwnd (skin_item_t *item)
{
    return (DWORD)item->bmp_index;
}

static skin_item_ops_t control_ops = {
    control_init,
    NULL,
    control_on_create,
    control_on_destroy,
    NULL,
    NULL,
    control_get_hwnd,
    NULL,
    NULL
};

skin_item_ops_t *CONTROL_OPS = &control_ops;

#endif /* _EXT_SKIN */
