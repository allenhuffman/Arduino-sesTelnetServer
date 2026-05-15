#if 1
__asm volatile ("nop");
#endif
/**
 * @file template.c
 *
 * @author Allen C. Huffman
 * @copyright Copyright (c) 2026 Sub-Etha Software
 * @note Origin: https://github.com/allenhuffman
 * @note This file follows the Barr-C Embedded C Coding Standard.
 *
 * @brief Implementation of the module.
 *
 * @details This module is implemented according to the Barr Group
 * Embedded C Coding Standard (Barr-C).
 *
 * @section history File History
 * - 2026-XX-XX allenh - Created.
 *
 * @todo Add module-specific functionality.
 * @todo Document any API-specific edge cases.
 */

/*-----------------------------------------------------------------------------
 
 Sub-Etha Software's Arduino Telnet Server
 By Allen C. Huffman
 www.subethasoftware.com
 
 This is a Telnet Server. It properly (?) parses various Telnet escape
 sequences, and honors a few of them. It has places where all the others can
 be trapped and handled, if needed.
 
 It can be compiled to use RAM storage for strings, or Flash storage.
 
 It can be compiled to include extensive Telnet debug output, showing all
 the incoming and outgoing Telnet escape sequences.
 
 For production use, it is recommended to have TELNET_DEBUG off, and USE_FLASH
 on.
 
 2013-04-12 0.00 allenh - First posted to www.subethasoftware.com.
 2014-03-03 1.00 allenh - Posted to GitHub.
 2015-02-14 1.01 allenh - Adding some "const" to make it build with 1.6.0.
 2020-10-10 1.02 allenh - Removed compiler warnings, fix bug where readCmdLine()
                          could write past the passed-in buffer.
 
 CHECK BACK FOR UPDATES! Much more still to be done...
 -----------------------------------------------------------------------------*/
#define VERSION "1.02"

/*---------------------------------------------------------------------------*/
// Telnet protocol stuff.
// Reference: http://www.softpanorama.net/Net/Application_layer/telnet.shtml

/* System headers */

/* This module's header (must be first among project headers) */
#include "sesTelnetServer.h"
#include "sesTelnetServerConfig.h"

/* External module headers */
#include <avr/pgmspace.h>
#include <Ethernet.h>
#include <SPI.h>
#include "sesATParser.h"

/* Public data definitions */

EthernetServer telnetServer = EthernetServer(23); // Server on this port.
#if defined(TELNET_MULTISERVER)
EthernetServer goawayServer = EthernetServer(23); // Additional listener.
#endif
EthernetClient  client;                   // Client connection.
bool            telnetConnected = false;
bool            offlineMode = false;
uint8_t         modeFlags = 0;            // Global option bit flags.

/* Private macros: all #define items, constants and function-like macros */

//#define SEND_TELNET_SB

//#define telnetModeEnable(x)  (modeFlags = modeFlags | x)
//#define telnetModeDisable (x) (modeFlags = modeFlags & ~x)
#define telnetMode(x)        (modeFlags & x)

#define MODE_SUPGA    bit(0)
#define MODE_ECHO     bit(1)
#define MODE_LINEMODE bit(2)

#define NUL   0  // NULL
#define BEL   7  // Bell
#define BS    8  // Backspace
#define HT    9  // Horizontal tab
#define LF    10 // Line feed
#define VT    11 // Vertical tab
#define FF    12 // Form feed
#define CR    13 // Carriage return
#define DEL   0x7f // Delete key for some terminals.

// Commands - IAC,<type of operation>,<option>
#define T_EOF   236 // End of file?
#define T_SP    237 // Suspend process?
#define T_ABORT 238 // Abort process?
#define T_EOR   239 // End of record?
#define T_SE    240 // End of subnegotiation parameters
#define T_NOP   241 // No operation
#define T_DM    242 // Data mark
#define T_BRK   243 // Break
#define T_IP    244 // Suspend
#define T_AO    245 // Abort output
#define T_AYT   246 // Are you there?
#define T_EC    247 // Erase character
#define T_EL    248 // Erase line
#define T_GA    249 // Go ahead
#define T_SB    250 // Subnegotiation of the indicated option follows
#define T_WILL  251 // Indicates the desire to being performing
#define T_WONT  252 // Indicates the refusal to perform
#define T_DO    253 // Indicates the request that the other party performs
#define T_DONT  254 // Indicates the demand that the other party stop performing
#define T_IAC   255 // Interpet as command

