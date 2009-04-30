/*
** $Id: varbitmap.c 7330 2007-08-16 03:17:58Z xgwang $
** 
** varbitmap.c: The Var Bitmap Font operation set.
**
** Copyright (C) 2003 ~ 2007 Feynman Software
** Copyright (C) 2000 ~ 2002 Wei Yongming
**
** All right reserved by Feynman Software.
**
** Create date: 2000/06/13
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "common.h"

#ifdef _VBF_SUPPORT

#include "minigui.h"
#include "gdi.h"
#include "endianrw.h"
#include "misc.h"

#ifdef HAVE_MMAP
    #include <fcntl.h>
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/mman.h>
    #include <sys/stat.h>
#endif

#include "devfont.h"
#include "varbitmap.h"
#include "charset.h"
#include "fontname.h"

/*************** Variable bitmap font operations *********************************/
static int get_char_width (LOGFONT* logfont, DEVFONT* devfont, 
                const unsigned char* mchar, int len)
{
    VBFINFO* vbf_info = VARFONT_INFO_P (devfont);
    unsigned short scale = GET_DEVFONT_SCALE (logfont, devfont);

    if (vbf_info->width == NULL)
        return vbf_info->max_width * scale;

    if (*mchar < vbf_info->first_char || *mchar > vbf_info->last_char)
        return vbf_info->width [vbf_info->def_char - vbf_info->first_char] * scale;

    return vbf_info->width [*mchar - vbf_info->first_char] * scale;
}

static int get_max_width (LOGFONT* logfont, DEVFONT* devfont)
{
   return VARFONT_INFO_P (devfont)->max_width * GET_DEVFONT_SCALE (logfont, devfont);
}

static int get_ave_width (LOGFONT* logfont, DEVFONT* devfont)
{
    return VARFONT_INFO_P(devfont)->ave_width * GET_DEVFONT_SCALE (logfont, devfont);
}

static int get_font_height (LOGFONT* logfont, DEVFONT* devfont)
{
    return VARFONT_INFO_P (devfont)->height * GET_DEVFONT_SCALE (logfont, devfont);
}

static int get_font_size (LOGFONT* logfont, DEVFONT* devfont, int expect)
{
#ifdef _USE_NEWGAL
    int height = VARFONT_INFO_P (devfont)->height;
    unsigned short scale = 1;

    if (logfont->style & FS_OTHER_AUTOSCALE)
        scale = GetBestScaleFactor (height, expect);

    SET_DEVFONT_SCALE (logfont, devfont, scale);

    return height * scale;
#else
    return VARFONT_INFO_P (devfont)->height;
#endif
}

static int get_font_ascent (LOGFONT* logfont, DEVFONT* devfont)
{
    int ascent = VARFONT_INFO_P (devfont)->height 
            - VARFONT_INFO_P (devfont)->descent;

    return ascent * GET_DEVFONT_SCALE (logfont, devfont);
}

static int get_font_descent (LOGFONT* logfont, DEVFONT* devfont)
{
    return VARFONT_INFO_P (devfont)->descent 
            * GET_DEVFONT_SCALE (logfont, devfont);
}

static const void* get_char_bitmap (LOGFONT* logfont, DEVFONT* devfont,
            const unsigned char* mchar, int len, unsigned short* scale)
{
    int offset;
    unsigned char eff_char = *mchar;
    VBFINFO* vbf_info = VARFONT_INFO_P (devfont);

    if (*mchar < vbf_info->first_char || *mchar > vbf_info->last_char)
        eff_char = vbf_info->def_char;

    if (vbf_info->offset == NULL)
        offset = (((size_t)vbf_info->max_width + 7) >> 3) * vbf_info->height 
                    * (eff_char - vbf_info->first_char);
    else {
        offset = vbf_info->offset [eff_char - vbf_info->first_char];
#if MGUI_BYTEORDER == MGUI_BIG_ENDIAN
        if (vbf_info->font_size)
            offset = ArchSwap16 (offset);
#endif
    }

    if (scale)
        *scale = GET_DEVFONT_SCALE (logfont, devfont);

    return vbf_info->bits + offset;
}

