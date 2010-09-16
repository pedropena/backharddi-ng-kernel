Summary: A record/playback proxy for VNC.
Name: rfbproxy
Version: 1.1.1
Release: 0.1
License: GPL
Group: Applications/System
URL: http://rfbproxy.sourceforge.net/
Source0: http://prdownloads.sourceforge.net/rfbproxy/%{name}-%{version}.tar.bz2?download
Buildroot: %{_tmppath}/%{name}-%{version}-buildroot

%description
The rfbproxy package is for recording VNC sessions for later playback.
It can record screen updates (from the server) or mouse/keyboard
events (from the viewer).  Use rfbplaymacro to replay mouse/keyboard
events.

%prep
%setup -q

%build
%configure
make CFLAGS="$RPM_OPT_FLAGS"

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/%{_bindir}
mkdir -p %{buildroot}/%{_mandir}/man1
install -m 755 rfbproxy $RPM_BUILD_ROOT/%{_bindir}/rfbproxy
install -m 644 rfbproxy.1 $RPM_BUILD_ROOT/%{_mandir}/man1/rfbproxy.1

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root)
%doc README COPYING TODO
%{_bindir}/rfbproxy
%{_mandir}/*/*

%changelog
* Fri Aug 26 2005 Brent Baccala <baccala@freesoft.org>
- changed links to point to rfbproxy.sourceforge.net

* Fri May 20 2005 Tim Waugh <twaugh@redhat.com>
- Ship TODO, not BUGS.
- Use the bz2 source file.

* Wed Jan  9 2002 Tim Waugh <twaugh@redhat.com>
- Run configure first.

* Fri May  4 2001 Tim Waugh <twaugh@redhat.com>
- Version 0.6.2.  Autoconficated by Marko Kreen.

* Wed Feb 28 2001 Tim Waugh <twaugh@redhat.com>
- Version 0.6.1.

* Fri Sep  1 2000 Tim Waugh <twaugh@redhat.com>
- Correct source URL.
- Add doc files.

* Tue Aug  8 2000 Tim Waugh <twaugh@redhat.com>
- Rebuild without -ggdb.

* Tue Aug  8 2000 Tim Waugh <twaugh@redhat.com>
- Version 0.6.0.

* Mon Aug  7 2000 Tim Waugh <twaugh@redhat.com>
- RPM packaging guide changes

* Wed Aug  2 2000 Tim Waugh <twaugh@redhat.com>
- Use RPM_OPT_FLAGS
- Install man page

* Tue Jul 25 2000 Tim Waugh <twaugh@redhat.com>
- Use version macro in source name
- Version 0.5.1.

* Fri Jul 21 2000 Tim Waugh <twaugh@redhat.com>
- Version 0.5.0.

* Thu Jul 6 2000 Tim Waugh <twaugh@redhat.com>
- Created
