<?

require('functions.php3');
###############################################
##    Set title of this page here    ##########
$title = 'gimp-print Print Plugin';
###############################################
###############################################
require('standard_html_header.php3');


### Content Below  ###
# Please remember to use <P> </P> tags !  ?>

<H4>High quality drivers for Canon, Epson, and PCL printers for use
with <a href="http://ghostscript.sourceforge.net">Ghostscript</a>, <A
HREF="http://www.cups.org">CUPS</A>, and <a
href="http://www.gimp.org">the Gimp</a>.</H4>

<P>For a quick download, please visit <a
href="http://sourceforge.net/project/showfiles.php?group_id=1537">here</a>.

</p>

<H2><font color="#ff0000"><blink>Gimp-Print 4.0 is
released!</blink></font></h2>

<P><strong><em><font color="#ff0000">As of October 28, 2000,
Gimp-Print 4.0.0 is out.</font></em></strong> This is the initial
stable release of 4.0, following the 3.1 development cycle.  This
version contains GhostScript and CUPS drivers equal to the Print
plugin in all respects, including supported printers.  <strong>Please
read the Ghost/README file very carefully if you use the Ghostscript
driver, as the usage is completely different from all prior
releases!</strong></p>

<P>Please check out <a
href="https://sourceforge.net/project/?group_id=1537">our
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
