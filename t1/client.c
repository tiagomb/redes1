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
#include "crc.h"


unsigned int sequencia = 31;
unsigned int last_seq = 31;

void escreve_arquivo(int soquete, protocolo_t pacote, char *nome){
	char *caminho = (char*) malloc(strlen(nome) + 10);
	char *comando = (char*) malloc(strlen(nome) + 20);
	unsigned char *buffer = malloc(63);
	long int tam, data;
	int removidos;
	sscanf((char *) pacote.dados, "%ld %ld", &tam, &data);
	struct statvfs stat;
	statvfs("./videos", &stat);
	if ((tam + 5) * 1000000 > stat.f_bsize * stat.f_bavail){
		printf ("%ld\n", tam);
		snprintf((char *) buffer, 63, "%d", 3);
		envia_buffer(soquete, inc_seq(&sequencia), ERRO, buffer, strlen((char *) buffer), &last_seq);
		exit(1);
	}
	envia_buffer(soquete, inc_seq(&sequencia), ACK, NULL, 0, &last_seq);
	snprintf(caminho, strlen(nome) + 10, "./videos/%s", nome);
	FILE *arquivo = fopen(caminho, "w");
	while (pacote.tipo != FIM_TRANSMISSAO){
		switch (recebe_buffer(soquete, &pacote, &last_seq)){
			case ACK:
				if (pacote.tipo == DADOS){
					removidos = remove_vlan(pacote.dados);
					fwrite(pacote.dados, 1, pacote.tamanho - removidos, arquivo);
					envia_buffer(soquete, inc_seq(&sequencia), ACK, NULL, 0, &last_seq);
				} else if (pacote.tipo == FIM_TRANSMISSAO){
					envia_buffer(soquete, inc_seq(&sequencia), ACK, NULL, 0, &last_seq);
					fclose(arquivo);
					snprintf(comando, strlen(caminho) + 10, "xdg-open %s", caminho);
					system(comando);
					printf ("Deseja baixar outro vídeo? (s/n): ");
					char c;
					getchar();
					scanf ("%c", &c);
					if (c == 'n' || c == 'N'){
						envia_buffer(soquete, inc_seq(&sequencia), FIM_TRANSMISSAO, NULL, 0, &last_seq);
						int aceito = recebe_confirmacao(soquete, &last_seq);
						switch (aceito){
							case ACK:
								break;
							case NACK:
								while (aceito == NACK){
									envia_buffer(soquete, sequencia, FIM_TRANSMISSAO, NULL, 0, &last_seq);
									aceito = recebe_confirmacao(soquete, &last_seq);
								}
								break;
							case TIMEOUT:
								envia_buffer(soquete, sequencia, FIM_TRANSMISSAO, NULL, 0, &last_seq);
								aceito = recebe_confirmacao(soquete, &last_seq);
								break;
							default:
								break;
						}
						exit(0);
					} else {
						envia_buffer(soquete, inc_seq(&sequencia), LISTA, NULL, 0, &last_seq);
						int aceito = recebe_confirmacao(soquete, &last_seq);
						switch (aceito){
							case ACK:
								break;
							case NACK:
								while (aceito == NACK){
									envia_buffer(soquete, sequencia, LISTA, NULL, 0, &last_seq);
									aceito = recebe_confirmacao(soquete, &last_seq);
								}
								break;
							case TIMEOUT:
								envia_buffer(soquete, sequencia, LISTA, NULL, 0, &last_seq);
								aceito = recebe_confirmacao(soquete, &last_seq);
								break;
							default:
								break;
						}
					}
				}
				break;
			case NACK:
				if (pacote.sequencia == last_seq){
					send(soquete, ultimo_enviado, sizeof(protocolo_t), 0);
				} else {
					envia_buffer(soquete, inc_seq(&sequencia), NACK, NULL, 0, &last_seq);
				}
				break;
			default:
				break;
		}
	}
}

void baixa_video(int soquete, protocolo_t pacote, unsigned char *input){
	envia_buffer(soquete, inc_seq(&sequencia), BAIXAR, input, strlen((char *) input), &last_seq);
	int aceito = recebe_confirmacao(soquete, &last_seq);
	switch (aceito){
		case ACK:
			break;
		case NACK:
			while (aceito == NACK){
				envia_buffer(soquete, sequencia, BAIXAR, input, strlen((char *) input), &last_seq);
				aceito = recebe_confirmacao(soquete, &last_seq);
			}
			break;
		case TIMEOUT:
			envia_buffer(soquete, sequencia, BAIXAR, input, strlen((char *) input), &last_seq);
			aceito = recebe_confirmacao(soquete, &last_seq);
			break;
		default:
			break;
	}
	switch (recebe_buffer(soquete, &pacote, &last_seq)){
		case ACK:
			if (pacote.tipo == DESCRITOR){
				escreve_arquivo(soquete, pacote, (char *) input);
			} else {
				envia_buffer(soquete, inc_seq(&sequencia), NACK, NULL, 0, &last_seq);
			}
			break;
		case NACK:
			if (pacote.sequencia == last_seq){
				send(soquete, ultimo_enviado, sizeof(protocolo_t), 0);
			} else {
				envia_buffer(soquete, inc_seq(&sequencia), NACK, NULL, 0, &last_seq);
			}
			break;
		default:
			break;
		}
}

void recebe_videos(int soquete, protocolo_t pacote, unsigned char *input){
	while (pacote.tipo != FIM_TRANSMISSAO){
		switch (recebe_buffer(soquete, &pacote, &last_seq)){
			case ACK:
				envia_buffer(soquete, inc_seq(&sequencia), ACK, NULL, 0, &last_seq);
				printf ("%s\n", pacote.dados);
				break;
			case NACK:
				if (pacote.sequencia == last_seq){
					send(soquete, ultimo_enviado, sizeof(protocolo_t), 0);
				} else {
					envia_buffer(soquete, inc_seq(&sequencia), NACK, NULL, 0, &last_seq);
				}
				break;
			default:
				break;
		}
	}
	printf ("Escolha o vídeo que deseja assistir: ");
	scanf ("%s", input);
	memset(&pacote, 0, sizeof(protocolo_t));
	baixa_video(soquete, pacote, input);
}

void trata_pacote(int soquete, unsigned char *input){
	protocolo_t pacote;
	switch (recebe_buffer(soquete, &pacote, &last_seq)){
		case ACK:
			switch (pacote.tipo){
				case MOSTRAR:
					envia_buffer(soquete, inc_seq(&sequencia), ACK, NULL, 0, &last_seq);
					printf ("%s\n", pacote.dados);
					recebe_videos(soquete, pacote, input);
					break;
				case DESCRITOR:
					break;
				case DADOS:
					break;
				case FIM_TRANSMISSAO:
					break;
				default:
					break;
			}
			break;
		case NACK:
			if (pacote.sequencia == last_seq){
				send(soquete, ultimo_enviado, sizeof(protocolo_t), 0);
			} else {
				envia_buffer(soquete, inc_seq(&sequencia), NACK, NULL, 0, &last_seq);
			}
			break;
		default:
			break;
	}
}

int main(int argc, char const* argv[]){
	int soquete = cria_raw_socket("enp2s0f1");
	unsigned char input[63];
	DIR *dir = opendir("./videos");
	if(dir){
		closedir(dir);
	} else if (ENOENT == errno){
		mkdir("./videos", 0777);
		dir = opendir("./videos");
	}
	while (1){
		trata_pacote(soquete, input);
	}
	close(soquete);
	return 0;
}
