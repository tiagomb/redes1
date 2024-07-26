import socket
import pickle

class Packet:
    def __init__(self, origin, destiny, data, kind):
        self.origin = origin
        self.destiny = destiny
        self.data = data
        self.kind = kind
        self.confirmation = False

def send_packet(packet, config):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        sock.sendto(pickle.dumps(packet), (config['IP_ENVIO'], int(config['PORTA_ENVIO'])))
    except socket.error as e:
        print(f"Erro ao enviar pacote: {e}")
    finally:
        sock.close()

def receive_packet(config):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        sock.bind((config['IP'], int(config['PORTA_RECIBO'])))
        while True:
            data, addr = sock.recvfrom(1024)
            packet = pickle.loads(data)
            return packet
            break
    except socket.error as e:
        print(f"Erro ao receber pacote: {e}")
    finally:
        sock.close()

def send_data(config, data, kind, offset=3):
    origin = int(config['MAQUINA'])
    destiny = (origin+offset)%4
    packet = Packet(origin, destiny, data, kind)
    send_packet(packet, config)
