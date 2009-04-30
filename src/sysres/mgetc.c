/*
 ** $Id: mgetc.c 7590 2007-09-17 09:20:06Z xwyan $
 **
 ** mgetc.c: definitions for incore resource.
 **
 ** Copyright (C) 2003 ~ 2007 Feynman Software.
 **
 ** Create date: 2003/09/22
 */
#include <stdio.h>

#include "common.h"
#include "minigui.h"

#ifdef _INCORE_RES

#define _ETC_CONFIG_EVENT

#ifndef DYNAMIC_LOAD

#ifdef _CUSTOM_IAL
/* 
 * Please modify this file to meet your customer's board 
 * system configuration options.
 */
#include "mgetc-custom.c"
#endif

#ifdef __VXWORKS__
/* 
 * System configure files for boards running VxWorks. 
 * Please move the file which you use to be the first file in this group.
 */
#include "mgetc-vxi386.c"
#include "mgetc-vxwifi.c"
#include "mgetc-vxsim.c"
#include "mgetc-vxppc.c"
#endif

#ifdef __UCOSII__
/* 
 * System configure files for boards running uC/OS-II. 
 * Please move the file which you use to be the first file in this group.
 */
#include "mgetc-ucosii-arm3000.c"
#include "mgetc-ucosii-skyeye.c"
#endif

#ifdef __ECOS__
/* 
 * System configure files for boards running eCos. 
 * Please move the file which you use to be the first file in this group.
 */
#include "mgetc-ecos-default.c"
#include "mgetc-ecos-ipaq-wifi.c"
#include "mgetc-ecos-palm2.c"
#endif

#ifdef __WINBOND_SWLINUX__
#include "mgetc-swlinux.c"
#endif

#ifdef __CYGWIN__
#include "mgetc-cygwin.c"
#endif

#ifdef WIN32
#include "mgetc-win32.c"
#endif

#ifdef __THREADX__
#include "mgetc-threadx.c"
#endif

#ifdef __NUCLEUS__
/* 
 * System configure files for boards running Nucleus. 
 * Please move the file which you use to be the first file in this group.
 */
#include "mgetc-nucleus.c"
#include "mgetc-nucleus-mnt.c"
#include "mgetc-nucleus-monaco.c"
#endif

#ifdef __PSOS__
#include "mgetc-psos-default.c"
#endif

#ifdef __OSE__
#include "mgetc-ose-mx21.c"
#endif

#ifdef __uClinux__
/* 
 * System configure files for boards running uClinux
 * Please move the file which you use to be the first file in this group.
 */
#include "mgetc-bfin.c"
#include "mgetc-axlinux.c"
#include "mgetc-bf533.c"
#include "mgetc-em86.c"
#include "mgetc-em85.c"
#include "mgetc-hh44b0.c"
#include "mgetc-uptech.c"
#include "mgetc-mb93493.c"
#include "mgetc-utpmc.c"
#endif

#ifndef _SYS_CFG_INCLUDED
/* system configure files for boards running Linux */
#   ifdef _IPAQ_IAL
#include "mgetc-ipaq.c"
#   endif
#   ifdef _FIGUEROA_IAL
#include "mgetc-figueroa.c"
#   endif
#   ifdef _FFT7202_IAL
#include "mgetc-fft7202.c"
#   endif
#   ifdef _DM270_IAL
#include "mgetc-dm270.c"
#   endif
#   ifdef _EVMV10_IAL
#include "mgetc-xscale.c"
#   endif
#   ifdef _EMBEST2410_IAL
#include "mgetc-embest2410.c"
#   endif
#   ifdef _FXRM9200_IAL
#include "mgetc-rm9200.c"
#   endif
#   ifdef _HH2410R3_IAL
#include "mgetc-hh2410r3.c"
#   endif
#   ifdef _HH2410R3_IAL
#include "mgetc-hh2440.c"
#   endif

#include "mgetc-pc.c"

#endif /* !_SYS_CFG_INCLUDED */

static char *SYSTEM_KEYS[] = {"gal_engine", "ial_engine", "mdev", "mtype"};

static char *FBCON_KEYS[] = {"defaultmode"};

static char *QVFB_KEYS[] = {"defaultmode", "display"};
#ifndef _SYS_CFG_QVFB_VALUES_DEFINED
static char *QVFB_VALUES[] = {"240x320-16bpp", "0"};
#endif

static char *CURSORINFO_KEYS[] = {"cursornumber"};
static char *CURSORINFO_VALUES[] = {"23"};

