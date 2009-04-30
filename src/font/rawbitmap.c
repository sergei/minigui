/*
** $Id: rawbitmap.c 7330 2007-08-16 03:17:58Z xgwang $
** 
** rawbitmap.c: The Raw Bitmap Font operation set.
**
** Copyright (C) 2003 ~ 2007 Feynman Software.
** Copyright (C) 2000 ~ 2002 Wei Yongming.
**
** All right reserved by Feynman Software.
**
** Current maintainer: Wei Yongming.
**
** Create date: 2000/06/13
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "minigui.h"
#include "gdi.h"
#include "misc.h"

#ifndef _INCORE_RES
#ifndef __NOUNIX__
#    include <sys/stat.h>
#    include <sys/types.h>
#    include <fcntl.h>
#    include <unistd.h>
#endif
#endif

#ifdef HAVE_MMAP
    #include <sys/mman.h>
#endif

#include "charset.h"
#include "devfont.h"
#include "rawbitmap.h"
#include "fontname.h"

#ifdef _RBF_SUPPORT

#ifdef _INCORE_RES

#ifndef DYNAMIC_LOAD

extern INCORE_RBFINFO __mgif_fixed6x12_iso8859_1;

#ifdef _INCORERBF_LATIN1_16
extern INCORE_RBFINFO __mgif_fixed8x16_iso8859_1;
#endif

#ifdef _INCORERBF_GB12
extern INCORE_RBFINFO __mgif_song12_gb2312;
#endif

#ifdef _INCORERBF_GB16
extern INCORE_RBFINFO __mgif_song16_gb2312;
#endif

#ifdef _INCORERBF_BIG5_12
extern INCORE_RBFINFO __mgif_ming12_big5;
#endif

#ifdef _INCORERBF_BIG5_16
extern INCORE_RBFINFO __mgif_ming16_big5;
#endif

static INCORE_RBFINFO* incore_rbfonts [] = {
    &__mgif_fixed6x12_iso8859_1,
#ifdef _INCORERBF_LATIN1_16
    &__mgif_fixed8x16_iso8859_1,
#endif
#ifdef _INCORERBF_GB12
    &__mgif_song12_gb2312,
#endif
#ifdef _INCORERBF_GB16
    &__mgif_song16_gb2312,
#endif
#ifdef  _INCORERBF_BIG5_12
    &__mgif_ming12_big5,
#endif
#ifdef  _INCORERBF_BIG5_16
    &__mgif_ming16_big5,
#endif
};

#define NR_RBFONTS  (sizeof (incore_rbfonts) /sizeof (INCORE_RBFINFO*))

#else /* !DYNAMIC_LOAD */

#include "rawfont.h"

#define NR_RBFONTS  get_rbf_size() 

static INCORE_RBFINFO** incore_rbfonts;

#endif /* DYNAMIC_LOAD */

static DEVFONT* incore_rbf_dev_fonts;
static RBFINFO* incore_rbf_infos;

BOOL InitIncoreRBFonts (void)
{
    int i;

#ifdef DYNAMIC_LOAD
    incore_rbfonts = set_rbf();
#endif

    incore_rbf_dev_fonts = calloc (NR_RBFONTS, sizeof (DEVFONT));
    incore_rbf_infos = calloc (NR_RBFONTS, sizeof (RBFINFO));

    if (!incore_rbf_dev_fonts || !incore_rbf_infos)
        return TRUE;

    for (i = 0; i < NR_RBFONTS; i++) {
        char charset [LEN_FONT_NAME + 1];

        if (!fontGetCharsetFromName (incore_rbfonts [i]->name, charset)) {
            fprintf (stderr, "GDI: Invalid font name (charset): %s.\n", 
                    incore_rbfonts [i]->name);
            goto error_load;
        }

        if ((incore_rbf_infos [i].charset_ops 
               = GetCharsetOpsEx (charset)) == NULL) {
            fprintf (stderr, "GDI: Not supported charset: %s.\n", charset);
            goto error_load;
        }

        if ((incore_rbf_infos [i].width = fontGetWidthFromName (incore_rbfonts [i]->name)) == -1) {
            fprintf (stderr, "GDI: Invalid font name (width): %s.\n", 
                    incore_rbfonts [i]->name);
            goto error_load;
        }
        
        if ((incore_rbf_infos [i].height = fontGetHeightFromName (incore_rbfonts [i]->name)) == -1) {
            fprintf (stderr, "GDI: Invalid font name (height): %s.\n",
                    incore_rbfonts [i]->name);
            goto error_load;
        }
        
        incore_rbf_infos [i].nr_chars = incore_rbf_infos [i].charset_ops->nr_chars;

        incore_rbf_infos [i].font_size = ((incore_rbf_infos [i].width + 7) >> 3) * 
                incore_rbf_infos [i].height * incore_rbf_infos [i].nr_chars;
        incore_rbf_infos [i].font = (unsigned char*) incore_rbfonts [i]->data;

        strncpy (incore_rbf_dev_fonts[i].name, incore_rbfonts [i]->name, LEN_DEVFONT_NAME);
        incore_rbf_dev_fonts[i].name [LEN_DEVFONT_NAME] = '\0';
        incore_rbf_dev_fonts[i].font_ops = &raw_bitmap_font_ops;
        incore_rbf_dev_fonts[i].charset_ops = incore_rbf_infos [i].charset_ops;
        incore_rbf_dev_fonts[i].data = incore_rbf_infos + i;
#if 0
        fprintf (stderr, "GDI: RBFDevFont %i: %s.\n", i, incore_rbf_dev_fonts[i].name);
#endif
    }

    for (i = 0; i < NR_RBFONTS; i++) {
        if (incore_rbf_infos [i].charset_ops->bytes_maxlen_char > 1)
            AddMBDevFont (incore_rbf_dev_fonts + i);
        else
            AddSBDevFont (incore_rbf_dev_fonts + i);
    }

    return TRUE;

error_load:
    fprintf (stderr, "GDI: Error in initializing incore raw bitmap fonts!\n");

    TermIncoreRBFonts ();
    return FALSE;
}

