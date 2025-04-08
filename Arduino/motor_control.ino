#include <Servo.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "GravityTDS.h"

Servo motorA;
Servo motorB;
Servo motorC;

const int motorAPin = 9;
const int motorBPin = 11;
const int motorCPin = 12;

int currentMotorASpeed = 0, targetMotorASpeed = 0;
int currentMotorBSpeed = 0, targetMotorBSpeed = 0;
int currentMotorCSpeed = 0, targetMotorCSpeed = 0;

// Sensor pins
#define TdsSensorPin A1
#define pressureSensorPin A0
#define temperatureSensorPin 4
#define pHsensorPin A2

// DS18B20 setup
OneWire oneWire(temperatureSensorPin);
DallasTemperature sensors(&oneWire);

// TDS sensor setup
GravityTDS gravityTds;
float tdsValue = 0;
float temperature = 16;

// Pressure sensor calibration
const float OffSet = 0.483;

// pH sensor calibration
const float calibration_value = 24.43;

// Timing variables
unsigned long previousSensorReadTime = 0;
const unsigned long sensorReadInterval = 1000;

// Motor control constants
const int MOTOR_MIN = 1000;
const int MOTOR_MAX = 2000;
const int MOTOR_NEUTRAL = 1500;
const int MOTOR_DEADBAND = 10; // ± value where motor should be neutral
const int MAX_RAMP_STEP = 20; // Maximum allowed change in motor speed per cycle
unsigned long lastCommandTime = 0;
const unsigned long SAFETY_TIMEOUT = 1000; // Stop motors if no commands for 1 second

void smoothTransition(int *currentSpeed, int targetSpeed) {
    const int maxStep = 5; // Maximum change per cycle (adjust for responsiveness)
    const int deadzone = 2; // Minimum change threshold
    
    int difference = targetSpeed - *currentSpeed;
    
    // Apply deadzone - don't make tiny adjustments
    if (abs(difference) <= deadzone) {
        *currentSpeed = targetSpeed;
        return;
    }
    
    // Calculate step size (proportional control)
    int step = constrain(abs(difference)/4, 1, maxStep);
    
    // Apply the step
    *currentSpeed += (difference > 0) ? step : -step;
    
    // Ensure we don't overshoot
    if ((difference > 0 && *currentSpeed > targetSpeed) || 
        (difference < 0 && *currentSpeed < targetSpeed)) {
        *currentSpeed = targetSpeed;
    }
}

void writeMotorWithDeadband(Servo &motor, int speed) {
    // Apply deadband around neutral
    if (abs(speed) <= MOTOR_DEADBAND) {
        motor.writeMicroseconds(MOTOR_NEUTRAL);
        return;
    }
    
    // Constrain speed to valid range
    speed = constrain(speed, -100, 100);
    
    // Map to microseconds with proper bounds checking
    int us = map(speed, -100, 100, MOTOR_MIN, MOTOR_MAX);
    us = constrain(us, MOTOR_MIN, MOTOR_MAX);
    
    motor.writeMicroseconds(us);
}

void applyRampLimits() {
    int deltaA = targetMotorASpeed - currentMotorASpeed;
    int deltaB = targetMotorBSpeed - currentMotorBSpeed;
    int deltaC = targetMotorCSpeed - currentMotorCSpeed;
    
    if (abs(deltaA) > MAX_RAMP_STEP) {
        targetMotorASpeed = currentMotorASpeed + (deltaA > 0 ? MAX_RAMP_STEP : -MAX_RAMP_STEP);
    }
    if (abs(deltaB) > MAX_RAMP_STEP) {
        targetMotorBSpeed = currentMotorBSpeed + (deltaB > 0 ? MAX_RAMP_STEP : -MAX_RAMP_STEP);
    }
    if (abs(deltaC) > MAX_RAMP_STEP) {
        targetMotorCSpeed = currentMotorCSpeed + (deltaC > 0 ? MAX_RAMP_STEP : -MAX_RAMP_STEP);
    }
}

