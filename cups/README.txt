README.txt - 09/10/2000 - CUPS Drivers based on GIMP-print
----------------------------------------------------------

This file describes the CUPS drivers based on the GIMP Print
plug-in.

All code is Copyright 1997-2000 by Easy Software Products and is
provided under the terms of the GNU General Public License.  The
licensing details are provided in the file "LICENSE.txt".


INTRODUCTION

The CUPS drivers contain all of the files needed to support
photo-quality printing on any printer supported by the GIMP
Print plug-in. You can find out more about the Common UNIX
Printing System ("CUPS"), an IPP-based printing system for
UNIX/Linux, at:

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

The second is called "genppd" which generates PPD files.
Currently these are only provided in English since the
GIMP-print drivers only report options in that language.  The
program reads printer driver information from the GIMP Print
driver database and produces a PPD file for each driver.


WHAT DRIVERS ARE INCLUDED?

Currently we support all of the GIMP-print drivers supported in
the plug-in.  The EPSON drivers also support simple printer
commands to do head cleaning, test prints, and alignment, as
well as an EPSON-specific backend that provides ink level
information to the user.

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


COMPILING FROM THE CVS SOURCES

If you downloaded a CVS snapshot of this software, this
directory will not include many of the source files that are
generated from the gimp-print source.  To create these files,
you must run the following commands from the parent directory:

   aclocal
   autoconf
   automake
   ./configure
   make cups

If this fails (because you do not have the GIMP or any of the
autoconfigure tools installed on your system), you can use the
configure script from any release, and run

   configure --disable-gimptest
   make cups


WHO DO I CONTACT TO GET HELP?

The main developer for these drivers is me, Michael Sweet, at
Easy Software Products (mike@easysw.com).  I am also the main
developer for CUPS.  Please don't expect an instant response (or
even a response within a week) as I often accumulate a backlog
of 100 or more messages each day...

The driver code is mirrored as much as possible from the main
GIMP Print tree, so the driver developers for the print plug-in
may be able to offer assistance as well.


COMMERCIAL SUPPORT

Easy Software Products provides commercial support for all of
its printing products.  Please see our web site for more
information:

    http://www.easysw.com


LEGAL STUFF

CUPS, the Common UNIX Printing System, and the CUPS logo are the
trademark property of Easy Software Products.  Please see the
CUPS software license for the terms of its use.
