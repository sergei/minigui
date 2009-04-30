/*
** $Id: devfont.c 9066 2008-01-24 05:42:50Z xwyan $
** 
** defont.c: Device fonts management.
**
** Copyright (C) 2003 ~ 2007 Feynman Software.
** Copyright (C) 2000 ~ 2002 Wei Yongming.
**
** All right reserved by Feynman Software.
**
** Current maintainer: Wei Yongming.
** 
** Create Date: 2000/07/07
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "minigui.h"
#include "gdi.h"

#include "sysfont.h"
#include "charset.h"
#include "devfont.h"
#include "fontname.h"
#include "rawbitmap.h"
#include "varbitmap.h"

#if defined(_TTF_SUPPORT) && !defined(_FT2_SUPPORT)
#include "freetype.h"
#endif /* _TTF_SUPPORT */

#ifdef _FT2_SUPPORT
#include "freetype2.h"
#endif /* _FT2_SUPPORT */

#ifdef _QPF_SUPPORT
#include "qpf.h"
#endif  /* _QPF_SUPPORT */

#ifdef _LITE_VERSION

static int init_count = 0;
BOOL GUIAPI InitVectorialFonts (void)
{
    if (init_count > 0)
       goto success;

#if defined (_TTF_SUPPORT) || defined (_FT2_SUPPORT)
    if (!InitFreeTypeFonts ()) {
        fprintf (stderr, 
            "GDI: Can not initialize TrueType fonts!\n");
        return FALSE;
    }
#endif

#ifdef _TYPE1_SUPPORT
    if (!InitType1Fonts ()) {
        fprintf (stderr, 
            "GDI: Can not initialize Type1 fonts!\n");
        goto error;
    }
#endif

success:
    init_count ++;
    return TRUE;

#ifdef _TYPE1_SUPPORT
error:
#endif

#if defined (_TTF_SUPPORT) || defined (_FT2_SUPPORT)
    TermFreeTypeFonts ();
#endif
    return FALSE;
}

void GUIAPI TermVectorialFonts (void)
{
    init_count --;
    if (init_count != 0)
        return;

#ifdef _TYPE1_SUPPORT
	TermType1Fonts();
#endif

#if defined (_TTF_SUPPORT) || defined (_FT2_SUPPORT)
    TermFreeTypeFonts ();
#endif
}

#endif /* _LITE_VERSION */

/**************************** Global data ************************************/
static DEVFONT* sb_dev_font_head = NULL;
static DEVFONT* mb_dev_font_head = NULL;
static int nr_sb_dev_fonts;
static int nr_mb_dev_fonts;

#define devfontDelSBCDevFont(head, del)         \
{                                               \
    if (head == del) {                          \
        head = head->sbc_next;                  \
    }                                           \
    else {                                      \
        DEVFONT* tail;                          \
        tail = head;                            \
        while (tail) {                          \
            if (tail->sbc_next == del) {        \
                tail->sbc_next = del->sbc_next; \
                break;                          \
            }                                   \
            tail = tail->sbc_next;              \
        }                                       \
    }                                           \
}

#define devfontDelMBCDevFont(head, del)         \
{                                               \
    if (head == del) {                          \
        head = head->mbc_next;                  \
    }                                           \
    else {                                      \
        DEVFONT* tail;                          \
        tail = head;                            \
        while (tail) {                          \
            if (tail->mbc_next == del) {        \
                tail->mbc_next = del->mbc_next; \
                break;                          \
            }                                   \
            tail = tail->mbc_next;              \
        }                                       \
    }                                           \
}

#define devfontAddSBCDevFont(head, new)         \
{                                               \
    if (head == NULL)                           \
        head = new;                             \
    else {                                      \
        DEVFONT* tail;                          \
        tail = head;                            \
        while (tail->sbc_next) tail = tail->sbc_next;   \
        tail->sbc_next = new;                       \
    }                                           \
                                                \
    new->sbc_next = NULL;                           \
}

