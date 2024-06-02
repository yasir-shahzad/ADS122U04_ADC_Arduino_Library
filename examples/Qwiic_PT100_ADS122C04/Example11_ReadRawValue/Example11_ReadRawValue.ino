/*
  Using the Qwiic PT100
  By: Paul Clark (PaulZC)
  Date: May 5th, 2020

  This example demonstrates how to read the PT100 temperature in Centigrade
  using 4-wire mode

  Hardware Connections:
  Plug a Qwiic cable into the PT100 and a BlackBoard
  If you don't have a platform with a Qwiic connection use the Fun Qwiic Breadboard Jumper
  Open the serial monitor at 115200 baud to see the output
*/

#include <Wire.h>

#include <ADS122U04_ADC_Arduino_Library.h>

SFE_ADS122U04 mySensor;

void setup(void)
{
  Serial.begin(115200);
  Serial1.begin(115200);
  Serial2.begin(9600);   // Debugging port
  while (!Serial); //Wait for user to open terminal
pinMode(2, INPUT);
  delay(2000);
  Serial.println(F("Read Raw Value"));

  // mySensor.enableDebugging(); //Uncomment this line to enable debug messages on Serials

  if (mySensor.begin() == false) //Connect to the PT100 using the defaults: Address 0x45 and the Wire port
  {
    Serial.println(F("Qwiic PT100 not detected at default I2C address. Please check wiring. Freezing."));
    while (1)
      ;
  }

  mySensor.start();  // start converstion

  // mySensor.configureADCmode(ADS122U04_4WIRE_MODE); // Configure the PT100 for 4-wire mode


}
// unsigned long av0[100];
unsigned long long myin=0;
String Alpha;

void loop()
{
//   for(int i=0;i<200;i++)
//   {
// myin+= mySensor.getRawValue();
// delay(25);
//   }
// myin=myin/200;
  // Get the temperature in Centigrade
  // if(digitalRead(2) == LOW) {
  int weig=0;
for(int i=0;i<10;i++)
{
weig+=mySensor.getRawValue();
delay(50);
}
weig=weig/10;
  //  uint32_t weightRaw =  mySensor.getRawValue();
  // Print the weight
  // int weigh=(3334623-myin)/115.758;
  Serial.print("The Weight is: ");
  Serial.println(weig);
  // delay(50);

  // Serial.println(weigh);
  //   char buffer[100];
  // sprintf(buffer, "%0ld", myin/1000000L); 
  // Serial.print(buffer);  
  // sprintf(buffer, "%0ld",myin%1000000L); 
  // Serial.println(buffer); 
  // }
  // delay(500); 
// 3334623-3218865/1000
//   115.758
}
