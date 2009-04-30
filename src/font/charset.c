/*
** $Id: charset.c 7330 2007-08-16 03:17:58Z xgwang $
** 
** charset.c: The charset operation set.
** 
** Copyright (C) 2003 ~ 2007 Feynman Software
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
#include <ctype.h>
#include <assert.h>

#include "common.h"
#include "minigui.h"
#include "gdi.h"
#include "charset.h"

/*************** Common Operations for single byte charsets ******************/
static int sb_len_first_char (const unsigned char* mstr, int len)
{
    if (len < 1) return 0;
    if (*mstr != '\0')  return 1;
    return 0;
}

static unsigned int sb_char_offset (const unsigned char* mchar)
{
    return (int)(*mchar);
}

static unsigned int sb_char_type (const unsigned char* mchar)
{
    unsigned int ch_type = MCHAR_TYPE_UNKNOWN;

    switch (*mchar) {
        case '\0':
            ch_type = MCHAR_TYPE_NUL;
            break;
        case '\a':
            ch_type = MCHAR_TYPE_BEL;
            break;
        case '\b':
            ch_type = MCHAR_TYPE_BS;
            break;
        case '\t':
            ch_type = MCHAR_TYPE_HT;
            break;
        case '\n':
            ch_type = MCHAR_TYPE_LF;
            break;
        case '\v':
            ch_type = MCHAR_TYPE_VT;
            break;
        case '\f':
            ch_type = MCHAR_TYPE_FF;
            break;
        case '\r':
            ch_type = MCHAR_TYPE_CR;
            break;
        case ' ':
            ch_type = MCHAR_TYPE_SPACE;
            break;
    }

    if (ch_type == MCHAR_TYPE_UNKNOWN) {
        if (*mchar < '\a')
            ch_type = MCHAR_TYPE_CTRL1;
        else if (*mchar < ' ')
            ch_type = MCHAR_TYPE_CTRL2;
        else
            ch_type = MCHAR_TYPE_GENERIC;
    }

    return ch_type;
}

static int sb_nr_chars_in_str (const unsigned char* mstr, int mstrlen)
{
    return mstrlen;
}

static int sb_len_first_substr (const unsigned char* mstr, int mstrlen)
{
    return mstrlen;
}

static int sb_pos_first_char (const unsigned char* mstr, int mstrlen)
{
    return 0;
}

static const unsigned char* sb_get_next_word (const unsigned char* mstr,
                int mstrlen, WORDINFO* word_info)
{
    int i;

    word_info->len = 0;
    word_info->delimiter = '\0';
    word_info->nr_delimiters = 0;

    if (mstrlen == 0) return NULL;

    for (i = 0; i < mstrlen; i++) {
        if (mstr[i] > 127) {
            if (word_info->len > 0) {
                word_info->delimiter = mstr[i];
                word_info->nr_delimiters ++;
            }
            else {
                word_info->len ++;
                word_info->delimiter = ' ';
                word_info->nr_delimiters ++;
            }
            break;
        }

        switch (mstr[i]) {
        case ' ':
        case '\t':
        case '\n':
        case '\r':
            if (word_info->delimiter == '\0') {
                word_info->delimiter = mstr[i];
                word_info->nr_delimiters ++;
            }
            else if (word_info->delimiter == mstr[i])
                word_info->nr_delimiters ++;
            else
                return mstr + word_info->len + word_info->nr_delimiters;
        break;

        default:
            if (word_info->delimiter != '\0')
                break;

            word_info->len ++;
        }
    }
            
    return mstr + word_info->len + word_info->nr_delimiters;
}

/************************* US-ASCII Specific Operations **********************/
static int ascii_is_this_charset (const unsigned char* charset)
{
    int i;
    char name [LEN_FONT_NAME + 1];

    for (i = 0; i < LEN_FONT_NAME + 1; i++) {
        if (charset [i] == '\0')
            break;
        name [i] = toupper (charset [i]);
    }
    name [i] = '\0';

    if (strstr (name, "ASCII"))
        return 0;

    return 1;
}

#ifdef _UNICODE_SUPPORT
static UChar32 ascii_conv_to_uc32 (const unsigned char* mchar)
{
    if (*mchar < 128)
        return (UChar32) (*mchar);
    else
        return '?';
}

static int ascii_conv_from_uc32 (UChar32 wc, unsigned char* mchar)
{
    if (wc < 128) {
        *mchar = (unsigned char) wc;
        return 1;
    }

    return 0;
}
#endif

static CHARSETOPS CharsetOps_ascii = {
    128,
    1,
    FONT_CHARSET_US_ASCII,
    {' '},
    sb_len_first_char,
    sb_char_offset,
    sb_char_type,
    sb_nr_chars_in_str,
    ascii_is_this_charset,
    sb_len_first_substr,
    sb_get_next_word,
    sb_pos_first_char,
#ifdef _UNICODE_SUPPORT
    ascii_conv_to_uc32,
    ascii_conv_from_uc32
#endif
};

