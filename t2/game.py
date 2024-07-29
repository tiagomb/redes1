import random
import connection as con
import os

class Card:
    def __init__(self, num, suit, weight):
        self.num = num
        self.suit = suit
        self.weight = weight

    def __repr__(self):
        return f"{self.num} {self.suit}"

class Deck:
    nums = ["4", "5", "6", "7", "Q", "J", "K", "A", "2", "3"]
    suits = ["♦", "♠", "♥", "♣"]
    nums_weights = {num: i for i, num in enumerate(nums)}
    def __init__(self):
        self.cards = [Card(num, suit, self.nums_weights[num]) for num in self.nums for suit in self.suits]

    def shuffle(self):
        random.shuffle(self.cards)
    
    def get_shackle(self):
        shackle = self.cards.pop()
        return shackle
    
    def get_hands(self, num, players):
        return [[self.cards.pop() for _ in range(num)] for _ in range(players)]
        
class Dealer:
    def __init__(self, num, vidas):
        self.deck = Deck()
        self.deck.shuffle()
        self.hands = self.deck.get_hands(num, 4) if num <=13 else self.deck.get_hands(13, 4)
        self.shackle = self.deck.get_shackle()
        self.bets = [0,0,0,0]
        self.points = [0,0,0,0]
        self.lifes = vidas
        self.plays = []
        self.winner = None

    def distribute_hands(self, player, config):
        for i in range(1,4): 
            con.send_data(config, self.hands[(player+i)%4], 'hand', i)
            packet = con.receive_packet(config)
            if packet.confirmation == False:
                print ("Erro: Destino não está na rede")
                con.send_data(config, 'Destino não está na rede', 'erro')
                exit(1)

    def check_winner(self):
        repeated = False
        for i, play in enumerate(self.plays):
            if play[1].weight == self.shackle.weight + 1:
                play[1].weight += 10 + self.deck.suits.index(play[1].suit)
            for j in range(i+1, len(self.plays)):
                if play[1].weight == self.plays[j][1].weight:
                    self.plays[j][1].weight = -1
                    repeated = True
            if repeated:
                play[1].weight = -1
                repeated = False
        self.winner = max(self.plays, key= lambda play: play[1].weight)
        self.winner = self.winner[0] if self.winner[1].weight != -1 else None
        if self.winner != None:
            self.points[self.winner] += 1
            
    def update_lifes(self):
        for i in range(4):
            self.lifes[i] -= abs(self.points[i] - self.bets[i])

class Controller:
    def __init__(self):
        self.cards = []
        self.shackle = None
        self.lifes = [7,7,7,7]
        self.alives = 4
        self.handSize = 1

    def update_shackle(self, shackle):
        print ("Manilha: ", shackle)
        self.shackle = shackle

    def update_hand(self, hand):
        print ("Suas cartas: ", hand)
        self.cards = hand
    
    def bet(self, bets, machine):
        jogadas = int(input("Quantos pontos você faz: "))
        bets[machine] = jogadas

    def show_bets(self, bets):
        print ("Apostas: ", end = "")
        for i, aposta in enumerate(bets):
            print(f"Jogador {i}: {aposta}", end = "  ")
        print()

    def show_plays(self, plays):
        print ("Manilha: ", self.shackle)
        print ("Cartas jogadas: ", end = "")
        for play in plays:
            print (f"Jogador {play[0]}: {play[1]}", end= "  ")
        print()

    def play(self):
        print ("Suas cartas: ", end = "")
        for i, card in enumerate(self.cards):
            print (f"{i}: {card}", end="  ")
        print()
        num = int(input("Digite o número da carta que deseja jogar: "))
        card = self.cards.pop(num)
        return card

    def show_winner(self, winner):
        os.system('clear')
        print ("Vencedor da rodada: ", winner)

    def update_lifes(self, lifes, machine):
        self.lifes = lifes
        print("Vidas: ", lifes[machine])
        if self.handSize < 13:
            self.handSize+=1 
        self.alives = sum(x>0 for x in self.lifes)