// Telnet Options
// http://www.iana.org/assignments/telnet-options/telnet-options.xml
// http://www.tcpipguide.com/free/t_TelnetOptionsandOptionNegotiation-2.htm
#define OPT_TRANSBIN   0  // TRANSMIT-BINARY
#define OPT_ECHO       1  // ECHO
#define OPT_RECONNECT  2  // reconnection
#define OPT_SUPGA      3  // SUPPRESS-GO-AHEAD
#define OPT_AMSN       4  // approx message size negotiation
#define OPT_STATUS     5  // STATUS
#define OPT_TIMINGMARK 6  // TIMING-MARK
#define OPT_RCTE       7  // RCTE remote controlled trans and echo
#define OPT_OUTLINEWID 8  // output line width
#define OPT_OUTPAGESIZ 9  // output page size
#define OPT_NAOCRD     10 // output carraige return disposition
#define OPT_NAOHTS     11 // output horizontal tab stops
#define OPT_NAOHTD     12 // output horizontal tab stop disposition
#define OPT_NAOFFD     13 // output formfeed disposition
#define OPT_NAOVTS     14 // output vertical tabstops
#define OPT_NAOVTD     15 // output vertical tab disposition
#define OPT_NAOLFD     16 // output linefeed disposition
#define OPT_EXTENDASC  17 // EXTEND-ASCII extended ascii
#define OPT_LOGOUT     18 // LOGOUT
#define OPT_BM         19 // BM byte macro
#define OPT_DET        20 // DET data entry terminal
#define OPT_SUPDUP     21 // SUPDUP display protocol
#define OPT_SUPDUPOUT  22 // SUPDUP-OUTPUT
#define OPT_SENDLOC    23 // SEND-LOCATION
#define OPT_TERMTYPE   24 // TERMINAL-TYPE
#define OPT_EOR        25 // END-OF-RECORD
#define OPT_TUID       26 // TUID tacacs user id
#define OPT_OUTMRK     27 // OUTMRK output marking
#define OPT_TTYLOC     28 // TTYLOC terminal location number
#define OPT_3270REGIME 29 // 3270-REGIME telnet 3270 regime
#define OPT_X3PAD      30 // X.3-PAD
#define OPT_NAWS       31 // NAWS negotiation about window size
#define OPT_TERMSPEED  32 // TERMINAL-SPEED
#define OPT_REMFLOWCTL 33 // TOGGLE-FLOW-CONTROL
#define OPT_LINEMODE   34 // LINEMODE
#define OPT_XDISPLOC   35 // X-DISPLAY-LOCATION (XDISPLOC)
#define OPT_ENVIRON    36 // ENVIRON telnet environment
#define OPT_AUTHEN     37 // AUTHENTICATION
#define OPT_ENCRYPT    38 // ENCRYPT encryption option
#define OPT_NEWENVIRON 39 // NEW-ENVIRON telnet new environment
#define OPT_TN3270E    40 // TN3270E
#define OPT_XAUTH      41 // *XAUTH
#define OPT_CHARSET    42 // CHARSET
#define OPT_RSP        43 // *telnet remote serial port (RSP)
#define OPT_COMMPORT   44 // COM-PORT-OPTION comm port control option
#define OPT_SUPPECHO   45 // *telnet suppress local echo
#define OPT_STARTTLS   46 // *telnet start TLP
#define OPT_KERMIT     47 // KERMIT
#define OPT_SENDURL    48 // *SEND-URL
#define OPT_FORWARDX   49 // *FORWARD-X
// 50-137 Unassigned
#define OPT_EXOPL      255// EXTENDED-OPTIONS-LIST (EXOPL) extended opt list

/* Private constants: typed, debugger-visible constants (prefer static const) */

// From sesTelnetServerConfig.h.
const char    telnetID[]  FLASHMEM = TELNETID;
const char    telnetAYT[] FLASHMEM = TELNETAYT;
const uint8_t mac[]       FLASHMEM = TELNET_MAC;
const uint8_t ip[]        FLASHMEM = TELNET_IP;

#if defined(TELNET_DEBUG)
// Store these strings in Flash to save RAM.
const char SEstr[]   FLASHMEM = "SE";
const char NOPstr[]  FLASHMEM = "NO";
const char DMstr[]   FLASHMEM = "DM";
const char BRKstr[]  FLASHMEM = "BRK";
const char IPstr[]   FLASHMEM = "IP";
const char AOstr[]   FLASHMEM = "AO";
const char AYTstr[]  FLASHMEM = "AYT";
const char ECstr[]   FLASHMEM = "EC";
const char ELstr[]   FLASHMEM = "EL";
const char GAstr[]   FLASHMEM = "GA";
const char SBstr[]   FLASHMEM = "SB";
const char WILLstr[] FLASHMEM = "WILL";
const char WONTstr[] FLASHMEM = "WONT";
const char DOstr[]   FLASHMEM = "DO";
const char DONTstr[] FLASHMEM = "DONT";
const char IACstr[]  FLASHMEM = "IAC";

// Create an array of pointers to Flash strings, in Flash.
const char * const telnetCmd[] FLASHMEM = // 240-255
{
  SEstr, NOPstr, DMstr, BRKstr, IPstr, AOstr, AYTstr, ECstr,
  ELstr, GAstr, SBstr, WILLstr, WONTstr, DOstr, DONTstr, IACstr
};

