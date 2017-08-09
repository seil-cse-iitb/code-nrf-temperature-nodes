#include "Arduino.h"
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include "LowPower.h"

#include <SPI.h> //Call SPI library so you can communicate with the nRF24L01+
#include <nRF24L01.h> //nRF2401 libarary found at https://github.com/tmrh20/RF24/
#include <RF24.h> //nRF2401 libarary found at https://github.com/tmrh20/RF24/


#include <string.h>
//#include <malloc.h>
#include <dht.h> // src: https://github.com/RobTillaart/Arduino/tree/master/libraries/DHTstable

// src: http://jeelabs.org/2012/05/04/measuring-vcc-via-the-bandgap/
// https://github.com/jcw/jeelib/blob/master/examples/Ports/bandgap/bandgap.ino

//Constants
#define DHT22_PIN_1 4     // dht sensor1
// #define DHT22_PIN_2 8     // dht sensor2

#define SLEEP_DELAY_IN_SECONDS  60  // value in secs

char node_id_String[] = "1";  //node id

unsigned long log_count = 0L; //sequence number of data entries //resets whenever node is restarted
char log_countString[10];

char temperatureString1[6];
char humidityString1[6];
// char temperatureString2[6];
// char humidityString2[6];

char vccString[6];
char result[50];

int count_freq = 0;
int curr_freq = 1; // freq = 1min at start
float prev = 0.0;
float cur = 1.0;
int k = 0;
unsigned long prev_send = millis();
//constant values
const int change_freq_value = 5;
int sleep_time;
float elapsed_time = 0.0;
unsigned long elapsed_time_sensor_dead_check = 15;


const int pinCE = 7; //This pin is used to set the nRF24 to standby (0) or active mode (1)
const int pinCSN = 8; //This pin is used to tell the nRF24 whether the SPI communication is a command or message to send out
int failCount = 0; //to keep track of nRF send tries

RF24 radio(pinCE, pinCSN); // Create your nRF24 object or wireless SPI connection


//const uint64_t wAddress = 0x00001E5000LL;  // lab            // Radio pipe addresses for the 2 nodes to communicate.
const uint64_t wAddress = 0x00001F5000LL;   //classroom
//const uint64_t rAddress = 0x00001E50B2LL;

void setup()
{

  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  //  delay(3000); // wait for console opening
  init_global(); //initialise any important variables

  Serial.println(F("Node parameters:"));

  Serial.print(F("Node ID: "));
  Serial.println(node_id_String);

  Serial.print(F("Sleep delay in secs is: "));
  Serial.println(SLEEP_DELAY_IN_SECONDS);

//  Serial.print("nrf receiver addr = ");
//  Serial.print(0x00001F5000LL); //error - serial can't print uint64 - will look at it later

  Serial.println(F("********************"));
}


void init_global() {
  //initialise any important variables
  radio.begin();
  radio.setAutoAck(1);
  radio.setRetries(15, 15);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_MIN);
  radio.setChannel(99);
  radio.openWritingPipe(wAddress);
  radio.stopListening();
}

void radioWrite()
{
  while (!radio.write(&result, sizeof(result)))
  {
    if (failCount >= 20)
    {
      failCount = 0;
      Serial.println("max retires attempted, breaking out");
      break;
    }
    Serial.println("fail");
    failCount++;
    delay(20);
  }
}


///*
long readVcc() {

  float internalREF = 1.1; // ideal
  //  float calibration_factor = 1.29 * 0.82;// calibration_factor = Vcc1 (given by voltmeter) / Vcc2 (given by readVcc() function)
  //  float calibration_factor = 1.19;
  float calibration_factor = 1.041;
  internalREF = internalREF * calibration_factor;
  long scale_constant = internalREF * 1023 * 1000;

  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
  ADMUX = _BV(MUX5) | _BV(MUX0);
#elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
  ADMUX = _BV(MUX3) | _BV(MUX2);
#else
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif

  delay(20); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA, ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both

  long result = (high << 8) | low;

  long myVcc = scale_constant / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return myVcc; // Vcc in millivolts
}
//*/



