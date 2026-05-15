#ifndef SESTELNETSERVER_H
#define SESTELNETSERVER_H

#include <stdint.h>

#include <Ethernet.h> 

extern const char telnetID[];   // FLASHMEM?  
extern const char telnetAYT[];  // FLASHMEM?

extern boolean  telnetConnected;
extern boolean  offlineMode;

extern EthernetClient client;

/* Prototypes */
void    telnetInit(void);
uint8_t telnetInput(EthernetClient client, char *cmdLine, uint8_t len);
void    telnetDisconnect (void);
void    telnetPrintCmd (uint8_t type);
void    telnetPrintHex (uint8_t val);
void    telnetPrintOpt(uint8_t opt);
bool    telnetHandleDo (uint8_t opt);
bool    telnetHandleDont(uint8_t opt);
bool    telnetHandleWill(uint8_t opt);
bool    telnetHandleWont(uint8_t opt);
bool    telnetHandleOptEnable (uint8_t opt);
bool    telnetHandleOptDisable (uint8_t opt);

bool telnetModeEnable (uint8_t mode);
bool telnetModeDisable (uint8_t mode);



#endif /* SESTELNETSERVER_H */
