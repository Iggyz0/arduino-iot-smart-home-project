import urllib.request
import requests

WRITE_KEY = ""
READ_KEY = ""

def sendDataToThingSpeak(allSensors):
    temp = float(allSensors["readingSensors"]["temperature"]["value"])
    light = int(allSensors["readingSensors"]["photoresistor"]["value"])
    relay = int(allSensors["counters"]["relay"]["opened"])
    garage = int(allSensors["counters"]["garage"]["opened"])

    urllib.request.urlopen('https://api.thingspeak.com/update?api_key={}&field1={}&field2={}&field3={}&field4={}'.format(WRITE_KEY, temp, light, garage, relay))


def readDataFromThingSpeak():
    global READ_KEY
    CHANNEL_ID = 0

    res = requests.get("https://api.thingspeak.com/channels/{}/feeds.json?api_key={}".format(CHANNEL_ID, READ_KEY))

    #data = res.json()
    #print(data['feeds'])
    
    return res.json()