void _read_dht_data(dht& _dht, int _pin)
{
  struct
  {
    uint32_t total;
    uint32_t ok;
    uint32_t crc_error;
    uint32_t time_out;
    uint32_t connect;
    uint32_t ack_l;
    uint32_t ack_h;
    uint32_t unknown;
  } stat = { 0, 0, 0, 0, 0, 0, 0, 0}; //for debugging dht errors

  // READ DATA
  Serial.print("DHT22, \t");

  pinMode(_pin, INPUT_PULLUP);

  uint32_t start = micros();
  int chk = _dht.read22(_pin);
  uint32_t stop = micros();

  //pinMode(_pin, INPUT); //test power consumption by commenting this line

  stat.total++;
  switch (chk)
  {
    case DHTLIB_OK:
      stat.ok++;
      Serial.print("OK,\t");
      break;
    case DHTLIB_ERROR_CHECKSUM:
      stat.crc_error++;
      Serial.print("Checksum error,\t");
      break;
    case DHTLIB_ERROR_TIMEOUT:
      stat.time_out++;
      Serial.print("Time out error,\t");
      break;
    default:
      stat.unknown++;
      Serial.print("Unknown error,\t");
      break;
  }
  // DISPLAY DATA
  //    Serial.print(DHT.humidity, 1);
  //  Serial.print(DHT.humidity);
  //  Serial.print(",\t");
  //  //    Serial.print(DHT.temperature, 1);
  //  Serial.print(DHT.temperature);
  //  Serial.print(",\t");
  //  Serial.print(stop - start);
  Serial.println();

  if (stat.total % 20 == 0)
  {
    Serial.println("\nTOT\tOK\tCRC\tTO\tUNK");
    Serial.print(stat.total);
    Serial.print("\t");
    Serial.print(stat.ok);
    Serial.print("\t");
    Serial.print(stat.crc_error);
    Serial.print("\t");
    Serial.print(stat.time_out);
    Serial.print("\t");
    Serial.print(stat.connect);
    Serial.print("\t");
    Serial.print(stat.ack_l);
    Serial.print("\t");
    Serial.print(stat.ack_h);
    Serial.print("\t");
    Serial.print(stat.unknown);
    Serial.println("\n");
  }
}

void get_temp_humidity(dht& local_dht, int pin) {
  _read_dht_data(local_dht, pin); //discard the first reading as DHT22 returns saved
  // values from memory - refer: http://akizukidenshi.com/download/ds/aosong/AM2302.pdf
  //  delay(2000); //wait for 2 secs before taking new reading // may be we can use sleep routine to save power
  //  _read_dht_data(local_dht, pin); //use this reading
}

//void get_temp_humidity(float* temp, float* hum ) { //using pointers to modifiy two values
//  read_dht_data();
//  *temp = DHT.temperature;
//  *hum = DHT.humidity;
//}

void sending(float cur)
{
  Serial.print("Sending data:  ");
  Serial.println(cur);

  radio.powerUp();
  delay(20);
  radioWrite();

  elapsed_time = 0;
  radio.powerDown();
  prev_send = millis();

}

int sensing(float prev, float cur)
{
  int k = 0;
  float loop_time = 0;
  unsigned long curr_time = millis();
  //  Serial.println("Sensing");
  //Serial.print("previous data");
  //Serial.println(abs((data[prev] - data[cur]));
  loop_time = curr_time - prev_send;
  prev_send = curr_time;
  elapsed_time = elapsed_time + (loop_time + (sleep_time * 1000.0)) / 60000.0; //time capturing the last change....used to check if sensor is not dead
  Serial.print("elapsed time -------");
  Serial.println(elapsed_time);
  if (abs((prev - cur)) >= 0.5)
  {
    Serial.println("major change in temperature");
    sending(cur);
    count_freq = 0;
    Serial.println("freq changes to 1min");
    curr_freq = 1; // curr_freq is set to 1 min when temperature is expected to change rapidly
    k = 1;
  }
  else if (curr_freq == 1) {  // when temperature is expected to change rapidly, send at increased freq
    Serial.println(F("Sending as rapid change in temperature expected"));
    sending(cur);
    k = 1;
    count_freq = count_freq + 1;
  }
  else if ((elapsed_time) >= elapsed_time_sensor_dead_check)
  {
    Serial.println("to check if sensor is not dead");
    sending(cur);
    k = 1;
  }
  else
  {
    //count_freq = count_freq + 1;
  }
  if (count_freq >= 5)
  {
    curr_freq = 5;
    Serial.println("freq_change to 5min");
  }
  Serial.flush();
  if (k == 1)
  {
    return 1;
  }
  else
  {
    return 0;
  }

}


