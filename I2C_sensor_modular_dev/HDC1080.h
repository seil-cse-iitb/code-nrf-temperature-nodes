#include "ClosedCube_HDC1080.h"

#define HDC1080ADD 0x40

ClosedCube_HDC1080 hdc;

void hdc_init()
{
  hdc.begin(HDC1080ADD);

  hdc.setResolution(HDC1080_RESOLUTION_14BIT, HDC1080_RESOLUTION_14BIT); // Setting Resolution for the Humidity and Temperature nodes.
}

float readHDC1080Temp()
{
  float temp;
  temp = hdc.readTemperature();
  return temp;
}

float readHDC1080Humidity()
{
  float humid;
  humid = hdc.readHumidity();
  return humid;
}


