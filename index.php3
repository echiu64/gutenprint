<?

require('functions.php3');
###############################################
##    Set title of this page here    ##########
$title = 'Gimp-Print Printer Drivers';
###############################################
###############################################
require('standard_html_header.php3');


### Content Below  ###
# Please remember to use <P> </P> tags !  ?>

<H4>High quality drivers for Canon, Epson, Lexmark, and PCL printers
for use with <a
href="http://ghostscript.sourceforge.net">Ghostscript</a>, <A
HREF="http://www.cups.org">CUPS</A>, <a
href="http://www.linuxprinting.org/foomatic.html">Foomatic</a>, and <a
href="http://www.gimp.org">the Gimp</a>.</H4>

<P>For a quick download, please visit <a
href="http://sourceforge.net/project/showfiles.php?group_id=1537">here</a>.

</p>

<H2><font color="#ff0000"><blink>Gimp-Print 4.1.7 is
released!</blink></font></h2>

<P><strong><em><font color="#ff0000">New as of May 9,
2001!</strong></em></font> Gimp-Print 4.1.7, a development release in
the 4.1 line, is out.  This features greatly improved print quality
(most notably in color fidelity), more supported printers, and other
major improvements.  Also note that the PPD files used by the native
CUPS driver are not forward or backward compatible with the PPD files
in any of the 4.0 releases.</p>

<P><strong><em><font color="#ff0000">New as of December 22,
2000!</strong></em></font> Daniele Berti has packaged up rpm's of
GhostScript 5.50 with the Gimp-Print stp driver (4.0.4).  It's
available at <a
href="http://space.tin.it/computer/livbe/linux/stp_driver/gs_stp.htm">http://space.tin.it/computer/livbe/linux/stp_driver/gs_stp.htm</a>.</p>

<P><strong><em><font color="#ff0000">New as of November 27,
2000!</strong></em></font> Gimp-Print 4.0.4 is out.  All users of the
stable 4.0 release should upgrade to 4.0.4.</p>

<P>Please check out <a
href="http://sourceforge.net/project/?group_id=1537">our
project page</a>, which contains a lot more information about us!</p>

<p>This package was first written by Michael Sweet of <a
href="http://www.easysw.com">Easy Software Products</a>.  In the summer of
1999, I (Robert Krawitz) purchased an <a
href="http://www.epson.com">Epson Stylus Photo EX</a> printer to feed
my photography hobby.  Finding no existing drivers, I adapted Mike's
Print plugin to the six-color printer, and by the end of the year
released 3.0, which went into the Gimp 1.1.  The intention was for
this to be the stable plugin in the Gimp 1.2, while development of the
Print plugin continued for later release.</p>

<p>I put the 3.1 development tree on <a
href="http://sourceforge.net">SourceForge</a>, and quickly found a
group of like-minded people who wanted to print high quality output on
inexpensive inkjet printers.  One of the main goals, which wasn't
expected to be met until late in the 3.1 cycle, was to write a
Ghostscript driver so that printing wouldn't be restricted to the
Gimp.  Imagine my surprise when Henryk "Buggs" Richter wrote one
within days!</p>

<p>In July 2000, not more than a year after I bought the EX, I was
honored to be invited to the Linux Printing Summit hosted by VA Linux
Systems.  In preparation for that, I spent long hours printing out
test images.  I went back to the last 3.0.9 release, which seemed like
such an advance at the time.  I was floored at how far we'd come in
four months!  Output that I had been impressed with using six colors
was put to shame by four color output, so that should give you an idea
what six color and variable dot size printers can do.  Just goes to
show what a group of committed people can do.</p>

<p>I came away from the Summit with lots of new ideas, and in the
month or so since the Summit, we've put in place another round of
improvements.  I think you'll really enjoy using gimp-print!</p>

<?require('standard_html_footer.php3');?>
