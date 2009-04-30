/*
** $Id: qpf.c 9056 2008-01-23 08:58:25Z xwyan $
** 
** qpf.c: The Qt Prerendered Font operation set.
**
** Copyright (C) 2003 ~ 2007 Feynman Software.
**
** All right reserved by Feynman Software.
**
** Create date: 2003/01/28
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "minigui.h"
#include "gdi.h"
#include "misc.h"

#ifdef _QPF_SUPPORT

#ifndef __NOUNIX__
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef HAVE_MMAP
    #include <sys/mman.h>
#endif

#endif /* _INCORE_RES */

#include "charset.h"
#include "devfont.h"
#include "qpf.h"
#include "fontname.h"

/********************** Init/Term of QPF font ***********************/

#ifdef _INCORE_RES
#ifndef DYNAMIC_LOAD
#ifdef __TARGET_MGDESKTOP__
extern QPFINFO __mgif_qpf_fmsong_12;
extern QPFINFO __mgif_qpf_fmsong_16;
extern QPFINFO __mgif_qpf_fmkai_18;
extern QPFINFO __mgif_qpf_fmhei_20;
#else
extern QPFINFO __mgif_qpf_micro_4;
extern QPFINFO __mgif_qpf_times_10;
extern QPFINFO __mgif_qpf_helvetica_10;
#endif

static QPFINFO* qpf_infos [] = {
#ifdef __TARGET_MGDESKTOP__
    &__mgif_qpf_fmsong_12,
    &__mgif_qpf_fmsong_16,
    &__mgif_qpf_fmkai_18,
    &__mgif_qpf_fmhei_20,
#else
    &__mgif_qpf_micro_4,
    &__mgif_qpf_times_10,
    &__mgif_qpf_helvetica_10,
#endif
    NULL,
};

#define NR_FONTS  (sizeof (qpf_infos) / sizeof (QPFINFO*) - 1)

#else /* !DYNAMIC_LOAD */

#include "qpffont.h"

#define NR_FONTS  get_qpf_size()
static QPFINFO** qpf_infos;

#endif /* DYNAMIC_LOAD */

static DEVFONT* qpf_dev_fonts;

static BOOL init_incore_glyph_tree (GLYPHTREE* tree)
{
    int i, n;
    unsigned int dataoffset;

    if (tree->less) {
        if (!init_incore_glyph_tree (tree->less))
            return FALSE;
    }
    if (tree->more) {
        if (!init_incore_glyph_tree (tree->more))
            return FALSE;
    }

    n = tree->max - tree->min + 1;
    tree->glyph = calloc (n, sizeof (GLYPH));
    if (tree->glyph == NULL)
        return FALSE;

    dataoffset = 0;
    for (i = 0; i < n; i++) {
        tree->glyph [i].metrics = tree->metrics + i;
        tree->glyph [i].data = tree->data + dataoffset;
        dataoffset += tree->glyph[i].metrics->linestep * 
                tree->glyph[i].metrics->height;
    }

    return TRUE;
}

static void clear_incore_glyph_tree (GLYPHTREE* tree)
{
    if (tree->less) {
        clear_incore_glyph_tree (tree->less);
    }
    if (tree->more) {
        clear_incore_glyph_tree (tree->more);
    }

    free (tree->glyph);
}