void setup() {
  // Initialize motor pins properly
  pinMode(motorAPin, OUTPUT);
  pinMode(motorBPin, OUTPUT);
  pinMode(motorCPin, OUTPUT);
  digitalWrite(motorAPin, LOW);
  digitalWrite(motorBPin, LOW);
  digitalWrite(motorCPin, LOW);
  delay(100);
  
  motorA.attach(motorAPin);
  motorB.attach(motorBPin);
  motorC.attach(motorCPin);
  motorA.writeMicroseconds(MOTOR_NEUTRAL);
  motorB.writeMicroseconds(MOTOR_NEUTRAL);
  motorC.writeMicroseconds(MOTOR_NEUTRAL);
  delay(2000); // ESC initialization

  Serial.begin(230400);

  sensors.begin();
  gravityTds.setPin(TdsSensorPin);
  gravityTds.setAref(5.0);
  gravityTds.setAdcRange(1024);
  gravityTds.begin();
}

void loop() {
  static unsigned long lastMotorUpdate = 0;
  static unsigned long lastSensorStage = 0;
  static byte sensorStage = 0;
  
  // Motor control (runs every 20ms max)
  if (millis() - lastMotorUpdate >= 20) {
    lastMotorUpdate = millis();
    
    if (Serial.available() > 0) {
      lastCommandTime = millis();
      String data = Serial.readStringUntil('\n');
      if (data.startsWith("Motor control: ")) {
        data.remove(0, 14);
        int firstComma = data.indexOf(',');
        int secondComma = data.indexOf(',', firstComma + 1);
        if (firstComma > 0 && secondComma > firstComma) {
          targetMotorASpeed = constrain(data.substring(0, firstComma).toInt(), -100, 100);
          targetMotorBSpeed = constrain(data.substring(firstComma + 1, secondComma).toInt(), -100, 100);
          targetMotorCSpeed = constrain(data.substring(secondComma + 1).toInt(), -100, 100);
        }
      }
    }

  // Safety timeout
  if (millis() - lastCommandTime > SAFETY_TIMEOUT) {
    targetMotorASpeed = 0;
    targetMotorBSpeed = 0;
    targetMotorCSpeed = 0;
  }

  applyRampLimits();
  smoothTransition(&currentMotorASpeed, targetMotorASpeed);
  smoothTransition(&currentMotorBSpeed, targetMotorBSpeed);
  smoothTransition(&currentMotorCSpeed, targetMotorCSpeed);

  writeMotorWithDeadband(motorA, currentMotorASpeed);
  writeMotorWithDeadband(motorB, currentMotorBSpeed);
  writeMotorWithDeadband(motorC, currentMotorCSpeed);

  if (millis() - previousSensorReadTime >= sensorReadInterval) {
    previousSensorReadTime = millis();

    sensors.requestTemperatures();
    temperature = sensors.getTempCByIndex(0);
    gravityTds.setTemperature(temperature);
    gravityTds.update();
    tdsValue = gravityTds.getTdsValue();

    int pressureValue = analogRead(pressureSensorPin);
    float voltage = pressureValue * (5.0 / 1023.0);
    float pressureKPa = (voltage - OffSet) * 400;
    float pressurePa = pressureKPa * 1000;
    float depth = pressurePa / (1000 * 9.81);

    // pH Sensor Reading
    int buffer_arr[10];
    for (int i = 0; i < 10; i++) {
      buffer_arr[i] = analogRead(pHsensorPin);
      delay(10);
    }

    for (int i = 0; i < 9; i++) {
      for (int j = i + 1; j < 10; j++) {
        if (buffer_arr[i] > buffer_arr[j]) {
          int temp = buffer_arr[i];
          buffer_arr[i] = buffer_arr[j];
          buffer_arr[j] = temp;
        }
      }
    }

    unsigned long avgval = 0;
    for (int i = 2; i < 8; i++) avgval += buffer_arr[i];
    float phVolt = (float)avgval * (5.0 / 1024.0) / 6;
    float phValue = -4.90 * phVolt + calibration_value;

    Serial.print("Sensor data:");
    Serial.print(temperature);
    Serial.print(",");
    Serial.print(tdsValue);
    Serial.print(",");
    Serial.print(depth);
    Serial.print(",");
    Serial.println(phValue);
  }
}  
}
