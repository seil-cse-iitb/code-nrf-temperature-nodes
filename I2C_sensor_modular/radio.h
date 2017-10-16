#include <SPI.h> //Call SPI library so you can communicate with the nRF24L01+
#include <nRF24L01.h> //nRF2401 libarary found at https://github.com/tmrh20/RF24/
#include <RF24.h> //nRF2401 libarary found at https://github.com/tmrh20/RF24/

const int pinCE = 7;
const int pinCSN = 8;
RF24 radio(pinCE, pinCSN);

//const uint64_t wAddress = 0x00001E5000LL;  // lab        // Radio pipe addresses for the 2 nodes to communicate.
//const uint64_t wAddress = 0x00001F5000LL;   //classroom
const uint64_t wAddress = 0x00001E6000LL;   // Server room

char node_id[] = "24";
char tempString[6];
char humidityString[6];
char vccString[5];
char result[32];

struct data_to_be_send {
  byte node_id = 1;
  float temperatureData = 0.0;
  float humidityData = 0.0;
  float battery_voltage = 0.0;
} data;

int failCount = 0;

void radio_init()
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

