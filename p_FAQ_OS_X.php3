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
<p>
<ol>
<li>
<h3>
What is Gimp-Print for Jaguar and Darwin? Why would I want
to install it?
</h3>
</p>
Gimp-Print is a package of high quality printer drivers for
Mac OS X Jaguar, Darwin, Linux, BSD, Solaris, IRIX, and
other UNIX-alike operating systems. In many cases, these
drivers rival or exceed the OEM drivers in quality and
functionality. Our goal is to produce the highest possible
output quality from all supported printers. To that end, we
have done extensive work on screening algorithms, color
generation, and printer feature utilization. We are
continuing our work in all of these areas to produce ever
higher quality results, particularly on the ubiquitous,
inexpensive inkjet printers that are nonetheless capable of
nearly photographic output quality. Additionally, Gimp-Print
provides excellent drivers for many printers that are
otherwise unsupported on Mac OS X.
<p>
<li>
<h3>
I've read this entire document but I'm still having
problems. How can I contact the developers?
</h3>
</p> If you're having problems it's a good bet that you're
not the only one. If you can't find a solution to your
problem in the Gimp-Print documentation or in this FAQ your
next stop should be the gimp-print project <a
href="http://sourceforge.net/forum/?group_id=1537">forums</a>.
 If you've looked through the forums and you still can't
