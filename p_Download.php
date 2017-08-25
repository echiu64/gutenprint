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
the file and following instructions.  You can find information and download links for the latest release at <a href="http://gimp-print.sourceforge.net/MacOSX.php">http://gimp-print.sourceforge.net/MacOSX.php</a></p>
<ul>
<p><li>Users of printers connected via USB (either directly or with
USB-parallel converters) may want to download "usbtb-1.0.17.uni.dmg".
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

<h3>Git access</h3>

<p>If you want to use the current development, you can clone the
repository:</p>

<pre>
git clone git://git.code.sf.net/p/gimp-print/source gimp-print-source
</pre>

<p>To view the repository, visit <a href="https://sourceforge.net/p/gimp-print/source/ci/master/tree/">https://sourceforge.net/p/gimp-print/source/ci/master/tree/</a></p>

<?require('standard_html_footer.php');?>
