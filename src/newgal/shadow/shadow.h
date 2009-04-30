/*
**  $Id: shadow.h 7427 2007-08-21 06:00:11Z weiym $
**  
**  Copyright (C) 2005 ~ 2007 Feynman Software.
*/

#ifndef _GAL_SHADOW_H
#define _GAL_SHADOW_H

#include "sysvideo.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/* Hidden "this" pointer for the video functions */
#define _THIS    GAL_VideoDevice *this

/* Private display data */

struct GAL_PrivateVideoData {
    int w, h, pitch;
    void *fb;
    BOOL alloc_fb;
    BOOL dirty;
    RECT update;
    pthread_t update_th;
    pthread_mutex_t update_lock;
};

struct shadowlcd_info {
    short height, width;  // Pixels
    short bpp;            // Depth (bits/pixel)
    short type;           // pixel type
    short rlen;           // Length of one raster line in bytes
    void  *fb;            // Frame buffer
};

struct shadow_lcd_ops {
    /* Return value: Zero on ok */
    int (*init) (void);
    /* Return value: Zero on ok. */
    int (*getinfo) (struct shadowlcd_info* lcd_info);
    /* Nullable, Should be set when getinfo is no NULL. Return value: Zero on ok */
    int (*release) (void);
    /* Nullable. Return value: The number of colors set, zeor on error. */
    int (*setclut) (int firstcolor, int ncolors, GAL_Color *colors);

    void (*sleep) (void);
    void (*refresh) (_THIS, const RECT* update);
};

extern struct shadow_lcd_ops __mg_shadow_lcd_ops;

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* _GAL_SHADOW_H */
