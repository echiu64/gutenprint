<?
require('state.php');

function includeImage ($imageName,$altString = "Image",$border="0",$misc="") {
	$imageName = ereg_replace("^","images/",$imageName);
	if ( file_exists($imageName) ) {includeTHISImage($imageName,$altString,$border,$misc);
		return ;}

	$newImageName = ereg_replace("$",".png",$imageName);
	if ( file_exists($newImageName) ) {includeTHISImage($newImageName,$altString,$border,$misc);return ;}

	$newImageName = ereg_replace("$",".gif",$imageName);
	if ( file_exists($newImageName) ) {includeTHISImage($newImageName,$altString,$border,$misc);return ;}

	$newImageName = ereg_replace("$",".jpg",$imageName);
	if ( file_exists($newImageName) ) {includeTHISImage($newImageName,$altString,$border,$misc);return ;}

	$newImageName = ereg_replace("$",".jpeg",$imageName);
	if ( file_exists($newImageName) ) {includeTHISImage($newImageName,$altString,$border,$misc);return ;}

	echo '[IMAGE NOT AVAILABLE (' . $imageName . ')]';
}

function includeTHISImage($imageName,$altString,$border,$misc) {
	$size = GetImageSize($imageName);
	printf('<IMG SRC="%s" %s ALT="%s [%.02f KB]" BORDER="%s" %s>',$imageName,$size[3],$altString,filesize($imageName) / 1024,$border,$misc);
}



function navbar($navbar_text='') {
	$handle=opendir('.');
	
	$returning = '<table border="0" width="100%" cellspacing="0"><tr><td bgcolor="#eeeeee"><table border="0"><TR>';

	$filename = "index.php";
	$destname = "Home";

	if (ereg($filename,$GLOBALS['SCRIPT_NAME'])) {
		$returning = $returning . '<TD BGCOLOR="#dedede"><P>' .  '<font color="#aaaaaa">' . $destname .  "</font></P></TD>";
	} else {
		$returning = $returning . '<TD BGCOLOR="#dedede"><P>' .'<A href="' . $filename . '">' . $destname .  "</A></P></TD>";
	}


	while ($filename = readdir($handle)) {
		if ( is_file($filename) & ( ereg("^p_.*php$",$filename) ) ) {
			$destname = ereg_replace(".php$","",$filename);
			$destname = ereg_replace("^p_","",$destname);
			$returning = $returning . '<TD BGCOLOR="#eeeeee"><font color="#dddddd">|-|</font></TD>';
			if (ereg($filename,$GLOBALS['SCRIPT_NAME'])) {
				$returning = $returning . '<TD BGCOLOR="#dedede"><P>' .  '<font color="#aaaaaa">' . $destname .  "</font></P></TD>";
			} else {
				$returning = $returning . '<TD BGCOLOR="#dedede"><P>' .'<A href="' . $filename . '">' . $destname .  "</A></P></TD>";
			}
		}
		
	}

	$returning = $returning . '</TR></TABLE></td><td bgcolor="#eeeeee"><p><font color="#d0d0d0">' . $navbar_text . '</font></p></td></tr></table>';

	return $returning;

}
?>
