/** @file sesTelnetConfig.h
 *
 * @author Allen C. Huffman
 * @copyright Copyright (c) 2026 Sub-Etha Software
 * @note Origin: https://github.com/allenhuffman
 * @note This file is being converted to the BARR-C Embedded C Coding Standard.
 *
 * @brief Public interface for the template module.
 *
 * @details Configuration file for the Telnet Server.
 *
 * @section history File History
 * - 2026-XX-XX allenh - Created.
 *
 */

#ifndef SESTELNETSERVERCONFIG_H
#define SESTELNETSERVERCONFIG_H

/*---------------------------------------------------------------------------*/
// System headers
/*---------------------------------------------------------------------------*/

#include <stdint.h>

/*---------------------------------------------------------------------------*/
// External module headers
/*---------------------------------------------------------------------------*/

// Define this to make all the strings live in Flash instead of RAM.
#define USE_FLASH

// Then include this to get the FLASHMEM, FLASTSTR, and FLASHPTR defines.
#include "FlashMem.h"

/*---------------------------------------------------------------------------*/
// Public macros: all #define items, constants and function-like macros
/*---------------------------------------------------------------------------*/

// Define this to include printing basic Telnet protocol information. This
// will include a bunch of Flash strings.
//#define TELNET_DEBUG // takes about 1176 bytes of Flash + 14 bytes of RAM.

// Define this to use multiserver support,but only if you have fixed your
// Ethernet library to allow it. See:
// http://subethasoftware.com/2013/04/09/arduino-ethernet-and-multiple-socket-server-connections/
//#define TELNET_MULTISERVER

// Define the ID string sent to the user upon initial connection.
#define TELNETID  "Sub-Etha Software's Arduino Telnet server."

// Define the AYT (Are You There) response string.
#define TELNETAYT "Yes. Why do you ask?"

/*---------------------------------------------------------------------------*/
// Public constants: typed, debugger-visible constants (prefer static const)
/*---------------------------------------------------------------------------*/

// Configure telnet server MAC address and IP address. These must be comma-
// separated lists numbers (hex or deximal) in curly braces.
#define TELNET_MAC { 0x2A, 0xA0, 0xD8, 0xFC, 0x8B, 0xEF }
#define TELNET_IP  { 192, 168, 0, 200}

/*---------------------------------------------------------------------------*/
// Public typedefs: type aliases and opaque handles
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// Public enums
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// Public structs: concrete data layouts used by this module
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// Public variables
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// Public function prototypes
/*---------------------------------------------------------------------------*/

#endif /* SESTELNETSERVERCONFIG_H */

/*** end of file ***/
