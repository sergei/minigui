
## Makefile.am for the null video driver

TOP_DIR=../../..
INCLUDES=-I../../../include -I../../include -I../

noinst_LTLIBRARIES = libvideo_null.la

# The SDL null video driver sources
SRC_FILES =             \
    nullvideo.c

HDR_FILES= \
    nullvideo.h

libvideo_null_la_SOURCES = $(SRC_FILES) $(HDR_FILES)

EXTRA_DIST=makefile.ng makefile.msvc

LIB_NAME=libvideo_null
