
INCLUDES = -I$(abs_top_srcdir)/src/include -I$(abs_top_srcdir)/include

noinst_LTLIBRARIES = libsysres.la

if SUNXI_MIN
SUBDIRS =
libsysres_la_SOURCES = resource.c incoreres.c mgetc.c resmgr.c resmgr.h
else
SUBDIRS = bmp icon font cursor 
if MG_ENABLE_SPLASH
if	MG_ENABLE_SCREENSAVER
SUBDIRS+=license
endif
endif
libsysres_la_SOURCES = resource.c incoreres.c mgetc.c resmgr.c resmgr.h
endif

SRC_FILES=$(libsysres_la_SOURCES)
LIB_NAME=libsysres

libsysres_la_LIBADD= 
#bmp/libincorebmps.la \
#	icon/libincoreicons.la \
#	font/libincorefonts.la \
#	cursor/libincorecursors.la 

EXTRA_DIST=makefile.ng makefile.msvc
