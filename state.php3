<?

$plugin_version_number = '4.1.9';

$maintainer = '<a href="https://sourceforge.net/developer/index.php?form_dev=5436">Robert Krawitz</a>';

$navbar_text = 'Welcome to Gimp-Print.';

$supported_printers = array(
'Epson' => array(
	'Stylus Photo Range' => array(
		'EPSON Stylus Photo' => 'Fully Operational',
		'EPSON Stylus Photo 700' => 'Fully Operational',
		'EPSON Stylus Photo EX' => 'Fully Operational',
		'EPSON Stylus Photo 720' => 'Needs Testing',
		'EPSON Stylus Photo 750' => 'Fully Operational',
		'EPSON Stylus Photo 780' => 'Fully Operational (4.1 only;
		use the 870 driver in 4.0)',
		'EPSON Stylus Photo 790' => 'Fully Operational (4.1 only;
		use the 870 driver in 4.0)',
		'EPSON Stylus Photo 870' => 'Fully Operational',
		'EPSON Stylus Photo 890' => 'Fully Operational (4.1 only;
		use the 870 driver in 4.0)',
		'EPSON Stylus Photo 1200' => 'Fully Operational',
		'EPSON Stylus Photo 1270' => 'Fully Operational',
		'EPSON Stylus Photo 1280' => 'Fully Operational (4.1 only;
		use the 1270 driver in 4.0)',
		'EPSON Stylus Photo 1290' => 'Fully Operational (4.1 only;
		use the 1270 driver in 4.0)',
		'EPSON Stylus Photo 2000P' => 'Testing in Progress, untuned'
		),
	'Stylus Color Range' => array(
		'EPSON Stylus Color' => 'Fully Operational',
		'EPSON Stylus Color Pro' => 'Fully Operational',
		'EPSON Stylus Color Pro XL' => 'Fully Operational',
		'EPSON Stylus Color 400' => 'Fully Operational',
		'EPSON Stylus Color 440' => 'Fully Operational',
		'EPSON Stylus Color 460' => 'Needs Testing',
		'EPSON Stylus Color 480' => 'Operational, cannot change cartridge (4.1 only)',
		'EPSON Stylus Color 500' => 'Fully Operational',
		'EPSON Stylus Color 580' => 'Operational, cannot change cartridge (4.1 only)',
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
		'EPSON Stylus Color 880' => 'Testing in Progress (4.1
		only; use the 860 driver in 4.0)',
		'EPSON Stylus Color 83' => 'Testing in Progress (4.1
		only; use the 860 driver in 4.0)',
		'EPSON Stylus Color 900' => 'Fully Operational (tuned in 4.1 only)',
		'EPSON Stylus Color 980' => 'Fully Operational (4.1
		only; use the 900 driver for partial operation in 4.0)',
		'EPSON Stylus Color 1160' => 'Fully Operational',
		'EPSON Stylus Color 1500' => 'Fully Operational',
		'EPSON Stylus Color 1520' => 'Fully Operational',
		'EPSON Stylus Color 3000' => 'Fully Operational'
		),
	'Stylus Pro Range' => array(
		'EPSON Stylus Pro 5000' => 'Needs Testing (4.1 only)',
		'EPSON Stylus Pro 5500' => 'Needs Testing (4.1 only)',
		'EPSON Stylus Pro 7000' => 'Needs Testing (4.1 only)',
		'EPSON Stylus Pro 7500' => 'Needs Testing (4.1 only)',
		'EPSON Stylus Pro 9000' => 'Needs Testing (4.1 only)',
		'EPSON Stylus Pro 9500' => 'Needs Testing (4.1 only)'
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
	       'BJC 30' => 'Operational (4.1 only)',
	       'BJC 50' => 'Operational (4.1 only)',
	       'BJC 55' => 'Operational (4.1 only)',
	       'BJC 80' => 'Operational (4.1 only)',
	       'BJC 85' => 'Operational (4.1 only)',
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
	       'S400' => 'Operational (4.1 only)',
	       'S450' => 'Operational (4.1 only)',
	       'S800' => 'Operational (4.1 only)',
	       'S4500' => 'Operational (4.1 only)'
	       )
       ),
'Lexmark (<strong><em>New!</em></strong>)' => array(
       'Ink Jet Printers' => array(
	       '3200' => 'Operational (4.1 only)',
	       '4076' => 'Operational',
	       'Z31' => 'Needs Testing (4.1 only)',
	       'Z52' => 'Operational (4.1 only)'
	       )
       ),
'Compaq (<strong><em>New!</em></strong>)' => array(
       'Ink Jet Printers' => array(
	       'IJ750' => 'Operational (4.1 only)'
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