// Store these strings in Flash to save RAM.
// "lowercase" are items where I couldn't find the official string
// name (not part of an RFC). More research is needed.
// 0-9
const char opt_transbin[]   FLASHMEM = "TRANSMIT-BINARY";
const char opt_echo[]       FLASHMEM = "ECHO";
const char opt_reconnect[]  FLASHMEM = "reconnection";
const char opt_supga[]      FLASHMEM = "SUPPRESS-GO-AHEAD";
const char opt_status[]     FLASHMEM = "STATUS";
const char opt_amsn[]       FLASHMEM = "amsn";
const char opt_timingmark[] FLASHMEM = "TIMING-MARK";
const char opt_rcte[]       FLASHMEM = "RCTE";
const char opt_outlinewid[] FLASHMEM = "output-line-width";
const char opt_outpagesiz[] FLASHMEM = "output-page-size";
// 10-19
const char opt_naocrd[]     FLASHMEM = "NAOCRD";
const char opt_naohts[]     FLASHMEM = "NAOHTS";
const char opt_naohtd[]     FLASHMEM = "NAOHTD";
const char opt_naoffd[]     FLASHMEM = "NAOFFD";
const char opt_naovts[]     FLASHMEM = "NAOVTS";
const char opt_naovtd[]     FLASHMEM = "NAOVTD";
const char opt_naolfd[]     FLASHMEM = "NAOLFD";
const char opt_extendasc[]  FLASHMEM = "EXTEND-ASCII";
const char opt_logout[]     FLASHMEM = "LOGOUT";
const char opt_bm[]         FLASHMEM = "BM";
// 20-29
const char opt_det[]        FLASHMEM = "DET";
const char opt_supdup[]     FLASHMEM = "SUPDUP";
const char opt_supdupout[]  FLASHMEM = "SUPDUP-OUTPUT";
const char opt_sendloc[]    FLASHMEM = "SEND-LOCATION";
const char opt_termtype[]   FLASHMEM = "TERMINAL-TYPE";
const char opt_eor[]        FLASHMEM = "END-OF-RECORD";
const char opt_tuid[]       FLASHMEM = "TUID";
const char opt_outmrk[]     FLASHMEM = "OUTMRK";
const char opt_ttyloc[]     FLASHMEM = "TTYLOC";
const char opt_3270regime[] FLASHMEM = "3270-REGIME";
// 30-39
const char opt_x3pad[]      FLASHMEM = "X.3-PAD";
const char opt_naws[]       FLASHMEM = "NAWS";
const char opt_termspeed[]  FLASHMEM = "TERMINAL-SPEED";
const char opt_remflowctl[] FLASHMEM = "TOGGLE-FLOW-CONTROL";
const char opt_linemode[]   FLASHMEM = "LINEMODE";
const char opt_xdisploc[]   FLASHMEM = "X-DISPLAY-LOCATION";
const char opt_environ[]    FLASHMEM = "ENVIRON";
const char opt_authen[]     FLASHMEM = "AUTHENTICATION";
const char opt_encrypt[]    FLASHMEM = "ENCRYPT";
const char opt_newenviron[] FLASHMEM = "NEW-ENVIRON";
// 40-49
const char opt_tn3270e[]    FLASHMEM = "TN3270E";
const char opt_xauth[]      FLASHMEM = "xauth";
const char opt_charset[]    FLASHMEM = "CHARSET";
const char opt_rsp[]        FLASHMEM = "rsp";
const char opt_commport[]   FLASHMEM = "COM-PORT-OPTION";
const char opt_suppecho[]   FLASHMEM = "suppress-echo";
const char opt_starttls[]   FLASHMEM = "start-tls";
const char opt_kermit[]     FLASHMEM = "KERMIT";
const char opt_sendurl[]    FLASHMEM = "send-url";
const char opt_forwardx[]   FLASHMEM = "forward-x";
// 255
const char opt_exopl[]      FLASHMEM = "EXTENDED-OPTIONS-LIST";

/* Private typedefs: type aliases and opaque handles  */

// Create an array of option codes and pointers to Flash strings, in Flash.
typedef struct
{
  const uint8_t code;
  const char    *name;
} TelnetOptStruct;

enum TelnetModes
{
  MODE_LOOKING_FOR_CMD,
  MODE_LOOKING_FOR_TYPE,
  MODE_LOOKING_FOR_OPT,
  MODE_LOOKING_FOR_SB_OPT,
  MODE_LOOKING_FOR_OPT_VAL,
  MODE_LOOKING_FOR_SE,
  MODE_LOOKING_FOR_DO_OPT,
  MODE_LOOKING_FOR_DONT_OPT,
  MODE_LOOKING_FOR_WILL_OPT,
  MODE_LOOKING_FOR_WONT_OPT,
  MODE_DONE
};

/* Private structs: concrete data layouts used by this module */

/* Private static variables */

