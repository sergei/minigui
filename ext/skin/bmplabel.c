/*
** $Id: bmplabel.c 7372 2007-08-16 05:33:55Z xgwang $
**
** bmplabel.c: skin bitmap lable implementation.
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
#include "item_comm.h"

#define EMPTY   ""
/*type : si_nrmlabel_t or si_bmplabel_t */
#define DECL_LABEL(type,ret) \
    type *mylabel = (type *) item->type_data; \
    if (!mylabel) return ret;

#define MAX_COL  20
#define SetValue(Var, Value)    if ( Var ){ *Var = Value;}

/* get bmp char's width height, and total col and row */
static int get_char_bmp_size (skin_item_t *item, int *w, int *h, int *col, int *row)
{
    int _w, _h, _col, _row;
    DECL_LABEL ( si_bmplabel_t, 0 );

    /* get total number of col and row */
    if (strlen(mylabel->label_chars) > MAX_COL) {
        _col =  MAX_COL;
        _row = (strlen (mylabel->label_chars) - 1) / MAX_COL + 1;
    }
    else {
        _col = strlen (mylabel->label_chars);
        _row = 1;
    }
    _w = ITEMBMP(item).bmWidth / _col ;
    _h = ITEMBMP(item).bmHeight/ _row ;

    SetValue ( w, _w );
    SetValue ( h, _h );
    SetValue ( col, _col );
    SetValue ( row, _row );

    return 1;
}

static DWORD get_label (skin_item_t *item)
{
    DECL_LABEL ( si_bmplabel_t, (DWORD)NULL );

    return (DWORD)mylabel->label;
}

#define BMP_MAX(a,b)   (a > b ? a : b)
static DWORD set_label (skin_item_t *item, DWORD label)
{
    SIZE Size;
    RECT Rect;
    char new_label[1024] = {0};
    
    DECL_LABEL ( si_bmplabel_t, (DWORD)FALSE );

    if ( (char *)label )    strcpy (new_label, (char *)label );

    /* new single bmp's size */
    get_char_bmp_size (item, &Size.cx, &Size.cy, NULL, NULL);

    /* Rect is the rect area that we should erase */
    memcpy (&Rect, &item->rc_hittest, sizeof(RECT) );
    Rect.right  = Rect.left + BMP_MAX( RECTW(Rect), Size.cx * strlen(new_label) );
    Rect.bottom = Rect.top  + BMP_MAX( RECTH(Rect), Size.cy );

    /* set new label */
    if (mylabel->label) free (mylabel->label);
        mylabel->label = strdup (new_label);
    
    /* new label's hittest rect */
    item->rc_hittest.right  = item->rc_hittest.left + Size.cx * strlen(new_label);
    item->rc_hittest.bottom = item->rc_hittest.top  + Size.cy;

    /* show new label */
    InvalidateRect ( item->hostskin->hwnd, &Rect, TRUE);

    /* we should refresh it's region also ... */

    return TRUE;
}

static int bmplabel_on_create (skin_item_t* item)
{
    SIZE Size;
    DECL_LABEL ( si_bmplabel_t, 0 );

    /* init label's hittest rectangle  */
    get_char_bmp_size (item, &Size.cx, &Size.cy, NULL, NULL);

    item->rc_hittest.right  = item->rc_hittest.left + Size.cx * strlen(mylabel->label);
    item->rc_hittest.bottom = item->rc_hittest.top  + Size.cy;

    return 1;
}

static int get_char_bmp_pos (skin_item_t* item, char ch, int* x, int* y, int* w, int* h)
{
    int i;
    int row, col;
    DECL_LABEL ( si_bmplabel_t, 0 );

    /* searches for the ch */
    for ( i = 0 ; mylabel->label_chars[i] != '\0' ; i++) {
        if (mylabel->label_chars[i] == ch)
            break;
    }
    if ( i >= strlen(mylabel->label_chars) )    /* not found */
        return 0;

    get_char_bmp_size (item, w, h, &col, &row);

    *x = (i % col) * (*w);
    *y = (i / col) * (*h);

    return 1;
}

static int bmplabel_init (skin_head_t* skin, skin_item_t *item)
{
    DECL_LABEL ( si_bmplabel_t, 0);

    mylabel->label = strdup ( mylabel->label ? mylabel->label : EMPTY );

    return 1;
}

static int bmplabel_deinit (skin_item_t *item)
{
    DECL_LABEL ( si_bmplabel_t, 0);

    if (mylabel->label) free (mylabel->label);

    return 1;
}

static void bmplabel_draw_attached (HDC hdc, skin_item_t* item)
{
    int x, y, w, h;
    char *p;
    int xx;
    BITMAP *bmp;

    si_bmplabel_t *mylabel = (si_bmplabel_t *) item->type_data;
    if ( !mylabel ) return;

    bmp = (BITMAP *)&ITEMBMP(item);
       
    //bmp->bmType = BMP_TYPE_COLORKEY;
    //bmp->bmColorKey = GetPixelInBitmap (&ITEMBMP(item), 10, 10);

    p = mylabel->label;
    xx = item->x;
    while (p && *p != '\0') {
        get_char_bmp_pos (item, *p, &x, &y, &w, &h);
        FillBoxWithBitmapPart (    hdc, xx, item->y, w, h, 0, 0, 
                                &ITEMBMP(item), x, y);
        p++;
        xx += w;
    }
}

static skin_item_ops_t bmplabel_ops = {
    bmplabel_init,
    bmplabel_deinit,
    bmplabel_on_create ,
    NULL,
    NULL,
    bmplabel_draw_attached,
    get_label,
    set_label,
    NULL
};

skin_item_ops_t *BMPLABEL_OPS = &bmplabel_ops;

#endif /* _EXT_SKIN */
