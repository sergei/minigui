/***************************************************************************
 *            ial_tomtom_ts.c
 *
 *  Fri Jan  4 18:42:38 2008
 *  Copyright  2008  nullpointer
 *  Email nullpointer[at]lavabit[dot]com
 ****************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

#ifdef _TOMTOM_TS_IAL

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/kd.h>
#include <errno.h>

#include "ial.h"
#include "ial_tomtom_ts.h"



typedef struct {
	short pressure;
	short x;
	short y;
	short state;
} TS_EVENT;

typedef struct {
	int min1;
	int max1;
	int min2;
	int max2;
} TS_CAL;

typedef struct {
	long An;
	long Bn;
	long Cn;
	long Dn;
	long En;
	long Fn;
	long Divider;
	int xMin;
	int xMax;
	int yMin;
	int yMax;
} TS_MATRIX;

#define TS_DRIVER_MAGIC	'f'
#define TS_SET_RAW_OFF	_IO(TS_DRIVER_MAGIC, 15)
#define TS_SET_CAL		_IOW(TS_DRIVER_MAGIC, 11, TS_MATRIX)

static int fd = -1;

typedef enum  {
	dpy240x320x565,		// Classic, 300, 500, 700
	dpy320x240x565,		// ONE, RAIDER
	dpy480x272x565,		// XL
} DisplayType;

static DisplayType displayType;
static BOOL rotatedTouchScreenCalib = FALSE;


static int mousex = 0;
static int mousey = 0;
static int stylus = 0;

static inline int get_middle (int x, int y, int z)
{
    return ((x + y + z) / 3);
}

static int mouse_update (void)
{
    TS_EVENT cBuffer;

	if( read (fd, &cBuffer, sizeof (TS_EVENT)) > 0){
		cBuffer.y = 239 - cBuffer.y;
		mousex = cBuffer.x;
		mousey = cBuffer.y;

        if (cBuffer.pressure==255)
			stylus = IAL_MOUSE_LEFTBUTTON;
        else
			stylus = 0;
    }

    return 1;
}

#ifdef _LITE_VERSION
static int wait_event (int which, int maxfd, fd_set *in, fd_set *out, fd_set *except,
                struct timeval *timeout)
#else
static int wait_event (int which, fd_set *in, fd_set *out, fd_set *except,
                struct timeval *timeout)
#endif
{
    fd_set rfds;
    int    retvalue = 0;
    int    e;

    if (!in) {
        in = &rfds;
        FD_ZERO (in);
    }

    if ((which & IAL_MOUSEEVENT) && fd >= 0) {
        FD_SET (fd, in);
#ifdef _LITE_VERSION
        if (fd > maxfd) maxfd = fd;
#endif
    }

#ifdef _LITE_VERSION
    e = select (maxfd + 1, in, out, except, timeout) ;
#else
    e = select (FD_SETSIZE, in, out, except, timeout) ;
#endif

    if (e > 0) {
        if (fd >= 0 && FD_ISSET (fd, in)) {
            FD_CLR (fd, in);
            retvalue |= IAL_MOUSEEVENT;
        }
    }
    else if (e < 0) {
        return -1;
    }

    return retvalue;
}


#if 0
#ifdef _LITE_VERSION
static int wait_event (int which, int maxfd, fd_set *in, fd_set *out, fd_set *except,
                struct timeval *timeout)
#else
static int wait_event (int which, fd_set *in, fd_set *out, fd_set *except,
                struct timeval *timeout)
#endif
{
    int retvalue = 0;
    int e;

#ifdef _LITE_VERSION
    e = select (maxfd + 1, in, out, except, timeout);
#else
    e = select (FD_SETSIZE, in, out, except, timeout);
#endif

    if (e < 0) {
        return -1;
    }
    else if ((which & IAL_MOUSEEVENT) && fd >= 0) {
        retvalue |= IAL_MOUSEEVENT;
    }

    return retvalue;
}
#endif /* 0 */

static int mouse_getbutton (void)
{
    return stylus;
}

static void mouse_getxy (int *x, int* y)
{
    *x = mousex;
    *y = mousey;
}


