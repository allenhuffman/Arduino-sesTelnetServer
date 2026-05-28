#if 1
__asm volatile ("nop");
#endif
/**
 * @file template.c
 *
 * @author Allen C. Huffman
 * @copyright Copyright (c) 2026 Sub-Etha Software
 * @note Origin: https://github.com/allenhuffman
 * @note This file is being converted to the BARR-C Embedded C Coding Standard.
 *
 * @brief Sub-Etha Software's Arduino Telnet Server.
 *
 * @details This is a Telnet Server. It properly (?) parses various Telnet
 * escape sequences, and honors a few of them. It has places where all the others can
 * be trapped and handled, if needed.
 *
 * It can be compiled to use RAM storage for strings, or Flash storage.
 *
 * It can be compiled to include extensive Telnet debug output, showing all
 * the incoming and outgoing Telnet escape sequences.
 *
 * For production use, it is recommended to have TELNET_DEBUG off, and
 * USE_FLASH on.
 *
 * @section history File History
 * - 2013-04-12 0.00 allenh - First posted to www.subethasoftware.com.
 * - 2014-03-03 1.00 allenh - Posted to GitHub.
 * - 2015-02-14 1.01 allenh - Adding some "const" to make it build with 1.6.0.
 * - 2020-10-10 1.02 allenh - Removed compiler warnings, fix bug where readCmdLine()
 *                            could write past the passed-in buffer.
 * - 2026-05-15 1.03 allenh - Updated to new template and cleanup.
 * - 2026-05-17 1.04 allenh - Added support for WiFi S3 and updated to use new
 *                            sesNetwork abstraction. Improvements to handling
 *                            DO/WONT/WILL/WONT and option negotiation.
 *
 * @todo Separate Ethernet and Telnet code into separate modules.
 */
#define VERSION "1.04"

//#define TELNET_DEBUG // Uncomment to enable debug output of Telnet commands and options.

// Telnet protocol stuff.
// Reference: http://www.softpanorama.net/Net/Application_layer/telnet.shtml

/*---------------------------------------------------------------------------*/
// System headers
/*---------------------------------------------------------------------------*/

/* This module's header (must be first among project headers) */
#include "sesTelnetServer.h"
#include "sesTelnetServerConfig.h"

/*---------------------------------------------------------------------------*/
// External module headers
/*---------------------------------------------------------------------------*/

#include "sesATParser.h"
#if defined(TELNET_DEBUG)
  #include "sesTelnetServerDebug.h"
#endif

/*---------------------------------------------------------------------------*/
// Public data definitions
/*---------------------------------------------------------------------------*/

SesServer telnetServer = SesServer(23); // Server on this port.
#if defined(TELNET_MULTISERVER)
  SesServer goawayServer = SesServer(23); // Additional listener.
#endif
SesClient client;                       // Client connection.

/*---------------------------------------------------------------------------*/
// Private macros: all #define items, constants and function-like macros
/*---------------------------------------------------------------------------*/

//#define SEND_TELNET_SB

//#define telnetModeEnable(x)   (modeFlags = modeFlags | x)
//#define telnetModeDisable(x)  (modeFlags = modeFlags & ~x)
//#define telnetMode(x)           (modeFlags & x)

#define MODE_SUPGA    bit(0)
#define MODE_ECHO     bit(1)
#define MODE_LINEMODE bit(2)

// User by telnetInput()
#define NUL     0  // NULL
#define BEL     7  // Bell
#define BS      8  // Backspace
#define HT      9  // Horizontal tab
#define LF      10 // Line feed
#define VT      11 // Vertical tab
#define FF      12 // Form feed
#define CR      13 // Carriage return
#define CAN     24
#define DEL     0x7f // Delete key for some terminals.

/*---------------------------------------------------------------------------*/
// Private constants: typed, debugger-visible constants (prefer static const)
/*---------------------------------------------------------------------------*/

// From sesTelnetServerConfig.h.
const char    telnetID[]  FLASHMEM = TELNETID;
const char    telnetAYT[] FLASHMEM = TELNETAYT;

/*---------------------------------------------------------------------------*/
// Private typedefs: type aliases and opaque handles
/*---------------------------------------------------------------------------*/

