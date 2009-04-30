/*
** $Id: text.c 7742 2007-09-29 07:08:34Z weiym $
**
** text.c: The Text Support of GDI.
**
** Copyright (C) 2003 ~ 2007 Feynman Software.
** Copyright (C) 2000 ~ 2002 Wei Yongming.
**
** Current maintainer: Wei Yongming.
**
** Create date: 2000/4/19
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "minigui.h"
#include "gdi.h"
#include "window.h"
#include "cliprect.h"
#include "gal.h"
#include "internals.h"
#include "ctrlclass.h"
#include "dc.h"
#include "drawtext.h"
#include "cursor.h"

#ifdef _UNICODE_SUPPORT
extern size_t __mg_strlen (PLOGFONT log_font, const char* mstr);
extern char* __mg_strnchr (PLOGFONT log_font, const char* s, 
                size_t n, int c, int* cl);
extern int __mg_substrlen (PLOGFONT log_font, const char* text, int len, 
                int delimiter, int* nr_delim);

static inline BOOL is_utf16_logfont (PDC pdc)
{
    DEVFONT* mbc_devfont; 
    mbc_devfont = pdc->pLogFont->mbc_devfont;
    if (mbc_devfont && strstr (mbc_devfont->charset_ops->name, "UTF-16")) {
        return TRUE;
    }

    return FALSE;
}

#else

static inline size_t __mg_strlen (PLOGFONT log_font, const char* mstr)
{
    return strlen (mstr);
}

static inline char* __mg_strnchr (PLOGFONT logfont, const char* s, 
                size_t n, int c, int* cl)
{
    *cl = 1;
    return strnchr (s, n, c);
}

static inline int __mg_substrlen (PLOGFONT logfont, const char* text, int len, 
                int delimiter, int* nr_delim)
{
    return substrlen (text, len, delimiter, nr_delim);
}

static inline BOOL is_utf16_logfont (PDC pdc)
{
    return FALSE;
}

#endif

int GUIAPI GetFontHeight (HDC hdc)
{
    PDC pdc = dc_HDC2PDC(hdc);
#if 0
    DEVFONT* sbc_devfont = pdc->pLogFont->sbc_devfont;
    DEVFONT* mbc_devfont = pdc->pLogFont->mbc_devfont;
    int sbc_height = (*sbc_devfont->font_ops->get_font_height) 
            (pdc->pLogFont, sbc_devfont);
    int mbc_height = 0;

    if (mbc_devfont)
        mbc_height = (*mbc_devfont->font_ops->get_font_height) 
                (pdc->pLogFont, mbc_devfont);
    
    return MAX(mbc_height, sbc_height);
#else
    return pdc->pLogFont->size;
#endif
}

int GUIAPI GetMaxFontWidth (HDC hdc)
{
    PDC pdc = dc_HDC2PDC(hdc);
    DEVFONT* sbc_devfont = pdc->pLogFont->sbc_devfont;
    DEVFONT* mbc_devfont = pdc->pLogFont->mbc_devfont;
    int sbc_max_width = (*sbc_devfont->font_ops->get_max_width) 
            (pdc->pLogFont, sbc_devfont);
    int mbc_max_width = 0;

    if (mbc_devfont)
        mbc_max_width = (*mbc_devfont->font_ops->get_max_width) 
                (pdc->pLogFont, mbc_devfont);
    
    return (sbc_max_width > mbc_max_width) ? sbc_max_width : mbc_max_width;
}

void GUIAPI GetTextExtent (HDC hdc, const char* spText, int len, SIZE* pSize)
{
    PDC pdc;

    pdc = dc_HDC2PDC(hdc);
    if (len < 0) len = __mg_strlen (pdc->pLogFont, spText);

    gdi_get_TextOut_extent (pdc, pdc->pLogFont, spText, len, pSize);
}

void GUIAPI GetTabbedTextExtent (HDC hdc, const char* spText, int len, 
                SIZE* pSize)
{
    PDC pdc;
    int tab_width;

    pdc = dc_HDC2PDC(hdc);
    if (is_utf16_logfont (pdc)) return;
    if (len < 0) len = __mg_strlen (pdc->pLogFont, spText);

    tab_width = pdc->tabstop 
            * (*pdc->pLogFont->sbc_devfont->font_ops->get_ave_width)
                    (pdc->pLogFont, pdc->pLogFont->sbc_devfont);

    gdi_get_TabbedTextOut_extent (pdc, pdc->pLogFont, 
                    tab_width, spText, len, pSize, NULL);
}

void GUIAPI GetLastTextOutPos (HDC hdc, POINT* pt)
{
    PDC pdc;

    pdc = dc_HDC2PDC(hdc);
    *pt = pdc->CurTextPos;
}

int GUIAPI TextOutLen (HDC hdc, int x, int y, const char* spText, int len)
{
    PDC pdc;
    SIZE size;

    if (!spText || len == 0) return 0;

    pdc = dc_HDC2PDC(hdc);
    if (len < 0) len = __mg_strlen (pdc->pLogFont, spText);

    gdi_get_TextOut_extent (pdc, pdc->pLogFont, spText, len, &size);
    {
        /* update text out position. */
        int width = size.cx;

        extent_x_SP2LP (pdc, &width);
        pdc->CurTextPos.x = x + width;
        pdc->CurTextPos.y = y;
    }

    if (dc_IsGeneralDC (pdc)) {
        LOCK_GCRINFO (pdc);
        if (!dc_GenerateECRgn (pdc, FALSE)) {
            UNLOCK_GCRINFO (pdc);
            return size.cx;
        }
    }

