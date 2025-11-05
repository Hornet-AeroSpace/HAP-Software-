
#include "Arduino.h"
#include <string>
#include <Wire.h>
#include "Adafruit_ICM20948.h"
#include "Adafruit_ICM20X.h"
#include <Adafruit_Sensor.h>
#include "Adafruit_BMP3XX.h"
#include "Adafruit_ADXL375.h"
#include "Adafruit_INA260.h"
#include <SPI.h>
#include <SD.h>

int stageNumber = 1;

// Classes & Structs 
struct Vector3D {
  float x, y, z;
};

struct FlightData {
  unsigned long timeStamp;
  Vector3D accel_HR;  Vector3D accel_LR;  Vector3D gyro;
  float pressure;
  float altitude;
  float temperature;   
};


void setup() {
Serial.Begin(9600);

Serial.print("Initializing SD card...");
  if (!SD.begin(chipSelect)) {
    Serial.println("Initialization failed!");
       while (1);
    }


 FlightData FData; 
}

void loop() {
// set flight data with new values every loop 

  switch (STAGENUMBER) {
    case 1:
    stageOne();   // Checking For accelration, ( needs 3 components bc the sensor can have multiple orientations)
      break;
    case 2:
    stageTwo();           // Wating 5.5s
      break;
    case 3:
    stageThree();        // Checking for Apogee 
      break;
    case 4: 
    stageFour();         // Checking for 1400ft
    default:
      break;
  }

}


//-------------- Function Definitions --------------

void stageOne() { //Idle
    //read accelerometer data
    //Determine if any movement has happened

    if (0 /* light jostle */) { //if (0) as placeholders; delete when implimented
        stageNumber = 2;
    }
}

void stageTwo() { //Potentially Launching
    //Preform On-Flight computation (sensorfusion based velocity)
    //Maintain a running total of the previous 30sec of all sensor data
    //Measure acceleration & barometric pressure
    
    if (0 /* accelerated at 1.7Gs */) { //Maybe - use microphone instead… Decibel level could indicate launch
        stageNumber = 3;
    } else if (0 /* went idle */) {
        stageNumber = 1;
    }
}

void stageThree() { //Actively Launching
    //Preform On-Flight computation (sensorfusion based velocity)
    //Switch off decision logic if spd
    //Transmit telemetry data through UART
    //Write telemetry data to sdCard

    if (0 /* pre-defined parameters to deploy parachute */) {
        //deploy parachute
        stageNumber = 4;
    }
}

void stageFour() { //Launch (Second Stage)
    //Preform On-Flight computation (sensorfusion based velocity)
    //Transmit telemetry data through UART
    //Write telemetry data to sdCard
    //Deploy parachute off of pre-defined params. (optional)
}