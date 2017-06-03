import pynmea2
import serial

from mlat.client.util import monotonic_time


class MSerialPort:
    report_interval = 20.0

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
            try:
                data=self.port.readline()
                line = data.decode()
                if (line.startswith("$GPGGA")):
                    rmc = pynmea2.parse(line, check=False)
                    lon = rmc.longitude
                    lat = rmc.latitude
                    alt = rmc.altitude
                    if(lon==0 or lat==0):
                        continue
                    if(self.coordinator):
                        now =monotonic_time()
                        if now > self.next_update_time:
                            self.coordinator.gps_position_update_event(lat,lon,alt)
                            self.next_update_time = monotonic_time() + self.report_interval
            except Exception as e:
                print("Error:Read GPS Serial Port Data Failed! Stop Read!!! ==>" ,e)
                break


