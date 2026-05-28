/** @file sesNetwork.h
 *
 * @brief Hardware transport abstraction for Ethernet/WiFi.
 */

#ifndef SESNETWORK_H
#define SESNETWORK_H

/*---------------------------------------------------------------------------*/
// System headers
/*---------------------------------------------------------------------------*/

#include <Arduino.h>

/*---------------------------------------------------------------------------*/
// External module headers
/*---------------------------------------------------------------------------*/

#include "sesTelnetServerConfig.h"

#if defined(SES_TRANSPORT_WIFI_S3)
  #include <WiFiS3.h>
  typedef WiFiClient SesClient;
  typedef WiFiServer SesServer;
#else
  #include <Ethernet.h>
  typedef EthernetClient SesClient;
  typedef EthernetServer SesServer;
#endif

/*---------------------------------------------------------------------------*/
// Public function prototypes
/*---------------------------------------------------------------------------*/

void      sesNetworkBegin(void);
IPAddress sesNetworkLocalIP(void);

#endif /* SESNETWORK_H */

/*** end of file ***/
