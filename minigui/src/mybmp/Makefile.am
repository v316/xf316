INCLUDES = -I$(abs_top_srcdir)/src/include -I$(abs_top_srcdir)/include

noinst_LTLIBRARIES = libmybmp.la
if SUNXI_MIN
SRC_FILES = mybmp.c winbmp.c jpeg.c png.c
else
SRC_FILES = mybmp.c winbmp.c gif.c jpeg.c png.c lbm.c pcx.c tga.c 
endif
libmybmp_la_SOURCES = $(SRC_FILES) 

EXTRA_DIST=makefile.ng makefile.msvc

LIB_NAME=libmybmp
