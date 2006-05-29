<?
require('functions.php3');
###############################################
##    Set title of this page here    ##########
$title = 'Gutenprint Downloads';
###############################################
###############################################
require('standard_html_header.php3');


### Content Below  ###
# Please remember to use <P> </P> tags !  ?>

<p>Please download Gutenprint from <a
href="http://sourceforge.net/project/?group_id=1537">our project page.</a></p>

<p>You may view the <a href="http://gimp-print.cvs.sourceforge.net/gimp-print">CVS web interface</a>.</p>

<p>If you want to use CVS source code, these are the commands (assuming you have CVS installed.) ;

<PRE>
cvs -d:pserver:anonymous@gimp-print.cvs.sourceforge.net:/cvsroot/gimp-print login

cvs -z3
-d:pserver:anonymous@gimp-print.cvs.sourceforge.net:/cvsroot/gimp-print co -P print
</PRE>






<?require('standard_html_footer.php3');?>
