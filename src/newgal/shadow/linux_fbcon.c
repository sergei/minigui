/*
**  $Id: linux_fbcon.c 7427 2007-08-21 06:00:11Z weiym $
**  
**  linux_fbcon.c: A subdriver of shadow NEWGAL engine for Linux 
**      FrameBuffer console.
**
**  Copyright (C) 2003 ~ 2007 Feynman Software.
**
**  This subdriver can support packed-pixel 1bpp/4bpp/8bpp modes
**  and can do the coordinate transformation.
**
**  TODO:
**      * add support for FB_TYPE_VGA_PLANES.
**      * add support for PE_MSBRIGHT;
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

#ifdef _NEWGAL_ENGINE_SHADOW 

#if defined (__LINUX__) && defined (__TARGET_FBCON__)

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <limits.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <linux/vt.h>

#undef ROP_XOR

#include "minigui.h"
#include "newgal.h"
#include "shadow.h"

#define PF_PALETTE  1
#define PF_DIRECT   2

#define PE_MSBLEFT  1
#define PE_MSBRIGHT 2

static struct _fbcon_info
{
    int fd_fb;
#ifdef _HAVE_TEXT_MODE
    int fd_tty;
#endif

    gal_uint8 type;
    gal_uint8 pixel_endiness;
    gal_uint8 pixel_format;
    gal_uint8 depth;
    gal_uint8 bpp; /* bytes per pixel */
    gal_uint8 planes;

    gal_uint16 xres;
    gal_uint16 yres;

    gal_uint32 pitch;
    gal_uint32 fb_size;

    gal_uint8* fb;
    gal_uint8* a_line;

    void (*put_one_line) (const BYTE* src_line, BYTE* dst_line, 
                    int x, int width);
} fbcon_info = {-1, -1};

static unsigned char pixel_bit [] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};

static void put_one_line_1bpp (const BYTE* src_bits, BYTE* dst_line,
                int left, int width)
{
    int x, i;
    dst_line += left >> 3;

    for (x = 0; x < width; x += 8) {
        *dst_line = 0;
        for (i = 0; i < 8; i++) {
            if (*src_bits < 128)
                *dst_line |= pixel_bit [i];
            src_bits ++;
        }
        dst_line ++;
    }
}

static void put_one_line_2bpp (const BYTE* src_bits, BYTE* dst_line,
                int left, int width)
{
    int x;
    dst_line += left >> 2;

    for (x = 0; x < width; x += 4) {
        *dst_line = (*src_bits >> 6); src_bits ++;
        *dst_line |= ((*src_bits >> 4) & 0x0C); src_bits ++;
        *dst_line |= ((*src_bits >> 2) & 0x30); src_bits ++;
        *dst_line |= ((*src_bits) & 0xC0); src_bits ++;
        dst_line ++;
    }
}

static void put_one_line_4bpp (const BYTE* src_bits, BYTE* dst_line,
                int left, int width)
{
    int x;
    dst_line += left >> 1;

    for (x = 0; x < width; x+=2) {
        *dst_line = (src_bits [0] >> 4) | (src_bits [1] & 0xF0);
        src_bits += 2;
        dst_line ++;
    }
}

static void put_one_line_8bpp (const BYTE* src_bits, BYTE* dst_line,
                int left, int width)
{
    dst_line += left * fbcon_info.bpp;

    memcpy (dst_line, src_bits, width * fbcon_info.bpp);
}

#ifdef _COOR_TRANS

#   if _ROT_DIR_CW == 1

static void _get_dst_rect (RECT* dst_rect, const RECT* src_rect)
{
    dst_rect->left = fbcon_info.xres - src_rect->bottom;
    dst_rect->top = src_rect->left;
    dst_rect->right = fbcon_info.xres - src_rect->top;
    dst_rect->bottom = src_rect->right;
}

