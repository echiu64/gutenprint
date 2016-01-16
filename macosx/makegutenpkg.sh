#!/bin/sh
#
# Script to make a versioned package of the Gutenprint drivers for OS X.
#
# Usage: makegutenpkg.sh version-number
#
# Example: makegutenpkg.sh 5.2.11
#

if test $# -lt 1 -o $# -gt 2; then
	echo "Usage: makegutenpkg.sh version-number [fast]"
	exit 1
fi

pkgversion="$1"
case "$pkgversion" in
	*-pre*)
		pkgnumversion="`echo $pkgversion | sed -e '1,$s/-pre/./'`"
		;;
	*)
		pkgnumversion="$pkgversion.10"
		;;
esac

# Bundles must be signed with a CA code signing certificate - the one you get
# from Apple isn't meant for direct distribution, just through the App Store
# and on your own test systems.
#
# Packages must be signed with the Mac Installer Distribution certificate from
# Apple.
if test "x$CODESIGN_IDENTITY" = x; then
	echo "Please set the CODESIGN_IDENTITY environment variable to the common name or"
	echo "serial number of an Apple Developer ID Application certificate."
	exit 1
fi

if test "x$PKGBUILD_IDENTITY" = x; then
	echo "Please set the PKGBUILD_IDENTITY environment variable to the common name or"
	echo "serial number of an Apple Developer ID Installer certificate."
	exit 1
fi

if test "x$2" != xfast; then
	# Clean build the software...
	test -f Makefile && make distclean

	LIBS="-framework IOKit -framework CoreFoundation" ./configure --prefix=/Library/Printers/Gutenprint.printerDriver/Contents/MacOS --datadir=/Library/Printers/Gutenprint.printerDriver/Contents/Resources --datarootdir=/Library/Printers/Gutenprint.printerDriver/Contents/Resources --localedir=/Library/Printers/Gutenprint.printerDriver/Contents/Resources --docdir=/Library/Printers/Gutenprint.printerDriver/Contents/Resources/doc --mandir=/Library/Printers/Gutenprint.printerDriver/Contents/Resources --disable-samples --disable-test --enable-nls-macosx --with-archflags='-mmacosx-version-min=10.6 -Os -arch i386 -arch x86_64 -D_PPD_DEPRECATED=""'
	make
fi

# Install files to a temporary directory...
pkgroot="/private/tmp/gutenprint-$pkgversion"
test -d "$pkgroot" && rm -rf "$pkgroot"

make "DESTDIR=$pkgroot" install

# command.types file is not needed on OS X...
rm -rf "$pkgroot/private"

# Move escputil to base directory, cups-calibrate is not needed on OS X...
mv "$pkgroot/Library/Printers/Gutenprint.printerDriver/Contents/MacOS/bin/escputil" "$pkgroot/Library/Printers/Gutenprint.printerDriver/Contents/MacOS"
rm -rf "$pkgroot/Library/Printers/Gutenprint.printerDriver/Contents/MacOS/bin"

# headers are not needed...
rm -rf "$pkgroot/Library/Printers/Gutenprint.printerDriver/Contents/MacOS/include"

