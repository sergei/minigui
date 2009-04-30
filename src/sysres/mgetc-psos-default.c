/*
 * mgetc-psos-default.c
 * system configuration file for pSOS.
 */

#ifndef _SYS_CFG_INCLUDED
    #define _SYS_CFG_INCLUDED

static char *SYSTEM_VALUES[] = {"commlcd", "auto", "/dev/ts", "none"};

static char *FBCON_VALUES[] = {"320x240-16bpp"};

static char *SYSTEMFONT_KEYS[] = 
{"font_number", "font0", "font1", "default", "wchar_def", "fixed", "caption", "menu", "control"};

static char *SYSTEMFONT_VALUES[] = 
{
    "2","rbf-fixed-rrncnn-6-12-ISO8859-1", "rbf-fixed-rrncnn-*-12-GB2312", 
    "0", "1", "1", "1", "1", "1"
};

#endif /* !_SYS_CFG_INCLUDED */
