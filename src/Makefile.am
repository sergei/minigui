TOP_DIR=..

COMMON_SUBDIRS = misc main kernel sysres ial control font gui mybmp include

lib_LTLIBRARIES = libminigui.la

COMMON_LADD =               \
    misc/libmisc.la         \
    control/libcontrol.la   \
    font/libfont.la         \
    font/in-core/libincore.la \
    ial/libial.la           \
    gui/libgui.la           \
    main/libmain.la         \
    mybmp/libmybmp.la       \
    kernel/libkernel.la     \
    sysres/libsysres.la

NEWGAL_LADD = newgal/libnewgal.la newgdi/libnewgdi.la
NEWGAL_SUBDIRS = newgal newgdi

DIST_SUBDIRS = $(COMMON_SUBDIRS) newgal newgdi
SUBDIRS = $(COMMON_SUBDIRS) $(NEWGAL_SUBDIRS)

libminigui_la_LIBADD =  \
    $(COMMON_LADD)      \
    $(NEWGAL_LADD)

#@SYSTEM_LIBS@

libminigui_la_SOURCES = template.c template.h
libminigui_la_LDFLAGS = -release $(LT_RELEASE) -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)

CLEANFILES=*.a

