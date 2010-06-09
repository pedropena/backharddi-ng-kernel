include ../../../../globalmakeflags

CFLAGS  += $(MODCFLAGS)
LDFLAGS  = -Wl,-rpath,${moddir} $(MODLDFLAGS)
SUBDIR   = src/modules/frontend/$(MODULE)

all: $(SOBJ)

$(SOBJ): $(OBJS)
	@echo Creating DSO $@ from $^
	$(CC) $(CFLAGS) -shared -o $@ $^ $(LDFLAGS)

install::
	install -d -m 755 $(DESTDIR)${moddir}/frontend
	install -m 755 $(SOBJ) $(DESTDIR)${moddir}/frontend

clean:
	-@rm -f $(SOBJ) $(OBJS) *~

