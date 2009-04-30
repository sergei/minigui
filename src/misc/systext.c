/*
** $Id: systext.c 8013 2007-10-30 05:11:41Z xwyan $
**
** systext.c: GetSysText function.
**
** Copyright (C) 2003 ~ 2007 Feynman Software.
** Copyright (C) 1999 ~ 2002 Wei Yongming.
**
** All rights reserved by Feynman Software.
** 
**
** Current maintainer: Wei Yongming.
**
** Create date: 2000/12/31
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "minigui.h"
#include "gdi.h"
#include "window.h"

/*
 * This function translates system strings.
 * You can use gettext to return the text.
 *
 * System text as follows:
 *
 const char* SysText [] =
 {
    "Windows",                  // 0
    "Start",                    // 1
    "Refresh Background",       // 2
    "Close All Windows",        // 3
    "End Session",              // 4
    "Operations",               // 5
    "Minimize",                 // 6
    "Maximize",                 // 7
    "Restore",                  // 8
    "Close",                    // 9
    "OK",                       // 10
    "Next",                     // 11
    "Cancel",                   // 12
    "Previous",                 // 13
    "Yes",                      // 14
    "No",                       // 15
    "Abort",                    // 16
    "Retry",                    // 17
    "Ignore",                   // 18
    "About MiniGUI...",         // 19
    "Open File",                // 20
    "Save File",                // 21
    "Color Selection",          // 22
    "Switch Layer",             // 23
    "Delete Layer",             // 24
    "Error",                     // 25
    "LOGO",                      // 26 
    "Current Path",              // 27 
    "File",                      // 28 
#if (!defined (__NOUNIX__) || defined (WIN32)) && defined (_EXT_CTRL_LISTVIEW)
    "Location",                 //0 + 29
    "Up",                       //1 + 29
    "Name",                     //2 + 29
    "Size",                     //3 + 29
    "Access Mode",              //4 + 29
    "Last Modify Time",         //5 + 29
    "Open",                     //6 + 29
    "File Name",                //7 + 29
    "File Type",                //8 + 29
    "Show Hide File",           //9 + 29
    "Sorry! not find %s ", //10 + 29
    "Can't Read %s !",        //11 + 29
    "Can't Write  %s !",      //12 + 29
    "Information",              //13 + 29
    "R",                        //14 + 29
    "W",                        //15 + 29
    "WR",                       //16 + 29
    "Save",                     //17 + 29
    "File %s exists, Replace or not?", //18 + 29
#endif
    NULL
};
*/

const char** local_SysText;

const char* SysText [] =
{
    "Windows",                  // 0
    "Start",                    // 1
    "Refresh Background",       // 2
    "Close All Windows",        // 3
    "End Session",              // 4
    "Operations",               // 5
    "Minimize",                 // 6
    "Maximize",                 // 7
    "Restore",                  // 8
    "Close",                    // 9
    "OK",                       // 10
    "Next",                     // 11
    "Cancel",                   // 12
    "Previous",                 // 13
    "Yes",                      // 14
    "No",                       // 15
    "Abort",                    // 16
    "Retry",                    // 17
    "Ignore",                   // 18
    "About MiniGUI...",         // 19
    "Open File",                // 20
    "Save File",                // 21
    "Color Selection",          // 22
    "Switch Layer",             // 23
    "Delete Layer",             // 24
    "Error",                     // 25
    "LOGO",                      // 26 
    "Current Path",              // 27 
    "File",                      // 28 
#if (!defined (__NOUNIX__) || defined (WIN32)) && defined (_EXT_CTRL_LISTVIEW)
    "Location",                 //0 + 29
    "Up",                       //1 + 29
    "Name",                     //2 + 29
    "Size",                     //3 + 29
    "Access Mode",              //4 + 29
    "Last Modify Time",         //5 + 29
    "Open",                     //6 + 29
    "File Name",                //7 + 29
    "File Type",                //8 + 29
    "Show Hide File",           //9 + 29
    "Sorry! not find %s ", //10 + 29
    "Can't Read %s !",        //11 + 29
    "Can't Write  %s !",      //12 + 29
    "Information",              //13 + 29
    "R",                        //14 + 29
    "W",                        //15 + 29
    "WR",                       //16 + 29
    "Save",                     //17 + 29
    "File %s exists, Replace or not?", //18 + 29
#endif
    NULL
};

