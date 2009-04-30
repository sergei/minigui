/*
** $Id: child.c 7376 2007-08-16 05:40:27Z xgwang $
**
** KON2 - Kanji ON Console -
** Copyright (C) 1992-1996 Takashi MANABE (manabe@papilio.tutics.tut.ac.jp)
**
** CCE - Console Chinese Environment -
** Copyright (C) 1998-1999 Rui He (herui@cs.duke.edu)
**
** VCOnGUI - Virtual Console On MiniGUi -
** Copyright (C) 1999-2002 Wei Yongming (ymwei@minigui.org)
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
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include "common.h"
#include "minigui.h"
#include "gdi.h"
#include "window.h"
#include "control.h"

#include "vcongui.h"
#include "defs.h"
#include "error.h"
#include "vc.h"
#include "child.h"

static void RunStartupCmd (const char* startupStr)
{
    char *p;
    char *str;

    if (!startupStr)
        return;

    str = strdup (startupStr);
    
    p = strtok (str, "\n");
    while(p) {
        system(p);
        p = strtok(NULL, "\n");
    }

    free (str);
}

void ChildStart (PCONINFO con, FILE *errfp, bool startupMessage, 
                    const char* startupStr, const char* execProg, 
                    const char* execArgs)
{
    char *tail;
    char buff [80];

    setgid (getgid ());
    setuid (getuid ());

    RunStartupCmd (startupStr);

//    sprintf (buff, "TERMCAP=:co#%d:li#%d:", con->cols, con->rows);
//    tcap = strdup (buff);
//    putenv (tcap);

    if (startupMessage)
    {
        printf("\rVCOnGUI - Virtual Console On MiniGUI " VCONGUI_VERSION
           " running on VC No. %c\r\n"
           "%s\r\n"
           "%s\r\n"
           "%s\r\n\r\n",
           *(ttyname (fileno (errfp)) + 8),
"Copyright (C) 1999-2001 Wei Yongming (ymwei@minigui.org).",
"Some idea comes from CCE by He Rui and others.",
"    CCE: Copyright (C) 1998-1999 He Rui and others.");
   }

    fflush(stdout);

    if (execProg)
        execlp (execProg, execProg, execArgs, NULL);
    else 
        {
        if ((execProg = getenv("SHELL")) == NULL)
            execProg = "/bin/sh";
        if ((tail = rindex(execProg, '/')) == NULL)
            tail = " sh";
        sprintf (buff, "-%s", tail + 1);
        execl (execProg, buff, 0);
    }
    
    fprintf (errfp, "VCOnGUI> couldn't exec shell\r\n");
    fprintf (errfp, "%s: %s\r\n", execProg, strerror(errno));
    exit (EXIT_FAILURE);
}

