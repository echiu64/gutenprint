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

<ul>
<?
while ( list( $key, $val ) = each( $GLOBALS['supported_printers']) ) {
	echo  '<li><P>' . $key ;
	echo '<ul>';
	while ( list( $ke, $va ) = each( $val ) ) {
		echo  '<li><P>' . $ke ;
		echo '<ul>';
		while ( list( $k, $v ) = each( $va ) ) {
			echo  '<li><p>' . $k ;
			echo '' . $v . '';
		}
		echo '</ul>';
	}
	echo '</ul>';
}
?>
</ul>


<?require('standard_html_footer.php3');?>
