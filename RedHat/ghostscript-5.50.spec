Summary: A PostScript(TM) interpreter and renderer.
Name: ghostscript
%define version	5.50
%define hpdjver 2.6
%define md2kver 0.2a
%define gpver 3.1.3
Version: %{version}
Release: 1
License: GPL
URL: http://gnu-gs.sourceforge.net/
Group: Applications/Publishing
Source0: ftp://ftp.cs.wisc.edu/pub/ghost/gnu/gs550/gnu-gs-%{version}.tar.gz
Source1: ftp://ftp.cs.wisc.edu/pub/ghost/gnu/gs550/gnu-gs-%{version}jpeg.tar.gz
Source2: http://www.erdw.ethz.ch/~bonk/ftp/gs-driver-distrib/hp8xxs13.zip
Source3: ftp://ftp.sbs.de/pub/graphics/ghostscript/pcl3/hpdj-%{hpdjver}.tar.gz
Source4: ftp://ftp.cs.wisc.edu/pub/ghost/aladdin/gs550/ghostscript-%{version}gnu.tar.gz
Source5: http://plaza26.mbn.or.jp/~higamasa/gdevmd2k/gdevmd2k-%{md2kver}.tar.gz
Source6: http://lcewww.et.tudelft.nl/~haver/cgi-bin/download/linux/epson740.tgz
Source7: http://download.sourceforge.net/gimp-print/print-3.1.3.tar.gz
Patch0: ghostscript-5.50-config.patch
Patch1: gs5.50-rth.patch
Patch3: gs5.50-hp8xx.patch
Patch4: gs5.50-hpdj.patch
# Location of files used to generate patch 5:
# http://www.ultranet.com/~setaylor/papers.htm
Patch5: gs5.50-lexmark5700.patch
# Location of files used to generate patch 6:
# http://bimbo.fjfi.cvut.cz/~paluch/l7kdriver/
Patch6: gs5.50-lexmark7000.patch
Requires: urw-fonts >= 1.1, ghostscript-fonts
BuildRoot: /var/tmp/ghostscript-root

%description
Ghostscript is a set of software that provides a PostScript(TM)
interpreter, a set of C procedures (the Ghostscript library, which
implements the graphics capabilities in the PostScript language) and
an interpreter for Portable Document Format (PDF) files.  Ghostscript
translates PostScript code into many common, bitmapped formats, like
those understood by your printer or screen.  Ghostscript is normally
used to display PostScript files and to print PostScript files to
non-PostScript printers.

If you need to display PostScript files or print them to
non-PostScript printers, you should install ghostscript.  If you
install ghostscript, you also need to install the ghostscript-fonts
package.

%prep
%setup -q -n gs%{version}
%setup -q -T -D -a 1 -a 5 -b 4 -n gs%{version}
ln -s jpeg-6b jpeg
%patch0 -p1 -b .config
%patch1 -p1 -b .rth
# Add support for HP 8xx printers
%patch3 -p1 -b .hp8xx
mkdir tmp
cd tmp
unzip -a -L %{SOURCE2}
mv *.c *.h ..
rm -f *
# Add upp files for epson 740
tar xzvf %{SOURCE6}
mv epson740/*.upp ..
# Add support for some other HP printers
tar xzfO %{SOURCE3} hpdj-%{hpdjver}/hpdj.tar | tar xf -
mv *.c *.h *.1 ..
cd ..
cat tmp/contrib.mak-%{version}.add >>contrib.mak
patch -p0 -z .hpdjauto < tmp/zmedia2.c-%{version}.diff
%patch4 -p1 -b .hpdj
%patch5 -p1 -b .lxm

# Add support for ALPS printers
mv gdevmd2k-%{md2kver}/*.[ch] .
cat gdevmd2k-%{md2kver}/gdevmd2k.mak-5.50 >>contrib.mak
perl -pi -e "s/^DEVICE_DEVS6=/DEVICE_DEVS6=md2k.dev md5k.dev /g" unix-gcc.mak

%patch6 -p1 -b .lx7000

# Add gimp-print (stp) driver
cd tmp
tar xzvf %{SOURCE7}
cd print-%{gpver}/Ghost
cp gdevstp-escp2.c gdevstp-print.h gdevstp-util.c gdevstp-dither.c \
   gdevstp-printers.c gdevstp.c gdevstp.h ../../..
cat contrib.mak.addon >>../../../contrib.mak
cp -p README ../../../README.stp
cd ../../..
perl -pi -e 's/^DEVICE_DEVS6=/DEVICE_DEVS6=\$(DD)stp.dev /g' unix-gcc.mak

rm -rf tmp
ln -s unix-gcc.mak Makefile

%build
# Ugly workaround for a bug in the dj850 driver...
mkdir obj
make obj/arch.h
ln -s obj/arch.h .
make RPM_OPT_FLAGS="$RPM_OPT_FLAGS" gdevcd8.o
mv gdevcd8.o obj

%ifarch alpha
make RPM_OPT_FLAGS="" prefix=/usr
%else
make RPM_OPT_FLAGS="$RPM_OPT_FLAGS" prefix=/usr
%endif

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/{bin,man,doc}
make install prefix=$RPM_BUILD_ROOT/usr
ln -sf gs.1.gz $RPM_BUILD_ROOT/usr/man/man1/ghostscript.1.gz
ln -sf gs $RPM_BUILD_ROOT/usr/bin/ghostscript
strip -R .comment $RPM_BUILD_ROOT/usr/bin/gs
cp -p README.stp /usr/doc/ghostscript-%{PACKAGE_VERSION}

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
/usr/doc/ghostscript-%{PACKAGE_VERSION}
/usr/bin/*
%dir /usr/share/ghostscript
%dir /usr/share/ghostscript/%{PACKAGE_VERSION}
/usr/share/ghostscript/%{PACKAGE_VERSION}/*ps
/usr/share/ghostscript/%{PACKAGE_VERSION}/*upp
%config /usr/share/ghostscript/%{version}/Fontmap
/usr/share/ghostscript/%{PACKAGE_VERSION}/examples
/usr/man/*/*