#ifdef _LITE_VERSION
    if (CHECK_DRAWING (pdc)) goto no_draw;
#endif

    /* Transfer logical to device to screen here. */
    coor_LP2SP(pdc, &x, &y);

    /* set rc_output to full device rect. */
    pdc->rc_output = pdc->DevRC;

    /* output text. */
    gdi_strnwrite (pdc, x, y, spText, len);

#ifdef _LITE_VERSION
no_draw:
#endif
    /* we are done. */
    UNLOCK_GCRINFO (pdc);

    return size.cx;
}

int GUIAPI TabbedTextOutLen (HDC hdc, int x, int y, const char* spText, int len) 
{
    PDC pdc;
    SIZE size;
    int tab_width;

    if (!spText || len == 0) return 0;

    pdc = dc_HDC2PDC(hdc);
    if (is_utf16_logfont (pdc)) return 0;
    if (len < 0) len = __mg_strlen (pdc->pLogFont, spText);

    coor_LP2SP (pdc, &pdc->CurTextPos.x, &pdc->CurTextPos.y);

    tab_width = pdc->tabstop 
            * (*pdc->pLogFont->sbc_devfont->font_ops->get_ave_width)
                    (pdc->pLogFont, pdc->pLogFont->sbc_devfont);
    gdi_get_TabbedTextOut_extent (pdc, pdc->pLogFont, 
                    tab_width, spText, len, &size, &pdc->CurTextPos);

    /* update text out position. */
    coor_SP2LP (pdc, &pdc->CurTextPos.x, &pdc->CurTextPos.y);

    if (dc_IsGeneralDC (pdc)) {
        LOCK_GCRINFO (pdc);
        if (!dc_GenerateECRgn (pdc, FALSE)) {
            UNLOCK_GCRINFO (pdc);
            return size.cx;
        }
    }

#ifdef _LITE_VERSION
    if (CHECK_DRAWING (pdc)) goto no_draw;
#endif

    /* Transfer logical to device to screen here. */
    coor_LP2SP(pdc, &x, &y);

    /* set rc_output to full device rect. */
    pdc->rc_output = pdc->DevRC;

    /* output text */
    gdi_tabbedtextout (pdc, x, y, spText, len);

#ifdef _LITE_VERSION
no_draw:
#endif
    /* done! */
    UNLOCK_GCRINFO (pdc);

    return size.cx;
}

