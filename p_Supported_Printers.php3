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
	echo  '<li><P>' . <strong>$key</strong> ;
	echo '<ul>';
	while ( list( $ke, $va ) = each( $val ) ) {
		echo  '<li><P>' . <strong>$ke</strong> ;
		echo '<ul>';
		while ( list( $k, $v ) = each( $va ) ) {
			echo  '<li><p>' . <strong>$k</strong> ;
			echo '  ' . <em>$v<em> . '';
		}
		echo '</ul>';
	}
	echo '</ul>';
}
?>
</ul>


<?require('standard_html_footer.php3');?>
