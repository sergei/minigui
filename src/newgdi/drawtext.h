/*
** $Id: drawtext.h 7359 2007-08-16 05:08:40Z xgwang $
**
** drawtext.h: Low Level text out routines.
**
** Copyright (C) 2003 ~ 2007 Feynman Software
** Copyright (C) 2000 ~ 2002 Wei Yongming.
*/

#ifndef GUI_GDI_DRAWTEXT_H
    #define GUI_GDI_DRAWTEXT_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

int gdi_strnwrite (PDC pdc, int x, int y, const char* text, int len);
int gdi_tabbedtextout (PDC pdc, int x, int y, const char* text, int len);
int gdi_width_one_char (LOGFONT* logfont, DEVFONT* devfont, 
                const char* mchar, int len, int* x, int* y);
void gdi_get_TextOut_extent (PDC pdc, LOGFONT* log_font, const char* text, int len, SIZE* size);
void gdi_get_TabbedTextOut_extent (PDC pdc, LOGFONT* log_font, int tab_stops,
                const char* text, int len, SIZE* size, POINT* last_pos);
void gdi_start_new_line (PDC pdc);
void gdi_draw_tabbed_text_oneline_back(PDC pdc, int x, int y, const char* text, int len);
int gdi_get_italic_added_width(LOGFONT* logfont);

BOOL InitTextBitmapBuffer (void);
void TermTextBitmapBuffer (void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif // GUI_GDI_DRAWTEXT_H

