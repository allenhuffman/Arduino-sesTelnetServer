/** @file sesTelnetServerDebug.h
 *
 * @author Allen C. Huffman
 * @copyright Copyright (c) 2026 Sub-Etha Software
 * @note Origin: https://github.com/allenhuffman
 * @note This file is being converted to the BARR-C Embedded C Coding Standard.
 *
 * @brief Public interface for the template module.
 *
 * @details 
 *
 * @section history File History
 * - 2026-05-15 0.00 allenh - Applied new template.
 */

#ifndef SESTELNETSERVERDEBUG_H
#define SESTELNETSERVERDEBUG_H

/*---------------------------------------------------------------------------*/
// System headers
/*---------------------------------------------------------------------------*/

#include <stdint.h>

/*---------------------------------------------------------------------------*/
// External module headers
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// Public macros: all #define items, constants and function-like macros
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// Public constants: typed, debugger-visible constants (prefer static const)
/*---------------------------------------------------------------------------*/

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

void telnetPrintCmd(uint8_t type);
void telnetPrintHex(uint8_t val);
void telnetPrintOpt(uint8_t opt);

#endif /* SESTELNETSERVER_H */

/*** end of file ***/
