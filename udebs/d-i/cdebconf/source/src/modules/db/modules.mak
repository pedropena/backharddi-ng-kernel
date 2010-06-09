include ../../../../globalmakeflags

CFLAGS  += $(MODCFLAGS)
LDFLAGS  = -Wl,-rpath,${moddir} $(MODLDFLAGS)
SUBDIR   = src/modules/db/$(MODULE)

all: $(SOBJ)

$(SOBJ): $(OBJS)
	@echo Creating DSO $@ from $^
	@$(CC) $(CFLAGS) -shared -o $@ $^ $(LDFLAGS)

install:
	install -d -m 755 $(DESTDIR)${moddir}/db
	install -m 755 $(SOBJ) $(DESTDIR)${moddir}/db

clean:
	-@rm -f $(SOBJ) $(OBJS) *~
