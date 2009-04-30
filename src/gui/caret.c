/*
** $Id: caret.c 7474 2007-08-28 07:40:45Z weiym $
**
** caret.c: The Caret module.
**
** Copyright (C) 2003 ~ 2007 Feynman Software.
** Copyright (C) 1999 ~ 2002 Wei Yongming.
**
** All rights reserved by Feynman Software.
**
** Current maintainer: Wei Yongming.
**
** Create date: 1999.07.03
*/

#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "minigui.h"
#include "gdi.h"
#include "window.h"
#include "cliprect.h"
#include "gal.h"
#include "internals.h"

BOOL GUIAPI CreateCaret (HWND hWnd, PBITMAP pBitmap, int nWidth, int nHeight)
{
    PMAINWIN pWin;

    pWin = (PMAINWIN)hWnd;

    if (!pWin->pCaretInfo) {
        if (!(pWin->pCaretInfo = malloc (sizeof (CARETINFO))))
            return FALSE;
        
        pWin->pCaretInfo->pBitmap = pBitmap;
        if (pBitmap) {
            nWidth  = pBitmap->bmWidth;
            nHeight = pBitmap->bmHeight;
        }
        pWin->pCaretInfo->nWidth  = nWidth;
        pWin->pCaretInfo->nHeight = nHeight;
#ifdef _USE_NEWGAL
        pWin->pCaretInfo->caret_bmp.bmType = BMP_TYPE_NORMAL;
        pWin->pCaretInfo->caret_bmp.bmBitsPerPixel = BITSPERPHYPIXEL;
        pWin->pCaretInfo->caret_bmp.bmBytesPerPixel = BYTESPERPHYPIXEL;
        pWin->pCaretInfo->caret_bmp.bmWidth = nWidth;
        pWin->pCaretInfo->caret_bmp.bmHeight = nHeight;
        pWin->pCaretInfo->caret_bmp.bmAlphaPixelFormat = NULL;

        pWin->pCaretInfo->nBytesNr = GAL_GetBoxSize (__gal_screen, 
                        nWidth, nHeight, &pWin->pCaretInfo->caret_bmp.bmPitch);
#else
        pWin->pCaretInfo->nEffWidth  = nWidth;
        pWin->pCaretInfo->nEffHeight = nHeight;

        pWin->pCaretInfo->nBytesNr = nWidth * nHeight * BYTESPERPHYPIXEL;
#endif
        pWin->pCaretInfo->pNormal = malloc (pWin->pCaretInfo->nBytesNr);
        pWin->pCaretInfo->pXored  = malloc (pWin->pCaretInfo->nBytesNr);

        if (pWin->pCaretInfo->pNormal == NULL ||
            pWin->pCaretInfo->pXored == NULL) {
            free (pWin->pCaretInfo);
            pWin->pCaretInfo = NULL;
            return FALSE;
        }

        pWin->pCaretInfo->x = pWin->pCaretInfo->y = 0;
        
        pWin->pCaretInfo->fBlink  = FALSE;
        pWin->pCaretInfo->fShow   = FALSE;
        
        pWin->pCaretInfo->hOwner  = hWnd;
        pWin->pCaretInfo->uTime   = 500;
    }

    SendMessage (HWND_DESKTOP, MSG_CARET_CREATE, (WPARAM)hWnd, 0);

    return TRUE;
}

BOOL GUIAPI ActiveCaret (HWND hWnd)
{
    PMAINWIN pWin;

    pWin = (PMAINWIN)hWnd;

    if (!pWin->pCaretInfo)
        return FALSE;

    SendMessage (HWND_DESKTOP, MSG_CARET_CREATE, (WPARAM)hWnd, 0);

    return TRUE;
}

BOOL GUIAPI DestroyCaret (HWND hWnd)
{
    PMAINWIN pWin;

    pWin = (PMAINWIN)hWnd;

    if (!pWin->pCaretInfo)
        return FALSE;

    free (pWin->pCaretInfo->pNormal);
    free (pWin->pCaretInfo->pXored);
    
    free (pWin->pCaretInfo);
    pWin->pCaretInfo = NULL;

    SendMessage (HWND_DESKTOP, MSG_CARET_DESTROY, (WPARAM)hWnd, 0);

    return TRUE;
}

