/*-----------------------------------------------------------------------------
 
 Flash Memory macros
 By Allen C. Huffman
 www.subethasoftware.com
 
 This header file contains macros/defines to simplify using PROGMEM flash
 storage for arrays, strings, and pointers to strings. A #define is used to
 specify whether these macros will place these items in flash (PROGMEM) or
 RAM.
 
 #define USE_FLASH
 #include "FlashMem.h"
 
 Then use the macros/defines. If USE_FLASH is not defined, the program will
 compile everything to use RAM. If USE_FLASH is defined, they will use flash
 storage. Not everything will work, since accessing flash data requires special
 functions (like memcpy_P), but for basic strings/arrays/arrays of strings it
 works great.
 
 2014-03-03 0.00 allenh - Posted to GitHub.
 2014-03-04 0.01 allenh - Added header comments, created README on how to use.
 -----------------------------------------------------------------------------*/
#ifdef USE_FLASH
#define FLASHMEM PROGMEM
#define FLASHSTR(x) (const __FlashStringHelper*)(x)
#define FLASHPTR(x) (const __FlashStringHelper*)pgm_read_word(&x)
#else
// If not using FLASH, no special casting or keywords.
#define FLASHMEM
#define FLASHSTR(x) (x) //(const char *)(x)
#define FLASHPTR(x) (x) //(const char *)(x)
#endif
//#define PGMT(pgm_ptr) (reinterpret_cast<const __FlashStringHelper *>(pgm_ptr))

