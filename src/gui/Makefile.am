INCLUDES = -I../../include -I../include

noinst_LTLIBRARIES = libgui.la

SRC_FILES = accelkey.c caret.c ctrlclass.c dialog.c menu.c window.c element.c \
    keyboard.c \
    defkeymap.c fr-pc-kmap.c fr-kmap.c de-kmap.c de-latin1-kmap.c \
    it-kmap.c es-kmap.c es-cp850-kmap.c

HDR_FILES = keyboard.h linux_kd.h  linux_keyboard.h  linux_types.h

libgui_la_SOURCES=$(SRC_FILES) $(HDR_FILES)

LIB_NAME=libgui

TOP_DIR=../..