UINT GUIAPI GetCaretBlinkTime (HWND hWnd)
{
    PMAINWIN pWin;

    pWin = (PMAINWIN)hWnd;

    if (!pWin->pCaretInfo)
        return 0;
    
    return pWin->pCaretInfo->uTime;
}

#define MIN_BLINK_TIME      100     // ms

BOOL GUIAPI SetCaretBlinkTime (HWND hWnd, UINT uTime)
{
    PMAINWIN pWin;

    pWin = (PMAINWIN)hWnd;

    if (!pWin->pCaretInfo)
        return FALSE;

    if (uTime < MIN_BLINK_TIME)
        uTime = MIN_BLINK_TIME;

    pWin->pCaretInfo->uTime = uTime;

    SendMessage (HWND_DESKTOP, MSG_CARET_CREATE, (WPARAM)hWnd, 0);

    return TRUE;
}

BOOL GUIAPI HideCaret (HWND hWnd)
{
    PMAINWIN pWin;

    pWin = (PMAINWIN)hWnd;

    if (!pWin->pCaretInfo)
        return FALSE;

    if (!pWin->pCaretInfo->fBlink)
        return FALSE;

    pWin->pCaretInfo->fBlink = FALSE;
    if (pWin->pCaretInfo->fShow) {
        HDC hdc;
        
        pWin->pCaretInfo->fShow = FALSE;

        // hide caret immediately
        hdc = GetClientDC (hWnd);
#ifdef _USE_NEWGAL
        pWin->pCaretInfo->caret_bmp.bmBits = pWin->pCaretInfo->pNormal;
        FillBoxWithBitmap (hdc,
                        pWin->pCaretInfo->x, pWin->pCaretInfo->y, 0, 0, 
                        &pWin->pCaretInfo->caret_bmp);
#else
        PutSavedBoxOnDC (hdc, 
                        pWin->pCaretInfo->x, pWin->pCaretInfo->y,
                        pWin->pCaretInfo->nEffWidth,
                        pWin->pCaretInfo->nEffHeight,
                        pWin->pCaretInfo->pNormal);
#endif /* _USE_NEWGAL */
        ReleaseDC (hdc);
    }

    return TRUE;
}

void GetCaretBitmaps (PCARETINFO pCaretInfo)
{
    int i;
    int sx, sy;
        
    // convert to screen coordinates
    sx = pCaretInfo->x;
    sy = pCaretInfo->y;
    ClientToScreen (pCaretInfo->hOwner, &sx, &sy);

    // save normal bitmap first.
#ifdef _USE_NEWGAL
    pCaretInfo->caret_bmp.bmBits = pCaretInfo->pNormal;
    GetBitmapFromDC (HDC_SCREEN, sx, sy, 
                    pCaretInfo->caret_bmp.bmWidth,
                    pCaretInfo->caret_bmp.bmHeight,
                    &pCaretInfo->caret_bmp);
#else
    SaveScreenBox (sx, sy, 
                       pCaretInfo->nEffWidth, pCaretInfo->nEffHeight,
                       pCaretInfo->pNormal);
#endif

    // generate XOR bitmap.
    if (pCaretInfo->pBitmap) {
        BYTE* normal;
        BYTE* bitmap;
        BYTE* xored;
                
        normal = pCaretInfo->pNormal;
        bitmap = pCaretInfo->pBitmap->bmBits;
        xored  = pCaretInfo->pXored;
            
        for (i = 0; i < pCaretInfo->nBytesNr; i++)
            xored[i] = normal[i] ^ bitmap[i];
    }
    else {
        BYTE* normal;
        BYTE* xored;
        BYTE xor_byte;

        if (BITSPERPHYPIXEL < 8)
            xor_byte = 0x0F;
        else
            xor_byte = 0xFF;
                
        normal = pCaretInfo->pNormal;
        xored  = pCaretInfo->pXored;
            
        for (i = 0; i < pCaretInfo->nBytesNr; i++)
            xored[i] = normal[i] ^ xor_byte;
    }
}

