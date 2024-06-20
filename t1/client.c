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
	printf ("Deseja baixar outro vídeo? (s/n): ");
	char c;
	getchar();
	scanf ("%c", &c);
	if (c == 'n' || c == 'N'){
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
	pergunta_videos(soquete);
}

void escreve_arquivo(int soquete, protocolo_t pacote, char *nome, unsigned char *buffer_sequencia){
	char *caminho = (char*) malloc(strlen(nome) + 10);
	unsigned char *buffer = (unsigned char *) malloc(TAMANHO);
	long int tam, data;
	int removidos;
	sscanf((char *) pacote.dados, "%ld %ld", &tam, &data);
	struct statvfs stat;
	statvfs("./videos", &stat);
	if ((tam + 5) * 1000000 > stat.f_bsize * stat.f_bavail){
		snprintf((char *) buffer, TAMANHO, "%d", 3);
		envia_buffer(soquete, inc_seq(&sequencia), ERRO, buffer, strlen((char *) buffer));
		exit(1);
	}
	envia_buffer(soquete, inc_seq(&sequencia), ACK, buffer_sequencia, strlen((char *) buffer_sequencia));
	snprintf(caminho, strlen(nome) + 10, "./videos/%s", nome);
	FILE *arquivo = fopen(caminho, "w");
	while (pacote.tipo != FIM_TRANSMISSAO){
		switch (recebe_buffer(soquete, &pacote, &last_seq)){
			case ACK:
				snprintf((char *) buffer_sequencia, TAMANHO, "%d", pacote.sequencia);
				if (pacote.tipo == DADOS){
					removidos = remove_vlan(pacote.dados);
					fwrite(pacote.dados, 1, pacote.tamanho - removidos, arquivo);
					envia_buffer(soquete, inc_seq(&sequencia), ACK, buffer_sequencia, strlen((char *) buffer_sequencia));
				} else if (pacote.tipo == FIM_TRANSMISSAO){
					envia_buffer(soquete, inc_seq(&sequencia), ACK, buffer_sequencia, strlen((char *) buffer_sequencia));
					fclose(arquivo);
					toca_video(soquete, caminho);
				}
				break;
			case NACK:
				snprintf((char *) buffer_sequencia, TAMANHO, "%d", pacote.sequencia);
				if (verifica_sequencias(pacote, last_seq)){
					envia_buffer(soquete, inc_seq(&sequencia), ACK, buffer_sequencia, strlen((char *) buffer_sequencia));
				} else {
					envia_buffer(soquete, inc_seq(&sequencia), NACK, buffer_sequencia, strlen((char *) buffer_sequencia));
				}
				break;
			default:
				break;
		}
	}
}

void baixa_video(int soquete, protocolo_t pacote, unsigned char *input, unsigned char *buffer_sequencia){
	trata_envio(soquete, &sequencia, BAIXAR, input, strlen((char *) input), &last_seq);
	switch (recebe_buffer(soquete, &pacote, &last_seq)){
		case ACK:
			snprintf((char *) buffer_sequencia, TAMANHO, "%d", pacote.sequencia);
			escreve_arquivo(soquete, pacote, (char *) input, buffer_sequencia);
			break;
		case NACK:
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

void recebe_videos(int soquete, protocolo_t pacote, unsigned char *input, unsigned char *buffer_sequencia){
	while (pacote.tipo != FIM_TRANSMISSAO){
		switch (recebe_buffer(soquete, &pacote, &last_seq)){
			case ACK:
				snprintf((char *) buffer_sequencia, TAMANHO, "%d", pacote.sequencia);
				envia_buffer(soquete, inc_seq(&sequencia), ACK, buffer_sequencia, strlen((char *) buffer_sequencia));
				printf ("%s\n", pacote.dados);
				break;
			case NACK:
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
	baixa_video(soquete, pacote, input, buffer_sequencia);
}

void trata_pacote(int soquete, unsigned char *input, unsigned char *buffer_sequencia){
	protocolo_t pacote;
	switch (recebe_buffer(soquete, &pacote, &last_seq)){
		case ACK:
			switch (pacote.tipo){
				case MOSTRAR:
					snprintf((char *) buffer_sequencia, TAMANHO, "%d", pacote.sequencia);
					envia_buffer(soquete, inc_seq(&sequencia), ACK, buffer_sequencia, strlen((char *) buffer_sequencia));
					printf ("%s\n", pacote.dados);
					recebe_videos(soquete, pacote, input, buffer_sequencia);
					break;
				default:
					snprintf((char *) buffer_sequencia, TAMANHO, "%d", 4);
					envia_buffer(soquete, inc_seq(&sequencia), ERRO, buffer_sequencia, 0);
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

int main(int argc, char const* argv[]){
	int soquete = cria_raw_socket("enp2s0f1");
	unsigned char input[TAMANHO], buffer_sequencia[TAMANHO];
	DIR *dir = opendir("./videos");
	if(dir){
		closedir(dir);
	} else if (ENOENT == errno){
		mkdir("./videos", 0777);
		dir = opendir("./videos");
	}
	while (1){
		trata_pacote(soquete, input, buffer_sequencia);
	}
	close(soquete);
	return 0;
}