int GUIAPI TabbedTextOutEx (HDC hdc, int x, int y, const char* spText,
		int nCount, int nTabs, int *pTabPos, int nTabOrig)
{
    PDC pdc;
    int line_len, sub_len;
    int nr_tab = 0, tab_pos, def_tab;
    int x_orig = x, max_x = x;
    int line_height;
    int nr_delim_newline, nr_delim_tab;

	DEVFONT* sbc_devfont;
	DEVFONT* mbc_devfont;	
	int mbc_height = 0;
	int sbc_height = 0;

    if (!spText || nCount == 0) return 0;

    pdc = dc_HDC2PDC(hdc);
    if (is_utf16_logfont (pdc)) return 0;
    if (nCount < 0) nCount = __mg_strlen (pdc->pLogFont, spText);

	/* Start: Fixed bug of italic font */
	sbc_devfont = pdc->pLogFont->sbc_devfont;
	mbc_devfont = pdc->pLogFont->mbc_devfont;

	sbc_height = (*sbc_devfont->font_ops->get_font_height) 
			(pdc->pLogFont, sbc_devfont);

	if (mbc_devfont) {
		mbc_height = (*mbc_devfont->font_ops->get_font_height) 
			(pdc->pLogFont, mbc_devfont);
	}
	/* End: Fixed bug of italic font */

    line_height = pdc->pLogFont->size + pdc->alExtra + pdc->blExtra;
    y += pdc->alExtra;
    if (nTabs == 0 || pTabPos == NULL) {
        int ave_width = (*pdc->pLogFont->sbc_devfont->font_ops->get_ave_width)
                        (pdc->pLogFont, pdc->pLogFont->sbc_devfont);
        def_tab = ave_width * pdc->tabstop;
    }
    else
        def_tab = pTabPos [nTabs - 1];

    while (nCount) {
        line_len = __mg_substrlen (pdc->pLogFont, 
                        spText, nCount, '\n', &nr_delim_newline);

        nCount -= line_len + nr_delim_newline;

        nr_tab = 0;
        x = x_orig;
        tab_pos = nTabOrig;
        while (line_len) {
            int i, width;

            sub_len = __mg_substrlen (pdc->pLogFont, 
                        spText, line_len, '\t', &nr_delim_tab);

            width = TextOutLen (hdc, x, y, spText, sub_len);

            x += width; 
            if (x >= tab_pos) {
                while (x >= tab_pos)
                    tab_pos += (nr_tab >= nTabs) ? def_tab : pTabPos [nr_tab++];
                for (i = 0; i < nr_delim_tab - 1; i ++)
                    tab_pos += (nr_tab >= nTabs) ? def_tab : pTabPos [nr_tab++];
            }
            else {
                for (i = 0; i < nr_delim_tab; i ++)
                    tab_pos += (nr_tab >= nTabs) ? def_tab : pTabPos [nr_tab++];
            }

	        /* Start: Fixed bug of background handling */
			if (pdc->bkmode != BM_TRANSPARENT && sub_len != line_len)
			{
				gal_pixel old_brush_color = pdc->brushcolor;	
				pdc->brushcolor = pdc->bkcolor;
				FillBox(hdc, x, y, tab_pos-x, MAX(mbc_height, sbc_height));
				pdc->brushcolor = old_brush_color;
			}
	        /* End: Fixed bug of background handling */

            x = tab_pos;

            line_len -= sub_len + nr_delim_tab;
            spText += sub_len + nr_delim_tab;
        }

        if (max_x < x) max_x = x;

        spText += nr_delim_newline;
        y += line_height * nr_delim_newline;
    }

    return max_x - x_orig;
}

static void txtDrawOneLine (PDC pdc, const char* pText, int nLen, int x, int y,
                    const RECT* prcOutput, UINT nFormat, int nTabWidth)
{
    /* set rc_output to the possible clipping rect */
    pdc->rc_output = *prcOutput;

    if (nFormat & DT_EXPANDTABS) {
        const char* sub = pText;
        const char* left;
        int nSubLen = nLen;
        int nOutputLen;
        int nTabLen;
                
        while ((left = __mg_strnchr (pdc->pLogFont, sub, 
                    nSubLen, '\t', &nTabLen))) {
                    
            nOutputLen = left - sub;
            x += gdi_strnwrite (pdc, x, y, sub, nOutputLen);
                    
            nSubLen -= (nOutputLen + nTabLen);
            sub = left + nTabLen;
            x += nTabWidth;
        }

        if (nSubLen > 0)
            gdi_strnwrite (pdc, x, y, sub, nSubLen);
    }
    else
        gdi_strnwrite (pdc, x, y, pText, nLen);
}