void loop()
{

  dht dht1; //obj of class dht
  // dht dht2; //obj of class dht
  //  float temperature = 0.0;
  //  float humidity = 0.0;
  //  get_temp_humidity(&temperature, &humidity); //using pointers to modifiy two values

  //discard the first reading as DHT22 returns saved values from memory
  //refer: http://akizukidenshi.com/download/ds/aosong/AM2302.pdf
  // get_temp_humidity(dht1, DHT22_PIN_1);
  // get_temp_humidity(dht2, DHT22_PIN_2);
  // delay(2000); //wait for 2 secs before taking new reading // may be we can use sleep routine to save power
  get_temp_humidity(dht1, DHT22_PIN_1);
  // get_temp_humidity(dht2, DHT22_PIN_2);

  float temperature1 = dht1.temperature;
  float humidity1 = dht1.humidity;
  // float temperature2 = dht2.temperature;
  // float humidity2 = dht2.humidity;

  // convert temperature to a string with two digits before the comma and 2 digits for precision
  dtostrf(temperature1, 2, 2, temperatureString1);
  dtostrf(humidity1, 2, 2, humidityString1);
  // dtostrf(temperature2, 2, 2, temperatureString2);
  // dtostrf(humidity2, 2, 2, humidityString2);
  //convert long to string using decimal format
  // ltoa(log_count, log_countString, 10); // works
  //  sprintf(log_countString, "%lu", log_count); // %lu => unsigned long //works

  // send temperature to the serial console
  Serial.print("temperature: ");
  Serial.println(temperatureString1);

  // send humidity to the serial console
  Serial.print("humidity: ");
  Serial.println(humidityString1);

  float myVCC = (float)readVcc(); // in millivolts
  myVCC = myVCC / 1000.0; // in Volts
  //convert float to string using decimal format
  dtostrf(myVCC, 2, 2, vccString);

  //prepare the data string to be written to file
  strcpy(result, node_id_String);
  strcat(result, ",");
  // strcat(result, log_countString);
  // strcat(result, ",");
  // strcat(result, now_String.c_str());
  // strcat(result, ",");
  strcat(result, temperatureString1);
  strcat(result, ",");
  strcat(result, humidityString1);
  strcat(result, ",");
  // strcat(result, temperatureString2);
  // strcat(result, ",");
  // strcat(result, humidityString2);
  // strcat(result, ",");
  strcat(result, vccString);

  Serial.println(result);

  cur = temperature1;
  //  k = sensing(prev, cur);//k==1 when there is a major change in temperature
  //  if (k == 1)
  //  {
  //    prev = cur;
  //    //Serial.print(cur);
  //  }
  sending(temperature1); //sending irrespective of change in temperature at fixed freq

  //delay(curr_freq * 2000);
    sleep_time = SLEEP_DELAY_IN_SECONDS;              // freq independent of change in temperature data
  //  sleep_time = curr_freq * SLEEP_DELAY_IN_SECONDS;  // freq dependent on change in temperature data

  // Serial.println("sleep for current frequency");
  // Serial.flush();
  // log_count += 1; //increment the sequence number of data entry by 1
  // delay(100);
  // Serial.print(F("Entering sleep mode"));
  Serial.print(F("Entering sleep mode for secs: "));
  Serial.println(sleep_time);
  // delay(100);
  //power down
  for (int i = 0; i < sleep_time; i++) {
    // Sleep for 1s with ADC module and BOD module off
    LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);
  }
  //  delay(SLEEP_DELAY_IN_SECONDS * 1000); //delay accepts value in msec
  Serial.println(F("Out of sleep mode..."));

}