void TermIncoreRBFonts (void)
{
    free (incore_rbf_dev_fonts);
    incore_rbf_dev_fonts = NULL;

    free (incore_rbf_infos);
    incore_rbf_infos = NULL;
}

#else

/********************** Load/Unload of raw bitmap font ***********************/
BOOL LoadRawBitmapFont (const char* file, RBFINFO* RBFInfo)
{
#ifdef HAVE_MMAP
    int fd;
#else
    FILE* fp;
#endif
    size_t size;

    size = ((RBFInfo->width + 7) >> 3) * RBFInfo->height * RBFInfo->nr_chars;
    RBFInfo->font_size = size;

#ifdef HAVE_MMAP
    if ((fd = open (file, O_RDONLY)) < 0)
        return FALSE;

    if ((RBFInfo->font = mmap (NULL, size, PROT_READ, MAP_SHARED, 
            fd, 0)) == MAP_FAILED) {
        close (fd);
        return FALSE;
    }

    close (fd);
#else
    // Allocate memory for font data.
    if (!(RBFInfo->font = (char *)malloc (size)) )
        return FALSE;

    // Open font file and read information to the system memory.
    if (!(fp = fopen (file, "rb"))) {
        return FALSE;
    }
    if (fread (RBFInfo->font, sizeof(char), size, fp) < size) {
        fclose(fp);
        return FALSE;
    }

    fclose(fp);
#endif  /* HAVE_MMAP */
    return TRUE;
}

void UnloadRawBitmapFont (RBFINFO* RBFInfo)
{
#ifdef HAVE_MMAP
    if (RBFInfo->font)
        munmap (RBFInfo->font, RBFInfo->font_size);
#else
    free (RBFInfo->font);
#endif
}

/********************** Init/Term of raw bitmap font ***********************/
static int nr_fonts;
static RBFINFO* rbf_infos;
static DEVFONT* rbf_dev_fonts;

#define SECTION_NAME "rawbitmapfonts"

BOOL InitRawBitmapFonts (void)
{
    int i;
    char font_name [LEN_DEVFONT_NAME + 1];

    if (GetMgEtcIntValue (SECTION_NAME, "font_number", 
                           &nr_fonts) < 0 )
        return FALSE;
    if ( nr_fonts < 1) return TRUE;

    rbf_infos = calloc (nr_fonts, sizeof (RBFINFO));
    rbf_dev_fonts = calloc (nr_fonts, sizeof (DEVFONT));

    if (rbf_infos == NULL || rbf_dev_fonts == NULL) {
        free (rbf_infos);
        free (rbf_dev_fonts);
        return FALSE;
    }

    for (i = 0; i < nr_fonts; i++)
        rbf_infos [i].font = NULL;

    for (i = 0; i < nr_fonts; i++) {
        char key [11];
        char charset [LEN_FONT_NAME + 1];
        char file [MAX_PATH + 1];

        sprintf (key, "name%d", i);
        if (GetMgEtcValue (SECTION_NAME, key, 
                           font_name, LEN_DEVFONT_NAME) < 0 )
            goto error_load;

        if (!fontGetCharsetFromName (font_name, charset)) {
            fprintf (stderr, "GDI: Invalid font name (charset): %s.\n", 
                    font_name);
            goto error_load;
        }

        if ((rbf_infos [i].charset_ops 
               = GetCharsetOpsEx (charset)) == NULL) {
            fprintf (stderr, "GDI: Not supported charset: %s.\n", charset);
            goto error_load;
        }

        if ((rbf_infos [i].width = fontGetWidthFromName (font_name)) == -1) {
            fprintf (stderr, "GDI: Invalid font name (width): %s.\n", 
                    font_name);
            goto error_load;
        }
        
        if ((rbf_infos [i].height = fontGetHeightFromName (font_name)) == -1) {
            fprintf (stderr, "GDI: Invalid font name (height): %s.\n",
                    font_name);
            goto error_load;
        }
        
        rbf_infos [i].nr_chars = rbf_infos [i].charset_ops->nr_chars;

        sprintf (key, "fontfile%d", i);
        if (GetMgEtcValue (SECTION_NAME, key, 
                           file, MAX_PATH) < 0)
            goto error_load;

        if (!LoadRawBitmapFont (file, rbf_infos + i))
            goto error_load;

        strncpy (rbf_dev_fonts[i].name, font_name, LEN_DEVFONT_NAME);
        rbf_dev_fonts[i].name [LEN_DEVFONT_NAME] = '\0';
        rbf_dev_fonts[i].font_ops = &raw_bitmap_font_ops;
        rbf_dev_fonts[i].charset_ops = rbf_infos [i].charset_ops;
        rbf_dev_fonts[i].data = rbf_infos + i;
#if 0
        fprintf (stderr, "GDI: RBFDevFont %i: %s.\n", i, rbf_dev_fonts[i].name);
#endif
    }

    for (i = 0; i < nr_fonts; i++) {
        if (rbf_infos [i].charset_ops->bytes_maxlen_char > 1)
            AddMBDevFont (rbf_dev_fonts + i);
        else
            AddSBDevFont (rbf_dev_fonts + i);
    }

    return TRUE;

error_load:
    fprintf (stderr, "GDI: Error in loading raw bitmap fonts!\n");
    for (i = 0; i < nr_fonts; i++)
        UnloadRawBitmapFont (rbf_infos + i);
    
    free (rbf_infos);
    free (rbf_dev_fonts);
    rbf_infos = NULL;
    rbf_dev_fonts = NULL;
    return FALSE;
}

