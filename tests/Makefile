include ../config

DIR := $(shell cd ..; pwd)
KERNEL := $(DIR)/boot/linux-$(KVERSION)-backharddi-ng
INITRD := $(DIR)/boot/minirt-$(KVERSION)-backharddi-ng.gz
GTKCMDLINE := video=vesa:ywrap,mtrr vga=788 
RFBPLAYMACRO := rfbplaymacro-0.2.2/src/rfbplaymacro
RFBPROXY := rfbproxy-1.1.1/rfbproxy
SLT := 1
REMOTE_DIR := /home/pedro/Escritorio/Desarrollo/workspace/backharddi-ng/backharddi-ng-kernel/tests
SERVER := 192.168.0.201
VNCVIEWER = vncviewer
RVNCVIEWER = echo Go | nc localhost 4000

.PHONY: tests
tests: test_001 test_002

$(RFBPLAYMACRO) $(RFBPROXY):
	echo $@
	cd $(firstword $(subst /, ,$@)); ./configure; make

.PHONY: record_%
record_%: $(RFBPROXY)
	@cd $*; i=0; for script in prepare_disk*; do i=$$((i+1)); echo " * [$*] Preparando disco $$i"; bash $$script >/dev/null 2>&1; done
	@echo " * [$*] Grabando test"; cd $*; kvm -vnc :10 -no-reboot -kernel $(KERNEL) -initrd $(INITRD) -append "$(shell cat $*/cmdline)" $(shell cat $*/params) &
	@$(RFBPROXY) :20 --server=:10 --type=events -r $*/fbs >/dev/null &
	@sleep $(SLT); [ "x$(REMOTE)" = "xyes" ] && $(RVNCVIEWER) || $(VNCVIEWER) :20 >/dev/null 2>&1

.PHONY: test_%
test_%: $(RFBPLAYMACRO)
	@cd test$*; i=0; for script in prepare_disk*; do i=$$((i+1)); echo " * [test$*] Preparando disco $$i"; bash $$script >/dev/null 2>&1; done
	@echo " * [test$*] Ejecutando test"; cd test$*; kvm -vnc :20 -no-reboot -kernel $(KERNEL) -initrd $(INITRD) -append "$(shell cat test$*/cmdline)" $(shell cat test$*/params) & \
	sleep $(SLT); [ ! -f view ] || { [ "x$(REMOTE)" = "xyes" ] && $(RVNCVIEWER) || $(VNCVIEWER) :20 >/dev/null 2>&1; } & \
	sleep $(SLT); cat fbs | ../$(RFBPLAYMACRO) --server :20; \
	wait
	@echo -n " * [test$*] Comprobando resultados... "; cd test$*; [ ! -f checks ] || bash checks

.PHONY: manual_%
manual_%: 
	@cd $*; kvm -no-reboot -kernel $(KERNEL) -initrd $(INITRD) -append "$(GTKCMDLINE) $(shell cat $*/cmdline)" $(shell cat $*/params)

remote_%:
	ssh -fN -R 4000:localhost:4000 $(SERVER)
	ssh -fN -L 5920:localhost:5920 $(SERVER)
	{ nc -l 4000; $(VNCVIEWER) :20 >/dev/null 2>&1; } &
	ssh -t $(SERVER) "cd $(REMOTE_DIR); make $* REMOTE=yes"