BOOL InitIncoreQPFonts (void)
{
    int i;

#ifdef DYNAMIC_LOAD
    qpf_infos = set_qpf ();
#endif

    if (NR_FONTS < 1) return TRUE;

    qpf_dev_fonts = calloc (NR_FONTS, sizeof (DEVFONT));

    if (qpf_dev_fonts == NULL) {
        free (qpf_dev_fonts);
        return FALSE;
    }

    for (i = 0; i < NR_FONTS; i++) {
        char charset [LEN_FONT_NAME + 1];
        CHARSETOPS* charset_ops;

        if (!fontGetCharsetFromName (qpf_infos [i]->font_name, charset)) {
            fprintf (stderr, "GDI: Invalid font name (charset): %s.\n", 
                    qpf_infos [i]->font_name);
            goto error_load;
        }

        if ((charset_ops 
               = GetCharsetOpsEx (charset)) == NULL) {
            fprintf (stderr, "GDI: Not supported charset: %s.\n", charset);
            goto error_load;
        }

        if ((qpf_infos [i]->height = 
                fontGetHeightFromName (qpf_infos [i]->font_name)) == -1) {
            fprintf (stderr, "GDI: Invalid font name (height): %s.\n",
                    qpf_infos [i]->font_name);
            goto error_load;
        }
        
        if ((qpf_infos [i]->width = 
                fontGetWidthFromName (qpf_infos [i]->font_name)) == -1) {
            fprintf (stderr, "GDI: Invalid font name (width): %s.\n",
                    qpf_infos [i]->font_name);
            goto error_load;
        }
        
        if (!init_incore_glyph_tree (qpf_infos [i]->tree)) {
            clear_incore_glyph_tree (qpf_infos [i]->tree);
            goto error_load;
        }

        strncpy (qpf_dev_fonts[i].name, qpf_infos [i]->font_name, 
                        LEN_UNIDEVFONT_NAME);
        qpf_dev_fonts[i].name [LEN_UNIDEVFONT_NAME] = '\0';
        qpf_dev_fonts[i].font_ops = &qpf_font_ops;
        qpf_dev_fonts[i].charset_ops = charset_ops;
        qpf_dev_fonts[i].data = qpf_infos [i];

    }

    for (i = 0; i < NR_FONTS; i++) {
        int nr_charsets;
        char charsets [LEN_UNIDEVFONT_NAME + 1];

        if (qpf_dev_fonts [i].charset_ops->bytes_maxlen_char > 1)
            AddMBDevFont (qpf_dev_fonts + i);
        else
            AddSBDevFont (qpf_dev_fonts + i);

        fontGetCharsetPartFromName (qpf_dev_fonts[i].name, charsets);
        if ((nr_charsets = charsetGetCharsetsNumber (charsets)) > 1) {

            int j;

            for (j = 1; j < nr_charsets; j++) {
                char charset [LEN_FONT_NAME + 1];
                CHARSETOPS* charset_ops;
                DEVFONT* new_devfont;

                charsetGetSpecificCharset (charsets, j, charset);
                if ((charset_ops = GetCharsetOpsEx (charset)) == NULL)
                    continue;

                new_devfont = calloc (1, sizeof (DEVFONT));
                memcpy (new_devfont, qpf_dev_fonts + i, sizeof (DEVFONT));
                new_devfont->charset_ops = charset_ops;
                new_devfont->relationship = qpf_dev_fonts + i;
                if (new_devfont->charset_ops->bytes_maxlen_char > 1)
                    AddMBDevFont (new_devfont);
                else
                    AddSBDevFont (new_devfont);
            }
        }
    }

    return TRUE;

error_load:
    fprintf (stderr, "GDI: Error in initializing incore QPF fonts!\n");
    
    free (qpf_dev_fonts);
    qpf_dev_fonts = NULL;
    return FALSE;
}

void TermIncoreQPFonts (void)
{
    int i;

    for (i = 0; i < NR_FONTS; i++) {
        clear_incore_glyph_tree (qpf_infos [i]->tree);
        DelRelatedDevFont (qpf_dev_fonts + i);
    }

    free (qpf_dev_fonts);
    qpf_dev_fonts = NULL;
}
#endif

#ifndef __NOUNIX__
/*load qpf from file, only support for linux*/
typedef unsigned char uchar;

static void readNode (GLYPHTREE* tree, uchar** data)
{
    uchar rw, cl;
    int flags, n;

    rw = **data; (*data)++;
    cl = **data; (*data)++;
    tree->min = (rw << 8) | cl;

    rw = **data; (*data)++;
    cl = **data; (*data)++;
    tree->max = (rw << 8) | cl;

    flags = **data; (*data)++;
    if (flags & 1)
        tree->less = calloc (1, sizeof (GLYPHTREE));
    else
        tree->less = NULL;

    if (flags & 2)
        tree->more = calloc (1, sizeof (GLYPHTREE));
    else
        tree->more = NULL;

    n = tree->max - tree->min + 1;
    tree->glyph = calloc (n, sizeof (GLYPH));

    if (tree->less)
        readNode (tree->less, data);
    if (tree->more)
        readNode (tree->more, data);
}

static void readMetrics (GLYPHTREE* tree, uchar** data)
{
    int i;
    int n = tree->max - tree->min + 1;

    for (i = 0; i < n; i++) {
        tree->glyph[i].metrics = (GLYPHMETRICS*) *data;

        *data += sizeof (GLYPHMETRICS);
    }

    if (tree->less)
        readMetrics (tree->less, data);
    if (tree->more)
        readMetrics (tree->more, data);
}

