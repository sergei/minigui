/*
** $Id: resource.c 7861 2007-10-17 07:11:59Z xwyan $
**
** resource.c: This file include some functions for system resource loading. 
**           some functions are from misc.c.
**
** Copyright (C) 2003 ~ 2007 Feynman Software.
** Copyright (C) 1999 ~ 2002 Wei Yongming.
**
** Create date: 2003/09/06
**
** Current maintainer: Wei Yongming.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "common.h"
#include "minigui.h"
#include "gdi.h"
#include "window.h"
#include "cursor.h"
#include "icon.h"
#include "sysres.h"
#include "misc.h"

BITMAP SystemBitmap [SYSBMP_ITEM_NUMBER];
HICON  LargeSystemIcon [SYSICO_ITEM_NUMBER] = {0};
HICON  SmallSystemIcon [SYSICO_ITEM_NUMBER] = {0};

#ifdef USE_DYNAMIC_RESLIB
#include "mgres.h"
StockBitmaps1 *StockBitmaps;

#else
#ifdef _CTRL_BUTTON
#   define _NEED_STOCKBMP_BUTTON            1
#endif

#if defined(_CTRL_COMBOBOX) || defined (_CTRL_MENUBUTTON)
#   define _NEED_STOCKBMP_DOWNARROW         1
#endif

#ifdef _CTRL_COMBOBOX
#   define _NEED_STOCKBMP_DOUBLEARROW       1
#endif

#ifdef _CTRL_LISTBOX
#   define _NEED_STOCKBMP_CHECKMARK         1
#endif

#if defined(_CTRL_TRACKBAR) && defined(_PHONE_WINDOW_STYLE)
#   define _NEED_STOCKBMP_TRACKBAR          1
#endif

#ifdef _EXT_CTRL_SPINBOX
#   define _NEED_STOCKBMP_SPINBOX           1
#endif

#ifdef _IME_GB2312
#   define _NEED_STOCKBMP_IME               1
#endif

#ifdef _MISC_ABOUTDLG
#   define _NEED_STOCKBMP_LOGO              1
#endif

#ifdef _EXT_CTRL_TREEVIEW
#   define _NEED_SYSICON_FOLD               1
#endif

#ifdef _EXT_CTRL_LISTVIEW
#   define _NEED_STOCKBMP_LVFOLD            1
#   define _NEED_SYSICON_FILETYPE           1
#endif

static struct _StockBitmaps {
    const char* name;
    BITMAP bmp;
} StockBitmaps [] =
{
    {STOCKBMP_CAPTION},
#ifdef _NEED_STOCKBMP_BUTTON
    {STOCKBMP_BUTTON},
#endif
#ifdef _NEED_STOCKBMP_DOWNARROW
    {STOCKBMP_DOWNARROW},
#endif
#ifdef _NEED_STOCKBMP_DOUBLEARROW
    {STOCKBMP_UPDOWNARROW},
    {STOCKBMP_LEFTRIGHTARROW},
#endif
#ifdef _NEED_STOCKBMP_CHECKMARK
    {STOCKBMP_CHECKMARK},
#endif
#ifdef _NEED_STOCKBMP_TRACKBAR
    {STOCKBMP_TRACKBAR_HBG},
    {STOCKBMP_TRACKBAR_HSLIDER},
    {STOCKBMP_TRACKBAR_VBG},
    {STOCKBMP_TRACKBAR_VSLIDER},
#endif
#ifdef _NEED_STOCKBMP_SPINBOX
    {STOCKBMP_SPINBOX_HORZ},
    {STOCKBMP_SPINBOX_VERT},
#endif
#ifdef _NEED_STOCKBMP_LVFOLD
    {STOCKBMP_LVFOLD},
    {STOCKBMP_LVUNFOLD},
#endif
#ifdef _NEED_STOCKBMP_IME
    {STOCKBMP_IMECTRLBTN},
#endif
#ifdef _NEED_STOCKBMP_LOGO
    {STOCKBMP_LOGO}
#endif
};

#endif

const BITMAP* GUIAPI GetStockBitmap (const char* name, int ckey_x, int ckey_y)
{
    int i;

    if (!name || name [0] == '\0')
        return NULL;

#ifdef USE_DYNAMIC_RESLIB
    StockBitmaps = get_stock_bitmaps ();
    for (i = 0; i < get_size_stockbitmaps (); i++) {
#else
        for (i = 0; i < TABLESIZE(StockBitmaps); i++) {
#endif
        if (strcmp (name, StockBitmaps [i].name) == 0) {
            PBITMAP bmp = &StockBitmaps [i].bmp;

            if (bmp->bmWidth == 0 || bmp->bmHeight == 0) {
                if (!LoadSystemBitmapEx (bmp, name, ckey_x, ckey_y))
                    return NULL;
            }

            return bmp;
        }
    }

    return NULL;
}

#ifndef _INCORE_RES

/****************************** System resource support *********************/

