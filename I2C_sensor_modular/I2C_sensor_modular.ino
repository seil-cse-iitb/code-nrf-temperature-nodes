#include <avr/sleep.h>
#include <avr/wdt.h>
#include "radio.h"
#include "ClosedCube_HDC1080.h" // Library for HDC1080 temperature and humidity sensor
#include "ADT7420.h"
#include "battery_monitor.h"
#include "HDC1080.h"

// Serial print compiler directive
//#define SERIAL_PRINT 1
#ifdef SERIAL_PRINT
#define _SER_BEGIN(x) Serial.begin(x)
#define _SER_PRINT(x) Serial.print(x)
#define _SER_PRINTLN(x) Serial.println(x)
#define _DELAY(x) delay(x)
#else
#define _SER_BEGIN(x)
#define _SER_PRINT(x)
#define _SER_PRINTLN(x)
#define _DELAY(x)
#endif

#define PWR_DWN_DELAY_SEC 60 // Delay for sleep mode in seconds( Min delay is 2 Seconds)

int sensorAddress[] = {72, 64};

int radioWrite()
{
  //Serial.println("Inside radio write");
  //while (!radio.write(&data, sizeof(data)))
  while (!radio.write(&result, sizeof(result)))
  {
    //Serial.println("Inside while");
    if (failCount >= 20)
    {
      failCount = 0;
      _SER_PRINTLN("max retires attempted, breaking out");
      break;
    }
    _SER_PRINTLN("fail");
    failCount++;
    delay(20);
  }
}

enum sensorEnum {
  ADT7420,
  HDC1080,
  unknown
};

sensorEnum sensor = unknown;

byte error;

void delayWDT(byte timer)
{
  sleep_enable();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  ADCSRA &= ~(1 << ADEN);
  WDTCSR |= 0b00011000;
  WDTCSR = 0b01000000 | timer;

  wdt_reset();
  sleep_cpu();
  sleep_disable();
  ADCSRA |= (1 << ADEN);
}

ISR(WDT_vect)
{
  wdt_disable();
  MCUSR = 0;
}

void setup()
{
  _SER_BEGIN(115200);
  _SER_PRINT("____I2C SENSOR NODES____");

  Wire.begin();

  radio_init();

  wdt_disable();

  _SER_PRINT("Scanning....");
  for (int i = 0; i < 2 ; i++)
  {
    Wire.beginTransmission(sensorAddress[i]);
    error = Wire.endTransmission();

    if (error == 0)
    {
      sensor = i;
    }
  }
  _SER_PRINT("I2C Device found at address ");
  _SER_PRINT(sensor);
  _SER_PRINTLN();

  if (sensor == HDC1080) hdc_init();
}

void loop()
{
  if (sensor == ADT7420)
  {
    data.temperatureData = readADT7420();
    data.humidityData = 0.0;
  }
  else if (sensor == HDC1080)
  {
    data.temperatureData = readHDC1080Temp();
    data.humidityData = readHDC1080Humidity();
  }
  else
  {
    _SER_PRINTLN("UNABLE TO FIND THE SENSOR");
  }
  data.battery_voltage = readVcc();

  dtostrf(data.temperatureData, 6, 3, tempString);
  dtostrf(data.humidityData, 5, 2, humidityString);
  dtostrf(data.battery_voltage, 4, 2, vccString);
  delay(20);

  strcpy(result, node_id);
  strcat(result, ",");
  strcat(result, tempString);
  strcat(result, ",");
  strcat(result, humidityString);
  strcat(result, ",");
  strcat(result, vccString);

  //  _SER_PRINT(data.node_id);
  //  _SER_PRINT(",");
  //  _SER_PRINT(data.temperatureData);
  //  _SER_PRINT(",");
  //  _SER_PRINT(data.humidityData);
  //  _SER_PRINT(",");
  //  _SER_PRINTLN(data.battery_voltage);
  _SER_PRINTLN(result);

  _DELAY(100);

  if (data.battery_voltage > 2.7) radioWrite();

  _DELAY(30);
  _SER_PRINTLN();
  _DELAY(30);
  for (int i = 0; i < (PWR_DWN_DELAY_SEC / 2); i++)
  {
    delayWDT(WDTO_2S);
  }
}
