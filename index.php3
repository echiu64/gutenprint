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

<P>A project to enhance the Print plugin for the GIMP.</P>

<P><strong>We have fixed the download page!  Our apologies for not
including a working download link.</strong></p>

<P>Please check out <a
href="https://sourceforge.net/project/?group_id=1537">our
project page</a>, which contains a lot more information about us!</p>

<p><a href="http://www.gimp.org">The Gimp</a> is an image editor and
manipulator comparable in many ways to <a
href="http://www.adobe.com">Adobe</a> Photoshop.  Michael Sweet of <a
href="http://www.easysw.com">Easy Software</a> wrote a very nice print
plugin for The Gimp. In the summer of 1999, Robert Krawitz purchased
an <a href="http://www.epson.com">Epson Stylus Photo EX</a> printer,
and finding that the driver didn't fully support this excellent
printer, had a go at hacking it.</p>

<p>After enhancing the dithering algorithm to support the 6-color inks
used by the Stylus Photo EX and to improve output quality in general,
Robert began to add additional features to the plugin to allow the
user more control over the output.</p>

<p>After stabilizing version 3.0 in preparation for releasing The Gimp
version 1.2, we started work on version 3.1.  This developers' version
will form the basis for version 3.2, which will be the next stable
release.<p>

<p>Bookmark this site, and keep checking back for more stuff! While we
have not done a public release of version 3.1 yet, due to the rapid
change of the source base, we hope to do one in the fairly near
future. Our CVS repository is publicly readable, so you're welcome to
download it and experiment.</p>

<p>A major goal of version 3.1 is to use the identical source base as
the core of a GhostScript driver, and perhaps as a plugin or filter to
other printing systems. Henryk "Buggs" Richter has already
demonstrated a prototype, and he reports excellent print quality.</p>

<?require('standard_html_footer.php3');?>
