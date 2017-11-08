#include "Adafruit_CCS811.h"

Adafruit_CCS811 ccs;

char co2String[6];
char tvocString[6];

int sensorData[2];

int* CCS811Data()
{
  while (ccs.available())
  {
    if (!ccs.readData())
    {
      sensorData[0] = ccs.geteCO2();
      sensorData[1] = ccs.getTVOC();

      return sensorData;
    }
  }
}

/*
  int tvocData()
  {
  while(ccs.available())
  {
    if(!ccs.readData())
    {
      int tvoc = ccs.getTVOC();
      return tvoc;
    }
  }
  }
*/

void ccs_init()
{
  //Serial.println("Initializing CCS811");
  ccs.begin();
}
