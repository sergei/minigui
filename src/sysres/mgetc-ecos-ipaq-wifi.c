/*
 * mgetc-ecos-ipaq-wifi.c
 * configuration file for eCos wifi phone on iPAQ.
 */

#ifndef _SYS_CFG_INCLUDED
    #define _SYS_CFG_INCLUDED

static char *SYSTEM_VALUES[] = {"commlcd", "ipaq", "/dev/ts", "none"};

static char *FBCON_VALUES[] = {"106x120-16bpp"};

static char *SYSTEMFONT_KEYS[] = 
{"font_number", "font0", "font1", "font2", "default", "wchar_def", "fixed", "caption", "menu", "control"};

static char *SYSTEMFONT_VALUES[] = 
{
    "3", "rbf-gothic-rrncnn-6-12-JISX0201-1", "rbf-gothic-rrncnn-12-12-JISX0208-1", 
        "*-fixed-rrncnn-*-16-JISX0208-1",
    "0", "1", "1", "1", "2", "1"
};

#endif /* !_SYS_CFG_INCLUDED */
