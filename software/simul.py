import time
import serial

# configure the serial connections (the parameters differs on the device you are connecting to)
ser = serial.Serial(
    port='COM29',
    baudrate=1200,
    parity=serial.PARITY_ODD,
    stopbits=serial.STOPBITS_ONE,
    bytesize=serial.SEVENBITS
)

ser.isOpen()

ser.write(chr(0x02))
ser.write(chr(0x0A) + "ADCO ABCDEFGHIJKLM" + chr(0x0D))
ser.write(chr(0x0A) + "PAPP 00900" + chr(0x0D))
ser.write(chr(0x03))
ser.write(chr(0x02))
ser.write(chr(0x0A) + "ADCO ABCDEFGHIJKLM" + chr(0x0D))
ser.write(chr(0x0A) + "PAPP 00900" + chr(0x0D))
ser.write(chr(0x03))
ser.write(chr(0x02))
ser.write(chr(0x0A) + "ADCO ABCDEFGHIJKLM" + chr(0x0D))
ser.write(chr(0x0A) + "PAPP 00900" + chr(0x0D))
ser.write(chr(0x03))