/************************* ISO8859-1 Specific Operations **********************/
static int iso8859_1_is_this_charset (const unsigned char* charset)
{
    int i;
    char name [LEN_FONT_NAME + 1];
    char* sub_enc;

    for (i = 0; i < LEN_FONT_NAME + 1; i++) {
        if (charset [i] == '\0')
            break;
        name [i] = toupper (charset [i]);
    }
    name [i] = '\0';

    if (strstr (name, "ISO") && (sub_enc = strstr (name, "8859-1"))
            && (sub_enc [6] == '\0'))
        return 0;

    if ((sub_enc = strstr (name, "LATIN1")) && (sub_enc [6] == '\0'))
        return 0;

    return 1;
}

#ifdef _UNICODE_SUPPORT
static UChar32 iso8859_1_conv_to_uc32 (const unsigned char* mchar)
{
    return (UChar32) (*mchar);
}

static int iso8859_1_conv_from_uc32 (UChar32 wc, unsigned char* mchar)
{
    if (wc < 256) {
        *mchar = (unsigned char) wc;
        return 1;
    }

    return 0;
}
#endif

static CHARSETOPS CharsetOps_iso8859_1 = {
    256,
    1,
    FONT_CHARSET_ISO8859_1,
    {' '},
    sb_len_first_char,
    sb_char_offset,
    sb_char_type,
    sb_nr_chars_in_str,
    iso8859_1_is_this_charset,
    sb_len_first_substr,
    sb_get_next_word,
    sb_pos_first_char,
#ifdef _UNICODE_SUPPORT
    iso8859_1_conv_to_uc32,
    iso8859_1_conv_from_uc32
#endif
};

#ifdef _LATIN9_SUPPORT

/************************* ISO8859-15 Specific Operations **********************/
static int iso8859_15_is_this_charset (const unsigned char* charset)
{
    int i;
    char name [LEN_FONT_NAME + 1];

    for (i = 0; i < LEN_FONT_NAME + 1; i++) {
        if (charset [i] == '\0')
            break;
        name [i] = toupper (charset [i]);
    }
    name [i] = '\0';

    if (strstr (name, "ISO") && strstr (name, "8859-15"))
        return 0;

    if (strstr (name, "LATIN") && strstr (name, "9"))
        return 0;

    return 1;
}

#ifdef _UNICODE_SUPPORT
static UChar32 iso8859_15_conv_to_uc32 (const unsigned char* mchar)
{
    switch (*mchar) {
        case 0xA4:
            return 0x20AC;  /* EURO SIGN */
        case 0xA6:
            return 0x0160;  /* LATIN CAPITAL LETTER S WITH CARO */
        case 0xA8:
            return 0x0161;  /* LATIN SMALL LETTER S WITH CARON */
        case 0xB4:
            return 0x017D;  /* LATIN CAPITAL LETTER Z WITH CARON */
        case 0xB8:
            return 0x017E;  /* LATIN SMALL LETTER Z WITH CARON */
        case 0xBC:
            return 0x0152;  /* LATIN CAPITAL LIGATURE OE */
        case 0xBD:
            return 0x0153;  /* LATIN SMALL LIGATURE OE */
        case 0xBE:
            return 0x0178;  /* LATIN CAPITAL LETTER Y WITH DIAERESIS */
        default:
            return *mchar;
    }
}

static int iso8859_15_conv_from_uc32 (UChar32 wc, unsigned char* mchar)
{
    switch (wc) {
        case 0x20AC:  /* EURO SIGN */
            *mchar = 0xA4;
            return 1;
        case 0x0160:  /* LATIN CAPITAL LETTER S WITH CARO */
            *mchar = 0xA6;
            return 1;
        case 0x0161:  /* LATIN SMALL LETTER S WITH CARON */
            *mchar = 0xA8;
            return 1;
        case 0x017D:  /* LATIN CAPITAL LETTER Z WITH CARON */
            *mchar = 0xB4;
            return 1;
        case 0x017E:  /* LATIN SMALL LETTER Z WITH CARON */
            *mchar = 0xB8;
            return 1;
        case 0x0152:  /* LATIN CAPITAL LIGATURE OE */
            *mchar = 0xBC;
            return 1;
        case 0x0153:  /* LATIN SMALL LIGATURE OE */
            *mchar = 0xBD;
            return 1;
        case 0x0178:  /* LATIN CAPITAL LETTER Y WITH DIAERESIS */
            *mchar = 0xBE;
            return 1;
    }

    if (wc <= 0xFF) {
        *mchar = (unsigned char)wc;
        return 1;
    }

    return 0;
}
#endif

static CHARSETOPS CharsetOps_iso8859_15 = {
    256,
    1,
    FONT_CHARSET_ISO8859_15,
    {' '},
    sb_len_first_char,
    sb_char_offset,
    sb_char_type,
    sb_nr_chars_in_str,
    iso8859_15_is_this_charset,
    sb_len_first_substr,
    sb_get_next_word,
    sb_pos_first_char,
#ifdef _UNICODE_SUPPORT
    iso8859_15_conv_to_uc32,
    iso8859_15_conv_from_uc32
#endif
};
#endif /* _LATIN9_SUPPORT */

