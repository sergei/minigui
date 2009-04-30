/*
** $Id: error.c 7376 2007-08-16 05:40:27Z xgwang $
**
** KON2 - Kanji ON Console -
** Copyright (C) 1992-1996 Takashi MANABE (manabe@papilio.tutics.tut.ac.jp)
**
** CCE - Console Chinese Environment -
** Copyright (C) 1998-1999 Rui He (herui@cs.duke.edu)
**
** VCOnGUI - Virtual Console On MiniGUi -
** Copyright (c) 1999-2002 Wei Yongming (ymwei@minigui.org)
** Copyright (C) 2003-2007 Feynman Software
**
*/

/*
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>

#include "common.h"
#include "minigui.h"
#include "gdi.h"
#include "window.h"
#include "control.h"

#include "defs.h"
#include "vcongui.h"
#include "vc.h"
#include "child.h"
#include "terminal.h"
#include "error.h"

static void VCOnGUIPrintf (const char *head, const char *format, va_list args)
{
#if 0
    FILE *fp = fopen ("/tmp/VCOnGUI-log","a");
    fprintf (fp, "%s", head);
    vfprintf (fp, format, args);
    fclose (fp);
#endif

    fprintf (stderr, "%s", head);
    vfprintf (stderr, format, args);
}

void MyFatal (const char *format, ...)
{
    va_list args;

    va_start (args, format);
    fprintf (stderr, "VCOnGUI> Fatal error: ");
    vfprintf (stderr, format, args);
    va_end (args);
    exit (EXIT_FAILURE);
}

void myWarn (const char *format, ...)
{
    va_list args;

    va_start(args, format);
    VCOnGUIPrintf ("VCOnGUI> Warning: ", format, args);
    va_end(args);
}

void myError (const char *format, ...)
{
    va_list args;

    va_start(args, format);
    VCOnGUIPrintf ("VCOnGUI> Error: ", format, args);
    va_end (args);
}

void myMessage (const char *format, ...)
{
    va_list args;

    va_start (args, format);
    VCOnGUIPrintf ("VCOnGUI> ", format, args);
    va_end (args);
}

void Perror (const char *msg)
{
    myMessage ("system error - %s: %s\r\n", msg, strerror(errno));
}

void PerrorExit (const char *message)
{
    fprintf (stderr, "%s: %s\r\n", message, strerror(errno));
    exit (0);
}

