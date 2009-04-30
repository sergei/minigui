/*
 * mgetc-figueroa.c
 * system configuration file for FiguerOA
 */

#ifndef _SYS_CFG_INCLUDED
    #define _SYS_CFG_INCLUDED

static char *SYSTEM_VALUES[] = {"fbcon", "figueroa", "NULL", "NULL"};
static char *FBCON_VALUES[] = {"128x160-16bpp"};

static char *SYSTEMFONT_KEYS[] = 
{"font_number", "font0", "font1", "font2", "default", "wchar_def", "fixed", "caption", "menu", "control"};

static char *SYSTEMFONT_VALUES[] = 
{
    "3","rbf-fixed-rrncnn-8-16-ISO8859-1", "*-fixed-rrncnn-*-16-GB2312", "*-SansSerif-rrncnn-*-16-GB2312", 
    "0", "1", "1", "2", "2", "2"
};

#endif /* !_SYS_CFG_INCLUDED */