static void a_refresh_cw (_THIS, const RECT* update)
{
    RECT src_update = *update;
    RECT dst_update;
    const BYTE* src_bits;
    BYTE* dst_line;
    int dst_width, dst_height;
    int x, y;

    _get_dst_rect (&dst_update, &src_update);

    /* Round the update rectangle.  */
    if (fbcon_info.depth == 1) {
        /* round x to be 8-aligned */
        dst_update.left = dst_update.left & ~0x07;
        dst_update.right = (dst_update.right + 7) & ~0x07;
    }
    else if (fbcon_info.depth == 2) {
        /* round x to be 4-aligned */
        dst_update.left = dst_update.left & ~0x03;
        dst_update.right = (dst_update.right + 3) & ~0x03;
    }
    else if (fbcon_info.depth == 4) {
        /* round x to be 2-aligned */
        dst_update.left = dst_update.left & ~0x01;
        dst_update.right = (dst_update.right + 1) & ~0x01;
    }

    dst_width = RECTW (dst_update);
    dst_height = RECTH (dst_update);

    if (dst_width <= 0 || dst_height <= 0)
        return;

    /* Copy the bits from Shadow FrameBuffer to console FrameBuffer */
    src_bits = this->hidden->fb;
    src_bits += (src_update.bottom - 1) * this->hidden->pitch 
                + src_update.left * fbcon_info.bpp;

    dst_line = fbcon_info.fb + dst_update.top * fbcon_info.pitch;

    for (x = 0; x < dst_height; x++) {
        /* Copy the bits from vertical line to horizontal line */
        const BYTE* ver_bits = src_bits;
        BYTE* hor_bits = fbcon_info.a_line;

        switch (fbcon_info.depth) {
            case 32:
                for (y = 0; y < dst_width; y++) {
                    *(gal_uint32*) hor_bits = *(gal_uint32*) ver_bits;
                    ver_bits -= this->hidden->pitch;
                    hor_bits += fbcon_info.bpp;
                }
                break;

           case 24:
                for (y = 0; y < dst_width; y++) {
                    hor_bits [0] = ver_bits [0];
                    hor_bits [1] = ver_bits [1];
                    hor_bits [2] = ver_bits [2];
                    ver_bits -= this->hidden->pitch;
                    hor_bits += fbcon_info.bpp;
                }
                break;

           case 15:
           case 16:
                for (y = 0; y < dst_width; y++) {
                    *(gal_uint16*) hor_bits = *(gal_uint16*) ver_bits;
                    ver_bits -= this->hidden->pitch;
                    hor_bits += fbcon_info.bpp;
                }
                break;

           default:
                for (y = 0; y < dst_width; y++) {
                    *hor_bits = *ver_bits;
                    ver_bits -= this->hidden->pitch;
                    hor_bits += fbcon_info.bpp;
                }
                break;
        }

        fbcon_info.put_one_line (fbcon_info.a_line, dst_line, 
                        dst_update.left, dst_width);

        src_bits += fbcon_info.bpp;
        dst_line += fbcon_info.pitch;
    }
}

#   else /* _ROT_DIR_CW == 1 */

static void _get_dst_rect (RECT* dst_rect, const RECT* src_rect)
{
    dst_rect->left = src_rect->top;
    dst_rect->bottom = fbcon_info.yres - src_rect->left;
    dst_rect->right = src_rect->bottom;
    dst_rect->top = fbcon_info.yres - src_rect->right;
}

