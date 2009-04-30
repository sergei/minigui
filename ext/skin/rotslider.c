/*
** $Id: rotslider.c 7372 2007-08-16 05:33:55Z xgwang $
**
** rotslider.c: skin rotation slider implementation.
**
** Copyright (C) 2003 ~ 2007 Feynman Software.
**
** Current maintainer: Allen
**
** Create date: 2003-10-21 by Allen
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef __MINIGUI_LIB__
	#include "common.h"
	#include "minigui.h"
	#include "gdi.h"
	#include "window.h"
	#include "control.h"
	#include "mgext.h"
	#include "fixedmath.h"
#else
	#include <minigui/common.h>
	#include <minigui/minigui.h>
	#include <minigui/gdi.h>
	#include <minigui/window.h>
	#include <minigui/control.h>
	#include <minigui/mgext.h>
	#include <minigui/fixedmath.h>
#endif

#if defined(_EXT_SKIN) && defined(_FIXED_MATH)

#include "skin.h"
#include "item_comm.h"

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif

#define DECLARE_VOID(type, var) \
    type *var = (type *) item->type_data; \
	if (!var) return;
#define DECLARE(type, var, ret) \
    type *var = (type *) item->type_data; \
	if (!var) return ret;

/* make degree belong [0, 360) */
static void adjust_degree ( int *degree )
{
	while ( *degree < 0 ){
		*degree += 360;
	};
	*degree %= 360;
}

/* wether degree between start_deg and end_deg (both belong [0, 360)) */
/* according to direction (clock or anitclock) */
static BOOL degree_between ( int degree, int start_deg, int end_deg, BOOL clock )
{
	adjust_degree ( & degree	);
	adjust_degree ( & start_deg	);
	adjust_degree ( & end_deg	);

	if ( clock ){	/* clock direction */
		if ( end_deg > start_deg )	end_deg -= 360;
		if ( degree  > start_deg )	degree  -= 360;
		return ( degree >= end_deg );
	}
	else{			/* anticlock direc */
		if ( end_deg < start_deg )	end_deg += 360;
		if ( degree  < start_deg )	degree  += 360;
		return ( degree <= end_deg );
	}
}


static DWORD rotslider_get_pos (skin_item_t *item) 
{
	DECLARE (si_rotslider_t, slider, -1);

    return slider->cur_pos;
}

static DWORD rotslider_set_pos (skin_item_t *item, DWORD pos) 
{
	DWORD old_pos;
	DECLARE (si_rotslider_t, slider, -1);

	old_pos = slider->cur_pos;
	slider->cur_pos = pos;
	if ( !degree_between(pos, slider->start_deg, slider->end_deg, item->style&SI_ROTSLIDER_CW) )
		slider->cur_pos = slider->start_deg;
	return old_pos;
}

static void rotslider_draw_bg (HDC hdc, skin_item_t *item)
{
    FillBoxWithBitmap (hdc, item->x, item->y, 0, 0, &ITEMBMP(item));
}

#define CAL_GOU(r, d)       \
        fixtoi (fixmul (itofix(r), fixcos (ftofix(d))))

#define CAL_GU(r, d)       \
        fixtoi (fixmul (itofix(r), fixsin (ftofix(d))))

static void rotslider_draw_attached (HDC hdc, skin_item_t* item)
{
	int xo = 0, degree;
	const BITMAP *bmp;
	DECLARE_VOID (si_rotslider_t, slider);

	rotslider_set_pos (item, (DWORD)slider->cur_pos); 

	if ( item->style & SI_STATUS_CLICKED )
		xo = 1;
	else if ( item->style & SI_STATUS_HILIGHTED )
		xo = 2;

	bmp = &BMP(item, slider->thumb_bmp_index ); 
	degree = slider->cur_pos;
	adjust_degree ( &degree );
	FillBoxWithBitmapPart (hdc, 
		(item->rc_hittest.left + item->rc_hittest.right)/2 + 
			CAL_GOU (slider->radius, (degree*M_PI/180)) - bmp->bmWidth/3/2, 
		(item->rc_hittest.top+ item->rc_hittest.bottom)/2 - 
			CAL_GU (slider->radius, (degree*M_PI/180)) -bmp->bmHeight/2, 
		bmp->bmWidth/3, bmp->bmHeight, 0, 0, 
		bmp, xo * bmp->bmWidth / 3, 0);
}

static int get_changed_pos (skin_item_t* item, int x, int y)
{
    int cx, cy, degree;
	fixed radius;
	fixed radian_acos, radian;
	DECLARE (si_rotslider_t, slider, -1);

	cx = (item->shape.left + item->shape.right) / 2;
	cy = (item->shape.top + item->shape.bottom) / 2;
	radius = fixhypot ( itofix (cx - x), itofix (cy - y));
	radian_acos = fixacos (fixdiv (itofix (x - cx), radius));
	radian = cy < y ? (fixsub (ftofix (2 * M_PI), radian_acos)) : radian_acos;
	degree = ((int)(fixtof (fixmul (radian, itofix (180))) / M_PI + 0.5) + 360) % 360;

	/* we've got current degree which belong [0, 360) */
	if ( !degree_between(degree, slider->start_deg, slider->end_deg, item->style&SI_ROTSLIDER_CW) )
		degree = slider->cur_pos;

	return degree;
}

static int rotslider_msg_proc (skin_item_t* item, int message, WPARAM wparam, LPARAM lparam)
{
	int pos = 0;
	
	if ( item->style & SI_ROTSLIDER_STATIC )
		return 0;
	
	switch (message) {
	case SKIN_MSG_LBUTTONDOWN:
		skin_set_check_status ( item->hostskin, item->id, TRUE );
	case SKIN_MSG_MOUSEDRAG:     /* ROTSLIDER_CHANGED event */
		/* calculate new degree [start_deg, end_deg] belong to [0,2*Pi) */
		pos = get_changed_pos (item, wparam, lparam);
		RAISE_EVENT ( SIE_SLIDER_CHANGED, (void *)pos );
		/* default operation */
		skin_set_thumb_pos (item->hostskin, item->id, pos);
		break;
	case SKIN_MSG_LBUTTONUP:
		skin_set_check_status ( item->hostskin, item->id, FALSE );
		break;
	case SKIN_MSG_SETFOCUS:
		RAISE_EVENT ( SIE_GAIN_FOCUS, NULL );
		break;
	case SKIN_MSG_KILLFOCUS:
		RAISE_EVENT ( SIE_LOST_FOCUS, NULL );
		break;
	}
	return 1;
}

static skin_item_ops_t rotslider_ops = {
    NULL,
    NULL,
    NULL,
    NULL,
    rotslider_draw_bg,
    rotslider_draw_attached,
    rotslider_get_pos,
    rotslider_set_pos,
    rotslider_msg_proc
};

skin_item_ops_t *ROTSLIDER_OPS = &rotslider_ops;

#endif /* _EXT_SKIN && _FIXED_MATH */