#ifdef _CURSOR_SUPPORT
PCURSOR LoadSystemCursor (int i)
{
    PCURSOR tempcsr;

    char szValue[MAX_NAME + 1];
    char szPathName[MAX_PATH + 1];
    char szKey[10];

    if (GetMgEtcValue (CURSORSECTION, "cursorpath", szPathName, MAX_PATH) < 0)
                 goto error;

    sprintf (szKey, "cursor%d", i);
    if (GetMgEtcValue (CURSORSECTION, szKey, szValue, MAX_NAME) < 0)
                goto error;

    strcat (szPathName, szValue);

    if (!(tempcsr = (PCURSOR)LoadCursorFromFile (szPathName)))
                     goto error;

    return tempcsr;

error:
    return 0;
}
#endif

BOOL GUIAPI LoadSystemBitmapEx (PBITMAP pBitmap, const char* szItemName, 
                int ckey_x, int ckey_y)
{
    char szPathName[MAX_PATH + 1];
    char szValue[MAX_NAME + 1];
    
    if (GetMgEtcValue ("bitmapinfo", szItemName,
            szValue, MAX_NAME) < 0 ) {
        fprintf (stderr, "LoadSystemBitmapEx: Get bitmap file name error!\n");
        return FALSE;
    }
    
    if (GetMgEtcValue ("bitmapinfo", "bitmappath",
            szPathName, MAX_PATH) < 0 ) {
        fprintf (stderr, "LoadSystemBitmapEx: Get bitmap path error!\n");
        return FALSE;
    }

    if (strcmp (szValue, "none") == 0) {
        memset (pBitmap, 0, sizeof (BITMAP));
        return TRUE;
    }

    strcat(szPathName, szValue);
    
    if (LoadBitmap (HDC_SCREEN, pBitmap, szPathName) < 0) {
        fprintf (stderr, "LoadSystemBitmapEx: Load bitmap error: %s!\n", 
                        szPathName);
        return FALSE;
    }
    
    if (ckey_x >= 0 && ckey_x < pBitmap->bmWidth
            && ckey_y >= 0 && ckey_y < pBitmap->bmHeight) {
        pBitmap->bmType = BMP_TYPE_COLORKEY;
        pBitmap->bmColorKey = GetPixelInBitmap (pBitmap, 0, 0);
    }

    return TRUE;
}

HICON GUIAPI LoadSystemIcon (const char* szItemName, int which)
{
    char szPathName[MAX_PATH + 1];
    char szValue[MAX_NAME + 1];
    HICON hIcon;
    
    if (GetMgEtcValue ("iconinfo", szItemName,
            szValue, MAX_NAME) < 0 ) {
        fprintf (stderr, "LoadSystemIcon: Get icon file name error!\n");
        return 0;
    }
    
    if (GetMgEtcValue ("iconinfo", "iconpath",
            szPathName, MAX_PATH) < 0 ) {
        fprintf (stderr, "LoadSystemIcon: Get icon path error!\n");
        return 0;
    }

    strcat (szPathName, szValue);

    if ((hIcon = LoadIconFromFile (HDC_SCREEN, szPathName, which)) == 0) {
        fprintf (stderr, "LoadSystemIcon: Load icon error: %s!\n", szPathName);
        return 0;
    }
    
    return hIcon;
}