const TelnetOptStruct telnetOpt[] FLASHMEM =
{
  { OPT_TRANSBIN,   opt_transbin       },
  { OPT_ECHO,       opt_echo           },
  { OPT_RECONNECT,  opt_reconnect      },
  { OPT_SUPGA,      opt_supga          },
  { OPT_AMSN,       opt_amsn           },
  { OPT_STATUS,     opt_status         },
  { OPT_TIMINGMARK, opt_timingmark     },
  { OPT_RCTE,       opt_rcte        },
  { OPT_OUTLINEWID, opt_outlinewid       },
  { OPT_OUTPAGESIZ, opt_outpagesiz       },
  // 10-19
  { 
    OPT_NAOCRD,     opt_naocrd       }
  ,
  { 
    OPT_NAOHTS,     opt_naohts       }
  ,
  { 
    OPT_NAOHTD,     opt_naohtd       }
  ,
  { 
    OPT_NAOFFD,     opt_naoffd       }
  ,
  { 
    OPT_NAOVTS,     opt_naovts       }
  ,
  { 
    OPT_NAOVTD,     opt_naovtd       }
  ,
  { 
    OPT_NAOLFD,     opt_naolfd       }
  ,
  { 
    OPT_EXTENDASC,  opt_extendasc       }
  ,
  { 
    OPT_LOGOUT,     opt_logout       }
  ,
  { 
    OPT_BM,         opt_bm       }
  ,
  // 20-29
  { 
    OPT_DET,        opt_det       }
  ,
  { 
    OPT_SUPDUP,     opt_supdup       }
  ,
  { 
    OPT_SUPDUPOUT,  opt_supdupout       }
  ,
  { 
    OPT_SENDLOC,    opt_sendloc       }
  ,
  { 
    OPT_TERMTYPE,   opt_termtype       }
  ,
  { 
    OPT_EOR,        opt_eor       }
  ,
  { 
    OPT_TUID,       opt_tuid       }
  ,
  { 
    OPT_OUTMRK,     opt_outmrk       }
  ,
  { 
    OPT_TTYLOC,     opt_ttyloc       }
  ,
  { 
    OPT_3270REGIME, opt_3270regime       }
  ,
  // 30
  { 
    OPT_X3PAD,      opt_x3pad       }
  ,
  { 
    OPT_NAWS,       opt_naws       }
  ,
  { 
    OPT_TERMSPEED,  opt_termspeed       }
  ,
  { 
    OPT_REMFLOWCTL, opt_remflowctl       }
  ,
  { 
    OPT_LINEMODE,   opt_linemode       }
  ,
  { 
    OPT_XDISPLOC,   opt_xdisploc       }
  ,
  { 
    OPT_ENVIRON,    opt_environ       }
  ,
  { 
    OPT_AUTHEN,     opt_authen       }
  ,
  { 
    OPT_ENCRYPT,    opt_encrypt       }
  ,
  { 
    OPT_NEWENVIRON, opt_newenviron       }
  ,
  // 40-49
  { 
    OPT_TN3270E,    opt_tn3270e       }
  ,
  { 
    OPT_XAUTH,      opt_xauth       }
  ,
  { 
    OPT_CHARSET,    opt_charset       }
  ,
  { 
    OPT_RSP,        opt_rsp       }
  ,
  { 
    OPT_COMMPORT,   opt_commport       }
  ,
  { 
    OPT_SUPPECHO,   opt_suppecho       }
  ,
  { 
    OPT_STARTTLS,   opt_starttls       }
  ,
  { 
    OPT_KERMIT,     opt_kermit       }
  ,
  { 
    OPT_SENDURL,    opt_sendurl       }
  ,
  { 
    OPT_FORWARDX,   opt_forwardx       }
  ,
  // 255
  { 
    OPT_EXOPL,      opt_exopl       }
};
#endif // #if defined(TELNET_DEBUG)

/* Private function prototypes */
static bool telnetHandleDo (uint8_t opt);
static bool telnetHandleDont(uint8_t opt);

static bool telnetHandleOptEnable (uint8_t opt);
static bool telnetHandleOptDisable (uint8_t opt);

static bool telnetHandleWill(uint8_t opt);
static bool telnetHandleWont(uint8_t opt);

static bool telnetModeEnable (uint8_t mode);
static bool telnetModeDisable (uint8_t mode);

static void telnetPrintCmd (uint8_t type);
static void telnetPrintHex (uint8_t val);
static void telnetPrintOpt(uint8_t opt);

static void telnetSendEsc(void);
static void telnetSendEscCmd(uint8_t cmd);
static void telnetSendEscCmd(uint8_t cmd, uint8_t option);

#if defined(SEND_TELNET_SB)
  static void telnetSendSb(uint8_t option, uint8_t val);
#endif

static bool telnetWaitForConnection (void);
static uint8_t telnetRead(EthernetClient client);

/* Public function definitions */

/**
 * @brief Initialize the Ethernet Shield.
 *
 * @param[in] void No parameters.
 *
 * @return void No return value.
 */
void telnetInit (void)
{
  // Temporary arrays so we don't waste RAM storing them.
  uint8_t tempMac[6];
  uint8_t tempIp[4];

  memcpy_P(tempMac, mac, 6);
  memcpy_P(tempIp, ip, 4);

  Ethernet.begin(tempMac, tempIp);

  Serial.print(F("Server address: "));
  Serial.println (Ethernet.localIP());

  telnetServer.begin();
#if defined(TELNET_MULTISERVER)
  goawayServer.begin();
#endif
}


/**
 * @brief Disconnect the Telnet client.
 *
 * @param[in] void No parameters.
 *
 * @return void No return value.
 */
