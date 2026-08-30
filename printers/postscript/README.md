# X16 Edit Printer Driver for IPP Based PostScript Printer/Print Server

## Installing

The printer driver is installed by copying the file X16EDITPD-POSTSCRIPT.DRV to the
root folder of your SD card. If there already is another printer driver on the
SD card, rename the file so that it doesn't start with "X16EDITPD-".


## Using the Driver

The printer driver communicates with the printer/print server over Wi-Fi using
the Serial and Network card and its built-in ZiModem.

Press Ctrl+H (Hard Copy) to show the printing dialog in X16 Edit.

The following settings are available:

- I/O Port: Ensure that the selected port matches the jumper settings on the serial and network card.

- BAUD rate: Select the BAUD rate that matches your Serial and Network card. The BAUD rate is 2.4 kbaud by default. 
It can be changed in a terminal program by the ATB command. You can't change the BAUD rate from the printer driver.

- Paper size: The driver supports A4 or Letter size paper.

- Units: Select if using metric (millimeters) or imperial (1/32 inch) measurement units. The measurement units are
currently only used for the page margins.

- Margins: The top, left, right and bottom margins are all set to the same size.

- Font size: Font size in points.

- Line spacing: Select single, one and a half or double line spacing.

- IPP address: The network address of the printer/print. Example if having a CUPS print server: "192.168.1.2:631/printers/myprinter" or
"cupsserver.local:631/printers/myprinter".


## Error messages

- "Invalid value": An option value is out of valid range

- "UART error-check I/O port": The selected I/O port doesn't match the network card jumper settings, displayed if the I/O Port RAM is not populated

- "UART error-check configuration": Most likely cause is that the correct BAUD rate has not been selected, displayed if ZiModem doesn't respond at all to the command "AT"

- "Write timeout": The write timeout error is thrown if ZiModem doesn't read at least one byte for approximately one minute - the error might be caused by communication problems between ZiModem and
the printer/print server

- "UART communication error": Other communication issue with the network card, displayed if ZiModem responds to the command "AT" with an error message


## Setting up a Raspberry Pi as Print Server

The printer driver has been tested with a Raspberry Pi print server, which is an easy
way to set up a print server with PostScript support. If your printer doesn't natively
support PostScript, the Raspberry Pi print server converts documents to a
format understood by the printer.

Follow these steps to configure the print server:

- Install Raspberry Pi OS using the Raspberry Pi Imager. Ensure that you setup the WiFi connection. Follow the instructions at [https://www.raspberrypi.com]
- ```sudo apt-get install cups```, installs the CUPS server
- If you have a HP printer, also do ```sudo apt-get install hplip```
- ```sudo usermod -a -G lpadmin pi```, add yourself to the printer admin group, replace "pi" with your actual login name if different
- ```sudo cupsctl --remote-admin --share-printers```, allowing remote access to the CUPS server
- ```sudo systemctl restart cups```, restarting CUPS
- Power on the Raspberry Pi
- Connect your printer to the Pi
- From your computer, go to the CUPS web interface at [http://<Address_of_your_Pi>:631]
- In the web interface, click Administration and add the printer. If connected through USB it should show
up under "Local printers". When naming the printer, use a name that can be typed in PETSCII (for instance no underscores). Remember to mark the "Share This Printer" box.

The address of the printer in X16 Edit will be "<Address_of_your_Pi>:631/printers/<Your_Printer_Name>

For more in-depth information on setting up a CUPS print server, go to this [turoria](cups-server-setup.md) by Desert-Fox.


### Troubleshooting Tips

If the driver reported an error you need to fix the cause of that.

The driver might, howver, report that "document was sent to printer" even though nothing is printed. Some troubleshooting tips for this situation:

- Check that you entered the correct print server address, port and path in X16 Edit.
- Verify that the print server is connected to WLAN.
- Login to the print server and list the printer spool files by typing ```sudo ls -al /var/spool/cups```. Verify that a spool file was created after you printed the document.
- Open the spool file by typing ```sudo nano /var/spool/cups/d00001-001```. Replace d000001-001 with the actual name of the spool file. Check that it looks like a valid PostScript file.
- Copy the spool file to your home directory by typing ```sudo cp /var/spool/cups/<filename> ./``` and change its access rights with ```sudo chmod a+r <filename>```. Type ```ps2pdf <filename>``` to verify
that GhostScript can convert the PostScript file. In case the last step fails, there is either a transmission error or a driver error resulting in an invalid PostScript file. You can try to open the resulting PDF file in a PDF viewer to verify that it looks as expected.
