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
#include <sys/statvfs.h>
#include <math.h>
#include <time.h>
#include "conexao.h"
#include "header.h"
#include "buffer.h"

unsigned int sequencia = 31;
unsigned int last_seq = 31;

int verifica_sequencias(protocolo_t pacote, unsigned int last_seq){
	switch (last_seq){
		case 0:
			if (pacote.sequencia == 0 || pacote.sequencia == 31 || pacote.sequencia == 30 || pacote.sequencia == 29 || pacote.sequencia == 28){
				return 1;
			}
			return 0;
			break;
		case 1:
			if (pacote.sequencia == 1 || pacote.sequencia == 0 || pacote.sequencia == 31 || pacote.sequencia == 30 || pacote.sequencia == 29){
				return 1;
			}
			return 0;
			break;
		case 2:
			if (pacote.sequencia == 2 || pacote.sequencia == 1 || pacote.sequencia == 0 || pacote.sequencia == 31 || pacote.sequencia == 30){
				return 1;
			}
			return 0;
			break;
		case 3:
			if (pacote.sequencia == 3 || pacote.sequencia == 2 || pacote.sequencia == 1 || pacote.sequencia == 0 || pacote.sequencia == 31){
				return 1;
			}
			return 0;
			break;
		default:
			if (pacote.sequencia >= last_seq - 4 && pacote.sequencia <= last_seq){
				return 1;
			}
			return 0;
			break;
	}
}

void pergunta_videos(int soquete){
	printf ("Vídeo baixado com sucesso e rodando! Deseja baixar outro vídeo? (s/n): ");
	char c;
	getchar();
	scanf ("%c", &c);
	if (c == 'n' || c == 'N'){
		printf ("Encerrando conexão...\n");
		trata_envio(soquete, &sequencia, FIM_TRANSMISSAO, NULL, 0, &last_seq);
		exit(0);
	} else {
		trata_envio(soquete, &sequencia, LISTA, NULL, 0, &last_seq);
	}
}

void toca_video(int soquete, char *caminho){
	char *comando = (char*) malloc(strlen(caminho) + 10);
	snprintf(comando, strlen(caminho) + 10, "xdg-open %s", caminho);
	system(comando);
	free (caminho);
	free (comando);
	pergunta_videos(soquete);
}

void escreve_arquivo(int soquete, protocolo_t pacote, char *nome, unsigned char *buffer_sequencia){
	char *caminho = (char*) malloc(strlen(nome) + 10);
	unsigned char *buffer = (unsigned char *) malloc(TAMANHO);
	long int tam, data;
	sscanf((char *) pacote.dados, "%ld %ld", &tam, &data);
	struct statvfs stat;
	statvfs(DIRETORIO, &stat);
	if ((tam + 5) * 1000000 > stat.f_bsize * stat.f_bavail){
		snprintf((char *) buffer, TAMANHO, "%d", 3);
		envia_buffer(soquete, inc_seq(&sequencia), ERRO, buffer, strlen((char *) buffer));
		exit(1);
	}
	printf ("Baixando %s no diretório %s\n", nome, DIRETORIO);
	envia_buffer(soquete, inc_seq(&sequencia), ACK, buffer_sequencia, strlen((char *) buffer_sequencia));
	snprintf(caminho, strlen(nome) + 10, "%s/%s", DIRETORIO, nome);
	FILE *arquivo = fopen(caminho, "w");
	int ack = 0, to_send = -1, last_sent = -1, quant = 0;
	while (pacote.tipo != FIM_TRANSMISSAO || !ack){
		while (quant < JANELA){
			switch(recebe_buffer(soquete, &pacote, &last_seq, 1)){
				case ACK:
					ack = 1;
					to_send = pacote.sequencia;
					if (pacote.tipo == DADOS){
						remove_vlan(pacote.dados);
						fwrite(pacote.dados, 1, pacote.tamanho, arquivo);
					} else if (pacote.tipo == FIM_TRANSMISSAO){
						envia_buffer(soquete, inc_seq(&sequencia), ACK, buffer_sequencia, strlen((char *) buffer_sequencia));
						fclose(arquivo);
						free(buffer);
						toca_video(soquete, caminho);
					}
					break;
				case NACK:
					if (!verifica_sequencias(pacote, last_seq)){
						ack = 0;
						to_send = (last_seq + 1) % 32;
					} else {
						ack = 1;
						to_send = pacote.sequencia;
					}
					break;
				case TIMEOUT:
					quant = 0;
					snprintf((char *) buffer_sequencia, TAMANHO, "%d", to_send);
					if (to_send != last_sent){
						last_sent = to_send;
						if (ack) {
							envia_buffer(soquete, inc_seq(&sequencia), ACK, buffer_sequencia, strlen((char *) buffer_sequencia));
						} else {
							envia_buffer(soquete, inc_seq(&sequencia), NACK, buffer_sequencia, strlen((char *) buffer_sequencia));
						}
					}
					break;
				default:
					break;
			}
			quant++;
		}
		snprintf((char *) buffer_sequencia, TAMANHO, "%d", to_send);
		last_sent = to_send;
		if (ack){
			envia_buffer(soquete, inc_seq(&sequencia), ACK, buffer_sequencia, strlen((char *) buffer_sequencia));
		} else {
			envia_buffer(soquete, inc_seq(&sequencia), NACK, buffer_sequencia, strlen((char *) buffer_sequencia));
		}
	}
}

