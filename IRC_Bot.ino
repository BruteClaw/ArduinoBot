
/* This is an IRC chat bot, originaly writen to connect to Twitch and be a chat control bot.  I have taken code from several places online including 
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
*/

//Includes
#if defined(ESP8266)
#include <pgmspace.h>
#else
#include <avr/pgmspace.h>
#endif
#include "config.h"
#include <SPI.h>
#include <Wire.h>
#include <Ethernet.h>
#include <Dhcp.h>
#include <Dns.h>
#include <EthernetClient.h>
#include <EthernetServer.h>
#include <EthernetUdp.h>
#include <LiquidCrystal.h>
#include <RtcDateTime.h>
#include <RtcDS3231.h>

//MP3 Comands
static int8_t Mp3_buf[8] = {0} ;

#define CMD_SET_VOLUME 0X06
#define CMD_SEL_DEV 0X09
#define DEV_TF 0X02
#define CMD_PLAY 0X0D
#define CMD_PAUSE 0X0E

//RTC define
RtcDS3231<TwoWire> Rtc(Wire);

/*Enter a MAC address for your controller below.
Must be unique and not match any other device on your network
Newer Ethernet shields have a MAC address printed on a sticker on the shield. */
byte mac[] = { 0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02 };

// Initialize the Ethernet client library
EthernetClient client;

// Connections:
// lcd1 RS to Arduino pin 28
// lcd1 RW to Arduino pin 26
// lcd1 Enable to Arduino pin 24
// lcd1 backlight to Arduino pin 22
// lcd1 pins d4, d5, d6, d7 to Arduino pins 23, 25, 27, 29
LiquidCrystal lcd1(28, 26, 24, 23, 25, 27, 29);
// lcd2 RS to Arduino pin 28
// lcd2 RW to Arduino pin 26
// lcd2 Enable to Arduino pin 30
// lcd2 backlight to Arduino pin 22
// lcd2 pins d4, d5, d6, d7 to Arduino pins 23, 25, 27, 29
LiquidCrystal lcd2(28, 26, 30, 23, 25, 27, 29);

//Pin Setup
int strbPin = 2; //strobe light connected to this pin
int backLight1 = 22; //lcd1 & lcd2 Backlight


void setup()
{
  //Set Strings
  channel = channel1;
  
  //Set Pin modes
  pinMode(strbPin, OUTPUT);
  pinMode(backLight1, OUTPUT);
  digitalWrite(strbPin, LOW);
  digitalWrite(backLight1, LOW);

  //Setting up lcd1 Screen
  lcd1.begin(40,2);              // columns, rows.  use 16,2 for a 16x2 lcd1, etc.
  lcd1.clear();                  // start with a blank screen
  lcd2.begin(40,2);              // columns, rows.  use 16,2 for a 16x2 lcd1, etc.
  lcd2.clear();
  lcd2.setCursor(0,0);           // set cursor to column 0, row 0 (the first row)
  lcd2.print("ArduinoBot");      // Startup Text
  lcd2.setCursor(0,1);
  lcd2.print("Current Channel :");
  lcd2.print(channel);

  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  /* this check is only needed on the Leonardo.  Commented out for use on Uno
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for Leonardo only.
  }*/
  Serial.println("Serial Port Opened");

  //display current time in RTC
  Serial.print("compiled: ");
  Serial.print(__DATE__);
  Serial.println(__TIME__);
  
  //Starting the Ethernet controller.
  Serial.println("Attempting to configure Ethernet using DHCP...");
  if (Ethernet.begin(mac) == 0)
  {
    Serial.println("Failed to configure Ethernet using DHCP!");
    //No Point in carring on until reset, so do nothing forever.
    for (;;)
      ;
  }

  //Printing IP address if successful.
  Serial.println("My IP address is: ");
  for (byte thisByte = 0; thisByte < 4; thisByte++)
  {
    //Print the value of each byte of the IP Address.
    Serial.print(Ethernet.localIP()[thisByte], DEC);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("====Setup Completed====");

    //--------RTC SETUP ------------
    Rtc.Begin();

    // if you are using ESP-01 then uncomment the line below to reset the pins to
    // the available pins for SDA, SCL
    // Wire.begin(0, 2); // due to limited pins, use pin 0 and 2 for SDA, SCL

    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
    printDateTime(compiled);
    Serial.println();

    if (!Rtc.IsDateTimeValid()) 
    {
        // Common Cuases:
        //    1) first time you ran and the device wasn't running yet
        //    2) the battery on the device is low or even missing

        Serial.println("RTC lost confidence in the DateTime!");

        // following line sets the RTC to the date & time this sketch was compiled
        // it will also reset the valid flag internally unless the Rtc device is
        // having an issue

        Rtc.SetDateTime(compiled);
    }

    if (!Rtc.GetIsRunning())
    {
        Serial.println("RTC was not actively running, starting now");
        Rtc.SetIsRunning(true);
    }

    RtcDateTime now = Rtc.GetDateTime();
    if (now < compiled) 
    {
        Serial.println("RTC is older than compile time!  (Updating DateTime)");
        Rtc.SetDateTime(compiled);
    }
    else if (now > compiled) 
    {
        Serial.println("RTC is newer than compile time. (this is expected)");
    }
    else if (now == compiled) 
    {
        Serial.println("RTC is the same as compile time! (not expected but all is fine)");
    }

    // never assume the Rtc was last configured by you, so
    // just clear them to your needed state
    Rtc.Enable32kHzPin(false);
    Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone); 

 // MP3 Player Setup
Serial2.begin(9600);
delay(500); //Wait for chip to inirialize
sendCommand(CMD_SEL_DEV, DEV_TF);//select the TF card
sendCommand(CMD_SET_VOLUME, 0x000F);//set volume to 15
    
}

