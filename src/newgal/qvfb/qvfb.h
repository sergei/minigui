/* $Id: qvfb.h 7354 2007-08-16 05:00:44Z xgwang $
**
** Copyright (C) 2003 ~ 2007 Feynman Software.
** Copyright (C) 2001 ~ 2002 Wei Yongming
*/

#ifndef _GAL_qvfb_h
#define _GAL_qvfb_h

#include <sys/types.h>

#include "sysvideo.h"

/* Hidden "this" pointer for the video functions */
#define _THIS	GAL_VideoDevice *this

#define QT_VFB_MOUSE_PIPE	"/tmp/.qtvfb_mouse-%d"
#define QT_VFB_KEYBOARD_PIPE	"/tmp/.qtvfb_keyboard-%d"

struct QVFbHeader
{
    int width;
    int height;
    int depth;
    int linestep;
    int dataoffset;
    RECT update;
    BYTE dirty;
    int  numcols;
    unsigned int clut[256];
};

struct QVFbKeyData
{
    unsigned int unicode;
    unsigned int modifiers;
    BOOL press;
    BOOL repeat;
};

/* Private display data */
struct GAL_PrivateVideoData {
    unsigned char* shmrgn;
    struct QVFbHeader* hdr;
};

#endif /* _GAL_qvfb_h */

