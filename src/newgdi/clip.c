/*
** $Id: clip.c 7359 2007-08-16 05:08:40Z xgwang $
**
** clip.c: Clipping operations of GDI.
**
** Copyright (C) 2003 ~ 2007 Feynman Software
** Copyright (C) 2000 ~ 2002 Wei Yongming.
**
** Current maintainer: Wei Yongming.
**
** Create date: 2000/06/12, derived from original gdi.c
*/

#include "common.h"
#include "minigui.h"
#include "gdi.h"
#include "window.h"
#include "cliprect.h"
#include "gal.h"
#include "internals.h"
#include "ctrlclass.h"
#include "dc.h"

/************************* Clipping support **********************************/
#define RESET_LCRGN(pdc)\
    if (pdc->lcrgn.head == NULL) {\
        RECT my_rc = {0, 0, 65535, 65535};\
        SetClipRgn (&pdc->lcrgn, &my_rc);\
    }

void GUIAPI ExcludeClipRect (HDC hdc, const RECT* prc)
{
    PDC pdc;
    RECT rc;

    pdc = dc_HDC2PDC (hdc);

    rc = *prc;
    NormalizeRect (&rc);
    if (IsRectEmpty (&rc))
        return;

    if (dc_IsGeneralDC (pdc)) {
        RESET_LCRGN(pdc);
        SubtractClipRect (&pdc->lcrgn, &rc);

        // Transfer logical to device to screen here.
        coor_DP2SP (pdc, &rc.left, &rc.top);
        coor_DP2SP (pdc, &rc.right, &rc.bottom);
    }

    SubtractClipRect (&pdc->ecrgn, &rc);
}

void GUIAPI IncludeClipRect (HDC hdc, const RECT* prc)
{
    PDC pdc;
    RECT rc;

    pdc = dc_HDC2PDC(hdc);

    rc = *prc;
    NormalizeRect(&rc);
    if (IsRectEmpty (&rc))
        return;

    if (dc_IsGeneralDC (pdc)) {
        if (pdc->lcrgn.head)
            AddClipRect (&pdc->lcrgn, &rc);
        else
            SetClipRgn (&pdc->lcrgn, &rc);
    
        // Transfer logical to device to screen here.
        coor_DP2SP (pdc, &rc.left, &rc.top);
        coor_DP2SP (pdc, &rc.right, &rc.bottom);

        LOCK_GCRINFO (pdc);
        dc_GenerateECRgn (pdc, TRUE);
        UNLOCK_GCRINFO (pdc);
    }
    else {
        AddClipRect (&pdc->ecrgn, &rc);
    }
}

void GUIAPI ClipRectIntersect (HDC hdc, const RECT* prc)
{
    PDC pdc;
    RECT rc;

    pdc = dc_HDC2PDC(hdc);

    rc = *prc;
    NormalizeRect(&rc);
    if (IsRectEmpty (&rc))
        return;

    if (dc_IsGeneralDC (pdc)) {
        RESET_LCRGN(pdc);
        IntersectClipRect (&pdc->lcrgn, &rc);

        // Transfer logical to device to screen here.
        coor_DP2SP (pdc, &rc.left, &rc.top);
        coor_DP2SP (pdc, &rc.right, &rc.bottom);
    }

    IntersectClipRect (&pdc->ecrgn, &rc);
}

void GUIAPI SelectClipRect (HDC hdc, const RECT* prc)
{
    PDC pdc;
    RECT rc;

    pdc = dc_HDC2PDC(hdc);

    if (prc) {
        rc = *prc;
        NormalizeRect (&rc);
        if (IsRectEmpty (&rc))
            return;
    }
    else 
        rc = pdc->DevRC;

    if (dc_IsGeneralDC (pdc)) {
        if (prc)
            SetClipRgn (&pdc->lcrgn, &rc);
        else
            EmptyClipRgn (&pdc->lcrgn);

#ifdef _REGION_DEBUG
        fprintf (stderr, "\n----------------------------\n");
        dumpRegion (&pdc->ecrgn);
#endif

        /* for general DC, regenerate effective region. */
        LOCK_GCRINFO (pdc);
        dc_GenerateECRgn (pdc, TRUE);
        UNLOCK_GCRINFO (pdc);

#ifdef _REGION_DEBUG
        dumpRegion (&pdc->ecrgn);
        fprintf (stderr, "----------------------------\n");
#endif
    }
    else {
        if (IntersectRect (&rc, &rc, &pdc->DevRC))
            SetClipRgn (&pdc->ecrgn, &rc);
        else
            EmptyClipRgn (&pdc->ecrgn);
    }
}

void GUIAPI SelectClipRegion (HDC hdc, const CLIPRGN* pRgn)
{
    PDC pdc;

    pdc = dc_HDC2PDC (hdc);
    if (dc_IsGeneralDC (pdc)) {
        ClipRgnCopy (&pdc->lcrgn, pRgn);

        /* for general DC, regenerate effective region. */
        LOCK_GCRINFO (pdc);
        dc_GenerateECRgn (pdc, TRUE);
        UNLOCK_GCRINFO (pdc);
    }
    else {
        ClipRgnCopy (&pdc->ecrgn, pRgn);
        IntersectClipRect (&pdc->ecrgn, &pdc->DevRC);
    }
}

void GUIAPI GetBoundsRect (HDC hdc, RECT* pRect)
{
    PDC pdc;

    pdc = dc_HDC2PDC (hdc);

    if (dc_IsGeneralDC (pdc))
        *pRect = pdc->lcrgn.rcBound;
    else
        *pRect = pdc->ecrgn.rcBound;
}

BOOL GUIAPI PtVisible (HDC hdc, int x, int y)
{
    PDC pdc;

    pdc = dc_HDC2PDC(hdc);

    if (dc_IsGeneralDC (pdc)) {
        if (pdc->lcrgn.head == NULL)
            return TRUE;
        return PtInRegion (&pdc->lcrgn, x, y);
    }

    return PtInRegion (&pdc->ecrgn, x, y);
}

BOOL GUIAPI RectVisible (HDC hdc, const RECT* pRect)
{
    PDC pdc;

    pdc = dc_HDC2PDC(hdc);

    if (dc_IsGeneralDC (pdc)) {
        if (pdc->lcrgn.head == NULL)
            return TRUE;
        return RectInRegion (&pdc->lcrgn, pRect);
    }

    return RectInRegion (&pdc->ecrgn, pRect);
}

