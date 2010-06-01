SUBDIRS = intl po src

EXTRA_DIST = ABOUT-NLS AUTHORS BOOT-ROOT-creation.txt BUGS COPYING ChangeLog \
             FORMAT FUTURE INSTALL README README.partimaged TESTS THANKS TODO \
             autogen.sh partimage.lsm 


certificates:
	@echo "Disabled on Debian"	

pamfile:
	@echo "# PAM config file for Partimaged" > partimaged.pam
	@echo >> partimaged.pam
	@echo "auth 	required	pam_unix.so" >> partimaged.pam
	@echo "auth	required	pam.warn.so" >> partimaged.pam
	@echo "auth 	required	pam_listfile.so \\" >> partimaged.pam
	@echo "	onerr=fail item=user sense=allow \\" >> partimaged.pam
	@echo "	file=@SYSCONFDIR@/partimaged/partimagedusers" >> partimaged.pam
	@echo "Now you have to 'su root' and put partimaged.pam into"
	@echo "/etc/pam.d/partimaged"
	
install-data-local:
	$(mkinstalldirs) ${sysconfdir}
	$(mkinstalldirs) ${sysconfdir}/partimaged

	if test ! -f ${sysconfdir}/partimaged/partimagedusers ; then \
	echo "# note: '#' introduces comments" > \
          ${sysconfdir}/partimaged/partimagedusers ;\
	echo -n "#add only users allowed to " >>  \
           ${sysconfdir}/partimaged/partimagedusers ;\
	echo "connect partimaged" >> ${sysconfdir}/partimaged/partimagedusers ;\
	echo "# (only one login per line)" >> \
           ${sysconfdir}/partimaged/partimagedusers ;\
	echo "" >> ${sysconfdir}/partimaged/partimagedusers ;\
	echo "#sample # user 'sample' is allowed to connect partimaged" >> \
           ${sysconfdir}/partimaged/partimagedusers ;\
	fi 

	$(mkinstalldirs) ${infodir}
	$(INSTALL_DATA) AUTHORS ${infodir}/AUTHORS
	$(INSTALL_DATA) BUGS ${infodir}/BUGS
	$(INSTALL_DATA) COPYING ${infodir}/COPYING
	$(INSTALL_DATA) ChangeLog ${infodir}/ChangeLog
	$(INSTALL_DATA) INSTALL ${infodir}/INSTALL
	$(INSTALL_DATA) README ${infodir}/README
	$(INSTALL_DATA) README.partimaged ${infodir}/README.partimaged
	$(INSTALL_DATA) TODO ${infodir}/TODO
	$(INSTALL_DATA) partimage.lsm ${infodir}/partimage.lsm

uninstall-local:
	-rm -f ${infodir}/AUTHORS
	-rm -f ${infodir}/BUGS
	-rm -f ${infodir}/COPYING
	-rm -f ${infodir}/ChangeLog
	-rm -f ${infodir}/INSTALL
	-rm -f ${infodir}/README
	-rm -f ${infodir}/README.partimaged
	-rm -f ${infodir}/TODO
	-rm -f ${infodir}/partimage.lsm

AUTOMAKE_OPTIONS = foreign

MAINTAINERCLEANFILES = Makefile.in config.h.in configure stamp-h.in aclocal.m4\
     install-sh missing mkinstalldirs depcomp