static int get_char_bbox (LOGFONT* logfont, DEVFONT* devfont,
            const unsigned char* mchar, int len,
            int* px, int* py, int* pwidth, int* pheight)
{
    int tempint;
    VBFINFO* vbf_info = VARFONT_INFO_P (devfont);
    unsigned short scale = GET_DEVFONT_SCALE (logfont, devfont);

    if (vbf_info->version == 2) {
        int offset;
        unsigned char eff_char = *mchar;

        if (*mchar < vbf_info->first_char || *mchar > vbf_info->last_char)
            eff_char = vbf_info->def_char;
        offset = eff_char - vbf_info->first_char;
    
        if (px) {
            tempint = vbf_info->bbox_x[offset];
#if MGUI_BYTEORDER == MGUI_BIG_ENDIAN
            tempint = ArchSwap32(tempint);
#endif
            *px += tempint * scale;
        }
        if (py) {
            tempint = vbf_info->bbox_y[offset];
#if MGUI_BYTEORDER == MGUI_BIG_ENDIAN
            tempint = ArchSwap32(tempint);
#endif
            *py -= tempint * scale;
        }
        if (pheight) {
            tempint = vbf_info->bbox_h[offset];
#if MGUI_BYTEORDER == MGUI_BIG_ENDIAN
            tempint = ArchSwap32(tempint);
#endif
            *pheight = tempint * scale;
        }

        tempint = vbf_info->bbox_w[offset];
#if MGUI_BYTEORDER == MGUI_BIG_ENDIAN
        tempint = ArchSwap32(tempint);
#endif
        tempint *= scale;

        if (pwidth) *pwidth = tempint;
    }
    else {
        tempint = get_char_width (logfont, devfont, mchar, len);

        if (pwidth)
            *pwidth     = tempint;
        if (pheight)
            *pheight    = vbf_info->height * scale;
        if (py)
            *py        -= get_font_ascent(logfont, devfont);
    }

    return tempint;
}

static void get_char_advance (LOGFONT* logfont, DEVFONT* devfont,
                const unsigned char* mchar, int len, 
                int* px, int* py)
{
    *px += get_char_width(logfont, devfont, mchar, len);
}


/**************************** Global data ************************************/
static FONTOPS var_bitmap_font_ops = {
    get_char_width,
    get_ave_width,
    get_max_width,
    get_font_height,
    get_font_size,
    get_font_ascent,
    get_font_descent,
    get_char_bitmap,
    NULL,
    NULL,
    get_char_bbox,
    get_char_advance,
    NULL,
    NULL
};

/******************* In-core var bitmap fonts ********************************/

#ifndef DYNAMIC_LOAD

#ifdef _INCOREFONT_SANSSERIF
extern VBFINFO __mgif_vbf_SansSerif11x13;
#endif

#ifdef _INCOREFONT_COURIER
extern VBFINFO __mgif_vbf_Courier8x13;
#endif

#ifdef _INCOREFONT_MODERN
#endif

#ifdef _INCOREFONT_SERIF
#endif

#ifdef _INCOREFONT_SMALL
#endif

#ifdef _INCOREFONT_SYMBOL
extern VBFINFO __mgif_vbf_symb12;
#endif

#ifdef _INCOREFONT_VGAS
extern VBFINFO __mgif_vbf_VGAOEM8x8;
extern VBFINFO __mgif_vbf_Terminal8x12;
extern VBFINFO __mgif_vbf_System14x16;
extern VBFINFO __mgif_vbf_Fixedsys8x15;
#endif

#ifdef _INCOREFONT_HELV
extern VBFINFO __mgif_vbf_helvR16;
extern VBFINFO __mgif_vbf_helvR21;
extern VBFINFO __mgif_vbf_helvR27;
#endif
static VBFINFO* incore_vbfonts [] = {
#ifdef _INCOREFONT_SANSSERIF
    &__mgif_vbf_SansSerif11x13,
#endif
#ifdef _INCOREFONT_COURIER
    &__mgif_vbf_Courier8x13,
#endif
#ifdef _INCOREFONT_SYMBOL
    &__mgif_vbf_symb12,
#endif
#ifdef _INCOREFONT_VGAS
    &__mgif_vbf_VGAOEM8x8,
    &__mgif_vbf_Terminal8x12,
    &__mgif_vbf_System14x16,
    &__mgif_vbf_Fixedsys8x15,
#endif
#ifdef _INCOREFONT_HELV
    &__mgif_vbf_helvR16,
    &__mgif_vbf_helvR21,
    &__mgif_vbf_helvR27,
#endif
    NULL,
};

