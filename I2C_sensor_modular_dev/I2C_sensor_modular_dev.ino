#include <avr/sleep.h>
#include <avr/wdt.h>
#include "radio.h"
#include "ClosedCube_HDC1080.h" // Library for HDC1080 temperature and humidity sensor
#include "ADT7420.h"
#include "battery_monitor.h"
#include "HDC1080.h"
#include "CCS811.h"

// Serial print compiler directive
#define SERIAL_PRINT 1  // Comment this to remove the Serial.print from the code(Do it once the program is finalize)
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

int sensorAddress[] = {72, 64, 90}; // Contains the I2C address of the sensors

// Enumerator to keep the list of the sensors. They are added according to their addresses
enum sensorEnum {
  ADT7420,
  HDC1080,
  CCS811,
  unknown
};
sensorEnum sensor = unknown;

byte error; // Stores the error generated during I2C communication

//If the particular sensor is connected then than sensorID gets the value 1
int sensorID[] = {0, 0, 0};

// Function to write over the nRF24L01+
int radioWrite()
{
  //while (!radio.write(&data, sizeof(data)))
  while (!radio.write(&result, sizeof(result)))
  {
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

// The function puts the microcontroller in the WATHCDOG TIMER SLEEP mode for the particular time
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

// Interrupt service router to get out of the sleep mode
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

  // Scans the given I2C address and if the device is found, the particular bit is made HIGH
  _SER_PRINT("Scanning....");
  for (int i = 0; i < 3 ; i++)
  {
    Wire.beginTransmission(sensorAddress[i]);
    error = Wire.endTransmission();

    if (error == 0)
    {
      sensorID[i] = 1;
    }
  }
  _SER_PRINT("I2C Device found at address ");
  _SER_PRINT(sensor);
  _SER_PRINTLN();

  // Initialization of the sensors
  if (sensorID[1] == 1) hdc_init();
  if (sensorID[2] == 1) ccs_init();
}

void loop()
{
  if (sensorID[0] == 1)
  {
    data.temperatureData = readADT7420();
    data.humidityData = 0.0;
  }
  if (sensorID[1] == 1)
  {
    data.temperatureData = readHDC1080Temp();
    data.humidityData = readHDC1080Humidity();
  }
  if (sensorID[2] == 1)
  {
    int *sensedData;
    sensedData = CCS811Data();
    data.co2Data = sensedData[0];
    data.tvocData = sensedData[1];
  }

  data.battery_voltage = readVcc();

  dtostrf(data.temperatureData, 6, 3, tempString);
  dtostrf(data.humidityData, 5, 2, humidityString);
  dtostrf(data.battery_voltage, 4, 2, vccString);
  if (sensorID[2] == 1)
  {
    itoa(data.co2Data, co2String, 10);
    itoa(data.tvocData, tvocString, 10);
    delay(20);
  }

  strcpy(result, node_id);
  strcat(result, ",");
  strcat(result, tempString);
  strcat(result, ",");
  strcat(result, humidityString);
  strcat(result, ",");
  if (sensorID[2] == 1)
  {
    strcat(result, co2String);
    strcat(result, ",");
    strcat(result, tvocString);
    strcat(result, ",");
  }
  strcat(result, vccString);

  _SER_PRINTLN(result);

  _DELAY(100);

  //radioWrite();

  _DELAY(30);
  _SER_PRINTLN();
  _DELAY(30);
  
  for (int i = 0; i < (PWR_DWN_DELAY_SEC / 2); i++)
  {
    delayWDT(WDTO_2S);
  }
}