static void a_refresh_ccw (_THIS, const RECT* update)
{
    RECT src_update = *update;
    RECT dst_update;
    const BYTE* src_bits;
    BYTE* dst_line;
    int dst_width, dst_height;
    int x, y;

    _get_dst_rect (&dst_update, &src_update);

    /* Round the update rectangle.  */
    if (fbcon_info.depth == 1) {
        /* round x to be 8-aligned */
        dst_update.left = dst_update.left & ~0x07;
        dst_update.right = (dst_update.right + 7) & ~0x07;
    }
    else if (fbcon_info.depth == 2) {
        /* round x to be 4-aligned */
        dst_update.left = dst_update.left & ~0x03;
        dst_update.right = (dst_update.right + 3) & ~0x03;
    }
    else if (fbcon_info.depth == 4) {
        /* round x to be 2-aligned */
        dst_update.left = dst_update.left & ~0x01;
        dst_update.right = (dst_update.right + 1) & ~0x01;
    }

    dst_width = RECTW (dst_update);
    dst_height = RECTH (dst_update);

    if (dst_width <= 0 || dst_height <= 0)
        return;

    /* Copy the bits from Shadow FrameBuffer to console FrameBuffer */
    src_bits = this->hidden->fb;
    src_bits += src_update.top * this->hidden->pitch 
                + src_update.left * fbcon_info.bpp;

    dst_line = fbcon_info.fb + (dst_update.bottom - 1) * fbcon_info.pitch;

    for (x = 0; x < dst_height; x++) {
        /* Copy the bits from vertical line to horizontal line */
        const BYTE* ver_bits = src_bits;
        BYTE* hor_bits = fbcon_info.a_line;

        switch (fbcon_info.depth) {
            case 32:
                for (y = 0; y < dst_width; y++) {
                    *(gal_uint32*) hor_bits = *(gal_uint32*) ver_bits;
                    ver_bits += this->hidden->pitch;
                    hor_bits += fbcon_info.bpp;
                }
                break;

           case 24:
                for (y = 0; y < dst_width; y++) {
                    hor_bits [0] = ver_bits [0];
                    hor_bits [1] = ver_bits [1];
                    hor_bits [2] = ver_bits [2];
                    ver_bits += this->hidden->pitch;
                    hor_bits += fbcon_info.bpp;
                }
                break;

           case 15:
           case 16:
                for (y = 0; y < dst_width; y++) {
                    *(gal_uint16*) hor_bits = *(gal_uint16*) ver_bits;
                    ver_bits += this->hidden->pitch;
                    hor_bits += fbcon_info.bpp;
                }
                break;

           default:
                for (y = 0; y < dst_width; y++) {
                    *hor_bits = *ver_bits;
                    ver_bits += this->hidden->pitch;
                    hor_bits += fbcon_info.bpp;
                }
                break;

        }

        fbcon_info.put_one_line (fbcon_info.a_line, dst_line, 
                        dst_update.left, dst_width);

        src_bits += fbcon_info.bpp;
        dst_line -= fbcon_info.pitch;
    }
}

#   endif /* _ROT_DIR_CW == 0 */

#else /* _COOR_TRANS */

static void a_refresh_normal (_THIS, const RECT* update)
{
    RECT src_update = *update;
    const BYTE* src_bits;
    BYTE* dst_line;
    int width, height;
    int y;

    if (fbcon_info.depth >= 8) {
        return;
    }

    /* Round the update rectangle.  */
    if (fbcon_info.depth == 1) {
        /* round x to be 8-aligned */
        src_update.left = src_update.left & ~0x07;
        src_update.right = (src_update.right + 7) & ~0x07;
    }
    else if (fbcon_info.depth == 2) {
        /* round x to be 4-aligned */
        src_update.left = src_update.left & ~0x03;
        src_update.right = (src_update.right + 3) & ~0x03;
    }
    else if (fbcon_info.depth == 4) {
        /* round x to be 2-aligned */
        src_update.left = src_update.left & ~0x01;
        src_update.right = (src_update.right + 1) & ~0x01;
    }

    width = RECTW (src_update);
    height = RECTH (src_update);

    if (width <= 0 || height <= 0)
        return;

    /* Copy the bits from Shadow FrameBuffer to console FrameBuffer */

    src_bits = this->hidden->fb;
    src_bits += src_update.top * this->hidden->pitch + src_update.left;

    dst_line = fbcon_info.fb + src_update.top * fbcon_info.pitch;

    for (y = 0; y < height; y++) {
        fbcon_info.put_one_line (src_bits, dst_line, src_update.left, width);
        src_bits += this->hidden->pitch;
        dst_line += fbcon_info.pitch;
    }
}

