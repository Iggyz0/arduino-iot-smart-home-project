from flask import Flask, render_template, jsonify
import serial
from threading import Thread
import time
from modules import emailCommunication, thingspeak
from datetime import datetime

PORT = "COM1"
BAUD_RATE = 9600
CONTROLLER_ID = 0

allSensors = {
    "readingSensors" : {
        "temperature": {"pin": "A1", "value": 0, "updated": "N/A", "name": "TEMP"}, 
        "photoresistor": {"pin": "A2", "value": 0, "updated": "N/A", "name": "LIGHT"}
    },
    "controlSensors": {
        "fan": {"pin": "5", "value": "0-100"},
        "relay": {"pin": "4", "value": "Nije bitno"},
        "garage": {"pin": "6", "value": "CLOSE/OPEN"}
    },
    "counters": {
        "relay": {"opened": 0, "updated": "N/A"},
        "garage": {"opened": 0, "updated": "N/A"}
    }
}

running = True
serialConnection = serial.Serial(PORT, BAUD_RATE)

def receive(serialConnection):
    global running
    
    while running:
        
        if serialConnection.in_waiting > 0:
            receivedMessage = serialConnection.read_until(b';').decode('ascii')
            #processMessage(receivedMessage)
        time.sleep(0.1)

def processMessage(message):
    global allSensors
    # ODGOVOR : #ARDUINO_ID:SENSOR:VAL;#ARDUINO_ID:SENSOR:VAL;#ARDUINO_ID:SENSOR:VAL;#ARDUINO_ID:SENSOR:VAL;
    # konkretan primer dobijene poruke -> #0:temperature:23.05;#0:photoresistor:6;#0:garage:3;#0:relay:7;
    
    splitList = message.split(";")
    splitList.pop()

    for item in splitList:
        if item[0:1] == "#":
            messageItemProcess(item)
    
    # kada je procitana poruka i kada su upisane vrednosti u globalni objekat, sada treba da posaljemo na thingspeak
    thingspeak.sendDataToThingSpeak(allSensors)

def messageItemProcess(item):
    global allSensors
    temp = item.split(":")
    if temp[1] == "temperature" or temp[1] == "photoresistor":
        allSensors["readingSensors"][temp[1]]["value"] = temp[2]
        allSensors["readingSensors"][temp[1]]["value"] = str(datetime.now())
    elif temp[1] == "garage" or temp[1] == "relay":
        allSensors["counters"][temp[1]]["opened"] = temp[2]
        allSensors["counters"][temp[1]]["updated"] = str(datetime.now())

# MESSAGE FORMAT -> ARDUINO_ID:R/W:PIN:VALUE[:DIR]; (ovo se salje arduinu)
def makeCommand(arduinoId, rw, pin, value):
    return str(arduinoId) + ":" + str(rw) + ":" + str(pin) + ":" + str(value) + ";"

# niti
threadReceiver = Thread(target=receive, args=(serialConnection,))
threadReceiver.start()

emailThread = Thread(target=emailCommunication.sendEmail)
emailThread.start()

app = Flask(__name__)

@app.route('/')
def dashboard():
    global allSensors
    return render_template("dashboard.html", data=allSensors)

@app.route('/execCommand/<rw>/<pin>/<value>', methods=['GET'])
def execCommand(rw, pin, value):
    global CONTROLLER_ID
    cmd = makeCommand(CONTROLLER_ID, rw, pin, value)
    # ovde serial komanda za slanje arduinu
    serialConnection.write(cmd.encode('ascii'))
    return jsonify(isError=False, message="Success", statusCode=200, data_cmd=cmd)

@app.route('/getReadings', methods=['GET'])
def getReadings():
    global allSensors
    return jsonify(isError=False, message="Success", statusCode=200, data_async=allSensors)

if __name__ == "__main__":
    app.run(port=5000, debug=True)