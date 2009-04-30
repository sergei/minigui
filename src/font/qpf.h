/*
** $Id: qpf.h 8100 2007-11-16 08:45:38Z xwyan $
**
** qpf.h: the head file of Qt Prerendered Font operation set.
**
** Copyright (C) 2003 ~ 2007 Feynman Software.
**
** All right reserved by Feynman Software.
**
** Create date: 2003/01/28
*/

#ifndef GUI_FONT_QPF_H
    #define GUI_FONT_QPF_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define FM_SMOOTH   1

#ifdef __NOUNIX__
typedef struct _GLYPHMETRICS {
#else
typedef struct __attribute__ ((packed)) _GLYPHMETRICS {
#endif
    Uint8 linestep;
    Uint8 width;
    Uint8 height;
    Uint8 padding;

    Sint8 bearingx;     /* Difference from pen position to glyph's left bbox */
    Uint8 advance;      /* Difference between pen positions */
    Sint8 bearingy;     /* Used for putting characters on baseline */

    Sint8 reserved;     /* Do not use */
} GLYPHMETRICS;

typedef struct _GLYPH
{
    const GLYPHMETRICS* metrics;
    const unsigned char* data;
} GLYPH;

typedef struct _GLYPHTREE
{
    unsigned int min, max;
    struct _GLYPHTREE* less;
    struct _GLYPHTREE* more;
    GLYPH* glyph;
#ifdef _INCORE_RES
    const GLYPHMETRICS* metrics;
    const unsigned char* data;
#endif
} GLYPHTREE;

#ifdef __NOUNIX__
typedef struct _QPFMETRICS
#else
typedef struct __attribute__ ((packed)) _QPFMETRICS
#endif
{
    Sint8 ascent, descent;
    Sint8 leftbearing, rightbearing;
    Uint8 maxwidth;
    Sint8 leading;
    Uint8 flags;
    Uint8 underlinepos;
    Uint8 underlinewidth;
    Uint8 reserved3;
} QPFMETRICS;

typedef struct 
{
    char font_name [LEN_UNIDEVFONT_NAME + 1];
    unsigned int height;
    unsigned int width;

    unsigned int file_size;
    QPFMETRICS* fm;

    GLYPHTREE* tree;

#if 0
    int max_bmp_size;
    unsigned char* std_bmp;
#endif
} QPFINFO;

extern FONTOPS qpf_font_ops;

BOOL LoadQPFont (const char* file, QPFINFO* QPFInfo);
void UnloadQPFont (QPFINFO* QPFInfo);

const DEVFONT* __mg_qpfload_devfont_fromfile(
                          const char *devfont_name, const char *file_name);
void __mg_qpf_destroy_devfont_fromfile (DEVFONT **devfont);

#define SBC_QPFONT_INFO(logfont) ((QPFINFO*)(((DEVFONT*) (logfont.sbc_devfont))->data))
#define MBC_QPFONT_INFO(logfont) ((QPFINFO*)(((DEVFONT*) (logfont.mbc_devfont))->data))

#define SBC_QPFONT_INFO_P(logfont) ((QPFINFO*)(((DEVFONT*) (logfont->sbc_devfont))->data))
#define MBC_QPFONT_INFO_P(logfont) ((QPFINFO*)(((DEVFONT*) (logfont->mbc_devfont))->data))

#define QPFONT_INFO_P(devfont) ((QPFINFO*)(devfont->data))
#define QPFONT_INFO(devfont) ((QPFINFO*)(devfont.data))

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* GUI_FONT_QPF_H */


