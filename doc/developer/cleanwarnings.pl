#	-*- Mode: Perl -*-
## $Id$
## Copyright (C) 2013 Robert Krawitz
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2, or (at your option)
## any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

# Remove annoying TeX warnings from db2pdf.

$nc="";
while (<>) {
    next if /^$/;
    chomp;
  LINE:
    if (/^Overfull \\hbox|^LaTeX Font Warning:|^LaTeX Warning: Reference.*undefined on input line|^Package hyperref Warning:/) {
      $nc=" ";
      $_=<>;
      $_=<>;
      while (<>) {
	  if (! /^ *$/) {
	      chomp;
	      goto LINE;
	  }
      }
  } else {
      print "${nc}$_";
      $nc="\n";
  }
}
if ($nc ne "") {
    print "\n";
}
