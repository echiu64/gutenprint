<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 3.2 Final//EN">
<HTML>
<HEAD><TITLE><? echo ($title) ?></TITLE></HEAD>
<META NAME="CLASSIFICATION" CONTENT="General HTML">
<META NAME="Author" CONTENT="The Gimp Print Project">
<META NAME="KEYWORDS" CONTENT="The Gimp, Gimp, Print, Printers, Linux,
Free Software, Epson, Canon, HP, PCL, PostScript, ESCP">
<META NAME="ABSTRACT" CONTENT="The Gimp Print Project"> 
<BODY BGCOLOR="#FFFFFF">
<STYLE TYPE="text/css">
<!-- /* Hide content from old browsers */

P {
	font-size: 10pt;
	font-family: Geneva, sans-serif;
	color: #000000;
}


H1 {
	font-family: Geneva, sans-serif;
	color: #999999;
}


/* end hiding content from old browsers */ -->
</STYLE>
<table border="0" width="100%" cellspacing="0"><tr><td bgcolor="#eeeeee" width="100%">
&#160;
</td></tr></table>

<TABLE BORDER="0">
<TR><TD>
<? includeImage('top_image','Welcome to the gimp-print Website') ?>
</TD><TD>
<P>
<? echo($maintainer) ?> 
<?
while ( list( $key, $val ) = each( $GLOBALS['dev_team']) ) {
   echo '<a href="' . $val . '">' . $key . '</a> ';
}
?>

<br>
based on version 2.0 by <a href="mailto:mike@easysw.com">Michael Sweet</a><br>
<br>
<A href="https://sourceforge.net/project/?group_id=1537">SourceForge Interface</a>
</P>
</TD></TR>
</TABLE>
<? echo(navbar($navbar_text)) ?>
<table border="0" width="100%" cellspacing=40><tr><td>

