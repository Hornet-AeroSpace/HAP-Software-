
#include "Arduino.h"
#include <string>
#include <Wire.h>
#include "Adafruit_ICM20948.h"
#include "Adafruit_ICM20X.h"
#include <Adafruit_Sensor.h>
#include "Adafruit_BMP3XX.h"
#include "Adafruit_ADXL375.h"
#include <SPI.h>
#include <SD.h>

// Pin Definitions
#define SPI_MOSI    23
#define SPI_MISO    19    
#define SPI_CLK     18
#define CS_ICM20948  4
#define CS_ADXL375  16
#define CS_SD       17
#define SDA_PIN     21
#define SCL_PIN     22
#define MOSFET_PIN  32

#define CONTINUITY_OUT  25    // Output pin (sends signal)
#define CONTINUITY_IN   26    // Input pin (receives signal)

// Flight Parameters
#define LAUNCH_G_THRESHOLD 3.5      // G-force to detect launch
#define MAX_FLIGHT_TIME    70000    // 70 sec backup deployment (ms)
#define LAUNCH_SUSTAIN_TIME 2000    // 2 sec sustained G for launch detect
#define SENSOR_RATE 100             // 100Hz sensor sampling
#define LOG_RATE 10                 // 10Hz data logging

int stageNumber = 1;
unsigned long flightStartTime = 0;
unsigned long lastSensorRead = 0;
unsigned long lastLogWrite = 0;
bool deploymentTriggered = false;

// Hardware Objects
Adafruit_ICM20948 icm;
Adafruit_ADXL375 adxl;
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
  Wire.begin(SDA_PIN, SCL_PIN);
  SPI.begin(SPI_CLK, SPI_MISO, SPI_MOSI);
  
  pinMode(MOSFET_PIN, OUTPUT);
  digitalWrite(MOSFET_PIN, LOW);
 
    
    // Configure continuity detection
    pinMode(CONTINUITY_OUT, OUTPUT);
    pinMode(CONTINUITY_IN, INPUT_PULLDOWN);
    digitalWrite(CONTINUITY_OUT, HIGH);  // Send 3.3V test signal
    
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
    
    // Stage state machine
    switch (stageNumber) {
      case 1:
        stageOne();
        break;
      case 2:
        stageTwo();
        break;
      case 3:
        stageThree();
        break;
      case 4: 
        stageFour();
        break;
      default:
        break;
    }
    
    // Emergency backup deployment
    if (flightStartTime > 0 && currentTime - flightStartTime > MAX_FLIGHT_TIME) {
      deployParachute("BACKUP_TIMEOUT");
    }
    
    // Log data at specified rate
    if (currentTime - lastLogWrite >= (1000/LOG_RATE)) {
      logFlightData();
      transmitTelemetry();
      lastLogWrite = currentTime;
    }
  }
}




bool checkContinuity() {
    static int lastContinuityState = -1;
    bool continuity = digitalRead(CONTINUITY_IN) == HIGH;
    if (lastContinuityState != continuity)
        Serial.println(String("Continuity: ") + String(continuity ? "TRUE" : "FALSE"));
    lastContinuityState = continuity;
    return continuity;
}

void stageOne() { // Pre-launch idle
    if (!checkContinuity()) {
        stageNumber = 2;
        Serial.println("Stage 2: ARMED - Pull pin removed!");
    }
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

void deployParachute(String reason) {
  if (!deploymentTriggered) {
    digitalWrite(MOSFET_PIN, HIGH);
    deploymentTriggered = true;
    Serial.println("PARACHUTE DEPLOYED: " + reason);
    
    // Keep deployment signal for 2 seconds
    delay(2000);
    digitalWrite(MOSFET_PIN, LOW);
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
      dataFile.println(deploymentTriggered);
      dataFile.close();
    }
  }
}

void transmitTelemetry() {
  // Compact telemetry string for radio transmission
  Serial.print("TLM:");
  Serial.print(currentData.timeStamp); Serial.print(",");
  Serial.print(currentData.stage); Serial.print(",");
  Serial.print(currentData.altitude, 1); Serial.print(",");
  Serial.print(currentData.accel_HR.y, 2); Serial.print(",");
  Serial.println(deploymentTriggered);
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
    if (millis() - currentData.timeStamp > 10000) { // 10 sec timeout
      stageNumber = 1;
    }
  }
}

void stageThree() { // Active flight - apogee detection
  static float maxAltitude = 0;
  
  if (currentData.altitude > maxAltitude) {
    maxAltitude = currentData.altitude;
  }
  
  if (apogeeReached(currentData.altitude)) {
    deployParachute("APOGEE_DETECTED");
    stageNumber = 4;
    Serial.println("Stage 4: Parachute deployed at apogee");
  }
}

void stageFour() { // Post-deployment descent
  // Continue logging and telemetry
  // Could add secondary chute deployment logic here
}

//-------------- Sensor & Data Functions --------------

bool initializeSensors() {
  bool success = true;
  
  // Initialize ICM20948
  if (!icm.begin_SPI(CS_ICM20948)) {
    Serial.println("Failed to find ICM20948");
    success = false;
  }
  
  // Initialize ADXL375  
  if (!adxl.begin(CS_ADXL375)) {
    Serial.println("Failed to find ADXL375");
    success = false;
  } else {
    adxl.setRange(ADXL375_RANGE_200_G);
    adxl.setDataRate(ADXL375_DATARATE_3200_HZ);
  }
  
  // Initialize BMP390
  if (!bmp.begin_I2C()) {
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

void updateFlightData() {
  currentData.timeStamp = millis();
  currentData.stage = stageNumber;
  currentData.deploymentStatus = deploymentTriggered;
  
  // Read accelerometer data
  sensors_event_t accel_lr, gyro_data, temp;
  icm.getEvent(&accel_lr, &gyro_data, &temp);
  
  currentData.accel_LR.x = accel_lr.acceleration.x / 9.81; // Convert m/s² to g
  currentData.accel_LR.y = accel_lr.acceleration.y / 9.81;
  currentData.accel_LR.z = accel_lr.acceleration.z / 9.81;
  
  currentData.gyro.x = gyro_data.gyro.x;
  currentData.gyro.y = gyro_data.gyro.y; 
  currentData.gyro.z = gyro_data.gyro.z;
  
  // Read high-G accelerometer
  sensors_event_t adxl_event;
  adxl.getEvent(&adxl_event);
  currentData.accel_HR.x = adxl_event.acceleration.x / 9.81;
  currentData.accel_HR.y = adxl_event.acceleration.y / 9.81;
  currentData.accel_HR.z = adxl_event.acceleration.z / 9.81;
  
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
}