#if defined(_GB_SUPPORT) | defined (_GBK_SUPPORT) | defined (_GB18030_SUPPORT)
static const char* SysText_GB [] =
{
    "´°¿Ú",              // 0
    "¿ªÊ¼",              // 1
    "Ë¢ÐÂ±³¾°",             // 2
    "¹Ø±ÕËùÓÐ´°¿Ú",         // 3
    "½áÊø»á»°",             // 4
    "´°¿Ú²Ù×÷",             // 5
    "×îÐ¡»¯",               // 6
    "×î´ó»¯",               // 7
    "»Ö¸´",                 // 8
    "¹Ø±Õ",                 // 9
    "È·¶¨",                 // 10
    "ÏÂÒ»²½",               // 11
    "È¡Ïû",                 // 12
    "ÉÏÒ»²½",               // 13
    "ÊÇ(Y)",                // 14
    "·ñ(N)",                // 15
    "ÖÕÖ¹(A)",              // 16
    "ÖØÊÔ(R)",              // 17
    "ºöÂÔ(I)",              // 18
    "¹ØÓÚ MiniGUI...",      // 19
    "´ò¿ªÎÄ¼þ",             // 20
    "±£´æÎÄ¼þ",             // 21
    "ÑÕÉ«Ñ¡Ôñ",             // 22
    "ÇÐ»»²ã",               // 23
    "É¾³ý²ã",               // 24
    "´íÎó",                 // 25
    "Í¼±ê",                 // 26
    "µ±Ç°Â·¾¶",             // 27
    "ÎÄ¼þ",                 // 28
#if (!defined (__NOUNIX__) || defined (WIN32)) && defined (_EXT_CTRL_LISTVIEW)
    "²éÕÒÎ»ÓÚ",                                 //0 + 29
    "ÉÏÒ»¼¶",                                   //1 + 29
    "Ãû³Æ",                                     //2 + 29
    "´óÐ¡",                                     //3 + 29
    "·ÃÎÊÈ¨ÏÞ",                                 //4 + 29
    "ÉÏ´ÎÐÞ¸ÄÊ±¼ä",                             //5 + 29
    "´ò¿ª",                                     //6 + 29
    "ÎÄ ¼þ Ãû",                                 //7 + 29
    "ÎÄ¼þÀàÐÍ",                                 //8 + 29
    "ÏÔÊ¾Òþ²ØÎÄ¼þ",                             //9 + 29
    "¶Ô²»Æð£¬Î´ÕÒµ½Ö¸¶¨µÄÄ¿Â¼£º %s ",     //10 + 29
    "²»ÄÜ¶ÁÈ¡ %s !",                          //11 + 29
    "¶Ô %s Ã»ÓÐÐ´È¨ÏÞ!",                      //12 + 29
    "ÌáÊ¾ÐÅÏ¢",                                 //13 + 29
    "¶Á",                                       //14 + 29
    "Ð´",                                       //15 + 29
    "¶ÁÐ´",                                     //16 + 29
    "±£´æ",                                     //17 + 29
    "ÎÄ¼þ %s ´æÔÚ£¬ÊÇ·ñÌæ»»?"                   //18 + 29
#endif
};
#endif

#ifdef _BIG5_SUPPORT
static const char* SysText_BIG5 [] =
{
    "µ¡¤f",
    "ÉÛ©l",
    "¨ê·s­I´º",
    "Ãö³¬©Ò¦³µ¡¤f",
    "µ²§ô·|¸Ü",
    "µ¡¤f¾Þ§@",
    "³Ì¤p¤Æ",
    "³Ì¤j¤Æ",
    "«ìÎ`",
    "Ãö³¬",
    "ÚÌ©w",
    "¤U¤@¨B",
    "¨ú®ø",
    "¤W¤@¨B",
    "¬O(Y)",
    "§_(N)",
    "²×¤î(A)",
    "­«¸Õ(R)",
    "©¿²¤(I)",
    "Ãö¤_ MiniGUI...",
    "¥´¶}¤å¥ó",
    "«O¦s¤å¥ó",
	"ÃC¦â¿ï¾Ü",
    "¤Á´«¼h",
    "§R°£¼h",
    "¿ù»~",                                 // 25
    "¹Ï¼Ð",                                 // 26 
    "·í«e¸ô®|",                             // 27 
    "¤å¥ó",                                 // 28 
#if (!defined (__NOUNIX__) || defined (WIN32)) && defined (_EXT_CTRL_LISTVIEW)
    "¬d§ä¦ì¤_",                             //0 + 29
    "¤W¤@¯Å",                               //1 + 29
    "¦WºÙ",                                 //2 + 29
    "¤j¤p",                                 //3 + 29
    "³X°ÝÅv­­",                             //4 + 29
    "¤W¦¸­×§ï®É¶¡",                         //5 + 29
    "¥´¶}",                                 //6 + 29
    "¤å ¥ó ¦W",                             //7 + 29
    "¤å¥óÃþ«¬",                             //8 + 29
    "Åã¥ÜÁôÂÃ¤å¥ó",                         //9 + 29
    "¹ï¤£°_¡A¥¼§ä¨ì«ü©wªº¥Ø¿ý¡G %s ", //10 + 29
    "¤£¯àÅª¨ú %s !",                      //11 + 29
    "¹ï %s ¨S¦³¼gÅv­­",                   //23 + 29
    "´£¥Ü«H®§",                             //13 + 29
    "Åª",                                   //14 + 29
    "¼g",                                   //15 + 29
    "Åª¼g",                                 //16 + 29
    "«O¦s",                                 //17 + 29
    "¤å¥ó %s ¦s¦b¡A¬O§_´À´«?",              //18 + 29
#endif
    NULL
};
#endif