/*************** Common Operations for double bytes charsets ******************/
#if defined(_GB_SUPPORT) | defined(_GBK_SUPPORT) | defined(_BIG5_SUPPORT) \
        | defined(_EUCKR_SUPPORT) | defined(_EUCJP_SUPPORT) \
        | defined(_SHIFTJIS_SUPPORT) \
        | defined(_UNICODE_SUPPORT)

static unsigned int mb_char_type (const unsigned char* mchar)
{
    /* TODO: get the subtype of the char */
    return MCHAR_TYPE_GENERIC;
}

static int db_nr_chars_in_str (const unsigned char* mstr, int mstrlen)
{
    assert ((mstrlen % 2) == 0);
    return mstrlen >> 1;
}

static const unsigned char* db_get_next_word (const unsigned char* mstr,
                int mstrlen, WORDINFO* word_info)
{
    assert ((mstrlen % 2) == 0);

    word_info->len = 0;
    word_info->delimiter = '\0';
    word_info->nr_delimiters = 0;

    if (mstrlen < 2) return NULL;

    word_info->len = 2;

    return mstr + 2;
}

#endif

#ifdef _GB_SUPPORT
/************************* GB2312 Specific Operations ************************/
#define IS_GB2312_CHAR(ch1, ch2) \
        if (((ch1 >= 0xA1 && ch1 <= 0xA9) || (ch1 >= 0xB0 && ch1 <= 0xF7)) \
                        && ch2 >= 0xA1 && ch2 <= 0xFE)

static int gb2312_0_len_first_char (const unsigned char* mstr, int len)
{
    unsigned char ch1;
    unsigned char ch2;

    if (len < 2) return 0;

    ch1 = mstr [0];
    if (ch1 == '\0')
        return 0;

    ch2 = mstr [1];
    IS_GB2312_CHAR (ch1, ch2)
        return 2;

    return 0;
}

static unsigned int gb2312_0_char_offset (const unsigned char* mchar)
{
    int area = mchar [0] - 0xA1;

    if (area < 9) {
        return (area * 94 + mchar [1] - 0xA1);
    }
    else if (area >= 15)
        return ((area - 6)* 94 + mchar [1] - 0xA1);

    return 0;
}

static int gb2312_0_is_this_charset (const unsigned char* charset)
{
    int i;
    char name [LEN_FONT_NAME + 1];

    for (i = 0; i < LEN_FONT_NAME + 1; i++) {
        if (charset [i] == '\0')
            break;
        name [i] = toupper (charset [i]);
    }
    name [i] = '\0';

    if (strstr (name, "GB") && strstr (name, "2312"))
        return 0;

    return 1;
}

static int gb2312_0_len_first_substr (const unsigned char* mstr, int mstrlen)
{
    unsigned char ch1;
    unsigned char ch2;
    int i, left;
    int sub_len = 0;

    left = mstrlen;
    for (i = 0; i < mstrlen; i += 2) {
        if (left < 2) return sub_len;

        ch1 = mstr [i];
        if (ch1 == '\0') return sub_len;

        ch2 = mstr [i + 1];
        IS_GB2312_CHAR (ch1, ch2)
            sub_len += 2;
        else
            return sub_len;

        left -= 2;
    }

    return sub_len;
}

static int gb2312_0_pos_first_char (const unsigned char* mstr, int mstrlen)
{
    unsigned char ch1;
    unsigned char ch2;
    int i, left;

    i = 0;
    left = mstrlen;
    while (left) {
        if (left < 2) return -1;

        ch1 = mstr [i];
        if (ch1 == '\0') return -1;

        ch2 = mstr [i + 1];
        IS_GB2312_CHAR (ch1, ch2)
            return i;

        i += 1;
        left -= 1;
    }

    return -1;
}

#ifdef _UNICODE_SUPPORT
static UChar32 gb2312_0_conv_to_uc32 (const unsigned char* mchar)
{
    return (UChar32)gbunicode_map [gb2312_0_char_offset (mchar)];
}

const unsigned char* __mg_map_uc16_to_gb (unsigned short uc16);

static int gb2312_0_conv_from_uc32 (UChar32 wc, unsigned char* mchar)
{
    const unsigned char* got;

    if (wc > 0xFFFF)
        return 0;

    got = __mg_map_uc16_to_gb ((unsigned short)wc);

    if (got) {
        mchar [0] = got [0];
        mchar [1] = got [1];
        return 2;
    }

    return 0;
}
#endif

static CHARSETOPS CharsetOps_gb2312_0 = {
    (0xFF-0xA1)*(72 + 9),
    2,
    FONT_CHARSET_GB2312_0,
    {'\xA1', '\xA1'},
    gb2312_0_len_first_char,
    gb2312_0_char_offset,
    mb_char_type,
    db_nr_chars_in_str,
    gb2312_0_is_this_charset,
    gb2312_0_len_first_substr,
    db_get_next_word,
    gb2312_0_pos_first_char,
#ifdef _UNICODE_SUPPORT
    gb2312_0_conv_to_uc32,
    gb2312_0_conv_from_uc32
#endif
};
#endif /* _GB_SUPPORT */