static char *ICONINFO_KEYS[] = {"iconnumber"};
static char *ICONINFO_VALUES[] = {"5"};

static char *BITMAPINFO_KEYS[] = {"bitmapnumber"};
static char *BITMAPINFO_VALUES[] = {"6"};
static char *BGPICTURE_KEYS[] = {"position", "file"};

#ifdef BACKGROUND_IMAGE_FILE
static char *BGPICTURE_VALUES[] = {"center", BACKGROUND_IMAGE_FILE};
#else
static char *BGPICTURE_VALUES[] = {"none", ""};
#endif

#ifdef _ETC_CONFIG_EVENT
static char *EVENT_KEYS[] = {"timeoutusec", "repeatusec"};
static char *EVENT_VALUES[] = {"300000", "50000"};
#endif

#ifdef _IME_GB2312
static char* IMEINFO_KEYS[] = {"imetabpath", "imenumber", "ime0"};
static char* IMEINFO_VALUES[] = {"/usr/local/lib/minigui/res/imetab/", "1", "pinyin"};
#endif

#if (defined(_TTF_SUPPORT) || defined(_FT2_SUPPORT)) && defined (__TARGET_MGDESKTOP__)
static char* TTFINFO_KEYS[] = {"font_number", "name0", "fontfile0", "name1", "fontfile1"};
static char* TTFINFO_VALUES[] = {"2", 
        "ttf-arial-rrncnn-0-0-ISO8859-1", "/usr/local/lib/minigui/res/font/arial.ttf",
        "ttf-times-rrncnn-0-0-ISO8859-1", "/usr/local/lib/minigui/res/font/times.ttf"
};
#endif

static ETCSECTION mgetc_sections [] =
{
  {0, 4, "system",       SYSTEM_KEYS,     SYSTEM_VALUES},
  {0, 2, "qvfb",         QVFB_KEYS,       QVFB_VALUES},
  {0, 2, "wvfb",         FBCON_KEYS,      FBCON_VALUES},
  {0, 2, "shadow",       FBCON_KEYS,      FBCON_VALUES},
  {0, 1, "fbcon",        FBCON_KEYS,      FBCON_VALUES},
  {0, 1, "dummy",        FBCON_KEYS,      FBCON_VALUES},
  {0, 1, "em85xxyuv",    FBCON_KEYS,      FBCON_VALUES},
  {0, 1, "em85xxosd",    FBCON_KEYS,      FBCON_VALUES},
  {0, 1, "svpxxosd",     FBCON_KEYS,      FBCON_VALUES},
  {0, 1, "utpmc",        FBCON_KEYS,      FBCON_VALUES},
  {0, 1, "bf533",        FBCON_KEYS,      FBCON_VALUES},
  {0, 1, "mb93493",      FBCON_KEYS,      FBCON_VALUES},
  {0, 1, "commlcd",      FBCON_KEYS,      FBCON_VALUES},
  {0, 1, "dfb",          FBCON_KEYS,      FBCON_VALUES},
  {0, TABLESIZE(SYSTEMFONT_KEYS), "systemfont",  SYSTEMFONT_KEYS, SYSTEMFONT_VALUES},
  {0, 1, "cursorinfo",   CURSORINFO_KEYS, CURSORINFO_VALUES},
  {0, 1, "iconinfo",     ICONINFO_KEYS,   ICONINFO_VALUES},
  {0, 1, "bitmapinfo",   BITMAPINFO_KEYS, BITMAPINFO_VALUES},
/* optional sections */
  {0, 2, "bgpicture", BGPICTURE_KEYS, BGPICTURE_VALUES},
  /*
  {1, "mouse", MOUSE_KEYS, MOUSE_VALUES},
  */
  {0, 2, "event", EVENT_KEYS, EVENT_VALUES},
#ifdef _IME_GB2312
  {0, 3, "imeinfo", IMEINFO_KEYS, IMEINFO_VALUES},
#endif
#if (defined (_TTF_SUPPORT) || defined (_FT2_SUPPORT)) && defined (__TARGET_MGDESKTOP__)
  {0, TABLESIZE(TTFINFO_KEYS), "truetypefonts", TTFINFO_KEYS, TTFINFO_VALUES},
#endif
};

static ETC_S MGETC = {0, TABLESIZE (mgetc_sections), mgetc_sections};

GHANDLE __mg_get_mgetc (void)
{
	return (GHANDLE) &MGETC;
}

#endif /* !DYNAMIC_LOAD */
#endif /* _INCORE_RES */

