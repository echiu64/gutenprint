<?
require('functions.php3');
###############################################
##    Set title of this page here    ##########
$title = 'gimp-print Supported Printers';
###############################################
###############################################
require('standard_html_header.php3');


### Content Below  ###
# Please remember to use <P> </P> tags !  ?>

<p>Please note that many of these drivers are currently under
development, and we do not necessarily have full specifications on all
of them.  We will fill in this list as we verify successful operation
of these printers.  You can help by testing this with your own printer
and reporting the results!</p>

<ul>
<?
while ( list( $key, $val ) = each( $GLOBALS['supported_printers']) ) {
	echo  '<p><li><h2>' . $key . '</h2>' ;
	echo '<ul>';
	while ( list( $ke, $va ) = each( $val ) ) {
		echo  '<li><h3>' . $ke . '</h3>' ;
		echo '<ul>';
		while ( list( $k, $v ) = each( $va ) ) {
			echo  '<li><strong>' . $k . '</strong>' ;
			echo '&nbsp;&nbsp;&nbsp;&nbsp;<em>' . $v . '</em>';
		}
		echo '</ul>';
	}
	echo '</ul>';
}
?>
</ul>

<p>The following printers are not all officially supported, but they
have been reported to work with the listed driver.  In some cases,
they may work incompletely, or they may not actually work at all.
Thanks to <a href="http://www.linuxprinting.org">linuxprinting.org</a>
for this data.</p>

<table>
<?
while ( list( $key, $val ) = each( $GLOBALS['all_known_printers']) ) {
      echo '<tr><td>' . $key . '</td><td>' . $val . '</td><td>' .
      $GLOBALS['printer_drivers'][$val] . '<td></tr>';
}
?>
</table>

<?require('standard_html_footer.php3');?>
