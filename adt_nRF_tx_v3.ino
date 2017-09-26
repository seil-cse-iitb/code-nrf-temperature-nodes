#include <avr/sleep.h>
#include <avr/wdt.h>
#include <Wire.h>
#include <SPI.h> //Call SPI library so you can communicate with the nRF24L01+
#include <nRF24L01.h> //nRF2401 libarary found at https://github.com/tmrh20/RF24/
#include <RF24.h> //nRF2401 libarary found at https://github.com/tmrh20/RF24/

// Serial print compiler directive
//#define SERIAL_PRINT 1
#ifdef SERIAL_PRINT
  #define _SER_BEGIN(x) Serial.begin(x)
  #define _SER_PRINT(x) Serial.print(x)
  #define _SER_PRINTLN(x) Serial.println(x)
  #define _SER_DELAY(x) delay(x)
#else
  #define _SER_BEGIN(x)
  #define _SER_PRINT(x)
  #define _SER_PRINTLN(x)
  #define _SER_DELAY(x)
#endif

#define ADT7420Address 0x48
#define ADT7420TempReg 0x00
#define ADT7420ConfigReg 0x03

#define PWR_DWN_DELAY_SEC 60     // Delay for sleep mode in seconds( Min delay is 2 Seconds)

char node_id_string[] = "15";
unsigned long log_count = 0;  // Sequence number of data entry
char log_count_string[10];

char temperatureString[4];
char vccString[5];
char result[32];

int count_freq = 0;
int curr_freq = 1;
float prev = 0.0;
float curr = 1.0;
int k = 0;
unsigned long prev_send = millis();
const int change_freq_value = 5;
int sleep_time;
float elapsed_time;
unsigned long elapsed_time_sensor_dead_check = 15;

//  long tempReading = 0;
//  float temp;
//  float internalRef = 1.1;

const int pinCE = 7;
const int pinCSN = 8;
int failCount = 0;
RF24 radio(pinCE, pinCSN);

const uint64_t wAddress = 0x00001E5000LL;  // lab            // Radio pipe addresses for the 2 nodes to communicate.
//const uint64_t wAddress = 0x00001F5000LL;   //classroom

float readADT7420()
{
  float temp = 0;
  long tempReading = 0;

  Wire.beginTransmission(ADT7420Address);
  Wire.write(0x03);
  Wire.write(B10100000); //Set 16bit mode and one-shot mode
  Wire.endTransmission();
  //delay(20); //wait for sensor

  byte MSB;
  byte LSB;
  // Send request for temperature register.
  Wire.beginTransmission(ADT7420Address);
  Wire.write(ADT7420TempReg);
  Wire.endTransmission();
  // Listen for and acquire 16-bit register address.
  Wire.requestFrom(ADT7420Address, 2);
  MSB = Wire.read();
  LSB = Wire.read();
  // Assign global 'tempReading' the 16-bit signed value.
  tempReading = ((MSB << 8) | LSB);
  if (tempReading > 32768)
  { tempReading = tempReading - 65535;
    temp = (tempReading / 128.0) * -1;
  }
  else
  {
    temp = (tempReading / 128.0);
  }

  return temp;
}

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

void initRadio()
{
  radio.begin();
  //radio.setAutoAck(1);
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
      _SER_PRINTLN("max retires attempted, breaking out");
      break;
    }
    _SER_PRINTLN("fail");
    failCount++;
    delay(20);
  }
}

float readVcc()
{
  float internalRef = 1.1;
  analogReference(EXTERNAL);  // Set analog refernce to AVCC

  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);

  unsigned long start = millis();
  while (start + 6 > millis()); // Delay for 3 seconds

  ADCSRA |= _BV(ADSC);  // start ADC
  while (bit_is_set(ADCSRA, ADSC)); // Wait until conversion

  int result = ADCL;
  result |= (ADCH << 8);  // Store the result

  float batVoltage = (internalRef / result) * 1024; // Convert it into battery voltage

  analogReference(INTERNAL);  // Again change the refence to INTERNAL

  return batVoltage;
}

void setup() {
  // put your setup code here, to run once:

  _SER_BEGIN(9600);
  _SER_PRINTLN("**** Sensor Node ****");

  initRadio();

  Wire.begin();

  wdt_disable();
}

void loop() {
  // put your main code here, to run repeatedly:

  // Read Temperature Data
  // Store it, make a proper package and send it over radio.
  // Go to sleep mode until next interval comes.
  float tempValue = readADT7420();
  dtostrf(tempValue, 2, 2, temperatureString);

  float batteryVoltage = readVcc();
  dtostrf(batteryVoltage, 1, 2, vccString);

  strcpy(result, node_id_string);
  strcat(result, ",");
  strcat(result, temperatureString);
  strcat(result, ",");
  strcat(result, "00");
  strcat(result, ",");
  strcat(result, vccString);

  _SER_PRINT("Data: ");
  _SER_PRINTLN(result);
  _SER_PRINTLN("");
  _SER_DELAY(40);

  //radio.powerUp();
  //delay(20);

  radioWrite();
  delay(20);
  //radio.powerDown();
  
  for (int i = 0; i < (PWR_DWN_DELAY_SEC / 2); i++)
  {
    delayWDT(WDTO_2S);
  }
}
