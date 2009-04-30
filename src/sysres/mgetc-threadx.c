/*
 * mgetc-threadx.c
 * configuration file for ThreadX.
 */

#ifndef _SYS_CFG_INCLUDED
    #define _SYS_CFG_INCLUDED

#ifdef _DUMMY_IAL
static char *SYSTEM_VALUES[] = {"dummy", "dummy", "/dev/mouse", "IMPS2"};
#else
static char *SYSTEM_VALUES[] = {"shadow", "comm", "/dev/mouse", "IMPS2"};
#endif

static char *FBCON_VALUES[] = {"132x176-16bpp"};

static char *SYSTEMFONT_KEYS[] = 
{"font_number", "font0","font1", "default", "wchar_def", "fixed", "caption", "menu", "control"};

static char *SYSTEMFONT_VALUES[] = 
{
    "2","rbf-fixed-rrncnn-6-12-ISO8859-1","*-fixed-rrncnn-*-12-GB2312",
    "0", "1", "1", "1", "1", "1"
};

#endif /* !_SYS_CFG_INCLUDED */