void telnetDisconnect (void)
{
  Serial.println (F("[Closing Connection]"));
  if (offlineMode)
  {
    offlineMode = false;
  }
  else
  {
    delay(1);
    client.stop();
    telnetConnected = false;
  }
}


/**
 * @brief Read string up to len bytes. This code comes from my Hayes AT Command
 * parser, so the variables are named differently.
 *
 * @param[in]  client  The EthernetClient to read from.
 * @param[out] cmdLine The buffer to store the read string in.
 * @param[in]  len     The maximum number of bytes to read (including null
 *                     terminator).
 *
 * @return The number of bytes read (not including null terminator), or 255
 *         if connection lost.
 */
#define CR           13
#define BEL          7
#define BS           8
#define CAN          24
uint8_t telnetInput (EthernetClient client, char *cmdLine, uint8_t len)
{
  int     ch;
  uint8_t cmdLen = 0;
  bool done;
  bool echoMode;

  // We cannot read zero bytes, so we won't even try.
  if (len==0) return 0;

  if (!offlineMode)
  {
    // Do we need to let them know they can send us stuff?
    if (client.connected() && !telnetMode(MODE_SUPGA)) telnetSendEscCmd(T_GA);
  }

  done = false;
  while(!done)
  {
    //ledBlink();

    // We use this multiple places, so do it once.
    echoMode = telnetMode(MODE_ECHO);

#if defined(TELNET_MULTISERVER)
    // Check for secondary connection
    EthernetClient client2 = goawayServer.available();
    if (client2)
    {
      if (client2.connected())
      {
        Serial.println (F("[Secondary client connected.]"));
        client2.println();
        client2.println(FLASHSTR(telnetID));
        client2.println(F("The system is busy right now. Please try again later."));
        delay(1);
        client2.stop();
        Serial.println (F("[Secondary client disconnected.]"));
      }
    }
#endif

    if (!offlineMode)
    {
      if (telnetConnected==false)
      {
        telnetConnected = telnetWaitForConnection();
        // If serial (false), start inputting?
        //if (telnetConnected==false) continue;
        // Otherwise...
        // On fresh connection, simulate CR from client.
        cmdLine[0] = '\0';
        return 0;
      }
      else if (client.connected()==false)
      {
        Serial.println (F("\n[Connection Lost]"));
        telnetDisconnect ();
        return 255;
      }
    }

    ch = -1; // -1 is no data available
    if (Serial.available()>0)
    {
      ch = Serial.read();
      if (cmdModeCheck (ch)==true) cmdMode();
      // Make sure we echo local typing to the remote client.
      echoMode = true;
    }
    else if (client.available()>0)
    {
      //ch = client.read();
      if (!offlineMode) ch = telnetRead(client);
    }
    else
    {
      if (cmdModeCheck (0)==true) cmdMode();
      continue; // No data. Go back to the while()...
    }
    switch(ch)
    {
    case -1: // No data available.
      break;

    case CR:
      //if (echoMode)
      if (!offlineMode && telnetConnected)
      {
        client.write((char)CR);
        client.write((char)LF);
      }
      Serial.println ();
      cmdLine[cmdLen] = '\0';
      done = true;
      break;

    case CAN: // ^X
      //print(F("[CAN]"));
      while(cmdLen>0)
      {
        if (!offlineMode && telnetConnected && echoMode)
        {
          client.write((char)BS);
          client.print(F(" "));
          client.write((char)BS);
        }
        Serial.write((char)BS);
        Serial.print(F(" "));
        Serial.write((char)BS);
        cmdLen--;
      }
      cmdLen = 0;
      break;

    case BS:
    case DEL:
      if (cmdLen>0)
      {
        if (!offlineMode && telnetConnected && echoMode)
        {
          client.write((char)BS);
          client.print(F(" "));
          client.write((char)BS);
        }
        Serial.write((char)BS);
        Serial.print(F(" "));
        Serial.write((char)BS);
        cmdLen--;
      }
      break;

    default:
      // If there is room, store any printable characters in the cmdline.
      if (cmdLen<len-1)
      {
        if ((ch>=32) && (ch<=128)) // isprint(ch) does not work.
        {
          if (!offlineMode && telnetConnected && echoMode) client.write((char)ch);
          Serial.write((char)ch);
          cmdLine[cmdLen] = ch; //toupper(ch);
          cmdLen++;
        }
        // Ignore other nonprintable characters.
      }
      else
      {
        //if (echoMode) ???
        if (!offlineMode && telnetConnected) client.write((char)BEL); // Overflow. Ring 'dat bell.
        Serial.write((char)BEL);
      }
      break;
    } // end of switch(ch)
  } // end of while(!done)

  return cmdLen;
}

/* Private function definitions */

/**
 * @brief  Block and wait for an incoming Ethernet TCP connection, or local
 *         keyboard connection.
 *
 * @param[in] 
 *
 * @return true if Telnet connection, false if local connection
 */