#endif /* !_COOR_TRANS */

static int a_getinfo (struct shadowlcd_info* lcd_info)
{
#ifdef _COOR_TRANS
    lcd_info->width = fbcon_info.yres;
    lcd_info->height = fbcon_info.xres;
    lcd_info->fb = NULL;

    fbcon_info.a_line = malloc (fbcon_info.xres * fbcon_info.bpp);
    if (fbcon_info.a_line == NULL)
        return 1;

#   if _ROT_DIR_CW == 1
    __mg_shadow_lcd_ops.refresh = a_refresh_cw;
#   else
    __mg_shadow_lcd_ops.refresh = a_refresh_ccw;
#   endif

#else /*  _COOR_TRANS */
    lcd_info->width = fbcon_info.xres;
    lcd_info->height = fbcon_info.yres;
    if (fbcon_info.depth >= 8) {
        lcd_info->fb = fbcon_info.fb;
        lcd_info->rlen = fbcon_info.pitch;
    }
    else
        lcd_info->fb = NULL;

    __mg_shadow_lcd_ops.refresh = a_refresh_normal;
#endif /* !_COOR_TRANS */

    lcd_info->bpp = fbcon_info.depth;

    switch (fbcon_info.depth) {
        case 1:
            fbcon_info.put_one_line = put_one_line_1bpp;
            break;
        case 2:
            fbcon_info.put_one_line = put_one_line_2bpp;
            break;
        case 4:
            fbcon_info.put_one_line = put_one_line_4bpp;
            break;
        default:
            fbcon_info.put_one_line = put_one_line_8bpp;
            break;
    }

    lcd_info->type = 0;
    return 0;
}

/* original hw palette */
static unsigned short saved_red[16];
static unsigned short saved_green[16];
static unsigned short saved_blue[16];

/* get framebuffer palette */
static void ioctl_getpalette (int start, int len, 
                unsigned short *red, 
                unsigned short *green, 
                unsigned short *blue)
{
	struct fb_cmap cmap;

    if (fbcon_info.pixel_format != PF_PALETTE)
        return;

	cmap.start = start;
	cmap.len = len;
	cmap.red = red;
	cmap.green = green;
	cmap.blue = blue;
	cmap.transp = NULL;

	ioctl (fbcon_info.fd_fb, FBIOGETCMAP, &cmap);
}

/* set framebuffer palette */
static void ioctl_setpalette (int start, int len, 
                unsigned short *red, 
                unsigned short *green, 
                unsigned short *blue)
{
	struct fb_cmap cmap;

    if (fbcon_info.pixel_format != PF_PALETTE)
        return;

	cmap.start = start;
	cmap.len = len;
	cmap.red = red;
	cmap.green = green;
	cmap.blue = blue;
	cmap.transp = NULL;

	ioctl (fbcon_info.fd_fb, FBIOPUTCMAP, &cmap);
}

static int a_setclut_1bpp (int first, int ncolors, GAL_Color *colors)
{
    int i, entry = first;
    int set = 0;
    unsigned short r, g, b;

    for (i = 0; i < ncolors; i++) {
        if (entry == 0 || entry == 255) {
            set++;
            r = colors[i].r << 8;
            g = colors[i].g << 8;
            b = colors[i].b << 8;
            ioctl_setpalette (entry/255, 1, &r, &g, &b);
        }
        entry ++;
    }

    return set;
}

