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

<H2><p><strong><em><font color=#ff0000">Macintosh OS X
Users!</font></em></strong></H2>

<p> <a
href="http://www.allosx.com/1030154694/index_html">This article on All
OS X</a> explains how to use Gimp-Print 4.2.1 with OS X 10.2 "Jaguar".</p>

<p>Red Hat users may want to check out <a
href="http://space.tin.it/computer/wvtberti/linux/stp_driver/gs_stp.htm">this
site</a> for quick and easy setup.</p>

<p>We recommend <a href="http://www.cups.org">CUPS</a> (the Common
UNIX Printing System) for the easiest setup.</p>

<H2><font color="#ff0000">Gimp-Print 4.3.3 is released!</font></h2>

<P><strong><em><font color="#ff0000">New as of July 14,
2002!</strong></em></font> Gimp-Print 4.3.3 is released.  This is a
development release on the Gimp-Print 4.3 development line.  This
contains a new dithering algorithm, and many mostly internal changes
to the Epson family driver.
</p>

<H2><font color="#ff0000">Gimp-Print 4.2.1 is released!</font></h2>

<P><strong><em><font color="#ff0000">New as of April 26,
2002!</strong></em></font> Gimp-Print 4.2.1 is released.  This is a
stable release on the Gimp-Print 4.2 release line.  This contains
Foomatic data for the Gimp-Print IJS driver and Macintosh OS X port,
in addition to a number of bug fixes.  Please read the release notes
<a
href="http://sourceforge.net/project/showfiles.php?group_id=1537">here</a>.
</p>

<H2><font color="#ff0000">Gimp-Print 4.2 is released!</font></h2>

<P><strong><em><font color="#ff0000">New as of November 24,
2001!</strong></em></font> Gimp-Print 4.2.0, the first stable release
in the 4.2 line, is out!  Please read the release notes <a
href="http://sourceforge.net/project/showfiles.php?group_id=1537">here</a>.
</p>

<p>The Gimp-Print Project is pleased to announce the release of
Gimp-Print 4.2.0, the newest stable release of Gimp-Print.  Gimp-Print
is a suite of printer drivers for all UNIX operating systems,
supporting printers from Canon, Epson, Hewlett-Packard, Lexmark, and
compatible printers from other vendors, featuring extremely high
quality, flexibility, and integration with most common printing
systems.</p>

<p>Gimp-Print 4.2 includes a core module (libgimpprint) that forms a
common print engine supporting CUPS, Ghostscript, and Foomatic.  It
also includes the Print plug-in for the Gimp (version 1.2).  The
libgimpprint core, which is portable to all POSIX-compliant operating
systems, can also be used by custom printing solutions.</p>

<p>Gimp-Print 4.2 is a successor to the Gimp-Print 4.0 release line.
Compared to that series, Gimp-Print 4.2 offers:</p>

<ul>

<li>Support for many new printers, including Lexmark printers for the
first time</li

<li>User's and developer's manuals</li>

<li>Greatly improved print quality, particularly in color fidelity</li>

<li>Internationalization, with translations into Danish, French,
Norwegian, Polish, Swedish, and British English</li>

<li>Greatly improved packaging</li>
</ul>

<p>A full list of new features, in addition to the current list of
supported printers, may be found in the NEWS file at the top level of
the distribution.</p>

<p>Gimp-Print is supplied in source code form under the GPL (GNU
General Public License).  Please be sure to read the README and NEWS
files in the distribution.</p>

<p>We anticipate further releases in the 4.2 line, primarily to support
new printers and improved functionality in existing printers.
Alongside that, we will be working on the 4.3 development line, which
will eventually become the successor to 4.2.</p>

<p>Enjoy!</p>

<p><i>- The Gimp-Print Project Team</i></p>

<hr>

<P>Please visit <a
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

<p>I came away from the Summit with lots of new ideas, and in November
2000 we released Gimp-Print 4.0, the culmination of 9 months of work
by the team.  The quality was already tremendously improved over what
we could do at the Printing Summit.</p>

<p>We started serious work on 4.1 in December, 2000.  Despite the fact
that 4.2 is a "minor" release over 4.0, there are vast improvements:
the driver is built as a shared library, making it much easier to
write higher layer software to, the packaging system follows GNU
standards, the print quality (in particular, color accuracy, a
well-known weakness in 4.0) is even better, performance is better,
there are more options, and more printers supported.  <a
href="http://www.linuxplanet.com/linuxplanet/opinions/3689/1/">This
article in LinuxPlanet</a> is one user's take on it.</a>

<p>We're not done.  We're going to start work on 4.3, which will
become 5.0.  We hope to implement proper color management, improved
dithering, and improve the performance (which is another known weak
point, although there are a lot of tradeoffs between performance and
quality that you can choose from).</p>

<p>We think you'll really enjoy using gimp-print!</p>

<?require('standard_html_footer.php3');?>
