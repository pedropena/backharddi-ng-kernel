VERSION = 1.10

DESTDIR = 

CC = gcc
RPM_OPT_FLAGS = -O2 -Wall -g
CFLAGS = $(RPM_OPT_FLAGS) $(EXTRA_CFLAGS)
DEFINES = -DVERSION=\"$(VERSION)\"
INSTALL = install
INSTALLFLAGS = -s
prefix = $(DESTDIR)/usr
INSTALLDIR = $(DESTDIR)/bin
MYDIR = dd_rescue
TARGETS = dd_rescue stidump cmospwd
OBJECTS = dd_rescue.o stidump.o
DOCDIR = $(prefix)/share/doc/packages
INSTASROOT = -o root -g root

default: $(TARGETS)

stidump: stidump.c
	$(CC) $(CFLAGS) $< -o $@

cmospwd: cmospwd.c
	$(CC) $(CFLAGS) $< -o $@

dd_rescue: dd_rescue.c
	$(CC) $(CFLAGS) $(DEFINES) $< -o $@

clean:
	rm -f $(TARGETS) $(OBJECTS) core

distclean: clean
	rm -f *~

install: $(TARGETS)
	mkdir -p $(INSTALLDIR)
	$(INSTALL) $(INSTALLFLAGS) $(INSTASROOT) -m 755 $(TARGETS) $(INSTALLDIR)
