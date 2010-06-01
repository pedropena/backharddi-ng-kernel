## Process this file with automake to produce Makefile.in
#
# Copyright (C) 1996-2003 Markus F.X.J. Oberhumer
#

AUTOMAKE_OPTIONS = 1.7.4 foreign

SUBDIRS = src doc


# /***********************************************************************
# // automake targets
# ************************************************************************/

dist-hook:
	-rm -rf ./$(distdir)/acconfig/
	cp -pR $(srcdir)/B/ $(srcdir)/acconfig/ $(distdir)/
	-rm -f `find ./$(distdir)/ -type l -print`

@MAINT@-include $(top_srcdir)/Makefile.maint
