from flask import Flask, render_template
from flask_socketio import SocketIO
import serial

app = Flask(__name__)
socketio = SocketIO(app)

arduino = serial.Serial('COM5', 9600)  # Change to your Arduino port

@app.route('/')
def index():
    return render_template('index.html')

@socketio.on('joystick_data')
def handle_joystick_data(data):
    # **Motor 1 (Bidirectional ESC - Proper Mapping)**
    motor1_speed = int(data['joystick1']['y'])  # Range: -100 to 100
    motor1_speed = max(-100, min(100, motor1_speed))  # Clamping to valid range

    # **Motor 2 and Motor 3 logic remains the same**
    if data['joystick2']['x'] < 0:
        motor2_speed = int((data['joystick2']['x'] + 100) * 1.275)
        motor2_speed = max(0, min(255, motor2_speed))
        motor3_speed = 0
    else:
        motor3_speed = int((data['joystick2']['x'] + 100) * 1.275)
        motor3_speed = max(0, min(255, motor3_speed))
        motor2_speed = 0

    # Send corrected values to Arduino
    arduino.write(f"{motor1_speed},{motor2_speed},{motor3_speed}\n".encode())

if __name__ == '__main__':
    socketio.run(app, host='0.0.0.0', port=5000)
