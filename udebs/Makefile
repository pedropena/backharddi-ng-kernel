#!/usr/bin/make -f

#Es necesario poner BUILDUSERNAME=root y BUILDUSERID=0 en pbuilderrc para poder compilar backharddi-kernel
#Dependencias: cowdancer (>= 0.47), pbuilder (>= 0.181), mkisofs, debhelper, fakeroot, reprepro (>= 3.3.1), kernel-wedge, dpatch, quilt, cdbs, sudo, debootstrap (>= 1.0.10)
SHELL := bash
DEBIANMIRROR := http://localhost:3142/ftp.debian.org/debian
DEBIANCOMPONENTS := main
UBUNTUMIRROR := http://localhost:3142/archive.ubuntu.com/ubuntu
UBUNTUCOMPONENTS := main universe
ROOTCMD := sudo
DIR := $(shell pwd)
REPOSITORY := $(DIR)/apt
PBUILDEROPTS = --buildplace $(DIR)/$(BUILDER)/build --hookdir $(DIR)/$(BUILDER)/hooks --bindmounts $(REPOSITORY)
PDEBUILDOPTS = --pbuilderroot $(ROOTCMD) --buildsourceroot fakeroot --pbuilder $(BUILDER) --buildresult $(DIR)/$(BUILDER)/result/$(DIST) --debbuildopts -b
UBUNTUDIST := hardy jaunty karmic
DEBIANDIST := etch lenny
BUILDDIST := etch
BUILDARCH := i386

BUILDER := pbuilder
PBUILDBASE := --basetgz
PBUILDEXT := tar.gz
#BUILDER := cowbuilder
#PBUILDBASE := --debian-etch-workaround --basepath
#PBUILDEXT := cow

define submake
	$(MAKE) --no-print-directory
endef

define log
	echo -e " * $(1)"
endef

define udebdir
	$(shell changelog=$$(find -wholename *$(1)/source/debian/changelog); echo -n $${changelog%*/debian/changelog})
endef

define is_source_changed
	$$(shell find $$(call udebdir,$(1)) -type f ! -name \.\*\.swp ! -wholename \*svn\* ! -wholename \*CVS\* 2>/dev/null)
endef

define changes_file
	$(shell echo $(BUILDER)/result/$(DIST)/$(1)_$(shell dpkg-parsechangelog -l$(shell find -wholename *$(1)/source/debian/changelog) 2>/dev/null | grep ^Version | cut -d " " -f 2 | sed "s/.\://")_$(ARCH).changes)
endef

define config_mirror_and_components
	for dist in $(DEBIANDIST); do [ $$dist = $(DIST) ] && { components="$(DEBIANCOMPONENTS)"; mirror=$(DEBIANMIRROR); }; done; \
	for dist in $(UBUNTUDIST); do [ $$dist = $(DIST) ] && { components="$(UBUNTUCOMPONENTS)"; mirror=$(UBUNTUMIRROR); }; done
endef

last = $(lastword $(subst _, ,$*))
second = $(word 2,$(subst _, ,$*))
third = $(word 3,$(subst _, ,$*))
fourth = $(word 4,$(subst _, ,$*))
first = $(firstword $(subst _, ,$*))

all: $(shell for f in $$(find -wholename */debian/changelog ! -wholename */.pc/*); do dir=$${f%*/source/debian/changelog}; pkg=$$(basename $$dir); echo -n "changes_$$(basename $$dir) "; done)

$(BUILDER)/$(DIST).$(ARCH).$(PBUILDEXT):
	@$(call log,Construyendo $(BUILDER) para la distribución $(DIST) $(ARCH))
	@install -d $(BUILDER)/result/$(DIST) $(BUILDER)/build $(BUILDER)/aptcache/$(DIST) $(REPOSITORY)
	@$(call config_mirror_and_components); \
	$(ROOTCMD) $(BUILDER) --create $(PBUILDEROPTS) --distribution $(DIST) $(PBUILDBASE) $(BUILDER)/$(DIST).$(ARCH).$(PBUILDEXT) --aptcache $(BUILDER)/aptcache/$(DIST) --mirror $$mirror --debootstrap debootstrap --components "$$components" --debootstrapopts --arch --debootstrapopts $(ARCH)