static int txtGetWidthOfNextWord (PDC pdc, const char* pText, int nCount, 
                int* nChars)
{
    SIZE size;
    DEVFONT* sbc_devfont = pdc->pLogFont->sbc_devfont;
    DEVFONT* mbc_devfont = pdc->pLogFont->mbc_devfont;
    WORDINFO word_info = {0}; 

    *nChars = 0;
    if (nCount == 0) return 0;

    if (mbc_devfont) {
        int mbc_pos, sub_len;

        mbc_pos = (*mbc_devfont->charset_ops->pos_first_char) 
                ((const unsigned char*)pText, nCount);
        if (mbc_pos == 0) {
            sub_len = (*mbc_devfont->charset_ops->len_first_substr) 
                    ((const unsigned char*)pText, nCount);

            (*mbc_devfont->charset_ops->get_next_word) 
                    ((const unsigned char*)pText, sub_len, &word_info);

            if (word_info.len == 0) {
                *nChars = 0;
                return 0;
            }
        }
        else if (mbc_pos > 0)
            nCount = mbc_pos;
    }

    if (word_info.len == 0)
        (*sbc_devfont->charset_ops->get_next_word) 
            ((const unsigned char*)pText, nCount, &word_info);

#if 0
    if (pdc->pLogFont->style & FS_WEIGHT_BOLD 
            && !(sbc_devfont->style & FS_WEIGHT_BOLD))
        extra ++;
    width = (*sbc_devfont->font_ops->get_str_width) 
                    (pdc->pLogFont, sbc_devfont, 
                     (const unsigned char*)pText, word_info.len, extra);
#else
    gdi_get_TextOut_extent (pdc, pdc->pLogFont, pText, 
                            word_info.len, &size);
#endif

    *nChars = word_info.len;

	/* Fixed bug of italic font */
    return size.cx - gdi_get_italic_added_width(pdc->pLogFont);
}