#define devfontAddMBCDevFont(head, new)         \
{                                               \
    if (head == NULL)                           \
        head = new;                             \
    else {                                      \
        DEVFONT* tail;                          \
        tail = head;                            \
        while (tail->mbc_next) tail = tail->mbc_next;   \
        tail->mbc_next = new;                       \
    }                                           \
                                                \
    new->mbc_next = NULL;                           \
}

void AddSBDevFont (DEVFONT* dev_font)
{
    devfontAddSBCDevFont (sb_dev_font_head, dev_font);
    dev_font->style = fontGetStyleFromName (dev_font->name);
    nr_sb_dev_fonts ++;
}

void AddMBDevFont (DEVFONT* dev_font)
{
    devfontAddMBCDevFont (mb_dev_font_head, dev_font);
    dev_font->style = fontGetStyleFromName (dev_font->name);
    nr_mb_dev_fonts ++;
}

void DelRelatedDevFont (DEVFONT* dev_font)
{
    DEVFONT *font, *relate_font;

    if (!dev_font)
        return;

    font = sb_dev_font_head;

    while (font) {
        if (font->relationship == dev_font) {
            relate_font = font;
            font = font->sbc_next;
            if (relate_font->charset_ops->bytes_maxlen_char > 1) {
                devfontDelMBCDevFont (mb_dev_font_head, relate_font);
            }
            else {
                devfontDelSBCDevFont (sb_dev_font_head, relate_font);
            }
            free (relate_font);
            relate_font = NULL;
            nr_sb_dev_fonts--;
            continue;
        }
        font = font->sbc_next;
    }

    font = mb_dev_font_head;

    while (font) {
        if (font->relationship == dev_font) {
            relate_font = font;
            font = font->mbc_next;
            if (relate_font->charset_ops->bytes_maxlen_char > 1) {
                devfontDelMBCDevFont (mb_dev_font_head, relate_font);
            }
            else {
                devfontDelSBCDevFont (sb_dev_font_head, relate_font);
            }
            free (relate_font);
            relate_font = NULL;
            nr_mb_dev_fonts--;
            continue;
        }
        font = font->mbc_next;
    }
}

void ResetDevFont (void)
{
    sb_dev_font_head = mb_dev_font_head = NULL;
    nr_sb_dev_fonts = 0;
    nr_mb_dev_fonts = 0;
}

#define MATCHED_TYPE        0x01
#define MATCHED_FAMILY      0x02
#define MATCHED_CHARSET     0x04

