/*
** $Id: element.c 7474 2007-08-28 07:40:45Z weiym $
**
** element.c: Implementation of the window element data.
**
** Copyright (C) 2004 ~ 2007 Feynman Software.
**
** All rights reserved by Feynman Software.
**
** Current maintainer: Wei Yongming.
**
** Create date: 2004/05/10
*/


#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "minigui.h"
#include "gdi.h"
#include "window.h"
#include "control.h"
#include "cliprect.h"
#include "gal.h"
#include "internals.h"
#include "element.h"

int get_window_element_data (HWND hwnd, Uint16 type, Uint16 item, Uint32* data)
{
    PMAINWIN pwnd = (PMAINWIN)hwnd;
    list_t* me;

    if (pwnd == NULL)
        return WED_INVARG;

    if (pwnd->wed == NULL)
        return WED_NODEFINED;

    list_for_each (me, &pwnd->wed->list) {
        WND_ELEMENT_DATA* wed;
        wed = list_entry (me, WND_ELEMENT_DATA, list);
        if (wed->type == type && wed->item == item) {
            *data = wed->data;
            return WED_OK;
        }
    }

    return WED_NOTFOUND;
}

int set_window_element_data (HWND hwnd, Uint16 type, Uint16 item, Uint32 new_data, Uint32* old_data)
{
    PMAINWIN pwnd = (PMAINWIN)hwnd;
    WND_ELEMENT_DATA* new_wed;
    list_t* me;

    if (pwnd == NULL)
        return WED_INVARG;

    if (pwnd->wed == NULL) {
        pwnd->wed = calloc (1, sizeof (WND_ELEMENT_DATA));
        if (pwnd->wed == NULL)
            return WED_MEMERR;

        INIT_LIST_HEAD (&pwnd->wed->list);

        new_wed = calloc (1, sizeof (WND_ELEMENT_DATA));
        if (new_wed == NULL)
            return WED_MEMERR;

        new_wed->type = type;
        new_wed->item = item;
        new_wed->data = new_data;
        list_add_tail (&new_wed->list, &pwnd->wed->list);
        return WED_NEW_DATA;
    }
    else {
        list_for_each (me, &pwnd->wed->list) {
            WND_ELEMENT_DATA* wed;
            wed = list_entry (me, WND_ELEMENT_DATA, list);
            if (wed->type == type && wed->item == item) {
                *old_data = wed->data;
                if (new_data == *old_data) {
                    return WED_NOT_CHANGED;
                }
                else {
                    wed->data = new_data;
                    return WED_OK;
                }
            }
        }

        new_wed = calloc (1, sizeof (WND_ELEMENT_DATA));
        if (new_wed == NULL)
            return WED_MEMERR;

        new_wed->type = type;
        new_wed->item = item;
        new_wed->data = new_data;
        list_add_tail (&new_wed->list, &pwnd->wed->list);
        return WED_NEW_DATA;
    }
}

int free_window_element_data (HWND hwnd)
{
    PMAINWIN pwnd = (PMAINWIN)hwnd;
    WND_ELEMENT_DATA* wed;

    if (pwnd == NULL)
        return WED_INVARG;

    if (pwnd->wed == NULL)
        return WED_OK;

    while (!list_empty (&pwnd->wed->list)) {
        wed = list_entry (pwnd->wed->list.next, WND_ELEMENT_DATA, list);
        list_del (&wed->list);
        free (wed);
    }
    
    free (pwnd->wed);
    pwnd->wed = NULL;
    return WED_OK;
}

