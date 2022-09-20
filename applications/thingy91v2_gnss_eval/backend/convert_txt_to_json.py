#!/usr/bin/env python3

from signal import signal
from syslog import LOG_ERR
import paho.mqtt.client as mqtt
import jsonpickle
import time
import signal
import sys
import re
import json
from collections import defaultdict
from pvt_decode import PVT, SV, ConnectionData
import argparse

topic_prefix = "45adb858-13f4-11ed-882f-97c94111fe8a"

pvts = defaultdict(lambda: defaultdict(list))
pvts_index = defaultdict(int)
connection_data_list = []

regex = r"45adb858-13f4-11ed-882f-97c94111fe8a\/(nrf-.*)\/(\w+) (.*)"

if __name__== '__main__':
	parser = argparse.ArgumentParser(
		description='Convert output of aquisition_to_stdout.py to JSON format',
	)
	parser.add_argument(
		'file',
		type=str,
		help='TXT input file',
		metavar='FILE',
	)
	args = parser.parse_args()

	with open(args.file, "r") as file:
		for line in file:
			match = re.search(regex, line)
			if match == None:
				continue
			device_name = match.groups()[0]
			topic = match.groups()[1]
			msg = match.groups()[2]
			try:
				if topic != "data":
					continue
				msg = msg.replace("'", "\"").replace("SV: ", "")
				msg_obj = json.loads(msg)
				if msg.startswith("{\"latitude\""):
					pvt = PVT(b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xbc\x07\x01\x05\x17;*\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00')
					pvt.latitude = msg_obj['latitude']
					pvt.longitude = msg_obj['longitude']
					pvt.altitude = msg_obj['altitude']
					pvt.accuracy = msg_obj['accuracy']
					pvt.altitude_accuracy = msg_obj['altitude_accuracy']
					pvt.speed = msg_obj['speed']
					pvt.speed_accuracy = msg_obj['speed_accuracy']
					pvt.vertical_speed = msg_obj['vertical_speed']
					pvt.vertical_speed_accuracy = msg_obj['vertical_speed_accuracy']
					pvt.heading = msg_obj['heading']
					pvt.heading_accuracy = msg_obj['heading_accuracy']
					pvt.datetime = msg_obj['datetime']
					pvt.pdop = msg_obj['pdop']
					pvt.hdop = msg_obj['hdop']
					pvt.vdop = msg_obj['vdop']
					pvt.tdop = msg_obj['tdop']
					pvt.flags = msg_obj['flags']
					pvt.execution_time = msg_obj['execution_time']
					for i in range(len(msg_obj['svs'])):
						pvt.svs[i].sv = msg_obj['svs'][i]['sv']
						pvt.svs[i].signal = msg_obj['svs'][i]['signal']
						pvt.svs[i].cn0 = msg_obj['svs'][i]['cn0']
						pvt.svs[i].elevation = msg_obj['svs'][i]['elevation']
						pvt.svs[i].azimuth = msg_obj['svs'][i]['azimuth']
						pvt.svs[i].flags = msg_obj['svs'][i]['flags']
					pvts[pvts_index[device_name]][device_name].append(pvt)
				if msg.startswith("{\"last_measure\""):
					connection_data = ConnectionData(b'\xc0\x0chZ\x83\x01\x00\x00\x14\x00\x1f\x00014ACE00\x00\x00\xff\x10')
					connection_data.last_measure = msg_obj['last_measure']
					connection_data.band = msg_obj['band']
					connection_data.rsrp = msg_obj['rsrp']
					connection_data.cellid = msg_obj['cellid']
					connection_data.vbatt = msg_obj['vbatt']
					pvts_index[device_name] = connection_data.last_measure
					connection_data_list.append((device_name, connection_data))
			except Exception as e:
				print("couldn't parse message [%s]" % msg)
				print(e)

	
	json_str = jsonpickle.dumps({
		"pvts":pvts,
		"connection_data": connection_data_list
		})
	with open("gnss_eval-%s.json" % args.file.strip(".txt"), "w") as f:
		f.write(json_str)
	