.PRECIOUS: $(BUILDER)/result/$(DIST)/%.changes $(REPOSITORY)/%/conf/distributions

$(REPOSITORY)/%/conf/distributions:
	mkdir -p $(REPOSITORY)/$*/conf || true
	echo "Origin: Backharddi NG" >$@
	echo "Suite: $(DIST)" >>$@
	echo "Codename: $(DIST)" >>$@
	echo "Architectures: source i386 amd64" >>$@
	echo "Components: main" >>$@
	echo "UdebComponents: main" >>$@
	echo "Description: Backharddi NG Packages" >>$@
	reprepro -b $(REPOSITORY)/$* export

$(BUILDER)/hooks/C10shell:
	@mkdir -p $(BUILDER)/hooks || true
	echo "#!/bin/sh" > $@
	echo "cd /tmp/buildd/*/debian/.." >> $@
	echo "/bin/bash < /dev/tty >/dev/tty 2>/dev/tty" >> $@
	chmod 755 $@

$(BUILDER)/hooks/D01apt:
	@mkdir -p $(BUILDER)/hooks || true
	echo "#!/bin/sh" > $@
	echo "cat<<EOF >>/etc/apt/sources.list" >> $@
	echo "deb file://$(REPOSITORY)/$(DIST) $(DIST) main" >> $@
	echo "EOF" >> $@
	echo "apt-get update" >> $@
	chmod 755 $@

.SECONDEXPANSION:
$(BUILDER)/result/$(DIST)/%.changes: $(BUILDER)/$(DIST).$(ARCH).$(PBUILDEXT) $(call is_source_changed,$$(first)) $(REPOSITORY)/$(DIST)/conf/distributions $(BUILDER)/hooks/C10shell $(BUILDER)/hooks/D01apt
	@$(call log,Construyendo paquete $(first) versión $(second) para la distribución $(DIST) $(ARCH))
	@rm $(BUILDER)/result/$(DIST)/$(first)_*_$(ARCH).changes 2> /dev/null || true
	@rm $(BUILDER)/result/$(DIST)/$(first)_*_source.changes 2> /dev/null || true
	@install -d $(BUILDER)/result/$(DIST)
	$(call config_mirror_and_components); \
	changelog=$$(find -wholename *$(first)/source/debian/changelog); \
	cd $${changelog%*/debian/changelog}; \
	pdebuild $(PDEBUILDOPTS) -- $(PBUILDEROPTS) --distribution $(DIST) --aptcache $(DIR)/$(BUILDER)/aptcache/$(DIST) $(PBUILDBASE) $(DIR)/$(BUILDER)/$(DIST).$(ARCH).$(PBUILDEXT) --mirror $$mirror --components "$$components"; \
	mv ../$(first)_* $(DIR)/$(BUILDER)/result/$(DIST)
	@reprepro -b $(REPOSITORY)/$(DIST) removefilter $(DIST) '$$Source (== $(first))' || true; \
	reprepro -b $(REPOSITORY)/$(DIST) -A source remove $(DIST) $(first) || true; \
	for pkg in $(UDEBS); do [ $(first)/ = $$pkg ] && { opts="-S debian-installer -P optional"; sed -i "/.*$(second).*\.deb/d" $@; break; }; done; \
	reprepro -b $(REPOSITORY)/$(DIST) $$opts --ignore=wrongdistribution include $(DIST) $(BUILDER)/result/$(DIST)/$(first)_$(second)_source.changes; \
	reprepro -b $(REPOSITORY)/$(DIST) --ignore=wrongdistribution --ignore=unusedarch include $(DIST) $@
 
.PHONY: changes_%
changes_%:
	@$(submake) $(call changes_file,$*)

changes_partman-base: changes_%: changes_parted
	@$(submake) $(call changes_file,$*)

changes_linux-ntfs: changes_%: changes_fuse
	@$(submake) $(call changes_file,$*)

changes_e2fsprogs: changes_%: changes_util-linux
	@$(submake) $(call changes_file,$*)

changes_partconf: changes_%: changes_parted
	@$(submake) $(call changes_file,$*)

#La siguiente regla es para deshabilitar la regla implicita para generar los ficheros .c. Si no estuviera deshabilitada el toolchain fallaría al compilar los paquetes e2fsprogs y partimage debido a los prerequisitos $$(shell find...
%.c: %.y
