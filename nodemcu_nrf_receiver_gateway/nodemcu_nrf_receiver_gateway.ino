/*
  Sketch which publishes data from a dht22 sensor to a MQTT topic.

  This sketch goes in deep sleep mode once the temperature has been sent to the MQTT
  topic and wakes up periodically (configure SLEEP_DELAY_IN_SECONDS accordingly).

  Hookup guide:
  - connect D0 pin to RST pin in order to enable the ESP8266 to wake up periodically

*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <string.h>
#include <malloc.h>

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

//add at the beginning of sketch // to check battery voltage using internal adc
// ADC_MODE(ADC_VCC); will result in compile error. use this instead.
//int __get_adc_mode(void) { return (int) (ADC_VCC); }
ADC_MODE(ADC_VCC); //vcc read //TOUT pin has to be disconnected in this mode.

//#include <dht.h> // src: https://github.com/RobTillaart/Arduino/tree/master/libraries/DHTstable

//dht DHT;


const int pinCE = D3;
const int pinCSN = D4;

RF24 radio(pinCE, pinCSN);
const uint64_t wAddress_lab = 0x00001E5000LL;  // lab nrf address           // Radio pipe addresses for the 2 nodes to communicate.
const uint64_t wAddress_SIC305 = 0x00001F5000LL;   //SIC305 nrf address
const uint64_t wAddress_lecture_hall = 0x00001E6000LL; //lecture_hall nrf address

const uint64_t rAddress = wAddress_lab;

bool received = false;
//char temperatureString[6];
//char humidityString[6];
//char vddString[6];
char result[32];

//Constants
//#define DHTPIN D4     // what pin we're connected to
//#define DHTTYPE DHT22   // DHT 22  (AM2302)
//DHT dht(DHTPIN, DHTTYPE); //// Initialize DHT sensor for normal 16mhz Arduino
//#define DHT22_PIN D4
// Static IP details...
IPAddress ip(192, 168, 1, 179);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress DNS(192, 168, 1, 1);

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
} stat = { 0, 0, 0, 0, 0, 0, 0, 0};

//#define SLEEP_DELAY_IN_SECONDS  600
#define SLEEP_DELAY_IN_SECONDS  10  // value in secs
#define ONE_WIRE_BUS            D4      // DS18B20 pin

//const char* ssid = "SEIL_SCC";
const char* ssid = "SEIL";
const char* password = "deadlock123";

float prev_temp = 0.0 ;

const char* mqtt_server = "10.129.149.9";  //production machine
const char* mqtt_username = "<MQTT_BROKER_USERNAME>";
const char* mqtt_password = "<MQTT_BROKER_PASSWORD>";

// const char* mqtt_topic = "data/kresit/dht/SEIL"; //test
//const char* mqtt_topic = "nodemcu/kresit/dht/FCK"; //production
//const char* mqtt_topic = "nodemcu/kresit/dht/sic305"; //classroom
const char* mqtt_topic = "nodemcu/kresit/dht/SEIL"; //LAB
//const char* mqtt_topic = "nodemcu/kresit/dht/lecture_hall"; //lecture_hall

//const char* client_id = "dht_SIC301";
//const char* client_id = "gateway1_lecture_hall";
const char* client_id = "lab_gateway";

//char node_id_String[] = "99";

WiFiClient espClient;
PubSubClient client(espClient);

//OneWire oneWire(ONE_WIRE_BUS);
//DallasTemperature DS18B20(&oneWire);

float vdd = 0.0; //for measuring supply voltage
float calibration_factor = 1.197; //for measuring supply voltage

void setup() {
  // setup serial port
  Serial.begin(115200);
  Serial.println("____Testing NRF with NodeMCU____");
  //  Serial.print("nrf receiver addr = ");
  //  Serial.print(0x00001F5000LL); //error - serial can't print uint64 - will look at it later

  // setup WiFi
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // setup OneWire bus
  //DS18B20.begin();
  //dht.begin();
  Serial.println(mqtt_topic);

  radio.begin();
  radio.setAutoAck(1);
  radio.setRetries(15, 15);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_MIN);
  radio.setChannel(99);
  radio.openReadingPipe(1, rAddress);
  radio.startListening();
}

void radioListen()
{
  radio.startListening();

  while (radio.available())
  {
    radio.read(&result, sizeof(result));
    received = true;
  }
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  //Serial.println(password);
  WiFi.config(ip, gateway, subnet, DNS);
  //  delay(100);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(client_id, mqtt_username, mqtt_password)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

/*float getTemperature() {
  Serial.println("Requesting DS18B20 temperature...");
  float temp;
  Serial.print("Previous temperature is ...") ;
  Serial.println(prev_temp);
  do {
    DS18B20.requestTemperatures();
    temp = DS18B20.getTempCByIndex(0);
    delay(100);
  } while (temp == 85.0 || temp == (-127.0));
  prev_temp = temp ;

  return temp;
  }*/



/*
  void read_dht_data()
  {
  // READ DATA
  Serial.print("DHT22, \t");

  uint32_t start = micros();
  int chk = DHT.read22(DHT22_PIN);
  uint32_t stop = micros();

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
  Serial.print(DHT.humidity);
  Serial.print(",\t");
  //    Serial.print(DHT.temperature, 1);
  Serial.print(DHT.temperature);
  Serial.print(",\t");
  Serial.print(stop - start);
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

  float measure_vdd()
  {

  float vdd = calibration_factor * ESP.getVcc() / 1000.0;
  //Serial.println(vdd);
  return vdd;
  }
*/

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (received)
  {
    Serial.println(result);
    client.publish(mqtt_topic, result);
    received = false;
  }

  radioListen();
}