typedef enum
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
} TelnetModes;

/*---------------------------------------------------------------------------*/
// Private structs: concrete data layouts used by this module
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// Private static variables
/*---------------------------------------------------------------------------*/

static bool     s_telnetConnected = false;
static bool     s_offlineMode = false;
static uint8_t  s_modeFlags = 0;                // Global option bit flags.
static uint8_t  s_mode = MODE_LOOKING_FOR_CMD;

/*---------------------------------------------------------------------------*/
// Private function prototypes
/*---------------------------------------------------------------------------*/

static bool telnetWaitForConnection(void);
static uint8_t telnetRead(SesClient &client);

static void telnetSendEsc(void);
static void telnetSendEscCmd(uint8_t cmd);
static void telnetSendEscCmd(uint8_t cmd, uint8_t option);
#if defined(SEND_TELNET_SB)
  static void telnetSendSb(uint8_t option, uint8_t val);
#endif

static bool telnetHandleWill(uint8_t opt);
static bool telnetHandleDo(uint8_t opt);

static bool telnetHandleDont(uint8_t opt);
static bool telnetHandleWont(uint8_t opt);

static bool telnetOptIsSupported(uint8_t opt);
static bool telnetHandleOptEnable(uint8_t opt);
static bool telnetHandleOptDisable(uint8_t opt);

static bool telnetModeEnable(uint8_t mode);
static bool telnetModeDisable(uint8_t mode);
static bool telnetModeIsEnabled(uint8_t mode);

static void telnetWriteByteEcho(uint8_t ch, bool echoMode);

/*---------------------------------------------------------------------------*/
// Public function definitions
/*---------------------------------------------------------------------------*/

/**
 * @brief Initialize the Ethernet Shield.
 *
 * @param[in] void No parameters.
 *
 * @return void No return value.
 */
void telnetInit (void)
{
  sesNetworkBegin();

  Serial.print(F ("Server address: "));
  Serial.println (sesNetworkLocalIP());

  telnetServer.begin ();
#if defined (TELNET_MULTISERVER)
  goawayServer.begin ();
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

  if (true == s_offlineMode)
  {
    s_offlineMode = false;
  }
  else
  {
    delay (1);
    client.stop ();
    s_telnetConnected = false;
  }

  // Reset telnetRead() mode.
  s_mode = MODE_LOOKING_FOR_CMD;
}


/**
 * @brief Read string up to bufferSize bytes. This code comes from my Hayes AT Command
 * parser, so the variables are named differently.
 *
 * @param[out] buffer The buffer to store the read string in.
 * @param[in]  bufferSize The maximum number of bytes to read (including null
 *                        terminator).
 *
 * @return The number of bytes read (not including null terminator), or 255
 *         if connection lost.
 */
