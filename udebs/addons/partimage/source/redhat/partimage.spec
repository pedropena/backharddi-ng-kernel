%define	name	partimage
%define	ver	0.6.2
%define	rel	1
%define	prefix	/usr
%define	sbindir	/sbin
%define mandir	/usr/man
%define aclocaldir	/usr/share/aclocal

Summary		: Partition Image
Name		: %{name}
Version		: %{ver}
Release		: %{rel}
Source		: partimage-%{ver}.tar.bz2
URL             : http://www.partimage.org/
Buildroot	: %{_tmppath}/%{name}-root
Packager	: Francois Dupoux <fdupoux@partimage.org>
Copyright	: GPL
Group		: Applications/System
%description
Partition Image is a Linux/UNIX partition imaging utility: it saves all used
blocks in a partition to an image file. This image file can be compressed
using gzip or bzip2 compression to save space, and even split into multiple
files to be copied to movable media such as Zip disks or CD-R.

The following partition types are supported:

  - Ext2FS   (the Linux standard)
  - ReiserFS (a new, powerful journalling file system)
  - NTFS     (Windows NT File System)
  - FAT16/32 (DOS & Windows file systems)
  - HPFS     (OS/2 File System)

This allows you to back up a full Linux/Windows system with a single
operation. When problems such as viruses, crashes, or other errors occur, you
just have to restore, and after several minutes your system can be restored
(boot record and all your files) and fully working.

This is also very useful when installing the same software on many machines:
just install one of them, create an image, and just restore the image on all
other machines. Then, after the first one, each machine installation can take
just minutes.

%prep

%setup
%build
CFLAGS="$RPM_OPT_FLAGS" CXXFLAGS="$RPM_OPT_FLAGS" ./configure --prefix=%{prefix} --with-sysconfdir=/etc --with-log-dir=/var/log --disable-ssl --enable-all-static
make

%install
rm -rf "$RPM_BUILD_ROOT"
make DESTDIR="$RPM_BUILD_ROOT" install
strip "${RPM_BUILD_ROOT}%{prefix}"/sbin/partimage
strip "${RPM_BUILD_ROOT}%{prefix}"/sbin/partimaged
cd $RPM_BUILD_ROOT
find . -type d | sed '1,2d;s,^\.,\%attr(-\,root\,root) \%dir ,' > $RPM_BUILD_DIR/file.list.%{name}
find . -type f | sed 's,^\.,\%attr(-\,root\,root) ,' >> $RPM_BUILD_DIR/file.list.%{name}
find . -type l | sed 's,^\.,\%attr(-\,root\,root) ,' >> $RPM_BUILD_DIR/file.list.%{name}

%clean
rm -rf "$RPM_BUILD_ROOT"

%files -f ../file.list.%{name}

