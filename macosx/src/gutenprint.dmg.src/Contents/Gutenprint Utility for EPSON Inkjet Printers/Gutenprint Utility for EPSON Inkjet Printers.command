#!/bin/sh

################################################################################
#                                                                              #
#   Product name:   Gutenprint Epson Utility                                   #
#        version:   1.0.4                                                      #
#                                                                              #
#	Copyright 2007-2008 by Matt Broughton <walterwego@macosx.com>              #
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
#                                                                              #
################################################################################

### This program is intended to provide a user interface for the 
### Gutenprint `escputil` Epson utility.

#set -x
EPSON_PRINTERS=( )
PPD_DIR=/etc/cups/ppd
if [ -x /usr/bin/escputil ]; then
	Xescputil=/usr/bin/escputil
else
	Xescputil=/usr/local/bin/escputil
fi


#######################################################################
## Need to declare functions before they are called.
## Functions to perform the available tasks.  Format for the functions
## is to take two variables -- the queue name and model.

## Check to make sure the printer is idle.  Allow up to 50 seconds and give up.
do_printer_status () {
busy=1
for ((bb=1;$bb < 6 ; bb++)) ; do
lpstat -p "$1" | grep -q "idle"
busy=$?
if [ $busy -eq 1 ] ; then
	if [ $bb -lt 5 ] ; then
	printf "\nPrinter $1 is busy or not excepting jobs.\nWill try again in 10 seconds.\n\n"
	sleep 10
	else
	printf "\nStill cannot access printer.  Unable to procede.\nGoodbye.\n\n"
	exit -1
	fi
else
break;
fi
done
}

do_disclaimer () {
if [ \! -x "${Xescputil}" ] ; then
printf "\n*** ERROR: cannot find /usr/local/bin/escputil.\n"
exit -1
else
printf "escputil --- Copyright 2000-2006 Robert Krawitz (rlk@alum.mit.edu)\nThis program is distributed in the hope that it will be useful, but\nWITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY\nor FITNESS FOR A PARTICULAR PURPOSE.\n\n"
#"${Xescputil}" -l
fi
}

do_nozzle () {
do_printer_status "$1"
echo
echo Starting nozzle check for "$2". . .
echo
"${Xescputil}" -P "$1" -m `grep '^\*StpDriverName' "${PPD_DIR}"/"$1".ppd | awk '{print $2}' | sed 's/"//g'` -n
}

do_clean () {
do_printer_status "$1"
echo
echo Starting head cleaning for "$2". . .
echo
"${Xescputil}" -P "$1" -m `grep '^\*StpDriverName' "${PPD_DIR}"/"$1".ppd | awk '{print $2}' | sed 's/"//g'` -c
}


do_align () {
do_printer_status "$1"
echo
echo Starting alignment procedure for "$2". 
echo Please follow the instructions. . .
echo
"${Xescputil}" -P "$1" -m `grep '^\*StpDriverName' "${PPD_DIR}"/"$1".ppd | awk '{print $2}' | sed 's/"//g'` -a
}

do_license () {
echo
"${Xescputil}" -l
}

#####################################################################
### Test to see if any of the printer queues have Epson v5.x.x
### PPDs.

### Sort through the printer queues in /etc/cups/ppd
### Single out Epson Gutenprint printers using v5.x.x PPDs
### TO DO (24 August, 2007) take into account v4.2.x queues if suitable 
### escputil and libraries are present.

### Using 2 strings for matching will take a bit of extra time, but it allows
### the greatest backward compatibility.  It will support everything from
### v5.0.0-rc1 through v5.9.9 if we don't use the "-" in the version string.
KEY_1=[Ee][Pp][Ss][Oo][Nn].*[Gg][Uu][Tt][Ee][Nn][Pp][Rr][Ii][Nn][Tt].*v5\.[0-9]\.[0-9]
KEY_2=[Ee][Pp][Ss][Oo][Nn].*[Gg][Uu][Tt][Ee][Nn][Pp][Rr][Ii][Nn][Tt].*v5\.0\.0-rc

