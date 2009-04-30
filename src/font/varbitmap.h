/*
** $Id: varbitmap.h 7330 2007-08-16 03:17:58Z xgwang $
**
** varbitmap.h: the head file of raw bitmap font operation set.
**
** Copyright (C) 2000 ~ 2002, Wei Yongming.
** Copyright (C) 2003 ~ 2007  Feynman Software.
** 
*/

#ifndef GUI_FONT_RAWBITMAP_H
    #define GUI_FONT_RAWBITMAP_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

typedef struct
{
    unsigned char   version;        /* font version */
    const char*     name;           /* font name */
    unsigned char   max_width;      /* max width in pixels */
    unsigned char   ave_width;      /* average width in pixels */
    int             height;         /* height in pixels */
    int             descent;        /* pixels below the base line */
    unsigned char   first_char;     /* first character in this font */
    unsigned char   last_char;      /* last character in this font */
    unsigned char   def_char;       /* default character in this font */
    const unsigned short* offset;   /* character glyph offsets into bitmap data or NULL */
    const unsigned char*  width;    /* character widths or NULL */
    const unsigned char*  bits;     /* 8-bit right-padded bitmap data */
    unsigned int    font_size;      /* used by mmap. It should be zero for in-core vbfs. */
    int*            bbox_x;
    int*            bbox_y;
    int*            bbox_w;
    int*            bbox_h;
} VBFINFO;

#define LEN_VERSION_INFO    10

#define VBF_VERSION         "vbf-1.0**"
#define VBF_VERSION2        "vbf-2.0**"

#define SBC_VARFONT_INFO(logfont) ((VBFINFO*)(((DEVFONT*) (logfont.sbc_devfont))->data))
#define MBC_VARFONT_INFO(logfont) ((VBFINFO*)(((DEVFONT*) (logfont.mbc_devfont))->data))

#define SBC_VARFONT_INFO_P(logfont) ((VBFINFO*)(((DEVFONT*) (logfont->sbc_devfont))->data))
#define MBC_VARFONT_INFO_P(logfont) ((VBFINFO*)(((DEVFONT*) (logfont->mbc_devfont))->data))

#define VARFONT_INFO_P(devfont) ((VBFINFO*)(devfont->data))
#define VARFONT_INFO(devfont) ((VBFINFO*)(devfont.data))

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif // GUI_FONT_VARBITMAP_H