void __mg_init_local_sys_text (void)
{
    const char* charset = GetSysCharset (TRUE);

    local_SysText = SysText;

    if (charset == NULL)
        charset = GetSysCharset (FALSE);

#ifdef _GB_SUPPORT
    if (strcmp (charset, FONT_CHARSET_GB2312_0) == 0) {
        local_SysText = SysText_GB;
	}
#endif

#ifdef _GBK_SUPPORT
    if (strcmp (charset, FONT_CHARSET_GBK) == 0) {
        local_SysText = SysText_GB;
	}
#endif

#ifdef _GB18030_SUPPORT
    if (strcmp (charset, FONT_CHARSET_GB18030_0) == 0) {
        local_SysText = SysText_GB;
	}
#endif

#ifdef _BIG5_SUPPORT
    if (strcmp (charset, FONT_CHARSET_BIG5) == 0) {
        local_SysText = SysText_BIG5;
	}
#endif

}

const char* GUIAPI GetSysText (unsigned int id)
{
    if (id > IDS_MGST_MAXNUM)
        return NULL;
        
    return local_SysText [id];
}

#ifdef _UNICODE_SUPPORT
static const char* SysText_GB_UTF8 [] =
{
    "çª—å£",              // 0
    "å¼€å§‹",              // 1
    "åˆ·æ–°èƒŒæ™¯",             // 2
    "å…³é—­æ‰€æœ‰çª—å£",         // 3
    "ç»“æŸä¼šè¯",             // 4
    "çª—å£æ“ä½œ",             // 5
    "æœ€å°åŒ– ",
    "æœ€å¤§åŒ– ",
    "æ¢å¤",                 // 8
    "å…³é—­",                 // 9
    "ç¡®å®š",                 // 10
    "ä¸‹ä¸€æ­¥ ", 
    "å–æ¶ˆ",                 // 12
    "ä¸Šä¸€æ­¥ ",
    "æ˜¯(Y)",                // 14
    "å¦(N)",                // 15
    "ç»ˆæ­¢(A)",              // 16
    "é‡è¯•(R)",              // 17
    "å¿½ç•¥(I)",              // 18
    "å…³äºŽ MiniGUI...",      // 19
    "æ‰“å¼€æ–‡ä»¶",             // 20
    "ä¿å­˜æ–‡ä»¶",             // 21
    "é¢œè‰²é€‰æ‹©",             // 22
    "åˆ‡æ¢å±‚ ",
    "åˆ é™¤å±‚ ",
    "é”™è¯¯",                 // 25
    "å›¾æ ‡",                 // 26
    "å½“å‰è·¯å¾„",             // 27
    "æ–‡ä»¶",                 // 28
#if (!defined (__NOUNIX__) || defined (WIN32)) && defined (_EXT_CTRL_LISTVIEW)
    "æŸ¥æ‰¾ä½äºŽ",                                 //0 + 29
    "ä¸Šä¸€çº§ ",
    "åç§°",                                     //2 + 29
    "å¤§å°",                                     //3 + 29
    "è®¿é—®æƒé™",                                 //4 + 29
    "ä¸Šæ¬¡ä¿®æ”¹æ—¶é—´",                             //5 + 29
    "æ‰“å¼€",                                     //6 + 29
    "æ–‡ ä»¶ å ",                                 //7 + 29
    "æ–‡ä»¶ç±»åž‹",                                 //8 + 29
    "æ˜¾ç¤ºéšè—æ–‡ä»¶",                             //9 + 29
    "å¯¹ä¸èµ·ï¼Œæœªæ‰¾åˆ°æŒ‡å®šçš„ç›®å½•ï¼š  %s ",
    "ä¸èƒ½è¯»å– %s !",                          //11 + 29
    "å¯¹ %s æ²¡æœ‰å†™æƒé™!",                      //12 + 29
    "æç¤ºä¿¡æ¯",                                 //13 + 29
    "è¯» ",
    "å†™ ",
    "è¯»å†™",                                     //16 + 29
    "ä¿å­˜",                                     //17 + 29
    "æ–‡ä»¶ %s å­˜åœ¨ï¼Œæ˜¯å¦æ›¿æ¢?"                   //18 + 29
#endif
};

