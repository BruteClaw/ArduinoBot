# ArduinoBot
This is an IRC chat bot, originaly writen to connect to Twitch and be a chat control bot.

I have taken code from several places online including 
http://www.hcidata.info/host2ip.htm
The Telnet Client from the example in the Arduino Library created 14 Sep 2010, modified 9 Apr 2012, by Tom Igoe
The Arduino Web Client by David A. Mellis created 18 Jan 2011 and modified by Keiran "Affix" Smith <affix@affix.me>
Modified and re-writen by
Les "Bruteclaw" Holdeman March 2015
les.holdeman@gmail.com
Updated to add lcd screen support in October 2015
December 2015 - Bug fix on lcd screen locking up.
January 2016 - Larger LCD screen update.
November 2016 - Adding a real time clock  From the DS3231 example file included with the RTC by Makuna library
December 2016 - Added a Serial MP3 player
Febuary 2017 - Updated the RTC info after library was updated.  Also added a channel change system and removed IP address from Display
March 2017 - Cleaned up some of the inculded libraries that were not being used.  Cleaned up MP3 Player comands to free memory and reset volume to new setup
Aug 2017 - Crude Multichannel support added, fixed the server connection settings
Oct 2017 - Recovered source files after they were deleted from my local system.  Cleaned up the command section to take less memory.  Moved config variable off to a second file.
Feb 2018 - Uploaded to GitHub and updated source command
Backend commands not listed to the public:
%chng - Change the current channel that the bot is talking on
%clsc - Clear the LCD screen
%load - Load info onto the LCD screen
%blon - Turn on the LCD backlight
%blof - Turn off the LCD backlight
Connections settings for server, user name and password configured in config.h 

