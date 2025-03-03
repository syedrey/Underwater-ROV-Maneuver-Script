#include <Servo.h>

// **Motor Setup**
Servo motor1;  // Bidirectional ESC
Servo motor2;
Servo motor3;

const int motor1Pin = 9;  // Motor 1 (Bidirectional ESC)
const int motor2Pin = 10; // Motor 2 (Normal ESC)
const int motor3Pin = 11; // Motor 3 (Normal ESC)

int currentMotor1Speed = 0;
int targetMotor1Speed = 0;
int motor2Speed = 1500;  // Neutral for ESC
int motor3Speed = 1500;  // Neutral for ESC

// **Sensor Pins**
const int depthSensorPin = A0;
const int tdsSensorPin = A1;
const int tempSensorPin = 4;  // Digital pin

void setup() {
  Serial.begin(115200);  // Higher baud rate for smooth operation
  
  motor1.attach(motor1Pin);
  motor2.attach(motor2Pin);
  motor3.attach(motor3Pin);

  // **Send Neutral Signal on Startup**
  motor1.writeMicroseconds(1500);
  motor2.writeMicroseconds(1500);
  motor3.writeMicroseconds(1500);
  delay(2000);  // Allow ESCs to initialize
}

void loop() {
  // **Read Sensor Data**
  float depth = readDepthSensor();
  float tds = readTDSSensor();
  float temperature = readTemperatureSensor();

  // **Send Sensor Data via Serial**
  Serial.print(millis());  // Timestamp
  Serial.print(",");
  Serial.print(depth, 2);
  Serial.print(",");
  Serial.print(tds, 2);
  Serial.print(",");
  Serial.println(temperature, 2);

  // **Read Motor Control Commands from Serial**
  if (Serial.available() > 0) {
    String data = Serial.readStringUntil('\n'); // Read until newline
    int firstComma = data.indexOf(',');
    int lastComma = data.lastIndexOf(',');

    if (firstComma > 0 && lastComma > firstComma) {
      targetMotor1Speed = data.substring(0, firstComma).toInt();
      motor2Speed = data.substring(firstComma + 1, lastComma).toInt();
      motor3Speed = data.substring(lastComma + 1).toInt();
    }
  }

  // **Smooth Transition for Motor 1**
  if (abs(currentMotor1Speed - targetMotor1Speed) > 2) {  // Deadband to prevent small fluctuations
    if (currentMotor1Speed < targetMotor1Speed) {
      currentMotor1Speed += 2;
    } else {
      currentMotor1Speed -= 2;
    }
  }

  // **Mapping Motor 1 to Correct PWM**
  int motor1PWM = 1500;  // Default neutral
  if (abs(targetMotor1Speed) > 5) {  // Zero-movement zone
    motor1PWM = map(targetMotor1Speed, -100, 100, 1000, 2000);
  }
  motor1.writeMicroseconds(motor1PWM);

  // **Mapping Motor 2 & 3 Correctly**
  int motor2PWM = map(motor2Speed, -100, 100, 1000, 2000);
  int motor3PWM = map(motor3Speed, -100, 100, 1000, 2000);

  motor2.writeMicroseconds(motor2PWM);
  motor3.writeMicroseconds(motor3PWM);

  delay(20);  // Small delay for smooth control
}

// **Sensor Reading Functions**
float readDepthSensor() {
  int rawValue = analogRead(depthSensorPin);
  return (rawValue / 1023.0) * 1.6;  // Convert to MPa
}

float readTDSSensor() {
  int rawValue = analogRead(tdsSensorPin);
  return (rawValue / 1023.0) * 1000;  // Convert to ppm
}

float readTemperatureSensor() {
  // Dummy function, replace with DS18B20 temperature sensor code if needed
  return 25.0; // Default temperature (replace with actual sensor reading)
}
