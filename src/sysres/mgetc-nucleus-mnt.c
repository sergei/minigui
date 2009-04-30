/*
 * mgetc-nucleus-mnt.c
 * configuration file for Nucleus MNT simulator.
 */

#ifndef _SYS_CFG_INCLUDED
    #define _SYS_CFG_INCLUDED

static char *SYSTEM_VALUES[] = {"wvfb", "auto", "/dev/ts", "none"};

static char *FBCON_VALUES[] = {"640x480-16bpp"};

static char *SYSTEMFONT_KEYS[] = 
{"font_number", "font0", "font1", "font2", "default", "wchar_def", "fixed", "caption", "menu", "control"};

static char *SYSTEMFONT_VALUES[] = 
{
    "3","rbf-fixed-rrncnn-6-12-ISO8859-1", "rbf-fixed-rrncnn-*-12-GB2312", "rbf-fixed-rrncnn-*-16-GB2312", 
    "0", "1", "1", "1", "1", "1"
};

#endif /* !_SYS_CFG_INCLUDED */
