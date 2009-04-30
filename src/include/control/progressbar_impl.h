/*
** $Id: progressbar_impl.h 7338 2007-08-16 03:46:09Z xgwang $
**
** prograssbar.h: the head file of PrograssBar control.
**
** Copyright (c) 2003 ~ 2007 Feynman Software.
** Copyright (c) 1999 ~ 2002 Wei Yongming.
**
** Create date: 1999/8/29
*/

#ifndef __PROGRESSBAR_IMPL_H_
#define __PROGRESSBAR_IMPL_H_

#ifdef  __cplusplus
extern  "C" {
#endif

typedef  struct tagPROGRESSDATA
{
    int nMin;
    int nMax;
    int nPos;
    int nStepInc;
}PROGRESSDATA;
typedef PROGRESSDATA* PPROGRESSDATA;

BOOL RegisterProgressBarControl (void);
void ProgressBarControlCleanup (void);

#ifdef  __cplusplus
}
#endif

#endif // __PROGRESSBAR_IMPL_H_

