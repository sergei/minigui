/*
 * mgetc-cygwin.c
 * configuration file for windows-cygwin.
 */

#ifndef _SYS_CFG_INCLUDED
    #define _SYS_CFG_INCLUDED

static char *SYSTEM_VALUES[] = {"wvfb", "wvfb", "/dev/mouse", "IMPS2"};
static char *FBCON_VALUES[] = {"640x480-16bpp"};

static char *SYSTEMFONT_KEYS[] = 
{"font_number", "font0", "default", "wchar_def", "fixed", "caption", "menu", "control"};

static char *SYSTEMFONT_VALUES[] = 
{
    "1","rbf-fixed-rrncnn-6-12-ISO8859-1",
    "0", "0", "0", "0", "0", "0"
};

#endif /* !_SYS_CFG_INCLUDED */