uint8_t telnetInput (char *buffer, uint8_t bufferSize)
{
  uint8_t cmdLen = 0;

  if ((NULL != buffer) && (bufferSize > 0))
  {
    int     ch = 0;
    bool    done = false;
    bool    echoMode = false;

    // If NOT in offline mode, if a client is connected and Supress Go-Ahead
    // is NOT enabled, send GA to let them know we are ready to receive.
    if ((false == s_offlineMode) &&
        (client.connected () == true) && 
        (telnetModeIsEnabled (MODE_SUPGA) == false))
    {
        telnetSendEscCmd (T_GA);
    }

    // Loop until input is complete.
    while (false == done)
    {
      //ledBlink();

  #if defined(TELNET_MULTISERVER)
      // Check for secondary connection
      SesClient client2 = goawayServer.available ();

      if (client2)
      {
        if (client2.connected ())
        {
          Serial.println (F("[Secondary client connected.]"));
          client2.println ();
          client2.println (FLASHSTR(telnetID));
          client2.println (F("The system is busy right now. Please try again later."));
          delay (1);
          client2.stop ();
          Serial.println (F("[Secondary client disconnected.]"));
        }
      }
  #endif

      // Check for new connection, or loss of existing connection.

      // We only do this check if NOT in offline mode. In offline mode, we
      // are just reading local input and are ignoring the network connection.
      if (false == s_offlineMode)
      {
        // If Telent is NOT connected...
        if (false == s_telnetConnected)
        {
          // ...wait for a new connection (either Telnet or local serial).
          s_telnetConnected = telnetWaitForConnection ();

          // On fresh connection, simulate CR from client.
          buffer[0] = '\0';

          //cmdLen = 0;
          return 0;
        }
        // Else if Telnet is connected, check to see Client still connected.
        else if (client.connected () == false)
        {
          Serial.println (F("\n[Connection Lost]"));
          telnetDisconnect ();
          //cmdLen = 255;
          return 255;
        }
      }

      // Read a byte from local serial or Telnet client and check for escape
      // sequence to enter command mode.
      if (Serial.available () > 0) // Local input.
      {
        ch = Serial.read ();

        echoMode = true; // Force echo for local input.
        
        // Feed byte into cmdModeCheck(). This never returns true.
        cmdModeFeed (ch);
      }
      else if (client.available () > 0) // Telnet input.
      {
        if (false == s_offlineMode)
        {
          echoMode = telnetModeIsEnabled (MODE_ECHO);

          ch = telnetRead (client);
        }
      }
      else // No local or Telnet input.
      {
        if (cmdModeCheck () == true)
        {
          cmdMode ();

          // While local command mode is active, remote input may queue up.
          // Discard any pending Telnet bytes so resume starts clean.
          if ((true == s_telnetConnected) &&
              (true == client.connected ()))
          {
            while (client.available () > 0)
            {
              (void)client.read ();
            }
          }

          // Escape sequence entered command mode; discard buffered data.
          cmdLen = 0;
          buffer[0] = '\0';

          // Return to caller so prompt redraw remains centralized in loop().
          break;
        }

        continue; // No data. Go back to the while()...
      }

      // Process input byte.
      switch (ch)
      {
        case LF:
          // Ignore line feed. We will use CR to signify end of line.
        break;

        case CR:
          // if ((false == s_offlineMode) && (true == s_telnetConnected) &&
          //     (true == echoMode))
          // {
          //   client.write ((char)CR);
          //   client.write ((char)LF);
          // }
          // Serial.println ();
          telnetWriteByteEcho ((char)CR, echoMode);
          telnetWriteByteEcho ((char)LF, echoMode);

          buffer[cmdLen] = '\0';
          done = true;
        break;

        case CAN: // ^X
          while (cmdLen > 0)
          {
            // if ((false == s_offlineMode) && (true == s_telnetConnected) &&
            //     (true == echoMode))
            // {
            //   client.write ((char)BS);
            //   client.write (' ');
            //   client.write ((char)BS);
            // }
            // Serial.write ((char)BS);
            // Serial.write (' ');
            // Serial.write ((char)BS);
            telnetWriteByteEcho ((char)BS, echoMode);
            telnetWriteByteEcho (' ', echoMode);
            telnetWriteByteEcho ((char)BS, echoMode);
            cmdLen--;
          }
          cmdLen = 0;
        break;

        case BS:
        case DEL:
          if (cmdLen > 0)
          {
            // if ((false == s_offlineMode) && (true == s_telnetConnected) &&
            //     (true == echoMode))
            // {
            //   client.write ((char)BS);
            //   client.write (' ');
            //   client.write ((char)BS);
            // }
            // Serial.write ((char)BS);
            // Serial.write (' ');
            // Serial.write ((char)BS);
            telnetWriteByteEcho ((char)BS, echoMode);
            telnetWriteByteEcho (' ', echoMode);
            telnetWriteByteEcho ((char)BS, echoMode);
            cmdLen--;
          }
        break;

        default:
          // If there is room, store any printable characters in the cmdline.
          if (cmdLen < bufferSize-1)
          {
            if ((ch >= 32) && (ch <= 126)) // isprint(ch) does not work.
            {
              // if ((false == s_offlineMode) && (true == s_telnetConnected) &&
              //     (true == echoMode))
              // {
              //   client.write ((char)ch);
              // }
              // Serial.write ((char)ch);
              telnetWriteByteEcho ((char)ch, echoMode);
              
              buffer[cmdLen] = ch; //toupper(ch);
              
              cmdLen++;
            }
            // Ignore other nonprintable characters.
          }
          else // No room in buffer.
          {
            // if ((false == s_offlineMode) && (true == s_telnetConnected))
            // {
            //   client.write ((char)BEL); // Overflow. Ring 'dat bell.
            // }
            // Serial.write ((char)BEL);
            // CoolTerm was treating this like a character even though it was
            // not advancing the cursor. I had to enable "Handle Bell
            // Character" to make it work proper. Unexpected, so I will
            // disable this for now.
            //telnetWriteByteEcho ((char)BEL, true);
          }
        break;
      } // end of switch (ch)
    } // end of while (false == done)
  } // end of if ((NULL != buffer) && (bufferSize > 0))

  return cmdLen;
}