#define NR_VBFONTS  (sizeof (incore_vbfonts) / sizeof (VBFINFO*) - 1)

#else /* !DYNAMIC_LOAD */

#include "varfont.h"

#define NR_VBFONTS  get_vbf_size()
static VBFINFO** incore_vbfonts;

#endif /* DYNAMIC_LOAD */

static DEVFONT* incore_vbf_dev_font;

static CHARSETOPS* vbfGetCharsetOps (VBFINFO* vbfont)
{
    int count = 0;
    const char* font_name = vbfont->name;

    while (*font_name) {
        if (*font_name == '-') count ++;
        if (count == 5) break;

        font_name ++;
    }

    if (*font_name != '-') return NULL;
    font_name ++;

    return GetCharsetOpsEx (font_name);
}

BOOL InitIncoreVBFonts (void)
{
    int i;

#ifdef DYNAMIC_LOAD
    incore_vbfonts = (VBFINFO**)set_vbf();
#endif

    if (NR_VBFONTS == 0)
        return TRUE;

    if ((incore_vbf_dev_font = malloc (NR_VBFONTS * sizeof (DEVFONT))) == NULL)
        return FALSE;

    for (i = 0; i < NR_VBFONTS; i++) {
        if ((incore_vbf_dev_font [i].charset_ops 
                = vbfGetCharsetOps (incore_vbfonts [i])) == NULL) {
            fprintf (stderr, 
                "GDI: Not supported charset for var-bitmap font %s.\n",
                incore_vbfonts[i]->name);
            free (incore_vbf_dev_font);
            return FALSE;
        }

        strncpy (incore_vbf_dev_font [i].name, incore_vbfonts [i]->name, LEN_DEVFONT_NAME);
        incore_vbf_dev_font [i].name [LEN_DEVFONT_NAME] = '\0';
        incore_vbf_dev_font [i].font_ops = &var_bitmap_font_ops;
        incore_vbf_dev_font [i].data     = incore_vbfonts [i];
    }

    for (i = 0; i < NR_VBFONTS; i++)
        AddSBDevFont (incore_vbf_dev_font + i);

    return TRUE;
}

void TermIncoreVBFonts (void)
{
    free (incore_vbf_dev_font);
    incore_vbf_dev_font = NULL;
}

#ifndef _INCORE_RES

