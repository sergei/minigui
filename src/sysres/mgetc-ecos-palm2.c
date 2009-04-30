/*
 * mgetc-palm2.c
 * configuration file for eCos palm2.
 */

#ifndef _SYS_CFG_INCLUDED
    #define _SYS_CFG_INCLUDED

static char *SYSTEM_VALUES[] = {"commlcd", "palm2", "/dev/ts", "none"};

static char *FBCON_VALUES[] = {"240x320-16bpp"};

#ifdef _PHONE_WINDOW_STYLE
static char *SYSTEMFONT_KEYS[] = 
{"font_number", "font0", "font1", "font2", "font3", "default", "wchar_def", "fixed", "caption", "menu", "control"};

static char *SYSTEMFONT_VALUES[] = 
{
    "4", "vbf-Helvetica-rrncnn-*-21-ISO8859-1", "vbf-Helvetica-rrncnn-*-21-GB2312", "*-Helvetica-rrncnn-*-16-GB2312", "*-fixed-rrncnn-*-16-GB2312",
    "0", "1", "3", "2", "2", "2"
};
#else
static char *SYSTEMFONT_KEYS[] = 
{"font_number", "font0", "font1", "font2", "default", "wchar_def", "fixed", "caption", "menu", "control"};

static char *SYSTEMFONT_VALUES[] = 
{
    "3","rbf-fixed-rrncnn-12-24-ISO8859-1", "vbf-Helvetica-rrncnn-*-27-ISO8859-1", "vbf-Helvetica-rrncnn-*-21-ISO8859-1", 
    "0", "0", "0", "2", "2", "1"
};
#endif

#endif /* !_SYS_CFG_INCLUDED */
