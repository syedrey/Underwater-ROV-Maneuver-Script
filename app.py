from flask import Flask, render_template
from flask_socketio import SocketIO
from flask_cors import CORS
import serial
import time
import threading
import os
import csv  # Added for CSV writing

app = Flask(__name__)
CORS(app)  # Enable CORS for Flask routes
socketio = SocketIO(app, async_mode="gevent", cors_allowed_origins="*")  # Enable gevent

# Initialize serial connection
arduino = serial.Serial('COM3', 230400)  # Change to your Arduino port
time.sleep(2)  # Give time for Arduino to initialize

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/restart', methods=['POST'])
def restart_pi():
    os.system('sudo /sbin/reboot')
    return {"message": "Restarting Raspberry Pi..."}, 200  # Return JSON

@app.route('/shutdown', methods=['POST'])
def shutdown_pi():
    os.system('sudo /sbin/shutdown -h now')
    return {"message": "Shutting down Raspberry Pi..."}, 200  # Return JSON

@app.route('/restart_rov', methods=['POST'])
def restart_rov():
    os.system('sudo systemctl restart rov')
    return {"message": "Restarting ROV service..."}, 200  # Return JSON

@app.route('/restart_mjpg', methods=['POST'])
def restart_mjpg():
    os.system('sudo systemctl restart mjpg')
    return {"message": "Restarting MJPG service..."}, 200  # Return JSON

@socketio.on('joystick_data')
def handle_joystick_data(data):
    motorv_speed = int(data['joystick1']['y'])
    motorv_speed = max(-100, min(100, motorv_speed))

    joystick2_x = int(data['joystick2']['x'])
    joystick2_y = int(data['joystick2']['y'])

    if joystick2_y > 5:  # Forward
        motorh1_speed = joystick2_y
        motorh2_speed = joystick2_y
    elif joystick2_y < -5:  # Reverse
        motorh1_speed = joystick2_y
        motorh2_speed = joystick2_y
    elif joystick2_x < -5:  # Left Turn
        motorh1_speed = joystick2_x
        motorh2_speed = 0.4 * joystick2_x
    elif joystick2_x > 5:  # Right Turn
        motorh1_speed = 0.4 * joystick2_x
        motorh2_speed = joystick2_x
    else:
        motorh1_speed = 0
        motorh2_speed = 0

    arduino.write(f"Motor control: {motorv_speed},{motorh1_speed},{motorh2_speed}\n".encode())

# Read sensor data from Arduino and emit via Socket.IO
def read_serial():
    csv_file = "sensor_data.csv"
    file_exists = os.path.isfile(csv_file)
    while True:
        try:
            line = arduino.readline().decode('utf-8').strip()
            print(f"Raw serial line: {line}")
            
            if line.startswith("Sensor data:"):
                parts = line.split(":")[1].split(",")
                if len(parts) == 4:  # Now expecting 4 values (temp, tds, depth, pH)
                    temperature = float(parts[0])
                    tds = float(parts[1])
                    depth = float(parts[2])
                    ph = float(parts[3])

                    data = {
                        "temperature": temperature,
                        "tds": tds,
                        "depth": depth,
                        "ph": ph  # Added pH value
                    }
                    
                    try:
                        with open(csv_file, "a", newline="") as f:
                            writer = csv.writer(f)
                            if not file_exists:
                                writer.writerow(["timestamp", "temperature", "tds", "depth", "ph"])
                                file_exists = True
                            writer.writerow([time.time(), temperature, tds, depth, ph])
                    except Exception as csv_error:
                        print(f"Error saving sensor data to CSV: {csv_error}")
                    
                    print(f"Parsed sensor data: {data}")
                    socketio.emit("sensor_data", data)
                    print("Emitted sensor data to client")
        except Exception as e:
            print(f"Error reading serial data: {e}")

# Start the serial reading thread
def start_serial_thread():
    serial_thread = threading.Thread(target=read_serial)
    serial_thread.daemon = True  # Daemonize thread to exit when the main program exits
    serial_thread.start()

if __name__ == '__main__':
    print("Starting server...")
    start_serial_thread()
    socketio.run(app, host='0.0.0.0', port=5000, debug=False)  # Running with gevent
