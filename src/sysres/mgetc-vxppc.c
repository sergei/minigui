/*
 * mgetc-vxworks-ppc.c
 * configuration file for VxWorks on PPC.
 */

#ifndef _SYS_CFG_INCLUDED
    #define _SYS_CFG_INCLUDED

#if defined(__VXWORKS__) && defined(__TARGET_PPC__)

#ifdef _COMM_IAL
static char *SYSTEM_VALUES[] = {"commlcd", "comm", "/dev/ts", "none"};
#else
static char *SYSTEM_VALUES[] = {"commlcd", "auto", "/dev/ts", "none"};
#endif
static char *FBCON_VALUES[] = {"640x480-24bpp"};

static char *SYSTEMFONT_KEYS[] = 
{"font_number", "font0", "font1", "font2", "default", "wchar_def", "fixed", "caption", "menu", "control"};

static char *SYSTEMFONT_VALUES[] = 
{
    "3","rbf-fixed-rrncnn-6-12-ISO8859-1", "rbf-fixed-rrncnn-*-16-GB2312", "rbf-fixed-rrncnn-*-12-GB2312", 
    "0", "1", "1", "1", "1", "1"
};
#endif /* __VXWORKS__ && __TARGET_PPC__*/
#endif /* _SYS_CFG_INCLUDED */