/********************** Load/Unload of var bitmap font ***********************/
static BOOL LoadVarBitmapFont (const char* file, VBFINFO* info)
{
#ifdef HAVE_MMAP
    int fd;
#else
    FILE* fp;
#endif
    char* temp = NULL;
    int len_header, len_offsets, len_widths, len_bits, len_bboxs = 0;
    int font_size;
    char version[LEN_VERSION_INFO + 1];

#ifdef HAVE_MMAP
    if ((fd = open (file, O_RDONLY)) < 0)
        return FALSE;

    if (read (fd, version, LEN_VERSION_INFO) == -1)
        goto error;
    version[LEN_VERSION_INFO] = '\0';

    if (!strcmp (version, VBF_VERSION2))
        info->version = 2;
    else
    {
        info->version = 1;
        if (strcmp (version, VBF_VERSION))
            fprintf (stderr, "Error on loading vbf: %s, version: %s, invalid version.\n", file, version);
    }

    if (read (fd, &len_header, sizeof (int)) == -1)
        goto error;
#if MGUI_BYTEORDER == MGUI_BIG_ENDIAN
    len_header = ArchSwap32 (len_header);
#endif

    if (read (fd, &info->max_width, sizeof (char) * 2) == -1) goto error;
    if (read (fd, &info->height, sizeof (int) * 2) == -1) goto error;

#if MGUI_BYTEORDER == MGUI_BIG_ENDIAN
    info->height = ArchSwap32 (info->height);
    info->descent = ArchSwap32 (info->descent);
#endif

    if (read (fd, &info->first_char, sizeof (char) * 3) == -1) goto error;

    if (lseek (fd, len_header - (((info->version == 2)?5:4) * sizeof (int)), SEEK_SET) == -1)
        goto error;

    if (read (fd, &len_offsets, sizeof (int)) == -1
            || read (fd, &len_widths, sizeof (int)) == -1
            || (info->version == 2 && read (fd, &len_bboxs, sizeof (int)) == -1)
            || read (fd, &len_bits, sizeof (int)) == -1
            || read (fd, &font_size, sizeof (int)) == -1)
        goto error;
#if MGUI_BYTEORDER == MGUI_BIG_ENDIAN
    len_offsets = ArchSwap32 (len_offsets);
    len_widths = ArchSwap32 (len_widths);
    if (info->version == 2)
        len_bboxs = ArchSwap32 (len_bboxs);
    len_bits = ArchSwap32 (len_bits);
    font_size = ArchSwap32 (font_size);
#endif

    if ((temp = mmap (NULL, font_size, PROT_READ, MAP_SHARED, 
            fd, 0)) == MAP_FAILED)
        goto error;

    temp += len_header;
    close (fd);
#else
    // Open font file and read information of font.
    if (!(fp = fopen (file, "rb")))
        return FALSE;

    if (fread (version, sizeof (char), LEN_VERSION_INFO, fp) < LEN_VERSION_INFO)
        goto error;
    version [LEN_VERSION_INFO] = '\0'; 

    if (!strcmp (version, VBF_VERSION2))
        info->version = 2;
    else
    {
        info->version = 1;
        if (strcmp (version, VBF_VERSION))
            fprintf (stderr, "Error on loading vbf: %s, version: %s, invalid version.\n", file, version);
    }

    if (fread (&len_header, sizeof (int), 1, fp) < 1)
        goto error;
#if MGUI_BYTEORDER == MGUI_BIG_ENDIAN
    len_header = ArchSwap32 (len_header);
#endif

    if (fread (&info->max_width, sizeof (char), 2, fp) < 2) goto error;
    if (fread (&info->height, sizeof (int), 2, fp) < 2) goto error;
#if MGUI_BYTEORDER == MGUI_BIG_ENDIAN
    info->height = ArchSwap32 (info->height);
    info->descent = ArchSwap32 (info->descent);
#endif
    if (fread (&info->first_char, sizeof (char), 3, fp) < 3) goto error;

    if (fseek (fp, len_header - (((info->version == 2)?5:4)*sizeof (int)), SEEK_SET) != 0)
            goto error;

    if (fread (&len_offsets, sizeof (int), 1, fp) < 1
            || fread (&len_widths, sizeof (int), 1, fp) < 1
            || (info->version == 2 && fread (&len_bboxs, sizeof (int), 1, fp) < 1)
            || fread (&len_bits, sizeof (int), 1, fp) < 1
            || fread (&font_size, sizeof (int), 1, fp) < 1)
        goto error;
#if MGUI_BYTEORDER == MGUI_BIG_ENDIAN
    len_offsets = ArchSwap32 (len_offsets);
    len_widths = ArchSwap32 (len_widths);
    if (info->version == 2)
        len_bboxs = ArchSwap32 (len_bboxs);
    len_bits = ArchSwap32 (len_bits);
    font_size = ArchSwap32 (font_size);
#endif

    // Allocate memory for font data.
    font_size -= len_header;
    if ((temp = (char *)malloc (font_size)) == NULL)
        goto error;

    if (fseek (fp, len_header, SEEK_SET) != 0)
        goto error;

    if (fread (temp, sizeof (char), font_size, fp) < font_size)
        goto error;

    fclose (fp);
#endif
    info->name = temp;
    info->offset = (unsigned short*) (temp + LEN_DEVFONT_NAME + 1);
    info->width = (unsigned char*) (temp + LEN_DEVFONT_NAME + 1 + len_offsets);
    if (info->version == 2)
    {
        info->bbox_x = (int*) (temp + LEN_DEVFONT_NAME + 1 + len_offsets + len_widths);
        info->bbox_y = (int*) (temp + LEN_DEVFONT_NAME + 1 + len_offsets + len_widths + len_bboxs);
        info->bbox_w = (int*) (temp + LEN_DEVFONT_NAME + 1 + len_offsets + len_widths + 2*len_bboxs);
        info->bbox_h = (int*) (temp + LEN_DEVFONT_NAME + 1 + len_offsets + len_widths + 3*len_bboxs);
        info->bits = (unsigned char*) (temp + LEN_DEVFONT_NAME + 1 + len_offsets + len_widths + 4*len_bboxs);
    }
    else
        info->bits = (unsigned char*) (temp + LEN_DEVFONT_NAME + 1 + len_offsets + len_widths);
    info->font_size = font_size;

#if 0
    fprintf (stderr, "VBF: %s-%dx%d-%d (%d~%d:%d).\n", 
            info->name, info->max_width, info->height, info->descent,
            info->first_char, info->last_char, info->def_char);
#endif

    return TRUE;

error:
#ifdef HAVE_MMAP
    if (temp)
        munmap (temp, font_size);
    close (fd);
#else
    free (temp);
    fclose (fp);
#endif
    
    return FALSE;
}

