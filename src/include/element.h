/*
** $Id: element.h 7337 2007-08-16 03:44:29Z xgwang $
**
** element.h: the head file of window element data.
** 
** Copyright (C) 2004 ~ 2007 Feynman Software.
**
** Create date: 2004/05/10
*/

#ifndef GUI_ELEMENT_H
    #define GUI_ELEMENT_H

#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define WET_METRICS         1   /* the metrics of element */
#define WET_COLOR           2   /* the color of element */

/* window element data struct */
typedef struct _wnd_element_data
{
    list_t      list;       /* list pointer */
    Uint16      type;       /* the item type */
    Uint16      item;       /* the item number */
    Uint32      data;       /* the data of the item */
} WND_ELEMENT_DATA;

#define WED_OK              0
#define WED_NEW_DATA        1
#define WED_NOT_CHANGED     2

#define WED_INVARG          -1
#define WED_NODEFINED       -2
#define WED_NOTFOUND        -3
#define WED_MEMERR          -4

extern int get_window_element_data (HWND hwnd, Uint16 type, Uint16 item, Uint32* data);
extern int set_window_element_data (HWND hwnd, Uint16 type, Uint16 item, Uint32 new_data, Uint32* old_data);
extern int free_window_element_data (HWND hwnd);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* GUI_ELEMENT_H */


