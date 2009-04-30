/*
** $Id: button_impl.h 7338 2007-08-16 03:46:09Z xgwang $
**
** button.h: the head file of Button Control module.
**
** Copyright (C) 2003 ~ 2007 Feynman Software.
** Copyright (C) 1999 ~ 2002 Wei Yongming.
**
** Note:
**   Originally by Zhao Jianghua. 
**
** Create date: 1999/8/23
*/


#ifndef __BUTTON_IMPL_H__
#define __BUTTON_IMPL_H__

#ifdef  __cplusplus
extern  "C" {
#endif

typedef struct tagBUTTONDATA
{
    DWORD status;           /* button flags */
    DWORD data;             /* bitmap or icon of butotn. */
} BUTTONDATA;
typedef BUTTONDATA* PBUTTONDATA;   

BOOL RegisterButtonControl (void);
void ButtonControlCleanup (void);

#ifdef  __cplusplus
}
#endif

#endif  // __BUTTON_IMPL_H__

