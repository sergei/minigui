/*
** $Id: item_comm.h 7372 2007-08-16 05:33:55Z xgwang $
**
** item_comm.h: skin item common function header.
**
** Copyright (C) 2002 ~ 2007 Feynman Software, all rights reserved.
**
** Use of this source package is subject to specific license terms
** from Beijing Feynman Software Technology Co., Ltd.
**
** URL: http://www.minigui.com
*/

#ifndef _MGUI_ITEM_COMM_H
#define _MGUI_ITEM_COMM_H

#ifdef _EXT_SKIN

#define RAISE_EVENT(event, data)    \
if( item->hostskin->event_cb &&     \
	!item->hostskin->event_cb(item->hostskin->hwnd, item, event, data) ) \
	return 0;

#define BMP(item,idx)	item->hostskin->bmps[idx]
#define ITEMBMP(item)	item->hostskin->bmps[item->bmp_index]
#define ROUND(val, min, max)	(val = (val < min ? min : (val > max ? max: val)))
#define LEN_MAX(a,b)   (a > b ? a : b)

#endif /* _EXT_SKIN */

#endif /* _MGUI_ITEM_COMM_H*/