BOOL InitSystemRes (void)
{
    int i;
    int nBmpNr, nIconNr;
    char szValue [12];
    
    /*
     * Load system bitmaps here.
     */
    if (GetMgEtcValue ("bitmapinfo", "bitmapnumber", 
                            szValue, 10) < 0)
        return FALSE;
    nBmpNr = atoi (szValue);
    if (nBmpNr <= 0) return FALSE;
    nBmpNr = nBmpNr < SYSBMP_ITEM_NUMBER ? nBmpNr : SYSBMP_ITEM_NUMBER;

    for (i = 0; i < nBmpNr; i++) {
        sprintf (szValue, "bitmap%d", i);

        if (!LoadSystemBitmap (SystemBitmap + i, szValue))
            return FALSE;
    }

    /*
     * Load system icons here.
     */
    if (GetMgEtcValue ("iconinfo", "iconnumber", 
                            szValue, 10) < 0 )
        return FALSE;
    nIconNr = atoi(szValue);
    if (nIconNr <= 0) return FALSE;
    nIconNr = nIconNr < SYSICO_ITEM_NUMBER ? nIconNr : SYSICO_ITEM_NUMBER;

    for (i = 0; i < nIconNr; i++) {
        sprintf(szValue, "icon%d", i);
        
        SmallSystemIcon [i] = LoadSystemIcon (szValue, 1);
        LargeSystemIcon [i] = LoadSystemIcon (szValue, 0);

        if (SmallSystemIcon [i] == 0 || LargeSystemIcon [i] == 0)
            return FALSE;
    }

    return TRUE;
}

#else /* _INCORE_RES */

#ifndef USE_DYNAMIC_RESLIB

#ifdef _CURSOR_SUPPORT
#   include "cursors.c"
#endif

#ifdef _FLAT_WINDOW_STYLE

#ifdef _NEED_STOCKBMP_BUTTON
#   include "button-flat-bmp.c"
#endif

#ifdef _NEED_STOCKBMP_DOWNARROW
#   include "downarrow-flat-bmp.c"
#endif

#ifdef _NEED_STOCKBMP_DOUBLEARROW
#   include "updownarrow-flat-bmp.c"
#   include "leftrightarrow-flat-bmp.c"
#endif

#ifdef _NEED_STOCKBMP_CHECKMARK
#   include "checkmark-flat-bmp.c"
#endif

#ifdef _NEED_STOCKBMP_SPINBOX
#   include "spinbox-flat-bmp.c"
#endif

#include "bmps-flat.c"

#include "icons-flat.c"

#elif defined (_PHONE_WINDOW_STYLE)

#ifdef _NEED_STOCKBMP_BUTTON
#   include "button-phone-bmp.c"
#endif

#ifdef _NEED_STOCKBMP_DOWNARROW
#   include "downarrow-phone-bmp.c"
#endif

#ifdef _NEED_STOCKBMP_DOUBLEARROW
#   include "updownarrow-phone-bmp.c"
#   include "leftrightarrow-phone-bmp.c"
#endif

#ifdef _NEED_STOCKBMP_CHECKMARK
#   include "checkmark-3d-bmp.c"
#endif

#ifdef _NEED_STOCKBMP_TRACKBAR
#   include "trackbar-phone-bmp.c"
#endif

#ifdef _NEED_STOCKBMP_SPINBOX
#include "spinbox-phone-bmp.c"
#endif

#include "bmps-phone.c"
#include "my_caption.c"

#include "icons-3d.c"

