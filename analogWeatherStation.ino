/****************************************************************************
 Analog Weather Station
   Do we need numbers every day? No. This device will show room temperature
   and actual weather condition with a combination of LEDs.
 
 Simple behaviour for pressure readings:
 P < 1000 mbar = low pressure -> simulate lightning with a flashing blue LED
 1000 < P < 1020 mbar -> variable weather, simulate soft light with both blue and yellow LEDs
 P > 1020 mbar = high pressure -> simulate sunshine with a fading yellow LED
 
 Simple behaviour for temperature readings (dedicated RGB LED):
 T < 12° -> very cold, blue color
 12° < T < 18° -> cold, cyan shades
 18° < T < 24° -> fair, green shades
 24° < T < 30° -> warm, yellow shades
 T > 30° -> hot, red shades

 Includes serial monitoring to check sensor readings.

 Copyright (C) 2018 Danilo Ciliberti dancili@gmail.com
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program. If not, see <https://www.gnu.org/licenses/>
***************************************************************************/

#include <SFE_BMP180.h>
#define ALTITUDE 125.0
SFE_BMP180 pressure;

// Pins supporting PWM
const int redPin = 6;
const int greenPin = 5;
const int bluePin = 3;
const int thunderLedPin = 10;
const int sunLedPin = 11;

const int Toffset = -3;             // temperature offset (w/r to another device)
int light = 0;                      // used to regulate LED intensity
int shade = 0;                      // used to regulate RGB LED color

// User-defined functions
void sunny() {
  // increase LED light
  while (light < 255)  {
    light = light + 1;
    analogWrite(sunLedPin,light);
    delay(100);
  }
  
  // decrease LED light
  while (light > 10) {
    light = light - 1;
    analogWrite(sunLedPin,light);
    delay(100);
  }
}

void stormy() {
  digitalWrite(thunderLedPin, HIGH);
  delay(random(50,200));
  digitalWrite(thunderLedPin, LOW);
  delay(random(50,200));
  digitalWrite(thunderLedPin, HIGH);
  delay(random(50,200));
  digitalWrite(thunderLedPin, LOW);
  delay(random(50,200));
  digitalWrite(thunderLedPin, HIGH);
  delay(random(50,200));
  digitalWrite(thunderLedPin, LOW);
  delay(random(50,200));
  digitalWrite(thunderLedPin, HIGH);
  delay(random(100,500));
  digitalWrite(thunderLedPin, LOW);
  delay(random(3000, 10000));
}

void variable(){
  analogWrite(sunLedPin,190);
  analogWrite(thunderLedPin,250);
  delay(10000);
}


