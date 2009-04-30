#ifndef _SYSRES_H
#define _SYSRES_H

#ifndef CURSORSECTION
#define CURSORSECTION    "cursorinfo"
#endif

#ifdef _INCORE_RES

typedef struct _SYSRES
{
    const char *name;      /* name of the resource */
    void*      res_data;   /* memory address of the resource */
    size_t     data_len;   /* item data len */
    int        flag;       /* flag = 0 means each item have a equal size */
} SYSRES;

#endif /* _INCORE_RES */

PCURSOR LoadSystemCursor (int i);

#endif /* _SYSRES_H */