/*
** This function return the normal characters' number (reference)
** and output width of the line (return value).
*/
static int txtGetOneLine (PDC pdc, const char* pText, int nCount, 
                int nTabWidth, 
                int maxwidth, UINT uFormat, int* nChar)
{
    int word_len, char_len;
    int word_width, char_width;
    int line_width;
    SIZE size;
	int italic_width = 0;

    *nChar = 0;

    if (uFormat & DT_SINGLELINE) {

        if (uFormat & DT_EXPANDTABS)
            gdi_get_TabbedTextOut_extent (pdc, pdc->pLogFont, nTabWidth, 
                            pText, nCount, &size, NULL);
        else
            gdi_get_TextOut_extent (pdc, pdc->pLogFont, pText, nCount, &size);

        *nChar = nCount;
        return size.cx;
    }

	italic_width = gdi_get_italic_added_width(pdc->pLogFont);
	/* Fixed bug of italic font */
	maxwidth -= italic_width;

    word_len = 0; word_width = 0;
    char_len = 0; char_width = 0;
    line_width = 0;
    while (TRUE) {
        if (uFormat & DT_CHARBREAK) {
            if (line_width > maxwidth) {
                *nChar -= char_len;
                if (*nChar <= 0) /* ensure to eat at least one char */
                    *nChar += char_len;
                break;
            }
            word_len = 0;
        }
        else if (uFormat & DT_WORDBREAK) {
            word_width = txtGetWidthOfNextWord (pdc, pText, nCount, 
                    &word_len);

            if (word_width > maxwidth) {
                word_len = GetTextExtentPoint ((HDC)pdc, pText, word_len, 
                            maxwidth - line_width, NULL, NULL, NULL, &size);
                word_width = size.cx;

                if (word_len == 0) { /* eat at least one char */
                    word_len = GetFirstMCharLen (GetCurFont ((HDC)pdc), 
                            pText, nCount);
                    gdi_get_TextOut_extent (pdc, pdc->pLogFont, pText, 
                            word_len, &size);

	                /* Fixed bug of italic font */
                    word_width = size.cx - italic_width;
                }
                *nChar += word_len;

	            /* Fixed bug of italic font */
                line_width += word_width + italic_width;
                break;
            }
            else if (line_width + word_width > maxwidth)
                break;
        }
        else {
            word_width = txtGetWidthOfNextWord (pdc, pText, nCount, &word_len);
        }

        if (word_len > 0) {
            pText += word_len;
            nCount -= word_len;

            *nChar += word_len;
            line_width += word_width;
        }

        if (nCount <= 0)
            break;

        char_len = 0;
        char_width = 0;
        if (*pText == '\t') {
            char_len = 1;

            if (uFormat & DT_EXPANDTABS) {
                char_width = nTabWidth;
                gdi_start_new_line (pdc);
            }
            else {
                gdi_get_TextOut_extent (pdc, pdc->pLogFont, pText, 1, &size);
	            /* Fixed bug of italic font */
                char_width = size.cx - italic_width;
            }
        }
        else if (*pText == '\n' || *pText == '\r') {
            (*nChar) ++;
            break;
        }
        else if (*pText == ' ') {
            char_len = 1;
            gdi_get_TextOut_extent (pdc, pdc->pLogFont, pText, 1, &size);
	        /* Fixed bug of italic font */
            char_width = size.cx - italic_width;
        }

        if ((word_len + char_len) == 0) { /* ensure to eat at least one char */
            char_len = GetFirstMCharLen (GetCurFont ((HDC)pdc), 
                            pText, nCount);
            gdi_get_TextOut_extent (pdc, pdc->pLogFont, pText, 
                            char_len, &size);

            char_width = size.cx - italic_width;
        }

        if (char_len > 0) {
            pText += char_len;
            nCount -= char_len;

            *nChar += char_len;
            line_width += char_width;
        }

        if (line_width > maxwidth)
            break;
    }

	/* Fixed bug of italic font */
    return line_width + italic_width;
}

