/*
 * mgetc-pc.c
 * system configuration file for Linux-PC.
 */

#ifndef _SYS_CFG_INCLUDED
    #define _SYS_CFG_INCLUDED

#ifdef _AUTO_IAL
static char *SYSTEM_VALUES[] = {"fbcon", "auto", "/dev/mouse", "imps2"};
static char *FBCON_VALUES[] = {"640x480-16bpp"};
#elif defined (_QVFB_IAL)
static char *SYSTEM_VALUES[] = {"qvfb", "qvfb", "/dev/mouse", "imps2"};
static char *FBCON_VALUES[] = {"640x480-16bpp"};
static char *QVFB_VALUES[] = {"640x480-16bpp", "0"};
#define _SYS_CFG_QVFB_VALUES_DEFINED
#else
static char *SYSTEM_VALUES[] = {"fbcon", "console", "/dev/input/mice", "imps2"};
static char *FBCON_VALUES[] = {"640x480-16bpp"};
#endif

static char *SYSTEMFONT_KEYS[] = 
{"font_number", "font0", "font1", "font2", "font3", "default", "wchar_def", "fixed", "caption", "menu", "control"};

static char *SYSTEMFONT_VALUES[] = 
{
    "4","rbf-fixed-rrncnn-8-16-ISO8859-1", "*-fixed-rrncnn-*-16-GB2312", "*-Helvetic-rrncnn-*-16-GB2312", "*-SansSerif-rrncnn-*-16-GB2312", 
    "0", "1", "1", "2", "2", "3"
};

#ifdef __TARGET_MGDESKTOP__
#define BACKGROUND_IMAGE_FILE	"/usr/local/lib/minigui/res/bmp/bk.jpg"
#endif

#endif /* !_SYS_CFG_INCLUDED */