static bool telnetWaitForConnection (void)
{
  Serial.println (F("[Waiting on Connection]"));
  modeFlags = 0;
  while(1)
  {
    //ledBlink();

    // Check for local connection
    if (Serial.available())
    {
      // Go offline if logging on locally.
      Serial.println (F("[Offline Connection]"));
      Serial.println ();
      Serial.println (FLASHSTR(telnetID));

      offlineMode = true;
      return false;
    }

    if (!offlineMode)
    {
      client = telnetServer.available();
      if (client)
      {
        //telnetConnected = true;
        Serial.println (F("[New Connection]"));

        client.println();
        client.println(FLASHSTR(telnetID));

        // Wait to see what the client has to say, if anything.
        delay(100); // half second pause
        while(telnetRead(client));

#if defined(SEND_TELNET_SB)
        // So... What is your terminal type?
        telnetSendSb(OPT_TERMTYPE, 1);
        delay(100); // half second pause
        while(telnetRead(client));
#endif
        /*
        // We will control the echo, too.
        if (telnetMode(MODE_ECHO)==false)
        {
          telnetSendEscCmd(DO, OPT_ECHO);
        }
        delay(100);
        while(telnetRead(client));
        */
        return true;
      }
    }
  }
}

/**
 * @brief Read data from Telnet connection.
 *
 * @param[in] client The Ethernet client to read from.
 *
 * @return The character read from the Telnet connection, or 0 if no data is 
 *         available.
 */
static uint8_t telnetRead (EthernetClient client)
{
  static uint8_t  mode = MODE_LOOKING_FOR_CMD;
  uint8_t         ch;
  bool            done = false;

  ch = 0; // Return 0 if we don't find any data.
  done = false;
  // While not done and there is data available...
  while (!done && client.available())
  {
    // Read character from telnet connection.
    ch = client.read();

    // What are we doing, currently?
    switch(mode)
    {
      // Normal mode. Look for commands.
    case MODE_LOOKING_FOR_CMD:
      if (ch==T_IAC)
      {
#if defined(TELNET_DEBUG)
        telnetPrintCmd(T_IAC);
#endif
        mode = MODE_LOOKING_FOR_TYPE;
      }
      else
      {
        // Not a command, just return it.
        done = true;
        continue; // Go back to while.
      }
      break;

      // IAC,<type of operation>,<option>
      // Looking for command type.
    case MODE_LOOKING_FOR_TYPE:
      if (ch==T_IAC)
      {
        // Two in a row is escaped, per PFudd on RFC comments.
        // http://www.faqs.org/rfcs/rfc854.html
#if defined(TELNET_DEBUG)
        telnetPrintHex(ch);
#endif
        done = true;
        continue;
      }
      // Print type.
#if defined(TELNET_DEBUG)
      telnetPrintCmd(ch);
#endif
      switch(ch)
      {
      case T_SE: // Subnegotiation ends.
        mode = MODE_DONE;
        break;

      case T_SB: // Subnegotiation follows.
        mode = MODE_LOOKING_FOR_SB_OPT;
        break;

      case T_DO:
        mode = MODE_LOOKING_FOR_DO_OPT;
        break;

      case T_DONT:
        mode = MODE_LOOKING_FOR_DONT_OPT;
        break;

      case T_WILL:
        mode = MODE_LOOKING_FOR_WILL_OPT;
        break;

      case T_WONT:
        mode = MODE_LOOKING_FOR_WONT_OPT;
        break;

      case T_AYT:
        mode = MODE_DONE;
        client.println(FLASHSTR(telnetAYT));
        break;

        // Commands with no options.
      case T_EC:
        ch = BS;
        mode = MODE_LOOKING_FOR_CMD;
        break;

      case T_EOF:
      case T_SP:
      case T_ABORT:
      case T_EOR:
      case T_NOP:
      case T_DM:
      case T_BRK:
      case T_IP:
      case T_AO:
      case T_EL:
      case T_GA:
        mode = MODE_LOOKING_FOR_CMD;
        break;

        // Anything we don't understand, we'll assume has no option...?
      default:
        mode = MODE_LOOKING_FOR_CMD;
        break;
      } // end of switch(ch)
      break;

      // Looking for option.
    case MODE_LOOKING_FOR_OPT:
    case MODE_LOOKING_FOR_SB_OPT:
#if defined(TELNET_DEBUG)
      telnetPrintOpt (ch);
#endif
      if (mode==MODE_LOOKING_FOR_SB_OPT)
      {
        //test: mode = MODE_LOOKING_FOR_SE;
        mode = MODE_LOOKING_FOR_OPT_VAL;
      }
      else
      {
        //telnetHandleDo (ch);
        mode = MODE_DONE;
      }
      break;

    case MODE_LOOKING_FOR_DO_OPT:
#if defined(TELNET_DEBUG)
      telnetPrintOpt (ch);
#endif
      telnetHandleDo (ch);
      mode = MODE_DONE;
      break;

    case MODE_LOOKING_FOR_DONT_OPT:
#if defined(TELNET_DEBUG)
      telnetPrintOpt (ch);
#endif
      telnetHandleDont(ch);
      mode = MODE_DONE;
      break;

    case MODE_LOOKING_FOR_WILL_OPT:
#if defined(TELNET_DEBUG)
      telnetPrintOpt (ch);
#endif
      telnetHandleWill(ch);
      mode = MODE_DONE;
      break;

    case MODE_LOOKING_FOR_WONT_OPT:
#if defined(TELNET_DEBUG)
      telnetPrintOpt (ch);
#endif
      telnetHandleWont(ch);
      mode = MODE_DONE;
      break;

    case MODE_LOOKING_FOR_OPT_VAL:
#if defined(TELNET_DEBUG)
      telnetPrintHex(ch);
#endif
      mode = MODE_LOOKING_FOR_SE;
      break;

      // Subnegotiation stream in progress.
    case MODE_LOOKING_FOR_SE:
      if (ch==T_IAC)
      {
#if defined(TELNET_DEBUG)
        telnetPrintCmd(T_IAC);
#endif
        mode = MODE_LOOKING_FOR_TYPE;
      }
      else
      {
        //if (isprint(ch)) {
        //  Serial.print((char)ch);
        //} else {
#if defined(TELNET_DEBUG)
        telnetPrintHex(ch);
#endif
        //}
      }
      break;

      // Unknown mode.
    default:
      // Serial.println ("*** Unknown mode... ***");
      break;
    }

    // If we are not in normal mode, nothing to return.
    if (mode!=MODE_LOOKING_FOR_CMD) ch = 0;

    // If done, toggle to normal mode.
    if (mode==MODE_DONE)
    {
      Serial.println ();
      mode = MODE_LOOKING_FOR_CMD;
    }
  } // end of while(client.avaialable())
  /*
  if (ch<32 || ch>127) {
   //Serial.print("Returning: ");
   Serial.print("(");
   Serial.print(ch, DEC);
   Serial.print(")");
   }
   */
  return ch;
}

