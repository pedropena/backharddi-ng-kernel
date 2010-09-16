DISTCLEANFILES = include/stamp-h[0-9]* src/stamp-h[0-9]* src/config.h
INCLUDES = -I$(top_srcdir)/include
bin_PROGRAMS = src/rfbplaymacro
src_rfbplaymacro_SOURCES = src/rfbplaymacro.c src/d3des.c src/d3des.h
EXTRA_DIST = rfbplaymacro.spec test.rfm

tag:
	cvs tag -c `echo V@VERSION@ | tr . _`
