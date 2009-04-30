/*
** $Id: icon.h 7337 2007-08-16 03:44:29Z xgwang $
**
** icon.h: the head file of Icon Support Lib.
**
** Copyright (c) 2003 ~ 2007 Feynman Software.
** Copyright (c) 1999 ~ 2002 Wei Yongming.
**
** Create date: 1999/01/06
*/

#ifndef GUI_ICON_H
    #define GUI_ICON_H

/* Struct definitions */
typedef struct _ICONDIRENTRY {
    BYTE bWidth;
    BYTE bHeight;
    BYTE bColorCount;
    BYTE bReserved;
    WORD wPlanes;
    WORD wBitCount;
    DWORD dwBytesInRes;
    DWORD dwImageOffset;
}ICONDIRENTRY;

#define SIZEOF_ICONDIRENTRY 16
typedef struct _ICONDIR {
    WORD cdReserved;
    WORD cdType;    // must be 1.
    WORD cdCount;
}ICONDIR;

#ifdef _USE_NEWGAL

typedef struct _ICON {
    Uint32  width;
    Uint32  height;
    Uint32  pitch;
    BYTE*   AndBits;
    BYTE*   XorBits;
}ICON;

#else

typedef struct _ICON {
    int     width;
    int     height;
    void*   AndBits;
    void*   XorBits;
}ICON;

#endif /* _USE_NEWGAL */

typedef ICON* PICON;

/* Function definitions */
#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/* Initialization and termination. */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif // GUI_ICON_H

