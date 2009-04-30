## Makefile.am for the NEWGAL library

TOP_DIR=../..
INCLUDES = -I../../include -I../include 

noinst_LTLIBRARIES = libnewgal.la

# Define which subdirectories need to be built
SUBDIRS = @VIDEO_SUBDIRS@
DIST_SUBDIRS = dummy fbcon qvfb commlcd shadow

DRIVERS = @VIDEO_DRIVERS@

# Include the architecture-independent sources
COMMON_SRCS =       \
    RLEaccel.c      \
    RLEaccel_c.h    \
    blit.c          \
    blit.h          \
    blit_0.c        \
    blit_1.c        \
    blit_A.c        \
    blit_A.h        \
    blit_N.c        \
    gamma.c         \
    leaks.h         \
    pixels.c        \
    pixels_c.h      \
    surface.c       \
    stretch.c       \
    stretch_c.h     \
    sysvideo.h      \
    video.c         \
    yuv.c           \
    yuvfuncs.h      \
    yuv_sw.c        \
    yuv_sw_c.h      \
    yuv_mmx.c       \
    newgal.c

libnewgal_la_SOURCES = $(COMMON_SRCS)
libnewgal_la_LIBADD = $(DRIVERS)
libnewgal_la_DEPENDENCIES = $(DRIVERS)

