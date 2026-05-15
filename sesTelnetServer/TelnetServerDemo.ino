/*-----------------------------------------------------------------------------
 
 Sub-Etha Software's Arduino Telnet Server DEMO
 By Allen C. Huffman
 www.subethasoftware.com
 
 This is an example of how to use the Sub-Etha Software Telnet Server. To
 configure, edit "sesTelnetServerConfig.h" as appropriate.
 
 2014-03-03 1.00 allenh - Created this demo program.
 
 CHECK BACK FOR UPDATES! Much more still to be done...
 -----------------------------------------------------------------------------*/

void setup()
{
  Serial.begin(9600);
  while(!Serial);

  Serial.println();
  Serial.println(FLASHSTR(telnetID));

  showFreeRam();

  telnetInit();
}

/*---------------------------------------------------------------------------*/

#define INPUT_SIZE 40
void loop()
{
  char buffer[INPUT_SIZE];
  byte count;

  showFreeRam();

  // If we are offline, we will just take local input.
  if (offlineMode)
  {
    Serial.print(F("[Offline]Command: "));
  }
  else if (telnetConnected)
  {
    // Else, we are talking remotely. Echo to remote and local.
    client.print(F("[Telnet]Command: "));
    Serial.print(F("[Telnet]Command: "));
  }

  // Get input from remote (if connected) or local.
  count = telnetInput(client, buffer, INPUT_SIZE);
  if (count==255) // 255=connection lost
  {
    Serial.println(F("[Connection Lost]"));
  }
  else // count is how many bytes of data we read in to buffer.
  {
    Serial.print(count);
    Serial.println(F(" bytes received from client."));
  }

  // If first three characters are "BYE"...
  if (strcmp_P(buffer, PSTR("BYE"))==0)
  {
    // If we are offline currently...
    if (offlineMode)
    {
      // ...leave offline mode.
      Serial.println(F("[Online Mode]"));
      offlineMode = false;
    }
    
    // And, if we are connected, disconnect.
    if (telnetConnected==true) telnetDisconnect();
  }
}

/*---------------------------------------------------------------------------*/

unsigned int freeRam()
{
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

void showFreeRam()
{
  Serial.print(F("Free RAM: "));
  Serial.println(freeRam());
}

/*---------------------------------------------------------------------------*/
// End of TelnetServerDemo

