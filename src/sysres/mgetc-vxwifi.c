/*
 * mgetc-wifi.c
 * configuration file for VxWORKS wifi phone.
 */

#ifndef _SYS_CFG_INCLUDED
    #define _SYS_CFG_INCLUDED

static char *SYSTEM_VALUES[] = {"commlcd", "comm", "/dev/ts", "none"};

static char *FBCON_VALUES[] = {"106x120-16bpp"};

static char *SYSTEMFONT_KEYS[] = 
{"font_number", "font0", "font1", "font2", "font3", "default", "wchar_def", "fixed", "caption", "menu", "control"};

static char *SYSTEMFONT_VALUES[] = 
{
    "4","rbf-fixed-rrncnn-6-12-ISO8859-1", "rbf-gothic-rrncnn-6-12-JISX0201-1", "rbf-gothic-rrncnn-12-12-JISX0208-1", 
        "*-fixed-rrncnn-*-16-JISX0208-1",
    "0", "2", "2", "2", "3", "2"
};

#endif /* !_SYS_CFG_INCLUDED */
