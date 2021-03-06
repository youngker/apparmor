# norootforbuild

Name:		apparmorapplet-gnome
Version:	0.8
Release:	1
URL:		http://forge.novell.com/modules/xfmod/project/?apparmor
BuildRequires:	gnome-common gnome-desktop-devel gnome-panel-devel 
%if %suse_version > 1010
BuildRequires:  dbus-1-glib-devel
%else
BuildRequires:  dbus-1-devel dbus-1-glib
%endif
Group:		System/GUI/GNOME
Requires:	apparmor-dbus
BuildRoot:	%{_tmppath}/%{name}-%{version}-build
Source0:	%{name}-%{version}.tar.bz2
Summary:	An AppArmor event notification applet for GNOME
License:	GPL

%description
This taskbar applet receives AppArmor events over DBUS, and notifies
the user when AppArmor prevents an application from functioning.

%prep
%setup -q

%build
autoreconf -f -i

%configure --prefix=%{_prefix} --libexecdir=%{_prefix}/lib/apparmorapplet
make %{?jobs:-j%jobs}

%install
%makeinstall

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr (-, root, root)
%doc AUTHORS COPYING ChangeLog NEWS README
%{_libdir}/bonobo/servers/*.server
%{_prefix}/lib/apparmorapplet
%{_datadir}/pixmaps/*

%changelog
* Wed Jul 25 2007 - mbarringer@suse.de
- Initial package creation