static void readData (GLYPHTREE* tree, uchar** data)
{
    int i;
    int n = tree->max - tree->min + 1;

    for (i = 0; i < n; i++) {
        int datasize;

        datasize = tree->glyph[i].metrics->linestep * 
                tree->glyph[i].metrics->height;
        tree->glyph[i].data = *data; *data += datasize;
    }

    if (tree->less)
        readData (tree->less, data);
    if (tree->more)
        readData (tree->more, data);
}


static void BuildGlyphTree (GLYPHTREE* tree, uchar** data)
{
    readNode (tree, data);
    readMetrics (tree, data);
    readData (tree, data);
}

BOOL LoadQPFont (const char* file, QPFINFO* QPFInfo)
{
    int fd;
    struct stat st;
    uchar* data;

    if ((fd = open (file, O_RDONLY)) < 0) return FALSE;
    if (fstat (fd, &st)) return FALSE;
    QPFInfo->file_size = st.st_size;

#ifdef HAVE_MMAP

    QPFInfo->fm = (QPFMETRICS*) mmap( 0, st.st_size,
                    PROT_READ, MAP_PRIVATE, fd, 0 );

    if (!QPFInfo->fm || QPFInfo->fm == (QPFMETRICS *)MAP_FAILED)
        goto error;

#else

    QPFInfo->fm = calloc (1, st.st_size);
    if (QPFInfo->fm == NULL)
        goto error;

    read (fd, QPFInfo->fm, st.st_size);

#endif

    QPFInfo->tree = calloc (1, sizeof (GLYPHTREE));

    data = (uchar*)QPFInfo->fm;
    data += sizeof (QPFMETRICS);
    BuildGlyphTree (QPFInfo->tree, &data);

    close (fd);
    return TRUE;

error:
    close (fd);
    return FALSE;
}

static void ClearGlyphTree (GLYPHTREE* tree)
{
    if (tree->less) {
        ClearGlyphTree (tree->less);
    }
    if (tree->more) {
        ClearGlyphTree (tree->more);
    }

    free (tree->glyph);
    free (tree->less);
    free (tree->more);
}

void UnloadQPFont (QPFINFO* QPFInfo)
{
    if (QPFInfo->file_size == 0)
        return;

    ClearGlyphTree (QPFInfo->tree);
    free (QPFInfo->tree);

#ifdef HAVE_MMAP
    munmap (QPFInfo->fm, QPFInfo->file_size);
#else
    free (QPFInfo->fm);
#endif
}

