import serial
import pynmea2
import time
import _thread
from mlat.client.util import monotonic_time

class MSerialPort:
    report_interval = 2s0.0

    def __init__(self,port,buand,coor):
        self.port=serial.Serial(port,baudrate=buand )
        self.coordinator = coor
        self.next_update_time =monotonic_time() + self.report_interval
        if not self.port.isOpen():
            self.port.open()
    def port_open(self):
        if not self.port.isOpen():
            self.port.open()
    def port_close(self):
        self.port.close()

    def read_data(self):
        while True:
            data=self.port.readline()
            line = data.decode()
            if (line.startswith("$GPGGA")):
                try:
                    rmc = pynmea2.parse(line, check=False)
                    lon = rmc.longitude
                    lat = rmc.latitude
                    alt = rmc.altitude
                    if(lon==0 or lat==0):
                        continue
                    if(self.coordinator):
                        #print(line , lon ,lat , alt)
                        now =monotonic_time()
                        if now > self.next_update_time:
                            self.coordinator.gps_position_update_event(lat,lon,alt)
                            self.next_update_time = monotonic_time() + self.report_interval
                except Exception:
                    continue


