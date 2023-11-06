import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import numpy as np
from communicate import Communicate


class RealTimePlot():
    _fig_width_cm = 18/2.4
    _fig_height_cm = 7/2.4
    _SAMPLE_TIME = 0.1
    _WINDOW_TIME = 10.1
    __i = 0

    def __init__(self, port: str, baudrate: int, pong = False):
        '''
        Classe para fazer o plot em tempo real.
        :param str port: porta de comunicação com o microcontrolador
        :param int baudrate: baudrate da comunicação
        :param bool pong: configura o envio de um sinal para o microcontrolador
        '''
        self._pong = pong
        self._y = []
        self._t = np.arange(0, -self._WINDOW_TIME, -self._SAMPLE_TIME)
        self._fig, self._axs = plt.subplots(figsize=(self._fig_width_cm, self._fig_height_cm), nrows = 1, ncols = 1)
        self._communicate = Communicate(port=port, baudrate=baudrate)

    def initAnimate(self) -> None:
        '''
        Método para iniciar o plot em tempo real.
        '''
        print("teste")
        self._ani = FuncAnimation(self._fig, self._update, interval = 200)
        print("teste 2")
        

    def getSignal(self, signal : list) -> None:
        '''
        Método para pegar o sinal que deve ser reenviado.
        :param list signal: sinal que deve ser reenviado
        '''
        self._signal = signal
        
    def _update(self, fig) -> None:
        print("Teste 3")
        try:
            if self._pong == True:
                print(f'Enviando a amostra {self.__i}: {self._signal[self.__i]}')
                self._communicate.send_int_to_uart(self._signal[self.__i])
                self.__i = (self.__i + 1) % len(self._signal)
            data = self._communicate.read_int_from_uart()
            print(f'Data: {data}')
            self._updateData(data)
            self._updatePlot()  
        except:
            print("Teste")      

    def _updateData(self, y : int) -> None:
        '''
        Método que atualiza o vetor de dados recebidos.
        '''
        self._y.insert(0, y)
        if len(self._y) > len(self._t):
            self._y = self._y[:len(self._t)]
        print(f'y: {self._y}')

    def _updatePlot(self) -> None:
        '''
        Método que atualiza o gráfico em tempo real
        '''
        try:
            self._axs.cla()
            self._axs.plot(self._t[:len(self._y)], self._y, linewidth = 2, color = "tab:blue")
            self._axs.set_xlim([-10,0])
            # self._axs.set_ylim([60, 130])
            self._axs.grid()
            print("teste")
        except:
            print("Erro ao plotar gráfico")

