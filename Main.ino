#include "Arduino.h"
#include <Wire.h>
#include "Adafruit_ICM20948.h"
#include "Adafruit_ICM20X.h"
#include <Adafruit_Sensor.h>
#include "Adafruit_BMP3XX.h"
#include "Adafruit_ADXL375.h"
#include <SPI.h>
#include <SD.h>



// MUST READ!! CONSTANTS TO DEFINE BEFORE LAUNCH

#define MOTOR_BURN_TIME           // (optional) written on motor casing
#define MAX_FLIGHT_TIME   70000   // 70 sec backup deployment (ms). Must confirm with simulation
#define MAIN_CHUTE_ALTITUDE 400   //  SET TO ZERO IF THERE IS NO MAIN
#define IS_SONIC false            // Is rocket approaching mach 1
#define MOSFET_PIN  8             // ought to double check esp pin wiring. 
#define MOSFET_PIN2 6
//-------------------------------------------------


// Pin Definitions
#define SPI_MOSI    23
#define SPI_MISO    19
#define SPI_CLK     18
#define CS_ICM20948  4
#define CS_BMP390    3
#define CS_ADXL375  16
#define CS_SD       17
#define SDA_PIN     21
#define SCL_PIN     22
#define CONTINUITY_OUT  25    // Output pin (sends signal)
#define CONTINUITY_IN   26    // Input pin (receives signal)

// Flight Parameters
#define LAUNCH_G_THRESHOLD 2.5     // G-force to detect launch
#define LAUNCH_SUSTAIN_TIME 50    // 50ms sustained G for launch detect
#define SENSOR_RATE 100             // 1000Hz sensor sampling
#define LOG_RATE 10                 // 20Hz data logging

int stageNumber = 1;
unsigned long flightStartTime = 0;
unsigned long lastSensorRead = 0;
unsigned long lastLogWrite = 0;

// Hardware Objects
Adafruit_ICM20948 icm;
Adafruit_ADXL375 adxl(CS_ADXL375, &SPI, 12345);
Adafruit_BMP3XX bmp;
File dataFile;

// Data Structures
struct Vector3D {
  float x, y, z;
};

struct FlightData {
  unsigned long timeStamp;
  Vector3D accel_HR;    // High-range accelerometer (ADXL375)
  Vector3D accel_LR;    // Low-range accelerometer (ICM20948)  
  Vector3D gyro;        // Gyroscope data
  float pressure;
  float altitude;
  float temperature;
  int stage;
  bool deploymentStatus;
};

FlightData currentData;
FlightData dataBuffer[300]; // 30 sec buffer at 10Hz
int bufferIndex = 0;

void setup() {
  Serial.begin(9600);
  while(!Serial); //wait for serial to connect
  Serial.println("Serial started");
  Wire.begin();
  SPI.begin();


  pinMode(MOSFET_PIN, OUTPUT);
  digitalWrite(MOSFET_PIN, LOW);

    // Configure continuity detection
  pinMode(CONTINUITY_OUT, OUTPUT);
  pinMode(CONTINUITY_IN, INPUT);
  digitalWrite(CONTINUITY_OUT, HIGH);  // Send 3.3V test signal
    
  //$$BUG: ICM20948 WILL FAIL WHEN SD CARD MODULE IS PLUGGED IN
  // Initialize sensors
  if (!initializeSensors()) {
    Serial.println("CRITICAL: Sensor initialization failed!");
    while(1) delay(1000); // Halt on sensor failure
  }
  
  // Initialize SD card
  if (!SD.begin(CS_SD)) {
    Serial.println("WARNING: SD card initialization failed!");
  }
  
  Serial.println("Flight computer initialized - ARMED");
  currentData.timeStamp = millis();
}

