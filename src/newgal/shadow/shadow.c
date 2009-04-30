/*
**  $Id: shadow.c 7513 2007-09-10 09:22:56Z xwyan $
**  
**  shadow.c: Shadow NEWGAL video driver.
**    Can be used to provide support for no-access to frame buffer directly.
**    Can be used to provide support for depth less than 8bpp.
**    Only MiniGUI-Threads supported.
**
**  Copyright (C) 2003 ~ 2007 Feynman Software.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "minigui.h"
#include "newgal.h"
#include "sysvideo.h"
#include "pixels_c.h"

#ifdef _NEWGAL_ENGINE_SHADOW
#include "shadow.h"

#define SHADOWVID_DRIVER_NAME "shadow"

/* Initialization/Query functions */
static int SHADOW_VideoInit (_THIS, GAL_PixelFormat *vformat);
static GAL_Rect **SHADOW_ListModes (_THIS, GAL_PixelFormat *format, Uint32 flags);
static GAL_Surface *SHADOW_SetVideoMode (_THIS, GAL_Surface *current, int width, int height, int bpp, Uint32 flags);
static int SHADOW_SetColors (_THIS, int firstcolor, int ncolors, GAL_Color *colors);
static void SHADOW_VideoQuit (_THIS);

/* Hardware surface functions */
static int SHADOW_AllocHWSurface (_THIS, GAL_Surface *surface);
static void SHADOW_FreeHWSurface (_THIS, GAL_Surface *surface);

/* SHADOW driver bootstrap functions */

static int SHADOW_Available(void)
{
    return 1;
}

static void SHADOW_DeleteDevice(GAL_VideoDevice *device)
{
    free (device->hidden);
    free (device);
}

static void SHADOW_UpdateRects (_THIS, int numrects, GAL_Rect *rects)
{
    int i;
    RECT bound;

    pthread_mutex_lock (&this->hidden->update_lock);

    bound = this->hidden->update;
    for (i = 0; i < numrects; i++) {
        RECT rc;

        SetRect (&rc, rects[i].x, rects[i].y, 
                        rects[i].x + rects[i].w, rects[i].y + rects[i].h);
        if (IsRectEmpty (&bound))
            bound = rc;
        else
            GetBoundRect (&bound, &bound, &rc);
    }

    if (!IsRectEmpty (&bound)) {
        if (IntersectRect (&bound, &bound, &g_rcScr)) {
            this->hidden->update = bound;
            this->hidden->dirty = TRUE;
        }
    }

    pthread_mutex_unlock (&this->hidden->update_lock);
}

static GAL_VideoDevice *SHADOW_CreateDevice(int devindex)
{
    GAL_VideoDevice *device;

    /* Initialize all variables that we clean on shutdown */
    device = (GAL_VideoDevice *)malloc(sizeof(GAL_VideoDevice));
    if (device) {
        memset (device, 0, (sizeof *device));
        device->hidden = (struct GAL_PrivateVideoData *)
                malloc((sizeof *device->hidden));
    }
    if ( (device == NULL) || (device->hidden == NULL)) {
        GAL_OutOfMemory ();
        if (device) {
            free (device);
        }
        return (0);
    }
    memset (device->hidden, 0, (sizeof *device->hidden));

    /* Set the function pointers */
    device->VideoInit = SHADOW_VideoInit;
    device->ListModes = SHADOW_ListModes;
    device->SetVideoMode = SHADOW_SetVideoMode;
    device->CreateYUVOverlay = NULL;
    device->SetColors = SHADOW_SetColors;
    device->VideoQuit = SHADOW_VideoQuit;
    device->AllocHWSurface = SHADOW_AllocHWSurface;
    device->CheckHWBlit = NULL;
    device->FillHWRect = NULL;
    device->SetHWColorKey = NULL;
    device->SetHWAlpha = NULL;
    device->FreeHWSurface = SHADOW_FreeHWSurface;
    device->UpdateRects = SHADOW_UpdateRects;

    device->free = SHADOW_DeleteDevice;
    return device;
}

VideoBootStrap SHADOW_bootstrap = {
    SHADOWVID_DRIVER_NAME, "Shadow LCD video driver",
    SHADOW_Available, SHADOW_CreateDevice
};

static int SHADOW_VideoInit (_THIS, GAL_PixelFormat *vformat)
{
    if (__mg_shadow_lcd_ops.init ())
        return -1;
    
    this->hidden->dirty = FALSE;
    SetRect (&this->hidden->update, 0, 0, 0, 0);

    pthread_mutex_init (&this->hidden->update_lock, NULL);

    /* We're done! */
    return 0;
}