const DEVFONT* 
__mg_qpfload_devfont_fromfile(const char *devfont_name, const char *file_name)
{
    QPFINFO*    qpf_info      = NULL;
    DEVFONT*    qpf_dev_font  = NULL;
    CHARSETOPS* charset_ops;
    char        charset [LEN_FONT_NAME + 1];
    char        type_name [LEN_FONT_NAME + 1];
    int         nr_charsets;
    char        charsets [LEN_UNIDEVFONT_NAME + 1];


	if ((devfont_name == NULL) || (file_name == NULL)) return NULL;

	if (!fontGetTypeNameFromName (devfont_name, type_name)) {
		fprintf(stderr, "Font type error\n");
		return NULL;
	}

	if (strcasecmp (type_name, FONT_TYPE_NAME_BITMAP_QPF)) {
		fprintf(stderr, "it's not qpf font type error\n");
		return NULL;
	}

    qpf_info      = calloc(1, sizeof(QPFINFO));
    qpf_dev_font  = calloc(1, sizeof(DEVFONT));

    if ((qpf_info == NULL) || (qpf_dev_font == NULL)) {
        free(qpf_info);
        free(qpf_dev_font);
        return NULL;
    }
    strncpy (qpf_info->font_name, 
             devfont_name, 
             MIN (LEN_UNIDEVFONT_NAME, strlen (devfont_name)));

    if (!fontGetCharsetFromName (qpf_info->font_name, charset)) {
        fprintf (stderr, "qpfLoadFromFile: Invalid font name (charset): %s.\n",
                devfont_name);
        goto error_load;
    }

    if ((charset_ops = GetCharsetOpsEx (charset)) == NULL) {
        fprintf (stderr, 
                "qpfLoadFromFile: Not supported charset: %s.\n", charset);
        goto error_load;
    }

    if ((qpf_info->height 
            = fontGetHeightFromName (qpf_info->font_name)) == -1) {
        fprintf (stderr, "qpfLoadFromFile: Invalid font name (height): %s.\n",
                qpf_info->font_name);
        goto error_load;
    }

    if ((qpf_info->width 
            = fontGetWidthFromName (qpf_info->font_name)) == -1) {
        fprintf (stderr, "qpfLoadFromFile: Invalid font name (width): %s.\n",
                qpf_info->font_name);
        goto error_load;
    }

    if (!LoadQPFont (file_name, qpf_info))
        goto error_load;

    strncpy (qpf_dev_font->name, qpf_info->font_name, 
            LEN_UNIDEVFONT_NAME);
    qpf_dev_font->name [LEN_UNIDEVFONT_NAME] = '\0';
    qpf_dev_font->font_ops = &qpf_font_ops;
    qpf_dev_font->charset_ops = charset_ops;
    qpf_dev_font->data = qpf_info;

    if (qpf_dev_font->charset_ops->bytes_maxlen_char > 1)
        AddMBDevFont (qpf_dev_font);
    else
        AddSBDevFont (qpf_dev_font);

    fontGetCharsetPartFromName (qpf_dev_font->name, charsets);

    if ((nr_charsets = charsetGetCharsetsNumber (charsets)) > 1) {
        int j;

        for (j = 1; j < nr_charsets; j++) {
            char        charset [LEN_FONT_NAME + 1];
            CHARSETOPS* ops;
            DEVFONT*    new_devfont;

            charsetGetSpecificCharset (charsets, j, charset);
            if ((ops = GetCharsetOpsEx (charset)) == NULL)
                continue;

            new_devfont = calloc (1, sizeof (DEVFONT));
            memcpy (new_devfont, qpf_dev_font, sizeof (DEVFONT));
            new_devfont->charset_ops = ops;
            new_devfont->relationship = qpf_dev_font;
            if (new_devfont->charset_ops->bytes_maxlen_char > 1)
                AddMBDevFont (new_devfont);
            else
                AddSBDevFont (new_devfont);
        }
    }
    return qpf_dev_font;

error_load:
    fprintf (stderr, "qpfLoadFromFile: Error in loading QPF fonts!\n");

    UnloadQPFont (qpf_info);
    free (qpf_info);
    free (qpf_dev_font);
    qpf_info = NULL;
    qpf_dev_font = NULL;

    return NULL;
}

void __mg_qpf_destroy_devfont_fromfile (DEVFONT **devfont)
{
    UnloadQPFont ((QPFINFO*)((*devfont)->data));
    free ((QPFINFO*)((*devfont)->data));

    DelDevFont (*devfont, TRUE);
}
#endif

#ifndef _INCORE_RES
static int nr_fonts;
static QPFINFO* qpf_infos;
static DEVFONT* qpf_dev_fonts;

#define SECTION_NAME "qpf"

