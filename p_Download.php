<?
require('functions.php');
###############################################
##    Set title of this page here    ##########
$title = 'Gutenprint Downloads';
###############################################
###############################################
require('standard_html_header.php');


### Content Below  ###
# Please remember to use <P> </P> tags !  ?>

<h2>Downloading Gutenprint</h2>

<p>Please download Gutenprint from <a
href="http://sourceforge.net/project/showfiles.php?group_id=1537">our project page.</a></p>

<h3>Macintosh Users</h3>

<p>Macintosh OS X users should download files whose names end in
".dmg".  These are pre-built packages that can be installed by opening
the file and following instructions.  Please download the following
packages:</p>

<ul>

<p><li>All users should download the main package, named
"gutenprint-<?php echo $plugin_version_number; ?>.U.dmg".  This provides the Gutenprint drivers and
PPD files required to print.  This is a universal binary, for both PPC
and Intel Macintosh systems.  You do not need to download the source
package "gutenprint-<?php echo $plugin_version_number; ?>.tar.bz2".</li></p>

<p><li>Users of printers connected via USB (either directly or with
USB-parallel converters) may need to download "usbtb-1.0.15.uni.dmg".
This is an alternative USB driver that may provide better performance
and functionality with certain printers and systems.  If you are
unable to print to a directly connected printer, or printing stops
partway, this driver may solve your problem.  It <i>may</i> also help
if your printer is printing extremely slowly, but there are many
reasons why printers may be slow and this will not resolve all of
them.  This is also a universal binary.</li></p>

<p><li>Users of Macintosh OS 10.2 ("Jaguar") who wish to print from
legacy applications that generate PostScript should install
"espgs-7.07.1.ppc.dmg".  This is a PPC-only binary.  You do not need
to download the source package "espgs-7.07.1-source.tar.bz2".</li></p>

</ul>

<h3>All Other Users</h3>

<p>Users of other operating systems, such as Linux, Solaris, FreeBSD,
OpenBSD, HP-UX, etc. should download the main package, named
"gutenprint-<?php echo $plugin_version_number; ?>.tar.bz2".  This is a "source tarball"; it contains
the source code for the package and must be compiled to produce binary
executables.  The command to unpack this file is</p>

<p><pre>tar xjvf gutenprint-<?php echo $plugin_version_number; ?>.tar.bz2</pre></p>

<p>On some systems, you may have to issue the following command:</p>

<p><pre>bunzip2 -c gutenprint-<?php echo $plugin_version_number; ?>.tar.bz2 | tar xvf -</pre></p>

<p>This will create a subdirectory named
<tt>gutenprint-<?php echo $plugin_version_number; ?></tt>.  You should read the files
named <tt>README</tt> and <tt>NEWS</tt> in that subdirectory for
further instructions.

<h3>CVS access</h3>

<p>You may view the <a
href="http://gimp-print.cvs.sourceforge.net/gimp-print">CVS web
interface</a>.  This facility is known to be erratic, and we have no
control over it.</p>

<p>If you want to use CVS source code, these are the commands (assuming you have CVS installed.) ;

<PRE>
cvs -d:pserver:anonymous@gimp-print.cvs.sourceforge.net:/cvsroot/gimp-print login

cvs -z3
-d:pserver:anonymous@gimp-print.cvs.sourceforge.net:/cvsroot/gimp-print co -P print
</PRE>






<?require('standard_html_footer.php');?>
