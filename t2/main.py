from game import Deck, Round, Game, Hand, Deal
import config as cfg
import connection as con

# def play_hand(jogador, config, maquina, rodada):
#     while jogador.rounds:
#         packet = con.receive_packet(config)
#         if packet.origin != maquina and packet.kind != 'token':
#             if packet.kind == 'bet':
#                 jogador.bet_sum += packet.data
#                 jogador.bet_quantity+=1
#             con.retransmit(packet, config)
#         elif packet.origin == maquina:
#             con.send_token(config)
#         else:
#             if jogador.bet == None:
#                 if jogador.bet_quantity == 3:
#                     jogador.bet = int(input("Digite a aposta: "))
#                     while (jogador.bet + jogador.bet_sum == len(jogador.hand)):
#                         jogador.bet = int(input("A soma das apostas não pode ser igual ao número de cartas, escolha novamente: "))
#                     jogador.bet_sum += jogador.bet
#                 else:
#                     jogador.bet = int(input("Digite a aposta: "))
#                     jogador.bet_sum += jogador.bet
#                     jogador.bet_quantity += 1
#                 con.send_data(config, jogador.bet, 'bet', (maquina + 3) % 4)
#             else:
#                 play = int(input("Selecione a carta que deseja jogar: "))
#                 rodada.set_card(maquina, jogador.hand[play])


# def main():
#     maquina = int(input("Digite o número da máquina(0 a 3): "))
#     config = cfg.get_config(maquina)
#     rodada = Round()
#     jogador = Player()
#     mao = Hand(1, 4, 0)
#     while jogador.alives > 1:
#         if maquina == mao.dealer:
#             con.send_data(config, mao.shackle, 'shackle', (maquina + 3) % 4)
#             jogador.hand = mao.hands[maquina]
#             jogador.rounds = len(jogador.hand)
#             packet = con.receive_packet(config)
#             if packet.origin == maquina and packet.confirmation == True:
#                 for i in range(4):
#                     if i != maquina:
#                         con.send_data(config, mao.hands[i], 'hand', i)
#                         packet = con.receive_packet(config)
#                         if packet.origin == maquina and packet.confirmation == True:
#             con.send_token(config)
#             play_hand(jogador, config, maquina, rodada)
#         packet = con.receive_packet(config)
#         if packet.kind == 'hand':
#             if packet.destiny != maquina:
#                 con.retransmit(packet, config)
#             else:
#                 packet.confirmation = True
#                 jogador.hand = packet.data
#                 jogador.rounds = len(jogador.hand)
#                 con.retransmit(packet, config)
#                 play_hand(jogador, config, maquina, rodada)            
#         else:
#             jogador.shackle = packet.data
#             if packet.destiny == maquina:
#                 packet.confirmation = True
#             con.retransmit(packet, config)
#         print(packet.kind)
#         print(packet.data)

def distribute_hands(maquina, config, carteador, vivos):
    for i in vivos:
        if i != maquina:
            con.send_data(config, carteador.hands[i], 'hand', i)
            packet = con.receive_packet(config)
            if packet.origin == maquina and packet.confirmation == True:
                print ("recebeu mão")

def reset_hand(mao):
    mao.points = 0
    mao.cards = []
    mao.shackle = None
    mao.bet = None
    mao.bet_sum = 0
    mao.bet_quantity = 0