#ifdef _BIG5_SUPPORT

/************************** BIG5 Specific Operations ************************/
static int big5_len_first_char (const unsigned char* mstr, int len)
{
    unsigned char ch1;
    unsigned char ch2;

    if (len < 2) return 0;

    ch1 = mstr [0];
    if (ch1 == '\0')
        return 0;

    ch2 = mstr [1];
    if (ch1 >= 0xA1 && ch1 <= 0xFE && 
            ((ch2 >=0x40 && ch2 <= 0x7E) || (ch2 >= 0xA1 && ch2 <= 0xFE)))
        return 2;

    return 0;
}

static unsigned int big5_char_offset (const unsigned char* mchar)
{
    if (mchar [1] & 0x80)
        return (mchar [0] - 0xA1) * 94 + mchar [1] - 0xA1;
    else
        return 94 * 94 + (mchar [0] - 0xa1) * 63 + (mchar [1] - 0x40);
}

static int big5_is_this_charset (const unsigned char* charset)
{
    int i;
    char name [LEN_FONT_NAME + 1];

    for (i = 0; i < LEN_FONT_NAME + 1; i++) {
        if (charset [i] == '\0')
            break;
        name [i] = toupper (charset [i]);
    }
    name [i] = '\0';

    if (strstr (name, "BIG5"))
        return 0;

    return 1;
}

static int big5_len_first_substr (const unsigned char* mstr, int mstrlen)
{
    unsigned char ch1;
    unsigned char ch2;
    int i, left;
    int sub_len = 0;

    left = mstrlen;
    for (i = 0; i < mstrlen; i += 2) {
        if (left < 2) return sub_len;

        ch1 = mstr [i];
        if (ch1 == '\0') return sub_len;

        ch2 = mstr [i + 1];
        if (ch1 >= 0xA1 && ch1 <= 0xFE && 
                ((ch2 >=0x40 && ch2 <= 0x7E) || (ch2 >= 0xA1 && ch2 <= 0xFE)))
            sub_len += 2;
        else
            return sub_len;

        left -= 2;
    }

    return sub_len;
}

static int big5_pos_first_char (const unsigned char* mstr, int mstrlen)
{
    unsigned char ch1;
    unsigned char ch2;
    int i, left;

    i = 0;
    left = mstrlen;
    while (left) {
        if (left < 2) return -1;

        ch1 = mstr [i];
        if (ch1 == '\0') return -1;

        ch2 = mstr [i + 1];
        if (ch1 >= 0xA1 && ch1 <= 0xFE && 
                ((ch2 >=0x40 && ch2 <= 0x7E) || (ch2 >= 0xA1 && ch2 <= 0xFE)))
            return i;

        i += 1;
        left -= 1;
    }

    return -1;
}

#ifdef _UNICODE_SUPPORT
static UChar32 big5_conv_to_uc32 (const unsigned char* mchar)
{
    unsigned short ucs_code = big5_unicode_map [big5_char_offset (mchar)];

    if (ucs_code == 0)
        return '?';

    return ucs_code;
}

const unsigned char* __mg_map_uc16_to_big5 (unsigned short uc16);
static int big5_conv_from_uc32 (UChar32 wc, unsigned char* mchar)
{
    const unsigned char* got;

    if (wc > 0xFFFF)
        return 0;

    got = __mg_map_uc16_to_big5 ((unsigned short)wc);

    if (got) {
        mchar [0] = got [0];
        mchar [1] = got [1];
        return 2;
    }

    return 0;
}
#endif

static CHARSETOPS CharsetOps_big5 = {
    14758,
    2,
    FONT_CHARSET_BIG5,
    {'\xA1', '\x40'},
    big5_len_first_char,
    big5_char_offset,
    mb_char_type,
    db_nr_chars_in_str,
    big5_is_this_charset,
    big5_len_first_substr,
    db_get_next_word,
    big5_pos_first_char,
#ifdef _UNICODE_SUPPORT
    big5_conv_to_uc32,
    big5_conv_from_uc32,
#endif
};

#endif /* _BIG5_SUPPORT */

#ifdef _UNICODE_SUPPORT

/************************* UTF-8 Specific Operations ************************/
static int utf8_len_first_char (const unsigned char* mstr, int len)
{
    int t, c = *((unsigned char *)(mstr++));
    int n = 1, ch_len = 0;

    /*for ascii character*/
    if (c < 0x80) {
        return 1;
    }

    if (c & 0x80) {
        while (c & (0x80 >> n))
            n++;

        if (n > len)
            return 0;

        ch_len = n;
        while (--n > 0) {
            t = *((unsigned char *)(mstr++));

            if ((!(t & 0x80)) || (t & 0x40))
                return 0;
        }
    }

    return ch_len;
}

