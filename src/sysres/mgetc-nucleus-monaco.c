/*
 * mgetc-nucleus-monaco.c
 * configuration file for Nucleus Monaco.
 */

#ifndef _SYS_CFG_INCLUDED
    #define _SYS_CFG_INCLUDED

static char *SYSTEM_VALUES[] = {"shadow", "comm", "/dev/mouse", "IMPS2"};

static char *FBCON_VALUES[] = {"128x160-16bpp"};

static char *SYSTEMFONT_KEYS[] = 
{"font_number", "font0", "default", "wchar_def", "fixed", "caption", "menu", "control"};

static char *SYSTEMFONT_VALUES[] = 
{
    "2", "rbf-fixed-rrncnn-6-12-ISO8859-1","qpf-song-rrncnn-12-12-GB2312", 
        "0", "1", "1", "1", "1", "1"
};

#endif /* !_SYS_CFG_INCLUDED */
