from game import Deck, Round
import config as cfg
from connection import send_data, send_token, receive_packet

def main():
    maquina = int(input("Digite o número da máquina(0 a 3): "))
    config = cfg.get_config(maquina)
    if maquina == 0:
        rodada = Round(1, 4)
        send_data(config, rodada.shackle, 'shackle', 3)
        packet = receive_packet(config)
        print (rodada.hands[0])
        if packet.origin == 0 and packet.confirmation == True:
            for i in range(1, 4):
                send_data(config, rodada.hands[i], 'hand', i)
                packet = receive_packet(config)
                if packet.origin == 0 and packet.confirmation == True:
                    print(f"Recebeu")
        send_token(config)
    while True:
        packet = receive_packet(config)
        if packet.destiny == maquina and packet.kind == 'hand':
            packet.confirmation = True
            send_data(config, packet.data, packet.kind, packet.destiny)
            print(f"Recebeu {packet.data}")
        else:
            send_data(config, packet.data, packet.kind, packet.destiny)
    

if __name__ == '__main__':
    main()