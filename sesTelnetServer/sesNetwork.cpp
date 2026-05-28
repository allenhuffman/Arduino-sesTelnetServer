/** @file sesNetwork.cpp
 *
 * @brief Hardware transport abstraction for Ethernet/WiFi.
 */

 /*---------------------------------------------------------------------------*/
// System headers
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// External module headers
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// This module's header (must be first among project headers)
/*---------------------------------------------------------------------------*/

#include "sesNetwork.h"

/*---------------------------------------------------------------------------*/
// External module headers
/*---------------------------------------------------------------------------*/

#if defined(SES_TRANSPORT_WIFI_S3)
  #include "arduino_secrets.h"
#else // Fallback to Ethernet, which needs SPI.
  #include <SPI.h>
#endif

/*---------------------------------------------------------------------------*/
// Public data definitions
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// Private macros: all #define items, constants and function-like macros
/*---------------------------------------------------------------------------*/

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

#if defined(SES_TRANSPORT_WIFI_S3)
  static void printWifiData(void);
  static void printCurrentNet(void);
  static void printMacAddress(uint8_t mac[]);
#endif

/*---------------------------------------------------------------------------*/
// Public function definitions
/*---------------------------------------------------------------------------*/

#if defined(SES_TRANSPORT_WIFI_S3)

/**
 * @brief Begin WiFi communication. Initializes the WiFi module and connects
 * to the WiFi network with the SSID and password defined in arduino_secrets.h.
 */

void sesNetworkBegin (void)
{
  // Connection code based on:
  // https://github.com/arduino/ArduinoCore-renesas/blob/main/libraries/WiFiS3/examples/ConnectWithWPA/ConnectWithWPA.ino

  ///////please enter your sensitive data in the Secret tab/arduino_secrets.h
  char  ssid[] = SECRET_SSID;        // your network SSID (name)
  char  pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
  int   status = WL_IDLE_STATUS;     // the WiFi radio's status

  // check for the WiFi module:
  if (WiFi.status () == WL_NO_MODULE)
  {
    Serial.println ("Communication with WiFi module failed!");
    // don't continue
    while (true);
    // TODO: Could do a retry.
  }

  String fv = WiFi.firmwareVersion ();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION)
  {
    Serial.println ("Please upgrade the firmware.");
  }

  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED)
  {
    Serial.print ("Attempting to connect to Network named: ");
    Serial.println (ssid);                   // print the network name (SSID);

    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin (ssid, pass);
    // wait 10 seconds for connection:
    delay (10000);
  }

  // you're connected now, so print out the data:
  Serial.println ("Connected to WiFi.");
  printCurrentNet ();
  printWifiData ();
}

/**
 * @brief Return the local IP address of the WiFi module.
 *
 * @return The local IP address of the WiFi module.
 */
IPAddress sesNetworkLocalIP (void)
{
  return WiFi.localIP ();
}

#else // Fallback to Ethernet

/**
 * @brief Begin Ethernet communication. Initializes the Ethernet module
 * with the MAC and IP address.
 */
void sesNetworkBegin (void)
{
  uint8_t tempMac[] = ETHERNET_MAC;
  uint8_t tempIp[]  = ETHERNET_IP;

  Ethernet.begin (tempMac, tempIp);
}


/**
 * @brief Return the local IP address of the Ethernet module.
 *
 * @return The local IP address of the Ethernet module.
 */
IPAddress sesNetworkLocalIP (void)
{
  return Ethernet.localIP ();
}

#endif

/*---------------------------------------------------------------------------*/
// Private function definitions
/*---------------------------------------------------------------------------*/

#if defined(SES_TRANSPORT_WIFI_S3)

// Functions from:
// https://github.com/arduino/ArduinoCore-renesas/blob/main/libraries/WiFiS3/examples/ConnectWithWPA/ConnectWithWPA.ino

/**
 * @brief Print the local IP address and MAC address of the WiFi module.
 */
static void printWifiData (void)
{
  // print your board's IP address:
  IPAddress ip = WiFi.localIP ();
  Serial.print ("IP Address: ");
  Serial.println (ip);

  // print your MAC address:
  uint8_t mac[6];
  WiFi.macAddress (mac);
  Serial.print ("MAC address: ");
  printMacAddress (mac);
}


/**
 * @brief Print the SSID, BSSID, signal strength, and encryption type of the
 * WiFi network
 */
static void printCurrentNet (void)
{
  // print the SSID of the network you're attached to:
  Serial.print ("SSID: ");
  Serial.println (WiFi.SSID ());

  // print the MAC address of the router you're attached to:
  uint8_t bssid[6];
  WiFi.BSSID (bssid);
  Serial.print ("BSSID: ");
  printMacAddress (bssid);

  // print the received signal strength:
  long rssi = WiFi.RSSI ();
  Serial.print ("Signal Strength (RSSI): ");
  Serial.println (rssi);

  // print the encryption type:
  uint8_t encryption = WiFi.encryptionType ();
  Serial.print ("Encryption Type: ");
  Serial.println (encryption, HEX);
  Serial.println ();
}


/**
 * @brief Print a MAC address in the standard format.
 *
 * @param[in] mac  The MAC address to be printed.
 */
static void printMacAddress (uint8_t mac[])
{
  for (int i = 0; i < 6; i++)
  {
    if (i > 0)
    {
      Serial.print (":");
    }

    if (mac[i] < 16)
    {
      Serial.print ("0");
    }
    
    Serial.print (mac[i], HEX);
  }
  Serial.println ();
}

#endif // SES_TRANSPORT_WIFI_S3

/*** end of file ***/
