README.txt - 07/30/2000 - CUPS Driver Development Kit
-----------------------------------------------------

This file describes the CUPS driver development kit, which is
currently being provided as part of the GIMP Print plug-in
development tree.  At some point this code may be separated or
replace the driver code in GIMP Print.

All code is Copyright 1997-2000 by Easy Software Products and is
provided under the terms of the GNU General Public License.  The
licensing details are provided in the CUPS license file
"LICENSE.txt".


INTRODUCTION

The CUPS driver development kit contains all of the files needed
to support the development of printer drivers for the Common
UNIX Printing System ("CUPS"), an IPP-based printing system for
UNIX/Linux.  For more information about CUPS see:

    http://www.cups.org


WHY DRIVERS FOR CUPS?

CUPS is designed from the ground up to support printing to
modern printers.  In order to support as many applications as
possible, CUPS provides a PostScript RIP (currently based on GNU
GhostScript 5.50) as well as an image file RIP and many file
filters that handle conversion of files to a format usable by a
printer driver.  The filter interface is extensible to support
new types of files that can be printed (e.g. a GNOME metafile,
etc.)  All of this filtering happens "behind the scenes" so is
transparent to the user.

In addition, CUPS uses PostScript Printer Description ("PPD")
files to describe printers, allowing applications to see the
available printer features and capabilities easily.  The CUPS
PPD files add a few additional attributes to the standard PPD
specification to support printing to non-PS printers.


WHAT TOOLS ARE INCLUDED?

Current we provide two tools for making CUPS drivers.

The first is called "calibrate" which allows you to do
super-simple color calibration of your printer drivers.  It is
an interactive program that prints several calibration images
through your driver until a final profile is produced.

The second is called "genppd" which generates PPD files in
different languages; the current release generates English,
French, German, Italian, and Spanish language PPD files.  The
program reads driver information files which describe each
printer driver.


WHAT DRIVERS ARE INCLUDED?

Currently we are only porting the EPSON printer drivers to
CUPS.  More will follow once the EPSON drivers stablize.

The printer drivers in this distribution are so-called "raster"
printer drivers.  This means that they receive a stream of
images, one per page, that contain all of the colors, etc. for
the printer.  The printer driver then only needs to convert this
to the appropriate printer commands and raster data to get a
printed page.

We also have a new experimental interface for printer commands
using CUPS printer command files - ASCII text files with printer
commands in them.  This allows you to do a head cleaning, align
the print heads, etc.


WHAT SOFTWARE DO I NEED?

Currently, CUPS 1.1 is required, as is an ANSI C compliant
compiler like GCC.  The code has been tested on a number of
vendor compilers and should be fairly portable.


HOW DO I START USING THESE DRIVERS?

For the impatient:

    ./configure
    make
    make install
    /etc/software/init.d/cups restart (location varies on OS)

and then use the web interface to add your printer.


WHO DO I CONTACT TO GET HELP?

The main developer for these drivers is me, Michael Sweet, at
Easy Software Products (mike@easysw.com).  I am also the main
developer for CUPS.  Please don't expect an instant response (or
even a response within a week) as I often accumulate a backlog
of 100 or more messages each day...

The EPSON driver code is mirrored as much as possible from the
main GIMP Print tree, so the driver developers for the print
plug-in may be able to offer assistance as well.
