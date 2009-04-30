/*
** $Id: chkbutton.c 8090 2007-11-16 04:52:02Z xwyan $
**
** button.c: skin check button implementation.
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

#ifdef __MINIGUI_LIB__
	#include "common.h"
	#include "minigui.h"
	#include "gdi.h"
	#include "window.h"
	#include "control.h"
	#include "mgext.h"
#else
	#include <minigui/common.h>
	#include <minigui/minigui.h>
	#include <minigui/gdi.h>
	#include <minigui/window.h>
	#include <minigui/control.h>
	#include <minigui/mgext.h>
#endif

#ifdef _EXT_SKIN

#include "skin.h"
#include "item_comm.h"

static void chkbutton_draw_bg (HDC hdc, skin_item_t *item)
{
	const BITMAP* bmp = &item->hostskin->bmps[item->bmp_index];
	int w = bmp->bmWidth / 4;
	int h = bmp->bmHeight;
	int style_no = 0;
	if ( item->style & SI_STATUS_HILIGHTED )
		style_no = 2;
	if ( item->style & SI_BTNSTATUS_CLICKED)
		style_no = 1;
	if ( item->style & SI_STATUS_DISABLED )
		style_no = 3;
	FillBoxWithBitmapPart (hdc,	item->x, item->y, w, h, 0, 0, bmp, style_no*w, 0);
}

static int chkbutton_msg_proc (skin_item_t* item, int message, WPARAM wparam, LPARAM lparam)
{
	static BOOL check = FALSE;
	/* assert (item != NULL) */
	switch (message) {
	case SKIN_MSG_LBUTTONDOWN:	/* click-down event */
		//RAISE_EVENT ( SIE_LBUTTONDOWN, NULL ); /*item msg not defined*/
		/* default operation */
		check = skin_get_check_status(item->hostskin,item->id);		/*store check status*/
		skin_set_check_status ( item->hostskin, item->id, TRUE );	/*button down*/
		break;
	case SKIN_MSG_LBUTTONUP:	/* click-up event */
		//RAISE_EVENT ( SIE_LBUTTONUP, NULL ); /*item msg not defined*/
		/* default operation */
		skin_set_check_status ( item->hostskin, item->id, check);	/*restore check status*/
		break;
	case SKIN_MSG_CLICK:		/* CLICK event */
		skin_set_check_status ( item->hostskin, item->id, !check );
		RAISE_EVENT ( SIE_BUTTON_CLICKED, NULL );
		/* default operation */
		/* switch check status */
		break;
	case SKIN_MSG_MOUSEDRAG:
		//RAISE_EVENT ( SIE_MOUSEDRAG, NULL );
		/* default operation */
		if ( PtInRegion (&item->region, (int)wparam, (int)lparam) ){
        	/* if mouse moves in, click-down item */
            if (!skin_get_check_status(item->hostskin, item->id))
                skin_set_check_status ( item->hostskin, item->id, TRUE);	/*button down*/
		}
		else{
        	/* if mouse moves out, click-up item */
            if (skin_get_check_status(item->hostskin, item->id))
                skin_set_check_status ( item->hostskin, item->id, FALSE);	/*button up*/
		}
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

static skin_item_ops_t chkbutton_ops = {
    NULL,
    NULL,
    NULL,
    NULL,
    chkbutton_draw_bg,
    NULL,
    NULL,
    NULL,
   	chkbutton_msg_proc
};

skin_item_ops_t *CHKBUTTON_OPS = &chkbutton_ops;

#endif /* _EXT_SKIN */