// Arduino functions
void setup() {

  // Initialize serial communication
  Serial.begin(9600);
  Serial.println(F("REBOOT"));
  
  // Initialize the pressure sensor (it is important to get calibration values stored on the device).
  if (pressure.begin())
    Serial.println(F("BMP180 init success"));
  else
  {
    // Oops, something went wrong, this is usually a connection problem,
    // see the comments at the top of this sketch for the proper connections.
    Serial.println(F("BMP180 init fail\n\n"));
    while(1); // Pause forever
  }

  // Assign digital pins as output
  pinMode(redPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(thunderLedPin, OUTPUT);
  pinMode(sunLedPin, OUTPUT);
}

void loop() {

  char status;
  double T, P;

  // Pressure measurements
  // If you want sea-level-compensated pressure, as used in weather reports,
  // you will need to know the altitude at which your measurements are taken.
  // We're using a constant called ALTITUDE in this sketch:
  
  Serial.println();
  Serial.print(F("provided altitude: "));
  Serial.print(ALTITUDE,0);
  Serial.print(F(" meters, "));
  Serial.print(ALTITUDE*3.28084,0);
  Serial.println(F(" feet"));

  // You must first get a temperature measurement to perform a pressure reading.
  // Start a temperature measurement:
  // If request is successful, the number of ms to wait is returned.
  // If request is unsuccessful, 0 is returned.

  status = pressure.startTemperature();
  if (status != 0)
  {
    // Wait for the measurement to complete:
    delay(status);

    // Retrieve the completed temperature measurement:
    // Note that the measurement is stored in the variable T.
    // Function returns 1 if successful, 0 if failure.

    status = pressure.getTemperature(T);
    // Correct temperature (if an offset has been previously calculated by the user with another device)
    T = T + Toffset;
    
    if (status != 0)
    {
      // Print out the measurement:
      Serial.print(F("temperature: "));
      Serial.print(T,2);
      Serial.print(F(" deg C, "));
      Serial.print((9.0/5.0)*T+32.0,2);
      Serial.println(F(" deg F"));
      
      // Start a pressure measurement:
      // The parameter is the oversampling setting, from 0 to 3 (highest res, longest wait).
      // If request is successful, the number of ms to wait is returned.
      // If request is unsuccessful, 0 is returned.

      status = pressure.startPressure(3);
      if (status != 0)
      {
        // Wait for the measurement to complete:
        delay(status);

        // Retrieve the completed pressure measurement:
        // Note that the measurement is stored in the variable P.
        // Note also that the function requires the previous temperature measurement (T).
        // (If temperature is stable, you can do one temperature measurement for a number of pressure measurements.)
        // Function returns 1 if successful, 0 if failure.

        status = pressure.getPressure(P,T);
        if (status != 0)
        {
          // Print out the measurement:
          Serial.print(F("absolute pressure: "));
          Serial.print(P,2);
          Serial.print(F(" mb, "));
          Serial.print(P*0.0295333727,2);
          Serial.println(F(" inHg"));

          // The pressure sensor returns abolute pressure, which varies with altitude.
          // To remove the effects of altitude, use the sealevel function and your current altitude.
          // This number is commonly used in weather reports.
          // Parameters: P = absolute pressure in mb, ALTITUDE = current altitude in m.
          // Result: P = sea-level compensated pressure in mb

          P = pressure.sealevel(P,ALTITUDE);
          Serial.print(F("relative (sea-level) pressure: "));
          Serial.print(P,2);
          Serial.print(F(" mb, "));
          Serial.print(P*0.0295333727,2);
          Serial.println(F(" inHg"));
        }
        else Serial.println(F("error retrieving pressure measurement\n"));
      }
      else Serial.println(F("error starting pressure measurement\n"));
    }
    else Serial.println(F("error retrieving temperature measurement\n"));
  }
  else Serial.println(F("error starting temperature measurement\n"));

  // Once determined pressure and temperature, we can activate the LEDs
  if (T <= 12){                    // blue color
    shade = 0;
    analogWrite(redPin,0);
    analogWrite(greenPin,0);
    analogWrite(bluePin,255);
    Serial.println(F("expected RGB values:"));
    Serial.println(0);    // red
    Serial.println(0);    // green
    Serial.println(255);  // blue
    
  } else if (T > 12 && T <= 18){   // cyan shades
    shade = map(T,12,18,0,255);
    analogWrite(redPin,0);
    analogWrite(greenPin,shade);
    analogWrite(bluePin,255);
    Serial.println(F("expected RGB values:"));
    Serial.println(0);      // red
    Serial.println(shade);  // green
    Serial.println(255);    // blue
    
  } else if (T > 18 && T <= 23){  // green shades
    shade = map(T,18,23,255,0);
    analogWrite(redPin,0);
    analogWrite(greenPin,255);
    analogWrite(bluePin,shade);
    Serial.println(F("expected RGB values:"));
    Serial.println(0);      // red
    Serial.println(255);    // green
    Serial.println(shade);  // blue
    
  } else if (T > 23 && T <= 29){  // yellow shades
    shade = map(T,23,29,0,255);
    analogWrite(redPin,shade);
    analogWrite(greenPin,255);
    analogWrite(bluePin,0);
    Serial.println(F("expected RGB values:"));
    Serial.println(shade);  // red
    Serial.println(255);    // green
    Serial.println(0);      // blue
    
  } else{ // T > 29°
    shade = map(T,29,40,255,0);   // red shades
    analogWrite(redPin,255);
    analogWrite(greenPin,shade);
    analogWrite(bluePin,0);
    Serial.println(F("expected RGB values:"));
    Serial.println(255);    // red
    Serial.println(shade);  // green
    Serial.println(0);      // blue
  }

  // Activate the LEDs according to forecast
  if (P < 1000) {
    Serial.println(F("probably stormy weather"));
    stormy();
    
  } else if (P > 1020){
    Serial.println(F("probably sunny weather"));
    sunny();
  }
  
  else{
    Serial.println(F("probably variable weather"));
    variable();
  }
  
}
