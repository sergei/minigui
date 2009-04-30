INCLUDES = -I../../include -I../include

noinst_LTLIBRARIES = libnewgdi.la

SRC_FILES = gdi.c attr.c clip.c map.c coor.c rect.c  \
              palette.c readbmp.c icon.c screen.c bitmap.c \
              text.c drawtext.c pixel.c line.c arc.c pixel_ops.c \
              region.c generators.c polygon.c flood.c \
              yuv.c \
              advapi.c midash.c mispans.c miwideline.c \
              mifillarc.c mifpolycon.c miarc.c rotatebmp.c

HDR_FILES = drawtext.h mi.h midc.h mistruct.h miwideline.h \
            pixel_ops.h mifillarc.h mispans.h polygon.h mifpoly.h 

libnewgdi_la_SOURCES = $(SRC_FILES) $(HDR_FILES)

LIB_NAME=libnewgdi
TOP_DIR=../..
