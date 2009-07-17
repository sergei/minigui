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
#include <linux/input.h>

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
#define TS_SET_RAW_ON	_IO(TS_DRIVER_MAGIC, 14)
#define TS_SET_RAW_OFF	_IO(TS_DRIVER_MAGIC, 15)
#define TS_GET_CAL		_IOR(TS_DRIVER_MAGIC, 10, TS_MATRIX)
#define TS_SET_CAL		_IOW(TS_DRIVER_MAGIC, 11, TS_MATRIX)

static int fd = -1;

typedef enum  {
	dpy240x320x565,		// Classic, 300, 500, 700
	dpy320x240x565,		// ONE, RAIDER
	dpy480x272x565,		// XL
} DisplayType;

static DisplayType displayType;
static BOOL rotatedTouchScreenCalib = FALSE;
static 	TS_MATRIX ts_matrix;
static 	TS_MATRIX saved_tsmatrix;
int     eventMode = 0; // Use the /dev/input/event0 for TS

//#define USE_NATIVE_CAL 1

static int mousex = 0;
static int mousey = 0;
static int stylus = 0;
static int xres = 480;
static int yres = 272;

static char touch_screen_driver_name[80]="/dev/ts";

static inline int get_middle (int x, int y, int z)
{
    return ((x + y + z) / 3);
}


static int readTS (TS_EVENT *pev) {
  struct input_event ev2;

  pev->pressure = 0;
  pev->x = -1; // Reset...
  pev->y = -1;

  if (eventMode) {
     int retval = read(fd, &ev2, sizeof(struct input_event)) == sizeof(struct input_event) ? 1 : 0;
     while (retval) {
       //printf("ev2{value=%d type=%d code=%d}\n", ev2.value, ev2.type, ev2.code);
       if (ev2.type==EV_ABS && ev2.code==ABS_X) {
    	   pev->x = ev2.value;
    	   pev->x = (((pev->x - ts_matrix.xMin) * (xres-30)) / (ts_matrix.xMax - ts_matrix.xMin)) +15;
           if (pev->x < 0)
        	   pev->x = 0;
           else if ((unsigned)pev->x > xres)
              pev->x = xres;
           pev->x = xres - pev->x;
           //printf("pev->x=%d xres=%d ts_matrix{xMin=%d xMax=%d}\n",pev->x, xres, ts_matrix.xMin, ts_matrix.xMax);
           if (pev->y!=-1) // -> full dataset.
              return 1;
        } else if (ev2.type==EV_ABS && ev2.code==ABS_Y) {
        	pev->y = ev2.value;
        	pev->y = (((pev->y - ts_matrix.yMin) * (yres-30)) / (ts_matrix.yMax - ts_matrix.yMin)) +15;
           if (pev->y < 0)
        	   pev->y = 0;
           else if ((unsigned)pev->y > yres)
        	   pev->y = yres;
           pev->y = yres - pev->y;
           //printf("pev->y=%d yres=%d ts_matrix{yMin=%d yMax=%d}\n",pev->y, yres, ts_matrix.yMin, ts_matrix.yMax);

           if (pev->x!=-1) // -> full dataset.
              return 1;
       } else if (ev2.type==EV_KEY && ev2.code==BTN_TOUCH) {

		 if (ev2.value) {
			 pev->pressure = 255;
			 pev->x = -1; // Reset...
			 pev->y = -1;
			 } else {
				   pev->pressure = 0;
				   return 1; // button-release -> another full dataset.
			   }
			}
        retval = read(fd, &ev2, sizeof(struct input_event)) == sizeof(struct input_event) ? 1 : 0;
     }
     return retval;
  } else
     return read(fd, pev, sizeof(TS_EVENT)) == sizeof(TS_EVENT) ? 1 : 0;
}

static int flushTS(void) {
  int count=0;
  TS_EVENT ev;
  while (readTS(&ev))
     count++;
  return count;
}

static int mouse_update (void)
{
    TS_EVENT cBuffer;

    //	if( read (fd, &cBuffer, sizeof (TS_EVENT)) > 0){
    if( readTS( &cBuffer) > 0){
#ifndef USE_NATIVE_CAL
		if ( eventMode )
			cBuffer.y = cBuffer.y;
		else if ( displayType == dpy480x272x565)
			cBuffer.y = 271 - cBuffer.y;
		else
			cBuffer.y = 239 - cBuffer.y;
#endif
		mousex = cBuffer.x;
		mousey = cBuffer.y;

        if (cBuffer.pressure==255)
			stylus = IAL_MOUSE_LEFTBUTTON;
        else
			stylus = 0;

        //printf("mouse_update x=%d y=%d stylus=%d\n", mousex, mousey, stylus );
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
	} else if(strcmp(str, "TomTom GO 920") == 0) {
		// Milan
		displayType = dpy480x272x565;
		rotatedTouchScreenCalib = FALSE;
		strcpy(touch_screen_driver_name, "/dev/input/event0");
		eventMode = 1;
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
	//TS_EVENT tsevent;

	long flag;


	if(GetPlatformInfo() == FALSE) {
		fprintf(stderr, "Error while getting platform info\n" );
		return FALSE;
	}

#ifndef USE_NATIVE_CAL

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
		fprintf (stderr, "could not open touchscreen calibration file\n");
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
		fprintf (stdout, "new cal xmin:%d xmax:%d ymin:%d ymax:%d\n",
			tsmatrix.xMin, tsmatrix.xMax, tsmatrix.yMin, tsmatrix.xMax);
	}

	printf("Opening [%s] for TS driver...\n", touch_screen_driver_name);

	if((fd = open(touch_screen_driver_name, O_RDWR | O_NOCTTY | O_NONBLOCK)) < 0) {
		fprintf (stderr, "could not open touchscreen (%s)\n", strerror(errno));
		fd = -1;
		return FALSE;
	}

	flag = fcntl(fd, F_GETFL, 0);
	flag |= O_NONBLOCK;
	fcntl(fd, F_SETFL, flag);


	/* Store original calibration */
	printf("Storing TomTom calibration\n");
	ioctl(fd, TS_GET_CAL, &saved_tsmatrix);
	fprintf (stdout, "stored cal xmin:%d xmax:%d ymin:%d ymax:%d\n",
		tsmatrix.xMin, tsmatrix.xMax, tsmatrix.yMin, tsmatrix.xMax);

	ioctl(fd, TS_SET_CAL, &tsmatrix);
	ioctl(fd, TS_SET_RAW_OFF, NULL);
	ts_matrix = tsmatrix;
#endif // USE_NATIVE_CAL

	// flush fifo
	//while(read(fd, &tsevent, sizeof(tsevent)) > 0) ;
	flushTS();

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
    {
#ifndef USE_NATIVE_CAL
    	printf("Restoring TomTom calibration ...\n");
    	ioctl(fd, TS_SET_CAL, &saved_tsmatrix);
    	ioctl(fd, TS_SET_RAW_ON, NULL);
#endif // USE_NATIVE_CAL
        close (fd);
    }
    fd = -1;
}

#endif /* _TOMTOM_TS_IAL */