void TermRawBitmapFonts (void)
{
    int i;

    for (i = 0; i < nr_fonts; i++)
        UnloadRawBitmapFont (rbf_infos + i);
    
    free (rbf_infos);
    free (rbf_dev_fonts);

    rbf_infos = NULL;
    rbf_dev_fonts = NULL;
}

#endif /* _INCORE_RES */

/*************** Raw bitmap font operations *********************************/
static int get_char_width (LOGFONT* logfont, DEVFONT* devfont, 
                const unsigned char* mchar, int len)
{
    return RBFONT_INFO_P (devfont)->width * GET_DEVFONT_SCALE (logfont, devfont);
}

static int get_ave_width (LOGFONT* logfont, DEVFONT* devfont)
{
    return RBFONT_INFO_P (devfont)->width * GET_DEVFONT_SCALE (logfont, devfont);
}

static int get_font_height (LOGFONT* logfont, DEVFONT* devfont)
{
    return RBFONT_INFO_P (devfont)->height * GET_DEVFONT_SCALE (logfont, devfont);
}

static int get_font_size (LOGFONT* logfont, DEVFONT* devfont, int expect)
{
#ifdef _USE_NEWGAL
    int height = RBFONT_INFO_P (devfont)->height;
    unsigned short scale = 1;

    if (logfont->style & FS_OTHER_AUTOSCALE)
        scale = GetBestScaleFactor (height, expect);

    SET_DEVFONT_SCALE (logfont, devfont, scale);

    return height * scale;
#else
    return RBFONT_INFO_P (devfont)->height;
#endif
}

static int get_font_ascent (LOGFONT* logfont, DEVFONT* devfont)
{
    int height = RBFONT_INFO_P (devfont)->height;

    height *= GET_DEVFONT_SCALE (logfont, devfont);

    if (height >= 40)
        return height - 6;
    else if (height >= 20)
        return height - 3;
    else if (height >= 15)
        return height - 2;
    else if (height >= 10)
        return height - 1;

    return height;
}

static int get_font_descent (LOGFONT* logfont, DEVFONT* devfont)
{
    int height = RBFONT_INFO_P (devfont)->height;
    
    height *= GET_DEVFONT_SCALE (logfont, devfont);

    if (height >= 40)
        return 6;
    else if (height >= 20)
        return 3;
    else if (height >= 15)
        return 2;
    else if (height >= 10)
        return 1;

    return 0;
}

static const void* get_char_bitmap (LOGFONT* logfont, DEVFONT* devfont,
            const unsigned char* mchar, int len, unsigned short* scale)
{
    int bitmap_size;
    int offset;

    offset = (*devfont->charset_ops->char_offset) (mchar);

    if (offset >= RBFONT_INFO_P (devfont)->font_size)
        offset = (*devfont->charset_ops->char_offset) 
                    ((const unsigned char*)devfont->charset_ops->def_char);

    bitmap_size = ((RBFONT_INFO_P (devfont)->width + 7) >> 3) 
                * RBFONT_INFO_P (devfont)->height; 

    if (scale)
        *scale = GET_DEVFONT_SCALE (logfont, devfont);

    return RBFONT_INFO_P (devfont)->font + bitmap_size * offset;
}

/**************************** Global data ************************************/
FONTOPS raw_bitmap_font_ops = {
    get_char_width,
    get_ave_width,
    get_ave_width,  // max_width same as ave_width
    get_font_height,
    get_font_size,
    get_font_ascent,
    get_font_descent,

    get_char_bitmap,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

#endif  /* _RBF_SUPPORT */

