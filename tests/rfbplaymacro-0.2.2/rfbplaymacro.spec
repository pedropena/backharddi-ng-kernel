Summary: A remote control tool for VNC.
Name: rfbplaymacro
Version: 0.2.2
Release: 0.1
License: GPL
Group: Applications/System
URL: http://cyberelk.net/tim/rfbplaymacro/
Source0: ftp://cyberelk.net/tim/data/rfbplaymacro/stable/%{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-buildroot

%description
This is a tool for programmatically controlling a VNC session.

%prep
%setup -q

%build
%configure
make

%install
rm -rf %{buildroot}
%makeinstall

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root)
%doc README NEWS ChangeLog AUTHORS
%{_bindir}/rfbplaymacro

%changelog
* Sun Jul  7 2002 Tim Waugh <twaugh@redhat.com>
- Initial spec file.
