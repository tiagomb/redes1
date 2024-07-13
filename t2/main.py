from game import Deck, Round
import config as cfg
import connection as con

def main():
    maquina = int(input("Digite o número da máquina(0 a 3): "))
    config = cfg.get_config(maquina)
    if maquina == 0:
        rodada = Round(1, 4)
        con.send_data(config, rodada.shackle, 'shackle', 3)
        packet = con.receive_packet(config)
        print (rodada.hands[0])
        if packet.origin == 0 and packet.confirmation == True:
            for i in range(1, 4):
                con.send_data(config, rodada.hands[i], 'hand', i)
                packet = con.receive_packet(config)
                if packet.origin == 0 and packet.confirmation == True:
                    print(f"Recebeu")
        con.send_token(config)
    while True:
        packet = con.receive_packet(config)
        if packet.destiny == maquina:
            print ("Entrei\n")
            packet.confirmation = True
            con.retransmit(packet, config)
            if packet.kind == 'hand':
                print(f"Recebeu {packet.data}")
        elif packet.origin != maquina:
            con.retransmit(packet, config)
    

if __name__ == '__main__':
    main()
