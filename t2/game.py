import random

class Card:
    def __init__(self, num, suit, weight):
        self.num = num
        self.suit = suit
        self.weight = weight

    def __repr__(self):
        return f"{self.num} {self.suit}"

class Deck:
    nums = ["4", "5", "6", "7", "J", "Q", "K", "A", "2", "3"]
    suits = ["♦", "♠", "♥", "♣"]
    nums_weights = {num: i for i, num in enumerate(nums)}
    def __init__(self):
        self.cards = [Card(num, suit, self.nums_weights[num]) for num in self.nums for suit in self.suits]

    def shuffle(self):
        random.shuffle(self.cards)
    
    def get_shackle(self):
        shackle = self.cards.pop()
        for card in self.cards:
            if card.weight == (shackle.weight+1) % 10:
                card.weight += self.suits.index(card.suit) + 10
        return shackle
    
    def get_hands(self, num, players):
        return [[self.cards.pop() for _ in range(num)] for _ in range(players)]



class Game:
    def __init__(self):
        self.alives = [0, 1, 2, 3]
        self.handSize = 1
        self.lifes = 7

    def end_round(self, hand, round, machine, winner, alives):
        if winner == machine:
            hand.points += 1
        round.plays = 0
        self.lifes -= abs(hand.bet - hand.points)
        self.alives = alives
        hand.reset()
        if self.lifes == 0:
            self.alives.remove(machine)
        print ("Vidas: ", self.lifes)

class Hand:
    def __init__(self):
        self.points = 0
        self.cards = []
        self.shackle = None
        self.bet = None
        self.bet_sum = 0
        self.bet_quantity = 0

    def reset(self):
        self.points = 0
        self.cards = []
        self.shackle = None
        self.bet = None
        self.bet_sum = 0
        self.bet_quantity = 0

    def update_bets(self, bet):
        self.bet_quantity += 1
        self.bet_sum += 
        print ("Apostas até agora: ", self.bet_sum)

    def update_cards(self, hand, game):
        self.cards = hand
        game.handSize = len(self.cards)
        print ("Cartas: ", self.cards)

    def update_shackle(self, shackle):
        self.shackle = shackle
        print ("Manilha: ", self.shackle)

    def place_bet(self, bet, alives):
        self.bet = int(input("Digite a aposta: "))
        if self.bet_quantity == len(alives) - 1:
            while (self.bet + self.bet_sum == len(self.cards)):
                self.bet = int(input("A soma das apostas não pode ser igual ao número de cartas, escolha novamente: "))
        self.bet_sum += self.bet
        self.bet_quantity += 1
        
class Deal:
    def __init__(self, num, players):
        self.deck = Deck()
        self.deck.shuffle()
        self.shackle = self.deck.get_shackle()
        self.hands = self.deck.get_hands(num ,players)

    def distribute_hands(self, player, config, vivos):
        index = vivos.index(player)
        for i in range(index+1, index+len(vivos)):
            con.send_data(config, self.hands[i%len(vivos)], 'hand', vivos[i%len(vivos)])
            packet = con.receive_packet(config)
            if packet.origin == player and packet.confirmation == True:
                print ("recebeu mão")

class Round:
    def __init__(self):
        self.winning_card = None
        self.winning_player = None
        self.plays = 0

    def set_card(self, player, card):
        if self.winning_card is None or card.weight > self.winning_card.weight:
            self.winning_card = card
            self.winning_player = player
        self.plays += 1
        print ("Carta jogada: ", card)

    def check_winner(self, winner, hand, machine):
        if winner = machine:
            hand.points += 1
        print ("Vencedor da rodada: ", winner)
        self.plays = 0

    def play_card(self, cards):
        print ("Suas cartas: ")
        for i, card in enumerate(cards):
            print (f"{i} - {card}")
        card = int(input("Digite o número da carta que deseja jogar: "))
        return cards.pop(card)

        
    def __repr__(self):
        return f"{self.hands} {self.shackle}"