static void* task_do_update (void* data)
{
    _THIS;

    this = data;

    while (this->hidden->fb) {  

        __mg_shadow_lcd_ops.sleep ();

        if (this->hidden->dirty) {

            pthread_mutex_lock (&this->hidden->update_lock);
            __mg_shadow_lcd_ops.refresh (this, &this->hidden->update); 
            SetRect (&this->hidden->update, 0, 0, 0, 0);
            this->hidden->dirty = FALSE;

            pthread_mutex_unlock (&this->hidden->update_lock);
        }
    }
    
    return NULL;
}

static GAL_Surface *SHADOW_SetVideoMode(_THIS, GAL_Surface *current,
                int width, int height, int bpp, Uint32 flags)
{
    int ret;
    struct shadowlcd_info li;
    
    if (__mg_shadow_lcd_ops.getinfo (&li)) {
        fprintf (stderr, "NEWGAL>SHADOW: "
                "Couldn't get the LCD information\n");
        return NULL;
    }

    if (li.fb == NULL) {
        if (li.bpp < 8) li.bpp = 8;

        li.rlen = li.width * ((li.bpp + 7) / 8);
        li.rlen = (li.rlen + 3) & ~3;
        li.fb = malloc (li.rlen * li.height);

        if (!li.fb) {
            if (__mg_shadow_lcd_ops.release)
                 __mg_shadow_lcd_ops.release ();

            fprintf (stderr, "NEWGAL>SHADOW: "
                "Couldn't allocate shadow frame buffer for requested mode\n");
            return (NULL);
        }
        
        this->hidden->alloc_fb = TRUE;
    }
    else
        this->hidden->alloc_fb = FALSE;
    
    memset (li.fb, 0, li.rlen * li.height);

    /* Allocate the new pixel format for the screen */
    if (!GAL_ReallocFormat (current, li.bpp, 0, 0, 0, 0)) {

        if (this->hidden->alloc_fb)
            free (li.fb);

        if (__mg_shadow_lcd_ops.release)
            __mg_shadow_lcd_ops.release ();

        li.fb = NULL;

        fprintf (stderr, "NEWGAL>SHADOW: "
            "Couldn't allocate new pixel format for requested mode");
        return (NULL);
    }

    this->hidden->w = li.width;
    this->hidden->h = li.height;
    this->hidden->pitch = li.rlen;
    this->hidden->fb = li.fb;
    
    /* Set up the new mode framebuffer */
    current->flags = GAL_HWSURFACE | GAL_FULLSCREEN;
    current->w = this->hidden->w;
    current->h = this->hidden->h;
    current->pitch = this->hidden->pitch;
    current->pixels = this->hidden->fb;
    
    {
        pthread_attr_t new_attr;
        pthread_attr_init (&new_attr);
#ifndef __LINUX__
        pthread_attr_setstacksize (&new_attr, 1024);
#endif
        pthread_attr_setdetachstate (&new_attr, PTHREAD_CREATE_DETACHED);
        ret = pthread_create (&this->hidden->update_th, &new_attr, task_do_update, this);
        pthread_attr_destroy (&new_attr);
    }

    if (ret != 0) {
        fprintf (stderr, "NEWGAL>SHADOW: Couldn't start updater\n");
    }

    /* We're done */
    return (current);
}

static void SHADOW_VideoQuit (_THIS)
{
    void* ret_value;

    if (this->hidden->fb) {
        this->hidden->fb = NULL;
        this->hidden->dirty = FALSE;
        pthread_mutex_destroy (&this->hidden->update_lock);
        
        if (this->screen && this->screen->pixels) {
            if (this->hidden->alloc_fb)
                free (this->screen->pixels);

            if (__mg_shadow_lcd_ops.release)
                __mg_shadow_lcd_ops.release ();

            this->screen->pixels = NULL;
        }
    }
}

static GAL_Rect **SHADOW_ListModes (_THIS, GAL_PixelFormat *format, 
                Uint32 flags)
{
    return (GAL_Rect **) -1;
}

/* We don't actually allow hardware surfaces other than the main one */
static int SHADOW_AllocHWSurface (_THIS, GAL_Surface *surface)
{
    return -1;
}

static void SHADOW_FreeHWSurface (_THIS, GAL_Surface *surface)
{
    surface->pixels = NULL;
}

static int SHADOW_SetColors (_THIS, int firstcolor, int ncolors, 
                GAL_Color *colors)
{
    if (__mg_shadow_lcd_ops.setclut)
    	return __mg_shadow_lcd_ops.setclut (firstcolor, ncolors, colors);

    return 0;
}

#endif /* _NEWGAL_ENGINE_SHADOW */

