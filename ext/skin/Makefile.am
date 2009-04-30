TOP_DIR=../..
INCLUDES = -I./ -I../include -I../../include

noinst_LTLIBRARIES = libskin.la

SRC_FILES =  skin.c  \
	cmdbutton.c  chkbutton.c  label.c    bmplabel.c  slider.c   rotslider.c   mgcontrol.c

HDR_FILES = item_comm.h  \
	cmdbutton.h  chkbutton.h  label.h    bmplabel.h  slider.h   rotslider.h   mgcontrol.h

AM_CFLAGS = -D__MGEXT_LIB__

libskin_la_SOURCES = $(SRC_FILES) $(HDR_FILES)

LIB_NAME=libskin