static UChar32 utf8_conv_to_uc32 (const unsigned char* mchar)
{
    UChar32 wc = *((unsigned char *)(mchar++));
    int n, t;

    if (wc & 0x80) {
        n = 1;
        while (wc & (0x80 >> n))
            n++;

        wc &= (1 << (8-n)) - 1;
        while (--n > 0) {
            t = *((unsigned char *)(mchar++));

            wc = (wc << 6) | (t & 0x3F);
        }
    }

    return wc;
}

static unsigned int utf8_char_offset (const unsigned char* mchar)
{
    return utf8_conv_to_uc32 (mchar);
}

static unsigned int utf8_char_type (const unsigned char* mchar)
{
    unsigned int ch_type;

    if (mchar [0] < 0x80) {
        ch_type = sb_char_type (mchar);
    }
    else {
        /* TODO: get the subtype of the char */
        ch_type = MCHAR_TYPE_GENERIC;
    }

    return ch_type;
}

static int utf8_nr_chars_in_str (const unsigned char* mstr, int mstrlen)
{
    int charlen;
    int n = 0;
    int left = mstrlen;

    while (left >= 0) {
        charlen = utf8_len_first_char (mstr, left);
        if (charlen > 0)
            n ++;

        left -= charlen;
        mstr += charlen;
    }

    return n;
}

static int utf8_is_this_charset (const unsigned char* charset)
{
    int i;
    char name [LEN_FONT_NAME + 1];

    for (i = 0; i < LEN_FONT_NAME + 1; i++) {
        if (charset [i] == '\0')
            break;
        name [i] = toupper (charset [i]);
    }
    name [i] = '\0';

    if (strstr (name, "UTF") && strstr (name, "8"))
        return 0;

    return 1;
}

static int utf8_len_first_substr (const unsigned char* mstr, int mstrlen)
{
    int ch_len;
    int sub_len = 0;

    while (mstrlen > 0) {
        ch_len = utf8_len_first_char (mstr, mstrlen);

        if (ch_len == 0)
            break;

        sub_len += ch_len;
        mstrlen -= ch_len;
        mstr += ch_len;
    }

    return sub_len;
}

static const unsigned char* utf8_get_next_word (const unsigned char* mstr,
                int mstrlen, WORDINFO* word_info)
{
    const unsigned char* mchar = mstr;
    int ch_len;
    UChar32 wc;

    word_info->len = 0;
    word_info->delimiter = '\0';
    word_info->nr_delimiters = 0;

    if (mstrlen == 0) return NULL;

    while (mstrlen > 0) {
        ch_len = utf8_len_first_char (mchar, mstrlen);

        if (ch_len == 0)
            break;

        wc = utf8_conv_to_uc32 (mchar);
        if (wc > 0x0FFF) {
            if (word_info->len)
                return mstr + word_info->len + word_info->nr_delimiters;
            else { /* Treate the first char as a word */
                word_info->len = ch_len;
                return mstr + word_info->len + word_info->nr_delimiters;
            }
        }

        switch (mchar[0]) {
        case ' ':
        case '\t':
        case '\n':
        case '\r':
            if (word_info->delimiter == '\0') {
                word_info->delimiter = mchar[0];
                word_info->nr_delimiters ++;
            }
            else if (word_info->delimiter == mchar[0])
                word_info->nr_delimiters ++;
            else
                return mstr + word_info->len + word_info->nr_delimiters;

            break;

        default:
            if (word_info->delimiter != '\0')
                return mstr + word_info->len + word_info->nr_delimiters;

            word_info->len += ch_len;
            break;
        }

        mchar += ch_len;
        mstrlen -= ch_len;
    }

    return mstr + word_info->len + word_info->nr_delimiters;
}

static int utf8_pos_first_char (const unsigned char* mstr, int mstrlen)
{
    int ch_len;
    int pos = 0;

    while (mstrlen > 0) {
        ch_len = utf8_len_first_char (mstr, mstrlen);

        /*charset encoding mismatch*/
        if (ch_len == 0)
            return -1;

        if (ch_len > 0)
            return pos;

        pos += ch_len;
        mstrlen -= ch_len;
        mstr += ch_len;
    }

    return -1;
}

static int utf8_conv_from_uc32 (UChar32 wc, unsigned char* mchar)
{
    int first, len;

    if (wc < 0x80) {
        first = 0;
        len = 1;
    }
    else if (wc < 0x800) {
        first = 0xC0;
        len = 2;
    }
    else if (wc < 0x10000) {
        first = 0xE0;
        len = 3;
    }
    else if (wc < 0x200000) {
        first = 0xF0;
        len = 4;
    }
    else if (wc < 0x400000) {
        first = 0xF8;
        len = 5;
    }
    else {
        first = 0xFC;
        len = 6;
    }

    switch (len) {
        case 6:
            mchar [5] = (wc & 0x3f) | 0x80; wc >>= 6; /* Fall through */
        case 5:
            mchar [4] = (wc & 0x3f) | 0x80; wc >>= 6; /* Fall through */
        case 4:
            mchar [3] = (wc & 0x3f) | 0x80; wc >>= 6; /* Fall through */
        case 3:
            mchar [2] = (wc & 0x3f) | 0x80; wc >>= 6; /* Fall through */
        case 2:
            mchar [1] = (wc & 0x3f) | 0x80; wc >>= 6; /* Fall through */
        case 1:
            mchar [0] = wc | first;
    }

    return len;
}

