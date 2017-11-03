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