/**
 * @brief Send the Telnet escape character.
 *
 * @param[in] 
 *
 * @return 
 */
static void telnetSendEsc (void)
{
  client.write(T_IAC);
#if defined(TELNET_DEBUG)
  Serial.print(F(">"));
  telnetPrintCmd(T_IAC);
#endif
}

/**
 * @brief Send the Telnet escape character, followed by a command.
 *
 * @param[in] cmd The Telnet command to send.
 *
 * @return 
 */
static void telnetSendEscCmd (uint8_t cmd)
{
  telnetSendEsc();
  client.write(cmd);
#if defined(TELNET_DEBUG)
  telnetPrintCmd(cmd);
#endif
}

/**
 * @brief Send the Telnet escape character, followed by a command and option.
 *
 * @param[in] cmd The Telnet command to send.
 * @param[in] option The Telnet option to send.
 *
 * @return 
 */
static void telnetSendEscCmd(uint8_t cmd, uint8_t option)
{
  telnetSendEscCmd(cmd);
  client.write(option);
#if defined(TELNET_DEBUG)
  telnetPrintOpt (option);
#endif
}

#if defined(SEND_TELNET_SB)
/**
 * @brief Send the Telnet subnegotiation command, followed by an option and
 *        value.
 *
 * @param[in] option The Telnet option to send.
 * @param[in] val The value to send.
 *
 * @return 
 */
static void telnetSendSb (uint8_t option, uint8_t val)
{
  telnetSendEscCmd(T_SB, option);
  client.write(val);
  client.write(T_IAC);
  client.write(T_SE);
#if defined(TELNET_DEBUG)
  telnetPrintHex(val);
  telnetPrintCmd(T_IAC);
  telnetPrintCmd(T_SE);
#endif
}
#endif

/**
 * @brief Handle the Telnet WILL command.
 *
 * @param[in] opt The Telnet option to handle.
 *
 * @return true if the option was enabled, false otherwise.
 */
// If the server asks us if we WILL use an option, if we will, we should
// respond and tell them we DO, or DONT.
static bool telnetHandleWill(uint8_t opt)
{
  if (telnetHandleOptEnable (opt)==true)
  {
    telnetSendEscCmd(T_DO, opt);
    return true;
  }
  telnetSendEscCmd(T_DONT, opt);
  return false;
}

/**
 * @brief Handle the Telnet DO command.
 *
 * @param[in] opt The Telnet option to handle.
 *
 * @return true if the option was enabled, false otherwise.
 */
static bool telnetHandleDo (uint8_t opt)
{
  if (telnetHandleOptEnable (opt)==true)
  {
    telnetSendEscCmd(T_WILL, opt);
    return true;
  }
  telnetSendEscCmd(T_WONT, opt);
  return false;
}

/**
 * @brief Handle the Telnet WONT command.
 *
 * @param[in] 
 *
 * @return true if the option was disabled, false otherwise.
 */
static bool telnetHandleDont(uint8_t opt)
{
  if (telnetHandleOptDisable(opt)==true)
  {
    // Tell them we WONT use it.
    telnetSendEscCmd(T_WONT, opt);
    return true;
  }
  // If we can't not do the option, what should we do? Ignore for now.
  return false;
}

/**
 * @brief Handle the Telnet WONT command.
 *
 * @param[in] opt The Telnet option to handle.
 *
 * @return true if the option was disabled, false otherwise.
 */
