/*
  This is a library written for the TI ADS122C04
  24-Bit 4-Channel 2-kSPS Delta-Sigma ADC With I2C Interface

  It allows you to measure temperature very accurately using a
  Platinum Resistance Thermometer

  SparkFun sells these at its website: www.sparkfun.com
  Do you like this library? Help support SparkFun. Buy a board!

  Written by: Paul Clark (PaulZC)
  Date: May 4th 2020

  Based on the TI datasheet:
  https://www.ti.com/product/ADS122C04
  https://www.ti.com/lit/ds/symlink/ads122c04.pdf
  Using the example code from the "High Precision Temperature Measurement
  for Heat and Cold Meters Reference Design" (TIDA-01526) for reference:
  http://www.ti.com/tool/TIDA-01526
  http://www.ti.com/lit/zip/tidcee5

  The MIT License (MIT)
  Copyright (c) 2020 SparkFun Electronics
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
  associated documentation files (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to
  do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial
  portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
  NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "ADS122U04_ADC_Arduino_Library.h"

SFE_ADS122C04::SFE_ADS122C04(void)
{
  // Constructor
}

//Attempt communication with the device and initialise it
//Return true if successful
bool SFE_ADS122C04::begin(uint8_t deviceAddress, HardwareSerial &serialPort)
{
  Serial.println("Entering function: " + String(__func__));
  _deviceAddress = deviceAddress; //If provided, store the I2C address from user
  _serialPort = &serialPort; // Grab which serial port the user wants us to use
  _wireMode = ADS122C04_RAW_MODE; //Default to using 'safe' settings (disable the IDAC current sources)

  delay(1); // wait for power-on reset to complete (datasheet says we should do this)

  if (isConnected() == false)
  {
    if (_printDebug == true)
    {
      _debugPort->println(F("begin: isConnected returned false"));
    }
    return (false);
  }

  reset(); // reset the ADS122C04 (datasheet says we should do this)
  delay(1);

  return(configureADCmode(ADS122C04_RAW_MODE)); // Default to using 'safe' settings (disable the IDAC current sources)
}

// Configure the chip for the selected wire mode
//Disable the burnout current source
//Use Internal Source
//No need for the IDAC1,2
// Short the Inputs and average over a range
//Consequently Subtract the averaged reading from the Result on the Analog Channels to Reduce the Noise Subsequently
// Data Counter Register to Check if the data value is new or not

bool SFE_ADS122C04::configureADCmode(uint8_t wire_mode, uint8_t rate)
{
  ADS122C04_initParam initParams; // Storage for the chip parameters

 if (wire_mode == ADS122C04_RAW_MODE) // Raw mode : disable the IDAC and use the internal reference
  {
    initParams.inputMux = ADS122C04_MUX_AIN0_AIN1; // Route AIN1 to AINP and AIN0 to AINN
    initParams.gainLevel = ADS122C04_GAIN_128; // Set the gain to 1
    initParams.pgaBypass = ADS122C04_PGA_ENABLED;
    initParams.dataRate = rate; // Set the data rate (samples per second). Defaults to 20
    initParams.opMode = ADS122C04_OP_MODE_TURBO; // Disable turbo mode
    initParams.convMode = ADS122C04_CONVERSION_MODE_CONTINUOUS; // Use single shot mode
    initParams.selectVref = ADS122C04_VREF_AVDD1; // Use the internal 2.048V reference todo
    initParams.tempSensorEn = ADS122C04_TEMP_SENSOR_OFF; // Disable the temperature sensor
    initParams.dataReadyEn = 0b0;
    initParams.dataCounterEn = ADS122C04_DCNT_DISABLE; // Disable the data counter
    initParams.dataCRCen = ADS122C04_CRC_DISABLED; // Disable CRC checking
    initParams.burnOutEn = ADS122C04_BURN_OUT_CURRENT_OFF; // Disable the burn-out current
    initParams.idacCurrent = ADS122C04_IDAC_CURRENT_OFF; // Disable the IDAC current
    initParams.routeIDAC1 = ADS122C04_IDAC1_DISABLED; // Disable IDAC1
    initParams.routeIDAC2 = ADS122C04_IDAC2_DISABLED; // Disable IDAC2
    initParams.outputMODE = 0b0; 
    initParams.gpio2DIR = 0b1; 
    initParams.gpio1DIR = 0b0; 
    initParams.gpio0DIR = 0b0; 
    initParams.gpio2SEL = 0b1; 
    initParams.gpio2DAT = 0b0; 
    initParams.gpio1DAT = 0b0; 
    initParams.gpio0DAT = 0b0; 
    _wireMode = ADS122C04_RAW_MODE; // Update the wire mode
  }
  else
  {
    if (_printDebug == true)
    {
      _debugPort->println(F("configureADCmode: unknown mode"));
    }
    return(false);
  }
  return(ADS122C04_init(&initParams)); // Configure the chip
}

//Returns true if device answers on _deviceAddress
bool SFE_ADS122C04::isConnected(void)
{
  //todo have to implement logic according to uart
    return 1;
}

//Enable or disable the printing of debug messages
void SFE_ADS122C04::enableDebugging(Stream &debugPort)
{
  _debugPort = &debugPort; //Grab which port the user wants us to use for debugging
  _printDebug = true; //Should we print the commands we send? Good for debugging
}

void SFE_ADS122C04::disableDebugging(void)
{
  _printDebug = false; //Turn off extra print statements
}

//Safely print messages
void SFE_ADS122C04::debugPrint(char *message)
{
  if (_printDebug == true)
  {
    _debugPort->print(message);
  }
}

//Safely print messages
void SFE_ADS122C04::debugPrintln(char *message)
{
  if (_printDebug == true)
  {
    _debugPort->println(message);
  }
}

uint32_t SFE_ADS122C04::getRawValue(){
  unsigned long start_time = millis(); // Record the start time so we can timeout
  bool drdy = false; // DRDY (1 == new data is ready)
  float ret_val = 0.0; // Return value
  float RTD, POLY; // Variables needed to convert RTD to Centigrade

  uint32_t currentWeight;

  // Read the conversion result
  if(ADS122C04_getConversionData(&currentWeight) == false)
  {
      _debugPort->println(F("readPT100Centigrade: ADS122C04_getConversionData failed"));
    return(ret_val);
  }
  else {
      return currentWeight;
  }
}

float SFE_ADS122C04::readPT100Centigrade(void) // Read the temperature in Centigrade
{
  raw_voltage_union raw_v; // union to convert uint32_t to int32_t
  unsigned long start_time = millis(); // Record the start time so we can timeout
  bool drdy = false; // DRDY (1 == new data is ready)
  float ret_val = 0.0; // Return value
  float RTD, POLY; // Variables needed to convert RTD to Centigrade

  // Start the conversion (assumes we are using single shot mode)
  // start();

  // Wait for DRDY to go valid
  while((drdy == false) && (millis() < (start_time + ADS122C04_CONVERSION_TIMEOUT)))
  {
    delay(1); // Don't pound the bus too hard
    // drdy = checkDataReady();
  }

  // Check if we timed out  qs
  if (drdy == false)
  {
    if (_printDebug == true)
    {
      _debugPort->println(F("readPT100Centigrade: checkDataReady timed out"));
    }
    // return(ret_val);
  }

  // Read the conversion result
  if(ADS122C04_getConversionData(&raw_v.UINT32) == false)
  {
    if (_printDebug == true)
    {
      _debugPort->println(F("readPT100Centigrade: ADS122C04_getConversionData failed"));
    }
    return(ret_val);
  }

  return 1;
  // The raw voltage is in the bottom 24 bits of raw_temp
  // If we just do a <<8 we will multiply the result by 256
  // Instead pad out the MSB with the MS bit of the 24 bits
  // to preserve the two's complement
  if ((raw_v.UINT32 & 0x00800000) == 0x00800000)
  {
    raw_v.UINT32 |= 0xFF000000;
  }

  // raw_v.UINT32 now contains the ADC result, correctly signed
  // Now we need to convert it to temperature using the PT100 resistance,
  // the gain, excitation current and reference resistor value

  // Formulae are taken from:
  // http://www.ti.com/lit/an/sbaa275/sbaa275.pdf
  // https://www.analog.com/media/en/technical-documentation/application-notes/AN709_0.pdf

  // 2^23 is 8388608
  RTD = ((float)raw_v.INT32) / 8388608.0; // Load RTD with the scaled ADC value
  RTD *= PT100_REFERENCE_RESISTOR; // Multiply by the reference resistor
  // Use the correct gain for high and low temperatures
  if ((_wireMode == ADS122C04_4WIRE_HI_TEMP) || (_wireMode == ADS122C04_3WIRE_HI_TEMP) || (_wireMode == ADS122C04_2WIRE_HI_TEMP))
  {
    RTD /= PT100_AMP_GAIN_HI_TEMP; // Divide by the amplifier gain for high temperatures
  }
  else
  {
    RTD /= PT100_AMPLIFIER_GAIN; // Divide by the amplifier gain for low temperatures
  }
  if ((_wireMode == ADS122C04_3WIRE_MODE) || (_wireMode == ADS122C04_3WIRE_HI_TEMP)) // If we are using 3-wire mode
  {
    RTD *= 2.0; // 3-wire mode needs a factor of 2
  }

  // RTD now contains the PT100 resistance in Ohms
  // Now we need to convert this to temperature

  // Calculate the temperature
  ret_val = RTD * -23.10e-9;
  ret_val += 17.5848089e-6;
  ret_val = sqrt(ret_val);
  ret_val -= 3.9083e-3;
  ret_val /= -1.155e-6;

  //  Check if the temperature is positive, return if it is
  if (ret_val >= 0.0)
    return(ret_val);

  // The temperature is negative so we need to use a different formula
  ret_val = -242.02;
  ret_val += 2.2228 * RTD;
  POLY = RTD * RTD; // Load the polynomial with RTD^2
  ret_val += 2.5859e-3 * POLY;
  POLY *= RTD; // Load the polynomial with RTD^3
  ret_val -= 4.8260e-6 * POLY;
  POLY *= RTD; // Load the polynomial with RTD^4
  ret_val -= 2.8183e-8 * POLY;
  POLY *= RTD; // Load the polynomial with RTD^5
  ret_val += 1.5243e-10 * POLY;

  return(ret_val);
}

float SFE_ADS122C04::readPT100Fahrenheit(void) // Read the temperature in Fahrenheit
{
  return((readPT100Centigrade() * 1.8) + 32.0); // Read Centigrade and convert to Fahrenheit
}

// Read the raw signed 24-bit ADC value as int32_t
// The result needs to be multiplied by VREF / GAIN to convert to Volts
int32_t SFE_ADS122C04::readRawVoltage(uint8_t rate)
{
  raw_voltage_union raw_v; // union to convert uint32_t to int32_t
  unsigned long start_time = millis(); // Record the start time so we can timeout
  bool drdy = false; // DRDY (1 == new data is ready)
  uint8_t previousWireMode = _wireMode; // Record the previous wire mode so we can restore it
  uint8_t previousRate = ADS122C04_Reg.reg1.bit.DR; // Record the previous rate so we can restore it
  bool configChanged = (_wireMode != ADS122C04_RAW_MODE) || (previousRate != rate); // Only change the configuration if we need to

  // Configure the ADS122C04 for raw mode
  // Disable the IDAC, use the internal 2.048V reference and set the gain to 1
  if (configChanged)
  {
    if ((configureADCmode(ADS122C04_RAW_MODE, rate)) == false)
    {
      if (_printDebug == true)
      {
        _debugPort->println(F("readRawVoltage: configureADCmode (1) failed"));
      }
      return(0);
    }
  }

  // Start the conversion (assumes we are using single shot mode)
  start();

  // Wait for DRDY to go valid
  while((drdy == false) && (millis() < (start_time + ADS122C04_CONVERSION_TIMEOUT)))
  {
    delay(1); // Don't pound the bus too hard
    drdy = checkDataReady();
  }

  // Check if we timed out
  if (drdy == false)
  {
    if (_printDebug == true)
    {
      _debugPort->println(F("readRawVoltage: checkDataReady timed out"));
    }
    if (configChanged)
      configureADCmode(previousWireMode, previousRate); // Attempt to restore the previous wire mode
    return(0);
  }

  // Read the conversion result
  if(ADS122C04_getConversionData(&raw_v.UINT32) == false)
  {
    if (_printDebug == true)
    {
      _debugPort->println(F("readRawVoltage: ADS122C04_getConversionData failed"));
    }
    if (configChanged)
      configureADCmode(previousWireMode, previousRate); // Attempt to restore the previous wire mode
    return(0);
  }

  // Restore the previous wire mode
  if (configChanged)
  {
    if ((configureADCmode(previousWireMode, previousRate)) == false)
    {
    if (_printDebug == true)
      {
        _debugPort->println(F("readRawVoltage: configureADCmode (2) failed"));
      }
      return(0);
    }
  }

  // The raw voltage is in the bottom 24 bits of raw_temp
  // If we just do a <<8 we will multiply the result by 256
  // Instead pad out the MSB with the MS bit of the 24 bits
  // to preserve the two's complement
  if ((raw_v.UINT32 & 0x00800000) == 0x00800000)
    raw_v.UINT32 |= 0xFF000000;
  return(raw_v.INT32);
}

// Read the raw signed 24-bit ADC value as uint32_t
// The ADC data is returned in the least-significant 24-bits
// Higher functions will need to convert the result to (e.g.) int32_t
uint32_t SFE_ADS122C04::readADC(void)
{
  uint32_t ret_val; // The return value

  // Read the conversion result
  if(ADS122C04_getConversionData(&ret_val) == false)
  {
    if (_printDebug == true)
    {
      _debugPort->println(F("readADC: ADS122C04_getConversionData failed"));
    }
    return(0);
  }

  return(ret_val);
}

// Read the internal temperature
float SFE_ADS122C04::readInternalTemperature(uint8_t rate)
{
  internal_temperature_union int_temp; // union to convert uint16_t to int16_t
  uint32_t raw_temp; // The raw temperature from the ADC
  unsigned long start_time = millis(); // Record the start time so we can timeout
  bool drdy = false; // DRDY (1 == new data is ready)
  float ret_val = 0.0; // The return value
  uint8_t previousWireMode = _wireMode; // Record the previous wire mode so we can restore it
  uint8_t previousRate = ADS122C04_Reg.reg1.bit.DR; // Record the previous rate so we can restore it
  bool configChanged = (_wireMode != ADS122C04_TEMPERATURE_MODE) || (previousRate != rate); // Only change the configuration if we need to

  // Enable the internal temperature sensor
  // Reading the ADC value will return the temperature
  if (configChanged)
  {
    if ((configureADCmode(ADS122C04_TEMPERATURE_MODE, rate)) == false)
    {
      if (_printDebug == true)
      {
        _debugPort->println(F("readInternalTemperature: configureADCmode (1) failed"));
      }
      return(ret_val);
    }
  }

  // Start the conversion
  start();

  // Wait for DRDY to go valid
  while((drdy == false) && (millis() < (start_time + ADS122C04_CONVERSION_TIMEOUT)))
  {
    delay(1); // Don't pound the bus too hard
    drdy = checkDataReady();
  }

  // Check if we timed out
  if (drdy == false)
  {
    if (_printDebug == true)
    {
      _debugPort->println(F("readInternalTemperature: checkDataReady timed out"));
    }
    if (configChanged)
      configureADCmode(previousWireMode, previousRate); // Attempt to restore the previous wire mode
    return(ret_val);
  }

  // Read the conversion result
  if(ADS122C04_getConversionData(&raw_temp) == false)
  {
    if (_printDebug == true)
    {
      _debugPort->println(F("readInternalTemperature: ADS122C04_getConversionData failed"));
    }
    if (configChanged)
      configureADCmode(previousWireMode, previousRate); // Attempt to restore the previous wire mode
    return(ret_val);
  }

  // Restore the previous wire mode
  if (configChanged)
  {
    if ((configureADCmode(previousWireMode, previousRate)) == false)
    {
      if (_printDebug == true)
      {
        _debugPort->println(F("readInternalTemperature: configureADCmode (2) failed"));
      }
      return(ret_val);
    }
  }

  if (_printDebug == true)
  {
    _debugPort->print(F("readInternalTemperature: raw_temp (32-bit) = 0x"));
    _debugPort->println(raw_temp, HEX);
  }

  // The temperature is in the top 14 bits of the bottom 24 bits of raw_temp
  int_temp.UINT16 = (uint16_t)(raw_temp >> 10); // Extract the 14-bit value

  // The signed temperature is now in the bottom 14 bits of int_temp.UINT16
  // If we just do a <<2 we will multiply the result by 4
  // Instead we will pad out the two MS bits with the MS bit of the 14 bits
  // to preserve the two's complement
  if ((int_temp.UINT16 & 0x2000) == 0x2000) // Check if the MS bit is 1
  {
    int_temp.UINT16 |= 0xC000; // Value is negative so pad with 1's
  }
  else
  {
    int_temp.UINT16 &= 0x3FFF;  // Value is positive so make sure the two MS bits are 0
  }

  ret_val = ((float)int_temp.INT16) * TEMPERATURE_SENSOR_RESOLUTION; // Convert to float including the 2 bit shift
  return(ret_val);
}

// Configure the input multiplexer
bool SFE_ADS122C04::setInputMultiplexer(uint8_t mux_config)
{
  if ((ADS122C04_readReg(ADS122C04_CONFIG_0_REG, &ADS122C04_Reg.reg0.all)) == false)
    return(false);
  ADS122C04_Reg.reg0.bit.MUX = mux_config;
  return(ADS122C04_writeReg(ADS122C04_CONFIG_0_REG, ADS122C04_Reg.reg0.all));
}

// Configure the gain
bool SFE_ADS122C04::setGain(uint8_t gain_config)
{
  if ((ADS122C04_readReg(ADS122C04_CONFIG_0_REG, &ADS122C04_Reg.reg0.all)) == false)
    return(false);
  ADS122C04_Reg.reg0.bit.GAIN = gain_config;
  return(ADS122C04_writeReg(ADS122C04_CONFIG_0_REG, ADS122C04_Reg.reg0.all));
}

// Enable/disable the Programmable Gain Amplifier
bool SFE_ADS122C04::enablePGA(uint8_t enable)
{
  if ((ADS122C04_readReg(ADS122C04_CONFIG_0_REG, &ADS122C04_Reg.reg0.all)) == false)
    return(false);
  ADS122C04_Reg.reg0.bit.PGA_BYPASS = enable;
  return(ADS122C04_writeReg(ADS122C04_CONFIG_0_REG, ADS122C04_Reg.reg0.all));
}

// Set the data rate (sample speed)
bool SFE_ADS122C04::setDataRate(uint8_t rate)
{
  if ((ADS122C04_readReg(ADS122C04_CONFIG_1_REG, &ADS122C04_Reg.reg1.all)) == false)
    return(false);
  ADS122C04_Reg.reg1.bit.DR = rate;
  return(ADS122C04_writeReg(ADS122C04_CONFIG_1_REG, ADS122C04_Reg.reg1.all));
}

// Configure the operating mode (normal / turbo)
bool SFE_ADS122C04::setOperatingMode(uint8_t mode)
{
  if ((ADS122C04_readReg(ADS122C04_CONFIG_1_REG, &ADS122C04_Reg.reg1.all)) == false)
    return(false);
  ADS122C04_Reg.reg1.bit.MODE = mode;
  return(ADS122C04_writeReg(ADS122C04_CONFIG_1_REG, ADS122C04_Reg.reg1.all));
}

// Configure the conversion mode (single-shot / continuous)
bool SFE_ADS122C04::setConversionMode(uint8_t mode)
{
  if ((ADS122C04_readReg(ADS122C04_CONFIG_1_REG, &ADS122C04_Reg.reg1.all)) == false)
    return(false);
  ADS122C04_Reg.reg1.bit.CMBIT = mode;
  return(ADS122C04_writeReg(ADS122C04_CONFIG_1_REG, ADS122C04_Reg.reg1.all));
}

// Configure the voltage reference
bool SFE_ADS122C04::setVoltageReference(uint8_t ref)
{
  if ((ADS122C04_readReg(ADS122C04_CONFIG_1_REG, &ADS122C04_Reg.reg1.all)) == false)
    return(false);
  ADS122C04_Reg.reg1.bit.VREF = ref;
  return(ADS122C04_writeReg(ADS122C04_CONFIG_1_REG, ADS122C04_Reg.reg1.all));
}

// Enable / disable the internal temperature sensor
bool SFE_ADS122C04::enableInternalTempSensor(uint8_t enable)
{
  if ((ADS122C04_readReg(ADS122C04_CONFIG_1_REG, &ADS122C04_Reg.reg1.all)) == false)
    return(false);
  ADS122C04_Reg.reg1.bit.TS = enable;
  if (_printDebug == true)
  {
    _debugPort->print(F("enableInternalTempSensor: ADS122C04_Reg.reg1.bit.TS = 0x"));
    _debugPort->println(ADS122C04_Reg.reg1.bit.TS, HEX);
  }
  return(ADS122C04_writeReg(ADS122C04_CONFIG_1_REG, ADS122C04_Reg.reg1.all));
}

// Enable / disable the conversion data counter
bool SFE_ADS122C04::setDataCounter(uint8_t enable)
{
  if ((ADS122C04_readReg(ADS122C04_CONFIG_2_REG, &ADS122C04_Reg.reg2.all)) == false)
    return(false);
  ADS122C04_Reg.reg2.bit.DCNT = enable;
  return(ADS122C04_writeReg(ADS122C04_CONFIG_2_REG, ADS122C04_Reg.reg2.all));
}

// Configure the data integrity check
bool SFE_ADS122C04::setDataIntegrityCheck(uint8_t setting)
{
  if ((ADS122C04_readReg(ADS122C04_CONFIG_2_REG, &ADS122C04_Reg.reg2.all)) == false)
    return(false);
  ADS122C04_Reg.reg2.bit.CRCbits = setting;
  return(ADS122C04_writeReg(ADS122C04_CONFIG_2_REG, ADS122C04_Reg.reg2.all));
}

// Enable / disable the 10uA burn-out current source
bool SFE_ADS122C04::setBurnOutCurrent(uint8_t enable)
{
  if ((ADS122C04_readReg(ADS122C04_CONFIG_2_REG, &ADS122C04_Reg.reg2.all)) == false)
    return(false);
  ADS122C04_Reg.reg2.bit.BCS = enable;
  return(ADS122C04_writeReg(ADS122C04_CONFIG_2_REG, ADS122C04_Reg.reg2.all));
}

// Configure the internal programmable current sources
bool SFE_ADS122C04::setIDACcurrent(uint8_t current)
{
  if ((ADS122C04_readReg(ADS122C04_CONFIG_2_REG, &ADS122C04_Reg.reg2.all)) == false)
    return(false);
  ADS122C04_Reg.reg2.bit.IDAC = current;
  if (_printDebug == true)
  {
    _debugPort->print(F("setIDACcurrent: ADS122C04_Reg.reg2.bit.IDAC = 0x"));
    _debugPort->println(ADS122C04_Reg.reg2.bit.IDAC, HEX);
  }
  return(ADS122C04_writeReg(ADS122C04_CONFIG_2_REG, ADS122C04_Reg.reg2.all));
}

// Configure the IDAC1 routing
bool SFE_ADS122C04::setIDAC1mux(uint8_t setting)
{
  if ((ADS122C04_readReg(ADS122C04_CONFIG_3_REG, &ADS122C04_Reg.reg3.all)) == false)
    return(false);
  ADS122C04_Reg.reg3.bit.I1MUX = setting;
  return(ADS122C04_writeReg(ADS122C04_CONFIG_3_REG, ADS122C04_Reg.reg3.all));
}

// Configure the IDAC2 routing
bool SFE_ADS122C04::setIDAC2mux(uint8_t setting)
{
  if ((ADS122C04_readReg(ADS122C04_CONFIG_3_REG, &ADS122C04_Reg.reg3.all)) == false)
    return(false);
  ADS122C04_Reg.reg3.bit.I2MUX = setting;
  return(ADS122C04_writeReg(ADS122C04_CONFIG_3_REG, ADS122C04_Reg.reg3.all));
}

// Read Config Reg 2 and check the DRDY bit
// Data is ready when DRDY is high
bool SFE_ADS122C04::checkDataReady(void)
{
  ADS122C04_readReg(ADS122C04_CONFIG_2_REG, &ADS122C04_Reg.reg2.all);
  return(ADS122C04_Reg.reg2.bit.DRDY > 0);
}

// Get the input multiplexer configuration
uint8_t SFE_ADS122C04::getInputMultiplexer(void)
{
  ADS122C04_readReg(ADS122C04_CONFIG_0_REG, &ADS122C04_Reg.reg0.all);
  return(ADS122C04_Reg.reg0.bit.MUX);
}

// Get the gain setting
uint8_t SFE_ADS122C04::getGain(void)
{
  ADS122C04_readReg(ADS122C04_CONFIG_0_REG, &ADS122C04_Reg.reg0.all);
  return(ADS122C04_Reg.reg0.bit.GAIN);
}

// Get the Programmable Gain Amplifier status
uint8_t SFE_ADS122C04::getPGAstatus(void)
{
  ADS122C04_readReg(ADS122C04_CONFIG_0_REG, &ADS122C04_Reg.reg0.all);
  return(ADS122C04_Reg.reg0.bit.PGA_BYPASS);
}

// Get the data rate (sample speed)
uint8_t SFE_ADS122C04::getDataRate(void)
{
  ADS122C04_readReg(ADS122C04_CONFIG_1_REG, &ADS122C04_Reg.reg1.all);
  return(ADS122C04_Reg.reg1.bit.DR);
}

// Get the operating mode (normal / turbo)
uint8_t SFE_ADS122C04::getOperatingMode(void)
{
  ADS122C04_readReg(ADS122C04_CONFIG_1_REG, &ADS122C04_Reg.reg1.all);
  return(ADS122C04_Reg.reg1.bit.MODE);
}

// Get the conversion mode (single-shot / continuous)
uint8_t SFE_ADS122C04::getConversionMode(void)
{
  ADS122C04_readReg(ADS122C04_CONFIG_1_REG, &ADS122C04_Reg.reg1.all);
  return(ADS122C04_Reg.reg1.bit.CMBIT);
}

// Get the voltage reference configuration
uint8_t SFE_ADS122C04::getVoltageReference(void)
{
  ADS122C04_readReg(ADS122C04_CONFIG_1_REG, &ADS122C04_Reg.reg1.all);
  return(ADS122C04_Reg.reg1.bit.VREF);
}

// Get the internal temperature sensor status
uint8_t SFE_ADS122C04::getInternalTempSensorStatus(void)
{
  ADS122C04_readReg(ADS122C04_CONFIG_1_REG, &ADS122C04_Reg.reg1.all);
  if (_printDebug == true)
  {
    _debugPort->print(F("getInternalTempSensorStatus: ADS122C04_Reg.reg1.bit.TS = 0x"));
    _debugPort->println(ADS122C04_Reg.reg1.bit.TS, HEX);
  }
  return(ADS122C04_Reg.reg1.bit.TS);
}

// Get the data counter status
uint8_t SFE_ADS122C04::getDataCounter(void)
{
  ADS122C04_readReg(ADS122C04_CONFIG_2_REG, &ADS122C04_Reg.reg2.all);
  return(ADS122C04_Reg.reg2.bit.DCNT);
}

// Get the data integrity check configuration
uint8_t SFE_ADS122C04::getDataIntegrityCheck(void)
{
  ADS122C04_readReg(ADS122C04_CONFIG_2_REG, &ADS122C04_Reg.reg2.all);
  return(ADS122C04_Reg.reg2.bit.CRCbits);
}

// Get the burn-out current status
uint8_t SFE_ADS122C04::getBurnOutCurrent(void)
{
  ADS122C04_readReg(ADS122C04_CONFIG_2_REG, &ADS122C04_Reg.reg2.all);
  return(ADS122C04_Reg.reg2.bit.BCS);
}

// Get the IDAC setting
uint8_t SFE_ADS122C04::getIDACcurrent(void)
{
  ADS122C04_readReg(ADS122C04_CONFIG_2_REG, &ADS122C04_Reg.reg2.all);
  if (_printDebug == true)
  {
    _debugPort->print(F("getIDACcurrent: ADS122C04_Reg.reg2.bit.IDAC = 0x"));
    _debugPort->println(ADS122C04_Reg.reg2.bit.IDAC, HEX);
  }
  return(ADS122C04_Reg.reg2.bit.IDAC);
}

// Get the IDAC1 mux configuration
uint8_t SFE_ADS122C04::getIDAC1mux(void)
{
  ADS122C04_readReg(ADS122C04_CONFIG_3_REG, &ADS122C04_Reg.reg3.all);
  return(ADS122C04_Reg.reg3.bit.I1MUX);
}

// Get the IDAC2 mux configuration
uint8_t SFE_ADS122C04::getIDAC2mux(void)
{
  ADS122C04_readReg(ADS122C04_CONFIG_3_REG, &ADS122C04_Reg.reg3.all);
  return(ADS122C04_Reg.reg3.bit.I2MUX);
}

// Update ADS122C04_Reg and initialise the ADS122C04 using the supplied parameters
bool SFE_ADS122C04::ADS122C04_init(ADS122C04_initParam *param)
{

  Serial.println("Entering function: " + String(__func__));
  ADS122C04_Reg.reg0.all = 0; // Reset all five register values to the default value of 0x00
  ADS122C04_Reg.reg1.all = 0;
  ADS122C04_Reg.reg2.all = 0;
  ADS122C04_Reg.reg3.all = 0;
  ADS122C04_Reg.reg4.all = 0;

  ADS122C04_Reg.reg0.bit.MUX = param->inputMux;
  ADS122C04_Reg.reg0.bit.GAIN = param->gainLevel;
  ADS122C04_Reg.reg0.bit.PGA_BYPASS = param->pgaBypass;

  ADS122C04_Reg.reg1.bit.DR = param->dataRate;
  ADS122C04_Reg.reg1.bit.MODE = param->opMode;
  ADS122C04_Reg.reg1.bit.CMBIT = param->convMode;
  ADS122C04_Reg.reg1.bit.VREF = param->selectVref;
  ADS122C04_Reg.reg1.bit.TS = param->tempSensorEn;

  ADS122C04_Reg.reg2.bit.DRDY = param->dataReadyEn;
  ADS122C04_Reg.reg2.bit.DCNT = param->dataCounterEn;
  ADS122C04_Reg.reg2.bit.CRCbits = param->dataCRCen;
  ADS122C04_Reg.reg2.bit.BCS = param->burnOutEn;
  ADS122C04_Reg.reg2.bit.IDAC = param->idacCurrent;

  ADS122C04_Reg.reg3.bit.I1MUX = param->routeIDAC1;
  ADS122C04_Reg.reg3.bit.I2MUX = param->routeIDAC2;
  ADS122C04_Reg.reg3.bit.AUTO = param->outputMODE;

  ADS122C04_Reg.reg4.bit.GPIO2DIR = param->gpio2DIR;
  ADS122C04_Reg.reg4.bit.GPIO1DIR = param->gpio1DIR;
  ADS122C04_Reg.reg4.bit.GPIO0DIR = param->gpio0DIR;
  ADS122C04_Reg.reg4.bit.GPIO2SEL = param->gpio2SEL;
  ADS122C04_Reg.reg4.bit.GPIO2DAT = param->gpio2DAT;
  ADS122C04_Reg.reg4.bit.GPIO1DAT = param->gpio1DAT;
  ADS122C04_Reg.reg4.bit.GPIO0DAT = param->gpio0DAT;

  bool ret_val = true; // Flag to show if the four writeRegs were successful
  // (If any one writeReg returns false, ret_val will be false)


  ret_val &= ADS122C04_writeReg(ADS122C04_CONFIG_0_REG, ADS122C04_Reg.reg0.all);
  delay(1);
  ret_val &= ADS122C04_writeReg(ADS122C04_CONFIG_1_REG, ADS122C04_Reg.reg1.all);
   delay(1);
  ret_val &= ADS122C04_writeReg(ADS122C04_CONFIG_2_REG, ADS122C04_Reg.reg2.all);
   delay(1);
  ret_val &= ADS122C04_writeReg(ADS122C04_CONFIG_3_REG, ADS122C04_Reg.reg3.all);
   delay(1);
  ret_val &= ADS122C04_writeReg(ADS122C04_CONFIG_4_REG, ADS122C04_Reg.reg4.all);

// Serial.println("E function: " + String(__func__));

  // Read and print the new configuration (if enableDebugging has been called)
  printADS122C04config();

  return(ret_val);
}

// Debug print of the ADS122C04 configuration
void SFE_ADS122C04::printADS122C04config(void)
{
  if (_printDebug == true)
  {
    bool successful = true; // Flag to show if the four readRegs were successful
    // (If any one readReg returns false, success will be false)
    successful &= ADS122C04_readReg(ADS122C04_CONFIG_0_REG, &ADS122C04_Reg.reg0.all);
    successful &= ADS122C04_readReg(ADS122C04_CONFIG_1_REG, &ADS122C04_Reg.reg1.all);
    successful &= ADS122C04_readReg(ADS122C04_CONFIG_2_REG, &ADS122C04_Reg.reg2.all);
    successful &= ADS122C04_readReg(ADS122C04_CONFIG_3_REG, &ADS122C04_Reg.reg3.all);

    if (successful == false)
    {
      _debugPort->println(F("printADS122C04config: readReg failed"));
      return;
    }
    else
    {
      _debugPort->print(F("ConfigReg0: MUX="));
      _debugPort->print(ADS122C04_Reg.reg0.bit.MUX);
      _debugPort->print(F(" GAIN="));
      _debugPort->print(ADS122C04_Reg.reg0.bit.GAIN);
      _debugPort->print(F(" PGA_BYPASS="));
      _debugPort->println(ADS122C04_Reg.reg0.bit.PGA_BYPASS);
      _debugPort->print(F("ConfigReg1: DR="));
      _debugPort->print(ADS122C04_Reg.reg1.bit.DR);
      _debugPort->print(F(" MODE="));
      _debugPort->print(ADS122C04_Reg.reg1.bit.MODE);
      _debugPort->print(F(" CMBIT="));
      _debugPort->print(ADS122C04_Reg.reg1.bit.CMBIT);
      _debugPort->print(F(" VREF="));
      _debugPort->print(ADS122C04_Reg.reg1.bit.VREF);
      _debugPort->print(F(" TS="));
      _debugPort->println(ADS122C04_Reg.reg1.bit.TS);
      _debugPort->print(F("ConfigReg2: DCNT="));
      _debugPort->print(ADS122C04_Reg.reg2.bit.DCNT);
      _debugPort->print(F(" CRC="));
      _debugPort->print(ADS122C04_Reg.reg2.bit.CRCbits);
      _debugPort->print(F(" BCS="));
      _debugPort->print(ADS122C04_Reg.reg2.bit.BCS);
      _debugPort->print(F(" IDAC="));
      _debugPort->println(ADS122C04_Reg.reg2.bit.IDAC);
      _debugPort->print(F("ConfigReg3: I1MUX="));
      _debugPort->print(ADS122C04_Reg.reg3.bit.I1MUX);
      _debugPort->print(F(" I2MUX="));
      _debugPort->println(ADS122C04_Reg.reg3.bit.I2MUX);
    }
  }
}

bool SFE_ADS122C04::reset(void)
{
  return(ADS122C04_sendCommand(ADS122C04_RESET_CMD));
}

bool SFE_ADS122C04::start(void)
{
  return(ADS122C04_sendCommand(ADS122C04_START_CMD));
}

bool SFE_ADS122C04::powerdown(void)
{
  return(ADS122C04_sendCommand(ADS122C04_POWERDOWN_CMD));
}

bool SFE_ADS122C04::ADS122C04_writeReg(uint8_t reg, uint8_t writeValue)
{
  uint8_t command = 0;
  command = ADS122C04_WRITE_CMD(reg);
  // Serial.print("command: 0x");
  // Serial.println(command, HEX);
  // Serial.print("value: 0x");
  // Serial.println(writeValue, HEX);
  return (ADS122C04_sendCommandWithValue(command, writeValue));
}

bool SFE_ADS122C04::ADS122C04_readReg(uint8_t reg, uint8_t *readValue)
{
  uint8_t command = 0;
  command = ADS122C04_READ_CMD(reg);

 
  _serialPort->write(ADS122C04_SYNC_HEAD);
  // Serial.print("command: 0x");
  // Serial.println(command, HEX);
  _serialPort->write(command);

  if (_serialPort->available() >= 1)
  {
    *readValue = _serialPort->read();
    return(true);
  }

  if (_printDebug == true)
  {
    _debugPort->println(F("ADS122C04_readReg: requestFrom returned no data"));
  }
  return(false);
}

bool SFE_ADS122C04::ADS122C04_sendCommand(uint8_t command)
{
  _serialPort->write(ADS122C04_SYNC_HEAD);
  _serialPort->write(command);
  return true; // todo chepi
}

bool SFE_ADS122C04::ADS122C04_sendCommandWithValue(uint8_t command, uint8_t value)
{
   _serialPort->write(ADS122C04_SYNC_HEAD);
  _serialPort->write(command);
  _serialPort->write(value);
  return true;   // todo chepi
}

// Read the conversion result with count byte.
// The conversion result is 24-bit two's complement (signed)
// and is returned in the 24 lowest bits of the uint32_t conversionData.
// Hence it will always appear positive.
// Higher functions will need to take care of converting it to (e.g.) float or int32_t.
bool SFE_ADS122C04::ADS122C04_getConversionDataWithCount(uint32_t *conversionData, uint8_t *count)
{
  uint8_t RXByte[4] = {0};

  _serialPort->write(ADS122C04_RDATA_CMD);

  // Note: the next line will need to be changed if data integrity is enabled.
  //       The code will need to request 6 bytes for CRC or 7 bytes for inverted data.

  if (_printDebug == true)
  {
    if (_serialPort->available() == 3)
    {
      _debugPort->println(F("ADS122C04_getConversionDataWithCount: only 3 bytes available. Maybe DCNT is disabled?"));
    }
  }

  if (_serialPort->available() >= 4)
  {
    RXByte[0] = _serialPort->read(); // Count
    RXByte[1] = _serialPort->read(); // MSB
    RXByte[2] = _serialPort->read();
    RXByte[3] = _serialPort->read(); // LSB
    if (_serialPort->available() > 0)// Note: this _should_ be redundant
    {
      if (_printDebug == true)
      {
        _debugPort->println(F("ADS122C04_getConversionDataWithCount: excess bytes available. Maybe data integrity is enabled?"));
      }
      while (_serialPort->available() > 0)
      {
        _serialPort->read(); // Read and ignore excess bytes (presumably inverted data or CRC)
      }
    }
  }
  else
  {
    if (_printDebug == true)
    {
      _debugPort->println(F("ADS122C04_getConversionDataWithCount: requestFrom failed"));
    }
    return(false);
  }

  *count = RXByte[0];
  *conversionData = ((uint32_t)RXByte[3]) | ((uint32_t)RXByte[2]<<8) | ((uint32_t)RXByte[1]<<16);
  return(true);
}

// Read the conversion result.
// The conversion result is 24-bit two's complement (signed)
// and is returned in the 24 lowest bits of the uint32_t conversionData.
// Hence it will always appear positive.
// Higher functions will need to take care of converting it to (e.g.) float or int32_t.
bool SFE_ADS122C04::ADS122C04_getConversionData(uint32_t *conversionData)
{
  uint8_t RXByte[3] = {0};


  _serialPort->write( ADS122C04_SYNC_HEAD);
  _serialPort->write(ADS122C04_RDATA_CMD);
  delay(10);

  // Note: the next line will need to be changed if data integrity is enabled.
  //       The code will need to request 5 bytes for CRC or 6 bytes for inverted data.

  if (_serialPort->available() >= 3)
  {
    RXByte[0] = _serialPort->read(); // MSB
      delayMicroseconds(10);
    RXByte[1] = _serialPort->read();
      delayMicroseconds(10);
    RXByte[2] = _serialPort->read(); // LSB
    if (_serialPort->available() > 0) // Note: this _should_ be redundant
    {
      if (_printDebug == true)
      {
        _debugPort->println(F("ADS122C04_getConversionData: excess bytes available. Maybe data integrity is enabled?"));
      }
      while (_serialPort->available() > 0)
      {
        _serialPort->read(); // Read and ignore excess bytes (presumably inverted data or CRC)
      }
    }
  }
  else
  {
    if (_printDebug == true)
    {
      _debugPort->println(F("ADS122C04_getConversionData: requestFrom failed"));
    }
    return(false);
  }

  // *conversionData = ((uint32_t)RXByte[2]) | ((uint32_t)RXByte[1]<<8) | ((uint32_t)RXByte[0]<<16);
  *conversionData =  (RXByte[0] + RXByte[1] * 256 + RXByte[2] * 65536);


  return (true);
}
