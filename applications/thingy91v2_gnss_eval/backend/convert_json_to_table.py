#!/usr/bin/env python3

import jsonpickle
import pandas as pd
import argparse

if __name__== '__main__':
	parser = argparse.ArgumentParser(
		description='Convert output of backend.py to a tabular format',
	)
	parser.add_argument(
		'file',
		type=str,
		help='JSON input file',
		metavar='FILE',
	)
	parser.add_argument(
		'--format',
		type=str,
		default='csv',
		choices=['csv', 'hdf5', 'pkl', 'parquet'],
		help='output format',
		metavar='FORMAT',
	)
	args = parser.parse_args()

	with open(args.file, "r") as f:
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

	if args.format == 'hdf5':
		# export dataframes to hdf5
		output_filename = args.file.strip(".json") + ".hdf5"
		pvt_df.to_hdf(output_filename, "pvt")
		sv_df.to_hdf(output_filename, "sv")
		connection_data_df.to_hdf(output_filename, "connection_data")
	elif args.format == 'csv':
		# export dataframes to CSV
		output_prefix = args.file.strip(".json")
		pvt_df.to_csv("%s-pvt.csv" % output_prefix)
		sv_df.to_csv("%s-sv.csv" % output_prefix)
		connection_data_df.to_csv("%s-conn.csv" % output_prefix)
	elif args.format == 'pkl':
		# export dataframes to pickle 4
		output_prefix = args.file.strip(".json")
		pvt_df.to_pickle("%s-pvt.pkl" % output_prefix, protocol=4)
		sv_df.to_pickle("%s-sv.pkl" % output_prefix, protocol=4)
		connection_data_df.to_pickle("%s-conn.pkl" % output_prefix, protocol=4)
	elif args.format == 'parquet':
		# export dataframes to parquet
		output_prefix = args.file.strip(".json")
		pvt_df.to_parquet("%s-pvt.gzip" % output_prefix, compression='gzip')
		sv_df.to_parquet("%s-sv.gzip" % output_prefix, compression='gzip')
		connection_data_df.to_parquet("%s-conn.gzip" % output_prefix, compression='gzip')

