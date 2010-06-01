INSTALL = /usr/bin/install -c
INSTALL_SCRIPT = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644

all:

install:
	$(INSTALL) -d $(DESTDIR)/usr/lib/finish-install.d
	$(INSTALL_SCRIPT) finish-install \
		$(DESTDIR)/usr/lib/finish-install.d/15cdrom-detect

clean:
