import config as cfg
import connection as con
from game import Dealer, Controller
import os
   
def main():
    machine = int(input("Digite o número da máquina(0 a 3): "))
    config = cfg.get_config(machine)
    controller = Controller(machine)
    dealer = None
    if machine == 0:
        dealer = Dealer(1, controller.lifes)
        controller.update_hand(dealer.hands[0])
        dealer.distribute_hands(0, config)
        controller.update_shackle(dealer.shackle)
        con.send_data(config, dealer.shackle, 'shackle')
    while controller.alives > 1:
        packet = con.receive_packet(config)
        if controller.lifes[machine] <= 0: #Você está morto, se estiver com o token, passa adiante, se não, somente se atualiza com os jogadores vivos e retransmite a msg
            if packet.origin == machine:
                con.send_data(config, None, 'token')
            else:
                if packet.kind == 'update':
                    controller.update_lifes(packet.data)
                if packet.destiny == machine:
                    packet.confirmation = True
                con.send_packet(packet, config)
        elif packet.origin != machine: #Você não é o carteador, somente leia/atualize as mensagens
            if packet.kind == 'hand':
                if packet.destiny == machine:
                    controller.update_hand(packet.data)
            elif packet.kind == 'shackle':
                controller.update_shackle(packet.data)
            elif packet.kind == 'bet':
                controller.bet(packet.data)
            elif packet.kind == 'show':
                controller.show_bets(packet.data)
            elif packet.kind == 'play':
                controller.show_plays(packet.data)
                card = controller.play()
                packet.data.append([machine, card])
            elif packet.kind == 'winner':
                controller.show_winner(packet.data)
            elif packet.kind == 'update':
                controller.update_lifes(packet.data)
            elif packet.kind == 'token':
                dealer = Dealer(controller.handSize, controller.lifes)
                controller.update_hand(dealer.hands[machine])
                dealer.distribute_hands(machine, config)
                controller.update_shackle(dealer.shackle)
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
                controller.bet(dealer.bets, machine)
                con.send_data(config, dealer.bets, 'bet')
            elif packet.kind == 'bet':
                dealer.bets = packet.data
                controller.show_bets(dealer.bets)
                con.send_data(config, dealer.bets, 'show')
            elif packet.kind == 'show':
                print ("Manilha: ", controller.shackle)
                card = controller.play()
                dealer.plays.append([machine, card])
                con.send_data(config, dealer.plays, 'play')
            elif packet.kind == 'play':
                dealer.plays = packet.data
                dealer.check_winner()
                controller.show_winner(dealer.winner)
                con.send_data(config, dealer.winner, 'winner')
            elif packet.kind == 'winner':
                if len(controller.cards) > 0:
                    dealer.plays = []
                    print ("Manilha: ", controller.shackle)
                    card = controller.play()
                    dealer.plays.append([machine, card])
                    con.send_data(config, dealer.plays, 'play')
                else:
                    dealer.update_lifes()
                    controller.update_lifes(dealer.lifes, machine)
                    con.send_data(config, dealer.lifes, 'update')
            else:
                con.send_data(config, None, 'token')
    if controller.lifes[machine] > 0:
        print ("Você ganhou!")
    else:
        print ("Você perdeu!")

if __name__ == '__main__':
    main()
