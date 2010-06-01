MODULE=newt
SOBJ=newt.so
OBJS=newt.opic

MODLDFLAGS=-lnewt -lslang

include ../modules.mak

install::
ifneq ($(TARGET),udeb)
	install -m 644 ${srcdir}/src/modules/frontend/newt/cdebconf_newt.h $(DESTDIR)${incdir}
endif
