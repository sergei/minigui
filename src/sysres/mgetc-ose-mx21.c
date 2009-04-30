/*
 * mgetc-ose-mx21.c
 * system configuration file for MX21 running OSE.
 */

#ifndef _SYS_CFG_INCLUDED
    #define _SYS_CFG_INCLUDED

static char *SYSTEM_VALUES[] = {"commlcd", "auto", "/dev/mouse", "IMPS2"};

static char *FBCON_VALUES[] = {"240x320-16bpp"};

static char *SYSTEMFONT_KEYS[] = 
{"font_number", "font0","font1", "default", "wchar_def", "fixed", "caption", "menu", "control"};

static char *SYSTEMFONT_VALUES[] = 
{
    "2","rbf-fixed-rrncnn-6-12-ISO8859-1","*-fixed-rrncnn-*-12-GB2312",
    "0", "1", "1", "1", "1", "1"
};

#endif /* !_SYS_CFG_INCLUDED */