BOOL BlinkCaret (HWND hWnd)
{
    PMAINWIN pWin;
    HDC hdc;

    pWin = (PMAINWIN)hWnd;

    if (!pWin->pCaretInfo)
        return FALSE;

    if (!pWin->pCaretInfo->fBlink)
        return FALSE;

    hdc = GetClientDC (hWnd);

    if (!pWin->pCaretInfo->fShow) {
        // show caret
#ifdef _USE_NEWGAL
        pWin->pCaretInfo->caret_bmp.bmBits = pWin->pCaretInfo->pXored;
        FillBoxWithBitmap (hdc,
                        pWin->pCaretInfo->x, pWin->pCaretInfo->y, 0, 0, 
                        &pWin->pCaretInfo->caret_bmp);
#else
        PutSavedBoxOnDC (hdc, 
                        pWin->pCaretInfo->x, pWin->pCaretInfo->y,
                        pWin->pCaretInfo->nEffWidth,
                        pWin->pCaretInfo->nEffHeight,
                        pWin->pCaretInfo->pXored);
#endif /* _USE_NEWGAL */

        pWin->pCaretInfo->fShow = TRUE;
    }
    else {
        // hide caret
#ifdef _USE_NEWGAL
        pWin->pCaretInfo->caret_bmp.bmBits = pWin->pCaretInfo->pNormal;
        FillBoxWithBitmap (hdc,
                        pWin->pCaretInfo->x, pWin->pCaretInfo->y, 0, 0, 
                        &pWin->pCaretInfo->caret_bmp);
#else
        PutSavedBoxOnDC (hdc, 
                        pWin->pCaretInfo->x, pWin->pCaretInfo->y,
                        pWin->pCaretInfo->nEffWidth,
                        pWin->pCaretInfo->nEffHeight,
                        pWin->pCaretInfo->pNormal);
#endif /* _USE_NEWGAL */

        pWin->pCaretInfo->fShow = FALSE;
    }
    
    ReleaseDC (hdc);

    return TRUE;
}

BOOL GUIAPI ShowCaret (HWND hWnd)
{
    PMAINWIN pWin;

    pWin = (PMAINWIN)hWnd;

    if (!pWin->pCaretInfo)
        return FALSE;

    if (pWin->pCaretInfo->fBlink)
        return FALSE;

    pWin->pCaretInfo->fBlink = TRUE;
    GetCaretBitmaps (pWin->pCaretInfo);
    
    if (!pWin->pCaretInfo->fShow) {

        HDC hdc;
        
        // show caret immediately
        hdc = GetClientDC (hWnd);
#ifdef _USE_NEWGAL
        pWin->pCaretInfo->caret_bmp.bmBits = pWin->pCaretInfo->pXored;
        FillBoxWithBitmap (hdc,
                        pWin->pCaretInfo->x, pWin->pCaretInfo->y, 0, 0, 
                        &pWin->pCaretInfo->caret_bmp);
#else
        PutSavedBoxOnDC (hdc, 
                        pWin->pCaretInfo->x, pWin->pCaretInfo->y,
                        pWin->pCaretInfo->nEffWidth,
                        pWin->pCaretInfo->nEffHeight,
                        pWin->pCaretInfo->pXored);
#endif /* _USE_NEWGAL */
        ReleaseDC (hdc);

        pWin->pCaretInfo->fShow = TRUE;
    }

    return TRUE;
}

