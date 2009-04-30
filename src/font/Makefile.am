
SUBDIRS = in-core

noinst_LTLIBRARIES = libfont.la

SRC_FILES = charset.c devfont.c fontname.c \
            rawbitmap.c varbitmap.c qpf.c \
            gbunimap.c big5unimap.c mapunitogb.c mapunitobig5.c \
            textops.c sysfont.c logfont.c 

HDR_FILES = charset.h rawbitmap.h varbitmap.h qpf.h

INCLUDES = -I../../include -I../include

libfont_la_SOURCES = $(SRC_FILES) $(HDR_FILES)

LIB_NAME=libfont

TOP_DIR=../..
