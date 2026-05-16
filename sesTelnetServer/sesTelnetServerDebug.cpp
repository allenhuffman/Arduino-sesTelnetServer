/**
 * @file sesTelnetServerDebug.cpp
 *
 * @author Allen C. Huffman
 * @copyright Copyright (c) 2026 Sub-Etha Software
 * @note Origin: https://github.com/allenhuffman
 * @note This file is being converted to the BARR-C Embedded C Coding Standard.
 *
 * @brief Debug functions for Sub-Etha Software's Arduino Telnet Server.
 *
 * @details This file contains debug functions for the Telnet Server. It provides
 * functions to output debug information about the Telnet protocol, including
 * incoming and outgoing Telnet escape sequences.
 *
 * @section history File History
 * - 2026-05-15 1.03 allenh - Created from sesTelnetServer.cpp.
 */

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

/* Private macros: all #define items, constants and function-like macros */

/* Private constants: typed, debugger-visible constants (prefer static const) */

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

/* Private structs: concrete data layouts used by this module */

/* Private static variables */

static const TelnetOptStruct telnetOpt[] FLASHMEM =
{
  { OPT_TRANSBIN,   opt_transbin    },
  { OPT_ECHO,       opt_echo        },
  { OPT_RECONNECT,  opt_reconnect   },
  { OPT_SUPGA,      opt_supga       },
  { OPT_AMSN,       opt_amsn        },
  { OPT_STATUS,     opt_status      },
  { OPT_TIMINGMARK, opt_timingmark  },
  { OPT_RCTE,       opt_rcte        },
  { OPT_OUTLINEWID, opt_outlinewid  },
  { OPT_OUTPAGESIZ, opt_outpagesiz  },
  // 10-19
  { OPT_NAOCRD,     opt_naocrd      },
  { OPT_NAOHTS,     opt_naohts      },
  { OPT_NAOHTD,     opt_naohtd      },
  { OPT_NAOFFD,     opt_naoffd      },
  { OPT_NAOVTS,     opt_naovts      },
  { OPT_NAOVTD,     opt_naovtd      },
  { OPT_NAOLFD,     opt_naolfd      },
  { OPT_EXTENDASC,  opt_extendasc   },
  { OPT_LOGOUT,     opt_logout      },
  { OPT_BM,         opt_bm          },
  // 20-29
  { OPT_DET,        opt_det         },
  { OPT_SUPDUP,     opt_supdup      },
  { OPT_SUPDUPOUT,  opt_supdupout   },
  { OPT_SENDLOC,    opt_sendloc     },
  { OPT_TERMTYPE,   opt_termtype    },
  { OPT_EOR,        opt_eor         },
  { OPT_TUID,       opt_tuid        },
  { OPT_OUTMRK,     opt_outmrk      },
  { OPT_TTYLOC,     opt_ttyloc      },
  { OPT_3270REGIME, opt_3270regime  },
  // 30-39
  { OPT_X3PAD,      opt_x3pad       },
  { OPT_NAWS,       opt_naws        },
  { OPT_TERMSPEED,  opt_termspeed   },
  { OPT_REMFLOWCTL, opt_remflowctl  },
  { OPT_LINEMODE,   opt_linemode    },
  { OPT_XDISPLOC,   opt_xdisploc    },
  { OPT_ENVIRON,    opt_environ     },
  { OPT_AUTHEN,     opt_authen      },
  { OPT_ENCRYPT,    opt_encrypt     },
  { OPT_NEWENVIRON, opt_newenviron  },
  // 40-49
  { OPT_TN3270E,    opt_tn3270e     },
  { OPT_XAUTH,      opt_xauth       },
  { OPT_CHARSET,    opt_charset     },
  { OPT_RSP,        opt_rsp         },
  { OPT_COMMPORT,   opt_commport    },
  { OPT_SUPPECHO,   opt_suppecho    },
  { OPT_STARTTLS,   opt_starttls    },
  { OPT_KERMIT,     opt_kermit      },
  { OPT_SENDURL,    opt_sendurl     },
  { OPT_FORWARDX,   opt_forwardx    },
  // 255
  { OPT_EXOPL,      opt_exopl       }
};

/* Private function prototypes */

/* Public function definitions */

/**
 * @brief Print a Telnet command.
 *
 * @param[in] type The Telnet command type to print.
 *
 * @return void No return value.
 */
void telnetPrintCmd (uint8_t type)
{
  Serial.print (F("["));
  if (type >= T_SE) //&& type<=T_IAC)
  {
    Serial.print (FLASHPTR(telnetCmd[type-T_SE]));
  }
  else
  {
    Serial.print (type);
  }
  Serial.print (F("]"));
}

/**
 * @brief Print a Telnet option.
 *
 * @param[in] opt The Telnet option to print.
 *
 * @return void No return value.
 */
void telnetPrintOpt (uint8_t opt)
{
  bool found;

  found = false;
  Serial.print (F("["));
  for (unsigned int i=0; i<(sizeof(telnetOpt)/sizeof(*telnetOpt)); i++)
  {
    if (pgm_read_byte(&telnetOpt[i].code) == opt)
    {
      Serial.print (FLASHPTR(telnetOpt[i].name));
      found = true;
      break;
    }
  }
  if (!found) Serial.print (opt, HEX);
  Serial.print (F("]"));
}

/**
 * @brief Print a hexadecimal value.
 *
 * @param[in] val The value to print in hexadecimal.
 *
 * @return void No return value.
 */
void telnetPrintHex (uint8_t val)
{
  Serial.print (F("["));
  Serial.print (val, HEX);
  Serial.print (F("]"));
}

/* Private function definitions */

/*** end of file ***/
