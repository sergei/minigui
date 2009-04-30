/*
** $Id: rawbitmap.h 7330 2007-08-16 03:17:58Z xgwang $
**
** rawbitmap.h: the head file of raw bitmap font operation set.
**
** Copyright (C) 2000 ~ 2002 Wei Yongming.
** Copyright (C) 2003 ~ 2007 Feynman Software.
**
*/

#ifndef GUI_FONT_RAWBITMAP_H
    #define GUI_FONT_RAWBITMAP_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

typedef struct
{
    CHARSETOPS* charset_ops;
    int nr_chars;
    int width;
    int height;
    unsigned char* font;
    long font_size;
} RBFINFO;

extern FONTOPS raw_bitmap_font_ops;

BOOL LoadRawBitmapFont (const char* file, RBFINFO* RBFInfo);
void UnloadRawBitmapFont (RBFINFO* RBFInfo);

#define SBC_RBFONT_INFO(logfont) ((RBFINFO*)(((DEVFONT*) (logfont.sbc_devfont))->data))
#define MBC_RBFONT_INFO(logfont) ((RBFINFO*)(((DEVFONT*) (logfont.mbc_devfont))->data))

#define SBC_RBFONT_INFO_P(logfont) ((RBFINFO*)(((DEVFONT*) (logfont->sbc_devfont))->data))
#define MBC_RBFONT_INFO_P(logfont) ((RBFINFO*)(((DEVFONT*) (logfont->mbc_devfont))->data))

#define RBFONT_INFO_P(devfont) ((RBFINFO*)(devfont->data))
#define RBFONT_INFO(devfont) ((RBFINFO*)(devfont.data))

#ifdef _INCORE_RES

typedef struct
{
    const char* name;
    const unsigned char* data;
} INCORE_RBFINFO;

#endif /* _INCORE_RES */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif // GUI_FONT_RAWBITMAP_H

