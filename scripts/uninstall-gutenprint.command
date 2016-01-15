#!/bin/sh
################################################################################             
# A unified uninstaller for Gutenprint and Gimp-Print.                        #
# This uninstaller will uninstall versions from 4.2.1                          #
# Through present day naming as of September 1, 2015                           #
#                                                                              #
# Copyright 2001 - 2015 Michael Sweet, Matt Broughton, Tyler Blessing          #
#                                                                              #
#   This program is freed software; you can redistribute it and/or modify it   #
#   under the terms of the GNU General Public License as published by the Free #
#   Software Foundation; either version 2 of the License, or (at your option)  #
#   any later version.                                                         #
#                                                                              #
#   This program is distributed in the hope that it will be useful, but        #
#   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY #
#   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License   #
#   for more details.                                                          #
#                                                                              #
#   You should have received a copy of the GNU General Public License          #
#   along with this program; if not, write to the Free Software                #
#   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. #
#                                                                              #
################################################################################
#set -x
IFS='
'
printf "\nThis will uninstall any version of the Gutenprint/Gimp-Print \ndrivers along with any installed printers.\nIt can be used on any version of OS X from 10.2 onward.\n"
printf "\nThis script is distributed in the hope that it will be useful,\nbut WITHOUT ANY WARRANTY; without even the implied warranty\nof MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n"

#############################################################
##### 1.  Check to make sure there is an installation. ######
#############################################################

## There is no reason to check the OS X versin at this time.
## Keep the syntax in case it is needed at a later date.
#Xvers=`sw_vers -productVersion | awk -F. '{printf "%d\n", $1 * 100 + $2}'`

## If pkgutil is available (10.6 and later), check to see if pkgutil has 
## any packages to deal with.
if [ -x /usr/sbin/pkgutil ] ; then
	pkgutil_pkg=(`pkgutil --pkgs | grep -E '[Gg]utenprint|sourceforge\.[Gg]utenprint|[Gg]imp'`)
fi

## For all versions of OS X use check /Library/Receipts in case
## pkgutil is not present or if pkgutil does not recognize packages
## with non-conforming (numbers) in CFBundleIdentifier.
oldpackages=(`ls /Library/Receipts | grep -E '[Gg]utenprint|[Gg]imp[Pp]rint|[Gg]imp-[Pp]rint'`)		

## Now test that there is any installation. If not, exit 0.
if [ ! -z "${oldpackages[0]}" -o ! -z "${pkgutil_pkg[0]}" ]; then 
 	break; 
 else 
	printf "\nNo Gutenprint/Gimp-Print installation receipt found.\nThere is nothing for this script to do.\n\nIf you still think there is an installation of Gutenprint on\nyour computer, please contact Gutenprint at\n\"http://gimp-print.sourceforge.net\"\n\n"
	exit 0
fi

###########################################################
##### 2.  Make sure the user is an administrator. #########
###########################################################
authority=1
/usr/bin/id -Gn `id -un` | grep -q admin
authority=$?
if [ $authority -ne 0 ]; then
	printf "\nYou do not have sufficient privileges to run this uninstaller.\nYou must run this script from an administrator's account.\n"
	exit 0
fi

## Onwards.
read -p "Do you want to continue? (Y,n)  " continued
if [[ "$continued" = [Yy] ]] || [[ "$continued" = [Yy][Ee][Ss] ]] ; then
	printf "Please enter you administrator's password if prompted.\nNothing will appear on the screen as you enter your password.\n"
else 
exit 0
fi

#################################################################
###  3. Now we can get down to work and do the uninstall. #######
#################################################################
# Remove Gutenprint/GimpPrint packages with receipts in /Library/Receipts...
## At least OS X 10.2 lists directories along with files no matter what.
## I also lists them with top directory first.  Be sure to do a sort -r.
for pkg in ${oldpackages[@]} ; do
	echo
	echo Removing installation of $pkg....
	## Remove files
	for gutenfile in ` lsbom -sf /Library/Receipts/$pkg/Contents/Archive.bom | sed  's/^\.//g' | sort -r `  ; do
		test -f "$gutenfile" && sudo /bin/rm -f "$gutenfile"
	done
	
	## Remove any symlinks
	for gutenlink in ` lsbom -ls /Library/Receipts/$pkg/Contents/Archive.bom | sed  's/^\.//g' ` ; do
		test -L "$gutenlink" &&  sudo /bin/rm -f "$gutenlink"
	done

	## Remove the receipt
	sudo /bin/rm -rf "/Library/Receipts/$pkg"