void recebe_videos(int soquete, protocolo_t pacote, unsigned char *input, unsigned char *buffer_sequencia){
	int ack = 0;
	while (pacote.tipo != FIM_TRANSMISSAO || !ack){
		switch (recebe_buffer(soquete, &pacote, &last_seq, 0)){
			case ACK:
				ack = 1;
				snprintf((char *) buffer_sequencia, TAMANHO, "%d", pacote.sequencia);
				envia_buffer(soquete, inc_seq(&sequencia), ACK, buffer_sequencia, strlen((char *) buffer_sequencia));
				printf ("%s\n", pacote.dados);
				break;
			case NACK:
				ack = 0;
				snprintf((char *) buffer_sequencia, TAMANHO, "%d", pacote.sequencia);
				if (pacote.sequencia == last_seq){
					send(soquete, ultimo_enviado, sizeof(protocolo_t), 0);
				} else {
					envia_buffer(soquete, inc_seq(&sequencia), NACK, buffer_sequencia, strlen((char *) buffer_sequencia));
				}
				break;
			default:
				break;
		}
	}
	printf ("Escolha o vídeo que deseja assistir: ");
	scanf ("%s", input);
	memset(&pacote, 0, sizeof(protocolo_t));
	trata_envio(soquete, &sequencia, BAIXAR, input, strlen((char *) input), &last_seq);
}

void trata_pacote(int soquete, unsigned char *input, unsigned char *buffer_sequencia){
	protocolo_t pacote;
	switch (recebe_buffer(soquete, &pacote, &last_seq, 0)){
		case ACK:
			switch (pacote.tipo){
				case MOSTRAR:
					printf ("Vídeos disponíveis no servidor: \n");
					snprintf((char *) buffer_sequencia, TAMANHO, "%d", pacote.sequencia);
					envia_buffer(soquete, inc_seq(&sequencia), ACK, buffer_sequencia, strlen((char *) buffer_sequencia));
					printf ("%s\n", pacote.dados);
					recebe_videos(soquete, pacote, input, buffer_sequencia);
					break;
				case DESCRITOR:
					snprintf((char *) buffer_sequencia, TAMANHO, "%d", pacote.sequencia);
					escreve_arquivo(soquete, pacote, (char *) input, buffer_sequencia);
					break;
				default:
					snprintf((char *) buffer_sequencia, TAMANHO, "%d", 4);
					envia_buffer(soquete, inc_seq(&sequencia), ERRO, buffer_sequencia, strlen((char *) buffer_sequencia));
					exit(1);
					break;
			}
			break;
		case NACK:
			if (pacote.sequencia == last_seq){
				send(soquete, ultimo_enviado, sizeof(protocolo_t), 0);
			} else {
				snprintf((char *) buffer_sequencia, TAMANHO, "%d", pacote.sequencia);
				envia_buffer(soquete, inc_seq(&sequencia), NACK, buffer_sequencia, strlen((char *) buffer_sequencia));
			}
			break;
		default:
			break;
	}
}

int main(int argc, char *argv[]){
	if (argc != 2){
        fprintf(stderr, "Uso: %s <interface de rede>\n", argv[0]);
        exit(1);
    }
    int soquete = cria_raw_socket(argv[1]);
	unsigned char input[TAMANHO], buffer_sequencia[TAMANHO];
	DIR *dir = opendir(DIRETORIO);
	dir ? closedir(dir) : mkdir(DIRETORIO, 0777);
	trata_envio(soquete, &sequencia, LISTA, NULL, 0, &last_seq);
	while (1){
		trata_pacote(soquete, input, buffer_sequencia);
	}
	close(soquete);
	return 0;
}