void loop() {
  unsigned long currentTime = millis();
  
  // Read sensors at specified rate
  if (currentTime - lastSensorRead >= (1000/SENSOR_RATE)) {
    updateFlightData();
    lastSensorRead = currentTime;
    
    // Emulated state machine
    switch (stageNumber) {
      case 1:
        stageOne();  // Idle: Pull pin removal 
        break;
      case 2:
        stageTwo();  // Launch Pad: 2.5 G threshold 
        break;
      case 3:
        stageThree(); // Boost: 4.5 second mach lockout (if speed> .7 mach )
        break;
      case 4: 
        stageFour(); // Coast: Motor has burned, monitoring for apogee
      case 5: 
        stageFive(); // Main : If set through constants, second parachute will deploy at set altitude
      case 6: 
        stageSix(); // Compute flight stats.  TBD
        break;
      default:
        break;
    }
    
    // Emergency backup deployment
    if (flightStartTime > 0 && currentTime - flightStartTime > MAX_FLIGHT_TIME) {
      deployParachute(0);
    }
    
    // Log data at specified rate

    if (currentTime - lastLogWrite >= (1000/LOG_RATE)) {

      logFlightData();
      transmitTelemetry();
      lastLogWrite = currentTime;
    }
  }
}


void stageOne() { // Pre-launch idle
    if (!checkContinuity()) {
        stageNumber = 2;
        Serial.println("Stage 2: ARMED - Pull pin removed!");
    }
}

void stageTwo() { // Potential launch detection
  static unsigned long highGStartTime = 0;
  
  float yAccel = currentData.accel_LR.y; // Y-axis points up
  
  if (yAccel > LAUNCH_G_THRESHOLD) {
    if (highGStartTime == 0) {
      highGStartTime = millis();
    } else if (millis() - highGStartTime > LAUNCH_SUSTAIN_TIME) {
      stageNumber = 3;
      flightStartTime = millis();
      Serial.println("Stage 3: LAUNCH DETECTED!");
      highGStartTime = 0;
    }
  } else {
    highGStartTime = 0; // Reset if acceleration drops
    
    // Return to idle if no sustained acceleration
    if (millis() - currentData.timeStamp > 5000) { // 5 second period to transition back to step 1
      stageNumber = 1; 
  }
}
}

void stageThree() { // Active flight - Boost phase 
  
  if(IS_SONIC){ 
  static unsigned long boostTimer = millis(); 

  if(boostTimer - millis() > (MOTOR_BURN_TIME * 1000)){ 
      stageNumber = 4; 
    }

  }else{ 
    stageNumber = 4; 
  }

}

void stageFour() { 
  
  static float maxAltitude = 0;
  
  if (currentData.altitude > maxAltitude) {
    maxAltitude = currentData.altitude;
  }
  
  if (apogeeReached(currentData.altitude)) {
    deployParachute(MOSFET_PIN);
    stageNumber = 4;
    Serial.println("Stage 4: Parachute deployed at apogee");
  }
}

void stageFive(){ 

  if(MAIN_CHUTE_ALTITUDE){ 
  if (currentData.Altitude <= MAIN_CHUTE_ALTITUDE){   // If statement returing true once altitude is below or equal to pre-set altitude
    deployParachute(MOSFET_PIN2); 
  }
}


}

void stageSix(){ 
// post-processing or Idle state. 

}



//-------------- Sensor & Data Functions --------------

bool initializeSensors() {
  pinMode(CS_ICM20948, OUTPUT); digitalWrite(CS_ICM20948, HIGH); //Sensors should idle HIGH before being initialized
  pinMode(CS_ADXL375,  OUTPUT); digitalWrite(CS_ADXL375,  HIGH);
  pinMode(CS_SD,       OUTPUT); digitalWrite(CS_SD,       HIGH);
  pinMode(CS_BMP390,   OUTPUT); digitalWrite(CS_BMP390,   HIGH);
  bool success = true;
  
  // Initialize ICM20948
  if (!icm.begin_SPI(CS_ICM20948)) {
    Serial.println("Failed to find ICM20948");
    success = false;
  }
  

  // Initialize ADXL375  
  if (!adxl.begin()) {
    Serial.println("Failed to find ADXL375");
    success = false;
  } else {
   // adxl.setRange(ADXL375_RANGE_200_G);
   // adxl.setDataRate(ADXL375_DATARATE_3200_HZ);
  }
  
  // Initialize BMP390
  if (!bmp.begin_SPI(CS_BMP390)) {
    Serial.println("Failed to find BMP390");
    success = false;
  } else {
    bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
    bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
    bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
    bmp.setOutputDataRate(BMP3_ODR_50_HZ);
  }
  
  return success;
}

