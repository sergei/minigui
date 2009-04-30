/*
** $Id: syscfg.c 7896 2007-10-22 04:07:12Z xwyan $
**
** sysfg.c: This file include some functions for getting 
**      system configuration value. 
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

#include "common.h"
#include "minigui.h"
#include "gdi.h"
#include "window.h"

#include "misc.h"
#include "cursor.h"
#include "icon.h"
#include "event.h"
#include "sysres.h"

gal_pixel WinElementColors [WEC_ITEM_NUMBER];

int WinMainMetrics [MWM_ITEM_NUMBER];
#ifndef _INCORE_RES

int WinMainMetrics [MWM_ITEM_NUMBER];

static const char* szMWMKeyNames [MWM_ITEM_NUMBER] = {
        "minwidth",
        "minheight",
        "border",
        "thickframe",
        "thinframe",
        "captiony",
        "iconx",
        "icony",
        "menubary",
        "menubaroffx",
        "menubaroffy",
        "menuitemy",
        "intermenuitemx",
        "intermenuitemy",
        "menuitemoffx",
        "menutopmargin",
        "menubottommargin",
        "menuleftmargin",
        "menurightmargin",
        "menuitemminx",
        "menuseparatory",
        "menuseparatorx",
        "sb_height",
        "sb_width",
        "sb_interx",
        "cxvscroll",
        "cyvscroll",
        "cxhscroll",
        "cyhscroll",
        "minbarlen",
        "defbarlen"
};


static const char* szWECKeyNames [WEC_ITEM_NUMBER] =
{
        "bkc_caption_normal",
        "fgc_caption_normal",
        "bkc_caption_actived",
        "fgc_caption_actived",
        "bkc_caption_disabled",
        "fgc_caption_disabled",
        "wec_frame_normal",
        "wec_frame_actived",
        "wec_frame_disabled",
        "bkc_menubar_normal",
        "fgc_menubar_normal",
        "bkc_menubar_hilite",
        "fgc_menubar_hilite",
        "fgc_menubar_disabled",
        "bkc_menuitem_normal",
        "fgc_menuitem_normal",
        "bkc_menuitem_hilite",
        "fgc_menuitem_hilite",
        "fgc_menuitem_disabled",
        "bkc_pppmenutitle",
        "fgc_pppmenutitle",
        "fgc_menuitem_frame",
        "wec_3dbox_normal",
        "wec_3dbox_reverse",
        "wec_3dbox_light",
        "wec_3dbox_dark",
        "wec_flat_border",
        "bkc_control_def",
        "fgc_control_normal",
        "fgc_control_disabled",
        "bkc_hilight_normal",
        "bkc_hilight_lostfocus",
        "fgc_hilight_normal",
        "fgc_hilight_disabled",
        "bkc_desktop",
        "bkc_dialog",
        "bkc_tip",
};

BOOL InitWindowElementColors (void)
{
    int i;
    unsigned long int rgb;

    for (i = 0; i < WEC_ITEM_NUMBER; i++) {

        char buff [20];

        if (GetMgEtcValue ("windowelementcolors", szWECKeyNames [i], 
                buff, 12) != ETC_OK)
            return FALSE;

        if ((rgb = strtoul (buff, NULL, 0)) > 0x00FFFFFF)
            return FALSE;
        WinElementColors [i] = RGB2Pixel (HDC_SCREEN, 
                GetRValue (rgb), GetGValue(rgb), GetBValue (rgb));
    }

    return TRUE;
}
#else /* _INCORE_RES */
static char* WinMainMetrics1 [MWM_ITEM_NUMBER] =
{
    "1", "1",
    "2", "2", "1",
    "+5", "16", "16",
    "+0", "8", "5", "+0",
    "16", "2", "18", "4", "4",
    "4", "4", "64", "4", "4", 
#if defined _PHONE_WINDOW_STYLE
    "16",
#else 
    "14",
#endif
     "16", "2", 
#ifdef _FLAT_WINDOW_STYLE
    "9", "9", "9", "9",
#elif defined (_PHONE_WINDOW_STYLE)
    "16", "16", "16", "16",
#else
    "12", "12", "12", "12", 
#endif
    "8",
    "18",
};

#ifdef _FLAT_WINDOW_STYLE


