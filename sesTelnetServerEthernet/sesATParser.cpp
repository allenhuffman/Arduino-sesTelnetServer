/**
 * @file sesATParser.c
 *
 * @author Allen C. Huffman
 * @copyright Copyright (c) 2026 Sub-Etha Software
 * @note Origin: https://github.com/allenhuffman
 * @note This file is being converted to the BARR-C Embedded C Coding Standard.
 *
 * @brief Hayes AT Command Parser
 *
 * @details Demonstrates monitoring for "+++" escape sequence to enter Command
 *          Mode.
 * 
 * References:
 * 
 * http://en.wikipedia.org/wiki/Hayes_command_set
 * http://playground.arduino.cc/Code/TimingRollover
 *
 * @section history File History
 * - 2013-03-28 0.0 allenh - Initial version.
 * - 2026-05-15     allenh - Reformatting and reorganizating source.
 *
 * @todo Make an echoPrint() function that will only print if in half duplex mode.
 */

/*---------------------------------------------------------------------------*/
// System headers
/*---------------------------------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>

/*---------------------------------------------------------------------------*/
// This module's header (must be first among project headers)
/*---------------------------------------------------------------------------*/

#include "sesATParser.h"

/*---------------------------------------------------------------------------*/
// External module headers
/*---------------------------------------------------------------------------*/

#include <Arduino.h> // for Serial

/*---------------------------------------------------------------------------*/
// Public data definitions
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// Private macros: all #define items, constants and function-like macros
/*---------------------------------------------------------------------------*/

#define INBUF_SIZE      80
#define OUTBUF_SIZE     80

#define LEDBLINK_PIN    13
#define LEDBLINK_MS     1000

// Define the escape sequence.
#define ESC_GUARD_TIME  1000 // Seconds required before/after escape sequence.
#define ESC_CHARACTER   '+'  // Default escape character.
#define ESC_TIMES       3    // Number of escape characters ("+++").

#define CMDLINE_SIZE    80
#define CR              13
#define BEL             7
#define BS              8
#define CAN             24

/*---------------------------------------------------------------------------*/
// Private constants: typed, debugger-visible constants (prefer static const)
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// Private typedefs: type aliases and opaque handles
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// Private enums
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// Private structs: concrete data layouts used by this module
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// Private static variables
/*---------------------------------------------------------------------------*/

// For Command Mode.
static unsigned int escGuardTime = ESC_GUARD_TIME; // Delay before/after esc sequence.
static char         escCharacter = ESC_CHARACTER;  // Escape character

/*---------------------------------------------------------------------------*/
// Private function prototypes
/*---------------------------------------------------------------------------*/

static uint8_t readCmdLine(char *cmdLine, size_t len);

/*---------------------------------------------------------------------------*/
// Public function definitions
/*---------------------------------------------------------------------------*/

/**
 * @brief Process some Hayes modem style "AT" commands.
 *
 * @param[in] ch  The character to be checked for escape sequence.
 *
 * @return true if the escape sequence is detected, false otherwise.
 */
bool cmdModeCheck (char ch)
{
  static unsigned long escCheckTime = 0; // Next time to check.
  static uint8_t       escCounter = 0;   // Number of esc chars seen.

  // If no character is being passed in, we are just doing a check to see if
  // we are in a "wait for end guard time" mode.
  if (0 == ch)
  {
    // See if we are waiting to enter command mode.
    // if (escSequence[escCounter]=='\0')
    if (ESC_TIMES == escCounter)
    {
      // Yep, we have already found all the escape sequence characters.
      if ((long)(millis()-escCheckTime) >= 0)
      {
        // And the pause has been long enough! We found an escape sequence.
        escCounter = 0;
        escCheckTime = millis () + escGuardTime;

        return true; // Yes, it is time for Command Mode.
      }
    }
  }
  else // if (0 == ch)
  {
    // If there has been a pause since the last input character...
    if ((long)(millis () - escCheckTime) >= 0)
    {
      // Check to see if it's an escape byte.
      // if (ch==escSequence[escCounter])
      if (ch == escCharacter)
      {
        // Move to next character to look for.
        escCounter++;

        // Are we out of escape characters to check for?
        // if (escSequence[escCounter]=='\0')
        if (ESC_TIMES <= escCounter)
        {
          // Set after delay to signify end of escape sequence.
          escCheckTime = millis () + escGuardTime;
        }  
      }
      else
      {
        // Reset. Not an escape character.
        escCounter = 0;
        escCheckTime = millis () + escGuardTime;
      }
    }
    else
    {
      // Reset. Not an escape character.
      escCounter = 0;
      escCheckTime = millis () + escGuardTime;
    }
  } // end of if (ch==0) else

  return false; // No, it is not time for Command Mode.
}

/**
 * @brief Process some Hayes modem style "AT" commands.
 *
 * @param[in] num1  The first number to be compared.
 * @param[in] num2  The second number to be compared.
 *
 * @return void
 */
void cmdMode (void)
{
  char    cmdLine[CMDLINE_SIZE];
  uint8_t len;

  Serial.println ();
  Serial.println ("OK");

  while (1)
  {
    len = readCmdLine (cmdLine, sizeof (cmdLine));
    if (len > 0)
    {
      Serial.print (">");
      Serial.println (cmdLine);

      // TODO: This should be moved into flash storage.
      if (strncmp (cmdLine, "ATO", 3) == 0)
      {
        break;
      }
      else if (strncmp (cmdLine, "ATDI", 3) == 0)
      {
        Serial.println ("Telnet...");
      }
    }
  } // end of while (1)
}

/* Private function definitions */

/**
 * @brief Process some Hayes modem style "AT" commands.
 *
 * @param[in] cmdLine  The command line buffer.
 * @param[in] len      The length of the command line buffer.
 *
 * @return The length of the command line read.
 */
static uint8_t readCmdLine (char *cmdLine, size_t len)
{
  char    ch;
  uint8_t cmdLen = 0;
  bool    done;

  done = false;
  while (!done)
  {
    //ledBlink();
    if (Serial.available () > 0)
    {  
      ch = Serial.read ();

      switch(ch)
      {
        case CR:
          Serial.println ();
          cmdLine[cmdLen] = '\0';
          done = true;
        break;

        case CAN:
          Serial.println ("[CAN]");
          cmdLen = 0;
        break;

        case BS:
          if (cmdLen > 0)
          {
            Serial.write (BS);
            Serial.print (" ");
            Serial.write (BS);
            cmdLen--;
          }
        break;

        default:
          // If there is room, store any printable characters in the cmdline.
          if (cmdLen < len)
          {
            if ((ch > 31) && (ch < 127)) // isprint(ch) does not work.
            {
              Serial.print (ch);
              cmdLine[cmdLen] = toupper (ch);
              cmdLen++;
            }
          }
          else
          {
            Serial.write (BEL); // Overflow. Ring 'dat bell.
          }
        break;
      } // end of switch(ch)           
    } // end of if (Serial.available()>0)
  } // end of while(!done)

  return cmdLen;
}

/*** end of file ***/