# Move cups-genppd and cups-genppdupdate to base directory...
mv $pkgroot/Library/Printers/Gutenprint.printerDriver/Contents/MacOS/sbin/* "$pkgroot/Library/Printers/Gutenprint.printerDriver/Contents/MacOS"
rm -rf "$pkgroot/Library/Printers/Gutenprint.printerDriver/Contents/MacOS/sbin"

# Move backend, filters, and driver interface to driver bundle
mv "$pkgroot/usr/libexec/cups/backend/gutenprint52+usb" "$pkgroot/Library/Printers/Gutenprint.printerDriver/Contents/MacOS"

mv "$pkgroot/usr/libexec/cups/driver/gutenprint.5.2" "$pkgroot/Library/Printers/Gutenprint.printerDriver/Contents/MacOS"

for file in commandtocanon commandtoepson rastertogutenprint.5.2; do
	mv "$pkgroot/usr/libexec/cups/filter/$file" "$pkgroot/Library/Printers/Gutenprint.printerDriver/Contents/MacOS"
done

for file in libgutenprint.a libgutenprint.la pkgconfig; do
	rm -rf "$pkgroot/Library/Printers/Gutenprint.printerDriver/Contents/MacOS/lib/$file"
done

# Drop everything else we don't need in /usr...
rm -rf "$pkgroot/usr"

# Add an Info.plist to the printerDriver bundle...
cat >"$pkgroot/Library/Printers/Gutenprint.printerDriver/Contents/Info.plist" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleDevelopmentRegion</key>
	<string>English</string>
	<key>CFBundleDisplayName</key>
	<string>Gutenprint Printer Drivers</string>
	<key>CFBundleIdentifier</key>
	<string>org.gutenprint.printer-driver</string>
	<key>CFBundleName</key>
	<string>Gutenprint Printer Drivers</string>
	<key>CFBundlePackageType</key>
	<string>BNDL</string>
	<key>CFBundleShortVersionString</key>
	<string>$pkgversion</string>
	<key>CFBundleSignature</key>
	<string>prnt</string>
	<key>CFBundleVersion</key>
	<string>$pkgnumversion</string>
	<key>LSMinimumSystemVersion</key>
	<string>10.6</string>
	<key>NSHumanReadableCopyright</key>
	<string>Copyright 1999-2015 by The Gutenprint Team. See http://gutenprint.sf.net for details.</string>
</dict>
</plist>
EOF

# Sign the driver bundle...
for file in commandtocanon commandtoepson cups-genppd.5.2 cups-genppdupdate escputil gutenprint.5.2 gutenprint52+usb lib/libgutenprint.2.dylib rastertogutenprint.5.2; do
	codesign -s "$CODESIGN_IDENTITY" -fv "$pkgroot/Library/Printers/Gutenprint.printerDriver/Contents/MacOS/$file"
done

codesign -s "$CODESIGN_IDENTITY" -fv "$pkgroot/Library/Printers/Gutenprint.printerDriver"

# Add a scripts directory for the post-install script needed...
mkdir -p "${pkgroot}-scripts"

cat >"${pkgroot}-scripts/preinstall" <<EOF
#!/bin/sh
# Only run on 10.6 and later
osvers=\`sw_vers -productVersion | awk -F. '{printf "%d\n", \$1 * 100 + \$2}'\`
if test \$osvers -lt 1006; then
	osascript -e 'tell application "System Events" to display alert "This version of Gutenprint requires OS X 10.6 or later." as critical'
	exit 1
fi

# Remove old Gutenprint/Gimp-Print packages...
for pkg in \`pkgutil --pkgs | grep -E '[Gg]utenprint|[Gg]imp-[Pp]rint'\`; do
	for file in \`pkgutil --files \$pkg --only-files\`; do
		/bin/rm -f "/\$file"
	done

	# Use both pkgutil and rm since some versions of pkgutil are "smart"...
	pkgutil --forget \$pkg
done

## remove old fashioned receipt
IFS='
'
gutenrecpt=\`ls /Library/Receipts | grep -E '[Gg]utenprint|[Gg]imp[Pp]rint|[Gg]imp-[Pp]rint'\`
test -d "\${gutenrecpt}" && /bin/rm -rf /Library/Receipts/\$gutenrecpt

for file in \`ls -1 /usr/lib | grep gutenprint\`; do
	test -e /usr/lib/\$file && /bin/rm -f /usr/lib/\$file
done

for file in \`ls -1 /usr/libexec/cups/backend | grep gutenprint\`; do
	test -e /usr/libexec/cups/backend/\$file && /bin/rm -f /usr/libexec/cups/backend/\$file
done

for file in \`ls -1 /usr/libexec/cups/driver | grep gutenprint\`; do
	test -e /usr/libexec/cups/driver/\$file && /bin/rm -f /usr/libexec/cups/driver/\$file
done

for file in \`ls -1 /usr/libexec/cups/filter | grep gutenprint\`; do
	test -e /usr/libexec/cups/filter/\$file && /bin/rm -f /usr/libexec/cups/filter/\$file
done

for file in \`ls -1 /usr/sbin | grep cups-genppd\`; do
	test -e /usr/sbin/\$file && /bin/rm -f /usr/sbin/\$file
done

for file in \`ls -1 /usr/share/cups/model | grep '^stp-'\`; do
	test -e /usr/share/cups/model/\$file && /bin/rm -f /usr/share/cups/model/\$file
done

for file in \`ls -1 /Library/Printers/PPDs/Contents/Resources | grep '^stp-'\`; do
	test -e /Library/Printers/PPDs/Contents/Resources/\$file && /bin/rm -f /Library/Printers/PPDs/Contents/Resources/\$file
done

for file in /usr/bin/cups-calibrate /usr/bin/escputil /usr/libexec/cups/filter/commandtocanon  /usr/libexec/cups/filter/commandtoepson /usr/local/bin/escputil; do
	test -e \$file && /bin/rm -f \$file
done

test -d /usr/include/gutenprint && /bin/rm -rf /usr/include/gutenprint
test -d /usr/share/doc/gutenprint && /bin/rm -rf /usr/share/doc/gutenprint
test -d /usr/share/gutenprint && /bin/rm -rf /usr/share/gutenprint
test -d /Library/Printers/Gutenprint.printerDriver && /bin/rm -rf /Library/Printers/Gutenprint.printerDriver

# Make sure CUPS directories are still there...
for dir in /usr/libexec/cups/backend /usr/libexec/cups/driver /usr/libexec/cups/filter; do
	test -d \$dir || mkdir -p \$dir
done

exit 0
EOF
chmod +x "${pkgroot}-scripts/preinstall"

cat >"${pkgroot}-scripts/postinstall" <<EOF
#!/bin/sh

# Create symbolic links...
ln -sf /Library/Printers/Gutenprint.printerDriver/Contents/MacOS/gutenprint52+usb /usr/libexec/cups/backend

ln -sf /Library/Printers/Gutenprint.printerDriver/Contents/MacOS/gutenprint.5.2 /usr/libexec/cups/driver

ln -sf /Library/Printers/Gutenprint.printerDriver/Contents/MacOS/commandtocanon /usr/libexec/cups/filter
ln -sf /Library/Printers/Gutenprint.printerDriver/Contents/MacOS/commandtoepson /usr/libexec/cups/filter
ln -sf /Library/Printers/Gutenprint.printerDriver/Contents/MacOS/rastertogutenprint.5.2 /usr/libexec/cups/filter

test -d /usr/local/bin || mkdir -p /usr/local/bin
ln -sf /Library/Printers/Gutenprint.printerDriver/Contents/MacOS/escputil /usr/local/bin

# Run cups-genppdupdate to update any Gutenprint PPD files...
echo Updating Gutenprint printer queues...
/Library/Printers/Gutenprint.printerDriver/Contents/MacOS/cups-genppdupdate
EOF
chmod +x "${pkgroot}-scripts/postinstall"

# Create a component plist...
cat >"${pkgroot}.plist" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<array>
	<dict>
		<key>BundleIsRelocatable</key>
		<false/>
		<key>BundleIsVersionChecked</key>
		<true/>
		<key>BundleOverwriteAction</key>
		<string>upgrade</string>
		<key>RootRelativeBundlePath</key>
		<string>Library/Printers/Gutenprint.printerDriver</string>
	</dict>
</array>
</plist>
EOF

# Now create a modern OS X package inside a disk image...
test -d ~/Desktop/gutenprint-$pkgversion && rm -rf ~/Desktop/gutenprint-$pkgversion
mkdir -p ~/Desktop/gutenprint-$pkgversion

echo pkgbuild --root "$pkgroot" --component-plist "${pkgroot}.plist" --scripts "${pkgroot}-scripts" --identifier org.gutenprint.printer-driver --version $pkgversion --sign "$PKGBUILD_IDENTITY" ~/Desktop/gutenprint-$pkgversion/gutenprint-$pkgversion.pkg
pkgbuild --root "$pkgroot" --component-plist "${pkgroot}.plist" --scripts "${pkgroot}-scripts" --identifier org.gutenprint.printer-driver --version $pkgversion --sign "$PKGBUILD_IDENTITY" ~/Desktop/gutenprint-$pkgversion/gutenprint-$pkgversion.pkg

cp AUTHORS ~/Desktop/gutenprint-$pkgversion/Authors.txt
cp COPYING ~/Desktop/gutenprint-$pkgversion/Copying.txt
cp NEWS ~/Desktop/gutenprint-$pkgversion/News.txt

cp scripts/uninstall-gutenprint.command ~/Desktop/gutenprint-$pkgversion/
chmod 755 ~/Desktop/gutenprint-$pkgversion/uninstall-gutenprint.command

cp -r "scripts/Gutenprint Utility for EPSON Inkjet Printers" ~/Desktop/gutenprint-$pkgversion/

test -f ~/Desktop/gutenprint-$pkgversion.dmg && rm -f ~/Desktop/gutenprint-$pkgversion.dmg
hdiutil create -srcfolder ~/Desktop/gutenprint-$pkgversion ~/Desktop/gutenprint-$pkgversion.dmg