/**
 * @brief Print a string to the Telnet client and local serial.
 *
 * @param[in] str The string to print.
 *
 * @return None.
 */
void telnetPrint (const char *str)
{
  client.print (str);
  Serial.print (str);
}

/**
 * @brief Print a string to the Telnet client and local serial.
 *
 * @param[in] str The string to print.
 *
 * @return None.
 */
void telnetPrint (const __FlashStringHelper *str)
{
  client.print (str);
  Serial.print (str);
}

/**
 * @brief Print a string to the Telnet client and local serial.
 *
 * @param[in] str The string to print.
 *
 * @return None.
 */
void telnetPrintln (const char *str)
{
  client.println (str);
  Serial.println (str);
}

/**
 * @brief Print a string to the Telnet client and local serial.
 *
 * @param[in] str The string to print.
 *
 * @return None.
 */
void telnetPrintln (const __FlashStringHelper *str)
{
  client.println (str);
  Serial.println (str);
}


/**
 * @brief Return true if Telnet client is connected, false otherwise.
 *
 * @return true if Telnet client is connected, false otherwise.
 */
bool telnetIsConnected (void)
{
  return s_telnetConnected;  
}

/**
 * @brief Return true if Telnet client is in offline mode, false otherwise.
 *
 * @return true if Telnet client is in offline mode, false otherwise.
 */
bool telnetIsOffline (void)
{
  return s_offlineMode;  
}


/**
 * @brief Set offline mode.
 * 
 * @param[in] telnetIsOffline If true, we are in offline mode and will ignore
 * the network connection and just read from the local serial port. If false,
 * we will use the network connection if available.
 *
 * @return 
 */
void telnetSetOffline (bool telnetIsOffline)
{
  s_offlineMode = telnetIsOffline;
}


/*---------------------------------------------------------------------------*/
// Private function definitions
/*---------------------------------------------------------------------------*/

/**
 * @brief  Block and wait for an incoming network connection, or local
 *         serial keyboard connection.
 *
 * @return true if Telnet connection, false if local connection
 */
static bool telnetWaitForConnection (void)
{
  bool status = false;

  Serial.println (F("[Waiting on Connection]"));

  s_modeFlags = 0;

  while (1)
  {
    //ledBlink();

    // Check for local connection
    if (Serial.available ())
    {
      // Go offline if logging on locally.
      Serial.println (F("[Offline Connection]"));
      Serial.println ();
      Serial.println (FLASHSTR(telnetID));

      s_offlineMode = true;
      status = false; // Local connection.

      // Discard any pending input from the serial buffer.
      while (Serial.available () > 0)
      {
        (void)Serial.read ();
      }
      
      break;
    }
    // Else check for Telnet connection.
    else 
    {
      client = telnetServer.available ();
      
      if (client)
      {
        Serial.println (F("[Telnet Connection]"));
        client.println ();
        client.println (FLASHSTR(telnetID));

        // Wait to see what the client has to say, if anything.
        delay (100); // Quick pause.
        while (telnetRead (client));

#if defined(SEND_TELNET_SB)
        // So... What is your terminal type?
        telnetSendSb (OPT_TERMTYPE, 1);
        delay (100); // Quick pause.
        while (telnetRead (client));
#endif

#if 0
        // We will control the echo, too.
        if (telnetMode(MODE_ECHO) == false)
        {
          telnetSendEscCmd (DO, OPT_ECHO);
        }
        delay(100);
        while(telnetRead(client));
#endif
        // Reset telnetRead() mode.
        s_mode = MODE_LOOKING_FOR_CMD;

        status = true; // Telnet connection.
        break;
      } // end of if (client)
    } // end of if (false == offlineMode)
  } // end of while(1)

  return status;
}

