/*
** $Id: simedit.c 8093 2007-11-16 07:37:30Z weiym $
**
** simedit.c: the Simple Edit Control module.
**
** Copyright (C) 2003 ~ 2007 Feynman Software.
** Copyright (C) 1999 ~ 2002 Wei Yongming.
**
** All rights reserved by Feynman Software.
** 
** Current maintainer: Yan Xiaowei
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "common.h"
#include "minigui.h"
#include "gdi.h"
#include "window.h"
#include "ctrl/ctrlhelper.h"
#include "ctrl/edit.h"
#include "cliprect.h"
#include "internals.h"
#include "ctrlclass.h"

#ifdef _CTRL_SIMEDIT

#include "ctrlmisc.h"
#include "simedit_impl.h"

#ifdef _FLAT_WINDOW_STYLE
#define WIDTH_EDIT_BORDER       1
#else
#define WIDTH_EDIT_BORDER       2
#endif

#define MARGIN_EDIT_LEFT        1
#define MARGIN_EDIT_TOP         1
#define MARGIN_EDIT_RIGHT       2
#define MARGIN_EDIT_BOTTOM      1

#define EST_FOCUSED     0x00000001L
#define EST_MODIFY      0x00000002L
#define EST_REPLACE     0x00000008L

#define EDIT_OP_NONE    0x00
#define EDIT_OP_DELETE  0x01
#define EDIT_OP_INSERT  0x02
#define EDIT_OP_REPLACE 0x03

static int SIMEditCtrlProc (HWND hWnd, int message, WPARAM wParam, LPARAM lParam);

BOOL RegisterSIMEditControl (void)
{
    WNDCLASS WndClass;

    WndClass.spClassName = CTRL_EDIT;
    WndClass.dwStyle     = WS_NONE;
    WndClass.dwExStyle   = WS_EX_NONE;
    WndClass.hCursor     = GetSystemCursor (IDC_IBEAM);
    WndClass.iBkColor    = GetWindowElementColor (BKC_EDIT_DEF);
    WndClass.WinProc     = SIMEditCtrlProc;

    return AddNewControlClass (&WndClass) == ERR_OK;
}

#if 0
void SIMEditControlCleanup (void)
{
    // do nothing
    return;
}
#endif

static inline int edtGetOutWidth (const CONTROL* pCtrl)
{
    return pCtrl->cr - pCtrl->cl 
            - ((PSIMEDITDATA)(pCtrl->dwAddData2))->leftMargin
            - ((PSIMEDITDATA)(pCtrl->dwAddData2))->rightMargin;
}

static int edtGetStartDispPosAtEnd (const CONTROL* pCtrl,
            PSIMEDITDATA simed)
{
    int         nOutWidth = edtGetOutWidth (pCtrl);
    int         endPos  = simed->dataEnd;
    int         newStartPos = simed->startPos;
    const char* buffer = simed->buffer;

    while (TRUE) {
        if ((endPos - newStartPos) * simed->charWidth < nOutWidth)
            break;
        
        if ((BYTE)buffer [newStartPos] > 0xA0) {
            newStartPos ++;
            if (newStartPos < simed->dataEnd) {
                if ((BYTE)buffer [newStartPos] > 0xA0)
                    newStartPos ++;
            }
        }
        else
            newStartPos ++;
    }

    return newStartPos;
}

static int edtGetDispLen (const CONTROL* pCtrl)
{
    int i, n = 0;
    int nOutWidth = edtGetOutWidth (pCtrl);
    int nTextWidth = 0;
    PSIMEDITDATA simed = (PSIMEDITDATA)(pCtrl->dwAddData2);
    const char* buffer = simed->buffer;

    for (i = simed->startPos; i < simed->dataEnd; i++) {
        if ((BYTE)buffer [i] > 0xA0) {
            i++;
            if (i < simed->dataEnd) {
                if ((BYTE)buffer [i] > 0xA0) {
                    nTextWidth += simed->charWidth * 2;
                    n += 2;
                }
                else
                    i--;
            }
            else {
                nTextWidth += simed->charWidth;
                n++;
            }
        }
        else {
            nTextWidth += simed->charWidth;
            n++;
        }

        if (nTextWidth > nOutWidth)
            break;
    }

    return n;
}

static int edtGetOffset (const SIMEDITDATA* simed, int x)
{
    int i;
    int newOff = 0;
    int nTextWidth = 0;
    const char* buffer = simed->buffer;

    x -= simed->leftMargin;
    for (i = simed->startPos; i < simed->dataEnd; i++) {
        if ((nTextWidth + (simed->charWidth >> 1)) >= x)
            break;

        if ((BYTE)buffer [i] > 0xA0) {
            i++;
            if (i < simed->dataEnd) {
                if ((BYTE)buffer [i] > 0xA0) {
                    nTextWidth += simed->charWidth * 2;
                    newOff += 2;
                }
                else
                    i --;
            }
            else {
                nTextWidth += simed->charWidth;
                newOff ++;
            }
        }
        else {
            nTextWidth += simed->charWidth;
            newOff ++;
        }

    }

    return newOff;
}

static BOOL edtIsACCharBeforePosition (const char* string, int pos)
{
    if (pos < 2)
        return FALSE;

    if ((BYTE)string [pos - 2] > 0xA0 && (BYTE)string [pos - 1] > 0xA0)
        return TRUE;

    return FALSE;
}

static BOOL edtIsACCharAtPosition (const char* string, int len, int pos)
{
    if (pos > (len - 2))
        return FALSE;

    if ((BYTE)string [pos] > 0xA0 && (BYTE)string [pos + 1] > 0xA0)
        return TRUE;

    return FALSE;
}

static int SIMEditCtrlProc (HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{   
    PCONTROL    pCtrl;
    DWORD       dwStyle;
    HDC         hdc;
    PSIMEDITDATA simed;

    pCtrl       = Control(hWnd);
    dwStyle     = pCtrl->dwStyle;

    switch (message)
    {
        case MSG_CREATE:
            if (!(simed = calloc (1, sizeof (SIMEDITDATA)))) {
                return -1;
            }

            simed->status         = 0;
            simed->bufferLen      = LEN_SIMEDIT_BUFFER;
            simed->buffer         = calloc (LEN_SIMEDIT_BUFFER, sizeof (char));

            if (simed->buffer == NULL) {
                free (simed);
                return -1;
            }

            if (!CreateCaret (hWnd, NULL, 1, GetSysFontHeight (SYSLOGFONT_FIXED))) {
                free (simed->buffer);
                free (simed);
                return -1;
            }
           
            simed->editPos        = 0;
            simed->caretOff       = 0;
            simed->startPos       = 0;
            
            simed->passwdChar     = '*';
            simed->leftMargin     = MARGIN_EDIT_LEFT;
            simed->topMargin      = MARGIN_EDIT_TOP;
            simed->rightMargin    = MARGIN_EDIT_RIGHT;
            simed->bottomMargin   = MARGIN_EDIT_BOTTOM;

            simed->hardLimit      = -1;
            
            simed->dataEnd        = strlen (pCtrl->spCaption);
            memcpy (simed->buffer, pCtrl->spCaption, MIN (LEN_SIMEDIT_BUFFER, simed->dataEnd));
            simed->dataEnd        = MIN (LEN_SIMEDIT_BUFFER, simed->dataEnd);
            
            pCtrl->dwAddData2 = (DWORD) simed;
            pCtrl->pLogFont = GetSystemFont (SYSLOGFONT_FIXED);
            simed->charWidth = GetSysFontMaxWidth (SYSLOGFONT_FIXED);
            break;

        case MSG_DESTROY:
            simed = (PSIMEDITDATA) (pCtrl->dwAddData2);
            DestroyCaret (hWnd);
            free (simed->buffer);
            free (simed);
            break;

        case MSG_ERASEBKGND:
            EditOnEraseBackground (hWnd, (HDC)wParam, (const RECT*)lParam);
            return 0;

        case MSG_FONTCHANGING:
            return -1;
        
        case MSG_SETCURSOR:
            if (dwStyle & WS_DISABLED) {
                SetCursor (GetSystemCursor (IDC_ARROW));
                return 0;
            }
            break;

        case MSG_KILLFOCUS:
            simed = (PSIMEDITDATA) (pCtrl->dwAddData2);
            if (simed->status & EST_FOCUSED) {
                simed->status &= ~EST_FOCUSED;
                HideCaret (hWnd);
                NotifyParent (hWnd, pCtrl->id, EN_KILLFOCUS);
            }
            break;
        
        case MSG_SETFOCUS:
            simed = (PSIMEDITDATA) (pCtrl->dwAddData2);
            if (simed->status & EST_FOCUSED)
                break;
            
            simed->status |= EST_FOCUSED;

            /* only implemented for ES_LEFT align format. */
            SetCaretPos (hWnd, simed->caretOff * simed->charWidth + simed->leftMargin, 
                simed->topMargin);
                
            ShowCaret (hWnd);
            ActiveCaret (hWnd);

            NotifyParent (hWnd, pCtrl->id, EN_SETFOCUS);
            break;
        
        case MSG_ENABLE:
            if ( (!(dwStyle & WS_DISABLED) && !wParam)
                    || ((dwStyle & WS_DISABLED) && wParam) ) {
                if (wParam)
                    pCtrl->dwStyle &= ~WS_DISABLED;
                else
                    pCtrl->dwStyle |=  WS_DISABLED;

                InvalidateRect (hWnd, NULL, TRUE);
            }
            return 0;

        case MSG_PAINT:
        {
            int     dispLen;
            char*   dispBuffer;
            RECT    rect;
            
            simed = (PSIMEDITDATA) (pCtrl->dwAddData2);

            GetClientRect (hWnd, &rect);

            /* only implemented ES_LEFT align format for single line edit. */
            rect.left += simed->leftMargin;
            rect.top += simed->topMargin;
            rect.right -= simed->rightMargin;
            rect.bottom -= simed->bottomMargin;
    
            hdc = BeginPaint (hWnd);

            if (dwStyle & ES_BASELINE) {
                SetPenColor (hdc, GetWindowElementColorEx (hWnd, FGC_CONTROL_NORMAL));
                DrawHDotLine (hdc, 
                            simed->leftMargin, 
                            simed->topMargin + GetSysFontHeight (SYSLOGFONT_FIXED) + 1,
                            RECTW (rect) - simed->leftMargin - simed->rightMargin);
            }

            dispLen = edtGetDispLen (pCtrl);
            if (dispLen == 0) {
                EndPaint (hWnd, hdc);
                break;
            }

            if (dwStyle & ES_PASSWORD) {
                /* dispBuffer = ALLOCATE_LOCAL (dispLen); */
                dispBuffer = FixStrAlloc (dispLen);
                memset (dispBuffer, simed->passwdChar, dispLen);
            }
            else
                dispBuffer = simed->buffer + simed->startPos;

            ClipRectIntersect (hdc, &rect);

            SetBkMode (hdc, BM_TRANSPARENT);
            SetBkColor (hdc, GetWindowBkColor (hWnd));
            if (dwStyle & WS_DISABLED)
                SetTextColor (hdc, GetWindowElementColorEx (hWnd, FGC_CONTROL_DISABLED));
            else
                SetTextColor (hdc, GetWindowElementColorEx (hWnd, FGC_CONTROL_NORMAL));

            TextOutLen (hdc, simed->leftMargin, simed->topMargin, dispBuffer, dispLen);

            if (dwStyle & ES_PASSWORD) {
                FreeFixStr (dispBuffer);
                /* DEALLOCATE_LOCAL (dispBuffer); */
            }

            EndPaint (hWnd, hdc);
            return 0;
        }

        case MSG_KEYDOWN:
        {
            BOOL    bChange = FALSE;
            int     i;
            RECT    InvRect;
            int     deleted;

            simed = (PSIMEDITDATA) (pCtrl->dwAddData2);
        
            switch (LOWORD (wParam))
            {
                case SCANCODE_KEYPADENTER:
                case SCANCODE_ENTER:
                    NotifyParent (hWnd, pCtrl->id, EN_ENTER);
                return 0;

                case SCANCODE_HOME:
                    if (simed->editPos == 0)
                        return 0;

                    simed->editPos  = 0;
                    simed->caretOff = 0;
                    
                    SetCaretPos (hWnd, 
                        simed->caretOff * simed->charWidth + simed->leftMargin, 
                        simed->topMargin);

                    if (simed->startPos != 0)
                        InvalidateRect (hWnd, NULL, TRUE);
                    
                    simed->startPos = 0;
                return 0;
           
                case SCANCODE_END:
                {
                    int newStartPos;
                   
                    if (simed->editPos == simed->dataEnd)
                        return 0;

                    newStartPos = edtGetStartDispPosAtEnd (pCtrl, simed);
                    
                    simed->editPos = simed->dataEnd;
                    simed->caretOff = simed->editPos - newStartPos;
                    
                    SetCaretPos (hWnd, 
                        simed->caretOff * simed->charWidth + simed->leftMargin, 
                        simed->topMargin);

                    if (simed->startPos != newStartPos)
                        InvalidateRect (hWnd, NULL, TRUE);
                    
                    simed->startPos = newStartPos;
                }
                return 0;

                case SCANCODE_CURSORBLOCKLEFT:
                {
                    BOOL bScroll = FALSE;
                    int  scrollStep;
                    
                    if (simed->editPos == 0)
                        return 0;

                    if (edtIsACCharBeforePosition (simed->buffer, 
                            simed->editPos)) {
                        scrollStep = 2;
                        simed->editPos -= 2;
                    }
                    else {
                        scrollStep = 1;
                        simed->editPos --;
                    }

                    simed->caretOff -= scrollStep;
                    if (simed->caretOff == 0 
                            && simed->editPos != 0) {

                        bScroll = TRUE;

                        if (edtIsACCharBeforePosition (simed->buffer, 
                                simed->editPos)) {
                            simed->startPos -= 2;
                            simed->caretOff = 2;
                        }
                        else {
                            simed->startPos --;
                            simed->caretOff = 1;
                        }
                    }
                    else if (simed->caretOff < 0) {
                        simed->startPos = 0;
                        simed->caretOff = 0;
                    }
                        
                    SetCaretPos (hWnd, 
                        simed->caretOff * simed->charWidth + simed->leftMargin, 
                        simed->topMargin);

                    if (bScroll)
                        InvalidateRect (hWnd, NULL, TRUE);
                }
                return 0;
                
                case SCANCODE_CURSORBLOCKRIGHT:
                {
                    BOOL bScroll = FALSE;
                    int  scrollStep, moveStep;

                    if (simed->editPos == simed->dataEnd)
                        return 0;

                    if (edtIsACCharAtPosition (simed->buffer, 
                                simed->dataEnd,
                                simed->startPos)) {
                        if (edtIsACCharAtPosition (simed->buffer, 
                                    simed->dataEnd,
                                    simed->editPos)) {
                            scrollStep = 2;
                            moveStep = 2;
                            simed->editPos  += 2;
                        }
                        else {
                            scrollStep = 2;
                            moveStep = 1;
                            simed->editPos ++;
                        }
                    }
                    else {
                        if (edtIsACCharAtPosition (simed->buffer, 
                                    simed->dataEnd,
                                    simed->editPos)) {
                                    
                            if (edtIsACCharAtPosition (simed->buffer, 
                                    simed->dataEnd,
                                    simed->startPos + 1))
                                scrollStep = 3;
                            else
                                scrollStep = 2;

                            moveStep = 2;
                            simed->editPos += 2;
                        }
                        else {
                            scrollStep = 1;
                            moveStep = 1;
                            simed->editPos ++;
                        }
                    }

                    simed->caretOff += moveStep;
                    if (simed->caretOff * simed->charWidth > edtGetOutWidth (pCtrl)) {
                        bScroll = TRUE;
                        simed->startPos += scrollStep;

                        simed->caretOff = 
                            simed->editPos - simed->startPos;
                    }

                    SetCaretPos (hWnd, 
                        simed->caretOff * simed->charWidth + simed->leftMargin, 
                        simed->topMargin);

                    if (bScroll)
                        InvalidateRect (hWnd, NULL, TRUE);
                }
                return 0;
                
                case SCANCODE_INSERT:
                    simed = (PSIMEDITDATA) (pCtrl->dwAddData2);
                    simed->status ^= EST_REPLACE;
                break;

                case SCANCODE_REMOVE:
                    simed = (PSIMEDITDATA) (pCtrl->dwAddData2);
                    if ((dwStyle & ES_READONLY)
                            || (simed->editPos == simed->dataEnd)){
                        return 0;
                    }
                    
                    if (edtIsACCharAtPosition (simed->buffer, 
                                    simed->dataEnd,
                                    simed->editPos))
                        deleted = 2;
                    else
                        deleted = 1;
                        
                    for (i = simed->editPos; 
                            i < simed->dataEnd - deleted;
                            i++)
                        simed->buffer [i] 
                            = simed->buffer [i + deleted];

                    simed->dataEnd -= deleted;
                    bChange = TRUE;
                    
                    InvRect.left = simed->leftMargin + simed->caretOff * simed->charWidth;
                    InvRect.top = simed->topMargin;
                    InvRect.right = pCtrl->cr - pCtrl->cl;
                    InvRect.bottom = pCtrl->cb - pCtrl->ct;
                    
                    InvalidateRect (hWnd, &InvRect, TRUE);
                break;

                case SCANCODE_BACKSPACE:
                    simed = (PSIMEDITDATA) (pCtrl->dwAddData2);
                    if ((dwStyle & ES_READONLY)
                            || (simed->editPos == 0)) {
                        return 0;
                    }

                    if (edtIsACCharBeforePosition (simed->buffer, 
                                    simed->editPos))
                        deleted = 2;
                    else
                        deleted = 1;
                        
                    for (i = simed->editPos; 
                            i < simed->dataEnd;
                            i++)
                        simed->buffer [i - deleted] 
                            = simed->buffer [i];

                    simed->dataEnd -= deleted;
                    simed->editPos -= deleted;
                    bChange = TRUE;
                    
                    simed->caretOff -= deleted;
                    if (simed->caretOff == 0 
                            && simed->editPos != 0) {
                        if (edtIsACCharBeforePosition (simed->buffer, 
                                simed->editPos)) {
                            simed->startPos -= 2;
                            simed->caretOff = 2;
                        }
                        else {
                            simed->startPos --;
                            simed->caretOff = 1;
                        }
                        
                        InvRect.left = simed->leftMargin;
                        InvRect.top = simed->topMargin;
                        InvRect.right = pCtrl->cr - pCtrl->cl;
                        InvRect.bottom = pCtrl->cb - pCtrl->ct;
                    }
                    else {
                        InvRect.left = simed->leftMargin
                                    + simed->caretOff * simed->charWidth;
                        InvRect.top = simed->topMargin;
                        InvRect.right = pCtrl->cr - pCtrl->cl;
                        InvRect.bottom = pCtrl->cb - pCtrl->ct;
                    }
                        
                    SetCaretPos (hWnd, 
                        simed->caretOff * simed->charWidth + simed->leftMargin, 
                        simed->topMargin);

                    InvalidateRect (hWnd, &InvRect, TRUE);
                break;

                default:
                break;
            }
       
            if (bChange)
                NotifyParent (hWnd, pCtrl->id, EN_CHANGE);
            return 0;
        }

        case MSG_CHAR:
        {
            unsigned char charBuffer [2];
            int  i, chars, scrollStep, inserting;
            RECT InvRect;

            simed = (PSIMEDITDATA) (pCtrl->dwAddData2);

            if (dwStyle & ES_READONLY) {
                return 0;
            }
            
            if (HIBYTE (wParam)) {
                charBuffer [0] = LOBYTE (wParam);
                charBuffer [1] = HIBYTE (wParam);
                chars = 2;
            }
            else {
                chars = 1;

                if (dwStyle & ES_UPPERCASE) {
                    charBuffer [0] = toupper (LOBYTE (wParam));
                }
                else if (dwStyle & ES_LOWERCASE) {
                    charBuffer [0] = tolower (LOBYTE (wParam));
                }
                else
                    charBuffer [0] = LOBYTE (wParam);
            }
            
            if (chars == 1) {
#if 0
                switch (charBuffer [0])
                {
                    case 0x00:  // NULL
                    case 0x07:  // BEL
                    case 0x08:  // BS
                    case 0x09:  // HT
                    case 0x0A:  // LF
                    case 0x0B:  // VT
                    case 0x0C:  // FF
                    case 0x0D:  // CR
                    case 0x1B:  // Escape
                    return 0;
                }
#else
                if (charBuffer [0] < 0x20 || charBuffer [0] == 0x7F)
                    return 0;
#endif
            }

            if (simed->status & EST_REPLACE) {
                if (simed->dataEnd == simed->editPos)
                    inserting = chars;
                else if (edtIsACCharAtPosition (simed->buffer, 
                                simed->dataEnd,
                                simed->editPos)) {
                    if (chars == 2)
                        inserting = 0;
                    else
                        inserting = -1;
                }
                else {
                    if (chars == 2)
                        inserting = 1;
                    else
                        inserting = 0;
                }
            }
            else
                inserting = chars;

            // check space
            if (simed->dataEnd + inserting > simed->bufferLen) {
                Ping ();
                NotifyParent (hWnd, pCtrl->id, EN_MAXTEXT);
                return 0;
            }
            else if ((simed->hardLimit >= 0) 
                        && ((simed->dataEnd + inserting) 
                            > simed->hardLimit)) {
                Ping ();
                NotifyParent (hWnd, pCtrl->id, EN_MAXTEXT);
                return 0;
            }

            if (inserting == -1) {
                for (i = simed->editPos; i < simed->dataEnd-1; i++)
                    simed->buffer [i] = simed->buffer [i + 1];
            }
            else if (inserting > 0) {
                for (i = simed->dataEnd + inserting - 1; 
                        i > simed->editPos + inserting - 1; 
                        i--)
                    simed->buffer [i] 
                            = simed->buffer [i - inserting];
            }

            for (i = 0; i < chars; i++)
                    simed->buffer [simed->editPos + i] 
                        = charBuffer [i];
            
            simed->editPos += chars;
            simed->caretOff += chars;
            simed->dataEnd += inserting;

            if (simed->caretOff * simed->charWidth
                            > edtGetOutWidth (pCtrl))
            {
                if (edtIsACCharAtPosition (simed->buffer, 
                                simed->dataEnd,
                                simed->startPos))
                    scrollStep = 2;
                else {
                    if (chars == 2) {
                        if (edtIsACCharAtPosition (simed->buffer, 
                                simed->dataEnd,
                                simed->startPos + 1))
                            scrollStep = 3;
                        else
                            scrollStep = 2;
                    }
                    else
                        scrollStep = 1;
                }
                    
                simed->startPos += scrollStep;

                simed->caretOff = 
                            simed->editPos - simed->startPos;

                InvRect.left = simed->leftMargin;
                InvRect.top = simed->topMargin;
                InvRect.right = pCtrl->cr - pCtrl->cl;
                InvRect.bottom = pCtrl->cb - pCtrl->ct;
            }
            else {
                InvRect.left = simed->leftMargin
                                    + (simed->caretOff - chars)
                                        * simed->charWidth;
                InvRect.top = simed->topMargin;
                InvRect.right = pCtrl->cr - pCtrl->cl;
                InvRect.bottom = pCtrl->cb - pCtrl->ct;
            }

            SetCaretPos (hWnd, 
                        simed->caretOff * simed->charWidth + simed->leftMargin, 
                        simed->topMargin);

            InvalidateRect (hWnd, &InvRect, TRUE);

            NotifyParent (hWnd, pCtrl->id, EN_CHANGE);
            return 0;
        }

        case MSG_GETTEXTLENGTH:
            simed = (PSIMEDITDATA) (pCtrl->dwAddData2);
            return simed->dataEnd;
        
        case MSG_GETTEXT:
        {
            char*   buffer = (char*)lParam;
            int     len;

            simed = (PSIMEDITDATA) (pCtrl->dwAddData2);

            len = MIN ((int)wParam, simed->dataEnd);

            memcpy (buffer, simed->buffer, len);
            buffer [len] = '\0';

            return len;
        }

        case MSG_SETTEXT:
        {
            int len;
            simed = (PSIMEDITDATA) (pCtrl->dwAddData2);
            
            len = strlen ((char*)lParam);
            len = MIN (len, simed->bufferLen);
            
            if (simed->hardLimit >= 0)
                len = MIN (len, simed->hardLimit);
           
            memcpy (simed->buffer, (char*)lParam, len);
            simed->dataEnd = len;
            if (simed->editPos > len) {
                /* reset caret position */
                simed->editPos = 0;
                simed->caretOff = 0;
                simed->startPos = 0;
                if (simed->status & EST_FOCUSED)
	                SetCaretPos (hWnd, simed->leftMargin, simed->topMargin);
            }
			InvalidateRect (hWnd, NULL, TRUE);
            NotifyParent (hWnd, GetDlgCtrlID(hWnd), EN_UPDATE);
            return 0;
        }

        case MSG_LBUTTONDBLCLK:
            NotifyParent (hWnd, pCtrl->id, EN_DBLCLK);
            break;
        
        case MSG_LBUTTONDOWN:
        {
            int newOff;
            
            simed = (PSIMEDITDATA) (pCtrl->dwAddData2);
            
            newOff = edtGetOffset (simed, LOSWORD (lParam));
            
            if (newOff != simed->caretOff) {
                simed->editPos += newOff - simed->caretOff;
                simed->caretOff = newOff;
                SetCaretPos (hWnd, 
                        simed->caretOff * simed->charWidth
                            + simed->leftMargin, 
                        simed->topMargin);
            }

            break;
        }

        case MSG_LBUTTONUP:
            NotifyParent (hWnd, pCtrl->id, EN_CLICKED);
            break;
        
        case MSG_GETDLGCODE:
            return DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTARROWS | DLGC_WANTENTER;

        case MSG_DOESNEEDIME:
            if (dwStyle & ES_READONLY)
                return FALSE;
            return TRUE;
        
        case EM_GETCARETPOS:
        {
            int* line_pos = (int *)wParam;
            int* char_pos = (int *)lParam;

            simed = (PSIMEDITDATA) (pCtrl->dwAddData2);

            if (line_pos) *line_pos = 0;
            if (char_pos) *char_pos = simed->editPos;

            return simed->editPos;
        }

        case EM_SETCARETPOS:
        {
            int char_pos = (int)lParam;

            simed = (PSIMEDITDATA) (pCtrl->dwAddData2);

            if (char_pos >= 0 && char_pos <= simed->dataEnd) {
                simed->editPos = char_pos;
                simed->caretOff = 
                            simed->editPos - simed->startPos;
                if (simed->status & EST_FOCUSED)
	                SetCaretPos (hWnd, simed->caretOff * simed->charWidth
                                    + simed->leftMargin, simed->topMargin);
                return 0;
            }

            return -1;
        }

        case EM_SETREADONLY:
            if (wParam)
                pCtrl->dwStyle |= ES_READONLY;
            else
                pCtrl->dwStyle &= ~ES_READONLY;
            return 0;
        
        case EM_SETPASSWORDCHAR:
            simed = (PSIMEDITDATA) (pCtrl->dwAddData2);

            if (simed->passwdChar != (char)wParam) {
                if (dwStyle & ES_PASSWORD) {
                    simed->passwdChar = (char)wParam;
                    InvalidateRect (hWnd, NULL, TRUE);
                }
            }
            return 0;
    
        case EM_GETPASSWORDCHAR:
        {
            simed = (PSIMEDITDATA) (pCtrl->dwAddData2);

            return simed->passwdChar;
        }
    
        case EM_LIMITTEXT:
        {
            int newLimit = (int)wParam;
            
            if (newLimit >= 0) {
                simed = (PSIMEDITDATA) (pCtrl->dwAddData2);
                if (simed->bufferLen < newLimit)
                    simed->hardLimit = -1;
                else
                    simed->hardLimit = newLimit;
            }
            return 0;
        }
    
        default:
            break;
    } 

    return DefaultControlProc (hWnd, message, wParam, lParam);
}

#endif /* _CTRL_SIMEDIT */

