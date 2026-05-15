sesTelnetServer
===============

Arduino telnet server.

===============================================================================

This code is designed to work with the standard Arduino ethernet libraries:

http://arduino.cc/en/Reference/Ethernet

The example has been split up to make it easy to implement.

See my original articles:

http://subethasoftware.com/2013/04/12/telnet-is-pretty-cool/
http://subethasoftware.com/2013/04/15/sub-etha-softwares-arduino-telnet-server/

REVISION
========
* 2014-03-03 allenh - Initial, hastily created README file.
* 2015-02-14 allenh - To build for Arduino 1.6.0, I needed to add some "const" declarations to some of the PROGMEM flash strings. Expanded the "RUNNING" section of this README.

FILES
=====

* README.md - this file
* sesATParser.ino - Hayes AT command parser, "+++" to enter command mode
* sesTelnetServer.ino - the actual Telnet server code
* sesTelnetServerConfig.h - IP/Mac address and some debug on/off build settings
* TelnetServerDemo.ino - end-user demo program on how to use the server

CONFIGURATION
=============

Edit the sesTelnetServerConfig.h as appropriate:

```
// Define this to make all the strings live in Flash instead of RAM.
#define USE_FLASH
```

* If defined, the sesTelnetServer will be compiled to put all strings in flash
  storage. This is recommended and even required if doing the debug build,
  which has a ton of strings. However, if your application was simple, and you
  had enough free RAM, you could comment this out and the program would run
  faster without all the extra code to access flash storage.
  
```
// Define this to include printing basic Telnet protocol information. This
// will include a bunch of Flash strings.
#define TELNET_DEBUG // takes about 1176 bytes of Flash + 14 bytes of RAM.
```

* If defined, the sesTelnetServer will print out a ton of Telnet debug
  information on the protocol/commands being parsed. This is a fun way to see
  what all is going on with Telnet, but there is no reason you would want to
  enable this for production.

```  
// Define this to use multiserver support,but only if you have fixed your
// Ethernet library to allow it. See:
// http://subethasoftware.com/2013/04/09/arduino-ethernet-and-multiple-socket-server-connections/
//#define TELNET_MULTISERVER
```

* As of the time I created this code, there was a bug in the Arduino Ethernet
  library that was preventing it from handling multiple incoming socket
  connections. I hacked the library to allow this to work, and documented my
  changes at the above link. If your library is modified, this define will
  enable a "go away" connection. If someone is connected to the Arduino, and
  a second connection is attempted, that connection will connect and a
  "server is busy, please try later" message will be sent back. Without this,
  connections would just hang and timeout while the server is in use.

```  
// Configure telnet server MAC address and IP address.
byte mac[] FLASHMEM = { 0x2A, 0xA0, 0xD8, 0xFC, 0x8B, 0xEF };
byte ip[] FLASHMEM  = { 192, 168, 0, 200};
```

* Server MAC address and IP address. Standard Arduino ethernet library stuff.

```
// Define the ID string sent to the user upon initial connection.
#define TELNETID  "Sub-Etha Software's Arduino Telnet server."

// Define the AYT (Are You There) response string.
#define TELNETAYT "Yes. Why do you ask?"
```

* These two strings are used by the Telnet protocol. The first is an ID
  message sent to the client when they first connect, and the second is the
  response to the Telnet "ARE YOU THERE" command. A compliant Telnet server
  should be able to respond to AYT with a message, so I made the actual
  message user-definable since I could not find anything in the RFC that
  said what it was supposed to be.

RUNNING
=======
 
 When configured and built, the program will monitor the Serial console and
 listen for incoming connections. If you "telnet" in, you will receive the
 TELNETID string, then be at a "[Telnet]Command:" prompt. For the demo,
 the only command implemented is "BYE" which will disconnect the session.
 
 At the console, when "[Waiting on Connection]", typing anything will enter
 the "[Offline]Command:" mode. This will disable incoming telnet
 connections until you type "BYE" to return to listening for connections.
 (This needs to be enhanced to send a message back to any incoming
 connection attempts saying "The system is in use. Please try back later."
 I will add this in the next update.)
 
 At any time from the console, whether in Offline mode, or with an active
 telnet session in progress, the Hayes-modem style escape sequence may
 be used. Typing "+++" with a pause before and after (about one second) will
 enter command mode and an "OK" prompt will show up. This could be used
 to interrupt a user and do special commands without them seeing them.
 For now, the only command would be "ATO" (Attention, Originate) to go
 back to the active Telnet session. There is nothing in the example
 to make use of this, but eventually the sesATParser code will be part
 of a larger project to use the Arduino as an ethernet gateway with
 serial/RS232 devices.
  
 For a production environment, there's probably little use for this, other
 than having a way to enter admin mode or something if that was part of
 your project.
 
 The example TelnetServerDemo can be used as the basis for your custom
 application. You will see it handles the printing of the command prompt
 and parsing the commands that come back from telnetInput().
 
 More to come...