static void UnloadVarBitmapFont (VBFINFO* info)
{
#ifdef HAVE_MMAP
    if (info->name)
        munmap ((void*)(info->name), info->font_size);
#else
    free ((void*)info->name);
#endif
}

/******************** Init/Term of var bitmap font in file *******************/
static int nr_fonts;
static VBFINFO* file_vbf_infos;
static DEVFONT* file_vbf_dev_fonts;

#define SECTION_NAME    "varbitmapfonts"

BOOL InitVarBitmapFonts (void)
{
    int i;
    char font_name [LEN_DEVFONT_NAME + 1];

    if (GetMgEtcIntValue (SECTION_NAME, "font_number", 
                           &nr_fonts) < 0 )
        return FALSE;
    if ( nr_fonts < 1) return TRUE;

    file_vbf_infos = calloc (nr_fonts, sizeof (VBFINFO));
    file_vbf_dev_fonts = calloc (nr_fonts, sizeof (DEVFONT));
    if (file_vbf_infos == NULL || file_vbf_dev_fonts == NULL) {
        free (file_vbf_infos);
        free (file_vbf_dev_fonts);
        return FALSE;
    }

    for (i = 0; i < nr_fonts; i++)
        file_vbf_infos [i].name = NULL;

    for (i = 0; i < nr_fonts; i++) {
        char key [11];
        char charset [LEN_FONT_NAME + 1];
        char file [MAX_PATH + 1];
        CHARSETOPS* charset_ops;

        sprintf (key, "name%d", i);
        if (GetMgEtcValue (SECTION_NAME, key, 
                           font_name, LEN_DEVFONT_NAME) < 0 )
            goto error_load;
        if (!fontGetCharsetFromName (font_name, charset)) {
            fprintf (stderr, "GDI: Invalid font name (charset): %s.\n", 
                    font_name);
            goto error_load;
        }

        if ((charset_ops = GetCharsetOpsEx (charset)) == NULL) {
            fprintf (stderr, "GDI: Not supported charset: %s.\n", charset);
            goto error_load;
        }

        sprintf (key, "fontfile%d", i);
        if (GetMgEtcValue (SECTION_NAME, key, 
                           file, MAX_PATH) < 0)
            goto error_load;

        if (!LoadVarBitmapFont (file, file_vbf_infos + i))
            goto error_load;

        strncpy (file_vbf_dev_fonts[i].name, font_name, LEN_DEVFONT_NAME);
        file_vbf_dev_fonts[i].name [LEN_DEVFONT_NAME] = '\0';
        file_vbf_dev_fonts[i].font_ops = &var_bitmap_font_ops;
        file_vbf_dev_fonts[i].charset_ops = charset_ops;
        file_vbf_dev_fonts[i].data = file_vbf_infos + i;
#if 0
        fprintf (stderr, "GDI: VBFDevFont %i: %s.\n", i, file_vbf_dev_fonts[i].name);
#endif
    }

    for (i = 0; i < nr_fonts; i++) {
        if (file_vbf_dev_fonts [i].charset_ops->bytes_maxlen_char > 1)
            AddMBDevFont (file_vbf_dev_fonts + i);
        else
            AddSBDevFont (file_vbf_dev_fonts + i);
    }

    return TRUE;

error_load:
    fprintf (stderr, "GDI: Error in loading vbf fonts!\n");
    for (i = 0; i < nr_fonts; i++)
        UnloadVarBitmapFont (file_vbf_infos + i);
    
    free (file_vbf_infos);
    free (file_vbf_dev_fonts);
    file_vbf_infos = NULL;
    file_vbf_dev_fonts = NULL;
    return FALSE;
}

void TermVarBitmapFonts (void)
{
    int i;

    for (i = 0; i < nr_fonts; i++)
        UnloadVarBitmapFont (file_vbf_infos + i);
    
    free (file_vbf_infos);
    free (file_vbf_dev_fonts);

    file_vbf_infos = NULL;
    file_vbf_dev_fonts = NULL;
}

#endif /* _INCORE_RES */

#endif /* _VBF_SUPPORT */

