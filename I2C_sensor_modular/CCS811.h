#include "Adafruit_CCS811.h"

#define CCS811_ADD 0x5A

Adafruit_CCS811 ccs;

void ccs811_init()
{
 ccs.begin(); 
}

int readCCS811Eco2()
{
  if(ccs.available())
  {
    if(!ccs.readData())
    {
      int eCO2 = ccs.geteCO2();
      return eCO2;
    }
  }
}

int readCCS811Tvoc()
{
  if (ccs.available())
  {
    if(!ccs.readData())
    {
      int tvoc = ccs.getTVOC();
      return tvoc;
    }
  }
}