#else

#ifdef _NEED_STOCKBMP_BUTTON
#   include "button-3d-bmp.c"
#endif

#ifdef _NEED_STOCKBMP_DOWNARROW
#   include "downarrow-3d-bmp.c"
#endif

#ifdef _NEED_STOCKBMP_DOUBLEARROW
#   include "updownarrow-3d-bmp.c"
#   include "leftrightarrow-3d-bmp.c"
#endif

#ifdef _NEED_STOCKBMP_CHECKMARK
#   include "checkmark-3d-bmp.c"
#endif

#ifdef _NEED_STOCKBMP_SPINBOX
#   include "spinbox-3d-bmp.c"
#endif

#include "bmps-3d.c"

#include "icons-3d.c"

#endif /* _FLAT_WINDOW_STYLE */

#ifdef _NEED_SYSICON_FOLD

#ifdef _FLAT_WINDOW_STYLE
#   include "fold-ico-flat.c"
#   include "unfold-ico-flat.c"
#else
#   include "fold-ico.c"
#   include "unfold-ico.c"
#endif 

#endif

#ifdef _NEED_SYSICON_FILETYPE

#ifdef _FLAT_WINDOW_STYLE
#   include "filetype-icons-flat.c"
#else
#   include "filetype-icons.c"
#endif

#endif

#ifdef _NEED_STOCKBMP_LVFOLD
#   include "lvfold-bmp.c"
#   include "lvunfold-bmp.c"
#endif

static SYSRES sysres_data [] = {
#ifdef _PHONE_WINDOW_STYLE
        {STOCKBMP_CAPTION, (void*)my_caption_data, sizeof(my_caption_data), 0},
#endif

#ifdef _NEED_STOCKBMP_BUTTON
        {STOCKBMP_BUTTON, (void*)button_bmp_data, sizeof(button_bmp_data), 0},
#endif
#ifdef _NEED_STOCKBMP_DOWNARROW
        {STOCKBMP_DOWNARROW, (void*)downarrow_bmp_data, sizeof(downarrow_bmp_data), 0},
#endif
#ifdef _NEED_STOCKBMP_DOUBLEARROW
        {STOCKBMP_UPDOWNARROW, (void*)updownarrow_bmp_data, sizeof(updownarrow_bmp_data), 0},
        {STOCKBMP_LEFTRIGHTARROW, (void*)leftrightarrow_bmp_data, sizeof(leftrightarrow_bmp_data), 0},
#endif
#ifdef _NEED_STOCKBMP_CHECKMARK
        {STOCKBMP_CHECKMARK, (void*)checkmark_bmp_data, sizeof(checkmark_bmp_data), 0},
#endif
#ifdef _NEED_STOCKBMP_TRACKBAR
        {STOCKBMP_TRACKBAR_HBG, (void*)trackbar_hbg_bmp, sizeof(trackbar_hbg_bmp), 0},
        {STOCKBMP_TRACKBAR_HSLIDER, (void*)trackbar_hslider_bmp, sizeof(trackbar_hslider_bmp), 0},
        {STOCKBMP_TRACKBAR_VBG, (void*)trackbar_vbg_bmp, sizeof(trackbar_vbg_bmp), 0},
        {STOCKBMP_TRACKBAR_VSLIDER, (void*)trackbar_vslider_bmp, sizeof(trackbar_vslider_bmp), 0},
#endif
#ifdef _NEED_STOCKBMP_SPINBOX
        {STOCKBMP_SPINBOX_VERT, (void*)spinbox_vert_bmp_data, sizeof(spinbox_vert_bmp_data), 0},
        {STOCKBMP_SPINBOX_HORZ, (void*)spinbox_horz_bmp_data, sizeof(spinbox_horz_bmp_data), 0},
#endif
#ifdef _NEED_STOCKBMP_LVFOLD
        {STOCKBMP_LVFOLD, (void*)lvfold_bmp_data, sizeof(lvfold_bmp_data), 0},
        {STOCKBMP_LVUNFOLD, (void*)lvunfold_bmp_data, sizeof(lvunfold_bmp_data), 0},
#endif
#ifdef _NEED_STOCKBMP_IME
        {STOCKBMP_IMECTRLBTN, NULL, 0, 0},
#endif
#ifdef _NEED_STOCKBMP_LOGO
        {STOCKBMP_LOGO, NULL, 0, 0},
#endif
#ifdef _NEED_SYSICON_FOLD
        {SYSICON_FOLD, (void*)fold_ico_data, sizeof(fold_ico_data), 0},
        {SYSICON_UNFOLD, (void*)unfold_ico_data, sizeof(unfold_ico_data), 0},
#endif
#ifdef _NEED_SYSICON_FILETYPE
        {SYSICON_FT_DIR, (void*)ft_dir_ico_data, sizeof(ft_dir_ico_data), 0},
        {SYSICON_FT_FILE, (void*)ft_file_ico_data, sizeof(ft_file_ico_data), 0},
#endif
        {"icons", (void*)icons_data, SZ_ICON, 0},
        {"bitmap", (void*)bmps_data, (int)sz_bmps, 1}
};

