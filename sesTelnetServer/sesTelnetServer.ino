/**
 * @file template.c
 *
 * @author Allen C. Huffman
 * @copyright Copyright (c) 2026 Sub-Etha Software
 * @note Origin: https://github.com/allenhuffman
 * @note This file uses the Barr-C Embedded C Coding Standard.
 *
 * @brief Telnet server test program.
 *
 * @details This is an example of how to use the Sub-Etha Software Telnet
 * Server. To configure, edit "sesTelnetServerConfig.h" as appropriate.
 *
 * @section history File History
 * - 2014-03-03 1.00 allenh - Created this demo program.
 *
 * @todo Get old UNO code working on new Arduinos.
 */

 /*---------------------------------------------------------------------------*/
// System headers
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// This module's header (must be first among project headers)
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// External module headers
/*---------------------------------------------------------------------------*/

#include <Arduino.h> // for access to Serial.

#include "FlashMem.h"
#include "sesTelnetServer.h"
#include "sesTelnetServerConfig.h"

/*---------------------------------------------------------------------------*/
// Public data definitions
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// Private macros: all #define items, constants and function-like macros
/*---------------------------------------------------------------------------*/

#define INPUT_SIZE 10 // 40

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

/*---------------------------------------------------------------------------*/
// Private function prototypes
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// Public function definitions
/*---------------------------------------------------------------------------*/

/**
 * @brief Arduino setup function. Initializes the serial port and the 
 *        Telnet server.
 *
 * @return None.
 */
void setup (void)
{
  Serial.begin (9600);
  while (!Serial);

  Serial.println ();
  Serial.println (FLASHSTR(telnetID));

  telnetInit ();
} // end of setup()


/**
 * @brief Arduino loop function. Handles input from the serial port and the 
 *        Telnet server.
 *
 * @return None.
 */
void loop (void)
{
  char    buffer[INPUT_SIZE];
  uint8_t count;

  // If we are offline, we will just take local input.
  if (telnetIsOffline () == true)
  {
    Serial.print (F("[Offline]Command: "));
  }
  else if (telnetIsConnected () == true)
  {
    // Else, we are talking remotely. Print to remote and local.
    telnetPrint (F("[Telnet]Command: "));
  }
  else
  {
    // Neither mode is active.
  }

  // Get input from remote (if connected) or local.
  count = telnetInput (buffer, sizeof(buffer));

  if (255 == count) // 255=connection lost
  {
    Serial.println (F("[Connection Lost]"));
  }
  else if (count > 0U) // count is how many bytes of data we read in to buffer.
  {
    Serial.print (count);
    Serial.println (F(" bytes received from client."));

    if (strcmp_P (buffer, PSTR("BYE")) == 0)
    {
      // If we are offline currently...
      if (telnetIsOffline () == true)
      {
        // Discard any trailing line-ending bytes from the BYE command.
        while (Serial.available () > 0)
        {
          (void)Serial.read ();
        }

        // ...leave offline mode.
        telnetSetOffline (false);
      }
      
      // And, if we are connected, disconnect.
      if (telnetIsConnected () == true)
      {
        telnetDisconnect ();
      }
    } // end of "BYE" check
  }
} // end of loop()

/*---------------------------------------------------------------------------*/
// Private function definitions
/*---------------------------------------------------------------------------*/

/*** end of file ***/