/**
 * @brief Read data from Telnet connection.
 *
 * @param[in] client The network client to read from.
 *
 * @return The character read from the Telnet connection, or 0 if no data is 
 *         available.
 */
static uint8_t telnetRead (SesClient &client)
{
  uint8_t         ch = 0; // Return 0 if we don't find any data.
  bool            done = false;

  // While not done and there is data available...
  while ((false == done) && (client.available ()))
  {
    // Read character from Telnet connection.
    ch = client.read ();

    // What are we doing, currently?
    switch (s_mode)
    {
      // Normal mode. Look for commands.
      case MODE_LOOKING_FOR_CMD:
        if (T_IAC == ch)
        {
#if defined(TELNET_DEBUG)
         telnetPrintCmd(T_IAC);
#endif
          s_mode = MODE_LOOKING_FOR_TYPE;
        }
        else
        {
          // Not a command, just return it.
          done = true;
          continue; // Go back to while.
        }
      break; // end of MODE_LOOKING_FOR_CMD

      // IAC,<type of operation>,<option>
      // Looking for command type.
      case MODE_LOOKING_FOR_TYPE:
        if (ch == T_IAC)
        {
          // Two in a row is escaped, per PFudd on RFC comments.
          // http://www.faqs.org/rfcs/rfc854.html
#if defined(TELNET_DEBUG)
          telnetPrintHex (ch);
#endif
          done = true;
          continue;
        } // end of if (ch == T_IAC)

        // Print type.
#if defined(TELNET_DEBUG)
        telnetPrintCmd (ch);
#endif

        switch(ch)
        {
          case T_SE: // Subnegotiation ends.
            s_mode = MODE_DONE;
          break;

          case T_SB: // Subnegotiation follows.
            s_mode = MODE_LOOKING_FOR_SB_OPT;
          break;

          case T_DO:
            s_mode = MODE_LOOKING_FOR_DO_OPT;
          break;

          case T_DONT:
            s_mode = MODE_LOOKING_FOR_DONT_OPT;
          break;

          case T_WILL:
            s_mode = MODE_LOOKING_FOR_WILL_OPT;
          break;

          case T_WONT:
            s_mode = MODE_LOOKING_FOR_WONT_OPT;
          break;

          case T_AYT:
            s_mode = MODE_DONE;
            client.println (FLASHSTR(telnetAYT));
          break;

          // Commands with no options.
          case T_EC:
            ch = BS;
            s_mode = MODE_LOOKING_FOR_CMD;
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
            s_mode = MODE_LOOKING_FOR_CMD;
          break;

            // Anything we don't understand, we'll assume has no option...?
          default:
            s_mode = MODE_LOOKING_FOR_CMD;
          break;
        } // end of switch(ch)
      break; // end of MODE_LOOKING_FOR_TYPE

      // Looking for option.
      case MODE_LOOKING_FOR_OPT:
      case MODE_LOOKING_FOR_SB_OPT:
#if defined(TELNET_DEBUG)
        telnetPrintOpt (ch);
#endif
        if (s_mode == MODE_LOOKING_FOR_SB_OPT)
        {
          //test: mode = MODE_LOOKING_FOR_SE;
          s_mode = MODE_LOOKING_FOR_OPT_VAL;
        }
        else
        {
          //telnetHandleDo (ch);
          s_mode = MODE_DONE;
        }
      break;

      case MODE_LOOKING_FOR_DO_OPT:
#if defined(TELNET_DEBUG)
        telnetPrintOpt (ch);
#endif
        telnetHandleDo (ch);
        s_mode = MODE_DONE;
      break;

      case MODE_LOOKING_FOR_DONT_OPT:
#if defined(TELNET_DEBUG)
       telnetPrintOpt (ch);
#endif
        telnetHandleDont (ch);
        s_mode = MODE_DONE;
      break;

     case MODE_LOOKING_FOR_WILL_OPT:
#if defined(TELNET_DEBUG)
        telnetPrintOpt (ch);
#endif
        telnetHandleWill (ch);
        s_mode = MODE_DONE;
     break;

      case MODE_LOOKING_FOR_WONT_OPT:
#if defined(TELNET_DEBUG)
        telnetPrintOpt (ch);
#endif
        telnetHandleWont (ch);
        s_mode = MODE_DONE;
      break;

      case MODE_LOOKING_FOR_OPT_VAL:
#if defined(TELNET_DEBUG)
       telnetPrintHex (ch);
#endif
        s_mode = MODE_LOOKING_FOR_SE;
      break;

      // Subnegotiation stream in progress.
      case MODE_LOOKING_FOR_SE:
        if (T_IAC == ch)
        {
#if defined(TELNET_DEBUG)
          telnetPrintCmd (T_IAC);
#endif
          s_mode = MODE_LOOKING_FOR_TYPE;
        }
        else
        {
          //if (isprint(ch)) {
          //  Serial.print((char)ch);
          //} else {
#if defined(TELNET_DEBUG)
         telnetPrintHex (ch);
#endif
        //}
        }
      break;

      // Unknown mode.
      default:
        // Serial.println ("*** Unknown mode... ***");
      break;
    } // end of switch (mode)

    // If we are not in normal mode, nothing to return.
    if (s_mode != MODE_LOOKING_FOR_CMD)
    {
      ch = 0;
    }

    // If done, toggle to normal mode.
    if (MODE_DONE == s_mode)
    {
      Serial.println ();
      s_mode = MODE_LOOKING_FOR_CMD;
    }
  } // end of while(client.avaialable())
  
#if 0
  if ((ch<32) || (ch>127))
  {
    //Serial.print("Returning: ");
    Serial.print ("(");
    Serial.print (ch, DEC);
    Serial.print (")");
  }
#endif

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
  client.write (T_IAC);
#if defined(TELNET_DEBUG)
  Serial.print (F(" -> "));
  telnetPrintCmd (T_IAC);
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
  telnetSendEsc ();
  client.write (cmd);
#if defined(TELNET_DEBUG)
  telnetPrintCmd (cmd);
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
static void telnetSendEscCmd (uint8_t cmd, uint8_t option)
{
  telnetSendEscCmd (cmd);
  client.write (option);
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
  telnetSendEscCmd (T_SB, option);
  client.write (val);
  client.write (T_IAC);
  client.write (T_SE);
#if defined(TELNET_DEBUG)
  telnetPrintHex (val);
  telnetPrintCmd (T_IAC);
  telnetPrintCmd (T_SE);
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
static bool telnetHandleWill (uint8_t opt)
{
  bool status = false;

  if (true == telnetOptIsSupported (opt))
  {
    // Supported option: only reply when state actually changes.
    status = true;
    if (true == telnetHandleOptEnable (opt))
    {
      telnetSendEscCmd (T_DO, opt);
    }
  }
  else
  {
    // Unsupported option.
    telnetSendEscCmd (T_DONT, opt);
  }

  return status;
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
  bool status = false;

  if (true == telnetOptIsSupported (opt))
  {
    // Supported option: only reply when state actually changes.
    status = true;
    if (true == telnetHandleOptEnable (opt))
    {
      telnetSendEscCmd (T_WILL, opt);
    }
  }
  else
  {
    // Unsupported option.
    telnetSendEscCmd (T_WONT, opt);
  }

  return status;
}

/**
 * @brief Handle the Telnet WONT command.
 *
 * @param[in] 
 *
 * @return true if the option was disabled, false otherwise.
 */
static bool telnetHandleDont (uint8_t opt)
{
  bool status = telnetHandleOptDisable (opt);

  if (true == status)
  {
    // If we could disable it, we did, so tell them we WONT do it.
    telnetSendEscCmd (T_WONT, opt);
  }
 
  return status;
}

/**
 * @brief Handle the Telnet WONT command.
 *
 * @param[in] opt The Telnet option to handle.
 *
 * @return true if the option was disabled, false otherwise.
 */
static bool telnetHandleWont (uint8_t opt)
{
  bool status = telnetHandleOptDisable (opt);

  if (true == status)
  {
    telnetSendEscCmd (T_DONT, opt);
  }

  return status;
}


/**
 * @brief Check if a Telnet option is supported by this server.
 *
 * @param[in] opt The Telnet option to check.
 *
 * @return true if supported, false otherwise.
 */
static bool telnetOptIsSupported (uint8_t opt)
{
  bool status = false;

  switch (opt)
  {
    case OPT_ECHO:
    case OPT_SUPGA:
      status = true;
    break;

    default:
      status = false;
    break;
  }

  return status;
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
  bool wasEnabled = false;

  switch (opt)
  {
    case OPT_ECHO:
      // Turn on echo mode bit.
      wasEnabled = telnetModeEnable (MODE_ECHO);
      if (wasEnabled)
      {
        Serial.print (F("(echo mode enabled)"));
      }
    break;

  case OPT_SUPGA:
      // Turn on suppress go ahead bit.
      wasEnabled = telnetModeEnable (MODE_SUPGA);
      if (wasEnabled)
      {
        Serial.print (F("(suppress go ahead enabled)"));
      }
    break;

#if 0
    case OPT_LINEMODE:
     // Turn on line mode bit.
     wasDisabled = telnetModeEnable (MODE_LINEMODE);
     if (wasDisabled)
     {
       Serial.print(F("(line mode enabled)"));
     }
    break;
#endif

#if 0
    case OPT_TERMTYPE:
      // Just here so the client can send it to us.
    break;
#endif

    // Else, we do not do this.
    default:
      wasEnabled = false;
    break;
  } // end of switch (opt)

  // If here, tell sender we will do as they requested.
  return wasEnabled;
}

/**
 * @brief Handle disabling the Telnet option.
 *
 * @param[in] opt The Telnet option to disable. 
 *
 * @return true if the option was disabled, false otherwise.
 */
static bool telnetHandleOptDisable (uint8_t opt)
{
  bool wasDisabled = false;

  switch (opt)
  {
    case OPT_ECHO:
      wasDisabled = telnetModeDisable (MODE_ECHO);
      if (wasDisabled)
      {
        Serial.println (F("(echo mode disabled)"));
      }
    break;

    case OPT_SUPGA:
      wasDisabled = telnetModeDisable (MODE_SUPGA);
      if (wasDisabled)
      {
        Serial.println (F("(suppress go ahead disabled)"));
      }
    break;

#if 0
    case OPT_LINEMODE:
      wasDisabled = telnetModeDisable (MODE_LINEMODE);
      if (wasDisabled)
      {
        Serial.println (F("(line mode disabled)"));
      }
    break;
#endif

    // Else, we have been told not to do something we don't know how to not
    // do... What do we do? "WONT" is the only valid response. So if we
    // don't know what it is, we probably won't be doing it. Right?
    default:
      // I guess this is fine. Should we respond?
      //telnetSendEscCmd (WONT, opt);
    break;
  } // end of switch (opt)

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
  bool status = false; // Unless we find otherwise.

  // If mode is disabled.
  if (telnetModeIsEnabled (mode) == false)
  {
    // We can enable it, so do so.
    s_modeFlags = (s_modeFlags | mode);

    status = true;
  }

  return status;
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
  bool status = false; // Unless we find otherwise.
  
  // If mode is enabled.
  if (telnetModeIsEnabled (mode) == true)
  {
    // We can disable it, so do so.
    s_modeFlags = (s_modeFlags & ~mode);

    status = true;
  }
  
  return status;
}

/**
 * @brief Check if a Telnet mode is enabled.
 *
 * @param[in] mode The Telnet mode to check.
 *
 * @return true if the mode is enabled, false otherwise.
 */
static bool telnetModeIsEnabled (uint8_t mode)
{
  return (s_modeFlags & mode) != 0;
}

static void telnetWriteByteEcho(uint8_t ch, bool echoMode)
{
  if ((false == s_offlineMode) &&
      (true == s_telnetConnected) &&
      (true == echoMode))
  {
    client.write (ch);
  }

  Serial.write (ch);
}

/*** end of file ***/
