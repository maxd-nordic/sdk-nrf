#!/usr/bin/env python3

from signal import signal
import paho.mqtt.client as mqtt
import jsonpickle
import time
import signal
import sys
from collections import defaultdict
from pvt_decode import PVT

topic_prefix = "45adb858-13f4-11ed-882f-97c94111fe8a"

pvts = defaultdict(list)

def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))

    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe(topic_prefix + "/#")


def on_message(client, userdata, msg):
    if len(msg.payload) == 232:
        pvt = PVT(msg.payload)
        device_name = msg.topic.split("/")[1]
        pvts[device_name].append(pvt)
    else:
        print(msg.topic+" "+str(msg.payload))

def on_shutdown(signum, frame):
    json_str = jsonpickle.dumps(pvts)
    with open("gnss_eval-%d.json"%int(time.time()), "w") as f:
        f.write(json_str)
    sys.exit(0)


if __name__ == '__main__':
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect("mqtt.eclipseprojects.io", 1883, 60)
    # client.publish(topic_prefix + "/command", payload=str(int(time.time())+30).encode("ASCII"), qos=0, retain=False)

    signal.signal(signal.SIGINT, on_shutdown)

    client.loop_forever()
