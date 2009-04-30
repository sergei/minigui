/*
** $Id: blockheap.c 7339 2007-08-16 03:47:29Z xgwang $
**
** blockheap.c: The heap of block data.
**
** Copyright (C) 2003 ~ 2007 Feynman Software.
** Copyright (C) 1999 ~ 2002 Wei Yongming.
**
** All rights reserved by Feynman Software.
**
** Current maintainer: Wei Yongming.
**
** Create date: 2001/01/10
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "common.h"
#include "minigui.h"
#include "gdi.h"
#include "blockheap.h"

void InitBlockDataHeap (PBLOCKHEAP heap, size_t bd_size, size_t heap_size)
{
#ifndef _LITE_VERSION
    pthread_mutex_init (&heap->lock, NULL);
#endif

    heap->heap = NULL;
    heap->bd_size = bd_size + sizeof (DWORD);
    heap->heap_size = heap_size;
}

void* BlockDataAlloc (PBLOCKHEAP heap)
{
    int i;
    char* block_data = NULL;

#ifndef _LITE_VERSION
    pthread_mutex_lock (&heap->lock);
#endif

    if (heap->heap == NULL) {
        if (!(heap->heap = calloc (heap->heap_size, heap->bd_size)))
            goto ret;
        heap->free = 0;
    }

    block_data = (char*) heap->heap + heap->bd_size*heap->free;
    for (i = heap->free; i < heap->heap_size; i++) {

        if (*((DWORD*)block_data) == BDS_FREE) {

            heap->free = i + 1;
            *((DWORD*)block_data) = BDS_USED;

#if 0
            fprintf (stderr, "heap: %p, Allocated: %d, free: %d.\n", heap, i, heap->free);
            fprintf (stderr, "Heap: (bd_size: %d, heap_size: %d, heap: %p).\n", 
                    heap->bd_size, heap->heap_size, heap->heap);
#endif
            goto ret;
        }

        block_data += heap->bd_size;
    }

    if (!(block_data = calloc (1, heap->bd_size)))
        goto ret;

    *((DWORD*)block_data) = BDS_SPECIAL;

ret:

#ifndef _LITE_VERSION
    pthread_mutex_unlock (&heap->lock);
#endif

    if (block_data)
        return block_data + sizeof (DWORD);
    return NULL;
}

void BlockDataFree (PBLOCKHEAP heap, void* data)
{
    int i;
    char* block_data;

#ifndef _LITE_VERSION
    pthread_mutex_lock (&heap->lock);
#endif

    block_data = (char*) data - sizeof (DWORD);
    if (*((DWORD*)block_data) == BDS_SPECIAL)
        free (block_data);
    else if (*((DWORD*)block_data) == BDS_USED) {
        *((DWORD*)block_data) = BDS_FREE;

        i = (block_data - (char*)heap->heap)/heap->bd_size;
        if (heap->free > i)
            heap->free = i;
        
#if 0
        fprintf (stderr, "Heap: %p: Freed: %d, free: %d.\n", heap, i, heap->free);
        fprintf (stderr, "Heap: (bd_size: %d, heap_size: %d, heap: %p).\n", 
                    heap->bd_size, heap->heap_size, heap->heap);
#endif
    }

#ifndef _LITE_VERSION
    pthread_mutex_unlock (&heap->lock);
#endif
}

void DestroyBlockDataHeap (PBLOCKHEAP heap)
{
#if 0
    fprintf (stderr, "Heap: (bd_size: %d, heap_size: %d, heap: %p).\n", 
                    heap->bd_size, heap->heap_size, heap->heap);
#endif
#ifndef _LITE_VERSION
    pthread_mutex_destroy (&heap->lock);
#endif
    free (heap->heap);
}