static int a_setclut_2bpp (int first, int ncolors, GAL_Color *colors)
{
    int i, entry = first;
    int set = 0;
    unsigned short r, g, b;

    for (i = 0; i < ncolors; i++) {
        if (entry == 0 || (entry + 1) % 64 == 0) {
            set++;
            r = colors[i].r << 8;
            g = colors[i].g << 8;
            b = colors[i].b << 8;
            ioctl_setpalette (entry/64, 1, &r, &g, &b);
        }
        entry ++;
    }

    return set;
}

static int a_setclut_4bpp (int first, int ncolors, GAL_Color *colors)
{
    int i, entry = first;
    int set = 0;
    unsigned short r, g, b;

    for (i = 0; i < ncolors; i++) {
        if (entry == 0 || ((entry + 1) % 16 == 0)) {
            set++;
            r = colors[i].r << 8;
            g = colors[i].g << 8;
            b = colors[i].b << 8;
            ioctl_setpalette (entry/16, 1, &r, &g, &b);
        }
        entry ++;
    }

    return set;
}

static int a_setclut_8bpp (int first, int ncolors, GAL_Color *colors)
{
    int i, entry = first;
    int set = 0;
    unsigned short r, g, b;

    for (i = 0; i < ncolors; i++) {
        if (entry < 256) {

            r = colors[i].r << 8;
            g = colors[i].g << 8;
            b = colors[i].b << 8;
            ioctl_setpalette (entry, 1, &r, &g, &b);

            set++;
            entry++;
        }
    }

    return set;
}