#define SYSRES_NR (sizeof(sysres_data) / sizeof(SYSRES))

#else
#define SYSRES_NR  get_sysres_num()
#define NR_BMPS get_nr_bmps()
#define NR_ICONS get_nr_icons()
#define SZ_ICON get_sz_icon()
#define NR_CURSORS get_nr_cursors()
static SYSRES *sysres_data;
int* sz_bmps;
const unsigned char *cursors_data;
int *cursors_offset;
#endif
static void* get_res_position (const char* szItemName, int *len)
{
    int i = 0;
    int idx_len = 0;
    int name_len;
    int item_idx = 0;
    const char *pidx;

    if (!szItemName || szItemName[0] == '\0')
        return NULL;

    name_len = strlen (szItemName);
    pidx = szItemName + name_len - 1;
    idx_len = 0;
    while ( isdigit((int)*pidx) )
    {
        idx_len ++;
        if (idx_len == name_len)
            return NULL;
        pidx --;
    }
    name_len -= idx_len;

    if (idx_len > 0)
        item_idx = atoi (szItemName+name_len);

#ifdef USE_DYNAMIC_RESLIB
    sysres_data = set_sysres_data();
#endif
    while ( strncmp(sysres_data[i].name, szItemName, name_len) != 0 && i < SYSRES_NR) i++;
    if (i >= SYSRES_NR)
        return NULL;

    if (sysres_data[i].flag > 0) {
        Uint8* pos = sysres_data[i].res_data;
        int j;
        for (j=0; j<item_idx; j++) {
            pos += *( (int*)sysres_data[i].data_len + j );
        }
        if (len)
            *len = *( (int*)sysres_data[i].data_len + item_idx );
        return pos;
    }
    if (len)
        *len = sysres_data[i].data_len;
    return (Uint8*) sysres_data[i].res_data + sysres_data[i].data_len *item_idx;
}

HICON GUIAPI LoadSystemIcon (const char* szItemName, int which)
{
    void *icon;

    icon = get_res_position (szItemName, NULL);
    if (!icon)
        return 0;

    return LoadIconFromMem (HDC_SCREEN, icon, which);
}