find a reference to your problem then you may simply be the
first person to encouter it. It's helpful to the developers
and users of Gimp-Print if you post a description of the
problem you're facing in the gimp-print project <a
href="http://sourceforge.net/forum/forum.php?forum_id=4359"
>help forums</a>. When you post in the forums, developers
can respond to your post and everyone browsing the forums
can benefit from the exchange. Please browse the forums
before posting as your question may already be answered.
<p>
<li>
<h3>
Sometimes when I try to print it works. Other times the
application appears to be sending the print job but it
never shows up in the printer queue and it never prints.
What gives?
</h3>
</p> There is a limitation with all prebuilt versions of
Gimp-Print and Mac OS X 10.2 where carbon applications that
generate postscript code (such as many adobe applications)
cannot print directly using Gimp-Print drivers. A
work-around is to click the "Preview" button in the print
panel and then print the resulting document from the
Preview application (this work-around may not work if you
use an application other than "Preview" for viewing print
previews). The solution is to install a PostScript
interpreter that has a plugin filter for the <a
href="http://www.cups.org/">CUPS</a> print spooler. <a
href="http://sourceforge.net/projects/espgs/">ESP
Ghostscript</a> is a good choice, and there is now a
pre-built OS X installer available at <a href=
"http://www.allosx.com/1031773924/index_html">All OS X</a>.
<p>
<li>
<h3>
What files are installed by the Gimp-Print installer for
Jaguar and Darwin? Where are they installed? I want to
remove them; how do I do it?
</h3>
</p> "The Gimp-print drivers for <a
href="http://www.cups.org/">CUPS</a>, and the required PPD
files." That's the short answer to the first question. The
long answer is that a lot of files are installed into the
(typically) hidden BSD unix layer of Jaguar. You shouldn't
need to remove gimp-print in order to restore your previous
printing capability. Just delete any Gimp-print printers
that are set up in Print Center. That said, you can remove
the Gimp-Print capability from your system by simply
removing the directory where the printer "driver" files are
stored. Here is the path: <tt>/usr/share/cups/model/C/</tt>
some ways to remove this folder/directory are to either
boot into mac os 9 and drag the "C" folder to the trash or
you can issue this command in a terminal: <tt>sudo rm -R
/usr/share/cups/model/C/</tt> you will need to enter an
admin password (yours if you're the only user on your
system) to use sudo.
<p>
<li>
<h3>
The Gimp-Print installer for Jaguar is very nice, thanks
for providing it. But, why haven't you provided me with an
easy way to remove the Gimp-Print files from my system?
</h3>
</p> The Gimp-Print files need to be installed into the BSD
layer because that's where the <a
href="http://www.cups.org/">CUPS</a> print spooler is
located. On any Jaguar system capable of using Gimp-Print
the total size of the installed files is negligible.
Leaving the Gimp-Print files in place will not cause any
problems. Trying to remove the files and mistakenly
removing some unrelated files (which is very easy to do)
<em>will</em> cause problems. There is currently no
"uninstaller" shipped with the OS X package because the
people working hard to provide you with Gimp-Print in an
easy to use manner, for free, thought that it was more
important to spend their limited time on getting it working
- <em>for many people using Jaguar, Gimp-Print is the only
printing solution available!</em> A future version may
include an "uninstall" utility.
<p>
<li>
<h3>
The list of supported printers says that PostScript Level 1
printing is supported, but I can't use Gimp-Print to print
to my Level 1 printer (Laserwriter plus, Laserwriter IINT,
etc...). What's wrong?
</h3>
</p> Currently, printing to level 1 printers is not
supported on Jaguar. Generating the necessary level 1
compatible output requires a postscript generator, like <a
href="http://sourceforge.net/projects/espgs/">ESP
Ghostscript</a> and some special printing "filters". A
solution to this problem may become available in the future.
<p>
<li>
<h3>
Which versions of Mac OS are compatible with Gimp-Print?
</h3>
</p> Gimp-Print is compatible with Mac OS X version 10.2.x
(Jaguar) or later. It does not work with version 10.1.x or
10.0.x or any version of Mac OS 9.
<p>
<li>
<h3>
I have never heard of version "10.1.x" but I have 10.1.5,
does Gimp-Print work with that?
</h3>
</p> "10.1.x" means "any" minor version of 10.1 such as
10.1.1, 10.1.2, ...,10.1.5. So, if you want to use
Gimp-Print you should upgrade to at least 10.2.
<p>
<li>
<h3>
Is my printer supported by Gimp-Print?
</h3>
</p> There is a comprehensive <a
href="http://gimp-print.sourceforge.net/p_Supported_
Printers.php3">list of supported printers</a> at the
Gimp-Print website. If your printer is listed as "Fully
Operational" then you should expect excellent output when
using Gimp-Print. Printers listed as something other than
"Fully Operational" may still work, but not all the
features of the printer will be available. For printers
that are not listed as supported you may still be able to
print by trying the driver for a printer that is similar to
yours, but the results may disappoint you.
<p>
<li>
<h3>
OK, I just installed Gimp-Print on Mac OS 10.2 (or later)
and I tried to print but I can't figure out how to set up
my printer in print center.
</h3>
</p>
<p>
The Gimp-Print 4.2.2 installer package (and all later
versions) contains an illustrated set-up guide called
"How to Print with Gimp-Print" located at the root level of the
installer disk image. This guide will walk you through the
printer set-up process for USB and TCP/IP printers. If you
don't have access to this set-up guide the following
instructions should help:
</p>
Setting up printers using the Gimp-Print driver can be
slightly different than setting up other drivers depending
on how you are connecting to your printer.
<ul>
<p>
<li>
<h4>
Make sure your printer is supported by Gimp-Print
</h4>
see question on supported printers.
</p>
<li>
<h4>
USB connection
</h4>
If your printer is supported by Gimp-Print and it is
connected directly to your Mac with a USB cable then you
should follow these steps to set it up:
<ol>
<p>
<li>Make sure the USB cable is properly connected to both
your printer and your Mac.
<li>Turn on your printer.
<li>Restart your Mac (if you know what you're doing and you
know that you do not have any print jobs in progress you can
do <tt>sudo killall -HUP cupsd</tt> instead of restarting)
<li>Open the Print Center utility (located in
<tt>/Applications/Utilities/</tt>).
<li>Hold down the option key on your keyboard and either
click on the "Add" button in the Print Center toolbar or
select "<tt>Add printer...</tt>" from the
"<tt>Printers</tt>" menu. A new sheet will open.
<li>In the sheet click on the top popup menu and select
"Advanced" from the very bottom of the list.
<li>Next, click on the "<tt>Device:</tt>" popup menu and
select your printer's name from the very bottom of the list
(if you see only "<tt>USB Printer (usb)</tt>" and not your
printer's name you need to restart your mac with your
printer turned on and connected).
<li>Finally, click on the "<tt>Printer Model</tt>" popup
menu and select the manufacturer for your printer, and in
the small window at the bottom of the sheet select the PPD
file for your printer model. It's important that you select
the correct PPD file, and the names are not overly
descriptive, but if you match your printer model number
with the number in the PPD file name you should be OK.
<li> Click "<tt>Add</tt>".
<li> Print a test page.
</p>
</ol>
<li>
<h4>
Network connection
</h4>
If your printer is supported by Gimp-Print and it is
available over TCP/IP via a built-in network card (such as
the Epson 10/100 ethernet type-b card available for certain
printers) or an inexpensive network print server (parallel
to ethernet converter) then you should set it up for IP
printing:
<p>
<ol>
<li>First you need to get the IP address and the print
queue name for your printer from your network
administrator. If you are the administrator for your
network consult the documentation that came with your
network device to learn how to determine this information.
If you can't determine the Queue Name for your print server
try using the default queue first. If that fails try a
common queue name like lp (lower case LP). If you don't use
the correct queue name you won't print.
<li>Open the Print Center utility (located in
<tt>/Applications/Utilities/</tt>).
<li>Click on the "<tt>Add</tt>" button in the Print Center
toolbar or select "<tt>Add printer...</tt>" from the
"Printers" menu. A new sheet will open.
<li>In the sheet click on the top popup menu and select
"<tt>IP Printing</tt>".
<li>Fill in the IP address of the printer and the queue
name in the appropriate boxes.
<li>Next, click on the "<tt>Printer Model</tt>" popup menu
and select the manufacturer for your printer, and in the
small window at the bottom of the sheet select the PPD file
for your printer model. It's important that you select the
correct PPD file, and the names are not too descriptive,
but if you match your printer model number with the number
in the PPD file name you should be OK.
<li> Click "<tt>Add</tt>".
<li> Print a test page.
</ol>
</p>
<li>
<h4>
Windows printer via SAMBA
</h4>
If you want to print to a shared printer connected to a
Windows XP computer try these instructions (this procedure
has not been thoroughly tested):
<ol>
<p>
<li>Open the Print Center utility (located in
<tt>/Applications/Utilities/</tt>).
<li>Hold down the option key on your keyboard and either
click on the <tt>Add</tt> button in the Print Center
toolbar or select <tt>Add printer...</tt> from the
<tt>Printers</tt> menu. A new sheet will open.
<li>In the sheet click on the top popup menu and select
<tt>Advanced</tt> from the very bottom of the list.
<li>Next, click on the <tt>Device:</tt> popup menu and
select <tt>Windows Printer via SAMBA</tt>.
<li> In the <tt>Device Name:</tt> field enter a descriptive
name for this printer.
<li>In the <tt>Device URI:</tt> field enter the URI for
this device in the form
<p>
<tt>smb://winuser:password@workgroup/server/printer </tt>
</p>
where
<ul>
<li><tt>winuser</tt> = XP login name
<li><tt>password</tt> = XP login password
<li><tt>workgroup</tt> = XP workgroup name
<li><tt>server</tt> = Computer name of the XP machine
<li><tt>printer</tt> = Share name of the shared XP printer
</ul>
<li>Finally, click on the <tt>Printer Model</tt> popup menu
and select the manufacturer for your printer, and in the
small window at the bottom of the sheet select the PPD file
for your printer model. It's important that you select the
correct PPD file, and the names are not overly descriptive,
but if you match your printer model number with the number
in the PPD file name you should be OK.
<li> Click <tt>Add</tt>.
<li> Print a test page. </p>
</ol>
</ul>
<p>
<li>
<h3>
OK, my printer is printing now, but how do I change the
print settings like paper type and resolution?
</h3>
</p> You can access the settings for the Gimp-Print driver
whenever a print sheet is open. Just click on the popup
menu that says "<tt>Copies & Pages</tt>" and choose
"<tt>Printer Features</tt>".
<p>
<li>
<h3>
Ok, I found all the settings, but what do they do?
</h3>
</p> Please see the <! a
href="http://gimp-print.sourceforge.net/p_FAQ.php3">
Gimp-Print General FAQ<! /a> for answers to these questions.
</ol>

<?require('standard_html_footer.php3');?>
