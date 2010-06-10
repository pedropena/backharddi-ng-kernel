top_srcdir=.
srcdir=.


prefix      = /usr/local
bindir      = ${exec_prefix}/bin
infodir     = ${datarootdir}/info
mandir      = ${datarootdir}/man
datarootdir = ${prefix}/share

CC         = gcc
CFLAGS     = -g -O2 -Wall
CPPFLAGS   = -I.  -I. 
HOST_ID    = -DCPU_x86_64 -DVENDOR_unknown -DOS_linux_gnu
DEFS       = -DHAVE_CONFIG_H $(HOST_ID)

LDFLAGS     = 
LIBS        = -ldl -lpthread 
SHLIB       = 
MACHDEPLIBS = 
LN_S        = @LN_S@

INSTALL         = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}

.SUFFIXES:
.SUFFIXES: .o .c
.SUFFIXES: .o .c


# For additional warnings & checks, add -Wstrict-prototypes -Wshadow to the
# following:
CFLAGS +=-Wall -DBB_FEATURE_UDPCAST_FEC -D_FILE_OFFSET_BITS=64 -DUSE_SYSLOG -DUSE_ASSEMBLER -O6
LDFLAGS +=-s
LIBS +=-ldl -lpthread 

BUSYBOX=../udp-busybox/busybox
BBTARGET=$(BUSYBOX)/udpcast
EXESUFFIX=

CFLAGS += -DNO_BB

all: udp-receiver$(EXESUFFIX) udp-sender$(EXESUFFIX) \
	manpages
#all: fec-test

disk: udpcast.img.gz

#disks: sender.img.gz receiver.img.gz

clean:
	rm -f *.o udp-sender$(EXESUFFIX) udp-receiver$(EXESUFFIX) \
	udp-sender.1 udp-receiver.1 *~

distclean: clean
	rm -f Makefile config.status config.log config.h

install: udp-sender udp-receiver rateGovernor.h
	install -d $(DESTDIR)$(prefix)/sbin
	install -m 755 udp-sender udp-receiver $(DESTDIR)$(prefix)/sbin
	install -d $(DESTDIR)$(mandir)/man1
	install -m 644 udp-sender.1 udp-receiver.1 $(DESTDIR)$(mandir)/man1
	install -d $(DESTDIR)$(prefix)/include/udpcast
	install -m 644 rateGovernor.h $(DESTDIR)$(prefix)/include/udpcast

udp-sender$(EXESUFFIX): udp-sender.o socklib.o udpcast.o \
	rateGovernor.o rate-limit.o auto-rate.o \
	sender-diskio.o senddata.o udps-negotiate.o \
	fifo.o produconsum.o participants.o log.o statistics.o \
	fec.o udpc_version.o console.o process.o rateGovernor.o
	$(CC) $(LDFLAGS) $^ $(LIBS) -o $@


udp-receiver$(EXESUFFIX): udp-receiver.o socklib.o udpcast.o \
	receiver-diskio.o receivedata.o udpr-negotiate.o produconsum.o \
	fifo.o log.o statistics.o fec.o udpc_version.o console.o process.o
	$(CC) $(LDFLAGS) $^ $(LIBS) -o $@


cmd.html.man: udp-receiver.txt udp-sender.txt cmd.html
	cat $^ | m4 >cmd.html.man

manpages: html2man.pl cmd.html.man
	perl $^

fec-test: fec-test.o
	$(CC) $(LDFLAGS) $^ -o $@

fec.o: fec.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) -fno-inline $<


bbcopy: $(BBTARGET) \
	$(BBTARGET)/fec.c $(BBTARGET)/participants.h $(BBTARGET)/socklib.c \
	$(BBTARGET)/udp-receiver.c $(BBTARGET)/fec.h \
	$(BBTARGET)/produconsum.c $(BBTARGET)/socklib.h \
	$(BBTARGET)/udp-receiver.h $(BBTARGET)/fec-test.c \
	$(BBTARGET)/produconsum.h $(BBTARGET)/statistics.c \
	$(BBTARGET)/udpr-negotiate.c $(BBTARGET)/fifo.c \
	$(BBTARGET)/rateGovernor.h $(BBTARGET)/rateGovernor.c \
	$(BBTARGET)/rate-limit.c $(BBTARGET)/rate-limit.h \
	$(BBTARGET)/auto-rate.c $(BBTARGET)/auto-rate.h \
	$(BBTARGET)/statistics.h \
	$(BBTARGET)/udp-sender.c $(BBTARGET)/fifo.h \
	$(BBTARGET)/udpcast.c $(BBTARGET)/udp-sender.h \
	$(BBTARGET)/libbb_udpcast.h $(BBTARGET)/receivedata.c \
	$(BBTARGET)/udpcast.h $(BBTARGET)/udps-negotiate.c \
	$(BBTARGET)/log.c $(BBTARGET)/receiver-diskio.c \
	$(BBTARGET)/udpc-protoc.h $(BBTARGET)/util.h \
	$(BBTARGET)/log.h $(BBTARGET)/senddata.c $(BBTARGET)/udpc_version.c \
	$(BBTARGET)/participants.c $(BBTARGET)/sender-diskio.c \
	$(BBTARGET)/udpc_version.h \
	$(BBTARGET)/console.h $(BBTARGET)/console.c \
	$(BBTARGET)/udpc_process.h $(BBTARGET)/process.c \
	$(BBTARGET)/threads.h \
	$(BBTARGET)/Config.in $(BBTARGET)/Kbuild $(BBTARGET)/Makefile.flags \
	$(BBTARGET)/applets.h $(BBTARGET)/usage.h \
	$(BBTARGET)/config.h

$(BBTARGET):
	mkdir $(BBTARGET)

$(BBTARGET)/Kbuild: Kbuild
	cp -f $< $(BBTARGET)/


$(BBTARGET)/%.c: %.c
	cp -f $< $(BBTARGET)

$(BBTARGET)/%.h: %.h
	cp -f $< $(BBTARGET)

$(BBTARGET)/%.flags: %.flags
	cp -f $< $(BBTARGET)

$(BBTARGET)/Config.in: Config.in
	cp -f $< $(BBTARGET)

bb: bbcopy
	make -C $(BUSYBOX)

.PHONY: manpages
