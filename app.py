from flask import Flask, render_template
from flask_socketio import SocketIO
import serial

app = Flask(__name__)
socketio = SocketIO(app)

arduino = serial.Serial('/dev/arduino', 115200)  # Change to your Arduino port

@app.route('/')
def index():
    return render_template('index.html')

@socketio.on('joystick_data')
def handle_joystick_data(data):
    # **Motor 1 (Bidirectional ESC - Proper Mapping)**
    motor1_speed = int(data['joystick1']['y'])  # Range: -100 to 100
    motor1_speed = max(-100, min(100, motor1_speed))  # Clamping to valid range

    motor2_speed = 0
    motor3_speed = 0

    # **Fixing Motor 2 & 3 behavior**
    if data['joystick2']['y'] > 10:  # Forward
        motor2_speed = data['joystick2']['y']
        motor3_speed = data['joystick2']['y']
    elif data['joystick2']['y'] < -10:  # Reverse
        motor2_speed = data['joystick2']['y']
        motor3_speed = data['joystick2']['y']
    elif data['joystick2']['x'] < -10:  # Left Turn
        motor2_speed = 100   # Full Forward
        motor3_speed = -100  # Full Reverse
    elif data['joystick2']['x'] > 10:  # Right Turn
        motor2_speed = -100  # Full Reverse
        motor3_speed = 100   # Full Forward
    else:
        motor2_speed = 0
        motor3_speed = 0  # Reset motors to neutral when joystick is centered

    # Send corrected values to Arduino
    arduino.write(f"{motor1_speed},{motor2_speed},{motor3_speed}\n".encode())

if __name__ == '__main__':
    socketio.run(app, host='0.0.0.0', port=5000)
