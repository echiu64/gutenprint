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
		'EPSON Stylus Photo 785' => 'Fully Operational',
		'EPSON Stylus Photo 790' => 'Fully Operational',
		'EPSON Stylus Photo 810' => 'Fully Operational',
		'EPSON Stylus Photo 820' => 'Fully Operational',
		'EPSON Stylus Photo 830' => 'Fully Operational',
		'EPSON Stylus Photo 870' => 'Fully Operational',
		'EPSON Stylus Photo 875' => 'Fully Operational',
		'EPSON Stylus Photo 890' => 'Fully Operational',
		'EPSON Stylus Photo 895' => 'Fully Operational',
		'EPSON Stylus Photo 915' => 'Fully Operational',
		'EPSON Stylus Photo 925' => 'Operational; paper cutter does not work',
		'EPSON Stylus Photo 950' => 'Operational, partially tuned',
		'EPSON Stylus Photo 960' => 'Operational, partially tuned',
		'EPSON Stylus Photo 1200' => 'Fully Operational',
		'EPSON Stylus Photo 1270' => 'Fully Operational',
		'EPSON Stylus Photo 1280' => 'Fully Operational',
		'EPSON Stylus Photo 1290' => 'Fully Operational',
		'EPSON Stylus Photo 2000P' => 'Testing in Progress, untuned',
		'EPSON Stylus Photo 2100' => 'In Testing, untuned',
		'EPSON Stylus Photo 2200' => 'In Testing, untuned'
		),
	'Stylus Color Range' => array(
		'EPSON Stylus C20SX' => 'Fully Operational',
		'EPSON Stylus C20UX' => 'Fully Operational',
		'EPSON Stylus C40SX' => 'Fully Operational',
		'EPSON Stylus C40UX' => 'Fully Operational',
		'EPSON Stylus C41SX' => 'Fully Operational',
		'EPSON Stylus C41UX' => 'Fully Operational',
		'EPSON Stylus C42SX' => 'Fully Operational',
		'EPSON Stylus C42UX' => 'Fully Operational',
		'EPSON Stylus C60' => 'Fully Operational',
		'EPSON Stylus C61' => 'Fully Operational',
		'EPSON Stylus C62' => 'Fully Operational',
		'EPSON Stylus C70' => 'Fully Operational',
		'EPSON Stylus C80' => 'Fully Operational',
		'EPSON Stylus C82' => 'Fully Operational',
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
		'EPSON Stylus Color 880' => 'Fully Operational',
		'EPSON Stylus Color 83' => 'Fully Operational',
		'EPSON Stylus Color 900' => 'Fully Operational',
		'EPSON Stylus Color 980' => 'Fully Operational',
		'EPSON Stylus Color 1160' => 'Fully Operational',
		'EPSON Stylus Color 1500' => 'Fully Operational',
		'EPSON Stylus Color 1520' => 'Fully Operational',
		'EPSON Stylus Color 3000' => 'Fully Operational'
		),
	'Stylus Pro Range' => array(
		'EPSON Stylus Pro 5000' => 'Operational',
		'EPSON Stylus Pro 5500' => 'Operational',
		'EPSON Stylus Pro 7000' => 'Operational',
		'EPSON Stylus Pro 7500' => 'Operational',
		'EPSON Stylus Pro 7600' => 'Untested',
		'EPSON Stylus Pro 9000' => 'Operational',
		'EPSON Stylus Pro 9500' => 'Operational',
		'EPSON Stylus Pro 9600' => 'Untested',
		'EPSON Stylus Pro 10000' => 'Operational'
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
'Canon' => array(
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
'Lexmark' => array(
       'Ink Jet Printers' => array(
	       '4076' => 'Operational',
	       'Z42' => 'Needs Testing',
	       'Z43' => 'Needs Testing',
	       'Z52' => 'Operational',
	       'Z53' => 'Operational'
	       )
       ),
'Postscript' => array(
	'Postscript' => array(
		'PostScript Level 1' => 'Operational',
		'PostScript Level 2' => 'Operational'
		)
	)
);

$printer_drivers = array(
	'bjc-1000' => 'CANON BJC 1000',
	'bjc-2000' => 'CANON BJC 2000',
	'bjc-210' => 'CANON BJC 210',
	'bjc-240' => 'CANON BJC 240',
	'bjc-250' => 'CANON BJC 250',
	'bjc-30' => 'CANON BJC 30',
	'bjc-3000' => 'CANON BJC 3000',
	'bjc-4300' => 'CANON BJC 4300',
	'bjc-4400' => 'CANON BJC 4400 photo',
	'bjc-50' => 'CANON BJC 50',
	'bjc-55' => 'CANON BJC 55',
	'bjc-6000' => 'CANON BJC 6000',
	'bjc-6100' => 'CANON BJC 6100',
	'bjc-6200' => 'CANON BJC 6200',
	'bjc-6500' => 'CANON BJC 6500',
	'bjc-7000' => 'CANON BJC 7000',
	'bjc-7100' => 'CANON BJC 7100',
	'bjc-80' => 'CANON BJC 80',
	'bjc-8200' => 'CANON BJC 8200',
	'bjc-85' => 'CANON BJC 85',
	'bjc-s400' => 'CANON S400',
	'bjc-s450' => 'CANON S450',
	'bjc-s4500' => 'CANON S4500',
	'bjc-s800' => 'CANON S800',
	'escp2' => 'EPSON Stylus Color',
	'escp2-10000' => 'EPSON Stylus Pro 10000',
	'escp2-1160' => 'EPSON Stylus Color 1160',
	'escp2-1200' => 'EPSON Stylus Photo 1200',
	'escp2-1270' => 'EPSON Stylus Photo 1270',
	'escp2-1280' => 'EPSON Stylus Photo 1280',
	'escp2-1290' => 'EPSON Stylus Photo 1290',
	'escp2-1500' => 'EPSON Stylus Color 1500',
	'escp2-1520' => 'EPSON Stylus Color 1520',
	'escp2-2000' => 'EPSON Stylus Photo 2000P',
	'escp2-2100' => 'EPSON Stylus Photo 2100',
	'escp2-2200' => 'EPSON Stylus Photo 2200',
	'escp2-3000' => 'EPSON Stylus Color 3000',
	'escp2-400' => 'EPSON Stylus Color 400',
	'escp2-440' => 'EPSON Stylus Color 440',
	'escp2-460' => 'EPSON Stylus Color 460',
	'escp2-480' => 'EPSON Stylus Color 480',
	'escp2-500' => 'EPSON Stylus Color 500',
	'escp2-5000' => 'EPSON Stylus Pro 5000',
	'escp2-5500' => 'EPSON Stylus Pro 5500',
	'escp2-580' => 'EPSON Stylus Color 580',
	'escp2-600' => 'EPSON Stylus Color 600',
	'escp2-640' => 'EPSON Stylus Color 640',
	'escp2-660' => 'EPSON Stylus Color 660',
	'escp2-670' => 'EPSON Stylus Color 670',
	'escp2-680' => 'EPSON Stylus Color 680',
	'escp2-700' => 'EPSON Stylus Photo 700',
	'escp2-7000' => 'EPSON Stylus Pro 7000',
	'escp2-720' => 'EPSON Stylus Photo 720',
	'escp2-740' => 'EPSON Stylus Color 740',
	'escp2-750' => 'EPSON Stylus Photo 750',
	'escp2-7500' => 'EPSON Stylus Pro 7500',
	'escp2-760' => 'EPSON Stylus Color 760',
	'escp2-7600' => 'EPSON Stylus Pro 7600',
	'escp2-777' => 'EPSON Stylus Color 777',
	'escp2-780' => 'EPSON Stylus Photo 780',
	'escp2-785' => 'EPSON Stylus Photo 785EPX',
	'escp2-790' => 'EPSON Stylus Photo 790',
	'escp2-800' => 'EPSON Stylus Color 800',
	'escp2-810' => 'EPSON Stylus Photo 810',
	'escp2-820' => 'EPSON Stylus Photo 820',
	'escp2-825' => 'EPSON Stylus Photo 825',
	'escp2-83' => 'EPSON Stylus Color 83',
	'escp2-830' => 'EPSON Stylus Photo 830',
	'escp2-850' => 'EPSON Stylus Color 850',
	'escp2-860' => 'EPSON Stylus Color 860',
	'escp2-870' => 'EPSON Stylus Photo 870',
	'escp2-875' => 'EPSON Stylus Photo 875',
	'escp2-880' => 'EPSON Stylus Color 880',
	'escp2-890' => 'EPSON Stylus Photo 890',
	'escp2-895' => 'EPSON Stylus Photo 895',
	'escp2-900' => 'EPSON Stylus Color 900',
	'escp2-9000' => 'EPSON Stylus Pro 9000',
	'escp2-915' => 'EPSON Stylus Photo 915',
	'escp2-925' => 'EPSON Stylus Photo 925',
	'escp2-950' => 'EPSON Stylus Photo 950',
	'escp2-9500' => 'EPSON Stylus Pro 9500',
	'escp2-960' => 'EPSON Stylus Photo 960',
	'escp2-9600' => 'EPSON Stylus Pro 9600',
	'escp2-980' => 'EPSON Stylus Color 980',
	'escp2-c20sx' => 'EPSON Stylus C20SX',
	'escp2-c20ux' => 'EPSON Stylus C20UX',
	'escp2-c40sx' => 'EPSON Stylus C40SX',
	'escp2-c40ux' => 'EPSON Stylus C40UX',
	'escp2-c41sx' => 'EPSON Stylus C41SX',
	'escp2-c41ux' => 'EPSON Stylus C41UX',
	'escp2-c42sx' => 'EPSON Stylus C42SX',
	'escp2-c42ux' => 'EPSON Stylus C42UX',
	'escp2-c60' => 'EPSON Stylus C60',
	'escp2-c61' => 'EPSON Stylus C61',
	'escp2-c62' => 'EPSON Stylus C62',
	'escp2-c70' => 'EPSON Stylus C70',
	'escp2-c80' => 'EPSON Stylus C80',
	'escp2-c82' => 'EPSON Stylus C82',
	'escp2-cl700' => 'EPSON CL-700',
	'escp2-cl750' => 'EPSON CL-750',
	'escp2-em900c' => 'EPSON EM-900C',
	'escp2-em930c' => 'EPSON EM-930C',
	'escp2-ex' => 'EPSON Stylus Photo EX',
	'escp2-ex3' => 'EPSON Stylus Photo EX3',
	'escp2-mc10000' => 'EPSON MC-10000',
	'escp2-mc2000' => 'EPSON MC-2000',
	'escp2-mc5000' => 'EPSON MC-5000',
	'escp2-mc7000' => 'EPSON MC-7000',
	'escp2-mc9000' => 'EPSON MC-9000',
	'escp2-mj5100c' => 'EPSON MJ-5100C',
	'escp2-mj520c' => 'EPSON MJ-520C',
	'escp2-mj6000c' => 'EPSON MJ-6000C',
	'escp2-mj8000c' => 'EPSON MJ-8000C',
	'escp2-photo' => 'EPSON Stylus Photo',
	'escp2-pm10000' => 'EPSON PM-10000',
	'escp2-pm2000c' => 'EPSON PM-2000C',
	'escp2-pm2200c' => 'EPSON PM-2200C',
	'escp2-pm3000c' => 'EPSON PM-3000C',
	'escp2-pm3300c' => 'EPSON PM-3300C',
	'escp2-pm3500c' => 'EPSON PM-3500C',
	'escp2-pm4000px' => 'EPSON PM-4000PX',
	'escp2-pm5000c' => 'EPSON PM-5000C',
	'escp2-pm7000c' => 'EPSON PM-7000C',
	'escp2-pm700c' => 'EPSON PM-700C',
	'escp2-pm730c' => 'EPSON PM-730C',
	'escp2-pm750c' => 'EPSON PM-750C',
	'escp2-pm760c' => 'EPSON PM-760C',
	'escp2-pm770c' => 'EPSON PM-770C',
	'escp2-pm780c' => 'EPSON PM-780C',
	'escp2-pm790pt' => 'EPSON PM-790PT',
	'escp2-pm800c' => 'EPSON PM-800C',
	'escp2-pm850pt' => 'EPSON PM-850PT',
	'escp2-pm880c' => 'EPSON PM-880C',
	'escp2-pm9000c' => 'EPSON PM-9000C',
	'escp2-pm950c' => 'EPSON PM-950C',
	'escp2-pro-xl' => 'EPSON Stylus Color Pro XL',
	'escp2-pro' => 'EPSON Stylus Color Pro',
	'escp2-px7000' => 'EPSON PX-7000',
	'escp2-px9000' => 'EPSON PX-9000',
	'escp2-scan2000' => 'EPSON Stylus Scan 2000',
	'escp2-scan2500' => 'EPSON Stylus Scan 2500',
	'lexmark-4076' => 'LEXMARK 4076',
	'lexmark-z42' => 'LEXMARK Z42',
	'lexmark-z43' => 'LEXMARK Z43',
	'lexmark-z52' => 'LEXMARK Z52',
	'lexmark-z53' => 'LEXMARK Z53',
	'pcl-1100' => 'HP DeskJet 1100C',
	'pcl-1120' => 'HP DeskJet 1120C',
	'pcl-1200' => 'HP DeskJet 1200C',
	'pcl-1220' => 'HP DeskJet 1220C',
	'pcl-1600' => 'HP DeskJet 1600C',
	'pcl-2' => 'HP LaserJet II series',
	'pcl-2000' => 'HP DeskJet 2000 series',
	'pcl-2500' => 'HP DeskJet 2500 series',
	'pcl-3' => 'HP LaserJet III series',
	'pcl-340' => 'HP DeskJet 340',
	'pcl-4' => 'HP LaserJet 4 series',
	'pcl-400' => 'HP DeskJet 400',
	'pcl-4l' => 'HP LaserJet 4L',
	'pcl-4v' => 'HP LaserJet 4V, 4Si',
	'pcl-5' => 'HP LaserJet 5 series',
	'pcl-500' => 'HP DeskJet 500',
	'pcl-501' => 'HP DeskJet 500C',
	'pcl-520' => 'HP DeskJet 520',
	'pcl-540' => 'HP DeskJet 540C',
	'pcl-550' => 'HP DeskJet 550C',
	'pcl-560' => 'HP DeskJet 560C',
	'pcl-5si' => 'HP LaserJet 5Si',
	'pcl-6' => 'HP LaserJet 6 series',
	'pcl-600' => 'HP DeskJet 600/600C',
	'pcl-601' => 'HP DeskJet 600 series',
	'pcl-690' => 'HP DeskJet 690 series',
	'pcl-750' => 'HP DesignJet 750C',
	'pcl-810' => 'HP DeskJet 810C',
	'pcl-812' => 'HP DeskJet 812C',
	'pcl-840' => 'HP DeskJet 840C',
	'pcl-842' => 'HP DeskJet 842C',
	'pcl-845' => 'HP DeskJet 845C',
	'pcl-850' => 'HP DeskJet 850C',
	'pcl-855' => 'HP DeskJet 855C',
	'pcl-870' => 'HP DeskJet 870C',
	'pcl-890' => 'HP DeskJet 890C',
	'pcl-895' => 'HP DeskJet 895C',
	'pcl-900' => 'HP DeskJet 900 series',
	'pcl-P1000' => 'HP PhotoSmart P1000',
	'pcl-P1100' => 'HP PhotoSmart P1100',
);

$all_known_printers = array(

	'Apollo P-2200'  => 'pcl-690',
	'Apple LaserWriter Select 360'  => 'pcl-2',
	'Brother DCP-1200'  => 'pcl-2',
	'Brother HL-4Ve'  => 'pcl-2',
	'Brother HL-10V'  => 'pcl-3',
	'Brother HL-10h'  => 'pcl-4',
	'Brother HL-630'  => 'pcl-2',
	'Brother HL-660'  => 'pcl-4',
	'Brother HL-760'  => 'pcl-4',
	'Brother HL-960'  => 'pcl-4',
	'Brother HL-1040'  => 'pcl-2',
	'Brother HL-1050'  => 'pcl-4',
	'Brother HL-1060'  => 'pcl-4',
	'Brother HL-1070'  => 'pcl-4',
	'Brother HL-1240'  => 'pcl-2',
	'Brother HL-1250'  => 'pcl-4',
	'Brother HL-1260'  => 'pcl-4',
	'Brother HL-1270N'  => 'pcl-4',
	'Brother HL-1660e'  => 'pcl-4',
	'Brother HL-2060'  => 'pcl-4',
	'Brother MFC-6550MC'  => 'pcl-2',
	'Brother MFC-8300'  => 'pcl-2',
	'Brother MFC-8300'  => 'pcl-3',
	'Brother MFC-9500'  => 'pcl-2',
	'Brother MFC-9600'  => 'pcl-2',
	'Canon BJ-30'  => 'bjc-30',
	'Canon BJC-50'  => 'bjc-50',
	'Canon BJC-55'  => 'bjc-55',
	'Canon BJC-80'  => 'bjc-80',
	'Canon BJC-85'  => 'bjc-85',
	'Canon BJC-210'  => 'bjc-210',
	'Canon BJC-240'  => 'bjc-240',
	'Canon BJC-250'  => 'bjc-250',
	'Canon BJC-1000'  => 'bjc-1000',
	'Canon BJC-2000'  => 'bjc-2000',
	'Canon BJC-2010'  => 'bjc-2000',
	'Canon BJC-2100'  => 'bjc-4300',
	'Canon BJC-2110'  => 'bjc-2000',
	'Canon BJC-3000'  => 'bjc-3000',
	'Canon BJC-4000'  => 'bjc-4300',
	'Canon BJC-4300'  => 'bjc-4300',
	'Canon BJC-4400'  => 'bjc-4400',
	'Canon BJC-6000'  => 'bjc-6000',
	'Canon BJC-6100'  => 'bjc-6100',
	'Canon BJC-6200'  => 'bjc-6200',
	'Canon BJC-6500'  => 'bjc-6500',
	'Canon BJC-7000'  => 'bjc-7000',
	'Canon BJC-7100'  => 'bjc-7100',
	'Canon BJC-8200'  => 'bjc-8200',
	'Canon GP 335'  => 'pcl-4',
	'Canon LBP-430'  => 'pcl-3',
	'Canon LBP-430'  => 'pcl-4l',
	'Canon LBP-4sx'  => 'pcl-3',
	'Canon LBP-1000'  => 'pcl-6',
	'Canon LBP-1260'  => 'pcl-6',
	'Canon LBP-1760'  => 'pcl-6',
	'Canon S100'  => 'bjc-4300',
	'Canon S300'  => 'bjc-s800',
	'Canon S400'  => 'bjc-s400',
	'Canon S450'  => 'bjc-s450',
	'Canon S500'  => 'bjc-8200',
	'Canon S600'  => 'bjc-8200',
	'Canon S630'  => 'bjc-8200',
	'Canon S800'  => 'bjc-s800',
	'Canon S4500'  => 'bjc-s4500',
	'Canon imageRunner 330s'  => 'pcl-4',
	'Citizen ProJet II'  => 'pcl-2',
	'DEC 1800'  => 'pcl-3',
	'DEC LN17'  => 'pcl-4',
	'Epson ActionLaser 1100'  => 'pcl-3',
	'Epson ActionLaser II'  => 'pcl-2',
	'Epson AcuLaser C2000'  => 'pcl-4',
	'Epson AcuLaser C2000PS'  => 'pcl-4',
	'Epson AcuLaser C8500'  => 'pcl-4',
	'Epson AcuLaser C8500PS'  => 'pcl-4',
	'Epson EPL-5200'  => 'pcl-3',
	'Epson EPL-5200+'  => 'pcl-3',
	'Epson EPL-5700'  => 'pcl-4',
	'Epson EPL-5800'  => 'pcl-4',
	'Epson EPL-5900'  => 'pcl-4',
	'Epson EPL-7100'  => 'pcl-2',
	'Epson Stylus C20SX'  => 'escp2-c20sx',
	'Epson Stylus C20UX'  => 'escp2-c20ux',
	'Epson Stylus C40SX'  => 'escp2-c40sx',
	'Epson Stylus C40UX'  => 'escp2-c40ux',
	'Epson Stylus C41SX'  => 'escp2-c41sx',
	'Epson Stylus C41UX'  => 'escp2-c41ux',
	'Epson Stylus C42SX'  => 'escp2-c42sx',
	'Epson Stylus C42UX'  => 'escp2-c42ux',
	'Epson Stylus C60'  => 'escp2-c60',
	'Epson Stylus C61'  => 'escp2-c61',
	'Epson Stylus C62'  => 'escp2-c62',
	'Epson Stylus C70'  => 'escp2-c70',
	'Epson Stylus C80'  => 'escp2-c80',
	'Epson Stylus C82'  => 'escp2-c82',
	'Epson Stylus Color'  => 'escp2',
	'Epson Stylus Color I'  => 'escp2',
	'Epson Stylus Color II'  => 'escp2',
	'Epson Stylus Color IIs'  => 'escp2',
	'Epson Stylus Color PRO'  => 'escp2-pro',
	'Epson Stylus Color 400'  => 'escp2-400',
	'Epson Stylus Color 440'  => 'escp2-440',
	'Epson Stylus Color 460'  => 'escp2-460',
	'Epson Stylus Color 480'  => 'escp2-480',
	'Epson Stylus Color 500'  => 'escp2-500',
	'Epson Stylus Color 580'  => 'escp2-580',
	'Epson Stylus Color 600'  => 'escp2-600',
	'Epson Stylus Color 640'  => 'escp2-640',
	'Epson Stylus Color 660'  => 'escp2-660',
	'Epson Stylus Color 670'  => 'escp2-670',
	'Epson Stylus Color 680'  => 'escp2-680',
	'Epson Stylus Color 740'  => 'escp2-740',
	'Epson Stylus Color 760'  => 'escp2-760',
	'Epson Stylus Color 777'  => 'escp2-777',
	'Epson Stylus Color 8 3'  => 'escp2-83',
	'Epson Stylus Color 800'  => 'escp2-800',
	'Epson Stylus Color 850'  => 'escp2-850',
	'Epson Stylus Color 860'  => 'escp2-860',
	'Epson Stylus Color 880'  => 'escp2-880',
	'Epson Stylus Color 900'  => 'escp2-900',
	'Epson Stylus Color 980'  => 'escp2-980',
	'Epson Stylus Color 1160'  => 'escp2-1160',
	'Epson Stylus Color 1500'  => 'escp2-1500',
	'Epson Stylus Color 1520'  => 'escp2-1520',
	'Epson Stylus Color 3000'  => 'escp2-3000',
	'Epson Stylus Photo'  => 'escp2-photo',
	'Epson Stylus Photo 700'  => 'escp2-700',
	'Epson Stylus Photo 720'  => 'escp2-720',
	'Epson Stylus Photo 750'  => 'escp2-750',
	'Epson Stylus Photo 780'  => 'escp2-780',
	'Epson Stylus Photo 785'  => 'escp2-785',
	'Epson Stylus Photo 790'  => 'escp2-790',
	'Epson Stylus Photo 810'  => 'escp2-810',
	'Epson Stylus Photo 820'  => 'escp2-820',
	'Epson Stylus Photo 825'  => 'escp2-825',
	'Epson Stylus Photo 830'  => 'escp2-830',
	'Epson Stylus Photo 870'  => 'escp2-870',
	'Epson Stylus Photo 875'  => 'escp2-875',
	'Epson Stylus Photo 890'  => 'escp2-890',
	'Epson Stylus Photo 895'  => 'escp2-895',
	'Epson Stylus Photo 915'  => 'escp2-915',
	'Epson Stylus Photo 925'  => 'escp2-925',
	'Epson Stylus Photo 950'  => 'escp2-950',
	'Epson Stylus Photo 960'  => 'escp2-960',
	'Epson Stylus Photo 1200'  => 'escp2-1200',
	'Epson Stylus Photo 1270'  => 'escp2-1270',
	'Epson Stylus Photo 1280'  => 'escp2-1280',
	'Epson Stylus Photo 1290'  => 'escp2-1290',
	'Epson Stylus Photo 2000P'  => 'escp2-2000',
	'Epson Stylus Photo 2100'  => 'escp2-2100',
	'Epson Stylus Photo 2200'  => 'escp2-2200',
	'Epson Stylus Photo EX'  => 'escp2-ex',
	'Epson Stylus Photo EX3'  => 'escp2-ex3',
	'Epson Stylus Pro 5000'  => 'escp2-5000',
	'Epson Stylus Pro 5500'  => 'escp2-5500',
	'Epson Stylus Pro 7000'  => 'escp2-7000',
	'Epson Stylus Pro 7500'  => 'escp2-7500',
	'Epson Stylus Pro 7600'  => 'escp2-7600',
	'Epson Stylus Pro 9000'  => 'escp2-9000',
	'Epson Stylus Pro 9500'  => 'escp2-9500',
	'Epson Stylus Pro 9600'  => 'escp2-9600',
	'Epson Stylus Pro 10000'  => 'escp2-10000',
	'Epson Stylus Pro XL'  => 'escp2-pro-xl',
	'Epson Stylus Scan 2000'  => 'escp2-scan2000',
	'Epson Stylus Scan 2500'  => 'escp2-scan2500',
	'Epson CL-700'  => 'escp2-cl700',
	'Epson CL-750'  => 'escp2-cl750',
	'Epson EM-900C'  => 'escp2-em900c',
	'Epson EM-930C'  => 'escp2-em930c',
	'Epson MC-2000'  => 'escp2-mc2000',
	'Epson MC-5000'  => 'escp2-mc5000',
	'Epson MC-7000'  => 'escp2-mc7000',
	'Epson MC-9000'  => 'escp2-mc9000',
	'Epson MJ-5100C'  => 'escp2-mj5100c',
	'Epson MJ-520C'  => 'escp2-mj520c',
	'Epson MJ-6000C'  => 'escp2-mj6000c',
	'Epson MJ-8000C'  => 'escp2-mj8000c',
	'Epson MC-10000'  => 'escp2-mc10000',
	'Epson PM-2000C'  => 'escp2-pm2000c',
	'Epson PM-2200C'  => 'escp2-pm2200c',
	'Epson PM-3000C'  => 'escp2-pm3000c',
	'Epson PM-3300C'  => 'escp2-pm3300c',
	'Epson PM-3500C'  => 'escp2-pm3500c',
	'Epson PM-4000PX'  => 'escp2-pm4000px',
	'Epson PM-5000C'  => 'escp2-pm5000c',
	'Epson PM-7000C'  => 'escp2-pm7000c',
	'Epson PM-10000'  => 'escp2-pm10000',
	'Epson PM-700C'  => 'escp2-pm700c',
	'Epson PM-730C'  => 'escp2-pm730c',
	'Epson PM-750C'  => 'escp2-pm750c',
	'Epson PM-760C'  => 'escp2-pm760c',
	'Epson PM-770C'  => 'escp2-pm770c',
	'Epson PM-780C'  => 'escp2-pm780c',
	'Epson PM-790PT'  => 'escp2-pm790pt',
	'Epson PM-800C'  => 'escp2-pm800c',
	'Epson PM-850PT'  => 'escp2-pm850pt',
	'Epson PM-880C'  => 'escp2-pm880c',
	'Epson PM-9000C'  => 'escp2-pm9000c',
	'Epson PM-950C'  => 'escp2-pm950c',
	'Epson PX-7000'  => 'escp2-px7000',
	'Epson PX-9000'  => 'escp2-px9000',
	'Fujitsu PrintPartner 10V'  => 'pcl-4',
	'Fujitsu PrintPartner 16DV'  => 'pcl-4',
	'Fujitsu PrintPartner 20W'  => 'pcl-6',
	'Fujitsu PrintPartner 8000'  => 'pcl-3',
	'HP Color LaserJet 4550'  => 'pcl-4',
	'HP Color LaserJet 5000'  => 'pcl-4',
	'HP DesignJet 750C'  => 'pcl-750',
	'HP DesignJet 750C Plus'  => 'pcl-750',
	'HP DeskJet 340C'  => 'pcl-340',
	'HP DeskJet 400'  => 'pcl-400',
	'HP DeskJet 500'  => 'pcl-500',
	'HP DeskJet 500C'  => 'pcl-501',
	'HP DeskJet 520'  => 'pcl-520',
	'HP DeskJet 540C'  => 'pcl-540',
	'HP DeskJet 550C'  => 'pcl-550',
	'HP DeskJet 560C'  => 'pcl-560',
	'HP DeskJet 600'  => 'pcl-600',
	'HP DeskJet 610C'  => 'pcl-601',
	'HP DeskJet 610CL'  => 'pcl-601',
	'HP DeskJet 612C'  => 'pcl-601',
	'HP DeskJet 640C'  => 'pcl-601',
	'HP DeskJet 648C'  => 'pcl-601',
	'HP DeskJet 660C'  => 'pcl-601',
	'HP DeskJet 670C'  => 'pcl-601',
	'HP DeskJet 672C'  => 'pcl-601',
	'HP DeskJet 680C'  => 'pcl-601',
	'HP DeskJet 682C'  => 'pcl-601',
	'HP DeskJet 690C'  => 'pcl-690',
	'HP DeskJet 692C'  => 'pcl-690',
	'HP DeskJet 693C'  => 'pcl-690',
	'HP DeskJet 694C'  => 'pcl-690',
	'HP DeskJet 695C'  => 'pcl-690',
	'HP DeskJet 697C'  => 'pcl-690',
	'HP DeskJet 810C'  => 'pcl-810',
	'HP DeskJet 812C'  => 'pcl-812',
	'HP DeskJet 815C'  => 'pcl-812',
	'HP DeskJet 830C'  => 'pcl-810',
	'HP DeskJet 832C'  => 'pcl-810',
	'HP DeskJet 840C'  => 'pcl-840',
	'HP DeskJet 841C'  => 'pcl-840',
	'HP DeskJet 842C'  => 'pcl-842',
	'HP DeskJet 845C'  => 'pcl-845',
	'HP DeskJet 850C'  => 'pcl-850',
	'HP DeskJet 855C'  => 'pcl-855',
	'HP DeskJet 870C'  => 'pcl-870',
	'HP DeskJet 890C'  => 'pcl-890',
	'HP DeskJet 895C'  => 'pcl-895',
	'HP DeskJet 930C'  => 'pcl-900',
	'HP DeskJet 932C'  => 'pcl-900',
	'HP DeskJet 950C'  => 'pcl-900',
	'HP DeskJet 952C'  => 'pcl-900',
	'HP DeskJet 970C'  => 'pcl-900',
	'HP DeskJet 990C'  => 'pcl-900',
	'HP DeskJet 1100C'  => 'pcl-1100',
	'HP DeskJet 1120C'  => 'pcl-1120',
	'HP DeskJet 1125C'  => 'pcl-1120',
	'HP DeskJet 1200C'  => 'pcl-1200',
	'HP DeskJet 1220C'  => 'pcl-1220',
	'HP DeskJet 1600C'  => 'pcl-1600',
	'HP DeskJet 1600CM'  => 'pcl-1600',
	'HP DeskJet 2000C'  => 'pcl-2000',
	'HP DeskJet 2500C'  => 'pcl-2500',
	'HP DeskJet 2500CM'  => 'pcl-2500',
	'HP LaserJet 2'  => 'pcl-2',
	'HP LaserJet 2D'  => 'pcl-2',
	'HP LaserJet 2P'  => 'pcl-2',
	'HP LaserJet 2P Plus'  => 'pcl-2',
	'HP LaserJet 3'  => 'pcl-3',
	'HP LaserJet 3D'  => 'pcl-3',
	'HP LaserJet 3P w/ PCL5'  => 'pcl-4l',
	'HP LaserJet 3P w/PS'  => 'pcl-3',
	'HP LaserJet 4'  => 'pcl-4',
	'HP LaserJet 4 Plus'  => 'pcl-4',
	'HP LaserJet 4L'  => 'pcl-4l',
	'HP LaserJet 4M'  => 'pcl-4',
	'HP LaserJet 4ML'  => 'pcl-4',
	'HP LaserJet 4P'  => 'pcl-4',
	'HP LaserJet 4Si'  => 'pcl-4',
	'HP LaserJet 4V'  => 'pcl-4v',
	'HP LaserJet 5'  => 'pcl-5',
	'HP LaserJet 5L'  => 'pcl-4',
	'HP LaserJet 5P'  => 'pcl-5',
	'HP LaserJet 5Si'  => 'pcl-5si',
	'HP LaserJet 6'  => 'pcl-6',
	'HP LaserJet 6L'  => 'pcl-6',
	'HP LaserJet 6P'  => 'pcl-4',
	'HP LaserJet 1100'  => 'pcl-6',
	'HP LaserJet 1100A'  => 'pcl-6',
	'HP LaserJet 1200'  => 'pcl-6',
	'HP LaserJet 1220'  => 'pcl-6',
	'HP LaserJet 2100'  => 'pcl-6',
	'HP LaserJet 2200'  => 'pcl-6',
	'HP LaserJet 3200'  => 'pcl-6',
	'HP LaserJet 3200m'  => 'pcl-6',
	'HP LaserJet 3200se'  => 'pcl-6',
	'HP LaserJet 4000'  => 'pcl-6',
	'HP LaserJet 4050'  => 'pcl-6',
	'HP LaserJet 4100'  => 'pcl-6',
	'HP LaserJet 8150'  => 'pcl-6',
	'HP LaserJet 9000'  => 'pcl-6',
	'HP OfficeJet'  => 'pcl-520',
	'HP OfficeJet 300'  => 'pcl-520',
	'HP OfficeJet 330'  => 'pcl-520',
	'HP OfficeJet 350'  => 'pcl-520',
	'HP OfficeJet LX'  => 'pcl-520',
	'HP OfficeJet Pro 1150C'  => 'pcl-850',
	'HP PhotoSmart P1000'  => 'pcl-P1000',
	'HP PhotoSmart P1100'  => 'pcl-P1100',
	'IBM 4019'  => 'pcl-2',
	'IBM 4029 030 LaserPrinter 10'  => 'pcl-2',
	'IBM 4029 030 LaserPrinter 10'  => 'pcl-3',
	'IBM Infoprint 12'  => 'pcl-6',
	'IBM Page Printer 3112'  => 'pcl-4',
	'Infotec 4651 MF'  => 'pcl-6',
	'Kyocera F-1010'  => 'pcl-2',
	'Kyocera FS-600'  => 'pcl-4',
	'Kyocera FS-680'  => 'pcl-4',
	'Kyocera FS-1000'  => 'pcl-4',
	'Kyocera FS-1000+'  => 'pcl-4',
	'Kyocera FS-1010'  => 'pcl-4',
	'Kyocera FS-1800'  => 'pcl-4',
	'Kyocera FS-1900'  => 'pcl-4',
	'Kyocera FS-3500'  => 'pcl-2',
	'Kyocera FS-3500'  => 'pcl-3',
	'Kyocera FS-3750'  => 'pcl-6',
	'Kyocera FS-3800'  => 'pcl-4',
	'Kyocera FS-9100DN'  => 'pcl-4',
	'Kyocera FS-9500DN'  => 'pcl-4',
	'Lexmark 4076'  => 'lexmark-4076',
	'Lexmark Optra E'  => 'pcl-4',
	'Lexmark Optra E+'  => 'pcl-4',
	'Lexmark Valuewriter 300'  => 'pcl-2',
	'Lexmark Z42'  => 'lexmark-z42',
	'Lexmark Z43'  => 'lexmark-z43',
	'Lexmark Z52'  => 'lexmark-z52',
	'Lexmark Z53'  => 'lexmark-z53',
	'Minolta PagePro 6'  => 'pcl-4',
	'Minolta PagePro 6e'  => 'pcl-4',
	'Minolta PagePro 6ex'  => 'pcl-4',
	'Minolta PagePro 8'  => 'pcl-4',
	'Minolta PagePro 8L'  => 'pcl-2',
	'Minolta PagePro 1100'  => 'pcl-6',
	'NEC SuperScript 660i'  => 'pcl-4',
	'NEC SuperScript 860'  => 'pcl-2',
	'NEC SuperScript 870'  => 'pcl-2',
	'NEC SuperScript 1260'  => 'pcl-2',
	'NEC SuperScript 1800'  => 'pcl-4',
	'Okidata OL400'  => 'pcl-2',
	'Okidata OL400e'  => 'pcl-2',
	'Okidata OL400ex'  => 'pcl-2',
	'Okidata OL410e'  => 'pcl-4',
	'Okidata OL600e'  => 'pcl-2',
	'Okidata OL610e/S'  => 'pcl-2',
	'Okidata OL800'  => 'pcl-2',
	'Okidata OL810ex'  => 'pcl-4',
	'Okidata Okipage 6e'  => 'pcl-4',
	'Okidata Okipage 6ex'  => 'pcl-4',
	'Okidata Okipage 8p'  => 'pcl-4',
	'Okidata Okipage 10e'  => 'pcl-4',
	'Okidata Okipage 10ex'  => 'pcl-4',
	'Okidata Okipage 14ex'  => 'pcl-4',
	'Okidata Super 6e'  => 'pcl-4l',
	'Olivetti JP350S'  => 'pcl-2',
	'Olivetti PG 306'  => 'pcl-2',
	'PCPI 1030'  => 'pcl-2',
	'Panasonic KX-P4410'  => 'pcl-2',
	'Panasonic KX-P4450'  => 'pcl-3',
	'Panasonic KX-P6150'  => 'pcl-2',
	'Panasonic KX-P6500'  => 'pcl-2',
	'Raven LP-410'  => 'pcl-2',
	'Ricoh Aficio 220'  => 'pcl-6',
	'Ricoh Aficio 401'  => 'pcl-4',
	'Ricoh Aficio 700'  => 'pcl-4',
	'Samsung ML-4600'  => 'pcl-6',
	'Samsung ML-5000a'  => 'pcl-4',
	'Samsung ML-6000/6100'  => 'pcl-4',
	'Samsung ML-7000/7000P/7000N'  => 'pcl-6',
	'Samsung ML-7050'  => 'pcl-6',
	'Samsung ML-85'  => 'pcl-4',
	'Samsung QL-5100A'  => 'pcl-4',
	'Samsung QL-6050'  => 'pcl-4',
	'Seiko SpeedJET 200'  => 'pcl-2',
	'Sharp AR-161'  => 'pcl-6',
	'Sony IJP-V100'  => 'pcl-601',
	'Star LS-04'  => 'pcl-2',
	'Star LaserPrinter 8'  => 'pcl-2',
	'Tally MT908'  => 'pcl-3',
	'Xerox Able 1406'  => 'pcl-4',
	'Xerox DocuPrint P8e'  => 'pcl-4',
	'Xerox DocuPrint P12'  => 'pcl-2',
	'Xerox DocuPrint P1202'  => 'pcl-6',
	'Xerox DocuPrint 4508'  => 'pcl-4',
	'Xerox DocuPrint N4512'  => 'pcl-4',
	'Xerox DocuPrint N4512 PS'  => 'pcl-4',
	'Xerox Document Centre 400'  => 'pcl-4'
);

?>
