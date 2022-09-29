#!/usr/bin/env python3

import sys
import pandas as pd

def annotate_helper(x, a, b):
	if x['version'] == "DK":
		return 'normal'
	if x['device_id_within_version']==0:
		return a
	return b

def annotate_config(prefix, pvt_df, connection_data_df, device_db):
	device_ids_used = pvt_df.device_id.unique()
	_device_db = device_db[device_db.device_id.isin(device_ids_used)].copy()
	_device_db['device_id_within_version'] =_device_db.groupby('version').cumcount()
	if prefix.endswith("normal"):
		_device_db['config']=_device_db.apply(lambda r: 'normal')
	elif prefix.endswith("upsidedown-big-number"):
		_device_db['config']=_device_db.apply(lambda r: annotate_helper(r, 'normal', 'upsidedown'),axis=1)
	elif prefix.endswith("upsidedown-small-number"):
		_device_db['config']=_device_db.apply(lambda r: annotate_helper(r, 'upsidedown', 'normal'),axis=1)
	elif prefix.endswith('sideways-big-number'):
		_device_db['config']=_device_db.apply(lambda r: annotate_helper(r, 'normal', 'sideways'),axis=1)
	elif prefix.endswith('sideways-small-number'):
		_device_db['config']=_device_db.apply(lambda r: annotate_helper(r, 'sideways', 'normal'),axis=1)
	elif prefix.endswith('usb-big-number'):
		_device_db['config']=_device_db.apply(lambda r: annotate_helper(r, 'normal', 'usb'),axis=1)
	elif prefix.endswith('usb-small-number'):
		_device_db['config']=_device_db.apply(lambda r: annotate_helper(r, 'usb', 'normal'),axis=1)
	connection_data_df = pd.merge(connection_data_df, _device_db).drop(['version', 'device_id_within_version'], axis=1)
	return connection_data_df

if __name__ == '__main__':
	if len(sys.argv) != 4:
		print("needs two prefixes to merge")
	pvt_df = [pd.read_csv('%s-pvt.csv' % prefix) for prefix in sys.argv[1:3]]
	sv_df = [pd.read_csv('%s-sv.csv' % prefix) for prefix in sys.argv[1:3]]
	device_db = pd.read_csv('gnss-test-devices.csv')
	connection_data_df = [pd.read_csv('%s-conn.csv' % prefix) for prefix in sys.argv[1:3]]
	connection_data_df = [df.query('measure_start_ms!=0') for df in connection_data_df]
	connection_data_df = [annotate_config(prefix, pvt, conn, device_db) for prefix, pvt, conn in zip(sys.argv[1:3], pvt_df, connection_data_df)]
	
	measurement_count = [len(df.query('measure_start_ms!=0').measure_start_ms.unique()) for df in connection_data_df]
	print("left set: %d, right set %d" % tuple(measurement_count))
	desired_size = min(*measurement_count)

	# connection_data_df = [df.head(desired_size) for df in connection_data_df]
	for i in range(len(connection_data_df)):
		measurements_to_take = sorted(list(connection_data_df[i].query('measure_start_ms!=0').measure_start_ms.unique()))[:desired_size]
		connection_data_df[i] = connection_data_df[i][connection_data_df[i].apply(lambda x: x['measure_start_ms'] in measurements_to_take, axis=1)]
		pvt_df[i] = pvt_df[i][pvt_df[i].apply(lambda x: x['measure_start_ms'] in measurements_to_take, axis=1)]
		sv_df[i] = sv_df[i][sv_df[i].apply(lambda x: x['measure_start_ms'] in measurements_to_take, axis=1)]

	# filter out devices not present on other side
	devices_used = [set(df.device_id.unique()) for df in connection_data_df]
	device_blacklist = devices_used[0].symmetric_difference(devices_used[1])
	for i in range(len(connection_data_df)):
		connection_data_df[i] = connection_data_df[i][connection_data_df[i].apply(lambda x: x['device_id'] not in device_blacklist, axis=1)]
		pvt_df[i] = pvt_df[i][pvt_df[i].apply(lambda x: x['device_id'] not in device_blacklist, axis=1)]
		sv_df[i] = sv_df[i][sv_df[i].apply(lambda x: x['device_id'] not in device_blacklist, axis=1)]

	pvt_df = pd.concat(pvt_df)
	sv_df = pd.concat(sv_df)
	connection_data_df = pd.concat(connection_data_df)

	for d in device_db.device_id.unique():
		print("device [%s] #PVT: %d #SV: %d" % (d, len(pvt_df[pvt_df.device_id==d]), len(sv_df[sv_df.device_id==d])))

	output_prefix = sys.argv[3]
	pvt_df.to_csv("%s-pvt.csv" % output_prefix)
	sv_df.to_csv("%s-sv.csv" % output_prefix)
	connection_data_df.to_csv("%s-conn.csv" % output_prefix)