int DrawTextEx2 (HDC hdc, const char* pText, int nCount, 
                RECT* pRect, int indent, UINT nFormat, DTFIRSTLINE *firstline)
{
    PDC pdc;
    int n, nLines = 0, width = 0;
    BOOL bOutput = TRUE;
    int x, y;
    RECT rcDraw, rcOutput;
    int nTabWidth; 
    int line_height;

    if (pText == NULL || nCount == 0 || pRect == NULL) return -1;

    if (RECTWP(pRect) == 0) return -1;
    if (RECTHP(pRect) == 0) return -1;

    if (nCount == 0) return -1;

    pdc = dc_HDC2PDC(hdc);
    if (is_utf16_logfont (pdc)) return -1;
    if (nCount < 0) nCount = __mg_strlen (pdc->pLogFont, pText);

    if (indent < 0) indent = 0;
    line_height = pdc->pLogFont->size + pdc->alExtra + pdc->blExtra;
    if (nFormat & DT_TABSTOP)
        nTabWidth = HIWORD (nFormat) * 
                    (*pdc->pLogFont->sbc_devfont->font_ops->get_ave_width)
                    (pdc->pLogFont, pdc->pLogFont->sbc_devfont);
    else {
        nTabWidth = pdc->tabstop * 
                    (*pdc->pLogFont->sbc_devfont->font_ops->get_ave_width)
                    (pdc->pLogFont, pdc->pLogFont->sbc_devfont);
    }

    /* Transfer logical to device to screen here. */
    rcDraw = *pRect;
    coor_LP2SP(pdc, &rcDraw.left, &rcDraw.top);
    coor_LP2SP(pdc, &rcDraw.right, &rcDraw.bottom);
    NormalizeRect (&rcDraw);

    /*If output rect is too small, we shouldn't output any text.*/
    if (RECTW(rcDraw) < pdc->pLogFont->size
        && RECTH(rcDraw) < pdc->pLogFont->size) {
        fprintf (stderr, 
            "Output rect is too small, we won't output any text. \n");
        return -1;
    }

    if (dc_IsGeneralDC (pdc)) {
        LOCK_GCRINFO (pdc);
        if (!dc_GenerateECRgn (pdc, FALSE))
            bOutput = FALSE;
    }

#ifdef _LITE_VERSION
    if (CHECK_DRAWING (pdc)) bOutput = FALSE;
#endif

    /* Draw text here. */
    if ((nFormat & DT_CALCRECT) || firstline)
        bOutput = FALSE;

    y = rcDraw.top;
    if (nFormat & DT_SINGLELINE) {
        if (nFormat & DT_BOTTOM)
            y = rcDraw.bottom - pdc->pLogFont->size;
        else if (nFormat & DT_VCENTER)
            y = rcDraw.top + ((RECTH (rcDraw) - pdc->pLogFont->size) >> 1);
    }

    memset (&rcOutput, 0, sizeof(RECT));

    while (nCount > 0) {
        int nOutput;
        int line_x, maxwidth;

        if (nLines == 0) {
            line_x = indent;
            maxwidth = rcDraw.right - rcDraw.left - indent;
            if (maxwidth <= 0) {
                y += line_height;
                nLines ++;
                continue;
            }
        }
        else {
            line_x = 0;
            maxwidth = rcDraw.right - rcDraw.left;
        }

        gdi_start_new_line (pdc);
        width = txtGetOneLine (pdc, pText, nCount, nTabWidth, 
                        maxwidth, nFormat, &n);

        if (n <= 0) {
            fprintf (stderr, "txtGetOneLine error of DrawTextEx2!\n");
            break;
        }

        nOutput = n;

        if ( (nFormat & DT_SINGLELINE) 
                        && (pText[n-1] == '\n' || pText[n-1] == '\r') ) {
            SIZE size;
            gdi_get_TextOut_extent(pdc, pdc->pLogFont, pText + n - 1, 1, &size);
            width += size.cx;
        }
 
        if (nFormat & DT_RIGHT)
            x = rcDraw.right - width;
        else if (nFormat & DT_CENTER)
            x = rcDraw.left + ((RECTW (rcDraw) - width) >> 1);
        else
            x = rcDraw.left;
        x += line_x;

        if (firstline) {
            firstline->nr_chars = nOutput;
            firstline->startx = x;
            firstline->starty = y;
            firstline->width = width;
            firstline->height = line_height;
            break;
        }

        rcOutput.left   = x;
        rcOutput.top    = y;
        rcOutput.right  = rcOutput.left + width;
        rcOutput.bottom = rcOutput.top + line_height;

        if (nFormat & DT_CALCRECT) {
            if (nLines == 0)
                *pRect = rcOutput;
            else
                GetBoundRect (pRect, pRect, &rcOutput);
        }

        /* draw one line */
        if (bOutput && width > 0) {
            if (nFormat & DT_NOCLIP)
                txtDrawOneLine (pdc, pText, nOutput, x, y, 
                        &rcOutput, nFormat, nTabWidth);
            else {
                RECT rcClip;
                if (IntersectRect (&rcClip, &rcOutput, &rcDraw))
                    txtDrawOneLine (pdc, pText, nOutput, x, y, 
                            &rcClip, nFormat, nTabWidth);
            }
        }

        pText += n;

        /* new line */
        y += line_height;
        nLines ++;
        /* left characters */
        nCount = nCount - n;
    }

    /* we are done, so release global clipping region */
    UNLOCK_GCRINFO (pdc);

    if (firstline) {
        coor_SP2LP (pdc, &firstline->startx, &firstline->starty);
        return 0;
    }

    if (nFormat & DT_CALCRECT) {
        coor_SP2LP (pdc, &pRect->left, &pRect->top);
        coor_SP2LP (pdc, &pRect->right, &pRect->bottom);
    }
    else {
        /* update text out position */
        x += width;
        y -= line_height;
        coor_SP2LP (pdc, &x, &y);
        pdc->CurTextPos.x = x;
        pdc->CurTextPos.y = y;
    }

    return line_height * nLines;
}