bool checkContinuity() {
    static int lastContinuityState = -1;
    bool continuity = digitalRead(CONTINUITY_IN) == HIGH;
    if (lastContinuityState != continuity)
        Serial.println(String("Continuity: ") + String(continuity ? "TRUE" : "FALSE"));
    lastContinuityState = continuity;
    return continuity;
}

bool apogeeReached(float altitude) {
    static unsigned long startTime = 0;
    static float startAltitude = 0;
    static bool initialized = false;
    
    const unsigned long DESCENT_DURATION = 500; // 0.5 seconds
    
    if (!initialized) {
        startTime = millis();
        startAltitude = altitude;
        initialized = true;
        return false;
    }
    
    unsigned long currentTime = millis();
    float altitudeChange = altitude - startAltitude;
    
    if (altitudeChange < 0) {
        // We're descending - check if we've been descending for 0.5 seconds
        if (currentTime - startTime >= DESCENT_DURATION) {
            return true; // Apogee reached!
        }
    } else {
        // Altitude increased - reset our descent timer
        startTime = currentTime;
        startAltitude = altitude;
    }
    
    return false;
}

void deployParachute(int pin) {

    digitalWrite(pin, HIGH);
    Serial.println(" PARACHUTE DEPLOYED: " + stageNumber);
    
    delay(1000);            // Send a signal for 1 second to ensure ematch has been ignited. 
    digitalWrite(pin, HIGH);  

  
  }
}

void logFlightData() {
  if (SD.begin(CS_SD)) {
    dataFile = SD.open("flight.csv", FILE_WRITE);
    if (dataFile) {
      // CSV format: timestamp,stage,accel_hr_y,accel_lr_y,altitude,pressure,temp,deployed
      dataFile.print(currentData.timeStamp); dataFile.print(",");
      dataFile.print(currentData.stage); dataFile.print(",");
      dataFile.print(currentData.accel_HR.y, 3); dataFile.print(",");
      dataFile.print(currentData.accel_LR.y, 3); dataFile.print(",");
      dataFile.print(currentData.altitude, 2); dataFile.print(",");
      dataFile.print(currentData.pressure, 2); dataFile.print(",");
      dataFile.print(currentData.temperature, 2); dataFile.print(",");

      dataFile.close();
    }
  }
}


void updateFlightData() {
  currentData.timeStamp = millis();
  currentData.stage = stageNumber;
  
  // Read accelerometer data
  sensors_event_t accel_lr, gyro_data, temp;
  icm.getEvent(&accel_lr, &gyro_data, &temp);
  
  currentData.accel_LR.x = accel_lr.acceleration.x; //In terms of meters instead of G's, since evrything else is in meters
  currentData.accel_LR.y = accel_lr.acceleration.y;
  currentData.accel_LR.z = accel_lr.acceleration.z;
  
  currentData.gyro.x = gyro_data.gyro.x;
  currentData.gyro.y = gyro_data.gyro.y; 
  currentData.gyro.z = gyro_data.gyro.z;
  
  // Read high-G accelerometer
  sensors_event_t adxl_event;
  adxl.getEvent(&adxl_event);
  currentData.accel_HR.x = adxl_event.acceleration.x;
  currentData.accel_HR.y = adxl_event.acceleration.y;
  currentData.accel_HR.z = adxl_event.acceleration.z;
  
  // Read barometric data
  if (bmp.performReading()) {
    currentData.pressure = bmp.pressure / 100.0; // hPa
    currentData.temperature = bmp.temperature;
    currentData.altitude = bmp.readAltitude(1013.25); // meters, adjust sea level pressure
  }
  
  // Store in circular buffer
  dataBuffer[bufferIndex] = currentData;
  bufferIndex = (bufferIndex + 1) % 300;
}

void transmitTelemetry() {
  // Compact telemetry string for radio transmission
  Serial.print("TLM:");
  Serial.print(currentData.timeStamp); Serial.print(",");
  Serial.print(currentData.stage); Serial.print(",");
  Serial.print(currentData.altitude, 1); Serial.print(",");
  Serial.print(currentData.accel_HR.y, 2); Serial.print(",");
}
