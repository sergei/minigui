/*
**  $Id: win_fb.c 7355 2007-08-16 05:03:05Z xgwang $
**  
**  win_fb.c: A subdriver of shadow NEWGAL engine for Windows WVFB 4bpp/1bpp.
**
**  Copyring (C) 2003 ~ 2007 Feynman Software.
*/

#include <stdio.h>
#include <stdlib.h>

#include "mgconfig.h"

#if defined (_NEWGAL_ENGINE_SHADOW) && defined (WIN32) && defined (__TARGET_WVFB__)

#include "windows.h"

static HANDLE hMutex;
static HANDLE hScreen;
static LPVOID lpScreen;

int wvfb_shadow_available (void)
{
	hMutex = OpenMutex (MUTEX_MODIFY_STATE, FALSE, "WVFBScreenObject");
	if (hMutex == 0 || hMutex == (HANDLE)ERROR_FILE_NOT_FOUND) {
	    fprintf (stderr, "WVFB is not available!\n");
		return 0;
	}

	return 1;
}

void *wvfb_shadow_init ()
{
	hScreen = OpenFileMapping (FILE_MAP_ALL_ACCESS, FALSE, "WVFBScreenMap");

	if (hScreen == NULL) {
	    fprintf (stderr, "Could not open file mapping object WVFBScreenMap.");
		return NULL;
	}

	lpScreen = MapViewOfFile (hScreen, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (lpScreen == NULL) {
	    fprintf (stderr, "Could not map view of file.");
		return NULL;
	}

	return (void *)lpScreen;
}

void wvfb_shadow_close (void)
{
	UnmapViewOfFile (lpScreen);
	CloseHandle (hScreen);
	CloseHandle (hMutex);
}

void wvfb_shadow_lock (void)
{
	WaitForSingleObject (hMutex, INFINITE);
}

void wvfb_shadow_unlock (void)
{
	ReleaseMutex (hMutex);
}

#endif /* _NEWGAL_ENGINE_SHADOW && WIN32 && __TARGET_WVFB__ */

