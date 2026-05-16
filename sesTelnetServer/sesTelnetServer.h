/** @file seTelnetServer.h
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

#ifndef SESTELNETSERVER_H
#define SESTELNETSERVER_H

/* System headers */

#include <stdint.h>

/* External module headers */

#include <Ethernet.h> // for EthernetClient 

/* Public macros: all #define items, constants and function-like macros */

// Commands - IAC,<type of operation>,<option>
#define T_EOF           236 // End of file?
#define T_SP            237 // Suspend process?
#define T_ABORT         238 // Abort process?
#define T_EOR           239 // End of record?
#define T_SE            240 // End of subnegotiation parameters
#define T_NOP           241 // No operation
#define T_DM            242 // Data mark
#define T_BRK           243 // Break
#define T_IP            244 // Suspend
#define T_AO            245 // Abort output
#define T_AYT           246 // Are you there?
#define T_EC            247 // Erase character
#define T_EL            248 // Erase line
#define T_GA            249 // Go ahead
#define T_SB            250 // Subnegotiation of the indicated option follows
#define T_WILL          251 // Indicates the desire to being performing
#define T_WONT          252 // Indicates the refusal to perform
#define T_DO            253 // Indicates the request that the other party performs
#define T_DONT          254 // Indicates the demand that the other party stop performing
#define T_IAC           255 // Interpet as command

// Telnet Options
// http://www.iana.org/assignments/telnet-options/telnet-options.xml
// http://www.tcpipguide.com/free/t_TelnetOptionsandOptionNegotiation-2.htm
#define OPT_TRANSBIN    0  // TRANSMIT-BINARY
#define OPT_ECHO        1  // ECHO
#define OPT_RECONNECT   2  // reconnection
#define OPT_SUPGA       3  // SUPPRESS-GO-AHEAD
#define OPT_AMSN        4  // approx message size negotiation
#define OPT_STATUS      5  // STATUS
#define OPT_TIMINGMARK  6  // TIMING-MARK
#define OPT_RCTE        7  // RCTE remote controlled trans and echo
#define OPT_OUTLINEWID  8  // output line width
#define OPT_OUTPAGESIZ  9  // output page size
#define OPT_NAOCRD      10 // output carraige return disposition
#define OPT_NAOHTS      11 // output horizontal tab stops
#define OPT_NAOHTD      12 // output horizontal tab stop disposition
#define OPT_NAOFFD      13 // output formfeed disposition
#define OPT_NAOVTS      14 // output vertical tabstops
#define OPT_NAOVTD      15 // output vertical tab disposition
#define OPT_NAOLFD      16 // output linefeed disposition
#define OPT_EXTENDASC   17 // EXTEND-ASCII extended ascii
#define OPT_LOGOUT      18 // LOGOUT
#define OPT_BM          19 // BM byte macro
#define OPT_DET         20 // DET data entry terminal
#define OPT_SUPDUP      21 // SUPDUP display protocol
#define OPT_SUPDUPOUT   22 // SUPDUP-OUTPUT
#define OPT_SENDLOC     23 // SEND-LOCATION
#define OPT_TERMTYPE    24 // TERMINAL-TYPE
#define OPT_EOR         25 // END-OF-RECORD
#define OPT_TUID        26 // TUID tacacs user id
#define OPT_OUTMRK      27 // OUTMRK output marking
#define OPT_TTYLOC      28 // TTYLOC terminal location number
#define OPT_3270REGIME  29 // 3270-REGIME telnet 3270 regime
#define OPT_X3PAD       30 // X.3-PAD
#define OPT_NAWS        31 // NAWS negotiation about window size
#define OPT_TERMSPEED   32 // TERMINAL-SPEED
#define OPT_REMFLOWCTL  33 // TOGGLE-FLOW-CONTROL
#define OPT_LINEMODE    34 // LINEMODE
#define OPT_XDISPLOC    35 // X-DISPLAY-LOCATION (XDISPLOC)
#define OPT_ENVIRON     36 // ENVIRON telnet environment
#define OPT_AUTHEN      37 // AUTHENTICATION
#define OPT_ENCRYPT     38 // ENCRYPT encryption option
#define OPT_NEWENVIRON  39 // NEW-ENVIRON telnet new environment
#define OPT_TN3270E     40 // TN3270E
#define OPT_XAUTH       41 // *XAUTH
#define OPT_CHARSET     42 // CHARSET
#define OPT_RSP         43 // *telnet remote serial port (RSP)
#define OPT_COMMPORT    44 // COM-PORT-OPTION comm port control option
#define OPT_SUPPECHO    45 // *telnet suppress local echo
#define OPT_STARTTLS    46 // *telnet start TLP
#define OPT_KERMIT      47 // KERMIT
#define OPT_SENDURL     48 // *SEND-URL
#define OPT_FORWARDX    49 // *FORWARD-X
// 50-137 Unassigned    
#define OPT_EXOPL       255// EXTENDED-OPTIONS-LIST (EXOPL) extended opt list

/* Public constants: typed, debugger-visible constants (prefer static const) */

/* Public typedefs: type aliases and opaque handles   */

/* Public enums */

/* Public structs: concrete data layouts used by this module */

/* Public variables */
extern const char       telnetID[];   // FLASHMEM?  
extern const char       telnetAYT[];  // FLASHMEM?

extern boolean          telnetConnected;
extern boolean          offlineMode;

extern EthernetClient   client;

/* Public function prototypes */

void    telnetInit(void);
uint8_t telnetInput(EthernetClient client, char *cmdLine, uint8_t len);
void    telnetDisconnect(void);

#endif /* SESTELNETSERVER_H */

/*** end of file ***/
