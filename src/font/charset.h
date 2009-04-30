/*
** $Id: charset.h 7330 2007-08-16 03:17:58Z xgwang $
**
** charset.h: the head file of charset operation set.
**
** Copyright (C) 2000 ~ 2002 Wei Yongming.
** Copyright (C) 2003 ~ 2007 Feynman Software.
*/

#ifndef GUI_FONT_CHARSET_H
    #define GUI_FONT_CHARSET_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

CHARSETOPS* GetCharsetOps (const char* charset);
CHARSETOPS* GetCharsetOpsEx (const char* charset);
BOOL IsCompatibleCharset (const char* charset, CHARSETOPS* ops);

#ifdef _GB_SUPPORT
extern unsigned short gbunicode_map [];
#endif

#ifdef _GBK_SUPPORT
extern unsigned short gbkunicode_map [];
#endif

#ifdef _GB18030_SUPPORT
extern unsigned short gb18030_0_unicode_map [];
#endif

#ifdef _BIG5_SUPPORT
extern unsigned short big5_unicode_map [];
#endif

#ifdef _EUCKR_SUPPORT
extern unsigned short ksc5601_0_unicode_map [];
#endif

#ifdef _EUCJP_SUPPORT
extern unsigned short jisx0208_0_unicode_map [];
#endif

#ifdef _SHIFTJIS_SUPPORT
extern unsigned short jisx0208_1_unicode_map [];
#endif

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif // GUI_FONT_CHARSET_H

