/*
** $Id: button.h,v 1.2 2000/09
**
** toolbar.h:the head file of ToolBar control.
**
** Copyright (C) 2003 ~ 2007 Feynman Software.
** Copyright (C) 2000 ~ 2002 Wei Yongming.
**
** Create date: 2000/9/20
*/


#ifndef __TOOLBAR_IMPL_H_
#define __TOOLBAR_IMPL_H_

#ifdef  __cplusplus
extern  "C" {
#endif


typedef struct toolbarCTRL
{
	int    	nCount;
	int 	ItemWidth;
	int		ItemHeight;

    struct 	toolbarItemData* head;
    struct 	toolbarItemData* tail;

	int 	iSel; 				// selected selected pic's insPos
	int 	iLBDn; 				// whether LBtn down in the valid pic 0:no 1:yes
	int 	iMvOver;
 }TOOLBARCTRL;
typedef TOOLBARCTRL* PTOOLBARCTRL;

typedef struct toolbarItemData
{
      int         id;
      int         insPos;
      RECT        RcTitle;
      PBITMAP     NBmp;
      PBITMAP     HBmp;
      PBITMAP     DBmp;
      struct     toolbarItemData* next;
} TOOLBARITEMDATA;
typedef TOOLBARITEMDATA* PTOOLBARITEMDATA;

BOOL RegisterToolbarControl (void);
void ToolbarControlCleanup (void);

#ifdef  __cplusplus
}
#endif

#endif // GUI_TOOBAR_IMPL_H_


