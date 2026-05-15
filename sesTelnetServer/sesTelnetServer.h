/** @file seTelnetServer.h
 *
 * @author Allen C. Huffman
 * @copyright Copyright (c) 2026 Sub-Etha Software
 * @note Origin: https://github.com/allenhuffman
 * @note This file follows the Barr-C Embedded C Coding Standard.
 *
 * @brief Public interface for the template module.
 *
 * @details This module is implemented according to the Barr Group
 * Embedded C Coding Standard (Barr-C).
 *
 * @section history File History
 * - 2026-05-15 0.00 allenh - Applied new template.
 */

#ifndef SESTELNETSERVER_H
#define SESTELNETSERVER_H

/* System headers */

#include <stdint.h>

/* External module headers */

#include <Ethernet.h> 

/* Public macros: all #define items, constants and function-like macros */

/* Public constants: typed, debugger-visible constants (prefer static const) */

/* Public typedefs: type aliases and opaque handles   */

/* Public enums */

/* Public structs: concrete data layouts used by this module */

/* Public variables */
extern const char telnetID[];   // FLASHMEM?  
extern const char telnetAYT[];  // FLASHMEM?

extern boolean  telnetConnected;
extern boolean  offlineMode;

extern EthernetClient client;

/* Public function prototypes */
void    telnetInit(void);
uint8_t telnetInput(EthernetClient client, char *cmdLine, uint8_t len);
void    telnetDisconnect (void);

#endif /* SESTELNETSERVER_H */

/*** end of file ***/
