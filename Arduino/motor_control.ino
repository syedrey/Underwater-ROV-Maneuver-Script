#include <Servo.h>

Servo motor1;  // Bidirectional ESC requires Servo library
Servo motor2;
Servo motor3;

const int motor1Pin = 9;  // Motor 1 (Bidirectional ESC)
const int motor2Pin = 10; // Motor 2 (Normal ESC)
const int motor3Pin = 11; // Motor 3 (Normal ESC)

int currentMotor1Speed = 0;
int targetMotor1Speed = 0;
int motor2Speed = 1500;  // Neutral for ESC
int motor3Speed = 1500;  // Neutral for ESC

void setup() {
  motor1.attach(motor1Pin);
  motor2.attach(motor2Pin);
  motor3.attach(motor3Pin);

  Serial.begin(9600);
  
  // **Safety: Send Neutral Signal to ESC on Startup**
  motor1.writeMicroseconds(1500);
  motor2.writeMicroseconds(1500);
  motor3.writeMicroseconds(1500);
  delay(2000);  // Wait for ESC to initialize
}

void loop() {
  if (Serial.available() > 0) {
    String data = Serial.readStringUntil('\n'); // Read serial data until newline
    int firstComma = data.indexOf(',');
    int lastComma = data.lastIndexOf(',');

    if (firstComma > 0 && lastComma > firstComma) {
      targetMotor1Speed = data.substring(0, firstComma).toInt();
      motor2Speed = data.substring(firstComma + 1, lastComma).toInt();
      motor3Speed = data.substring(lastComma + 1).toInt();
    }
  }

  // **Smooth Transition for Motor 1**
  if (currentMotor1Speed != targetMotor1Speed) {
    if (currentMotor1Speed < targetMotor1Speed) {
      currentMotor1Speed += 2;
      if (currentMotor1Speed > targetMotor1Speed) currentMotor1Speed = targetMotor1Speed;
    } else {
      currentMotor1Speed -= 2;
      if (currentMotor1Speed < targetMotor1Speed) currentMotor1Speed = targetMotor1Speed;
    }
  }

  // **Mapping Motor 1 to Correct PWM**
  int motor1PWM = map(currentMotor1Speed, -100, 100, 1000, 2000);  
  motor1.writeMicroseconds(motor1PWM); // Use Servo library for precise PWM

  // **Mapping Motor 2 & 3 Correctly**
  int motor2PWM = map(motor2Speed, -100, 100, 1000, 2000);
  int motor3PWM = map(motor3Speed, -100, 100, 1000, 2000);

  motor2.writeMicroseconds(motor2PWM);
  motor3.writeMicroseconds(motor3PWM);

  delay(20);  // Small delay for smooth control
}