done

test -d /usr/share/gutenprint && /bin/rm -rf /usr/share/gutenprint
test -d /Library/Printers/Gutenprint.printerDriver && /bin/rm -rf /Library/Printers/Gutenprint.printerDriver

echo Done removing receipts from /Library/Receipts.

## Now look for anything left that can be found with pkgutil
## We check so there is no error message that pkgutil was not found.
## Rerun check for pkgutil packages. We may have taken care of them
## under oldpackages removal.
if [ -x /usr/sbin/pkgutil ] ; then
	pkgutil_pkg=(`pkgutil --pkgs | grep -E '[Gg]utenprint|sourceforge\.[Gg]utenprint|[Gg]imp'`)
fi
if [ ! -z "${pkgutil_pkg[0]}" ] ; then
	for newerpkg in ${pkgutil_pkg[@]} ; do
		echo
		echo Removing installation of $newerpkg....
		for dbfile in `pkgutil --files $newerpkg --only-files`; do
			test -f /"$newerpkg" && sudo /bin/rm -f /"$newerpkg"
	done
	sudo pkgutil --forget "$newerpkg"

	# Remove symlinks...
	for file in /usr/libexec/cups/backend/gutenprint52+usb /usr/libexec/cups/driver/gutenprint.5.2  /usr/libexec/cups/filter/commandtocanon  /usr/libexec/cups/filter/commandtoepson /usr/libexec/cups/filter/rastertogutenprint.5.2 /usr/local/bin/escputil; do
		test -L $file && sudo /bin/rm -f $file
	done
	done		
fi
################################################
### 4. CLEAN THINGS UP BEFORE WE LEAVE ########
################################################
## Clean up any remaining PPDs in /Library/Printers
MODEL_PPD_DIR="/Library/Printers/PPDs/Contents/Resources"
##############################################################
## Remove any PPDs that might be lying around from a previous 
## install of some sort.
LAST_PPD=($(find ${MODEL_PPD_DIR} -name 'stp-*\.5\.[0-2]\.ppd\.gz' | sort)) 
	if [ ${#LAST_PPD[@]} -gt 0 ]; then
		echo Removing the PPDs...
		for ((jj=0;$jj < ${#LAST_PPD[@]} ; jj++)) ; do
			#echo ${LAST_PPD[$jj]}
			/bin/rm ${LAST_PPD[$jj]}
		done
	fi
	
	## Now remove any queues.  The basic awk routine is left over from 
## Tyler Blessing.
# set the CUPS ppd directory variable
CUPS_PPD_DIR="/etc/cups/ppd/"

## awk cannot handle an escaped \+ (plus sign) so use . (any character)
## in the regexp if you are going to use the + sign in the key.
QUEUE_KEY_1=.*[Gg][Uu][Tt][Ee][Nn][Pp][Rr][Ii][Nn][Tt].*5\.[012]
QUEUE_KEY_2=.*[Gg][Ii][Mm][Pp]-[Pp][Rr][Ii][Nn][Tt].*[45]\.[012]
# scan for existing Gutenprint queues...
#
# we want only the queue name so strip the leading directories and the .ppd suffix...
QUEUE=( `awk "/${QUEUE_KEY_1}/||/${QUEUE_KEY_2}/ {print FILENAME;nextfile;}" ${CUPS_PPD_DIR}* | awk '{n=split($0,a,"/"); split(a[n],b,".ppd");print b[1];}'` )
echo
echo removing the following queues... ${QUEUE[@]}
for NAME in ${QUEUE[@]}
do
# actually remove the queue
     if [  "`lpadmin -x $NAME 2>&1`" ] ; then
         echo ..........
         echo The printer queue $NAME failed to be removed.
         echo  Please delete the printer queue manually.
      else
         echo The printer queue $NAME was removed.
      fi
done

exit 0
