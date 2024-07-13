import random

class Card:
    def __init__(self, num, init, weight):
        self.num = num
        self.init = init
        self.weight = weight

    def __repr__(self):
        return f"{self.num} {self.suit} {self.weight}"

class Deck:
    nums = ["4", "5", "6", "7", "J", "Q", "K", "A", "2", "3"]
    suits = ["♦", "♠", "♥", "♣"]
    nums_weights = {num: i*5 for i, num in enumerate(nums)}
    suits_weights = {suit: i for i, suit in enumerate(suits)}

    def __init__(self):
        self.cards = [Card(num, suit, self.nums_weights[num] + self.suits_weights[suit]) for num in self.nums for suit in self.suits]

    def shuffle(self):
        random.shuffle(self.cards)
    
    def get_shackle(self):
        shackle = self.cards.pop()
        for card in self.cards:
            if card.weight == shackle.weight+1:
                card.weight += 100
        return shackle
    
    def get_hands(self, num, players):
        return [[self.cards.pop() for _ in range(num)] for _ in range(players)]

class Round:
    def __init__(self, num, players):
        self.deck = Deck()
        self.deck.shuffle()
        self.shackle = self.deck.get_shackle()
        self.hands = self.deck.get_hands(num, players)
        self.winning_card = None
        self.winning_player = None

    def set_card(self, player, card):
        if self.winning_card is None or card.weight > self.winning_card.weight:
            self.winning_card = card
            self.winning_player = player
        self.hands[player].remove(card)
        return self.winning_player
        
    def __repr__(self):
        return f"{self.hands} {self.shackle}"
