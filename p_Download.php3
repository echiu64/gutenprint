<?
require('functions.php3');
###############################################
##    Set title of this page here    ##########
$title = 'gimp-print Downloads';
###############################################
###############################################
require('standard_html_header.php3');


### Content Below  ###
# Please remember to use <P> </P> tags !  ?>

<p>Please download gimp-print from <a
href="http://sourceforge.net/project/?group_id=1537">our project page.</a></p>

<p>Debian packages are <a href="http://gimp-print.sourceforge.net/debian
">available</a> for the current stable and development releases
.  Insert the following lines into your <tt>/etc/apt/sources.list</tt>:
<PRE>
#GIMP-print
deb-src http://gimp-print.sourceforge.net/debian woody main
deb http://gimp-print.sourceforge.net/debian woody main</tt>
</PRE>
Other distributions may be available; see the archive for more details.
Official Debian packages are available from <a href="http://www.debian.org">www.debian.org</a>.</p>

<p>You may view the <a href="http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/gimp-print">CVS web interface</a>.</p>

<p>If you want to use CVS source code, these are the commands (assuming you have CVS installed.) ;

<PRE>
cvs -d:pserver:anonymous@cvs.gimp-print.sourceforge.net:/cvsroot/gimp-print login

cvs -z3 -d:pserver:anonymous@cvs.gimp-print.sourceforge.net:/cvsroot/gimp-print co print
</PRE>






<?require('standard_html_footer.php3');?>
