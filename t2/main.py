from game import Deck, Turn, Game, Hand, Dealer
import config as cfg
import connection as con

def main():
    maquina = int(input("Digite o número da máquina(0 a 3): "))
    config = cfg.get_config(maquina)
    rodada = Turn()
    jogo = Game()
    mao = Hand()
    if maquina == 0:
        carteador = Deal(1, 4)
        con.send_data(config, carteador.shackle, 'shackle', (maquina + 3) % 4)
        mao.update_shackle(carteador.shackle)
        mao.update_cards(carteador.hands[maquina], jogo)
    while len(jogo.alives) > 1:
        packet = con.receive_packet(config)
        if jogo.lifes == 0:
            if packet.destiny == maquina:
                packet.confirmation = True
            con.retransmit(packet, config)
        elif packet.origin != maquina and packet.kind != 'token': # Case where you update and sync your info and retransmit the packet
            if packet.kind == 'shackle':
                mao.update_shackle(packet.data)
            elif packet.kind == 'hand':
                if packet.destiny == maquina:
                    mao.update_cards(packet.data, jogo)
            elif packet.kind == 'bet':
                mao.update_bets(packet.data)
            elif packet.kind == 'play':
                rodada.set_card(packet.origin, packet.data)
            elif packet.kind == 'win':
                rodada.check_winner(packet.data, mao, maquina)
            elif packet.kind == 'update':
                jogo.end_round(mao, rodada, maquina, packet.data[1], packet.data[0])
                packet.data[0] = jogo.alives
            elif packet.kind == 'error':
                print ("Erro: ", packet.data)
                con.retransmit(packet, config)
                exit(1)
            if packet.destiny == maquina:
                packet.confirmation = True
            con.retransmit(packet, config)
        elif packet.origin == maquina: # Case where you received your own packet and have the token, usually you just pass the token
            if packet.confirmation == False:
                print ("Erro: Destino não está na rede")
                con.send_data(config, 'Destino não está na rede', 'error', (maquina+3)%4)
                exit(1)
            if packet.kind == 'shackle': # If you have the token and received a shackle, it means you are the dealer and need to distribute the hands
                carteador.distribute_hands(maquina, config, jogo.alives)
                con.send_token(config)
            elif packet.kind == 'update': #Means the hand is over and you need to become the new dealer
                jogo.alives = packet.data[0]
                if maquina == mao.dealer:
                    carteador = Deal(jogo.handSize+1, len(jogo.alives))
                    mao.update_shackle(carteador.shackle)
                    con.send_data(config, mao.shackle, 'shackle', (maquina + 3) % 4)
                    mao.update_cards(carteador.hands[maquina], jogo)
                else:
                    con.send_token(config)
            else:
                con.send_token(config)
        else: # Case where you just received the token and hasn't sent any packet yet
            if mao.bet == None: 
                if len(mao.cards) > 0: # If you haven't bet and have cards, you need to bet
                    mao.place_bet(mao.bet, len(jogo.alives))
                    con.send_data(config, mao.bet, 'bet', (maquina + 3) % 4)
                elif maquina == mao.dealer: # You have no cards and no bet, so either you are the dealer or you are waiting for the dealer to distribute the cards
                    carteador = Deal(jogo.handSize+1, len(jogo.alives))
                    mao.update_shackle(carteador.shackle)
                    con.send_data(config, mao.shackle, 'shackle', (maquina + 3) % 4)
                    mao.update_cards(carteador.hands[maquina], jogo)
                else:
                    con.send_token(config)
            elif len(mao.cards) > 0: 
                if len(rodada.plays) == 0: # If nobody played on this turn, you must be the previous turn winner to start
                    if maquina == rodada.starter:
                        card = rodada.play_card(mao.cards)
                        con.send_data(config, card, 'play', (maquina + 3) % 4)
                        rodada.set_card(maquina, card)
                    else:
                        con.send_token(config)
                if rodada.plays != len(jogo.alives): # Means you didn't play this turn, so you need to play
                    card = rodada.play_card(mao.cards)
                    con.send_data(config, card, 'play', (maquina + 3) % 4)
                    rodada.set_card(maquina, card)
                else: # Everybody played, you need to check the winner
                    rodada.check_winner(rodada.winning, mao, maquina)
                    con.send_data(config, rodada.winning, 'win', (maquina + 3) % 4)
            else: # If you don't have cards, the hand is over and you need to update the game
                jogo.end_round(mao, rodada, maquina, rodada.winning, jogo.alives)
                con.send_data(config, [jogo.alives, rodada.winning], 'update', (maquina + 3) % 4)
    if jogo.lifes == 0:
        print("Você perdeu!")
    else:
        print("Você ganhou!")
                

if __name__ == '__main__':
    main()
