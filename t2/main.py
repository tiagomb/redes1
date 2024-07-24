import config as cfg
import connection as con
from game import Dealer
import os

def check_packet_player(packet, machine, cards, lifes, size, dealer):
    if packet.kind == 'hand':
        cartas = packet.data
        print ("Suas cartas: ", packet.data)
    elif packet.kind == 'shackle':
        print ("Manilha: ", packet.data)
    elif packet.kind == 'bet':
        jogadas = int(input("Quantos pontos você faz?"))
        packet.data[machine] = jogadas
    elif packet.kind == 'show':
        print ("Apostas: ")
        for i, aposta in enumerate(packet.data):
            print(f"Jogador {i} apostou que faz {aposta} pontos")
    elif packet.kind == 'play':
        for play in packet.data:
            print (f"Jogador {play[0]} jogou {play[1]}")
        print ("Suas cartas: ")
        for i, card in enumerate(cards):
            print (f"{i} - {card}")
        num = int(input("Digite o número da carta que deseja jogar: "))
        card = cartas.pop(num)
        packet.data.append([machine, card])
    elif packet.kind == 'winner':
        print ("Vencedor da rodada: ", packet.data)
    elif packet.kind == 'update':
        lifes = packet.data
        if size < 13:
            size+=1
    elif packet.kind == 'token':
        dealer = Dealer(size, lifes)
        print ("Suas cartas: ", dealer.hands[0])
        dealer.distribute_hands(0, config)
        print ("Manilha: ", dealer.shackle)
        con.send_data(config, dealer.shackle, 'shackle')
    else:
        print("Erro: ", packet.data)
        con.send_packet(packet, config)
        exit(1)
    if packet.destiny == machine:
        packet.confirmation = True
    con.send_packet(packet, config)

def check_packet_dealer(packet, machine, cards, lifes, size, dealer):
    if packet.confirmation == False:
        con.send_data(config, 'erro', 'error')
        exit(1)
    if packet.kind == 'shackle:
        jogadas = int(input("Quantos pontos você faz?"))
        dealer.bets[machine] = jogadas
        con.send_data(config, dealer.bets, 'bet')
    elif packet.kind == 'bet':
        dealer.bets = packet.data
        print ("Apostas: ")
        for i, aposta in enumerate(packet.data):
            print(f"Jogador {i} apostou que faz {aposta} pontos")
        con.send_data(config, dealer.bets, 'bet')
    elif packet.kind == 'show':
        print ("Suas cartas: ")
        for i, card in enumerate(cards):
            print (f"{i} - {card}")
        num = int(input("Digite o número da carta que deseja jogar: "))
        card = cartas.pop(num)
        dealer.plays.append([machine, card])
        con.send_data(config, dealer.plays, 'play')
    elif packet.kind == 'play':
        carteador.check_winner()
        print ("Vencedor da rodada: ", carteador.winner)
        con.send_data(config, carteador.winner, 'winner')
    elif packet.kind == 'winner':
        if len(cards) > 0:
            dealer.plays = []
            print ("Suas cartas: ")
            for i, card in enumerate(cards):
                print (f"{i} - {card}")
            num = int(input("Digite o número da carta que deseja jogar: "))
            card = cartas.pop(num)
            dealer.plays.append([machine, card])
            con.send_data(config, dealer.plays, 'play')
        else:
            carteador.update_lifes()
            lifes = dealer.lifes
            alives = sum(x>0 for x in lifes)
            if size < 13:
                size+=1
            con.send_data(config, dealer.lifes, 'update')
    else:
        con.send_token()

def main():
    machine = int(input("Digite o número da máquina(0 a 3): "))
    config = cfg.get_config(machine)
    cards = []
    lifes = [7, 7, 7, 7]
    alives = sum(x>0 for x in lifes)
    dealer = None
    size = 1
    if machine == 0:
        dealer = Dealer(1, lifes)
        print ("Suas cartas: ", dealer.hands[0])
        dealer.distribute_hands(0, config)
        print ("Manilha: ", dealer.shackle)
        con.send_data(config, dealer.shackle, 'shackle')
    while alives > 0:
        packet = con.receive_packet(config)
        if lifes[machine] == 0:
            if packet.kind == 'update':
                lifes = packet.data
                alives = sum(x>0 for x in lifes)
            if packet.destiny == machine:
                packet.confirmation = True
            con.send_packet(packet, config)
        elif packet.origin != machine:
            check_packet_player(packet, machine, cards, lifes, size, dealer)
        else:
            check_packet_dealer(packet, machine, cards, lifes, size, dealer)