int GUIAPI GetTextExtentPoint (HDC hdc, const char* text, int len, 
                int max_extent, 
                int* fit_chars, int* pos_chars, int* dx_chars, SIZE* size)
{
    PDC pdc = dc_HDC2PDC (hdc);
    LOGFONT* log_font = pdc->pLogFont;
    DEVFONT* sbc_devfont = log_font->sbc_devfont;
    DEVFONT* mbc_devfont = log_font->mbc_devfont;
    int len_cur_char, width_cur_char;
    int left_bytes = len;
    int char_count = 0;
    int x = 0, y = 0;

    gdi_start_new_line(pdc);

    size->cy = log_font->size + pdc->alExtra + pdc->blExtra;
    size->cx = 0;
    while (left_bytes > 0) {
        if (pos_chars)
            pos_chars[char_count] = len - left_bytes;
        if (dx_chars)
            dx_chars[char_count] = size->cx;

        if (mbc_devfont && 
                (len_cur_char = (*mbc_devfont->charset_ops->len_first_char) 
                    ((const unsigned char*)text, left_bytes)) > 0) {
            width_cur_char = gdi_width_one_char (log_font, mbc_devfont,
                    text, len_cur_char, &x, &y);
        }
        else {
            if ((len_cur_char = (*sbc_devfont->charset_ops->len_first_char)
                    ((const unsigned char*)text, left_bytes)) > 0) {
                width_cur_char = gdi_width_one_char (log_font, sbc_devfont,
                    text, len_cur_char, &x, &y);
            }
            else
                break;
        }

        width_cur_char += pdc->cExtra;

        if (max_extent > 0 && (size->cx + width_cur_char) > max_extent) {
            goto ret;
        }

        char_count ++;
        size->cx += width_cur_char;
        left_bytes -= len_cur_char;
        text += len_cur_char;
    }

ret:
    if (fit_chars) *fit_chars = char_count;
    return len - left_bytes;
}

int GUIAPI GetTabbedTextExtentPoint (HDC hdc, const char* text, 
                int len, int max_extent, 
                int* fit_chars, int* pos_chars, int* dx_chars, SIZE* size)
{
    PDC pdc = dc_HDC2PDC (hdc);
    LOGFONT* log_font = pdc->pLogFont;
    DEVFONT* sbc_devfont = log_font->sbc_devfont;
    DEVFONT* mbc_devfont = log_font->mbc_devfont;
    DEVFONT* devfont;
    int left_bytes = len;
    int len_cur_char;
    int tab_width, line_height;
    int last_line_width = 0;
    int char_count = 0;
    int x = 0, y = 0;

    gdi_start_new_line (pdc);

    size->cx = 0;
    size->cy = 0;
    tab_width = (*sbc_devfont->font_ops->get_ave_width) (log_font, sbc_devfont)
                    * pdc->tabstop;
    line_height = log_font->size + pdc->alExtra + pdc->blExtra;

    while (left_bytes > 0) {
        if (pos_chars)
            pos_chars [char_count] = len - left_bytes;
        if (dx_chars)
            dx_chars [char_count] = last_line_width;

        devfont = NULL;
        len_cur_char = 0;

        if (mbc_devfont)
            len_cur_char = mbc_devfont->charset_ops->len_first_char 
                                ((const unsigned char*)text, left_bytes);

        if (len_cur_char > 0)
            devfont = mbc_devfont;
        else {
            len_cur_char = sbc_devfont->charset_ops->len_first_char 
                                ((const unsigned char*)text, left_bytes);
            if (len_cur_char > 0)
                devfont = sbc_devfont;
        }

        if (devfont) {
            int ch_type = devfont->charset_ops->char_type 
                                ((const unsigned char*)text);
            switch (ch_type) {
                case MCHAR_TYPE_ZEROWIDTH:
                    break;
                case MCHAR_TYPE_LF:
                    size->cy += line_height;
                case MCHAR_TYPE_CR:
                    if (last_line_width > size->cx)
                        size->cx = last_line_width;
                    last_line_width = 0;
                    gdi_start_new_line (pdc);
                    break;

                case MCHAR_TYPE_HT:
#if 0
                    last_line_width = (last_line_width/tab_width + 1)*tab_width;
#else
                    last_line_width += tab_width;
#endif
                    gdi_start_new_line (pdc);
                    break;

                default:
                    last_line_width += gdi_width_one_char (log_font, devfont, 
                            text, len_cur_char, &x, &y);
                    last_line_width += pdc->cExtra;
                    break;
            }
        }
        else
            break;

        if (max_extent > 0 && last_line_width > max_extent) {
            goto ret;
        }

        if (last_line_width > size->cx)
            size->cx = last_line_width;
        char_count ++;
        left_bytes -= len_cur_char;
        text += len_cur_char;
    }

ret:
    if (fit_chars) *fit_chars = char_count;
    return len - left_bytes;
}