static int a_init (void)
{
    char *env;
    int err_value = 0;
    struct fb_fix_screeninfo fb_fix;
    struct fb_var_screeninfo fb_var;

    /* locate and open framebuffer, get info*/
    err_value++;
    if (!(env = getenv ("FRAMEBUFFER")))
        env = "/dev/fb0";
    fbcon_info.fd_fb = open (env, O_RDWR);
    if (fbcon_info.fd_fb < 0) {
        goto fail;
    }

    err_value++;
    if (ioctl (fbcon_info.fd_fb, FBIOGET_FSCREENINFO, &fb_fix) == -1 ||
            ioctl (fbcon_info.fd_fb, FBIOGET_VSCREENINFO, &fb_var) == -1) {
        goto fail;
    }

    /* setup screen device from framebuffer info*/
    fbcon_info.type = fb_fix.type;

    fbcon_info.xres = fb_var.xres;
    fbcon_info.yres = fb_var.yres;

    /* set planes from fb type*/
    if (fbcon_info.type == FB_TYPE_VGA_PLANES)
        fbcon_info.planes = 4;
    else if (fbcon_info.type == FB_TYPE_PACKED_PIXELS)
        fbcon_info.planes = 1;
    else
        fbcon_info.planes = 0;

    fbcon_info.depth = fb_var.bits_per_pixel;

    /* set pitch to byte length, possibly converted later*/
    fbcon_info.pitch = fb_fix.line_length;

    if (fb_var.red.msb_right)
        fbcon_info.pixel_endiness = PE_MSBRIGHT;
    else
        fbcon_info.pixel_endiness = PE_MSBLEFT;

    /* set pixel format*/
    if (fb_fix.visual == FB_VISUAL_TRUECOLOR 
                    || fb_fix.visual == FB_VISUAL_DIRECTCOLOR) {
        fbcon_info.pixel_format = PF_DIRECT;
    }
    else 
        fbcon_info.pixel_format = PF_PALETTE;

    /* select the setclut operation based on bpp */
    switch (fbcon_info.depth) {
        case 1:
            __mg_shadow_lcd_ops.setclut = a_setclut_1bpp;
            break;
        case 2:
            __mg_shadow_lcd_ops.setclut = a_setclut_2bpp;
            break;
        case 4:
            __mg_shadow_lcd_ops.setclut = a_setclut_4bpp;
            break;
        case 8:
            __mg_shadow_lcd_ops.setclut = a_setclut_8bpp;
            break;
    }

#ifdef _HAVE_TEXT_MODE
    {
        /* open tty, enter graphics mode*/
        char* tty_dev;
        if (geteuid() == 0)
            tty_dev = "/dev/tty0";
        else    /* not a super user, so try to open the control terminal */
            tty_dev = "/dev/tty";

        err_value++;
        fbcon_info.fd_tty = open (tty_dev, O_RDWR);
        if (fbcon_info.fd_tty < 0) {
            goto fail;
        }

        err_value++;
        if (ioctl (fbcon_info.fd_tty, KDSETMODE, KD_GRAPHICS) == -1) {
            goto fail;
        }
    }
#endif

    /* mmap framebuffer into this address space*/
    fbcon_info.fb_size = fbcon_info.pitch * fbcon_info.yres;
    fbcon_info.fb_size = (fbcon_info.fb_size + getpagesize () - 1) 
            / getpagesize () * getpagesize ();

    fbcon_info.fb = 
#ifdef _FXRM9200_IAL /* workaround for Fuxu RM9200 */
        mmap (NULL, fbcon_info.fb_size, PROT_READ | PROT_WRITE, MAP_SHARED,
                        fbcon_info.fd_fb, 0);
#elif defined (__uClinux__)
        mmap (NULL, fbcon_info.fb_size, PROT_READ | PROT_WRITE, 0, 
                        fbcon_info.fd_fb, 0);
#else
        mmap (NULL, fbcon_info.fb_size, PROT_READ | PROT_WRITE, MAP_SHARED, 
                        fbcon_info.fd_fb, 0);
#endif

    err_value++;
    if (fbcon_info.fb == NULL || fbcon_info.fb == (unsigned char *)-1) {
        goto fail;
    }

    switch (fbcon_info.depth) {
        case 32: fbcon_info.bpp = 4; break;
        case 24: fbcon_info.bpp = 3; break;
        case 16: fbcon_info.bpp = 2; break;
        case 15: fbcon_info.bpp = 2; break;
        default: fbcon_info.bpp = 1; break;
    }

    /* save original palette*/
    ioctl_getpalette (0, 16, saved_red, saved_green, saved_blue);

    return 0;    /* success*/

fail:

#ifdef _HAVE_TEXT_MODE
    /* enter text mode */
    if (fbcon_info.fd_tty >= 0) {
        ioctl (fbcon_info.fd_tty, KDSETMODE, KD_TEXT);
        close (fbcon_info.fd_tty);
        fbcon_info.fd_tty = -1;
   }
#endif

    if (fbcon_info.fd_fb >= 0) {
        close (fbcon_info.fd_fb);
        fbcon_info.fd_fb = -1;
    }

    return err_value;
}


static int a_release (void)
{
    /* reset hw palette */
    ioctl_setpalette (0, 16, saved_red, saved_green, saved_blue);
  
    /* unmap framebuffer */
    munmap (fbcon_info.fb, fbcon_info.fb_size);
  
#ifdef _HAVE_TEXT_MODE
    /* enter text mode */
    if (fbcon_info.fd_tty >= 0) {
        ioctl (fbcon_info.fd_tty, KDSETMODE, KD_TEXT);
        close (fbcon_info.fd_tty);
        fbcon_info.fd_tty = -1;
    }
#endif

    /* close framebuffer */
    if (fbcon_info.fd_fb >= 0) {
        close (fbcon_info.fd_fb);
        fbcon_info.fd_fb = -1;
    }

    if (fbcon_info.a_line) {
        free (fbcon_info.a_line);
        fbcon_info.a_line = NULL;
    }
    return 0;
}

static void a_sleep (void)
{
    /* 20ms */
    usleep (20000);
}

struct shadow_lcd_ops __mg_shadow_lcd_ops = {
    a_init,
    a_getinfo,
    a_release,
    NULL,
    a_sleep,
    NULL
};

#endif /* __LINUX__ && __TARGET_FBCON__ */

#endif /* _NEWGAL_ENGINE_SHADOW */