BOOL InitQPFonts (void)
{
    int i;

    if (GetMgEtcIntValue (SECTION_NAME, "font_number", 
                           &nr_fonts) < 0 )
        return FALSE;
    if ( nr_fonts < 1) return TRUE;

    qpf_infos = calloc (nr_fonts, sizeof (QPFINFO));
    qpf_dev_fonts = calloc (nr_fonts, sizeof (DEVFONT));

    if (qpf_infos == NULL || qpf_dev_fonts == NULL) {
        free (qpf_infos);
        free (qpf_dev_fonts);
        return FALSE;
    }

    for (i = 0; i < nr_fonts; i++) {
        char key [11];
        char charset [LEN_FONT_NAME + 1];
        char file [MAX_PATH + 1];
        CHARSETOPS* charset_ops;

        sprintf (key, "name%d", i);
        if (GetMgEtcValue (SECTION_NAME, key, 
                           qpf_infos[i].font_name, LEN_UNIDEVFONT_NAME) < 0 )
            goto error_load;

        if (!fontGetCharsetFromName (qpf_infos[i].font_name, charset)) {
            fprintf (stderr, "GDI: Invalid font name (charset): %s.\n", 
                    qpf_infos[i].font_name);
            goto error_load;
        }

        if ((charset_ops 
               = GetCharsetOpsEx (charset)) == NULL) {
            fprintf (stderr, "GDI: Not supported charset: %s.\n", charset);
            goto error_load;
        }

        if ((qpf_infos [i].height 
                = fontGetHeightFromName (qpf_infos[i].font_name)) == -1) {
            fprintf (stderr, "GDI: Invalid font name (height): %s.\n",
                    qpf_infos[i].font_name);
            goto error_load;
        }
        
        if ((qpf_infos [i].width 
                = fontGetWidthFromName (qpf_infos[i].font_name)) == -1) {
            fprintf (stderr, "GDI: Invalid font name (width): %s.\n",
                    qpf_infos[i].font_name);
            goto error_load;
        }
        
        sprintf (key, "fontfile%d", i);
        if (GetMgEtcValue (SECTION_NAME, key, 
                           file, MAX_PATH) < 0)
            goto error_load;

        if (!LoadQPFont (file, qpf_infos + i))
            goto error_load;

        strncpy (qpf_dev_fonts[i].name, qpf_infos[i].font_name, 
                        LEN_UNIDEVFONT_NAME);
        qpf_dev_fonts[i].name [LEN_UNIDEVFONT_NAME] = '\0';
        qpf_dev_fonts[i].font_ops = &qpf_font_ops;
        qpf_dev_fonts[i].charset_ops = charset_ops;
        qpf_dev_fonts[i].data = qpf_infos + i;

    }

    for (i = 0; i < nr_fonts; i++) {
        int nr_charsets;
        char charsets [LEN_UNIDEVFONT_NAME + 1];

        if (qpf_dev_fonts [i].charset_ops->bytes_maxlen_char > 1)
            AddMBDevFont (qpf_dev_fonts + i);
        else
            AddSBDevFont (qpf_dev_fonts + i);

        fontGetCharsetPartFromName (qpf_dev_fonts[i].name, charsets);
        if ((nr_charsets = charsetGetCharsetsNumber (charsets)) > 1) {

            int j;

            for (j = 1; j < nr_charsets; j++) {
                char charset [LEN_FONT_NAME + 1];
                CHARSETOPS* charset_ops;
                DEVFONT* new_devfont;

                charsetGetSpecificCharset (charsets, j, charset);
                if ((charset_ops = GetCharsetOpsEx (charset)) == NULL)
                    continue;

                new_devfont = calloc (1, sizeof (DEVFONT));
                memcpy (new_devfont, qpf_dev_fonts + i, sizeof (DEVFONT));
                new_devfont->charset_ops = charset_ops;
                new_devfont->relationship = qpf_dev_fonts + i;
                if (new_devfont->charset_ops->bytes_maxlen_char > 1)
                    AddMBDevFont (new_devfont);
                else
                    AddSBDevFont (new_devfont);
            }
        }
    }

    return TRUE;

error_load:
    fprintf (stderr, "GDI: Error in loading QPF fonts!\n");
    for (i = 0; i < nr_fonts; i++)
        UnloadQPFont (qpf_infos + i);
    
    free (qpf_infos);
    free (qpf_dev_fonts);
    qpf_infos = NULL;
    qpf_dev_fonts = NULL;
    return FALSE;
}

void TermQPFonts (void)
{
    int i;

    for (i = 0; i < nr_fonts; i++)
        UnloadQPFont (qpf_infos + i);
    
    free (qpf_infos);

    for (i = 0; i < nr_fonts; i++) {
        DelRelatedDevFont (qpf_dev_fonts + i);
    }

    free (qpf_dev_fonts);

    qpf_infos = NULL;
    qpf_dev_fonts = NULL;
}
#endif /* !_INCORE_RES */

/*************** QPF font operations *********************************/
static GLYPH* get_glyph (GLYPHTREE* tree, unsigned int ch)
{
    if (ch < tree->min) {

        if (!tree->less)
            return NULL;

        return get_glyph (tree->less, ch);
    } else if ( ch > tree->max ) {

        if (!tree->more) {
            return NULL;
        }
        return get_glyph (tree->more, ch);
    }

    return &tree->glyph [ch - tree->min];
}

static GLYPHMETRICS def_metrics = {1, 8, 2, 0, 0, 8, 0};
static unsigned char def_bitmap [] = {0xFE, 0x7F};

