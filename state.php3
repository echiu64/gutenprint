<?

$plugin_version_number = '4.2.0';

$maintainer = '<a href="http://sourceforge.net/developer/index.php?form_dev=5436">Robert Krawitz</a>';

$navbar_text = 'Welcome to Gimp-Print.';

$supported_printers = array(
'Epson' => array(
	'Stylus Photo Range' => array(
		'EPSON Stylus Photo' => 'Fully Operational',
		'EPSON Stylus Photo 700' => 'Fully Operational',
		'EPSON Stylus Photo EX' => 'Fully Operational',
		'EPSON Stylus Photo 720' => 'Fully Operational',
		'EPSON Stylus Photo 750' => 'Fully Operational',
		'EPSON Stylus Photo 780' => 'Fully Operational',
		'EPSON Stylus Photo 790' => 'Fully Operational',
		'EPSON Stylus Photo 810' => 'Fully Operational',
		'EPSON Stylus Photo 820' => 'Fully Operational',
		'EPSON Stylus Photo 870' => 'Fully Operational',
		'EPSON Stylus Photo 890' => 'Fully Operational',
		'EPSON Stylus Photo 1200' => 'Fully Operational',
		'EPSON Stylus Photo 1270' => 'Fully Operational',
		'EPSON Stylus Photo 1280' => 'Fully Operational',
		'EPSON Stylus Photo 1290' => 'Fully Operational',
		'EPSON Stylus Photo 2000P' => 'Testing in Progress, untuned'
		),
	'Stylus Color Range' => array(
		'EPSON Stylus C20SX' => 'Fully Operational',
		'EPSON Stylus C20UX' => 'Fully Operational',
		'EPSON Stylus C40SX' => 'Fully Operational',
		'EPSON Stylus C40UX' => 'Fully Operational',
		'EPSON Stylus C60' => 'Fully Operational',
		'EPSON Stylus C70' => 'Fully Operational',
		'EPSON Stylus C80' => 'Fully Operational',
		'EPSON Stylus Color' => 'Fully Operational',
		'EPSON Stylus Color Pro' => 'Fully Operational',
		'EPSON Stylus Color Pro XL' => 'Fully Operational',
		'EPSON Stylus Color 400' => 'Fully Operational',
		'EPSON Stylus Color 440' => 'Fully Operational',
		'EPSON Stylus Color 460' => 'Needs Testing',
		'EPSON Stylus Color 480' => 'Operational, cannot change cartridge',
		'EPSON Stylus Color 500' => 'Fully Operational',
		'EPSON Stylus Color 580' => 'Operational, cannot change cartridge',
		'EPSON Stylus Color 600' => 'Fully Operational',
		'EPSON Stylus Color 640' => 'Fully Operational',
		'EPSON Stylus Color 660' => 'Fully Operational',
		'EPSON Stylus Color 670' => 'Fully Operational',
		'EPSON Stylus Color 680' => 'Fully Operational',
		'EPSON Stylus Color 740' => 'Fully Operational',
		'EPSON Stylus Color 760' => 'Fully Operational',
		'EPSON Stylus Color 777' => 'Fully Operational',
		'EPSON Stylus Color 800' => 'Fully Operational',
		'EPSON Stylus Color 850' => 'Fully Operational',
		'EPSON Stylus Color 860' => 'Fully Operational',
		'EPSON Stylus Color 880' => 'Testing in Progress',
		'EPSON Stylus Color 83' => 'Testing in Progress',
		'EPSON Stylus Color 900' => 'Fully Operational',
		'EPSON Stylus Color 980' => 'Fully Operational',
		'EPSON Stylus Color 1160' => 'Fully Operational',
		'EPSON Stylus Color 1500' => 'Fully Operational',
		'EPSON Stylus Color 1520' => 'Fully Operational',
		'EPSON Stylus Color 3000' => 'Fully Operational'
		),
	'Stylus Pro Range' => array(
		'EPSON Stylus Pro 5000' => 'Needs Testing',
		'EPSON Stylus Pro 5500' => 'Needs Testing',
		'EPSON Stylus Pro 7000' => 'Needs Testing',
		'EPSON Stylus Pro 7500' => 'Needs Testing',
		'EPSON Stylus Pro 9000' => 'Needs Testing',
		'EPSON Stylus Pro 9500' => 'Needs Testing',
		'EPSON Stylus Pro 10000' => 'Needs Testing'
		),
	'Stylus Scan Range' => array(
		'EPSON Stylus Scan 2000' => 'Fully Operational',
		'EPSON Stylus Scan 2500' => 'Fully Operational'
		)
	),
'HP' => array(
	'DeskJet Range' => array(
		'HP DeskJet 340' => 'Fully Operational',
		'HP DeskJet 400' => 'Fully Operational',
		'HP DeskJet 500, 520' => 'Fully Operational',
		'HP DeskJet 500C, 540C' => 'Fully Operational',
		'HP DeskJet 550C, 560C' => 'Fully Operational',
		'HP DeskJet 600 series' => 'Operational',
		'HP DeskJet 800 series' => 'Operational',
		'HP DeskJet 900 series' => 'Operational',
		'HP PhotoSmart P1000, P1100' => 'Operational',
		'HP DeskJet 1100C, 1120C' => 'Operational',
		'HP DeskJet 1200C, 1220C, 1600C' => 'Operational',
		'HP DeskJet 2000 series' => 'Operational',
		'HP DeskJet 2500 series' => 'Operational',
		),
	'DesignJet Range' => array(
		'HP DesignJet series' => 'Operational',
		),
	'LaserJet Range' => array(
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
	       'BJC 30' => 'Operational',
	       'BJC 50' => 'Operational',
	       'BJC 55' => 'Operational',
	       'BJC 80' => 'Operational',
	       'BJC 210' => 'Needs Testing',
	       'BJC 240' => 'Needs Testing',
	       'BJC 250' => 'Needs Testing',
	       'BJC 1000' => 'Untested',
	       'BJC 2000' => 'Untested',
	       'BJC 3000' => 'Untested',
	       'BJC 4300' => 'Operational',
	       'BJC 6000' => 'Operational',
	       'BJC 6100' => 'Untested',
               'BJC 6200' => 'Operational',
	       'BJC 7000' => 'Untested',
	       'BJC 7100' => 'Untested',
	       'BJC 8200' => 'Operational',
	       'S400' => 'Operational',
	       'S450' => 'Operational',
	       'S800' => 'Operational',
	       'S4500' => 'Operational'
	       )
       ),
'Lexmark (<strong><em>New!</em></strong>)' => array(
       'Ink Jet Printers' => array(
	       '4076' => 'Operational',
	       'Z42' => 'Needs Testing',
	       'Z43' => 'Needs Testing',
	       'Z52' => 'Operational'
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
