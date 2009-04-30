/*
 * mgetc-ecos-default.c
 * system configuration file for eCos default.
 */

#ifndef _SYS_CFG_INCLUDED
    #define _SYS_CFG_INCLUDED

#ifdef _NEWGAL_ENGINE_QVFB
/* this building is for eCos Linux synthetic */
static char *SYSTEM_VALUES[] = {"qvfb", "qvfb", "/dev/ts", "none"};
static char *FBCON_VALUES[] = {"240x320-16bpp"};
static char *QVFB_VALUES[] = {"240x320-16bpp", "0"};
#define _SYS_CFG_QVFB_VALUES_DEFINED
#elif defined (__TARGET_IPAQ__)
#ifdef _NEWGAL_ENGINE_SHADOW
static char *SYSTEM_VALUES[] = {"shadow", "ipaq", "/dev/ts", "none"};
static char *FBCON_VALUES[] = {"240x320-16bpp"};
#else
static char *SYSTEM_VALUES[] = {"commlcd", "ipaq", "/dev/ts", "none"};
static char *FBCON_VALUES[] = {"320x240-16bpp"};
#endif
#else
static char *SYSTEM_VALUES[] = {"dummy", "dummy", "/dev/ts", "none"};
static char *FBCON_VALUES[] = {"240x320-16bpp"};
#endif

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
