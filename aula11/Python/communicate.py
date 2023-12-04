import serial
from time import sleep

class Communicate():
    def __init__(self, port: str, baudrate: int):
        '''
        Claase para realizar a comunicação UART
        :param str port: porta de comunicação uart
        :param int baudrate: baudrate da comunicação
        '''
        # Configurar a porta serial
        try:
            self._serial_port = serial.Serial(port = port, baudrate = baudrate, timeout=1)
            print("Conexão realizada com sucesso!")
        except:
            print(f"Conexão com a porta {port} falhou!")

    def read_int_from_uart(self, num_bytes = 2, byteorder = "little") -> int:
        '''
        Método utilizado para fazer a leitura das informações vindas da UART.
        :param  int num_bytes: numero de bytes da informação
        :param str byteorder: ordem dos bytes recebidos
        '''
        # Obter o número de bytes disponíveis para leitura
        # Obter o número de bytes disponíveis para leitura
        tam_buffer = 1024
        bytes_disponiveis = self._serial_port.in_waiting
        print(f'Bytes Disponiveis: {bytes_disponiveis}')
        if bytes_disponiveis >= tam_buffer * num_bytes:
            # message = int.from_bytes(self._serial_port.read(tam_buffer * num_bytes), byteorder, signed = False)
            datas = memoryview(self._serial_port.read(tam_buffer * num_bytes))
            list_msg = []
            for data in datas:
                list_msg.append(data)
            # print(f"Dado recebido: {message}")
            return list_msg
        else:
            pass

    def send_int_to_uart(self, value: int, num_bytes = 1, byteorder = "little") -> None:
        '''
        Método para fazer o envio de dados para o microcontrolador via UART.
        :param int value: informação a ser enviada.
        :param int num_bytes: numero de bytes utilizado para enviar a informação.
        :param str byteorder: ordem dos bytes a ser enviadas.
        '''
        # Converte o valor inteiro em bytes
        bytes_to_send = value.to_bytes(num_bytes, byteorder, signed=False)
        # Envia os bytes pela porta serial
        try:
            self._serial_port.write(bytes_to_send)
            print(f"Amostra {value} enviado com sucesso!")
            sleep(1)
        except:
            print("Falha ao enviar os dados!")

    def send_signal(self, signal: list) -> None:
        '''
        Método para enviar o sinal que se deseja reproduzir
        '''
        for sample in signal:
            self.send_int_to_uart(sample)
            self.read_int_from_uart()

    def __del__(self):
        # Fechar a porta serial quando terminar
        self._serial_port.close()
        print("Porta serial fechada!")
        pass
