/*
** $Id: devfont.h 9810 2008-03-14 03:13:00Z zhounuohua $
**
** devfont.h: the head file of Device Font Manager.
**
** Copyright (C) 2003 ~ 2007 Feynman Software.
** Copyright (C) 2000 ~ 2002 Wei Yongming.
**
** Create date: 2000/xx/xx
*/

#ifndef GUI_DEVFONT_H
    #define GUI_DEVFONT_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/* defined in varbitmap.c */
#ifdef _VBF_SUPPORT
BOOL InitIncoreVBFonts (void);
void TermIncoreVBFonts (void);

BOOL InitVarBitmapFonts (void);
void TermVarBitmapFonts (void);
#endif

/* defined in rawbitmap.c */
#ifdef _RBF_SUPPORT

#ifdef _INCORE_RES
BOOL InitIncoreRBFonts (void);
void TermIncoreRBFonts (void);
#else
BOOL InitRawBitmapFonts (void);
void TermRawBitmapFonts (void);
#endif /* _INCORE_RES */

#endif

/* defined in qpf.c */
#ifdef _QPF_SUPPORT
BOOL InitQPFonts (void);
void TermQPFonts (void);
BOOL InitIncoreQPFonts (void);
void TermIncoreQPFonts (void);
#endif

/* defined in freetype.c or freetype2.c */
#if defined (_TTF_SUPPORT) || defined (_FT2_SUPPORT)
BOOL InitFreeTypeFonts (void);
void TermFreeTypeFonts (void);
#endif

/* defined in type1.c */
#ifdef _TYPE1_SUPPORT
BOOL InitType1Fonts (void);
void TermType1Fonts (void);
#endif

/* Device font management functions */
void AddSBDevFont (DEVFONT* dev_font);
void AddMBDevFont (DEVFONT* dev_font);
void DelRelatedDevFont (DEVFONT* dev_font);
void ResetDevFont (void);

void DelSBDevFont (DEVFONT* dev_font);    
void DelMBDevFont (DEVFONT* dev_font);
void DelDevFont (DEVFONT* dev_font, BOOL del_relate);

DEVFONT* GetMatchedSBDevFont (LOGFONT* log_font);
DEVFONT* GetMatchedMBDevFont (LOGFONT* log_font);

#define GET_DEVFONT_SCALE(logfont, devfont) \
        ((devfont->charset_ops->bytes_maxlen_char > 1)?logfont->mbc_scale:logfont->sbc_scale)

#define SET_DEVFONT_SCALE(logfont, devfont, scale) \
        ((devfont->charset_ops->bytes_maxlen_char > 1)?(logfont->mbc_scale = scale):(logfont->sbc_scale = scale))

#ifdef _USE_NEWGAL
unsigned short GetBestScaleFactor (int height, int expect);
#endif

#ifdef _DEBUG
void dumpDevFonts (void);
#endif

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif // GUI_DEVFONT_H