BOOL GUIAPI SetCaretPos (HWND hWnd, int x, int y)
{
    PMAINWIN pWin;

    pWin = (PMAINWIN)hWnd;

    if (!pWin->pCaretInfo)
        return FALSE;

    if (pWin->pCaretInfo->x == x && pWin->pCaretInfo->y == y)
        return TRUE;

    if (pWin->pCaretInfo->fBlink) {
        if (pWin->pCaretInfo->fShow) {
        
            HDC hdc;

            // hide caret first
            hdc = GetClientDC (hWnd);
#ifdef _USE_NEWGAL
            pWin->pCaretInfo->caret_bmp.bmBits = pWin->pCaretInfo->pNormal;
            FillBoxWithBitmap (hdc,
                        pWin->pCaretInfo->x, pWin->pCaretInfo->y, 0, 0, 
                        &pWin->pCaretInfo->caret_bmp);
#else
            PutSavedBoxOnDC (hdc, 
                        pWin->pCaretInfo->x, pWin->pCaretInfo->y,
                        pWin->pCaretInfo->nEffWidth,
                        pWin->pCaretInfo->nEffHeight,
                        pWin->pCaretInfo->pNormal);
#endif /* _USE_NEWGAL */

            // then update position
            pWin->pCaretInfo->x = x;
            pWin->pCaretInfo->y = y;

            // save normal bitmap first
            GetCaretBitmaps (pWin->pCaretInfo);
            
            // show caret again
#ifdef _USE_NEWGAL
            pWin->pCaretInfo->caret_bmp.bmBits = pWin->pCaretInfo->pXored;
            FillBoxWithBitmap (hdc,
                        pWin->pCaretInfo->x, pWin->pCaretInfo->y, 0, 0, 
                        &pWin->pCaretInfo->caret_bmp);
#else
            PutSavedBoxOnDC (hdc, 
                        pWin->pCaretInfo->x, pWin->pCaretInfo->y,
                        pWin->pCaretInfo->nEffWidth,
                        pWin->pCaretInfo->nEffHeight,
                        pWin->pCaretInfo->pXored);
#endif /* _USE_NEWGAL */

            ReleaseDC (hdc);
        }
        else {
            HDC hdc;
        
            // update position
            pWin->pCaretInfo->x = x;
            pWin->pCaretInfo->y = y;

            // save normal bitmap first
            GetCaretBitmaps (pWin->pCaretInfo);

            // show caret
            hdc = GetClientDC (hWnd);
#ifdef _USE_NEWGAL
            pWin->pCaretInfo->caret_bmp.bmBits = pWin->pCaretInfo->pXored;
            FillBoxWithBitmap (hdc,
                        pWin->pCaretInfo->x, pWin->pCaretInfo->y, 0, 0, 
                        &pWin->pCaretInfo->caret_bmp);
#else
            PutSavedBoxOnDC (hdc, 
                        pWin->pCaretInfo->x, pWin->pCaretInfo->y,
                        pWin->pCaretInfo->nEffWidth,
                        pWin->pCaretInfo->nEffHeight,
                        pWin->pCaretInfo->pXored);
#endif /* _USE_NEWGAL */

            ReleaseDC (hdc);

            pWin->pCaretInfo->fShow = TRUE;
        }
    }
    else {
        // update position
        pWin->pCaretInfo->x = x;
        pWin->pCaretInfo->y = y;
    }

    return TRUE;
}

