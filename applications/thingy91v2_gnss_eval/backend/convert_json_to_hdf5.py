#!/usr/bin/env python3

from pvt_decode import PVT, ConnectionData
from collections import defaultdict
import jsonpickle
import sys
import pandas as pd

if __name__== '__main__':
	if len(sys.argv) != 2:
		sys.exit(1)
	input_filename = sys.argv[1]

	with open(input_filename, "r") as f:
		input_str = f.read()
	input_data = jsonpickle.loads(input_str)

	sv_columns = [
		# indices to get to the individual sv entry
		"device_id", "measure_start_ms", "pvt_idx", "sv_idx",
		# actual sv entry data
		"sv", "signal", "cn0", "elevation", "azimuth", "flags"]
	sv_list = []

	pvt_columns = [
		# indices to get to the individual pvt entry
		"device_id", "measure_start_ms", "pvt_idx",
		# actual pvt entry data (without svs)
		"latitude",
		"longitude",
		"altitude",
		"accuracy",
		"altitude_accuracy",
		"speed",
		"speed_accuracy",
		"vertical_speed",
		"vertical_speed_accuracy",
		"heading",
		"heading_accuracy",
		"datetime_ms",
		"pdop",
		"hdop",
		"vdop",
		"tdop",
		"flags",
		"execution_time"
	]

	pvt_list = []

	# read out all the data rows
	for measure_id, devs in input_data['pvts'].items():
		for device_id, pvts in devs.items():
			for pvt_idx, pvt in enumerate(pvts):
				pvt_list.append([
					device_id, measure_id, pvt_idx,
					pvt.latitude,
					pvt.longitude,
					pvt.altitude,
					pvt.accuracy,
					pvt.altitude_accuracy,
					pvt.speed,
					pvt.speed_accuracy,
					pvt.vertical_speed,
					pvt.vertical_speed_accuracy,
					pvt.heading,
					pvt.heading_accuracy,
					pvt.datetime,
					pvt.pdop,
					pvt.hdop,
					pvt.vdop,
					pvt.tdop,
					pvt.flags,
					pvt.execution_time
				])
				for sv_idx, sv in enumerate(pvt.svs):
					sv_list.append([
						device_id, measure_id, pvt_idx, sv_idx,
						sv.sv, sv.signal, sv.cn0,
						sv.elevation, sv.azimuth, sv.flags
					])

	# create dataframes
	pvt_df = pd.DataFrame(data=pvt_list, columns=pvt_columns)
	sv_df = pd.DataFrame(data=sv_list, columns=sv_columns)
	connection_data_df = pd.DataFrame(
		data=[[device_id, c.last_measure, c.band, c.rsrp, c.cellid, c.vbatt]
			for device_id, c in input_data['connection_data']],
		columns=["device_id", "measure_start_ms", "band", "rsrp", "cell_id", "vbatt"])

	# export dataframes
	output_filename = input_filename.strip(".json") + ".hdf5"
	pvt_df.to_hdf(output_filename, "pvt")
	sv_df.to_hdf(output_filename, "sv")
	connection_data_df.to_hdf(output_filename, "connection_data")