void loop()
{
  //Connecting to the IRC server and channel
  if (!client.connected())
  {
    Serial.print("Connecting to ");
    Serial.println(server);
    lcd1.setCursor(0,0);
    lcd1.print("Connecting to ");
    lcd1.print(server);
    if (client.connect(serverC, port)) //Could not get this command to use a string var, so used a char var
    {
      Serial.println("Connected to Server");
      delay(1000);
      client.print("PASS ");
      client.println(pass);
      Serial.println("Password has been Sent");
      delay(500);
      client.print("NICK ");
      client.println(nick);
      Serial.println("Nickname has been set");
      delay(500);
      client.print("USER ");
      client.println(user);
      Serial.println("User Name has been Sent");
      delay(500);
      Serial.print("Joining Channels ");
      Serial.println(channel1);
      client.print("JOIN ");
      client.println(channel1);
      delay(500);
      Serial.println(channel2);
      client.print("JOIN ");
      client.println(channel2);
      delay(500);
      Serial.println("Channels Joined");
      Serial.println("====Connection to server Completed====");
      lcd1.setCursor(0,1);
      lcd1.print("Connect Complete");
      handle_irc_connection();
    }
    else
    {
      //If you didn't get a connection to the server, wait 2 minutes and try again.
      Serial.println("Connection Failed");
      Serial.println("Waiting for 2 minutes to try again");
      lcd1.setCursor(0,1);
      lcd1.print("Connect Failed");
      delay(120000);
    }
  }
}

//commands function
void command(String response, String title)
{
          client.print("PRIVMSG ");
          client.print(channel);
          client.print(" :");
          client.println(response);
          Serial.println(title);
          lcd2.clear();
          lcd2.setCursor(0,0);           
          lcd2.print("ArduinoBot");      
          lcd2.setCursor(0,1);
          lcd2.print("Current Channel :");
          lcd2.print(channel);
          lcd1.clear();
          lcd1.setCursor(0,0);
          lcd1.print("Last Command:");
          lcd1.setCursor(0,1);
          lcd1.print(title);
}

//MP3 Player function
void sendCommand(int8_t command, int16_t dat)
{
  delay(20);
  Mp3_buf[0] = 0x7e; //starting byte
  Mp3_buf[1] = 0xff; //version
  Mp3_buf[2] = 0x06; //the number of bytes of the command without starting byte and ending byte
  Mp3_buf[3] = command; //
  Mp3_buf[4] = 0x00;//0x00 = no feedback, 0x01 = feedback
  Mp3_buf[5] = (int8_t)(dat >> 8);//datah
  Mp3_buf[6] = (int8_t)(dat); //datal
  Mp3_buf[7] = 0xef; //ending byte
  for(uint8_t i=0; i<8; i++)//
  {
    Serial2.write(Mp3_buf[i]) ;
  }
}

//Time retrival function
#define countof(a) (sizeof(a) / sizeof(a[0]))

void printDateTime(const RtcDateTime& dt)
{
    char datestring[20];
    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u:%02u:%02u %02u/%02u/%04u"),
            dt.Hour(),
            dt.Minute(),
            dt.Second(),
            dt.Month(),
            dt.Day(),
            dt.Year() );
            client.println(datestring);
            Serial.println(datestring);
}

