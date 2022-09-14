#!/usr/bin/env python3

import struct
import datetime
import logging

class ConnectionData:
    def __init__(self, raw: bytes) -> None:
        assert(len(raw) == 24)
        self.last_measure, self.band, self.rsrp, self.cellid, self.vbatt = struct.unpack(
            "<qHH10sH", raw)
        self.cellid = self.cellid.decode("ASCII").strip('\x00')

class SV:
    def __init__(self, raw: bytes) -> None:
        assert(len(raw) == 12)
        self.sv, self.signal, self.cn0, self.elevation, self.azimuth, self.flags = struct.unpack(
            "<HBxHhhBx", raw)

    def __repr__(self):
        return "SV: " + str(self.__dict__)


class PVT:
    def decode_time(self) -> None:
        assert(len(self.datetime) == 10)
        year, month, day, hour, minute, seconds, millis =\
            struct.unpack("<HBBBBBxH", self.datetime)
        self.datetime = datetime.datetime(year, month, day, hour,
                                          minute, seconds, microsecond=millis*1000)
        self.datetime = int(self.datetime.timestamp()*1000)

    def decode_svs(self) -> None:
        assert(len(self.svs) == 144)
        self.svs = [SV(self.svs[12*i:12*(i+1)]) for i in range(12)]

    def __repr__(self):
        return "PVT: " + str(self.__dict__)

    def __init__(self, raw: bytes) -> None:
        assert(len(raw) == 232)
        self.latitude,\
            self.longitude,\
            self.altitude,\
            self.accuracy,\
            self.altitude_accuracy,\
            self.speed,\
            self.speed_accuracy,\
            self.vertical_speed,\
            self.vertical_speed_accuracy,\
            self.heading,\
            self.heading_accuracy,\
            self.datetime,\
            self.pdop,\
            self.hdop,\
            self.vdop,\
            self.tdop,\
            self.flags,\
            self.svs,\
            self.execution_time = struct.unpack(
                "<ddfffffffff10sxxffffBx144sxxL", raw)

        try:
            self.decode_time()
        except:
            logging.error("couldn't decode time")
        self.decode_svs()
