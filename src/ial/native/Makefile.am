INCLUDES = -I../../../include -I../../include 

noinst_LTLIBRARIES=libnative.la

EXTRA_DIST=README

MOUSE_SRC= mou_gpm.c mou_ps2.c mou_ms3.c mou_ms.c mou_imps2.c

libnative_la_SOURCES=native.c native.h vtswitch-lite.c kbd_tty.c $(MOUSE_SRC)