BOOL GUIAPI LoadSystemBitmapEx (PBITMAP pBitmap, const char* szItemName, 
                int ckey_x, int ckey_y)
{
    Uint8* bmpdata;
    int len, nc;

    bmpdata = get_res_position (szItemName, &len);
    if (len == 0) {
        memset (pBitmap, 0, sizeof (BITMAP));
        return TRUE;
    }

    if (!bmpdata) {
        fprintf (stderr, "LoadSystemBitmapEx: Get bitmap data error!\n");
        return FALSE;
    }

    if ((nc = LoadBitmapFromMem (HDC_SCREEN, pBitmap,
                              bmpdata, len, "bmp"))) {
        fprintf (stderr, "LoadSystemBitmapEx: Load bitmap "
                        "error: %p, %d: %d!\n", bmpdata, len, nc);
        return FALSE;
    }

    if (ckey_x >= 0 && ckey_x < pBitmap->bmWidth
            && ckey_y >= 0 && ckey_y < pBitmap->bmHeight) {
        pBitmap->bmType = BMP_TYPE_COLORKEY;
        pBitmap->bmColorKey = GetPixelInBitmap (pBitmap, 0, 0);
    }

    return TRUE;
}

BOOL InitSystemRes (void)
{
    int i;
    const Uint8* bmp;
    const Uint8* icon;

#ifdef USE_DYNAMIC_RESLIB
    bmp = get_bmps_data ();
    icon = get_icons_data ();
    sz_bmps = get_sz_bmps ();
    cursors_offset = (int*)get_cursors_offset();
#else
    bmp = bmps_data;
    icon = icons_data;
    
#endif
    for (i = 0; i < NR_BMPS; i++) {
        if (sz_bmps [i] > 0) {
            if (LoadBitmapFromMem (HDC_SCREEN, SystemBitmap + i, bmp, 
                                sz_bmps [i], "bmp")) {
                fprintf (stderr, "InitSystemRes: error when loading %d "
                                "system bitmap.\n", i);
                return FALSE;
            }
            bmp += sz_bmps [i];
        }
    }

    for (i = 0; i < NR_ICONS; i++) {
        SmallSystemIcon [i] = LoadIconFromMem (HDC_SCREEN, icon, 1);
        LargeSystemIcon [i] = LoadIconFromMem (HDC_SCREEN, icon, 0);

        icon += SZ_ICON;

        if (SmallSystemIcon [i] == 0 || LargeSystemIcon [i] == 0) {
            fprintf (stderr, "InitSystemRes: error when loading %d "
                            "system icon.\n", i);
            return FALSE;
        }
    }

    return TRUE;
}

#ifdef _CURSOR_SUPPORT
/*
static const unsigned char *cursor[NR_CURSORS] = {
    d_arrow,
    d_beam,
    d_pencil,
    d_cross,
    d_move,
    d_sizenwse,
    d_sizenesw,
    d_sizewe,
    d_sizens,
    d_uparrow,
    d_none,
    d_help,
    d_busy,
    d_wait,
    g_rarrow,
    g_col,
    g_row,
    g_drag,
    g_nodrop,
    h_point,
    h_select,
    ho_split,
    ve_split
};
*/

PCURSOR LoadSystemCursor (int i)
{
    if (i >= NR_CURSORS)
        return NULL;

#ifdef USE_DYNAMIC_RESLIB
    cursors_data = get_cursors_data ();
#else
    return (PCURSOR)LoadCursorFromMem (cursors_data + cursors_offset [i]);
#endif
}
#endif

#endif /* _INCORE_RES */

void TerminateStockBitmap (void)
{
    int i;
    
    for (i = 0; i < TABLESIZE(StockBitmaps); i++) {
        PBITMAP bmp = &StockBitmaps [i].bmp;
        if (bmp->bmWidth != 0 && bmp->bmHeight != 0) {
            UnloadBitmap (bmp);
        }
    }
}

void TerminateSystemRes (void)
{
    int i;
    
    TerminateStockBitmap ();

    for (i=0; i<SYSBMP_ITEM_NUMBER; i++)
        UnloadBitmap (SystemBitmap + i);

    for (i=0; i<SYSICO_ITEM_NUMBER; i++) {
        if (SmallSystemIcon [i])
            DestroyIcon (SmallSystemIcon [i]);

        if (LargeSystemIcon [i])
            DestroyIcon (LargeSystemIcon [i]);
    }
}