DEVFONT* GetMatchedSBDevFont (LOGFONT* log_font)
{
    int i;
    char charset_req [LEN_FONT_NAME + 1];
    int min_error;
    DEVFONT* dev_font;
    DEVFONT* matched_font;

#ifdef HAVE_ALLOCA
    BYTE* match_bits = alloca (nr_sb_dev_fonts);
#else
    BYTE* match_bits = (BYTE*)FixStrAlloc (nr_sb_dev_fonts);
#endif

    /* is charset requested is a single byte charset? */
    if (GetCharsetOps (log_font->charset)->bytes_maxlen_char > 1) {
        fontGetCharsetFromName (g_SysLogFont[0]->sbc_devfont->name, charset_req);
    }
    else
        strcpy (charset_req, log_font->charset);
        
    i = 0;
    dev_font = sb_dev_font_head;
    while (dev_font) {
        int type_req;
        char family [LEN_FONT_NAME + 1];
        
        /* clear match_bits first. */
        match_bits [i] = 0;

        /* does match this font type? */
        type_req = fontConvertFontType (log_font->type);
        if (type_req == FONT_TYPE_ALL)
            match_bits [i] |= MATCHED_TYPE;
        else if (type_req == fontGetFontTypeFromName (dev_font->name))
            match_bits [i] |= MATCHED_TYPE;

        /* does match this family? */
        fontGetFamilyFromName (dev_font->name, family);
        if (strcasecmp (family, log_font->family) == 0) {
            match_bits [i] |= MATCHED_FAMILY;
        }

        /* does match this charset */
        if (IsCompatibleCharset (charset_req, dev_font->charset_ops)) {
            match_bits [i] |= MATCHED_CHARSET;
        }

        /* FIXME: ignore style */

        dev_font = dev_font->sbc_next;
        i ++;
    }

    min_error = FONT_MAX_SIZE;
    matched_font = NULL;
    dev_font = sb_dev_font_head;
    for (i = 0; i < nr_sb_dev_fonts; i++) {
        int error;
        if ((match_bits [i] & MATCHED_TYPE)
                && (match_bits [i] & MATCHED_FAMILY)
                && (match_bits [i] & MATCHED_CHARSET)) {
            error = abs (log_font->size - 
                    (*dev_font->font_ops->get_font_size) (log_font, dev_font, 
                                                          log_font->size));
            if (min_error > error) {
                min_error = error;
                matched_font = dev_font;
            }
        }
        dev_font = dev_font->sbc_next;
    }

    if (matched_font) {
#ifndef HAVE_ALLOCA
        FreeFixStr ((char*)match_bits);
#endif
        matched_font->font_ops->get_font_size (log_font, matched_font, log_font->size);
        return matched_font;
    }

    /* check charset here. */
    min_error = FONT_MAX_SIZE;
    dev_font = sb_dev_font_head;
    for (i = 0; i < nr_sb_dev_fonts; i++) {
        int error;
        if (match_bits [i] & MATCHED_CHARSET) {
            error = log_font->size - 
                    (*dev_font->font_ops->get_font_size) (log_font, dev_font, log_font->size);
            error = ABS (error);
            if (min_error > error) {
                min_error = error;
                matched_font = dev_font;
            }
        }
        dev_font = dev_font->sbc_next;
    }

#ifndef HAVE_ALLOCA
    FreeFixStr ((char*)match_bits);
#endif
    if (matched_font == NULL)
       matched_font = g_SysLogFont [0]->sbc_devfont;

    matched_font->font_ops->get_font_size (log_font, matched_font, log_font->size);
        
    return matched_font;
}

DEVFONT* GetMatchedMBDevFont (LOGFONT* log_font)
{
    DEVFONT* dev_font;
    int i;
    BYTE* match_bits;
    char charset_req [LEN_FONT_NAME + 1];
    int min_error;
    DEVFONT* matched_font;

    /* is charset requested is a multiple-byte charset? */
    if (GetCharsetOps (log_font->charset)->bytes_maxlen_char < 2)
        return NULL;

#ifdef HAVE_ALLOCA
    match_bits = alloca (nr_sb_dev_fonts);
#else
    match_bits = (BYTE *)FixStrAlloc (nr_mb_dev_fonts);
#endif

    strcpy (charset_req, log_font->charset);
        
    i = 0;
    dev_font = mb_dev_font_head;
    while (dev_font) {
        int type_req;
        char family [LEN_FONT_NAME + 1];
        
        /* clear match_bits first. */
        match_bits [i] = 0;

        /* does match this font type? */
        type_req = fontConvertFontType (log_font->type);
        if (type_req == FONT_TYPE_ALL)
            match_bits [i] |= MATCHED_TYPE;
        else if (type_req == fontGetFontTypeFromName (dev_font->name))
            match_bits [i] |= MATCHED_TYPE;

        /* does match this family? */
        fontGetFamilyFromName (dev_font->name, family);
        if (strcasecmp (family, log_font->family) == 0) {
            match_bits [i] |= MATCHED_FAMILY;
        }

        /* does match this charset */
        if (IsCompatibleCharset (charset_req, dev_font->charset_ops)) {
            match_bits [i] |= MATCHED_CHARSET;
        }

        /* FIXME: ignore style */

        dev_font = dev_font->mbc_next;
        i ++;
    }

    matched_font = NULL;
    min_error = FONT_MAX_SIZE;
    dev_font = mb_dev_font_head;
    for (i = 0; i < nr_mb_dev_fonts; i++) {
        int error;
        if ((match_bits [i] & MATCHED_TYPE)
                && (match_bits [i] & MATCHED_FAMILY)
                && (match_bits [i] & MATCHED_CHARSET)) {
            error = log_font->size - 
                    (*dev_font->font_ops->get_font_size) (log_font, dev_font, 
                                                          log_font->size);
            error = ABS (error);
            if (min_error > error) {
                min_error = error;
                matched_font = dev_font;
            }
        }
        dev_font = dev_font->mbc_next;
    }

    if (matched_font) {
#ifndef HAVE_ALLOCA
        FreeFixStr ((char*)match_bits);
#endif
        matched_font->font_ops->get_font_size (log_font, matched_font, log_font->size);
        return matched_font;
    }

    min_error = FONT_MAX_SIZE;
    dev_font = mb_dev_font_head;
    for (i = 0; i < nr_mb_dev_fonts; i++) {
        int error;
        if (match_bits [i] & MATCHED_CHARSET) {
            error = log_font->size - 
                    (*dev_font->font_ops->get_font_size) (log_font, dev_font, 
                                                          log_font->size);
            error = ABS (error);
            if (min_error > error) {
                min_error = error;
                matched_font = dev_font;
            }
        }
        dev_font = dev_font->mbc_next;
    }

#ifndef HAVE_ALLOCA
    FreeFixStr ((char*)match_bits);
#endif
    if (matched_font)
        matched_font->font_ops->get_font_size (log_font, matched_font, log_font->size);
    return matched_font;
}