%changelog
* Mon Feb 14 2000 Bernhard Rosenkraenzer <bero@redhat.com>
- 5.50 at last...
- hpdj 2.6
- Added 3rd party drivers:
  - Lexmark 5700 (lxm5700m)
  - Alps MD-* (md2k, md5k)
  - Lexmark 2050, 3200, 5700 and 7000 (lex2050, lex3200, lex5700, lex7000)

* Fri Feb  4 2000 Bernhard Rosenkraenzer <bero@redhat.com>
- rebuild to compress man page
- fix gs.1 symlink

* Wed Jan 26 2000 Bill Nottingham <notting@redhat.com>
- add stylus 740 uniprint files

* Thu Jan 13 2000 Preston Brown <pbrown@redhat.com>
- add lq850 dot matrix driver (#6357)

* Thu Oct 28 1999 Bill Nottingham <notting@redhat.com>
- oops, include oki182 driver.

* Tue Aug 24 1999 Bill Nottingham <notting@redhat.com>
- don't optimize on Alpha. This way it works.

* Thu Jul 29 1999 Michael K. Johnson <johnsonm@redhat.com>
- added hpdj driver
- changed build to use tar_cat so adding new drivers is sane

* Thu Jul  1 1999 Bill Nottingham <notting@redhat.com>
- add OkiPage 4w+, HP 8xx drivers
* Mon Apr  5 1999 Bill Nottingham <notting@redhat.com>
- fix typo in config patch.

* Sun Mar 21 1999 Cristian Gafton <gafton@redhat.com>
- auto rebuild in the new build environment (release 6)

* Mon Mar 15 1999 Cristian Gafton <gafton@redhat.com>
- added patch from rth to fix alignement problems on the alpha.

* Wed Feb 24 1999 Preston Brown <pbrown@redhat.com>
- Injected new description and group.

* Mon Feb 08 1999 Bill Nottingham <notting@redhat.com>
- add uniprint .upp files

* Sat Feb 06 1999 Preston Brown <pbrown@redhat.com>
- fontpath update.

* Wed Dec 23 1998 Preston Brown <pbrown@redhat.com>
- updates for ghostscript 5.10

* Fri Nov 13 1998 Preston Brown <pbrown@redhat.com>
- updated to use shared urw-fonts package.
* Mon Nov 09 1998 Preston Brown <pbrown@redhat.com>
- turned on truetype (ttf) font support.

* Thu Jul  2 1998 Jeff Johnson <jbj@redhat.com>
- updated to 4.03.

* Tue May 05 1998 Cristian Gafton <gafton@redhat.com>
- enabled more printer drivers
- buildroot

* Mon Apr 27 1998 Prospector System <bugs@redhat.com>
- translations modified for de, fr, tr

* Mon Mar 03 1997 Erik Troan <ewt@redhat.com>
- Made /usr/share/ghostscript/3.33/Fontmap a config file.