static CHARSETOPS CharsetOps_utf8 = {
    0x7FFFFFFF,
    6,
    FONT_CHARSET_UTF8,
    {' '},
    utf8_len_first_char,
    utf8_char_offset,
    utf8_char_type,
    utf8_nr_chars_in_str,
    utf8_is_this_charset,
    utf8_len_first_substr,
    utf8_get_next_word,
    utf8_pos_first_char,
    utf8_conv_to_uc32,
    utf8_conv_from_uc32,
};

/************************* UTF-16LE Specific Operations ***********************/
static int utf16le_len_first_char (const unsigned char* mstr, int len)
{
    UChar16 w1, w2;

    if (len < 2)
        return 0;

    w1 = MAKEWORD (mstr[0], mstr[1]);

    if (w1 < 0xD800 || w1 > 0xDFFF)
        return 2;

    if (w1 >= 0xD800 && w1 <= 0xDBFF) {
        if (len < 4)
            return 0;
        w2 = MAKEWORD (mstr[2], mstr[3]);
        if (w2 < 0xDC00 || w2 > 0xDFFF)
            return 0;
    }

    return 4;
}

static UChar32 utf16le_conv_to_uc32 (const unsigned char* mchar)
{
    UChar16 w1, w2;
    UChar32 wc;

    w1 = MAKEWORD (mchar[0], mchar[1]);

    if (w1 < 0xD800 || w1 > 0xDFFF)
        return w1;

    w2 = MAKEWORD (mchar[2], mchar[3]);

    wc = w1;
    wc <<= 10;
    wc |= (w2 & 0x03FF);
    wc += 0x10000;

    return wc;
}

static unsigned int utf16le_char_offset (const unsigned char* mchar)
{
    return utf16le_conv_to_uc32 (mchar);
}

static unsigned int utf16le_char_type (const unsigned char* mchar)
{
    unsigned int ch_type;
    unsigned char c1 = mchar [0], c2 = mchar [1];

    if (c1 == 0xFE && c2 == 0xFF) {
        ch_type = MCHAR_TYPE_ZEROWIDTH;
    }
    else if (c2 == 0) {
        ch_type = sb_char_type (&c1);
    }
    else {
        /* TODO: get the subtype of the char */
        ch_type = MCHAR_TYPE_GENERIC;
    }

    return ch_type;
}

static int utf16le_nr_chars_in_str (const unsigned char* mstr, int mstrlen)
{
    int charlen;
    int n = 0;
    int left = mstrlen;

    while (left >= 0) {
        charlen = utf16le_len_first_char (mstr, left);
        if (charlen > 0)
            n ++;

        left -= charlen;
        mstr += charlen;
    }

    return n;
}

static int utf16le_is_this_charset (const unsigned char* charset)
{
    int i;
    char name [LEN_FONT_NAME + 1];

    for (i = 0; i < LEN_FONT_NAME + 1; i++) {
        if (charset [i] == '\0')
            break;
        name [i] = toupper (charset [i]);
    }
    name [i] = '\0';

    if (strstr (name, "UTF") && strstr (name, "16LE"))
        return 0;

    return 1;
}

static int utf16le_len_first_substr (const unsigned char* mstr, int mstrlen)
{
    int ch_len;
    int sub_len = 0;

    while (mstrlen > 0) {
        ch_len = utf16le_len_first_char (mstr, mstrlen);

        if (ch_len == 0)
            break;

        sub_len += ch_len;
        mstrlen -= ch_len;
        mstr += ch_len;
    }

    return sub_len;
}

static const unsigned char* utf16le_get_next_word (const unsigned char* mstr,
                int mstrlen, WORDINFO* word_info)
{
    const unsigned char* mchar = mstr;
    UChar32 wc;
    int ch_len;

    word_info->len = 0;
    word_info->delimiter = '\0';
    word_info->nr_delimiters = 0;

    if (mstrlen == 0) return NULL;

    while (mstrlen > 0) {
        ch_len = utf16le_len_first_char (mchar, mstrlen);

        if (ch_len == 0)
            break;

        wc = utf16le_conv_to_uc32 (mchar);
        if (wc > 0x0FFF) {
            if (word_info->len)
                return mstr + word_info->len + word_info->nr_delimiters;
            else { /* Treate the first char as a word */
                word_info->len = ch_len;
                return mstr + word_info->len + word_info->nr_delimiters;
            }
        }

        switch (wc) {
        case ' ':
        case '\t':
        case '\n':
        case '\r':
            if (word_info->delimiter == '\0') {
                word_info->delimiter = (unsigned char)wc;
                word_info->nr_delimiters += ch_len;
            }
            else if (word_info->delimiter == (unsigned char)wc)
                word_info->nr_delimiters += ch_len;
            else
                return mstr + word_info->len + word_info->nr_delimiters;
            break;

        default:
            if (word_info->delimiter != '\0')
                return mstr + word_info->len + word_info->nr_delimiters;

            word_info->len += ch_len;
            break;
        }

        mstrlen -= ch_len;
        mchar += ch_len;
    }

    return mstr + word_info->len + word_info->nr_delimiters;
}

