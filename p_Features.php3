<?
require('functions.php3');
###############################################
##    Set title of this page here    ##########
$title = 'gimp-print Features';
###############################################
###############################################
require('standard_html_header.php3');


### Content Below  ###
# Please remember to use <P> </P> tags !  ?>



<P>Features I've added to the plugin include:

<ul>
<li><p>Support for the Stylus Photo, Stylus Photo 700, and Stylus
Photo EX printer 6-color capability (these printers have light cyan
and light magenta ink, improving the printing of paler tones).  These
printers also print much faster in 720 and 1440 dpi modes, offering
various tradeoffs between print speed and quality.

<li><p>Computations are done on 16-bit lookup tables rather than
8-bit, which eliminates stair-stepping effects in pale tones.  The
stair-stepping happens because the mapping between color levels and
ink quantities is not linear, so multiple input levels will map to a
single output level if there isn't enough resolution.  With 16 bits of
resolution, this problem is eliminated.

<li><p>Use of CMY mixtures in addition to black for the black
component.  This reduces speckling effects.

<li><p>Lots of hand tuning of the dithering code to improve
smoothness.

<li><p>Lots of additional output controls.
</ul>

<p>At this point, I prefer the output of my driver in many ways to the
output of the Windows driver.  I consider the color fidelity and tonal
characteristics to be better, at the expense of increased graininess
with my driver (part of what the Windows driver appears to do is avoid
printing altogether in areas where grain is likely to show up).

<p>I've also moved all of the Gimp-specific code outside of all of the
core print engine, to make it easier to port to Ghostscript or some
other free RIP.  That hasn't actually been done yet.  One of these
days...








<?require('standard_html_footer.php3');?>
