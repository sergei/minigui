TOP_DIR=../../..
INCLUDES=-I../../../include -I../../include -I../

SRC_FILES = shadow.c unknown.c threadx_anvil.c nucleus_monaco.c \
		linux_qvfb.c linux_fbcon.c win_wvfb.c win_fb.c ecos_generic.c
HDR_FILES = shadow.h

noinst_LTLIBRARIES = libvideo_shadow.la
libvideo_shadow_la_SOURCES = $(SRC_FILES) $(HDR_FILES)

EXTRA_DIST=makefile.ng makefile.msvc

LIB_NAME=libvideo_shadow
