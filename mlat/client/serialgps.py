import serial
import pynmea2
import time
import _thread

class MSerialPort:
    def __init__(self,port,buand,coor):
        self.port=serial.Serial(port,buand )
        self.coordinator = coor
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
                        print(line , lon ,lat , alt)
                        self.coordinator.gps_position_update_event(lat,lon,alt)
                except Exception:
                    continue


