/*
 * mgetc-swlinux.c
 * configuration file for swlinux.
 */


#ifndef _SYS_CFG_INCLUDED
    #define _SYS_CFG_INCLUDED

static char *SYSTEM_VALUES[] = {"svpxxosd", "svpxx", "/dev/mouse", "IMPS2"};

static char *FBCON_VALUES[] = {"640x480-16bpp"};

static char *SYSTEMFONT_KEYS[] = 
{"font_number", "font0", "font1", "font2", "default", "wchar_def", "fixed", "caption", "menu", "control"};

static char *SYSTEMFONT_VALUES[] = 
{
    "3","rbf-fixed-rrncnn-6-12-ISO8859-1", "*-fixed-rrncnn-*-16-GB2312", "*-Helvetica-rrncnn-*-16-GB2312", 
    "0", "1", "1", "2", "2", "2"
};

#ifdef _SVPXX_IAL
static char *EVENT_KEYS[] = {"timeoutusec", "repeatusec"};
static char *EVENT_VALUES[] = {"100000", "50000"};
#ifdef _ETC_CONFIG_EVENT
  #undef _ETC_CONFIG_EVENT
#endif
#endif

#endif /* !_SYS_CFG_INCLUDED */