const DEVFONT* GUIAPI GetNextDevFont (const DEVFONT* dev_font)
{
    if (dev_font == NULL) {
        return sb_dev_font_head;
    }
    else if (dev_font->charset_ops->bytes_maxlen_char == 1) {
        dev_font = dev_font->sbc_next;
        if (dev_font == NULL)
            return mb_dev_font_head;

        return dev_font;
    }

    return dev_font->mbc_next;
}

#ifdef _USE_NEWGAL
unsigned short GetBestScaleFactor (int height, int expect)
{
    int error, min_error;
    unsigned short scale = 1;
    
    min_error = height - expect;
    min_error = ABS (min_error);

    while (min_error) {
        scale ++;
        error = (height * scale) - expect;
        error = ABS (error);

        if (error == 0)
            break;

        if (error > min_error) {
            scale --;
            break;
        }
        else
            min_error = error;
    }

    return scale;
}
#endif /* _USE_NEWGAL */

BOOL GUIAPI LoadDevFontFromFile 
    (const char *devfont_name, const char *file_name, DEVFONT **devfont)
{
	DEVFONT*    dev_font = NULL;
	char        type_name [LEN_FONT_NAME + 1];

    if (!devfont_name || !file_name)
        return FALSE;

	if (!fontGetTypeNameFromName (devfont_name, type_name)) {
		fprintf(stderr, "Font type error\n");
		return FALSE;
	}

	if (!strcasecmp (type_name, FONT_TYPE_NAME_SCALE_TTF)) {
#if defined(_TTF_SUPPORT) && !defined(_FT2_SUPPORT)
		dev_font = 
            (DEVFONT*)__mg_ttfload_devfont_fromfile (devfont_name, file_name);
#elif defined (_FT2_SUPPORT)
		dev_font = 
            (DEVFONT*)__mg_ft2load_devfont_fromfile (devfont_name, file_name);
#else
		fprintf(stderr, "Sorry, you don't open freetype font engine.\n");
        return FALSE;
#endif
	} else if (!strcasecmp (type_name, FONT_TYPE_NAME_BITMAP_QPF)) {
#if !defined(__NOUNIX__) && defined(_QPF_SUPPORT)
		dev_font = 
            (DEVFONT*)__mg_qpfload_devfont_fromfile (devfont_name, file_name);
#else
		fprintf(stderr, "Sorry, you don't open qpf font engine.\n");
		return FALSE;
#endif
	} else {
		fprintf(stderr, "font type error, it must be freetype or qpf\n");
		return FALSE;
	}

	*devfont = dev_font;
	return (dev_font != NULL);
}