static GLYPHMETRICS def_smooth_metrics = {8, 8, 2, 0, 0, 8, 0};
static unsigned char def_smooth_bitmap [] = 
{
        0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00
};

static GLYPH def_glyph = {&def_metrics, def_bitmap};
static GLYPH def_smooth_glyph = {&def_smooth_metrics, def_smooth_bitmap};

static int get_char_width (LOGFONT* logfont, DEVFONT* devfont, 
                const unsigned char* mchar, int len)
{
    unsigned short scale = GET_DEVFONT_SCALE (logfont, devfont);
    unsigned int uc16;
    GLYPH* glyph;

    uc16 = (*devfont->charset_ops->conv_to_uc32) (mchar);
    glyph = get_glyph (QPFONT_INFO_P (devfont)->tree, uc16);

    if (glyph == NULL) {
        if (uc16 < 0x80 && logfont->sbc_devfont != devfont) {   /* ASCII */
            unsigned char ascii_ch = uc16;
            return logfont->sbc_devfont->font_ops->get_char_width (logfont,
                            logfont->sbc_devfont, &ascii_ch, 1);
        }
        else {
            glyph = &def_glyph;
        }
    }

    return glyph->metrics->advance * scale;
}

static int get_ave_width (LOGFONT* logfont, DEVFONT* devfont)
{
    return QPFONT_INFO_P (devfont)->width 
            * GET_DEVFONT_SCALE (logfont, devfont);
}

static int get_max_width (LOGFONT* logfont, DEVFONT* devfont)
{
    return QPFONT_INFO_P (devfont)->fm->maxwidth
            * GET_DEVFONT_SCALE (logfont, devfont);
}

static int get_font_height (LOGFONT* logfont, DEVFONT* devfont)
{
    return (QPFONT_INFO_P (devfont)->fm->ascent 
                    + QPFONT_INFO_P (devfont)->fm->descent) 
            * GET_DEVFONT_SCALE (logfont, devfont);
}

static int get_font_size (LOGFONT* logfont, DEVFONT* devfont, int expect)
{
#ifdef _USE_NEWGAL
    int height = QPFONT_INFO_P (devfont)->fm->ascent + 
            QPFONT_INFO_P (devfont)->fm->descent;
    unsigned short scale = 1;

    if (logfont->style & FS_OTHER_AUTOSCALE)
        scale = GetBestScaleFactor (height, expect);

    SET_DEVFONT_SCALE (logfont, devfont, scale);

    return height * scale;
#else
    return get_font_height (logfont, devfont);
#endif
}

static int get_font_ascent (LOGFONT* logfont, DEVFONT* devfont)
{
    return QPFONT_INFO_P (devfont)->fm->ascent
            * GET_DEVFONT_SCALE (logfont, devfont);
}

static int get_font_descent (LOGFONT* logfont, DEVFONT* devfont)
{
    return QPFONT_INFO_P (devfont)->fm->descent
            * GET_DEVFONT_SCALE (logfont, devfont);
}

static const void* get_char_bitmap (LOGFONT* logfont, DEVFONT* devfont,
            const unsigned char* mchar, int len, unsigned short* scale)
{
    unsigned int uc16;
    GLYPH* glyph;

    uc16 = (*devfont->charset_ops->conv_to_uc32) (mchar);
    glyph = get_glyph (QPFONT_INFO_P (devfont)->tree, uc16);

    if (glyph == NULL) {
        if (uc16 < 0x80 && logfont->sbc_devfont != devfont) {   /* ASCII */
            unsigned char ascii_ch = uc16;
            return logfont->sbc_devfont->font_ops->get_char_bitmap (logfont,
                            logfont->sbc_devfont, &ascii_ch, 1, scale);
        }
        else
            glyph = &def_glyph;
    }

    if (QPFONT_INFO_P (devfont)->fm->flags & FM_SMOOTH) {
        glyph = &def_glyph;
    }

    if (scale)
        *scale = GET_DEVFONT_SCALE (logfont, devfont);
    return glyph->data;
}