static const char* SysText_BIG5_UTF8 [] =
{
	"çª—å£",              // 0
    "é–‹å§‹",              // 1
    "åˆ·æ–°èƒŒæ™¯",             // 2
    "é—œé–‰æ‰€æœ‰çª—å£",         // 3
    "çµæŸæœƒè©±",             // 4
    "çª—å£æ“ä½œ",             // 5
    "æœ€å°åŒ– ",               // 6
    "æœ€å¤§åŒ– ",               // 7
    "æ¢å¾©",                 // 8
    "é—œé–‰",                 // 9
    "ç¢ºå®š",                 // 10
    "ä¸‹ä¸€æ­¥ ",               // 11
    "å–æ¶ˆ",                 // 12
    "ä¸Šä¸€æ­¥ ",               // 13
    "æ˜¯(Y)",                // 14
    "å¦(N)",                // 15
    "çµ‚æ­¢(A)",              // 16
    "é‡è©¦(R)",              // 17
    "å¿½ç•¥(I)",              // 18
    "é—œäºŽ MiniGUI...",      // 19
    "æ‰“é–‹æ–‡ä»¶",             // 20
    "ä¿å­˜æ–‡ä»¶",             // 21
    "é¡è‰²é¸æ“‡",             // 22
    "åˆ‡æ›å±¤ ",               // 23
    "åˆªé™¤å±¤ ",               // 24
    "éŒ¯èª¤",                 // 25
    "åœ–æ¨™",                 // 26
    "ç•¶å‰è·¯å¾‘",             // 27
    "æ–‡ä»¶",                 // 28
#if (!defined (__NOUNIX__) || defined (WIN32)) && defined (_EXT_CTRL_LISTVIEW)
    "æŸ¥æ‰¾ä½äºŽ",                                 //0 + 29
    "ä¸Šä¸€ç´š ",                                   //1 + 29
    "åç¨±",                                     //2 + 29
    "å¤§å°",                                     //3 + 29
    "è¨ªå•æ¬Šé™",                                 //4 + 29
    "ä¸Šæ¬¡ä¿®æ”¹æ™‚é–“",                             //5 + 29
    "æ‰“é–‹",                                     //6 + 29
    "æ–‡ ä»¶ å ",                                 //7 + 29
    "æ–‡ä»¶é¡žåž‹",                                 //8 + 29
    "é¡¯ç¤ºéš±è—æ–‡ä»¶",                             //9 + 29
    "å°ä¸èµ·ï¼Œæœªæ‰¾åˆ°æŒ‡å®šçš„ç›®éŒ„ï¼š%s ",     //10 + 29
    "ä¸èƒ½è®€å– %s !",                          //11 + 29
    "å° %s æ²’æœ‰å¯«æ¬Šé™!",                      //12 + 29
    "æç¤ºä¿¡æ¯",                                 //13 + 29
    "è®€ ",                                       //14 + 29
    "å¯« ",                                       //15 + 29
    "è®€å¯«",                                     //16 + 29
    "ä¿å­˜",                                     //17 + 29
    "æ–‡ä»¶ %s å­˜åœ¨ï¼Œæ˜¯å¦æ›¿æ›?",                  //18 + 29
#endif
    NULL
};

const char** GUIAPI GetSysTextInUTF8 (const char* language)
{
    if (strncasecmp (language, "zh_CN", 5) == 0) {
        return SysText_GB_UTF8;
	}
    else if (strncasecmp (language, "zh_TW", 5) == 0) {
        return SysText_BIG5_UTF8;
	}

    return SysText;
}
#endif /* _UNICODE_SUPPORT */

