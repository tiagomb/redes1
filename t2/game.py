import random
import connection as con

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
        self.message = 'FODA-SE'

    def end_hand(self, hand, turn, machine, winner, alives):
        if winner == machine:
            hand.points += 1
        turn.plays = []
        turn.winning = None
        self.lifes -= abs(hand.bet - hand.points)
        self.alives = alives
        hand.end()
        turn.starter = (hand.dealer + 1) % 4
        if self.lifes == 0:
            self.alives.remove(machine)
        print ("Vidas: ", self.message[:lifes])

class Hand:
    def __init__(self):
        self.points = 0
        self.cards = []
        self.shackle = None
        self.bet = None
        self.bet_sum = 0
        self.bet_quantity = 0
        self.dealer = 0

    def end(self):
        self.points = 0
        self.cards = []
        self.shackle = None
        self.bet = None
        self.bet_sum = 0
        self.bet_quantity = 0
        self.dealer = 3 if self.dealer == 0 else self.dealer - 1

    def update_bets(self, bet):
        self.bet_quantity += 1
        self.bet_sum += bet

    def update_cards(self, hand, game):
        self.cards = hand
        game.handSize = len(self.cards)
        print ("Cartas: ", self.cards)

    def update_shackle(self, shackle):
        self.shackle = shackle
        print ("Manilha: ", self.shackle)

    def place_bet(self, bet, alives):
        print ("Apostas até agora: ", self.bet_sum)
        self.bet = int(input("Digite a aposta: "))
        if self.bet_quantity == alives - 1:
            while (self.bet + self.bet_sum == len(self.cards)):
                self.bet = int(input("A soma das apostas não pode ser igual ao número de cartas, escolha novamente: "))
        self.bet_sum += self.bet
        self.bet_quantity += 1
        
class Dealer:
    def __init__(self, num, players):
        self.deck = Deck()
        self.deck.shuffle()
        self.shackle = self.deck.get_shackle()
        self.hands = self.deck.get_hands(num,players) if num <=13 else self.deck.get_hands(13, players)

    def distribute_hands(self, player, config, vivos):
        index = vivos.index(player)
        for i in range(index+1, index+len(vivos)):
            con.send_data(config, self.hands[i%len(vivos)], 'hand', vivos[i%len(vivos)])
            packet = con.receive_packet(config)
            if packet.confirmation == False:
                print ("Erro: Destino não está na rede")
                con.send_data(config, 'Destino não está na rede', 'erro', (player+3)%4)
                exit(1)

class Turn:
    def __init__(self):
        self.winning = None
        self.starter = 1
        self.plays = []

    def set_card(self, player, card):
        for play in plays:
            if card.weight == play[0].weight:
                card.weight = play[0].weight = -1
        self.plays.append((card, player))
        self.plays.sort(key = lambda play: play[0].weight, reverse = True)
        self.winning = plays[0][1] if plays[0][0].weight != -1 else None

    def check_winner(self, winner, hand, machine):
        if winner == machine:
            hand.points += 1
        self.starter = winner
        print ("Vencedor da rodada: ", winner)
        self.plays = []
        self.winning = None

    def play_card(self, cards):
        print ("Cartas jogadas até agora: ")
        for play in plays:
            print (f"Jogador {play[1]} jogou {play[0]}")
        print ("Suas cartas: ")
        for i, card in enumerate(cards):
            print (f"{i} - {card}")
        card = int(input("Digite o número da carta que deseja jogar: "))
        return cards.pop(card)

        

