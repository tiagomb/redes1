import config as cfg
import connection as con
from game import Dealer
import os
   

def main():
    machine = int(input("Digite o número da máquina(0 a 3): "))
    config = cfg.get_config(machine)
    cards = []
    shackle = None
    lifes = [7, 7, 7, 7]
    alives = sum(x>0 for x in lifes)
    dealer = None
    size = 1
    if machine == 0:
        dealer = Dealer(1, lifes)
        cards = dealer.hands[0]
        print ("Suas cartas: ", cards)
        dealer.distribute_hands(0, config)
        shackle = dealer.shackle
        print ("Manilha: ", dealer.shackle)
        con.send_data(config, dealer.shackle, 'shackle')
    while alives > 1:
        packet = con.receive_packet(config)
        if lifes[machine] <= 0: #Você está morto, se estiver com o token, passa adiante, se não, somente se atualiza com os jogadores vivos e retransmite a msg
            if packet.origin == machine:
                con.send_token(config)
            else:
                if packet.kind == 'update':
                    lifes = packet.data
                    alives = sum(x>0 for x in lifes)
                if packet.destiny == machine:
                    packet.confirmation = True
                con.send_packet(packet, config)
        elif packet.origin != machine: #Você não é o carteador, somente leia/atualize as mensagens
            if packet.kind == 'hand':
                if packet.destiny == machine:
                    for card in packet.data:
                        cards.append(card)
                    print ("Suas cartas: ", cards)
            elif packet.kind == 'shackle':
                shackle = packet.data
                print ("Manilha: ", packet.data)
            elif packet.kind == 'bet':
                jogadas = int(input("Quantos pontos você faz: "))
                packet.data[machine] = jogadas
            elif packet.kind == 'show':
                print ("Apostas: ", end = "")
                for i, aposta in enumerate(packet.data):
                    print(f"Jogador {i}: {aposta}", end = "  ")
                print()
            elif packet.kind == 'play':
                print ("Manilha: ", shackle)
                print ("Cartas jogadas: ", end = "")
                for play in packet.data:
                    print (f"Jogador {play[0]}: {play[1]}", end= "  ")
                print()
                print ("Suas cartas: ", end = "")
                for i, card in enumerate(cards):
                    print (f"{i}: {card}", end="  ")
                print()
                num = int(input("Digite o número da carta que deseja jogar: "))
                card = cards.pop(num)
                packet.data.append([machine, card])
            elif packet.kind == 'winner':
                os.system('clear')
                print ("Vencedor da rodada: ", packet.data)
            elif packet.kind == 'update':
                lifes = packet.data
                print("Vidas:" , lifes[machine])
                if size < 13:
                    size+=1
            elif packet.kind == 'token':
                dealer = Dealer(size, lifes)
                cards = dealer.hands[machine]
                print ("Suas cartas: ", dealer.hands[machine])
                dealer.distribute_hands(machine, config)
                print ("Manilha: ", dealer.shackle)
                con.send_data(config, dealer.shackle, 'shackle')
            else:
                print("Erro: ", packet.data)
                con.send_packet(packet, config)
                exit(1)
            if packet.destiny == machine:
                packet.confirmation = True
            con.send_packet(packet, config)
        else: #Você é o carteador, gere as mensagens certas e mande-as
            if packet.confirmation == False:
                con.send_data(config, 'erro', 'error')
                exit(1)
            if packet.kind == 'shackle':
                jogadas = int(input("Quantos pontos você faz: "))
                dealer.bets[machine] = jogadas
                con.send_data(config, dealer.bets, 'bet')
            elif packet.kind == 'bet':
                dealer.bets = packet.data
                print ("Apostas: ", end = "")
                for i, aposta in enumerate(packet.data):
                    print(f"Jogador {i}: {aposta}", end = "  ")
                print()
                con.send_data(config, dealer.bets, 'show')
            elif packet.kind == 'show':
                print ("Manilha: ", shackle)
                print ("Suas cartas: ", end = "")
                for i, card in enumerate(cards):
                    print (f"{i}: {card}", end="  ")
                print()
                num = int(input("Digite o número da carta que deseja jogar: "))
                card = cards.pop(num)
                dealer.plays.append([machine, card])
                con.send_data(config, dealer.plays, 'play')
            elif packet.kind == 'play':
                dealer.plays = packet.data
                dealer.check_winner()
                os.system('clear')
                print ("Vencedor da rodada: ", dealer.winner)
                con.send_data(config, dealer.winner, 'winner')
            elif packet.kind == 'winner':
                if len(cards) > 0:
                    dealer.plays = []
                    print ("Manilha: ", shackle)
                    print ("Suas cartas: ", end = "")
                    for i, card in enumerate(cards):
                        print (f"{i}: {card}", end="  ")
                    print()
                    num = int(input("Digite o número da carta que deseja jogar: "))
                    card = cards.pop(num)
                    dealer.plays.append([machine, card])
                    con.send_data(config, dealer.plays, 'play')
                else:
                    dealer.update_lifes()
                    lifes = dealer.lifes
                    print("Vidas: ", lifes[machine])
                    alives = sum(x>0 for x in lifes)
                    if size < 13:
                        size+=1
                    con.send_data(config, dealer.lifes, 'update')
            else:
                con.send_token(config)
    if lifes[machine] > 0:
        print ("Você ganhou!")
    else:
        print ("Você perdeu!")

if __name__ == '__main__':
    main()
