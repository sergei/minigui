/*
 * mgetc-vxi386.c
 * configuration file for VxWorks on i386.
 */

#ifndef _SYS_CFG_INCLUDED
    #define _SYS_CFG_INCLUDED

static char *SYSTEM_VALUES[] = {"commlcd", "comm", "/dev/ts", "none"};
#if 0
/* vga mode */
static char *FBCON_VALUES[] = {"320x200-8bpp"};
#else
/* vesa mode */
static char *FBCON_VALUES[] = {"1024x768-16bpp"};
#endif

static char *SYSTEMFONT_KEYS[] = 
{"font_number", "font0", "font1", "font2", "default", "wchar_def", "fixed", "caption", "menu", "control"};

static char *SYSTEMFONT_VALUES[] = 
{
    "3","rbf-fixed-rrncnn-6-12-ISO8859-1", "rbf-fixed-rrncnn-*-16-GB2312", "rbf-fixed-rrncnn-*-12-GB2312", 
    "0", "1", "1", "1", "1", "1"
};

#endif /* !_SYS_CFG_INCLUDED */
