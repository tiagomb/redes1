// Client side C program to demonstrate Socket
// programming
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <linux/if.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <errno.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <math.h>
#include "conexao.h"
#include "header.h"
#include "buffer.h"
#include "crc.h"

void baixa_video(int soquete, int sequencia, protocolo_t *pacote, char *input){
	envia_buffer(soquete, sequencia++, BAIXAR, input, strlen(input));
	while (pacote->tipo != FIM_TRANSMISSAO){
		switch (recebe_buffer(soquete, pacote)){
			case ACK:
				envia_buffer(soquete, sequencia++, ACK, NULL, 0);
				break;
			case NACK:
				envia_buffer(soquete, sequencia++, NACK, NULL, 0);
				break;
			default:
				break;
		}
	}
}

void recebe_videos(int soquete, int sequencia, protocolo_t *pacote, char *input){
	while (pacote->tipo != FIM_TRANSMISSAO){
		switch (recebe_buffer(soquete, pacote)){
			case ACK:
				envia_buffer(soquete, sequencia++, ACK, NULL, 0);
				printf ("%s\n", pacote->dados);
				break;
			case NACK:
				envia_buffer(soquete, sequencia++, NACK, NULL, 0);
				break;
			default:
				break;
		}
	}
	printf ("Escolha o vídeo que deseja assistir: ");
	scanf ("%s", input);
	baixa_video(soquete, sequencia, pacote, input);
}

void trata_pacote(int soquete, int sequencia, char *input){
	protocolo_t pacote;
	switch (recebe_buffer(soquete, &pacote)){
		case ACK:
			switch (pacote.tipo){
				case LISTA:
					break;
				case BAIXAR:
					break;
				case MOSTRAR:
					envia_buffer(soquete, sequencia++, ACK, NULL, 0);
					printf ("%s\n", pacote.dados);
					recebe_videos(soquete, sequencia, &pacote, input);
					break;
				case DESCRITOR:
					break;
				case DADOS:
					break;
				case FIM_TRANSMISSAO:
					break;
				case ERRO:
					break;
				default:
					break;
			}
			break;
		case NACK:
			envia_buffer(soquete, sequencia++, NACK, NULL, 0);
			break;
		default:
			break;
	}
}

int main(int argc, char const* argv[]){
	int soquete = cria_raw_socket("enp2s0f1");
	int sequencia = 0;
	char input[63];
	while (1){
		trata_pacote(soquete, sequencia, input);
	}
	close(soquete);
	return 0;
}
