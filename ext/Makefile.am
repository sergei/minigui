TOP_DIR=../
INCLUDES = -I./include -I../include

SUBDIRS = control mywins skin include res

lib_LTLIBRARIES = libmgext.la

COMMON_LADD =               \
    control/libcontrol.la   \
    mywins/libmywins.la     \
    skin/libskin.la

AM_CFLAGS = -D__MGEXT_LIB__

libmgext_la_LIBADD = $(COMMON_LADD)
libmgext_la_SOURCES = template.c template.h
libmgext_la_LDFLAGS = -release $(LT_RELEASE) -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)

CLEANFILES=*.a
