# Sensor Node Codes

The folder contains code for both *Sensor Nodes* as well as *Gateway Nodes*. 

1. _I2C_sensor_modular_dev_ consists of codes for all the sensors that we use in lab. Each sensor has its own hex file in which APIs are provided to initialize the sensor, collect the data and finally send the data over nRF24L041+. Here the frequency of sensing is constant and can be set only once during the initialization of the program.
2. _I2C_sensor_modular_var_freq_ has similar functions as above but the frequency of sensing and sending data can be varied depending the parameter that is getting monitored.
3. _nodemcu_nrf_reveiver_gateway_ contains the code of the gateway side. The code basically recieves data from different sensors over nRF and then transmits the same data over MQTT. 

* Note: 'radio.h' file contains all the details about communication. It contains nRF addresses for each room, sensor node ID and initial setup for RF communication.