static BOOL GetPlatformInfo( void )
{
	char str[32];
	int fd;
	int i;

	if((fd = open("/proc/barcelona/modelname", O_RDONLY)) < 0) {
		fprintf( stderr, "could not open /proc/barcelona/modelname: %s", strerror(errno));
		return FALSE;
	}
	if((i = read(fd, str, sizeof(str) - 1)) < 0) {
		fprintf( stderr, "could not read from /proc/barcelona/modelname");
		close(fd);
		return FALSE;
	}
	close(fd);
	str[i] = '\0';
	if((i > 0) && (str[i - 1] == '\n'))
		str[i - 1] = '\0';

	fprintf( stdout, "TomTom model shortname: [%s]", str);

	if(strcmp(str, "GO") == 0) {
		// GO Classic
		displayType = dpy240x320x565;
		rotatedTouchScreenCalib = TRUE;
	} else if(strcmp(str, "GO 300") == 0) {
		// GO 300
		displayType = dpy240x320x565;
		rotatedTouchScreenCalib = TRUE;
	} else if(strcmp(str, "GO 500") == 0) {
		// GO 500
		displayType = dpy240x320x565;
		rotatedTouchScreenCalib = TRUE;
	} else if(strcmp(str, "GO 700") == 0) {
		// GO 700
		displayType = dpy240x320x565;
		rotatedTouchScreenCalib = TRUE;
	} else if(strcmp(str, "ONE") == 0) {
		// ONE
		displayType = dpy320x240x565;
		rotatedTouchScreenCalib = FALSE;
	} else if(strcmp(str, "TomTom ONE XL") == 0) {
		// Limerick
		displayType = dpy480x272x565;
		rotatedTouchScreenCalib = FALSE;
	} else if(strcmp(str, "RAIDER") == 0) {
		// RAIDER
		displayType = dpy320x240x565;
		rotatedTouchScreenCalib = FALSE;
	} else {
		// unknown - handle like a ONE
		displayType = dpy320x240x565;
		rotatedTouchScreenCalib = TRUE;
	}

	return TRUE;
}

BOOL InitTomtomTSInput (INPUT* input, const char* mdev, const char* mtype)
{

	TS_CAL tscal;
	TS_MATRIX tsmatrix;
	TS_EVENT tsevent;

	long flag;


	if(GetPlatformInfo() == FALSE) {
		fprintf(stderr, "Error while getting platform info\n" );
		return FALSE;
	}


	/*
	* Calibrate the touch-panel device
	*/

	tsmatrix.An = -363;
	tsmatrix.Bn = 0;
	tsmatrix.Cn = 360416;
	tsmatrix.Dn = 0;
	tsmatrix.En = 258;
	tsmatrix.Fn = -12676;
	tsmatrix.Divider = 1000;
	if((fd = open("/mnt/flash/sysfile/cal", O_RDONLY)) < 0) {
		fprintf (stderr, "could not open touchscreen calibration file");
		tsmatrix.xMin = 0;
		tsmatrix.xMax = 1023;
		tsmatrix.yMin = 0;
		tsmatrix.yMax = 1023;
	} else {
		read(fd, &tscal, sizeof(tscal));
		close(fd);
		if(rotatedTouchScreenCalib == TRUE) {
			tsmatrix.xMin = tscal.min2;
			tsmatrix.xMax = tscal.max2;
			tsmatrix.yMin = tscal.min1;
			tsmatrix.yMax = tscal.max1;
		} else {
			tsmatrix.xMin = tscal.min1;
			tsmatrix.xMax = tscal.max1;
			tsmatrix.yMin = tscal.min2;
			tsmatrix.yMax = tscal.max2;
		}
		fprintf (stdout, "touchscreen calibration xmin:%d xmax:%d ymin:%d ymax:%d",
			tsmatrix.xMin, tsmatrix.xMax, tsmatrix.yMin, tsmatrix.xMax);
	}
	if((fd = open("/dev/ts", O_RDWR | O_NOCTTY | O_NONBLOCK)) < 0) {
		fprintf (stderr, "could not open touchscreen");
		fd = -1;
		return FALSE;
	}

	flag = fcntl(fd, F_GETFL, 0);
	flag |= O_NONBLOCK;
	fcntl(fd, F_SETFL, flag);

	ioctl(fd, TS_SET_CAL, &tsmatrix);
	ioctl(fd, TS_SET_RAW_OFF, NULL);

	// flush fifo
	while(read(fd, &tsevent, sizeof(tsevent)) > 0) ;

	input->update_mouse = mouse_update;
    input->get_mouse_xy = mouse_getxy;
    input->set_mouse_xy = NULL;
    input->get_mouse_button = mouse_getbutton;
    input->set_mouse_range = NULL;

    input->wait_event = wait_event;

    return TRUE;
}

void TermTomtomTSInput (void)
{
    if (fd >= 0)
        close (fd);
    fd = -1;
}

#endif /* _TOMTOM_TS_IAL */
