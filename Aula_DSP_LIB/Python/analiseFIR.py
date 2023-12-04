import serial
import struct
import matplotlib.pyplot as plt
from time import sleep

# Configurar a porta serial
porta_serial = serial.Serial('COM14', baudrate=115200, timeout=1)

porta_serial.flush()
while True:
    try:
        porta_serial.write(b'0')
        sleep(2)
        data = porta_serial.read_all()
        datadecoded = struct.unpack('640f',data)
        indata = datadecoded[0:320]
        outdata = datadecoded[320:640]
        plt.plot(indata)
        plt.plot(outdata)
        plt.show()

    except Exception as e:
        print('Erro: ', e.args)

# Fechar a porta serial quando terminar
porta_serial.close()