BOOL GUIAPI ChangeCaretSize (HWND hWnd, int newWidth, int newHeight)
{
    PMAINWIN pWin;

    pWin = (PMAINWIN)hWnd;

    if (!pWin->pCaretInfo)
        return FALSE;
    
#ifdef _USE_NEWGAL
    if (newWidth == pWin->pCaretInfo->caret_bmp.bmWidth
            && newHeight == pWin->pCaretInfo->caret_bmp.bmHeight)
        return TRUE;
#else
    if (newWidth == pWin->pCaretInfo->nEffWidth
            && newHeight == pWin->pCaretInfo->nEffHeight)
        return TRUE;
#endif /* _USE_NEWGAL */
        
    if (newWidth > pWin->pCaretInfo->nWidth
            || newHeight > pWin->pCaretInfo->nHeight
            || newWidth <= 0 || newHeight <= 0)
        return FALSE;

    if (pWin->pCaretInfo->fBlink) {
        if (pWin->pCaretInfo->fShow) {
        
            HDC hdc;

            // hide caret first
            hdc = GetClientDC (hWnd);
#ifdef _USE_NEWGAL
            pWin->pCaretInfo->caret_bmp.bmBits = pWin->pCaretInfo->pNormal;
            FillBoxWithBitmap (hdc,
                        pWin->pCaretInfo->x, pWin->pCaretInfo->y, 0, 0, 
                        &pWin->pCaretInfo->caret_bmp);
#else
            PutSavedBoxOnDC (hdc, 
                        pWin->pCaretInfo->x, pWin->pCaretInfo->y,
                        pWin->pCaretInfo->nEffWidth,
                        pWin->pCaretInfo->nEffHeight,
                        pWin->pCaretInfo->pNormal);
#endif /* _USE_NEWGAL */

            // then update size info
#ifdef _USE_NEWGAL
            pWin->pCaretInfo->caret_bmp.bmWidth = newWidth;
            pWin->pCaretInfo->caret_bmp.bmHeight= newHeight;
            pWin->pCaretInfo->nBytesNr = pWin->pCaretInfo->caret_bmp.bmPitch * newHeight;
#else
            pWin->pCaretInfo->nEffWidth = newWidth;
            pWin->pCaretInfo->nEffHeight = newHeight;
            pWin->pCaretInfo->nBytesNr = newWidth * newHeight * BYTESPERPHYPIXEL;
#endif /* _USE_NEWGAL */

            // save normal bitmap first
            GetCaretBitmaps (pWin->pCaretInfo);
            
            // show caret again
#ifdef _USE_NEWGAL
            pWin->pCaretInfo->caret_bmp.bmBits = pWin->pCaretInfo->pXored;
            FillBoxWithBitmap (hdc,
                        pWin->pCaretInfo->x, pWin->pCaretInfo->y, 0, 0, 
                        &pWin->pCaretInfo->caret_bmp);
#else
            PutSavedBoxOnDC (hdc, 
                        pWin->pCaretInfo->x, pWin->pCaretInfo->y,
                        pWin->pCaretInfo->nEffWidth,
                        pWin->pCaretInfo->nEffHeight,
                        pWin->pCaretInfo->pXored);
#endif /* _USE_NEWGAL */

            ReleaseDC (hdc);
        }
        else {
            HDC hdc;
        
            // then update size info
#ifdef _USE_NEWGAL
            pWin->pCaretInfo->caret_bmp.bmWidth = newWidth;
            pWin->pCaretInfo->caret_bmp.bmHeight= newHeight;
            pWin->pCaretInfo->nBytesNr = pWin->pCaretInfo->caret_bmp.bmPitch * newHeight;
#else
            pWin->pCaretInfo->nEffWidth = newWidth;
            pWin->pCaretInfo->nEffHeight = newHeight;
            pWin->pCaretInfo->nBytesNr = newWidth * newHeight * BYTESPERPHYPIXEL;
#endif /* _USE_NEWGAL */

            // save normal bitmap first
            GetCaretBitmaps (pWin->pCaretInfo);

            // show caret
            hdc = GetClientDC (hWnd);
#ifdef _USE_NEWGAL
            pWin->pCaretInfo->caret_bmp.bmBits = pWin->pCaretInfo->pXored;
            FillBoxWithBitmap (hdc,
                        pWin->pCaretInfo->x, pWin->pCaretInfo->y, 0, 0, 
                        &pWin->pCaretInfo->caret_bmp);
#else
            PutSavedBoxOnDC (hdc, 
                        pWin->pCaretInfo->x, pWin->pCaretInfo->y,
                        pWin->pCaretInfo->nEffWidth,
                        pWin->pCaretInfo->nEffHeight,
                        pWin->pCaretInfo->pXored);
#endif /* _USE_NEWGAL */

            ReleaseDC (hdc);

            pWin->pCaretInfo->fShow = TRUE;
        }
    }
    else {
        // update size info
#ifdef _USE_NEWGAL
        pWin->pCaretInfo->caret_bmp.bmWidth = newWidth;
        pWin->pCaretInfo->caret_bmp.bmHeight= newHeight;
        pWin->pCaretInfo->nBytesNr = pWin->pCaretInfo->caret_bmp.bmPitch * newHeight;
#else
        pWin->pCaretInfo->nEffWidth  = newWidth;
        pWin->pCaretInfo->nEffHeight = newHeight;
        pWin->pCaretInfo->nBytesNr = newWidth * newHeight * BYTESPERPHYPIXEL;
#endif
    }

    return TRUE;
}

BOOL GUIAPI GetCaretPos (HWND hWnd, PPOINT pPt)
{
    PMAINWIN pWin;

    pWin = (PMAINWIN)hWnd;

    if (!pWin->pCaretInfo)
        return FALSE;

    pPt->x = pWin->pCaretInfo->x;
    pPt->y = pWin->pCaretInfo->y;

    return TRUE;
}


