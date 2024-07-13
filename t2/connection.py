import socket

class Packet:
    def __init__(self, origin, destiny, data):
        self.initmarker = '1111'
        self.origin = origin
        self.destiny = destiny
        self.data = data
        self.confirmation = False
        self.error = False
        self.endmarker = '0000'

