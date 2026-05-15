/** @file sesATParser.h
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

#ifndef SESTATPARSER_H
#define SESTATPARSER_H

/* System headers */

#include <stdbool.h>
#include <stddef.h>     // for size_t
#include <stdint.h>

/* External module headers */

/* Public macros: all #define items, constants and function-like macros */

/* Public constants: typed, debugger-visible constants (prefer static const) */

/* Public typedefs: type aliases and opaque handles   */

/* Public enums */

/* Public structs: concrete data layouts used by this module */

/* Public variables */

/* Public function prototypes */

bool    cmdModeCheck(char ch);

void    cmdMode(void);

#endif /* SESTATPARSER_H */

/*** end of file ***/