void GUIAPI DestroyDynamicDevFont (DEVFONT **devfont)
{
	char type_name [LEN_FONT_NAME + 1];

    if (!devfont || !(*devfont))
        return;

	if (!fontGetTypeNameFromName ((*devfont)->name, type_name)) {
		fprintf(stderr, "Destroy device font type error.\n");
		return;
	}

	if (!strcasecmp (type_name, FONT_TYPE_NAME_SCALE_TTF)) {
#if defined(_TTF_SUPPORT) && !defined(_FT2_SUPPORT)
            __mg_ttf_destroy_devfont_fromfile (devfont);
#elif defined (_FT2_SUPPORT)
            __mg_ft2_destroy_devfont_fromfile (devfont);
#else
		fprintf(stderr, "Sorry, you don't open freetype font engine.\n");
#endif
	} else if (!strcasecmp (type_name, FONT_TYPE_NAME_BITMAP_QPF)) {
#if !defined(__NOUNIX__) && defined(_QPF_SUPPORT)
            __mg_qpf_destroy_devfont_fromfile (devfont);
#else
		fprintf(stderr, "Sorry, you don't open qpf font engine.\n");
#endif
	}

    *devfont = NULL;
}

#ifdef _DEBUG
void dumpDevFonts (void)
{
    int         count = 0;
    DEVFONT*    devfont;

    fprintf (stderr, "============= SBDevFonts ============\n");

    devfont = sb_dev_font_head;
    while (devfont) {
        fprintf (stderr, "  %d: %s, charsetname: %s, style: %#lx\n", 
                count, 
                devfont->name, devfont->charset_ops->name, devfont->style);
        devfont = devfont->sbc_next;
        count++;
    }
    fprintf (stderr, "========== End of SBDevFonts =========\n");

    fprintf (stderr, "\n============= MBDevFonts ============\n");

    devfont = mb_dev_font_head;
    while (devfont) {
        fprintf (stderr, "  %d: %s, charsetname: %s, style: %#lx\n", 
                count, 
                devfont->name, devfont->charset_ops->name, devfont->style);
        devfont = devfont->mbc_next;
        count++;
    }
    fprintf (stderr, "========== End of MBDevFonts =========\n");
}
#endif

void DelSBDevFont (DEVFONT* dev_font)
{
  DEVFONT *tmp, *prev = NULL;

  tmp = sb_dev_font_head;
  while (tmp != NULL) {
      if (tmp == dev_font) {
	  if (prev != NULL)
              prev->sbc_next = tmp->sbc_next;
	  else
              sb_dev_font_head = tmp->sbc_next;

	  free (tmp);
          nr_sb_dev_fonts --;
          
	  break;
      }
      
      prev = tmp;
      tmp = prev->sbc_next;
  }
  
  return;
}

/*  
 * Removes an element from multiple-byte devfont linked list. If two elements
 * contain the same data, only the first is removed. If none of the elements
 * contain the data, the multiple-byte devfont linked list is unchanged   
 */
void DelMBDevFont (DEVFONT* dev_font)
{
  DEVFONT *tmp, *prev = NULL;

  tmp = mb_dev_font_head;
  while (tmp != NULL) {
      if (tmp == dev_font) {
	  if (prev != NULL)
              prev->mbc_next = tmp->mbc_next;
	  else
              mb_dev_font_head = tmp->mbc_next;

	  free (tmp);
          nr_mb_dev_fonts --;
          
	  break;
      }
      prev = tmp;
      tmp = prev->mbc_next;
  }
  
  return;
}

void DelDevFont (DEVFONT* dev_font, BOOL del_relate)
{
    if (del_relate)
        DelRelatedDevFont (dev_font);

    if (dev_font->charset_ops->bytes_maxlen_char > 1)
        DelMBDevFont (dev_font);
    else
        DelSBDevFont (dev_font);
}