static int utf16le_pos_first_char (const unsigned char* mstr, int mstrlen)
{
    int ch_len;
    int pos = 0;

    while (mstrlen > 0) {
        ch_len = utf16le_len_first_char (mstr, mstrlen);

        if (ch_len > 0)
            return pos;

        pos += ch_len;
        mstrlen -= ch_len;
        mstr += ch_len;
    }

    return -1;
}

static int utf16le_conv_from_uc32 (UChar32 wc, unsigned char* mchar)
{
    UChar16 w1, w2;

    if (wc > 0x10FFFF) {
        return 0;
    }

    if (wc < 0x10000) {
        mchar [0] = LOBYTE (wc);
        mchar [1] = HIBYTE (wc);
        return 2;
    }

    wc -= 0x10000;
    w1 = 0xD800;
    w2 = 0xDC00;

    w1 |= (wc >> 10);
    w2 |= (wc & 0x03FF);

    mchar [0] = LOBYTE (w1);
    mchar [1] = HIBYTE (w1);
    mchar [2] = LOBYTE (w2);
    mchar [3] = HIBYTE (w2);
    return 4;
}

static CHARSETOPS CharsetOps_utf16le = {
    0x7FFFFFFF,
    4,
    FONT_CHARSET_UTF16LE,
    {'\xA1', '\xA1'},
    utf16le_len_first_char,
    utf16le_char_offset,
    utf16le_char_type,
    utf16le_nr_chars_in_str,
    utf16le_is_this_charset,
    utf16le_len_first_substr,
    utf16le_get_next_word,
    utf16le_pos_first_char,
    utf16le_conv_to_uc32,
    utf16le_conv_from_uc32
};

/************************* UTF-16BE Specific Operations ***********************/
static int utf16be_len_first_char (const unsigned char* mstr, int len)
{
    UChar16 w1, w2;

    if (len < 2)
        return 0;

    w1 = MAKEWORD (mstr[1], mstr[0]);

    if (w1 < 0xD800 || w1 > 0xDFFF)
        return 2;

    if (w1 >= 0xD800 && w1 <= 0xDBFF) {
        if (len < 4)
            return 0;
        w2 = MAKEWORD (mstr[3], mstr[2]);
        if (w2 < 0xDC00 || w2 > 0xDFFF)
            return 0;
    }

    return 4;
}

static UChar32 utf16be_conv_to_uc32 (const unsigned char* mchar)
{
    UChar16 w1, w2;
    UChar32 wc;

    w1 = MAKEWORD (mchar[1], mchar[0]);

    if (w1 < 0xD800 || w1 > 0xDFFF)
        return w1;

    w2 = MAKEWORD (mchar[3], mchar[2]);

    wc = w1;
    wc <<= 10;
    wc |= (w2 & 0x03FF);
    wc += 0x10000;

    return wc;
}

static unsigned int utf16be_char_offset (const unsigned char* mchar)
{
    return utf16be_conv_to_uc32 (mchar);
}

static unsigned int utf16be_char_type (const unsigned char* mchar)
{
    unsigned int ch_type;
    unsigned char c1 = mchar [1], c2 = mchar [0];

    if (c1 == 0xFE && c2 == 0xFF) {
        ch_type = MCHAR_TYPE_ZEROWIDTH;
    }
    else if (c2 == 0) {
        ch_type = sb_char_type (&c1);
    }
    else {
        /* TODO: get the subtype of the char */
        ch_type = MCHAR_TYPE_GENERIC;
    }

    return ch_type;
}

static int utf16be_nr_chars_in_str (const unsigned char* mstr, int mstrlen)
{
    int charlen;
    int n = 0;
    int left = mstrlen;

    while (left >= 0) {
        charlen = utf16be_len_first_char (mstr, left);
        if (charlen > 0)
            n ++;

        left -= charlen;
        mstr += charlen;
    }

    return n;
}

static int utf16be_is_this_charset (const unsigned char* charset)
{
    int i;
    char name [LEN_FONT_NAME + 1];

    for (i = 0; i < LEN_FONT_NAME + 1; i++) {
        if (charset [i] == '\0')
            break;
        name [i] = toupper (charset [i]);
    }
    name [i] = '\0';

    if (strstr (name, "UTF") && strstr (name, "16BE"))
        return 0;

    return 1;
}

static int utf16be_len_first_substr (const unsigned char* mstr, int mstrlen)
{
    int ch_len;
    int sub_len = 0;

    while (mstrlen > 0) {
        ch_len = utf16be_len_first_char (mstr, mstrlen);

        if (ch_len == 0)
            break;

        sub_len += ch_len;
        mstrlen -= ch_len;
        mstr += ch_len;
    }

    return sub_len;
}

