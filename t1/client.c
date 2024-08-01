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

unsigned int sequencia = 31; //Variável para indicar a sequência enviada. Começa em 31 pois é incrementada antes do envio, ou seja, o primeiro envio vai ser 0.
unsigned int last_seq = 31; // Variável para indicar a última sequência recebida com sucesso. Recebe o valor de seq_esperada antes de ela ser atualizada.
unsigned int seq_esperada = 0; //Variável para indicar a sequência esperada. Quando uma mensagem é recebida com sucesso, seu valor é incrementado.

void pergunta_videos(int soquete){
	printf ("Vídeo baixado com sucesso e rodando! Deseja baixar outro vídeo? (s/n): ");
	char c;
	getchar();
	scanf ("%c", &c);
	if (c == 'n' || c == 'N'){
		printf ("Encerrando conexão...\n");
		trata_envio(soquete, &sequencia, FIM_TRANSMISSAO, NULL, 0, &last_seq, &seq_esperada);
		exit(0);
	} else {
		trata_envio(soquete, &sequencia, LISTA, NULL, 0, &last_seq, &seq_esperada);
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
	off_t tam;
	unsigned int removidos, erro;
	memcpy(&tam, pacote.dados, sizeof(off_t));
	struct statvfs stat;
	statvfs(DIRETORIO, &stat);
	if ((tam + 5) * 1000000 > stat.f_bsize * stat.f_bavail){
		erro = 3;
		memcpy(buffer, &erro, sizeof(unsigned int));
		envia_buffer(soquete, inc_seq(&sequencia), ERRO, buffer, sizeof(unsigned int));
		exit(1);
	}
	printf ("Baixando %s no diretório %s\n", nome, DIRETORIO);
	envia_buffer(soquete, inc_seq(&sequencia), ACK, buffer_sequencia, sizeof(unsigned int));
	snprintf(caminho, strlen(nome) + 10, "%s/%s", DIRETORIO, nome);
	FILE *arquivo = fopen(caminho, "w");
	int ack = 0;
	while (pacote.tipo != FIM_TRANSMISSAO || !ack){
		switch (recebe_buffer(soquete, &pacote, &last_seq, &seq_esperada)){
			case ACK:
				ack = 1;
				memcpy(buffer_sequencia, &last_seq, sizeof(unsigned int));
				if (pacote.tipo == DADOS){
					removidos = remove_vlan(pacote.dados);
					fwrite(pacote.dados, 1, pacote.tamanho - removidos, arquivo);
					envia_buffer(soquete, inc_seq(&sequencia), ACK, buffer_sequencia, sizeof(unsigned int));
				} else if (pacote.tipo == FIM_TRANSMISSAO){
					envia_buffer(soquete, inc_seq(&sequencia), ACK, buffer_sequencia, sizeof(unsigned int));
					fclose(arquivo);
					free(buffer);
					toca_video(soquete, caminho);
				}
				break;
			case NACK:
				ack = 0;
				memcpy(buffer_sequencia, &seq_esperada, sizeof(unsigned int));
				if (pacote.sequencia == last_seq){
					send(soquete, ultimo_enviado, sizeof(protocolo_t), 0);
				} else {
					envia_buffer(soquete, inc_seq(&sequencia), NACK, buffer_sequencia, sizeof(unsigned int));
				}
				break;
			default:
				break;
		}
	}
}

void recebe_videos(int soquete, protocolo_t pacote, unsigned char *input, unsigned char *buffer_sequencia){
	int ack = 0;
	while (pacote.tipo != FIM_TRANSMISSAO || !ack){
		switch (recebe_buffer(soquete, &pacote, &last_seq, &seq_esperada)){
			case ACK:
				ack = 1;
				memcpy(buffer_sequencia, &last_seq, sizeof(unsigned int));
				envia_buffer(soquete, inc_seq(&sequencia), ACK, buffer_sequencia, sizeof(unsigned int));
				printf ("%s\n", pacote.dados);
				break;
			case NACK:
				ack = 0;
				memcpy(buffer_sequencia, &seq_esperada, sizeof(unsigned int));
				if (pacote.sequencia == last_seq){
					send(soquete, ultimo_enviado, sizeof(protocolo_t), 0);
				} else {
					envia_buffer(soquete, inc_seq(&sequencia), NACK, buffer_sequencia, sizeof(unsigned int));
				}
				break;
			default:
				break;
		}
	}
	printf ("Escolha o vídeo que deseja assistir: ");
	scanf ("%s", input);
	memset(&pacote, 0, sizeof(protocolo_t));
	trata_envio(soquete, &sequencia, BAIXAR, input, strlen((char *) input), &last_seq, &seq_esperada);
}

void trata_pacote(int soquete, unsigned char *input, unsigned char *buffer_sequencia){
	protocolo_t pacote;
	unsigned int erro;
	switch (recebe_buffer(soquete, &pacote, &last_seq, &seq_esperada)){
		case ACK:
			switch (pacote.tipo){
				case MOSTRAR:
					printf ("Vídeos disponíveis no servidor: \n");
					memcpy(buffer_sequencia, &last_seq, sizeof(unsigned int));
					envia_buffer(soquete, inc_seq(&sequencia), ACK, buffer_sequencia, sizeof(unsigned int));
					printf ("%s\n", pacote.dados);
					recebe_videos(soquete, pacote, input, buffer_sequencia);
					break;
				case DESCRITOR:
					memcpy(buffer_sequencia, &last_seq, sizeof(unsigned int));
					escreve_arquivo(soquete, pacote, (char *) input, buffer_sequencia);
					break;
				default:
					erro = 4;
					memcpy(buffer_sequencia, &erro, sizeof(unsigned int));
					envia_buffer(soquete, inc_seq(&sequencia), ERRO, buffer_sequencia, sizeof(unsigned int));
					exit(erro);
					break;
			}
			break;
		case NACK:
			if (pacote.sequencia == last_seq){
				send(soquete, ultimo_enviado, sizeof(protocolo_t), 0);
			} else {
				memcpy(buffer_sequencia, &seq_esperada, sizeof(seq_esperada));
				envia_buffer(soquete, inc_seq(&sequencia), NACK, buffer_sequencia, sizeof(unsigned int));
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
	trata_envio(soquete, &sequencia, LISTA, NULL, 0, &last_seq, &seq_esperada);
	while (1){
		trata_pacote(soquete, input, buffer_sequencia);
	}
	close(soquete);
	return 0;
}