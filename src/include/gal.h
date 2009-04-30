/*
** $Id: gal.h 7337 2007-08-16 03:44:29Z xgwang $
**
** gal.h: the head file of Graphics Abstract Layer
**
** Copyright (C) 2003 ~ 2007 Feynman Software.
** Copyright (C) 2001 ~ 2002 Wei Yongming
**
** Create date: 2001/10/07
*/

#ifndef GUI_GAL_H
    #define GUI_GAL_H

#ifdef _LITE_VERSION

extern BOOL __mg_switch_away; // always be zero for clients.

#ifndef _STAND_ALONE
extern GHANDLE __mg_layer;

void unlock_draw_sem (void);
void lock_draw_sem (void);
#endif

#endif

#ifdef _USE_NEWGAL
    #include "newgal.h"
#else
    #include "oldgal.h"
#endif

#ifdef _COOR_TRANS
  #define WIDTHOFPHYSCREEN      HEIGHTOFPHYGC
  #define HEIGHTOFPHYSCREEN     WIDTHOFPHYGC
#else
  #define WIDTHOFPHYSCREEN      WIDTHOFPHYGC
  #define HEIGHTOFPHYSCREEN     HEIGHTOFPHYGC
#endif

#endif  /* GUI_GAL_H */