static const char *strdot = "...";
#define STRDOT_LEN 3

static int str_omitted_3dot (char *dest, const char *src, 
                             int *pos_chars, int reserve)
{
    int nbytes = 0;
    if (reserve >= 1) {
        nbytes = *(pos_chars + reserve);
        memcpy (dest, src, nbytes);
    }
    strcpy ((dest + nbytes), strdot);
    return nbytes + 3;
}

int GUIAPI TextOutOmitted (HDC hdc, int x, int y, 
                           const char *mtext, int len, int max_extent)
{
    int mtextlen, fit_chars, *pos_chars, *dx_chars, nchar;
    int firstchar_len, firstchar_width;
    SIZE size_dot, size_text;
    char *strfixed, firstchar[8];
    PLOGFONT cur_font;

    cur_font = GetCurFont (hdc);

    /* get the whole string's width and compare with w 
       if w >= size.y, just dislay the string and return */
    mtextlen = (len < 0) ? __mg_strlen (cur_font, mtext) : len;
    GetTextExtent (hdc, mtext, mtextlen, &size_text);
    
    if (max_extent >= size_text.cx) {
        return TextOutLen (hdc, x, y, mtext, mtextlen);
    }

    /* Get the first word's width of the string */
    firstchar_len = GetFirstMCharLen (cur_font, mtext, mtextlen);
    memcpy (firstchar, mtext, firstchar_len);
    firstchar[firstchar_len] = '\0';
    GetTextExtent (hdc, firstchar, firstchar_len, &size_text);    
    firstchar_width = size_text.cx;
   
    GetTextExtent (hdc, strdot, STRDOT_LEN, &size_dot);
    
    /* the given width < first but it > '...' width, 
       just display ... */
    if (max_extent < (firstchar_width + size_dot.cx)) {
        if (max_extent > size_dot.cx) {
            return TextOutLen (hdc, x, y, strdot, STRDOT_LEN);
        } else {
            /* nothing can display */
            return 0;
        }
    }

    /* the given width >= first char width + ... 
       but < it's require width */                        
    if ((pos_chars = malloc (mtextlen * sizeof(int))) == NULL) {
        return 0;
    }
    if ((dx_chars = malloc (mtextlen * sizeof(int))) == NULL) {
        free (pos_chars);
        return 0;
    }
    
    nchar = GetTextExtentPoint (hdc, mtext, mtextlen, 
                max_extent - size_dot.cx, &fit_chars,
                pos_chars, dx_chars, &size_text);
    
    if ((strfixed = malloc (nchar + STRDOT_LEN + 1)) == NULL) {
         free (pos_chars);
         free (dx_chars);
         return 0;
    }

    /* append ... at the end of string */
    str_omitted_3dot (strfixed, mtext, pos_chars,fit_chars);
    nchar = TextOutLen (hdc, x, y, strfixed, nchar + STRDOT_LEN);
    
    free (strfixed);
    free (pos_chars);
    free (dx_chars);
    
    return nchar;
}

#undef STRDOT_LEN

