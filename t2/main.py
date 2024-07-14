from game import Deck, Round, Player, Hand
import config as cfg
import connection as con

def main():
    maquina = int(input("Digite o número da máquina(0 a 3): "))
    config = cfg.get_config(maquina)
    rodada = Round()
    jogador = Player()
    if maquina == 0:
        mao = Hand(1, 4)
        con.send_data(config, mao.shackle, 'shackle', 3)
        packet = con.receive_packet(config)
        jogador.hand = mao.hands[0]
        jogador.shackle = mao.shackle
        if packet.origin == 0 and packet.confirmation == True:
            for i in range(1, 4):
                con.send_data(config, mao.hands[i], 'hand', i)
                packet = con.receive_packet(config)
                if packet.origin == 0 and packet.confirmation == True:
                    print(f"Recebeu")
        con.send_token(config)
    while jogador.alives > 1:
        packet = con.receive_packet(config)
        if packet.origin != maquina and packet.kind != 'token':
            if packet.kind == 'hand':
                if packet.destiny != maquina:
                    con.retransmit(packet, config)
                else:
                    packet.confirmation = True
                    jogador.hand = packet.data
                    jogador.rodadas = len(jogador.hand)
                    con.retransmit(packet, config)
                    while jogador.rodadas:
                        packet = con.receive_packet(config)
                        if packet.origin != maquina and packet.kind != 'token':
                            retransmit(packet, config)
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
                            print (jogador.bet_sum)             
            elif packet.kind == 'shackle':
                jogador.shackle = packet.data
                if packet.destiny == maquina:
                    packet.confirmation = True
                con.retransmit(packet, config)


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
