Summary: The gimp-print plugin
Name: gimp-print
Version: 3.1.3
Release: 1
Copyright: GPL
Group: Applications/Multimedia
Source0: http://download.sourceforge.net/gimp-print/print-3.1.3.tar.gz
BuildRoot: /var/tmp/%{name}-%{version}-%{release}-root

%description
This is the Print plug-in for the Gimp.

This is a development release, and as such, not all printers or
combinations of options will work.  Please check our web site at
http://gimp-print.sourceforge.net for details about what is and is not
supported.

%prep
%setup -q -n print-%{version}

%build
sh configure
perl -pi -e 's,-O2,\$(RPM_OPT_FLAGS),' Makefile

make

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/local/bin $RPM_BUILD_ROOT/usr/lib/gimp/1.0/plug-ins

make prefix=$RPM_BUILD_ROOT/usr/local install-binPROGRAMS
/usr/bin/install -c print $RPM_BUILD_ROOT/usr/lib/gimp/1.0/plug-ins/print

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
/usr/local/bin/escp2-weavetest
/usr/local/bin/pcl-unprint
/usr/local/bin/unprint
/usr/lib/gimp/1.0/plug-ins/print

%changelog
* Fri Apr 28 2000 Mark Hindess <rpm@beanz.uklinux.net>
- First attempt at gimp-print rpm