def main():
    maquina = int(input("Digite o número da máquina(0 a 3): "))
    config = cfg.get_config(maquina)
    rodada = Round()
    jogo = Game()
    mao = Hand()
    if maquina == 0:
        carteador = Deal(1, 4)
        con.send_data(config, carteador.shackle, 'shackle', (maquina + 3) % 4)
        mao.shackle = carteador.shackle
        print("Shackle: ", mao.shackle)
        mao.cards = carteador.hands[maquina]
        print("Cartas: ", mao.cards)
    while len(jogo.alives) > 1:
        packet = con.receive_packet(config)
        if jogo.lifes == 0:
            if packet.destiny == maquina:
                packet.confirmation = True
            con.retransmit(packet, config)
        elif packet.origin != maquina and packet.kind != 'token':
            if packet.kind == 'shackle':
                mao.shackle = packet.data
                print ("Shackle: ", mao.shackle)
            elif packet.kind == 'hand':
                if packet.destiny == maquina:
                    mao.cards = packet.data
                    jogo.handSize = len(mao.cards)
                    print("Cartas: ", mao.cards)
            elif packet.kind == 'bet':
                mao.bet_sum += packet.data
                mao.bet_quantity+=1
                print ("Apostas até agora: ", mao.bet_sum)
            elif packet.kind == 'play':
                rodada.set_card(packet.origin, packet.data)
                rodada.plays += 1
                print ("Carta jogada: ", packet.data)
            elif packet.kind == 'win':
                if packet.data == maquina:
                    mao.points += 1
                print ("Vencedor da rodada: ", packet.data)
                rodada.plays = 0
            elif packet.kind == 'update':
                if packet.data[1] == maquina:
                    mao.points += 1
                rodada.plays = 0
                jogo.lifes -= abs(mao.bet - mao.points)
                jogo.alives = packet.data[0]
                reset_hand(mao)
                if jogo.lifes == 0:
                    jogo.alives.remove(maquina)
                packet.data[0] = jogo.alives
                print ("Vidas: ", jogo.lifes)
            if packet.destiny == maquina:
                packet.confirmation = True
            con.retransmit(packet, config)
        elif packet.origin == maquina:
            if packet.kind == 'shackle':
                distribute_hands(maquina, config, mao)
            elif packet.kind == 'update':
                jogo.alives = packet.data[0]
                if len(jogo.alives) > 1:
                    if jogo.handSize == 7:
                        carteador = Deal(jogo.handSize, len(jogo.alives))
                    else:
                        carteador = Deal(jogo.handSize+1, len(jogo.alives))
                    con.send_data(config, mao.shackle, 'shackle', (maquina + 3) % 4)
                    mao.cards = carteador.hands[maquina]
                    jogo.handSize = mao.handSize
                    distribute_hands(maquina, config, mao, jogo.alives)
            con.send_token(config)
        else:
            if mao.bet == None:
                if mao.bet_quantity == mao.alives - 1:
                    mao.bet = int(input("Digite a aposta: "))
                    while (mao.bet + mao.bet_sum == len(mao.cards)):
                        mao.bet = int(input("A soma das apostas não pode ser igual ao número de cartas, escolha novamente: "))
                    mao.bet_sum += mao.bet
                else:
                    mao.bet = int(input("Digite a aposta: "))
                    mao.bet_sum += mao.bet
                    mao.bet_quantity += 1
                con.send_data(config, mao.bet, 'bet', (maquina + 3) % 4)
            elif len(mao.cards) > 0:
                if rodada.plays != jogo.alives:
                    play = int(input("Selecione a carta que deseja jogar: "))
                    con.send_data(config, jogador.hand[play], 'card', (maquina + 3) % 4)
                    rodada.set_card(maquina, mao.cards[play])
                    rodada.plays += 1
                else:
                    if rodada.winning_player == maquina:
                        mao.points += 1
                    rodada.plays = 0
                    con.send_data(config, rodada.winning_player, 'win', (maquina + 3) % 4)
            else:
                if rodada.winning_player == maquina:
                    mao.points += 1
                rodada.plays = 0
                jogo.lifes -= abs(mao.bet - mao.points)
                reset_hand(mao)
                if jogo.lifes == 0:
                    jogo.alives.remove(maquina)
                con.send_data(config, [jogo.alives, rodada.winning_player], 'update', (maquina + 3) % 4)
    if jogo.lifes == 0:
        print("Você perdeu!")
    else:
        print("Você ganhou!")
                

if __name__ == '__main__':
    main()
