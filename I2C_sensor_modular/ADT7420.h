

#define ADT7420Address 0x48
#define ADT7420TempReg 0x00
#define ADT7420ConfigReg 0x03

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
  { 
    tempReading = tempReading - 65535;
    temp = (tempReading / 128.0) * -1;
  }
  else
  {
    temp = (tempReading / 128.0);
  }

  return temp;
}

