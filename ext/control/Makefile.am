TOP_DIR=../..
INCLUDES = -I./ -I../include -I../../include -I../../src/include/control

noinst_LTLIBRARIES = libcontrol.la

SRC_FILES = \
	initmgext.c \
	treeview.c \
	monthcalendar.c \
    spinbox.c \
	coolbar.c \
	listview.c \
    gif89a.c \
	animation.c \
	iconview.c \
	gridview.c \
	gridviewcelltype.c

HDR_FILES = \
	treeview_impl.h \
	monthcalendar_impl.h \
	spinbox_impl.h \
	coolbar_impl.h \
	listview_impl.h \
	animation_impl.h \
	iconview_impl.h \
	gridview_impl.h

AM_CFLAGS = -D__MGEXT_LIB__

LIB_NAME=libcontrol
libcontrol_la_SOURCES=$(SRC_FILES) $(HDR_FILES)
