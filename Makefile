all: minirt

include config

.PHONY: pkg-list
pkg-list: udeb.list
	cut -f 1 -d " " udeb.list | sed "/modules/d;/kernel/d" > $@

.PHONY: modules-list
modules-list: udeb.list
	cut -f 1 -d " " udeb.list | sed -n "/modules/p;/kernel/p" > $@

.PHONY: update-modules-list
update-modules-list:
	sed -i "s/-2\.6.*-/-$(KVERSION)-/" modules-list

clean: clean-patches
	rm udeb.list || true
	[ ! -d debian.orig ] || mv debian debian.d-i
	[ ! -d debian.orig ] || mv debian.orig debian
	$(ROOTCMD) rm $(BUILDDIR) build/localudebs/* boot usr udebs/apt -rf

.PHONY: localudebs
localudebs:
	rm build/localudebs/* -f || true
	@for c in $$(find $$(pwd) -wholename */source/debian/changelog ! -wholename */.pc/*); do \
		dir=$${c%*/source/debian/changelog}; \
		cd $$dir; \
		echo " * Aplicando parches a $$(basename $$dir)"; \
		quilt push -a || true; \
		echo; \
	done
	mkdir build/localudebs || true
	$(MAKE) -C udebs DIST=$(SUITE) ARCH=$(ARCH)
	@for c in $$(find $$(pwd) -wholename */source/debian/changelog ! -wholename */.pc/*); do \
		dir=$${c%*/source/debian/changelog}; \
		cd $$dir; \
		echo " * Limpiando parches de $$(basename $$dir)"; \
		quilt pop -a -f || true; \
		echo; \
	done
	find udebs/apt -name \*.udeb | xargs -I'{}' cp '{}' build/localudebs
 
.PHONY: clean-patches
clean-patches:
	@for c in $$(find $$(pwd) -wholename */source/debian/changelog ! -wholename */.pc/*); do \
		dir=$${c%*/source/debian/changelog}; \
		cd $$dir; \
		echo " * Limpiando parches de $$(basename $$dir)"; \
		quilt pop -a -f || true; \
		echo; \
	done

minirt: localudebs
	[ -d debian.orig ] || { mv debian debian.orig; mv debian.d-i debian; } 
	rm boot usr -rf
	echo "BUILDUSERNAME=root\nBUILDUSERID=0" >$(BUILDDIR)/.pbuilderrc
	HOME=$(BUILDDIR); pdebuild $(PDEBUILDOPTS) --debbuildopts "-b -Itests -Idebian.orig -I.git -Iudebs -I.project -Iusr -I.gitignore -I.pydevproject" -- $(BUILDEROPTS) --distribution $(SUITE) --aptcache $(BUILDDIR)/aptcache/$(SUITE) $(BUILDERBASE) $(BUILDDIR)/$(SUITE).$(ARCH).$(BUILDEREXT) --mirror $(MIRROR) --components $(COMPONENTS) || true
	dpkg -x $(BUILDDIR)/result/$(SUITE)/backharddi-ng-kernel-i386_$$(dpkg-parsechangelog | grep Version: | cut -f 2 -d " ")_all.deb .
	mv debian debian.d-i
	mv debian.orig debian
	
srelease:
	dpkg-buildpackage -S -I.git -I*.udeb -I$(BUILDDIR) -I.project -I.gitignore -I.pydevproject -Iudebs/apt -Iudebs/pbuilder
	fakeroot ./debian/rules clean
	
brelease:
	dpkg-buildpackage -b -I.git -I*.udeb -I$(BUILDDIR) -I.project -I.gitignore -I.pydevproject -Iudebs/apt -Iudebs/pbuilder
	fakeroot ./debian/rules clean