static const unsigned long __mg_wec [WEC_ITEM_NUMBER] =
{
    0x00FFFFFF, 0x00000000, 0x00000000,
    0x00FFFFFF, 0x00808080, 0x00FFFFFF,
    0x00000000, 0x00FFFFFF, 0x00000000,
    0x00FFFFFF, 0x00000000, 0x00000000,
    0x00FFFFFF, 0x00808080, 0x00FFFFFF,
    0x00000000, 0x00000000, 0x00FFFFFF,
    0x00808080, 0x00808080, 0x00000000,
    0x00000000, 0x00FFFFFF, 0x00000000,
    0x00FFFFFF, 0x00000000, 0x00000000,
    0x00FFFFFF, 0x00000000, 0x00808080,
    0x00000000, 0x00000000, 0x00FFFFFF,
    0x00808080, 0x00FFFFFF, 0x00FFFFFF,
    0x00808080,
};

#elif defined (_PHONE_WINDOW_STYLE)

static const unsigned long __mg_wec [WEC_ITEM_NUMBER] =
{
    0x00808080, 0x00C0C0C0, 0x00CE2418,
    0x00FFFFFF, 0x00808080, 0x00C0C0C0,
    0x002E2B2A, 0x00FFFFFF, 0x00000000,
    0x00FFFFFF, 0x00000000, 0x00000000,
    0x00FFFFFF, 0x00808080, 0x00FFFFFF,
    0x00000000, 0x00EFD3C6, 0x00000000,
    0x00808080, 0x00C0C0C0, 0x00CE2418,
    0x00C66931, 0x00FFFFFF, 0x00000000,
    0x00FFFFFF, 0x00000000, 0x002E2B2A,
    0x00E5F5FE, 0x00000000, 0x00808080,
    0x00FF0000, 0x00BDA69C, 0x00FFFFFF,
    0x00C0C0C0, 0x00E5F5FE, 0x00E5F5FE,
    0x00C8FCF8,
};

#else   /* _PC3D_WINDOW_STYLE */

static const unsigned long __mg_wec [WEC_ITEM_NUMBER] =
{
    0x00808080, 0x00C8D0D4, 0x006A240A,
    0x00FFFFFF, 0x00808080, 0x00C8D0D4,
    0x00FFFFFF, 0x00FFFFFF, 0x003704EA,
    0x00CED3D6, 0x00000000, 0x003704EA,
    0x00FFFFFF, 0x00848284, 0x00CED3D6,
    0x00000000, 0x006B2408, 0x00FFFFFF,
    0x00848284, 0x00C0C0C0, 0x006B2408,
    0x00C66931, 0x00CED3D6, 0x00000000,
    0x00FFFFFF, 0x00808080, 0x00808080,
    0x00CED3D6, 0x00000000, 0x00848284,
    0x006B2408, 0x00BDA69C, 0x00FFFFFF,
    0x00C0C0C0, 0x00C08000, 0x00CED3D6,
    0x00E7FFFF,
};
#endif /* _PC3D_WINDOW_STYLE */

BOOL InitWindowElementColors (void)
{
    int i;
    unsigned long int rgb;

    for (i = 0; i < WEC_ITEM_NUMBER; i++) {
        rgb = __mg_wec[i];
        WinElementColors [i] = RGB2Pixel (HDC_SCREEN, 
                GetRValue (rgb), GetGValue(rgb), GetBValue (rgb));
    }

#ifdef __TARGET_STB810__
    WinElementColors [34] = RGBA2Pixel (HDC_SCREEN, 0, 0, 0, 0);
#endif
    return TRUE;
}
#endif /* _INCORE_RES */

BOOL InitMainWinMetrics (void)
{
    int i, tmp, height_sysfont;
    char buff [21];

    height_sysfont = GetSysCharHeight ();

    for (i = 0; i < MWM_ITEM_NUMBER; i++) {
#ifndef _INCORE_RES
        if (GetMgEtcValue ("mainwinmetrics", szMWMKeyNames [i], 
                buff, 20) != ETC_OK)
            return FALSE;
        tmp = atoi (buff + 1);
#else
        memcpy(buff, WinMainMetrics1[i], sizeof(WinMainMetrics1[i]));
        tmp = atoi (buff);
#endif
        if (i < 8)
            height_sysfont = GetSystemFont (SYSLOGFONT_CAPTION)->size;
        else if (i < 22)
            height_sysfont = GetSystemFont (SYSLOGFONT_MENU)->size;

        switch (buff [0]) {
        case '+':
            WinMainMetrics [i] = height_sysfont + tmp;
            break;
        case '-':
            WinMainMetrics [i] = height_sysfont - tmp;
            break;
        case '*':
            WinMainMetrics [i] = height_sysfont * tmp;
            break;
        case '/':
            WinMainMetrics [i] = height_sysfont / tmp;
            break;
        case '%':
            WinMainMetrics [i] = height_sysfont % tmp;
            break;
        default:
            WinMainMetrics [i] = atoi (buff);
            break;
        }
    }

    return TRUE;
}


