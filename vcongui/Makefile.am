INCLUDES = -I../include -I../ext/include

lib_LTLIBRARIES = libvcongui.la

if LIB_VCONGUI
libvcongui_la_SOURCES =     \
    child.c child.h error.c error.h      \
    key.c key.h paint.c paint.h resource.h terminal.c       \
    terminal.h vc.c vc.h vcongui.c vt.c vt.h defs.h
else
libvcongui_la_SOURCES = foo.c
endif

EXTRA_DIST = foo.c \
    child.c child.h error.c error.h      \
    key.c key.h paint.c paint.h resource.h terminal.c       \
    terminal.h vc.c vc.h vcongui.c vt.c vt.h defs.h

libvcongui_la_LDFLAGS = -release $(LT_RELEASE) -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
