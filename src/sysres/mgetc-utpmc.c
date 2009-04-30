/*
 * mgetc-utpmc.c
 * configuration file for uClinux utpmc.
 */

#ifndef _SYS_CFG_INCLUDED
    #define _SYS_CFG_INCLUDED

static char *SYSTEM_VALUES[] = {"utpmc", "SMDK2410", "", ""};

static char *UTPMC_VALUES[] = {"320x240-8bpp"};
static char *UTPMC_KEYS[] = {"defaultmode"};


static char *FBCON_VALUES[] = {"320x240-8bpp"};

static char *SYSTEMFONT_KEYS[] = 
{"font_number", "font0", "font1", "font2", "default", "wchar_def", "fixed", "caption", "menu", "control"};

static char *SYSTEMFONT_VALUES[] = 
{
    "3","rbf-fixed-rrncnn-6-12-ISO8859-1", "*-fixed-rrncnn-*-16-GB2312", "*-Helvetica-rrncnn-*-16-GB2312", 
    "0", "1", "1", "2", "2", "2"
};

#endif /* _SYS_CFG_INCLUDED */
