/*
** $Id: foo.c 7376 2007-08-16 05:40:27Z xgwang $
** 
** Copyright (C) 2005 ~ 2007 Feynman Software.
**
** Current maintainer: Wei Yongming
**
** Create date: 2005-01-05 by Wei Yongming
*/

/*
**  This program is free software; you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation; either version 2 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program; if not, write to the Free Software
**  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "common.h"
#include "minigui.h"
#include "gdi.h"
#include "window.h"

#include "vcongui.h"

void* VCOnMiniGUI (void* data)
{
    return NULL;
}

#ifndef _LITE_VERSION
void* NewVirtualConsole (PCHILDINFO pChildInfo)
{
    return NULL;
}
#endif

