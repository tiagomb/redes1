import socket

class Packet:
    def __init__(self, origin, destiny, data, kind):
        self.initmarker = '1111'
        self.origin = origin
        self.destiny = destiny
        self.data = data
        self.kind = kind
        self.confirmation = False
        self.error = False
        self.endmarker = '0000'

def send_packet(packet, config):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        sock.sendto(str(packet).encode(), (config['IP_ENVIO'], int(config['PORTA_ENVIO'])))
    except socket.error as e:
        print(f"Erro ao enviar pacote: {e}")
    finally:
        sock.close()

def receive_packet(config):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        sock.bind((config['IP'], int(config['PORTA_RECIBO'])))
        while True:
            data, addr = socket.recvfrom(1024)
            packet = data.decode()
            return packet
            break
    except socket.error as e:
        print(f"Erro ao receber pacote: {e}")
    finally:
        sock.close()

def send_token(config):
    origin = int(config['MAQUINA'])
    destiny = (origin + 1) % 4
    packet = Packet(origin, destiny, None, 'token')
    send_packet(packet, config)

def send_data(config, data, kind, destiny):
    origin = int(config['MAQUINA'])
    packet = Packet(origin, destiny, data, kind)
    send_packet(packet, config)