EPSON_PRINTERS_PREQUALIFY=($(awk -v key1="${KEY_1}" -v key2="${KEY_2}" '/NickName/ && ( $0 ~ key1 || $0 ~ key2) {oldORS=ORS;ORS=".";n=split( FILENAME,a,"/");m=split(a[n],b,".");for (i=1;i<m-1;i++) print b[i];ORS=oldORS;print b[i];nextfile;}' "${PPD_DIR}"/*))
#echo prequalified are ${EPSON_PRINTERS_PREQUALIFY[@]}

## Make sure that the printers are not laser printers and require an
## stp-escp2 PPD
for ii in ${EPSON_PRINTERS_PREQUALIFY[@]} ; do
cat "${PPD_DIR}"/$ii.ppd | grep -q "StpDriverName.*escp2"
inkjet=$?
if [ $inkjet -eq 0 ] ; then
EPSON_PRINTERS=( "${EPSON_PRINTERS[@]}" "${ii}" )
#echo printer list is: ${EPSON_PRINTERS[@]}
fi
done

#echo Epson Printers are: ${EPSON_PRINTERS[@]}

if [ ${#EPSON_PRINTERS[@]} -eq 0 ] ; then
## No eligible printers found
printf "\nNo supported Epson printers found.\n"
exit 0
elif [ ${#EPSON_PRINTERS[@]} -eq 1 ] ; then
## One eligible printer found
NAME=${EPSON_PRINTERS[0]}
MODEL=`awk -F\" '/ModelName/{print $2}' "${PPD_DIR}"/${NAME}.ppd`
elif 
[ ${#EPSON_PRINTERS[@]} -gt 1 ] ; then
## Multiple eligible printers found
printf "\nPlease enter the number associated with the printer you want use.\n"
## Validate the input and present the list until the user makes
## a proper selection.
validate=1
until [ $validate -eq 0 ] ; do
select NAME in "${EPSON_PRINTERS[@]}" "None of the above" ; do
[ "${NAME}" != "" ] 
validate=$?
	if [ $validate -eq 0 ] ; then
	
	if [ "${NAME}" = "None of the above" ] ; then
	printf "\nNo printer chosen.  Exiting.\n"
	exit 0
	fi
	## get model name for confirmation
MODEL=`awk -F\" '/ModelName/{print $2}' "${PPD_DIR}"/${NAME}.ppd`
	break;
	else
	printf "\nInvalid entry, please try again.\nEnter the list item number for the printer you want to use.\n"
	break;
	fi
done
done
	
fi


duty=
until [[ "$duty" = [Qq] ]] ; do
clear
do_disclaimer
## make things look interesting.
printf "* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *"

## Find out what the user wants to do
printf "\nThe following procedures will assume that your printer is an \n${MODEL}.  If this is not your printer model,\nplease choose (q)uit at the prompt.\n"

printf "\nThe following actions are available.\n   (n)ozzle check\n   (c)lean print heads\n   (a)lign print heads\n   (l)icense -- display license\n   (q)uit\n"
read -p "What do you want to do (n, c, a, l, q)? " duty
#echo action to perform: $duty
if [ "$duty" = "n" ]; then
do_nozzle "${NAME}" "${MODEL}"
## make things look interesting.
for ((jj=1;$jj < 30 ; jj++)) ; do
sleep 0.25
printf " ="
done
elif [ "$duty" = "c" ] ; then
do_clean "${NAME}" "${MODEL}"
## make things look interesting.
for ((jj=1;$jj < 21 ; jj++)) ; do
sleep 1
printf " ="
done
elif [ "$duty" = "a" ] ; then
do_align "${NAME}" "${MODEL}"
elif [ "$duty" = "l" -o "$duty" = "escputil -l" -o "$duty" = "escputil" ] ; then
clear
do_license
printf "\n* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\n"
sleep 10
elif [ "$duty" = "q" ] ; then
exit 0
fi
done
fi
