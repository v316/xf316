
## Makefile.am for the SUNXI video driver

INCLUDES = -I$(abs_top_srcdir)/src/include -I$(abs_top_srcdir)/include \
			            -I$(abs_top_srcdir)/src/newgal/

noinst_LTLIBRARIES = libvideo_sunxi.la

# The sunxi video driver sources
SRC_FILES = sunxi.c hwmem.c sunxi_alloc.c ion_alloc.c

HDR_FILES = sunxi.h

libvideo_sunxi_la_SOURCES = $(SRC_FILES) $(HDR_FILES)

libvideo_sunxi_la_CFLAGS = 

EXTRA_DIST=

LIB_NAME=libvideo_sunxi
