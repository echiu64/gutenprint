<?

$plugin_version_number = '3.1';

$maintainer = '<a href="https://sourceforge.net/developer/index.php?form_dev=5436">Robert Krawitz</a>';

$dev_team = array(
'Andy Thaller' => 'https://sourceforge.net/sendmessage.php?touser=2228',
'Karl Heinz Kremer' => 'https://sourceforge.net/sendmessage.php?touser=2818',
'Eric Sharkey' => 'https://sourceforge.net/sendmessage.php?touser=8749',
'Nicholas Piper' => 'https://sourceforge.net/sendmessage.php?touser=8275'
);

$navbar_text = 'Welcome to Gimp-Print.';

$supported_printers = array(
'Epson' => array(
	'Stylus Photo Range' => array(
		'EPSON Stylus Photo 700' => 'Fully Operational',
		'EPSON Stylus Photo EX' => 'Fully Operational',
		'EPSON Stylus Photo 750' => 'Needs Testing',
		'EPSON Stylus Photo 1200' => 'Needs Testing',
		'EPSON Stylus Photo' => 'Fully Operational'
		),
	'Stylus Color Range' => array(
		'EPSON Stylus Color' => 'Fully Operational',
		'EPSON Stylus Color Pro' => 'Fully Operational',
		'EPSON Stylus Color Pro XL' => 'Fully Operational',
		'EPSON Stylus Color 1500' => 'Fully Operational',
		'EPSON Stylus Color 400' => 'Fully Operational',
		'EPSON Stylus Color 440' => 'Needs Testing',
		'EPSON Stylus Color 500' => 'Fully Operational',
		'EPSON Stylus Color 600' => 'Fully Operational',
		'EPSON Stylus Color 640' => 'Needs Testing',
		'EPSON Stylus Color 740' => 'In Testing',
		'EPSON Stylus Color 800' => 'Fully Operational',
		'EPSON Stylus Color 850' => 'Works only at 360dpi currently.',
		'EPSON Stylus Color 900' => 'Needs Testing',
		'EPSON Stylus Color 1520' => 'Fully Operational',
		'EPSON Stylus Color 3000' => 'Fully Operational'
		)
	),
'HP' => array(
	'DeskJet Range' => array(
		'HP DeskJet 500, 520' => 'Fully Operational',
		'HP DeskJet 500C, 540C' => 'Fully Operational',
		'HP DeskJet 550C, 560C' => 'Fully Operational',
		'HP DeskJet 600 series' => 'Fully Operational',
		'HP DeskJet 800 series' => 'Fully Operational',
		'HP DeskJet 1100C, 1120C' => 'Fully Operational',
		'HP DeskJet 1200C, 1600C' => 'Fully Operational'
		),
	'LaserJet' => array(
		'HP LaserJet II series' => 'Fully Operational',
		'HP LaserJet III series' => 'Fully Operational',
		'HP LaserJet 4 series' => 'Fully Operational',
		'HP LaserJet 4V, 4Si' => 'Fully Operational',
		'HP LaserJet 5 series' => 'Fully Operational',
		'HP LaserJet 5Si' => 'Fully Operational',
		'HP LaserJet 6 series' => 'Fully Operational'
		)
	),
'Canon (<strong><em>New!</em></strong>)' => array(
       'BJC Range' => array(
	       'BJC 1000' => 'Untested',
	       'BJC 2000' => 'Untested',
	       'BJC 3000' => 'Untested',
	       'BJC 6000' => 'Operational but incomplete',
	       'BJC 6100' => 'Untested',
	       'BJC 7000' => 'Untested',
	       'BJC 7100' => 'Untested'
	       )
       ),
'Postscript' => array(
	'Postscript' => array(
		'PostScript Level 1' => 'Operational',
		'PostScript Level 2' => 'Operational'
		)
	)
);


?>
