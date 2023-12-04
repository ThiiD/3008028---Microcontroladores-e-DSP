from realtimeplot import RealTimePlot
import matplotlib.pyplot as plt


freq = 2000
my_signal = [67,  68,  70,  72,  74,  76,  78,  79,  81,  82,  84,
        86,  87,  89,  90,  91,  93,  94,  95,  96,  96,  97,
        98,  98,  99,  99,  99,  99, 100,  99,  99,  99,  99,
        98,  98,  97,  96,  96,  95,  94,  93,  91,  90,  89,
        87,  86,  84,  82,  81,  79,  78,  76,  74,  72,  70,
        68,  67,  65,  62,  61,  59,  57,  55,  54,  52,  51,
        48,  47,  46,  44,  43,  42,  40,  39,  38,  37,  36,
        35,  35,  34,  34,  33,  33,  33,  33,  33,  33,  33,
        34,  34,  35,  35,  36,  37,  38,  39,  40,  42,  43,
        44,  46,  47,  48,  51,  52,  54,  55,  57,  59,  61,
        62,  65]
    

port = 'COM14'
baudrate = 115200
print(len(my_signal))
comm = RealTimePlot(port = port, baudrate = baudrate, pong = False)
comm.getSignal(signal=my_signal)
comm.sendSignal()