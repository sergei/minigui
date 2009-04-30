/*
** $Id: newtoolbar_impl.h 7338 2007-08-16 03:46:09Z xgwang $
**
** newtoolbar.h: the head file of NewToolBar control module.
**
** Copyright (C) 2003 ~ 2007 Feynman Software.
**
** Create date: 2003/04/24
*/


#ifndef __NEWTOOLBAR_IMPL_H_
#define __NEWTOOLBAR_IMPL_H_

#ifdef  __cplusplus
extern  "C" {
#endif

#define WIDTH_SEPARATOR     5

#define GAP_BMP_TEXT_HORZ   2
#define GAP_BMP_TEXT_VERT   2

#define GAP_ITEM_ITEM_HORZ  2
#define GAP_ITEM_ITEM_VERT  2

#define MARGIN_HORZ         9

#ifdef _FLAT_WINDOW_STYLE
#define MARGIN_VERT         2
#else
#define MARGIN_VERT         3
#endif

#ifdef _FLAT_WINDOW_STYLE
#define MARGIN_V_HORZ       2
#else
#define MARGIN_V_HORZ       3
#endif
#define MARGIN_V_VERT       9

typedef struct ntbItem
{
    struct ntbItem *next;
    struct ntbItem *prev;
    DWORD flags;
    int id;
    int bmp_cell;
    char text[NTB_TEXT_LEN + 1];
    char tip[NTB_TIP_LEN + 1];
    RECT rc_item;
    RECT rc_text;
    RECT rc_hotspot;
    HOTSPOTPROC hotspot_proc;
    DWORD add_data;
} NTBITEM;
typedef NTBITEM* PNTBITEM;

typedef struct ntbCtrlData {
    NTBITEM*        head;
    NTBITEM*        tail;

    DWORD           style;

    BITMAP*         image;
    int             nr_cells;
    int             nr_cols;

    int             w_cell;
    int             h_cell;

    int             nr_items;

    NTBITEM*        sel_item;
    BOOL            btn_down;
} NTBCTRLDATA;
typedef NTBCTRLDATA* PNTBCTRLDATA;

BOOL RegisterNewToolbarControl (void);
void NewToolbarControlCleanup (void);

#ifdef  __cplusplus
}
#endif

#endif /* __NEWTOOLBAR_IMPL_H_ */