static const unsigned char* utf16be_get_next_word (const unsigned char* mstr,
                int mstrlen, WORDINFO* word_info)
{
    const unsigned char* mchar = mstr;
    UChar32 wc;
    int ch_len;

    word_info->len = 0;
    word_info->delimiter = '\0';
    word_info->nr_delimiters = 0;

    if (mstrlen == 0) return NULL;

    while (mstrlen > 0) {
        ch_len = utf16be_len_first_char (mchar, mstrlen);

        if (ch_len == 0)
            break;

        wc = utf16be_conv_to_uc32 (mchar);
        if (wc > 0x0FFF) {
            if (word_info->len)
                return mstr + word_info->len + word_info->nr_delimiters;
            else { /* Treate the first char as a word */
                word_info->len = ch_len;
                return mstr + word_info->len + word_info->nr_delimiters;
            }
        }

        switch (wc) {
        case ' ':
        case '\t':
        case '\n':
        case '\r':
            if (word_info->delimiter == '\0') {
                word_info->delimiter = (unsigned char)wc;
                word_info->nr_delimiters += ch_len;
            }
            else if (word_info->delimiter == (unsigned char)wc)
                word_info->nr_delimiters += ch_len;
            else
                return mstr + word_info->len + word_info->nr_delimiters;
            break;

        default:
            if (word_info->delimiter != '\0')
                return mstr + word_info->len + word_info->nr_delimiters;

            word_info->len += ch_len;
            break;
        }

        mstrlen -= ch_len;
        mchar += ch_len;
    }

    return mstr + word_info->len + word_info->nr_delimiters;
}

static int utf16be_pos_first_char (const unsigned char* mstr, int mstrlen)
{
    int ch_len;
    int pos = 0;

    while (mstrlen > 0) {
        ch_len = utf16be_len_first_char (mstr, mstrlen);

        if (ch_len > 0)
            return pos;

        pos += ch_len;
        mstrlen -= ch_len;
        mstr += ch_len;
    }

    return -1;
}

static int utf16be_conv_from_uc32 (UChar32 wc, unsigned char* mchar)
{
    UChar16 w1, w2;

    if (wc > 0x10FFFF) {
        return 0;
    }

    if (wc < 0x10000) {
        mchar [1] = LOBYTE (wc);
        mchar [0] = HIBYTE (wc);
        return 2;
    }

    wc -= 0x10000;
    w1 = 0xD800;
    w2 = 0xDC00;

    w1 |= (wc >> 10);
    w2 |= (wc & 0x03FF);

    mchar [1] = LOBYTE (w1);
    mchar [0] = HIBYTE (w1);
    mchar [3] = LOBYTE (w2);
    mchar [2] = HIBYTE (w2);
    return 4;
}

static CHARSETOPS CharsetOps_utf16be = {
    0x7FFFFFFF,
    4,
    FONT_CHARSET_UTF16BE,
    {'\xA1', '\xA1'},
    utf16be_len_first_char,
    utf16be_char_offset,
    utf16be_char_type,
    utf16be_nr_chars_in_str,
    utf16be_is_this_charset,
    utf16be_len_first_substr,
    utf16be_get_next_word,
    utf16be_pos_first_char,
    utf16be_conv_to_uc32,
    utf16be_conv_from_uc32
};

/************************* End of UNICODE *************************************/
#endif /* _UNICODE_SUPPORT */

static CHARSETOPS* Charsets [] =
{
    &CharsetOps_ascii,
#ifdef _LATIN9_SUPPORT
    &CharsetOps_iso8859_15,
#endif
    &CharsetOps_iso8859_1,
#ifdef _GB_SUPPORT
    &CharsetOps_gb2312_0,
#endif
#ifdef _BIG5_SUPPORT
    &CharsetOps_big5,
#endif
#ifdef _UNICODE_SUPPORT
    &CharsetOps_utf8,
    &CharsetOps_utf16le,
    &CharsetOps_utf16be,
#endif
};

#define NR_CHARSETS     (sizeof(Charsets)/sizeof(CHARSETOPS*))

CHARSETOPS* GetCharsetOps (const char* charset)
{
    int i;

    for (i = 0; i < NR_CHARSETS; i++) {
        if ((*Charsets [i]->is_this_charset) 
                        ((const unsigned char*)charset) == 0)
            return Charsets [i];
    }

    return NULL;
}

CHARSETOPS* GetCharsetOpsEx (const char* charset)
{
    int i;
    char name [LEN_FONT_NAME + 1];

    for (i = 0; i < LEN_FONT_NAME + 1; i++) {
        if (charset [i] == '\0')
            break;
        name [i] = toupper (charset [i]);
    }
    name [i] = '\0';

    for (i = 0; i < NR_CHARSETS; i++) {
        if (strcmp (Charsets [i]->name, name) == 0)
            return Charsets [i];
    }

    return NULL;
}

BOOL IsCompatibleCharset (const char* charset, CHARSETOPS* ops)
{
    int i;

    for (i = 0; i < NR_CHARSETS; i++) {
        if ((*Charsets [i]->is_this_charset) 
                        ((const unsigned char*)charset) == 0)
            if (Charsets [i] == ops)
                return TRUE;
    }

    return FALSE;
}