static bool telnetHandleWont(uint8_t opt)
{
  if (telnetHandleOptDisable(opt)==true)
  {
    // Tell them we DONT use it.
    telnetSendEscCmd(T_DONT, opt);
    return true;
  }
  // If we can't not do the option, what should we do? Ignore for now.
  return false;
}

/**
 * @brief Handle enabling the Telnet option.
 *
 * @param[in] opt The Telnet option to enable.
 *
 * @return true if the option was enabled, false otherwise.
 */
// Enable the option, if we can.
// true = we can, and we did.
// false = we cannot, and we did not.
static bool telnetHandleOptEnable (uint8_t opt)
{
  switch(opt)
  {
  case OPT_ECHO:
    // Turn on echo mode bit.
    telnetModeEnable(MODE_ECHO);
    Serial.print(F("(echo mode enabled)"));
    break;

  case OPT_SUPGA:
    telnetModeEnable(MODE_SUPGA);
    Serial.print(F("(suppress go ahead enabled)"));
    break;

    /*
    case OPT_LINEMODE:
     telnetModeEnable(MODE_LINEMODE);
     Serial.print(F("(line mode enabled)"));
     break;
     */
    /*
    case OPT_TERMTYPE:
     // Just here so the client can send it to us.
     break;
     */
    // Else, we do not do this. Let them know.
  default:
    return false;
  }
  // If here, tell sender we will do as they requested.
  return true;
}

/**
 * @brief Handle disabling the Telnet option.
 *
 * @param[in] opt The Telnet option to disable. 
 *
 * @return true if the option was disabled, false otherwise.
 */
// Disable the option, if we can.
// true = we did
// false = we did not
static bool telnetHandleOptDisable (uint8_t opt)
{
  bool wasDisabled = false;
  switch(opt)
  {
  case OPT_ECHO:
    wasDisabled = telnetModeDisable (MODE_ECHO);
    if (wasDisabled) Serial.println (F("(echo mode disabled)"));
    break;

  case OPT_SUPGA:
    wasDisabled = telnetModeDisable (MODE_SUPGA);
    if (wasDisabled) Serial.println (F("(suppress go ahead disabled)"));
    break;
    /*
    case OPT_LINEMODE:
     wasDisabled = telnetModeDisable (MODE_LINEMODE);
     if (wasDisabled) Serial.println (F("(line mode disabled)"));
     break;
     */
    // Else, we have been told not to do something we don't know how to not
    // do... What do we do? "WONT" is the only valid response. So if we
    // don't know what it is, we probably won't be doing it. Right?
  default:
    // I guess this is fine. Should we respond?
    //telnetSendEscCmd(WONT, opt);
    return false; // Let called know we had a request we didn't handle.
  }
  // If here, tell the sended we wont do what they asked us not to do.
  //telnetSendEscCmd(WONT, opt);
  return wasDisabled;
}

/**
 * @brief Enable a Telnet mode.
 *
 * @param[in] mode The Telnet mode to enable.
 *
 * @return true if the mode was enabled, false otherwise.
 */
static bool telnetModeEnable (uint8_t mode)
{
  modeFlags = modeFlags | mode;
  return true;
}

/**
 * @brief Disable a Telnet mode.
 *
 * @param[in] mode The Telnet mode to disable.
 *
 * @return true if the mode was disabled, false otherwise.
 */
static bool telnetModeDisable (uint8_t mode)
{
  if (telnetMode(mode)==false) return false;
  modeFlags = modeFlags & ~mode;
  return true;
}

/*---------------------------------------------------------------------------*/
// Telnet Protocol debug stuff. These routines will print out text versions
// of the Telnet protocol messages sent and recieved.

#if defined(TELNET_DEBUG)
/**
 * @brief Print a Telnet command.
 *
 * @param[in] type The Telnet command type to print.
 *
 * @return void No return value.
 */
static void telnetPrintCmd (uint8_t type)
{
  Serial.print(F("["));
  if (type>=T_SE) //&& type<=T_IAC)
  {
    Serial.print(FLASHPTR(telnetCmd[type-T_SE]));
  }
  else
  {
    Serial.print(type);
  }
  Serial.print(F("]"));
}

/**
 * @brief Print a Telnet option.
 *
 * @param[in] opt The Telnet option to print.
 *
 * @return void No return value.
 */
static void telnetPrintOpt (uint8_t opt)
{
  bool found;

  found = false;
  Serial.print(F("["));
  for (unsigned int i=0; i<(sizeof(telnetOpt)/sizeof(*telnetOpt)); i++)
  {
    if (pgm_read_byte(&telnetOpt[i].code) == opt)
    {
      Serial.print(FLASHPTR(telnetOpt[i].name));
      found = true;
      break;
    }
  }
  if (!found) Serial.print(opt, HEX);
  Serial.print(F("]"));
}

/**
 * @brief Print a hexadecimal value.
 *
 * @param[in] val The value to print in hexadecimal.
 *
 * @return void No return value.
 */
static void telnetPrintHex (uint8_t val)
{
  Serial.print(F("["));
  Serial.print(val, HEX);
  Serial.print(F("]"));
}
#endif // #if defined(TELNET_DEBUG)

/*** end of file ***/