static const void* get_char_pixmap (LOGFONT* logfont, DEVFONT* devfont,
            const unsigned char* mchar, int len, 
            int* pitch, unsigned short* scale)
{
    unsigned int uc16;
    GLYPH* glyph;

    if (scale)
        *scale = GET_DEVFONT_SCALE (logfont, devfont);

    if (!(QPFONT_INFO_P (devfont)->fm->flags & FM_SMOOTH)) {
        return NULL;
    }

    uc16 = (*devfont->charset_ops->conv_to_uc32) (mchar);
    glyph = get_glyph (QPFONT_INFO_P (devfont)->tree, uc16);

    if (glyph == NULL) {
        glyph = &def_smooth_glyph;
    }

    *pitch = glyph->metrics->linestep;
    return glyph->data;
}

static int get_char_bbox (LOGFONT* logfont, DEVFONT* devfont, 
                const unsigned char* mchar, int len, 
                int* px, int* py, int* pwidth, int* pheight)
{
    unsigned int uc16;
    GLYPH* glyph;
    unsigned short scale = GET_DEVFONT_SCALE (logfont, devfont);

    uc16 = (*devfont->charset_ops->conv_to_uc32) (mchar);
    glyph = get_glyph (QPFONT_INFO_P (devfont)->tree, uc16);

    if (glyph == NULL) {
        if (uc16 < 0x80 && logfont->sbc_devfont != devfont) {   /* ASCII */
            unsigned char ascii_ch = uc16;
            if (logfont->sbc_devfont->font_ops->get_char_bbox) {
                return logfont->sbc_devfont->font_ops->get_char_bbox (logfont,
                            logfont->sbc_devfont, &ascii_ch, 1,
                            px, py, pwidth, pheight);
            }
            else {
                int width = logfont->sbc_devfont->font_ops->get_char_width 
                        (logfont, logfont->sbc_devfont, &ascii_ch, 1);
                int height = logfont->sbc_devfont->font_ops->get_font_height 
                        (logfont, logfont->sbc_devfont);
                int ascent = logfont->sbc_devfont->font_ops->get_font_ascent
                        (logfont, logfont->sbc_devfont);

                if (pwidth) *pwidth = width;
                if (pheight) *pheight = height;
                if (py) *py  -= ascent;

                return width;
            }
        }
        else {
            glyph = &def_glyph;
            def_metrics.bearingy = (QPFONT_INFO_P (devfont)->fm->ascent+2)/2;
        }
    }

    if (pwidth) *pwidth = glyph->metrics->width * scale;
    if (pheight) *pheight = glyph->metrics->height * scale;

    if (px) *px += glyph->metrics->bearingx * scale;
    if (py) *py -= glyph->metrics->bearingy * scale;
    return glyph->metrics->width * scale;
}

static void get_char_advance (LOGFONT* logfont, DEVFONT* devfont, 
                const unsigned char* mchar, int len, int* px, int* py)
{
    unsigned int uc16;
    GLYPH* glyph;

    uc16 = (*devfont->charset_ops->conv_to_uc32) (mchar);
    glyph = get_glyph (QPFONT_INFO_P (devfont)->tree, uc16);

    if (glyph == NULL) {
        if (uc16 < 0x80 && logfont->sbc_devfont != devfont) {   /* ASCII */
            unsigned char ascii_ch = uc16;
            if (logfont->sbc_devfont->font_ops->get_char_advance) {
                logfont->sbc_devfont->font_ops->get_char_advance (logfont,
                            logfont->sbc_devfont, &ascii_ch, 1, px, py);
            }
            else {
                *px += logfont->sbc_devfont->font_ops->get_char_width (logfont, 
                                logfont->sbc_devfont, &ascii_ch, 1);
            }

            return;
        }
        else
            glyph = &def_glyph;
    }

    *px += glyph->metrics->advance 
            * GET_DEVFONT_SCALE (logfont, devfont);
}

static DEVFONT* new_instance (LOGFONT* logfont, DEVFONT* devfont,
               BOOL need_sbc_font)
{
    if (QPFONT_INFO_P (devfont)->fm->flags & FM_SMOOTH) {
        logfont->style &= ~FS_WEIGHT_SUBPIXEL;
        logfont->style |=  FS_WEIGHT_BOOK;
    }

    return devfont;
}

/**************************** Global data ************************************/
FONTOPS qpf_font_ops = {
    get_char_width,
    get_ave_width,
    get_max_width,
    get_font_height,
    get_font_size,
    get_font_ascent,
    get_font_descent,

    get_char_bitmap,
    get_char_pixmap,
    NULL,
    get_char_bbox,
    get_char_advance,
    new_instance,
    NULL
};

#endif  /* _QPF_SUPPORT */

