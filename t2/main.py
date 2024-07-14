from game import Deck, Round, Player, Hand
import config as cfg
import connection as con

def play_hand(jogador, config, maquina, rodada):
    while jogador.rounds:
        packet = con.receive_packet(config)
        if packet.origin != maquina and packet.kind != 'token':
            if packet.kind == 'bet':
                jogador.bet_sum += packet.data
                jogador.bet_quantity+=1
            con.retransmit(packet, config)
        elif packet.origin == maquina:
            con.send_token(config)
        else:
            if jogador.bet == None:
                if jogador.bet_quantity == 3:
                    jogador.bet = int(input("Digite a aposta: "))
                    while (jogador.bet + jogador.bet_sum == len(jogador.hand)):
                        jogador.bet = int(input("A soma das apostas não pode ser igual ao número de cartas, escolha novamente: "))
                    jogador.bet_sum += jogador.bet
                else:
                    jogador.bet = int(input("Digite a aposta: "))
                    jogador.bet_sum += jogador.bet
                    jogador.bet_quantity += 1
                con.send_data(config, jogador.bet, 'bet', (maquina + 3) % 4)
            else:
                play = int(input("Selecione a carta que deseja jogar: "))
                rodada.set_card(maquina, jogador.hand[play])


def main():
    maquina = int(input("Digite o número da máquina(0 a 3): "))
    config = cfg.get_config(maquina)
    rodada = Round()
    jogador = Player()
    mao = Hand(1, 4, 0)
    # if maquina == 0:
    #     mao = Hand(1, 4)
    #     con.send_data(config, mao.shackle, 'shackle', 3)
    #     packet = con.receive_packet(config)
    #     jogador.hand = mao.hands[0]
    #     jogador.shackle = mao.shackle
    #     if packet.origin == 0 and packet.confirmation == True:
    #         for i in range(1, 4):
    #             con.send_data(config, mao.hands[i], 'hand', i)
    #             packet = con.receive_packet(config)
    #             if packet.origin == 0 and packet.confirmation == True:
    #                 print(f"Recebeu")
    #     con.send_token(config)
    while jogador.alives > 1:
        if maquina == mao.dealer:
            con.send_data(config, mao.shackle, 'shackle', (maquina + 3) % 4)
            jogador.hand = mao.hands[maquina]
            jogador.rounds = len(jogador.hand)
            packet = con.receive_packet(config)
            if packet.origin == maquina and packet.confirmation == True:
                for i in range(4):
                    if i != maquina:
                        con.send_data(config, mao.hands[i], 'hand', i)
                        packet = con.receive_packet(config)
                        if packet.origin == maquina and packet.confirmation == True:
            con.send_token(config)
            play_hand(jogador, config, maquina, rodada)
        packet = con.receive_packet(config)
        if packet.kind == 'hand':
            if packet.destiny != maquina:
                con.retransmit(packet, config)
            else:
                packet.confirmation = True
                jogador.hand = packet.data
                jogador.rounds = len(jogador.hand)
                con.retransmit(packet, config)
                play_hand(jogador, config, maquina, rodada)            
        else:
            jogador.shackle = packet.data
            if packet.destiny == maquina:
                packet.confirmation = True
            con.retransmit(packet, config)
        print(packet.kind)
        print(packet.data)


    # while True:
    #     packet = con.receive_packet(config)

    #     if packet.destiny == maquina:
    #         packet.confirmation = True
    #         con.retransmit(packet, config)
    #         if packet.kind == 'hand':
    #             print(f"Recebeu {packet.data}")
    #     elif packet.origin != maquina:
    #         con.retransmit(packet, config)
    #     elif packet.origin == maquina and packet.confirmation = False:
    #         con.retransmit(packet, config)
    

if __name__ == '__main__':
    main()
