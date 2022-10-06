#!/usr/bin/env python3

from signal import signal
import paho.mqtt.client as mqtt
import time
import sys
import signal

topic_prefix = "45adb858-13f4-11ed-882f-97c94111fe8a"
sampling_period = 300
sync_time = 120

def on_shutdown(signum, frame):
    sys.exit(0)

if __name__ == '__main__':
    signal.signal(signal.SIGINT, on_shutdown)
    client = mqtt.Client()
    while True:
        client.connect("test.mosquitto.org", 1883, 60)

        current_time_s = int(time.time())
        print("%d: next sample in %ds" % (current_time_s, sync_time))
        client.publish(topic_prefix + "/command",
                       payload=str(current_time_s+sync_time).encode("ASCII"),
                       qos=0, retain=False)

        client.disconnect()
        time.sleep(sampling_period)
