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



<p>The source for release 3.0.3 may be found <a
href="http://www.tiac.net/users/rlk/print-3.0.3.tar.gz">here</a>.

<p>The source for <b>experimental</b> release 3.0.5 may be found <a
href="http://www.tiac.net/users/rlk/print-3.0.5.tar.gz">here</a>.  This has some performance
improvements, bug fixes, and other minor improvements, but my printer
head is currently partially clogged, so I can't test everything
comprehensively!

<p>You may view the <a href="http://cvs.sourceforge.net/cgi-bin/cvsweb.cgi/?cvsroot=gimp-print">CVS web interface</a>.

<p>If you want to use CVS source code, these are the commands (assuming you have CVS installed.) ;

<PRE>
cvs -d:pserver:anonymous@cvs.gimp-print.sourceforge.net:/cvsroot/gimp-print login

cvs -z3 -d:pserver:anonymous@cvs.gimp-print.sourceforge.net:/cvsroot/gimp-print co print
</PRE>






<?require('standard_html_footer.php3');?>
