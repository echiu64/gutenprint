<?
require('functions.php3');
###############################################
##    Set title of this page here    ##########
$title = 'Gimp-Print FAQ for Mac OS X and Darwin';
###############################################
###############################################
require('standard_html_header.php3');


### Content Below  ###
# Please remember to use <P> </P> tags !  ?>
<H2>Gimp-Print FAQ for Mac OS X Jaguar and Darwin</H2>
 <ol>
 <li>
 <p>
 <h3>
 What is Gimp-Print for Jaguar and Darwin? Why would I want to install it?
 </h3>
 </p>
 <p>
 Gimp-Print is a package of high quality printer drivers for Mac OS X
 Jaguar, Darwin, Linux, BSD, Solaris, IRIX, and other UNIX-alike
 operating systems. In many cases, these drivers rival or exceed the OEM
 drivers in quality and functionality. Our goal is to produce the highest
 possible output quality from all supported printers. To that end, we have
 done extensive work on screening algorithms, color generation, and
 printer feature utilization. We are continuing our work in all of these
 areas to produce ever higher quality results, particularly on the
 ubiquitous, inexpensive inkjet printers that are nonetheless capable of
 nearly photographic output quality. Additionally, Gimp-Print provides
 excellent drivers for many printers that are otherwise unsupported on
 Mac OS X.
 </p>
 <li>
 <h3>
 How can I find out more information about Gimp-Print? Where can I get
 the newest version?
 </h3>
 <p>
 For the latest information or the newest version of Gimp-Print be sure
 to check the <a href= "http://gimp-print.sourceforge.net/MacOSX.php3"
 >Gimp-Print web page</a>.
 </p>
 <li>
 <h3>
 I've read this entire document but I'm still having problems. How can I
 contact the developers?
 </h3>
 <p>
 If you're having problems it's a good bet that you're not the only one.
 If you can't find a solution to your problem in this FAQ or in <em>How
 to Print with Gimp-Print</em>, located on the installer disk, your next
 stop should be the Gimp-Print project <a href=
 "http://sourceforge.net/forum/?group_id=1537" >forums</a>. If you've
 looked through the forums and you still can't find a reference to your
 problem then you may simply be the first person to encouter it. It's
 helpful to the developers and users of Gimp-Print if you post a
 description of the problem you're facing in the Gimp-Print project <a
 href= "http://sourceforge.net/forum/forum.php?forum_id=4359" >help
 forums</a>. When you post in the forums, developers can respond to your
 post and everyone browsing the forums can benefit from the exchange.
 Please browse the forums before posting as your question may already be
 answered.
 </p>
 <li>
 <h3>
 I do not see the "Advanced" option in Print Center. How can I find it?
 </h3>
 <p>
 Make sure you are holding down the <em>option</em> key (on the keyboard)
 when you click the <tt>Add Printer</tt> button.
 </p>
 <li>
 <h3>
 Printing does not work from "Carbon" applications (Adobe Photoshop, Acrobat,
 Appleworks, etc...), but I can print from "Cocoa" applications (Preview,
 TextEdit, etc...) just fine. How can I fix this ?
 </h3>
 <p>
 The Gimp-Print drivers for OS X are CUPS plugin filters. In Mac OS 10.2
 and 10.2.1 when users print with CUPS-based drivers from a "Carbon"
 application the application generates PostScript output instead of the
 OS X native PDF (this is not the case with vendor-supplied printer
 drivers). If <a href=
 "http://sourceforge.net/project/showfiles.php?group_id=18073&release_id=109322"
 >ESP Ghostscript</a> is not installed the print job simply fails
 without indication (other than the lack of a print!). If you haven't 
 installed ESP Ghostscript just download the <a href=
 "http://prdownloads.sourceforge.net/espgs/espgs-7.05.5-0.ppc.dmg?download"
 >Mac OS X installer</a> and run it.
 </p>
 <li>
 <h3>
 I want to know more about the new printing system in Mac OS X Jaguar. How do
 the various components like CUPS, Gimp-Print, Ghostscript, etc... interact?
 </h3>
 <p>
 The best place to start is with the CUPS documentation. In particular, the <a
 href= "http://www.cups.org/overview.html" >CUPS overview</a> will help you
 understand how the filters are chained together. You'll find more filter
 information <a href= "http://www.cups.org/sdd.html#3_7" >here</a>.
 </p><p>
 When printing, CUPS tries to string together a series of tools in order to
 convert the submitted file to the format needed by a printer. In OS X the
 files submitted are generally either PDF or PICT files with embedded
 PostScript. When printing to a gimp-print driver, if the input file format is
 PDF then the OS X PDF to raster filter is run and the raster data handed to
 the gimp-print driver. If instead the input file format is PICT with
 PostScript, then the OS X PICT to PostScript filter is run. In order to get
 from PostScript to raster, the ESP GhostScript filter is run next in the chain
 and then the raster data is handed to the gimp-printer.
 </p><p>
 So you have the following two chains:
 </p>
 <pre>
 PDF file -> cgpdftoraster filter -> rastertoprinter filter -> printer

 PICTwPS -> pictwpstops filter -> pstoraster (GhostScript) filter ->
 rastertoprinter filter -> printer
 </pre> 
 </p><p>
 The application determines which of these two paths are invoked. Most OS X
 applications submit a PDF for printing. Certain PostScript centric programs
 such as Adobe applications cause the second filter chain to run.
 </p>
 <li>
 <h3>
 I can not print to my Epson Stylus Pro 7600, but it's supposed to be supported.
 </h3>
 <p>  
 That driver is broken in the 4.2.2 release. Try using the 4.2.3-pre1 release,
 but please note that the driver is not fully tested or optimized, so the
 output quality may be less than you expect. Keep checking the Gimp-Print web
 site for newer releases that may offer more improvements for this driver.  
 </p>
 <li>
 <h3>
 What files are installed by the Gimp-Print installer for Jaguar and
 Darwin? Where are they installed? I want to remove them; how do I do it?
 </h3>
 <p>
 "The Gimp-Print drivers for <a href= "http://www.cups.org/" >CUPS</a>,
 and the required PPD files." That's the short answer to the first
 question. The long answer is that a lot of files are installed into the
 (typically) hidden BSD unix layer of Jaguar. You shouldn't need to
 remove Gimp-Print in order to restore your previous printing capability.
 Just delete any Gimp-Print printers that are set up in Print Center. That
 said, you can remove the Gimp-Print capability from your system by simply
 removing the directory where the printer "driver" files are stored. Here
 is the path: <tt>/usr/share/cups/model/C/</tt> some ways to remove this
 folder/directory are to either boot into mac os 9 and drag the "C"
 folder to the trash or you can issue this command in a terminal:
 <tt>sudo rm -R /usr/share/cups/model/C/</tt> you will need to enter an
 admin password (yours if you're the only user on your system) to use
 sudo.
 </p>
 <li>
 <h3>
 The Gimp-Print installer for Jaguar is very nice, thanks for providing
 it. But, why haven't you provided me with an easy way to remove the
 Gimp-Print files from my system?
 </h3>
 <p>
 The Gimp-Print files need to be installed into the BSD layer because
 that's where the <a href= "http://www.cups.org/" >CUPS</a> print spooler
 is located. On any Jaguar system capable of using Gimp-Print the total
 size of the installed files is negligible. Leaving the Gimp-Print files
 in place will not cause any problems. Trying to remove the files and
 mistakenly removing some unrelated files (which is very easy to do)
 <em>will</em> cause problems. There is currently no "uninstaller"
 shipped with the OS X package because the people working hard to provide
 you with Gimp-Print in an easy to use manner, for free, thought that it
 was more important to spend their limited time on getting it working -
 <em>for many people using Jaguar, Gimp-Print is the only printing
 solution available!</em> A future version may include an "uninstall"
 utility.
 </p>
 <li>
 <h3>
 The list of supported printers says that PostScript Level 1 printing is
 supported, but I can't use Gimp-Print to print to my Level 1 printer
 (Laserwriter plus, Laserwriter IINT, etc...). What's wrong?
 </h3>
 <p>
 Currently, printing to level 1 printers is not supported on Jaguar.
 Generating the necessary level 1 compatible output requires a postscript
 generator, like <a href= "http://sourceforge.net/projects/espgs/" >ESP
 Ghostscript</a> and some special printing "filters". A solution to this
 problem may become available in the future.
 </p>
 <li>
 <h3>
 Which versions of Mac OS are compatible with Gimp-Print?
 </h3>
 <p>
 Gimp-Print is compatible with Mac OS X version 10.2.x (Jaguar) or later.
 It does not work with version 10.1.x or 10.0.x or any version of Mac OS 9.
 </p>
 <li>
 <h3>
 I have never heard of version "10.1.x" but I have 10.1.5, does
 Gimp-Print work with that?
 </h3>
 <p>
 "10.1.x" means "any" minor version of 10.1 such as 10.1.1, 10.1.2,
 ...,10.1.5. So, if you want to use Gimp-Print you should upgrade to at
 least 10.2.
 </p>
 <li>
 <h3>
 Is my printer supported by Gimp-Print?
 </h3>
 <p>
 There is a comprehensive <a href=
 "http://Gimp-Print.sourceforge.net/p_Supported_Printers.php3" >list of
 supported printers</a> at the Gimp-Print website. If your printer is
 listed as "Fully Operational" then you should expect excellent output
 when using Gimp-Print. Printers listed as something other than "Fully
 Operational" may still work, but not all the features of the printer
 will be available. For printers that are not listed as supported you may
 still be able to print by trying the driver for a printer that is similar
 to yours, but the results may disappoint you.
 </p>
 <li>
 <h3>
 OK, I just installed Gimp-Print on Mac OS 10.2 (or later) and I tried to
 print but I can't figure out how to set up my printer in print center.
 </h3>
 <p>
 The Gimp-Print 4.2.2 installer package (and all later versions) contains
 an illustrated set-up guide called <em>How to Print with Gimp-Print</em> 
 located at the root level of the installer disk image. This guide will
 walk you through the printer set-up process for USB and TCP/IP printers.
 If you don't have access to this set-up guide the following instructions
 should help:
 </p>
 <p>
 Setting up printers using the Gimp-Print driver can be slightly
 different than setting up other drivers depending on how you are
 connecting to your printer.
 <ul>
 <li>
 <h4>
 Make sure your printer is supported by Gimp-Print
 </h4>
 <p>
 see question on supported printers.
 </p>
 <li>
 <h4>
 USB connection
 </h4>
 If your printer is supported by Gimp-Print and it is connected directly
 to your Mac with a USB cable then you should follow these steps to set
 it up:
 <ol>
 <p>
 <li>Make sure the USB cable is properly connected to both your printer
 and your Mac.
 <li>Turn on your printer.
 <li>Restart your Mac (if you know what you're doing and you know that
 you do not have any print jobs in progress you can do <tt>sudo killall
 -HUP cupsd</tt> instead of restarting)
 <li>Open the Print Center utility (located in
 <tt>/Applications/Utilities/</tt>).
 <li>Hold down the option key on your keyboard and either click on the
 "Add" button in the Print Center toolbar or select "<tt>Add
 printer...</tt>" from the "<tt>Printers</tt>" menu. A new sheet will
 open.
 <li>In the sheet click on the top popup menu and select "Advanced" from
 the very bottom of the list.
 <li>Next, click on the "<tt>Device:</tt>" popup menu and select your
 printer's name from the very bottom of the list (if you see only
 "<tt>USB Printer (usb)</tt>" and not your printer's name you need to
 restart your mac with your printer turned on and connected).
 <li>Finally, click on the "<tt>Printer Model</tt>" popup menu and select
 the manufacturer for your printer, and in the small window at the bottom
 of the sheet select the PPD file for your printer model. It's important
 that you select the correct PPD file, and the names are not overly
 descriptive, but if you match your printer model number with the number
 in the PPD file name you should be OK.
 <li> Click "<tt>Add</tt>".
 <li> Print a test page.
 </p>
 </ol>
 <li>
 <h4>
 Network connection
 </h4>
 If your printer is supported by Gimp-Print and it is available over
 TCP/IP via a built-in network card (such as the Epson 10/100 ethernet
 type-b card available for certain printers) or an inexpensive network
 print server (parallel to ethernet converter) then you can set it up
 for IP printing:
 <p>
 <ol>
 <li>First you need to get the IP address and the print queue name for
 your printer from your network administrator. If you are the
 administrator for your network consult the documentation that came with
 your network device to learn how to determine this information (if you
 don't understand the ins and outs of TCP/IP networking you should find
 a friend who does). If you can't determine the Queue Name for your print
 server try using the default queue first. If that fails try a common
 queue name like lp (lower case LP). If you have the correct IP Address
 but you don't use the correct queue name you won't print.
 <li>Open the Print Center utility (located in
 <tt>/Applications/Utilities/</tt>).
 <li>Click on the "<tt>Add</tt>" button in the Print Center toolbar or
 select "<tt>Add printer...</tt>" from the "Printers" menu. A new sheet
 will open.
 <li>In the sheet click on the top popup menu and select "<tt>IP
 Printing</tt>".
 <li>Fill in the IP address of the printer and the queue name in the
 appropriate boxes.
 <li>Next, click on the "<tt>Printer Model</tt>" popup menu and select
 the manufacturer for your printer, and in the small window at the bottom
 of the sheet select the PPD file for your printer model. It's important
 that you select the correct PPD file, and the names are not too
 descriptive, but if you match your printer model number with the number
 in the PPD file name you should be OK.
 <li> Click "<tt>Add</tt>".
 <li> Print a test page.
 </ol>
 </p>
 <li>
 <h4>
 Windows printer via SAMBA
 </h4>
 If you want to print to a shared printer connected to a Windows XP
 computer try these instructions (this procedure has not been thoroughly
 tested):
 <p>
 <ol>
 <li>Open the Print Center utility (located in
 <tt>/Applications/Utilities/</tt>).
 <li>Hold down the option key on your keyboard and either click on the
 <tt>Add</tt> button in the Print Center toolbar or select <tt>Add
 printer...</tt> from the <tt>Printers</tt> menu. A new sheet will open.
 <li>In the sheet click on the top popup menu and select
 <tt>Advanced</tt> from the very bottom of the list.
 <li>Next, click on the <tt>Device:</tt> popup menu and select
 <tt>Windows Printer via SAMBA</tt>.
 <li> In the <tt>Device Name:</tt> field enter a descriptive name for
 this printer.
 <li>In the <tt>Device URI:</tt> field enter the device URI in the
 following form:
 <p>
 <tt>smb://winuser:password@workgroup/server/printer </tt>
 <dl>
 <dt><tt>winuser</tt> <dd>XP login name <dt><tt>password</tt> <dd>XP
 login password <dt><tt>workgroup</tt> <dd>XP workgroup name
 <dt><tt>server</tt> <dd>Computer name of the XP machine
 <dt><tt>printer</tt> <dd>Share name of the shared XP printer
 </dl>
 </p>
 <li>Finally, click on the <tt>Printer Model</tt> popup menu and select
 the manufacturer for your printer, and in the small window at the bottom
 of the sheet select the PPD file for your printer model. It's important
 that you select the correct PPD file, and the names are not overly
 descriptive, but if you match your printer model number with the number
 in the PPD file name you should be OK.
 <li> Click <tt>Add</tt>.
 <li> Print a test page.
 </ol>
 </ul>
 </p>
 <li>
 <h3>
 OK, my printer is printing now, but how do I change the print settings
 like paper type and resolution?
 </h3>
 <p>
 You can access the settings for the Gimp-Print driver whenever a print
 sheet is open. Just click on the popup menu that says "<tt>Copies &
 Pages</tt>" and choose "<tt>Printer Features</tt>".
 </p>
 <li>
 <h3>
 Ok, I found all the settings, but what do they do?
 </h3>
 <p>
 Please see the <! a href= "http://Gimp-Print.sourceforge.net/p_FAQ.php3"
 >Gimp-Print General FAQ<! /a> for answers to these questions.
 <p>
 </ol>
 <p>
 <h2>
 Gimp-Print General FAQ
 </h2>
 </p>
 <ol>
 <p>
 <li>
 <h3>
 Is it only for Gimp?
 </h3>
 <p>
 No, it can be used for many printing needs. Gimp-Print started out as a
 driver for The Gimp, the well known image manipulation program. Early in
 development versions for Ghostscript and later CUPS were added. The
 emphasis is still on quality color printing, though performance gets a
 lot of attention these days.
 </p>
 <li>
 <h3>
 I cannot install it, it complains about missing Gimp files
 </h3>
 <p>
 <ul>
 <li>If you have the Gimp installed on your system, you probably have the
 user package, but not the development package, installed. You will need
 to install the latter from your installation media; it's usually named
 gimp-devel.
 <p>
 <li>If you do have the Gimp installed, and you've installed the
 gimp-devel package, you may also need to install the gtk-devel and
 glib-devel packages.
 <p>
 <li>If you've installed the Gimp from source, you may need to run
 <tt>ldconfig</tt> as root. The installation procedure for the Gimp
 doesn't run ldconfig, which is needed on many systems to tell the system
 about the shared libraries that are installed. If you don't understand
 this, don't worry; just do it. If you're nervous about doing that,
 reboot.
 <p>
 <li>If you don't have the Gimp installed on your system, and just want
 to compile Gimp-Print for CUPS (for example), you need to pass
 <tt>configure</tt> the option <tt>--with-gimp=no</tt>, so it won't try
 to look for the Gimp and fail.
 </ul>
 </p>
 <li>
 <h3>
 What is the difference between B/W, Line art, solid color and Photo mode
 (or ImageType in Ghostscript)?
 </h3>
 <p>
 Photo mode does a lot of work to make colors as similar to screen
 presentation as possible. This takes time. Line art is faster, but
 colors may be off. Solid Colors is somewhere in between. B/W mode does
 not use color ink when printing, and is much faster.
 </p>
 <li>
 <h3>
 I selected my printer and it doesn't work at all!
 </h3>
 <p>
 Please check your printing system (lpd, CUPS, LPRng, etc.)
 configuration. You may also have a problem with your parallel port or
 USB connection, so take a look at <tt>/var/log/messages</tt> (or
 wherever your system logs are kept).
 </p>
 <li>
 <h3>
 I selected my printer and it simply feeds paper without printing
 </h3>
 <p>
 Many Epson printers (in particular) do this if they encounter an error
 in the command stream. This usually indicates a bug in Gimp-Print;
 please report it to <a
 mailto= "Gimp-Print-devel@sourceforge.net" >Gimp-Print-devel@
 sourceforge.net</a> or via the bug tracking system at <a
 href= "http://Gimp-Print.sourceforge.net" >http://
 Gimp-Print.sourceforge.net</a>. Make sure you report the printer you
 have and all of the settings that you used. But first, triple check that
 you're using the right printer model!
 </p>
 <li>
 <h3>
 I selected my printer, and it prints the image badly distorted, or at
 completely the wrong place on the page
 </h3>
 <p>
 This usually indicates a bug in the package. Please report it as
 described above. Also as described above, make sure you've set the right
 printer.
 </p>
 <li>
 <h3>
 I selected the right printer, but garbage, or only part of the page,
 gets printed
 </h3>
 <p>
 Printers for which support just has been added may not have been tested,
 as the developers do not have access to the printer. It is worth trying
 different settings. For example, change the resolution to a mainstream
 value as used on that printer. Also photo mode is better tested than the
 optimized versions. When you find out what works and what doesn't, file a
 bug report.
 </p>
 <p>
 One common cause of this is not using "raw" mode when printing from the
 Gimp plugin. Depending upon your printing system, you will need to use
 either <tt>-l</tt> (traditional BSD lpd), <tt>-oraw</tt> (CUPS lpr), or
 <tt>-d</tt> (most versions of System V lp, including CUPS). Otherwise
 the printing system attempts to interpret the output as something else,
 and tries to apply a filter to it to convert it to something else
 (usually PostScript).
 </p>
 <p>
 Another less common cause of this (it usually causes other symptoms,
 like printing only part of a page) is lack of space somewhere. This is
 most commonly an issue when using the Gimp Print plugin. The plugin
 creates a huge temporary file that gets sent to the printing system. The
 size of the file varies; it's proportional to the page size and the
 resolution setting chosen. Full-page, high resolution photographs can
 result in 100 MB of output. The system may need to have 2 or 3 copies of
 this file for short periods of time. If your /tmp, /var, or wherever your
 spooler keeps its temporary files is too small, you'll have problems.
 </p>
 <p>
 Finally, problems with your parallel or USB port may be the cause of
 problems here. Certain Epson printers in particular are known to be very
 sensitive to the quality of connecting cables, and may have trouble with
 long or low quality cables, or USB hubs. If nothing else works, and
 you're certain you've tried everything else, try a better cable or a
 direct USB connection.
 </p>
 <li>
 <h3>
 What's up with the HP Deskjet 1200?
 </h3>
 <p>
 HP had sold two printers with the 1200 model designation. The old
 version is 300 DPI and has a heating element to dry the ink. It was
 manufactured around 1990. The new version is of 2000 vintage and has
 higher resolution. The one supported by this package is the new one???
 </p>
 <li>
 <h3>
 I selected the right printer and the quality is lousy
 </h3>
 <p>
 Try selecting a different resolution or quality setting. Especially
 lower resolutions have a problem putting enough ink on paper. Also, use
 Photo mode. If you find settings that do not work at all (you get
 garbage or no output, but other settings work), report these as bugs.
 High resolutions should produce a similar (but smoother) result than
 medium resolutions. Resolutions under a certain printer dependent figure
 are seen as draft-only - for example lower than 360 DPI on Epsons with
 standard paper or lower than 300 DPI on HP.
 </p>
 <p>
 Also make sure that you have the right kind of paper selected. Selecting
 plain paper when you're printing on high quality photo paper is certain
 to result in a light, grainy image. Selecting photo paper when you're
 printing on plain paper will result in a dark, muddy image that bleeds
 through the paper. There are differences between different kinds of
 paper; you may need to tweak the density and color settings slightly.
 </p>
 <p>
 In addition, certain printers don't work well on certain kinds of paper.
 Epson printers work well on Epson papers, but don't work well on many
 third party papers (particularly the high quality photo papers made by
 other vendors). This isn't a conspiracy to lock you into their paper,
 it's because they've formulated the paper and ink to work well together.
 </p>
 <p>
 If you use Ghostscript, make sure the Ghostscript resolution is not set
 higher than the driver resolution. If it is higher, the driver has to
 throw away part of the pixels, leading to uneven strokes in text and
 slanted lines with interruptions.
 </p>
 <li>
 <h3>
 How do I start setting options for Ghostscript?
 </h3>
 <p>
 Please see src/ghost/README for more information.
 </p>
 <li>
 <h3>
 <tt>escputil -i</tt> or <tt>escputil -d</tt> fails as follows:
 </h3>
 <p>
 <pre>
 % escputil -r /dev/lp0 -i
 
 [ ... license info omitted ... ]
 
 Cannot read from /dev/lp0: Invalid argument
 </pre>
 </p>
 <p>
 You need to rebuild your kernel with CONFIG_PRINTER_READBACK enabled.
 </p>
 <li>
 <h3>
 I tried to test my Epson printer by 'cat .cshrc > /dev/lp0' and nothing
 prints!!!???
 </h3>
 <p>
 The classic test of printer connectivity -- sending an ASCII file to it
 -- doesn't work on many Epson printers out of the box (or after printing
 from Windows or Macintosh). Epson printers from the Stylus Color 740 and
 newer use a special "packet mode" in which they do not recognize
 standard commands or ASCII text. They must be sent a special sequence
 that takes them out of packet mode. The command
 </p>
 <p>
 <pre>escputil -u -s -r /dev/lp0</pre>
 </p>
 <p>
 will take the printer out of packet mode and enable you to print to it.
 Of course, as soon as you've read back status from the printer, you know
 it's working (although if you're unable to read status out of the
 printer, you might have a different problem; see above).
 </p>
 <p>
 Printing to your printer from Gimp-Print, whether you use the Print
 plugin, the CUPS driver, or the Ghostscript driver, will also take the
 printer out of packet mode. But then again, if you successfully print to
 your printer, you know it's working, so why worry? If you're trying to
 test your spooler, though, the escputil trick above will do it. Just
 make sure that /dev/lp0 is the right device; if it isn't, substitute
 whatever is.
 </p>
 <p>
 <em>Note:</em> this does not apply to printers prior to the 740 (such as
 the Stylus Photo EX, Stylus Color 850, or anything even older). Those
 printers are always capable of printing ASCII text, and don't have
 packet mode. You can read status from them, but you must leave off the
 '-u' option.
 </p>
 <li>
 <h3>
 My USB-connected Epson Stylus printer won't work with {Free,Net,Open}BSD!
 </h3>
 <p>
 By default, the BSD device driver for the USB printer device (usually
 ulpt0) does a prime, or USB bus reset, when the device is opened. This
 causes the printer to reset itself (one can hear the print head moving
 back and forth when this happens) and lose sync. After this the printer
 won't go into graphics mode and instead spews characters all over you
 expensive photo paper. This has been observed on the Stylus Photo 870;
 it likely exists with other USB-connected Epson Stylus printers.
 </p>
 <p>
 The fix is to use the "unlpt0" device instead of "ulpt0". The driver
 doesn't perform the USB prime when unlpt is opened. If this device
 doesn't exist on your system you can create it with
 </p>
 <p>
 <pre>mknod unlpt0 c 113 64 root wheel</pre>
 </p>
 <p>
 in the /dev directory.
 </p>
 <li>
 <h3>
 I try to print with StarOffice and it doesn't print correctly!
 </h3>
 <p>
 If you use CUPS, and your prints from StarOffice come out incorrectly
 (particularly at low resolution), try the following. This assumes a
 network installation of StarOffice 5.2.
 <ol>
 <p>
 <li>Ensure that <tt>root</tt> does not have a .Xpdefaults file (if it
 does, the procedure below will edit root's version rather than the
 system-wide version in <tt>.../office52/share/xp3/Xpdefaults</tt>).
 </p>
 <p>
 <li>As root, start <tt>.../office52/program/spadmin</tt>. This is the
 StarOffice printer administration program.
 </p>
 <p>
 <li>Click on <b>Install New Driver</b>. For the <b>Driver directory</b>,
 select your CUPS PPD directory. This is usually <tt>/etc/cups/ppd</tt>.
 This should list the names of all of the drivers you have installed.
 </p>
 <p>
 <li>If there are no drivers visible, you may need to give the .ppd files
 names ending in .PS. The following script will accomplish this:
 </p>
 <p>
 <pre>
 # cd /etc/cups/ppd
 # for f in * ; do
 > ln -s $f `echo $f | sed 's/ppd$/PS/'`
 > done
 </pre>
 </p>
 <p>
 Following this, restart <tt>spadmin</tt> and click on <b>Install New
 Driver</b>. When you select your CUPS PPD directory, you will find the
 necessary drivers listed.
 </p>
 <p>
 <li>Select the drivers you want StarOffice to know about and click OK.
 </p>
 <p>
 <li>If you have been using the generic Postscript printer, remove all of
 your old queues.
 </p>
 <p>
 <li>Select the appropriate new driver(s) and click <b>Add New
 Printer</b>.
 </p>
 <p>
 <li>Select the appropriate printer queue and click <b>Connect</b> to
 connect it to the printer queue of your choice.
 </ol>
 </p>
 <p>
 You can now set up appropriate options for this printer. Note that you
 can create multiple queues with different settings, for example one for
 draft mode and one for high quality.
 </p>
 <li>
 <h3>
 I'm printing through Samba, and my printer prints garbage!
 </h3>
 <p>
 There are a number of Samba configuration issues that cause problems; a
 common problem is translation from UNIX newlines (\n) to Windows
 newlines (\r\n). It's important to ensure that sending raw data, with no
 translation, to the printer.
 </p>
 </ol>

<?require('standard_html_footer.php3');?>
