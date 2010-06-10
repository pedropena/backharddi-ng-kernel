Name: hwdata
Summary: Hardware identification and configuration data
Version: 0.191
Release: 1
License: GPL/MIT
Group: System Environment/Base
Source: hwdata-%{version}.tar.gz
BuildArch: noarch
Conflicts: Xconfigurator, system-config-display < 1.0.31, pcmcia-cs, kudzu < 1.2.0
Requires: module-init-tools >= 3.2
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root

%description
hwdata contains various hardware identification and configuration data,
such as the pci.ids database and MonitorsDb databases.

%prep

%setup -q

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc LICENSE COPYING
%dir /usr/share/hwdata
%config(noreplace) /etc/modprobe.d/blacklist
%config /usr/share/hwdata/*

%changelog
* Mon Oct 09 2006 Phil Knirsch <pknirsch@redhat.com> - 0.191-1
- Update to latest pci.ids for RHEL5

* Thu Sep 21 2006 Adam Jackson <ajackson@redhat.com> - 0.190-1
- Add a description for the 'intel' driver.

* Mon Sep 18 2006 Phil Knirsch <pknirsch@redhat.com> - 0.189-1
- Updated usb.ids for FC6

* Mon Sep 11 2006 Phil Knirsch <pknirsch@redhat.com> - 0.188-1
- Update of pci.ids for FC6

* Thu Aug 31 2006 Adam Jackson <ajackson@redhat.com> - 0.187-1
- Fix sync ranges for Samsung SyncMaster 710N (#202344)

* Thu Aug 03 2006 Phil Knirsch <pknirsch@redhat.com> - 0.186-1
- Updated pci.ids once more.

* Tue Jul 25 2006 Phil Knirsch <pknirsch@redhat.com> - 0.185-1
- Added the 17inch Philips LCD monitor entry (#199828)

* Mon Jul 24 2006 Phil Knirsch <pknirsch@redhat.com> - 0.184-1
- Added one more entry for missing Philips LCD monitor (#199828)

* Tue Jul 18 2006 Phil Knirsch <pknirsch@redhat.com> - 0.183-1
- Updated pci.ids before FC6 final (#198994)
- Added several missing Samsung monitors (#197463)
- Included a new inf2mondb.py from Matt Domsch (#158723)

* Tue Jul 11 2006 Adam Jackson <ajackson@redhat.com> - 0.182-1
- Added ast driver description to videodrivers
- Numerous Dell monitor additions (#196734)
- Numerous Belinea monitor additions (#198087)

* Sat Jul  8 2006 Adam Jackson <ajackson@redhat.com> - 0.181-1
- Updated videodrivers to mention i945
- New monitors: Sony CPD-G420 (#145902), Compaq P1110 (#155120).

* Thu May 11 2006 Phil Knirsch <pknirsch@redhat.com> - 0.180-1
- Updated and added some MonitorsDB entries

* Tue May 02 2006 Phil Knirsch <pknirsch@redhat.com> - 0.179-1
- Updated PCI ids from upstream (#180402)
- Fixed missing monitor entry in MonitorsDB (#189446)

* Wed Mar 01 2006 Phil Knirsch <pknirsch@redhat.com> - 0.178-1
- Commented out the VT lines at the end of usb.ids as our tools don't handle
  them properly.

* Fri Feb 24 2006 Bill Nottingham <notting@redhat.com> - 0.177-1
- remove stock videoaliases in favor of driver-specific ones in
  the X driver packages

* Wed Feb 22 2006 Phil Knirsch <pknirsch@redhat.com> - 0.176-1
- More entries from Dell to MonitorsDB (#181008)

* Fri Feb 10 2006 Phil Knirsch <pknirsch@redhat.com> - 0.175-1
- Added a few more entries to MonitorsDB

* Wed Feb 01 2006 Phil Knirsch <pknirsch@redhat.com> - 0.174-1
- Some cleanup and adds to the MonitorDB which closes several db related bugs.

* Tue Dec 13 2005 Bill Nottingham <notting@redhat.com> - 0.173-1
- add some IDs to the generic display entries for matching laptops

* Fri Nov 18 2005 Bill Nottingham <notting@redhat.com> - 0.172-1
- ditto for radeon

* Fri Nov 18 2005 Jeremy Katz <katzj@redhat.com> - 0.171-1
- r128 -> ati.  should fix the unresolved symbol and kem says its more 
  generally the "right" thing to do

* Wed Nov 16 2005 Bill Nottingham <notting@redhat.com> - 0.170-1
- handle mptsas for migration as well
- move videoaliases file to a subdir

* Fri Sep 16 2005 Bill Nottingham <notting@redhat.com>
- add Iiyama monitor (#168143)

* Tue Sep 13 2005 Bill Nottingham <notting@redhat.com>
- add IBM monitor (#168080)

* Thu Sep  8 2005 Bill Nottingham <notting@redhat.com> - 0.169-1
- remove Cards, pcitable. Add videodrivers

* Fri Sep  2 2005 Dan Williams <dcbw@redhat.com> - 0.168-1
- Add more Gateway monitors

* Fri Sep  2 2005 Dan Williams <dcbw@redhat.com> - 0.167-1
- Add some ADI monitors, one BenQ, and and DPMS codes for two Apples

* Fri Sep  2 2005 Bill Nottingham <notting@redhat.com> - 0.166-1
- add videoaliases file
- remove CardMonitorCombos, as nothing uses it

* Thu Aug 25 2005 Dan Williams <dcbw@redhat.com> - 0.165-1
- Add a bunch of Acer monitors

* Tue Aug  9 2005 Jeremy Katz <katzj@redhat.com> - 0.164-1
- migrate sk98lin -> skge

* Sat Jul 30 2005 Bill Nottingham <notting@redhat.com>
- migrate mpt module names (#161420)
- remove pcitable entries for drivers in modules.pcimap
- switch lone remaining 'Server' entry - that can't work right

* Tue Jul 26 2005 Bill Nottingham <notting@redhat.com>
- add Daytek monitor (#164339)

* Wed Jul 13 2005 Bill Nottingham <notting@redhat.com> - 0.162-1
- remove /etc/pcmcia/config, conflict with pcmcia-cs

* Fri Jul  7 2005 Bill Nottingham <notting@redhat.com> - 0.160-1
- move blacklist to /etc/modprobe.d, require new module-init-tools
- add LG monitors (#162466, #161734)
- add orinoco card (#161696)
- more mptfusion stuff (#107088)

* Thu Jun 23 2005 Bill Nottingham <notting@redhat.com>
- add Samsung monitor (#161013)

* Wed Jun 22 2005 Bill Nottingham <notting@redhat.com> - 0.159-1
- pcitable: make branding happy (#160047)
- Cards: add required blank line (#157972)
- add some monitors
- add JVC CD-ROM (#160907, <richard@rsk.demon.co.uk>)
- add hisax stuff to blacklist (#154799, #159068)

* Mon May 16 2005 Bill Nottingham <notting@redhat.com> - 0.158-1
- add a orinoco card (#157482)

* Thu May  5 2005 Jeremy Katz <katzj@redhat.com> - 0.157-1
- add 20" Apple Cinema Display

* Sun Apr 10 2005 Mike A. Harris <mharris@redhat.com> 0.156-1
- Update SiS entries in Cards/pcitable to match what Xorg X11 6.8.2 supports

* Wed Mar 30 2005 Dan Williams <dcbw@redhat.com> 0.155-1
- Add a boatload of BenQ, Acer, Sony, NEC, Mitsubishi, and Dell monitors

* Wed Mar 30 2005 Dan Williams <dcbw@redhat.com> 0.154-1
- Add Typhoon Speednet Wireless PCMCIA Card mapping to atmel_cs driver

* Mon Mar 28 2005 Bill Nottingham <notting@redhat.com> 0.153-1
- update the framebuffer blacklist

* Wed Mar  9 2005 Bill Nottingham <notting@redhat.com> 0.152-1
- fix qlogic driver mappings, add upgradelist mappings for the modules
  that changed names (#150621)

* Wed Mar  2 2005 Mike A. Harris <mharris@redhat.com> 0.151-1
- Added one hundred billion new nvidia PCI IDs to pcitable and Cards to
  synchronize it with X.Org X11 6.8.2.  (#140601)

* Tue Jan 11 2005 Dan Williams <dcbw@redhat.com> - 0.150-1
- Add Dell UltraSharp 1704FPV (Analog & Digital)

* Sun Nov 21 2004 Bill Nottingham <notting@redhat.com> - 0.148-1
- add Amptron monitors (#139142)

* Wed Nov 10 2004 Bill Nottingham <notting@redhat.com> - 0.147-1
- update usb.ids (#138533)
- migrate dpt_i2o to i2o_block (#138603)

* Tue Nov  9 2004 Bill Nottingham <notting@redhat.com> - 0.146-1
- update pci.ids (#138233)
- add Apple monitors (#138481)

* Wed Oct 20 2004 Bill Nottingham <notting@redhat.com> - 0.145-1
- remove ahci mappings, don't prefer it over ata_piix

* Tue Oct 19 2004 Kristian Høgsberg <krh@redhat.com> - 0.144-1
- update IDs for Cirrus, Trident, C&T, and S3

* Tue Oct 12 2004 Bill Nottingham <notting@redhat.com> - 0.143-1
- add ahci mappings to prefer it over ata_piix
- map davej's ancient matrox card to vesa (#122750)

* Thu Oct  7 2004 Dan Williams <dcbw@redhat.com> - 0.141-1
- Add Belkin F5D6020 ver.2 (802.11b card based on Atmel chipset)

* Fri Oct  1 2004 Bill Nottingham <notting@redhat.com> - 0.140-1
- include /etc/hotplug/blacklist here

* Thu Sep 30 2004 Bill Nottingham <notting@redhat.com> - 0.136-1
- add S3 UniChrome (#131403)
- update pci.ids

* Thu Sep 23 2004 Bill Nottingham <notting@redhat.com> - 0.135-1
- megaraid -> megaraid_mbox

* Wed Sep 22 2004 Bill Nottingham <notting@redhat.com> - 0.134-1
- map ncr53c8xx to sym53c8xx (#133181)

* Fri Sep 17 2004 Bill Nottingham <notting@redhat.com> - 0.132-1
- fix 3Ware 9000 mapping (#132851)

* Tue Sep 14 2004 Kristian Høgsberg <krh@redhat.com> - 0.131-1
- Add python script to check sorting of pci.ids

* Thu Sep  9 2004 Kristian Høgsberg <krh@redhat.com> 0.131-1
- Add pci ids and cards for new ATI, NVIDIA and Intel cards

* Sat Sep  4 2004 Bill Nottingham <notting@redhat.com> 0.130-1
- trim pcitable - now just ids/drivers

* Wed Sep  1 2004 Bill Nottingham <notting@redhat.com> 0.125-1
- pci.ids updates
- remove updsftab.conf.*

* Sun Aug 29 2004 Mike A. Harris <mharris@redhat.com>  0.124-1
- Updates to pcitable/Cards for 'S3 Trio64 3D' cards. (#125866,59956)

* Fri Jul  9 2004 Mike A. Harris <mharris@redhat.com>  0.123-1
- Quick pcitable/Cards update for ATI Radeon and FireGL boards

* Mon Jun 28 2004 Bill Nottingham <notting@redhat.com>
- add Proview monitor (#125853)
- add ViewSonic monitor (#126324)
- add a Concord camera (#126673)

* Wed Jun 23 2004 Brent Fox <bfox@redhat.com> - 0.122-1
- Add Vobis monitor to MonitorsDB (bug #124151)

* Wed Jun 09 2004 Dan Williams <dcbw@redhat.com> - 0.121-1
- add Belkin F5D5020 10/100 PCMCIA card (#125581)

* Fri May 28 2004 Bill Nottingham <notting@redhat.com>
- add modem (#124663)

* Mon May 24 2004 Bill Nottingham <notting@redhat.com> - 0.120-1
- mainly:
  fix upgradelist module for CMPci cards (#123647)
- also:
  add another wireless card (#122676)
  add wireless card (#122625)
  add 1280x800 (#121548)
  add 1680x1050 (#121148)
  add IntelligentStick (#124313)

* Mon May 10 2004 Jeremy Katz <katzj@redhat.com> - 0.119-1
- veth driver is iseries_veth in 2.6

* Wed May  5 2004 Jeremy Katz <katzj@redhat.com> - 0.118-1
- add a wireless card (#122064)
- and a monitor (#121696)

* Fri Apr 16 2004 Bill Nottingham <notting@redhat.com> 0.117-1
- fix makefile

* Thu Apr 15 2004 Bill Nottingham <notting@redhat.com> 0.116-1
- move updfstab.conf here
- add wireless card (#116865)
- add laptop display panel (#117385)
- add clipdrive (#119928)
- add travelling disk (#119143)
- add NEXDISK (#106782)

* Thu Apr 15 2004 Brent Fox <bfox@redhat.com> 0.115-1
- replace snd-es1960 driver with snd-es1968 in pcitable (bug #120729)

* Mon Mar 29 2004 Bill Nottingham <notting@redhat.com> 0.114-1
- fix entries pointing to Banshee (#119388)

* Tue Mar 16 2004 Bill Nottingham <notting@redhat.com> 0.113-1
- add a Marvell sk98lin card (#118467, <64bit_fedora@comcast.net>)

* Fri Mar 12 2004 Brent Fox <bfox@redhat.com> 0.112-1
- add a Sun flat panel to MonitorsDB (bug #118138)

* Fri Mar  5 2004 Brent Fox <bfox@redhat.com> 0.111-1
- add Samsung monitor to MonitorsDB (bug #112112)

* Mon Mar  1 2004 Mike A. Harris <mharris@redhat.com> 0.110-1
- Added 3Dfx Voodoo Graphics and Voodoo II entries to the Cards database, both
  pointing to Alan Cox's new "voodoo" driver which is now included in XFree86
  4.3.0-62 and later builds in Fedora development.  Mapped their PCI IDs to
  the new Cards entry in pcitable.
- Updated the entries for 3Dfx Banshee

* Mon Feb 23 2004 Bill Nottingham <notting@redhat.com> 0.109-1
- pci.ids and other updates

* Thu Feb 19 2004 Mike A. Harris <mharris@redhat.com> 0.108-1
- Added Shamrock C407L to MonitorsDB for bug (#104920)

* Thu Feb 19 2004 Mike A. Harris <mharris@redhat.com> 0.107-1
- Massive Viewsonic monitor update for MonitorsDB (#84882)

* Fri Feb 13 2004 John Dennis <jdennis@finch.boston.redhat.com> 0.106-1
- fix typo, GP should have been HP

* Thu Jan 29 2004 Bill Nottingham <notting@redhat.com> 0.105-1
- many monitor updates (#114260, #114216, #113993, #113932, #113782,
  #113685, #113523, #111203, #107788, #106526, #63005)
- add some PCMCIA cards (#113006, #112505)

* Tue Jan 20 2004 Bill Nottingham <notting@redhat.com> 0.104-1
- switch sound module mappings to alsa drivers

* Mon Jan 19 2004 Brent Fox <bfox@redhat.com> 0.103-1
- fix tab spacing

* Fri Jan 16 2004 Brent Fox <bfox@redhat.com> 0.102-1
- added an entry for ATI Radeon 9200SE (bug #111306)

* Sun Oct 26 2003 Jeremy Katz <katzj@redhat.com> 0.101-1
- add 1920x1200 Generic LCD as used on some Dell laptops (#108006)

* Thu Oct 16 2003 Brent Fox <bfox@redhat.com> 0.100-1
- add entry for Sun (made by Samsung) monitor (bug #107128)

* Tue Sep 23 2003 Mike A. Harris <mharris@redhat.com> 0.99-1
- Added entries for Radeon 9600/9600Pro/9800Pro to Cards
- Fixed minor glitch in pcitable for Radeon 9500 Pro

* Tue Sep 23 2003 Jeremy Katz <katzj@redhat.com> 0.98-1
- add VMWare display adapter pci id and map to vmware X driver

* Thu Sep 11 2003 Bill Nottingham <notting@redhat.com> 0.97-1
- bcm4400 -> b44

* Sun Sep  7 2003 Bill Nottingham <notting@redhat.com> 0.96-1
- fix provided Dell tweaks (#103892)

* Fri Sep  5 2003 Bill Nottingham <notting@redhat.com> 0.95-1
- Dell tweaks (#103861)

* Fri Sep  5 2003 Bill Nottingham <notting@redhat.com> 0.94-1
- add adaptec pci id (#100844)

* Thu Sep  4 2003 Brent Fox <bfox@redhat.com> 0.93-1
- add an SGI monitor for bug (#74870)

* Wed Aug 27 2003 Bill Nottingham <notting@redhat.com> 0.92-1
- updates from sourceforge.net pci.ids, update pcitable accordingly

* Mon Aug 18 2003 Mike A. Harris <mharris@redhat.com> 0.91-1
- Added HP monitors for bug (#102495)

* Fri Aug 15 2003 Brent Fox <bfox@redhat.com> 0.90-1
- added a sony monitor (bug #101550)

* Tue Jul 15 2003 Bill Nottingham <notting@redhat.com> 0.89-1
- updates from modules.pcimap

* Sat Jul 12 2003 Mike A. Harris <mharris@redhat.com> 0.88-1
- Update MonitorsDB for new IBM monitors from upstream XFree86 bugzilla:
  http://bugs.xfree86.org/cgi-bin/bugzilla/show_bug.cgi?id=459

* Mon Jun  9 2003 Bill Nottingham <notting@redhat.com> 0.87-1
- fusion update

* Mon Jun  9 2003 Jeremy Katz <katzj@redhat.com> 0.86-1
- pci id for ata_piix

* Wed Jun  4 2003 Brent Fox <bfox@redhat.com> 0.85-1
- correct entry for Dell P991 monitor

* Tue Jun  3 2003 Bill Nottingham <notting@redhat.com> 0.84-1
- fix qla2100 mapping (#91476)
- add dell mappings (#84069)

* Mon Jun  2 2003 John Dennis <jdennis@redhat.com>
- Add new Compaq and HP monitors - bug 90570, bug 90707, bug 90575, IT 17231

* Wed May 21 2003 Brent Fox <bfox@redhat.com> 0.81-1
- add an entry for SiS 650 video card (bug #88271)

* Wed May 21 2003 Michael Fulbright <msf@redhat.com> 0.80-1
- Changed Generic monitor entries in MonitorsDB to being in LCD and CRT groups

* Tue May 20 2003 Bill Nottingham <notting@redhat.com> 0.79-1
- pci.ids and usb.ids updates

* Tue May  6 2003 Brent Fox <bfox@redhat.com> 0.78-1
- added a Samsung monitor to MonitorsDB

* Fri May  2 2003 Bill Nottingham <notting@redhat.com>
- add Xircom wireless airo_cs card (#90099)

* Fri Apr 18 2003 Jeremy Katz <katzj@redhat.com> 0.77-1
- add generic framebuffer to Cards

* Mon Mar 17 2003 Mike A. Harris <mharris@redhat.com> 0.76-1
- Updated MonitorsDb for Dell monitors (#86072)

* Tue Feb 18 2003 Mike A. Harris <mharris@redhat.com> 0.75-1
- Change savage MX and IX driver default back to "savage" for the 1.1.27t
  driver update
  
* Tue Feb 18 2003 Brent Fox <bfox@redhat.com> 0.74-1
- Use full resolution description for Dell laptop screens (bug #80398)

* Thu Feb 13 2003 Mike A. Harris <mharris@redhat.com> 0.73-1
- Updated pcitable and Cards database to fix Savage entries up a bit, and
  change default Savage/MX driver to 'vesa' as it is hosed and with no sign
  of working in time for 4.3.0.  Fixes (#72476,80278,80346,80423,82394)

* Wed Feb 12 2003 Brent Fox <bfox@redhat.com> 0.72-1
- slightly alter the sync rates for the Dell 1503FP (bug #84123)

* Tue Feb 11 2003 Bill Nottingham <notting@redhat.com> 0.71-1
- large pcitable and pci.ids updates
- more tg3, e100

* Mon Feb 10 2003 Mike A. Harris <mharris@redhat.com> 0.69-1
- Updated pcitable and Cards database for new Intel i852/i855/i865 support

* Mon Feb 10 2003 Mike A. Harris <mharris@redhat.com> 0.68-1
- Massive update of all ATI video hardware PCI IDs in pcitable and a fair
  number of additions and corrections to the Cards database as well
  
* Wed Jan 29 2003 Brent Fox <bfox@redhat.com> 0.67-1
- change refresh rates of sny0000 monitors to use a low common denominator

* Wed Jan 29 2003 Bill Nottingham <notting@redhat.com> 0.66-1
- don't force DRI off on R200 (#82957)

* Fri Jan 24 2003 Mike A. Harris <mharris@redhat.com> 0.65-1
- Added Card:S3 Trio64V2 (Unsupported RAMDAC) entry to pcitable, pci.ids, and
  Cards database to default this particular variant to "vesa" driver (#81659)

* Thu Jan  2 2003 Bill Nottingham <notting@redhat.com> 0.64-1
- pci.ids and associated pcitable updates

* Sun Dec 29 2002 Mike A. Harris <mharris@redhat.com> 0.63-1
- Updates for GeForce 2 Go, GeForce 4 (#80209)

* Thu Dec 12 2002 Jeremy Katz <katzj@redhat.com> 0.62-2
- fix Cards for NatSemi Geode

* Thu Dec 12 2002 Jeremy Katz <katzj@redhat.com> 0.62-1
- use e100 instead of eepro100 for pcmcia

* Mon Nov 25 2002 Mike A. Harris <mharris@redhat.com>
- Complete reconstruction of all Neomagic hardware entries in Cards
  database to reflect current XFree86, as well as pcitable update,
  and submitted cleaned up entries to sourceforge

* Mon Nov  4 2002 Bill Nottingham <notting@redhat.com> 0.61-1
- move pcmcia config file here
- sort MonitorsDB, add some entries, remove dups
- switch some network driver mappings

* Tue Sep 24 2002 Bill Nottingham <notting@redhat.com> 0.48-1
- broadcom 5704 mapping
- aic79xx (#73781)

* Thu Sep  5 2002 Bill Nottingham <notting@redhat.com> 0.47-1
- pci.ids updates
- add msw's wireless card

* Tue Sep  3 2002 Jeremy Katz <katzj@redhat.com> 0.46-1
- Card entries in pcitable need matching in Cards

* Sun Sep  1 2002 Mike A. Harris <mharris@redhat.com> 0.45-1
- Update G450 entry in Cards

* Tue Aug 13 2002 Bill Nottingham <notting@redhat.com> 0.44-1
- fix some of the Dell entries
- add cardbus controller id (#71198)
- add audigy mapping
- add NEC monitor (#71320)

* Tue Aug 13 2002 Preston Brown <pbrown@redhat.com> 0.43-1
- pci.id for SMC wireless PCI card (#67346)

* Sat Aug 10 2002 Mike A. Harris <mharris@redhat.com> 0.42-1
- Change default driver for old S3 based "Miro" card for bug (#70743)

* Fri Aug  9 2002 Preston Brown <pbrown@redhat.com> 0.41-1
- fix tabs in pci.ids
- Change pci ids for the PowerEdge 4 series again...

* Tue Aug  6 2002 Preston Brown <pbrown@redhat.com> 0.39-1
- Dell PERC and SCSI pci.id additions

* Tue Aug  6 2002 Mike A. Harris <mharris@redhat.com> 0.38-1
- Removed and/or invalid entries from Cards database BLOCKER (#70802)

* Mon Aug  5 2002 Mike A. Harris <mharris@redhat.com> 0.37-1
- Changed Matrox G450 driver default options to fix bug (#66697)
- Corrected S3 Trio64V2 bug in Cards file (#66492)

* Tue Jul 30 2002 Bill Nottingham <notting@redhat.com> 0.36-1
- tweaks for Dell Remote Assisstant cards (#60376)

* Thu Jul 26 2002 Mike A. Harris <mharris@redhat.com> 0.35-1
- Updated Cards db for CT69000
- Various ATI cleanups and additions to Cards and pcitable
- Updated S3 Trio3D to default to "vesa" driver (#59956)

* Tue Jul 23 2002 Bill Nottingham <notting@redhat.com> 0.33-1
- Eizo monitor updates (#56080, <triad@df.lth.se>)
- pci.ids updates, corresponding pcitable updates
- pcilint for pcitable 

* Fri Jun 28 2002 Bill Nottingham <notting@redhat.com> 0.32-1
- switch de4x5 back to tulip

* Mon Jun 24 2002 Mike A. Harris <mharris@redhat.com> 0.31-1
- Modified ATI entries in pcitable to be able to autodetect the FireGL 8700
  and FireGL 8800 which both have the same ID, but different subdevice ID's.
  Added entries to Cards database for the 8700/8800 as well.

* Tue May 28 2002 Mike A. Harris <mharris@redhat.com> 0.30-1
- Reconfigured Cards database to default to XFree86 4.x for ALL video
  hardware, since 3.3.6 support is being removed.  Video cards not
  supported natively by 4.x will be changed to use the vesa or vga
  driver, or completely removed as unsupported.

* Tue Apr 17 2002 Michael Fulbright <msf@redhat.com> 0.14-1
- another megaraid variant

* Mon Apr 15 2002 Michael Fulbright <msf@redhat.com> 0.13-1
- fix monitor entry for Dell 1600X Laptop Display Panel

* Fri Apr 12 2002 Bill Nottingham <notting@redhat.com> 0.13-1
- more aacraid

* Tue Apr  9 2002 Bill Nottingham <notting@redhat.com> 0.12-1
- another 3ware, another megaraid

* Fri Apr  5 2002 Mike A. Harris <mharris@redhat.com> 0.11-1
- Added commented out line for some Radeon 7500 cards to Cards database.

* Tue Apr  2 2002 Mike A. Harris <mharris@redhat.com> 0.10-1
- Fixed i830 entry to use driver "i810" not "i830" which doesn't exist

* Mon Apr  1 2002 Bill Nottingham <notting@redhat.com> 0.9-1
- fix rebuild (#62459)
- SuperSavage ids (#62101)
- updates from pci.ids

* Mon Mar 18 2002 Bill Nottingham <notting@redhat.com> 0.8-2
- fix errant space (#61363)

* Thu Mar 14 2002 Bill Nottingham <notting@redhat.com> 0.8-1
- nVidia updates

* Wed Mar 13 2002 Bill Nottingham <notting@redhat.com> 0.7-1
- lots of pcitable updates

* Tue Mar  5 2002 Mike A. Harris <mharris@redhat.com> 0.6-1
- Updated Cards database

* Mon Mar  4 2002 Mike A. Harris <mharris@redhat.com> 0.5-1
- Built new package with updated database files for rawhide.

* Fri Feb 22 2002 Bill Nottingham <notting@redhat.com> 0.3-1
- return of XFree86-3.3.x

* Wed Jan 30 2002 Bill Nottingham <notting@redhat.com> 0.1-1
- initial build
