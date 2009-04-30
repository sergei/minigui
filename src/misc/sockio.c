/*
** $Id: sockio.c 7342 2007-08-16 03:53:33Z xgwang $
**
** sockio.c: routines for socket i/o.
**
** Copyright (C) 2003 ~ 2007 Feynman Software.
** Copyright (C) 2002 Wei Yongming.
**
** All rights reserved by Feynman Software.
**
** Current maintainer: Wei Yongming.
**
** Create date: 2000/12/26
*/

#include <stdio.h>
#include <string.h>

#include "common.h"

#if defined(_LITE_VERSION) && !defined(_STAND_ALONE)

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>

#include "minigui.h"
#include "ourhdr.h"
#include "sharedres.h"
#include "sockio.h"

int sock_write_t (int fd, const void* buff, int count, unsigned int timeout)
{
    const void* pts = buff;
    int status = 0, n;
    unsigned int start_tick = SHAREDRES_TIMER_COUNTER;

    if (count < 0) return SOCKERR_OK;

    while (status != count) {
        n = write (fd, pts + status, count - status);
        if (n < 0) {
            if (errno == EPIPE)
                return SOCKERR_CLOSED;
            else if (errno == EINTR) {
                if (timeout && (SHAREDRES_TIMER_COUNTER 
                                        > start_tick + timeout)) {
                    return SOCKERR_TIMEOUT;
                }
                continue;
            }
            else
                return SOCKERR_IO;
        }
        status += n;
    }

    return status;
}

int sock_read_t (int fd, void* buff, int count, unsigned int timeout)
{
    void* pts = buff;
    int status = 0, n;
    unsigned int start_tick = SHAREDRES_TIMER_COUNTER;

    if (count <= 0) return SOCKERR_OK;

    while (status != count) {
        n = read (fd, pts + status, count - status);

        if (n < 0) {
            if (errno == EINTR) {
                if (timeout && (SHAREDRES_TIMER_COUNTER > start_tick + timeout))
                    return SOCKERR_TIMEOUT;
                continue;
            }
            else
                return SOCKERR_IO;
        }

        if (n == 0)
            return SOCKERR_CLOSED;

        status += n;
    }

    return status;
}

#endif /* _LITE_VERSION */
