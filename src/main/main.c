/*
** $Id: main.c 7341 2007-08-16 03:52:26Z xgwang $
**
** main.c: The main function wrapper.
**
** Copyright (C) 2003 ~ 2007 Feynman Software.
** Copyright (C) 1999 ~ 2002 Wei Yongming.
**
** Current maintainer: Wei Yongming.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "minigui.h"
#include "gal.h"
#include "ial.h"


#ifdef _LITE_VERSION
BOOL mgIsServer = FALSE;
#endif

void GUIAPI MiniGUIPanic (int exitcode)
{
    exitcode = 1;
#ifdef _LITE_VERSION
    if (mgIsServer)
#endif
    {
        TerminateIAL ();
    } 

    TerminateGAL ();

#ifndef __NOUNIX__
    _exit (exitcode);
#endif
}

