/*
 * mgetc-em85xx.c
 * configuration file for EM85xx OSD and YUV engines.
 */

#ifndef _SYS_CFG_INCLUDED
    #define _SYS_CFG_INCLUDED

#ifdef _NEWGAL_ENGINE_EM85XXYUV

static char *SYSTEM_VALUES[] = {"em85xxyuv", "em85", "/dev/ir", "none"};
static char *FBCON_VALUES[] = {"720x480-16bpp"};

static char *SYSTEMFONT_KEYS[] = 
{"font_number", "font0", "font1", "font2", "default", "wchar_def", "fixed", "caption", "menu", "control"};

static char *SYSTEMFONT_VALUES[] = 
{
    "3","rbf-fixed-krncnn-12-24-ISO8859-1", "rbf-fixed-krncnn-*-24-GB2312", "rbf-fixed-krncnn-*-24-GB2312",
    "0", "2", "2", "2", "2", "2"
};

#elif defined(_NEWGAL_ENGINE_EM85XXOSD)

static char *SYSTEM_VALUES[] = {"em85xxosd", "em85", "/dev/ir", "none"};
static char *FBCON_VALUES[] = {"640x480-8bpp"};

static char *SYSTEMFONT_KEYS[] = 
{"font_number", "font0", "font1", "font2", "default", "wchar_def", "fixed", "caption", "menu", "control"};

static char *SYSTEMFONT_VALUES[] = 
{
    "3","rbf-fixed-krncnn-12-24-ISO8859-1", "rbf-fixed-krncnn-*-24-GB2312", "rbf-fixed-krncnn-*-24-GB2312",
    "0", "2", "2", "2", "2", "2"
};

#endif

#endif /* !_SYS_CFG_INCLUDED */
