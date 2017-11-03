#include <avr/sleep.h>
#include <avr/wdt.h>
#include <Wire.h>
#include "radio.h"
#include "ADT7420.h"
#include "battery_monitor.h"
//#include "HDC1080.h"
#include "CCS811.h"

// Serial print compiler directive
#define SERIAL_PRINT 1
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

int sensorAddress[] = {72, 64, 90};

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
  CCS811,
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
  for (int i = 0; i < 3 ; i++)
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

  //if (sensor == HDC1080) hdc_init();
  if (sensor == CCS811) ccs811_init();
}

void loop()
{
  if (sensor == ADT7420)
  {
    data.temperatureData = readADT7420();
    data.humidityData = 0.0;
  }
  /*
    if (sensor == HDC1080)
    {
    data.temperatureData = readHDC1080Temp();
    data.humidityData = readHDC1080Humidity();
    }
  */
  if (sensor == CCS811)
  {
    data.CO2Data = readccsCO2();
    data.TVOCData = readccsTVOC();
  }
  else
  {
    _SER_PRINTLN("UNABLE TO FIND THE SENSOR");
  }
  data.battery_voltage = readVcc();

  //dtostrf(data.temperatureData, 6, 3, tempString);
  //dtostrf(data.humidityData, 5, 2, humidityString);
  //dtostrf(data.battery_voltage, 4, 2, vccString);
  //  dtostrf(data.CO2Data, 7, 2, co2String);
  //  dtostrf(data.TVOCData, 7, 2, tvocString);
  delay(10);
  itoa(data.CO2Data, co2String, 10);
  itoa(data.TVOCData, tvocString, 10);

  delay(20);

  strcpy(result, node_id);
  strcat(result, ",");
  strcat(result, tempString);
  strcat(result, ",");
  strcat(result, humidityString);
  strcat(result, ",");
  strcat(result, vccString);
  strcat(result, ",");
  strcat(result, co2String);
  strcat(result, ",");
  strcat(result, tvocString);

  //  _SER_PRINT(data.node_id);
  //  _SER_PRINT(",");
  //  _SER_PRINT(data.temperatureData);
  //  _SER_PRINT(",");
  //  _SER_PRINT(data.humidityData);
  //  _SER_PRINT(",");
  //  _SER_PRINTLN(data.battery_voltage);
  _SER_PRINTLN(result);

  _DELAY(100);

  radioWrite();

  _DELAY(30);
  _SER_PRINTLN();
  _DELAY(30);
  for (int i = 0; i < (PWR_DWN_DELAY_SEC / 2); i++)
  {
    delayWDT(WDTO_2S);
  }
}