//IRC handeling function
void handle_irc_connection()
{
  char c;
  String msg = "";
  client.print("PRIVMSG ");
  client.print(channel);
  client.print(" :");
  client.println(".me has arrived!");
  
  while (true)
  {
    //Allowing Serial console to send messages back to the server
    if (Serial.available() > 0)
    {
      while (Serial.available() > 0)
      {
        msg += char(Serial.read());
        delay(25);
      }
      client.print("PRIVMSG ");
      client.print(channel);
      client.print(" :");
      client.println(msg);
      Serial.println(msg); //Allows for feedback to see what was sent
      msg = ""; //Clears the variable after it has been sent
    }

    //If server connection fails, stop this function and return to attempting to connect again
    if (!client.connected())
    {
      return;
    }

    //Handeling the IRC traffic and commands
    if (client.available())
    {
      //Print IRC channel traffic to serial console
      char c = client.read();
      Serial.print(c);

      //Ping Pong IRC keep alive
      if (c == 'P')
      {
        char buf[5];
        memset(buf, 0, sizeof(buf));
        buf[0] = c;
        for (int i = 1; i < 4; i++)
        {
          c = client.read();
          buf[i] = c;
        }
        if (strcmp(buf, "PING") == 0)
        {
          client.println("PONG\r");
          Serial.println(buf);
          Serial.println("Ping Pong Played");
        }
      }

      //Command Preface Character
      if (c == '%')
      {
        char buf[6];
        memset(buf, 0, sizeof(buf));
        buf[0] = c;
        for (int i = 1; i < 5; i++)
        {
          c = client.read();
          buf[i] = c;
        }

        //Commands Section
/*
        if (strcmp(buf, "<command>") == 0)
        {
          command("<response>", "<title>");
        }
 */

        //Help
        if (strcmp(buf, "%help") == 0)
        {
          command("Please use %com1 to see the list of my commands.", "Help");
        }
        
        //Testing
        if (strcmp(buf, "%test") == 0)
        {
          command("Test Failed Successfully", "Testing");
        }

        //Information
        if (strcmp(buf, "%info") == 0)
        {
          command("I am Version 3.5 of a bot built on the Arduino Mega.  Programmed by BruteClaw.  Currently only usefull as a command bot.  More features may be added at a later time.", "Information");
        }

        //Source Code
        if (strcmp(buf, "%srce") == 0)
        {
          command("My source code can be found at: https://github.com/BruteClaw/ArduinoBot", "Source Code");
        }

        //Commands 1
        if (strcmp(buf, "%com1") == 0)
        {
          command("My Current commands are: %test - Test Message, %info - Information About me, %srce - My Source Code, %time - Time where I currently am, %ston - Get the streamers attention, %stof - Turn off the strobe, %snd1 & snd0 - Start and stop music", "Commands 1");
        }

        //Strobe Light On 
        if (strcmp(buf, "%ston") == 0)
        {
          command("Strobe Light On", "Strobe On");
          digitalWrite(strbPin, HIGH);
          sendCommand(CMD_PLAY, 0X0000);//Play Folder 1
        }

        //Strobe Light Off
        if (strcmp(buf, "%stof") == 0)
        {
          command("Strobe Light Off", "Strobe Off");
          digitalWrite(strbPin, LOW);
          sendCommand(CMD_PAUSE, 0X0000);//pause playing
        }

        //Backlight On
        if (strcmp(buf, "%blon") == 0)
        {
          command("Backlight On", "Backlight On");
          digitalWrite(backLight1, HIGH);
        }

        //Backlight Off
        if (strcmp(buf, "%blof") == 0)
        {
          command("Backlight Off", "Backlight Off");
          digitalWrite(backLight1, LOW);
        }

        //Reloading the LCD
        if (strcmp(buf, "%load") == 0)
        {
          command("Screen Reloaded","Screen Reloaded");
        }

        //Play MP3s
        if (strcmp(buf, "%snd1") == 0)
        {
          command("Unz, Unz, Unz, Unz", "Deploying Unz Unz");
          sendCommand(CMD_PLAY, 0X0000);//continue playing
        }

        //Stop MP3s
        if (strcmp(buf, "%snd0") == 0)
        {
          command("Awwww :(", "Unz Unz Stopped");
          sendCommand(CMD_PAUSE, 0X0000);//pause playing
        }
        
        //These ones I did not shortend with the function due to special things
        //Time
        if (strcmp(buf, "%time") == 0)
        {
          if (!Rtc.IsDateTimeValid()) 
            {
            // Common Cuases:
            //    1) the battery on the device is low or even missing and the power line was disconnected
            client.print("PRIVMSG ");
            client.print(channel);
            client.println("RTC lost confidence in the DateTime!");
            }
          RtcDateTime now = Rtc.GetDateTime();
          client.print("PRIVMSG ");
          client.print(channel);
          client.print(" :The time is: ");
          printDateTime(now);
          Serial.println("Time");
          Serial.println();
          lcd1.clear();
          lcd1.setCursor(0,0);
          lcd1.print("Last Command:");
          lcd1.setCursor(0,1);
          lcd1.print("Time");
        }

        //Clear Screen
        if (strcmp(buf, "%clsc") == 0)
        {
          client.print("PRIVMSG ");
          client.print(channel);
          client.println(" :Screen Cleared");
          Serial.println("Clear Screen");
          lcd1.clear();
          lcd2.clear();
        }
        
        //Channel changing
        if (strcmp(buf, "%chng") == 0)
        {
          client.print("PRIVMSG ");
          client.print(channel);
          client.println(" :I am changing to another channel");
          if (channel == channel1)
          {
            channel = channel2;
          }
          else if (channel == channel2)
          {
            channel = channel1;
          }
          client.print("PRIVMSG ");
          client.print(channel);
          client.println(" :I have changed to this channel");
          Serial.println("Channel Change");
          lcd2.clear();
          lcd2.setCursor(0,0);           
          lcd2.print("ArduinoBot");      
          lcd2.setCursor(0,1);
          lcd2.print("Current Channel :");
          lcd2.print(channel);
          lcd1.clear();
          lcd1.setCursor(0,0);
          lcd1.print("Last Command:");
          lcd1.setCursor(0,1);
          lcd1.print("Changed Channels ");
        }


        //End of commands
        delay(300);
      }
    }
  }
}
