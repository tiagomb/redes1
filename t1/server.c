// Server side C program to demonstrate Socket
// programming
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <net/ethernet.h>
#include <linux/if.h>
#include <linux/if_packet.h>
#include <unistd.h>
#include "conexao.h"
#include "header.h"
#include "buffer.h"
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>

unsigned int sequencia = 31; //Variável para indicar a sequência enviada. Começa em 31 pois é incrementada antes do envio, ou seja, o primeiro envio vai ser 0.
unsigned int last_seq = 31; // Variável para indicar a última sequência recebida com sucesso. Recebe o valor de seq_esperada antes de ela ser atualizada.
unsigned int seq_esperada = 0; //Variável para indicar a sequência esperada. Quando uma mensagem é recebida com sucesso, seu valor é incrementado.

void lista_videos(int soquete, unsigned char *buffer_sequencia){
    DIR* diretorio = opendir(DIRETORIO);
    unsigned int erro;
    if (diretorio == NULL){
        unsigned char buffer[TAMANHO] = { 0 };
        switch (errno){
            case EACCES:
                erro = 1;
                memcpy(buffer, &erro, sizeof(unsigned int));
                envia_buffer(soquete, inc_seq(&sequencia), ERRO, buffer, sizeof(unsigned int));
                break;
            case ENOENT:
                erro = 2;
                memcpy(buffer, &erro, sizeof(unsigned int));
                snprintf((char *) buffer, TAMANHO, "%d", 2);
                envia_buffer(soquete, inc_seq(&sequencia), ERRO, buffer, sizeof(unsigned int));
                break;
            default:
                erro = 4;
                memcpy(buffer, &erro, sizeof(unsigned int));
                envia_buffer(soquete, inc_seq(&sequencia), ERRO, buffer, sizeof(unsigned int));
                break;
        }
        exit(erro);
    }
    envia_buffer(soquete, inc_seq(&sequencia), ACK, buffer_sequencia, sizeof(unsigned int));
    unsigned char nome[TAMANHO] = { 0 };
    struct dirent* entrada = NULL;
    while ((entrada = readdir(diretorio)) != NULL){
        char *extensao = strrchr(entrada->d_name, '.');
        memset(nome, 0, TAMANHO);
        if ((!strcmp(extensao, ".mp4") || !strcmp(extensao, ".avi")) && strlen(entrada->d_name) <= TAMANHO){
            memcpy(nome, entrada->d_name, strlen(entrada->d_name));
            trata_envio(soquete, &sequencia, MOSTRAR, nome, strlen((char *) nome), &last_seq, &seq_esperada);
        }   
    }
    trata_envio(soquete, &sequencia, FIM_TRANSMISSAO, NULL, 0, &last_seq, &seq_esperada);
    closedir(diretorio);
}

void le_arquivo(int soquete, char *nome){
    FILE *arquivo = fopen(nome, "rb");
    unsigned char *buffer = malloc(TAMANHO);
    int removidos, lidos = 0;
    while ((lidos = fread(buffer, 1, TAMANHO, arquivo)) > 0){
        removidos = insere_vlan(buffer);
        fseek(arquivo, -removidos, SEEK_CUR);
        trata_envio(soquete, &sequencia, DADOS, buffer, lidos, &last_seq, &seq_esperada);
        memset(buffer, 0, TAMANHO);
    }
    fclose(arquivo);
    free(buffer);
    trata_envio(soquete, &sequencia, FIM_TRANSMISSAO, NULL, 0, &last_seq, &seq_esperada);
}

void manda_video(int soquete, protocolo_t pacote, unsigned char *buffer_sequencia){
    char nome[TAMANHO + 10] = { 0 };
    unsigned char *buffer = malloc(TAMANHO);
    snprintf(nome, TAMANHO+10, "%s/%s", DIRETORIO, pacote.dados);
    struct stat info;
    unsigned int erro;
    if (stat(nome, &info) == -1){
        switch (errno){
            case EACCES:
                erro = 1;
                memcpy(buffer, &erro, sizeof(unsigned int));
                envia_buffer(soquete, inc_seq(&sequencia), ERRO, buffer, sizeof(unsigned int));
                break;
            case ENOENT:
                erro = 2;
                memcpy(buffer, &erro, sizeof(unsigned int));
                envia_buffer(soquete, inc_seq(&sequencia), ERRO, buffer, sizeof(unsigned int));
                break;
            default:
                erro = 4;
                memcpy(buffer, &erro, sizeof(unsigned int));
                envia_buffer(soquete, inc_seq(&sequencia), ERRO, buffer, sizeof(unsigned int));
                break;
        }
        exit(erro);
    }
    envia_buffer(soquete, inc_seq(&sequencia), ACK, buffer_sequencia, strlen((char *) buffer_sequencia));
    memcpy(buffer, &info.st_size, sizeof(off_t));
    trata_envio(soquete, &sequencia, DESCRITOR, buffer, sizeof(unsigned int), &last_seq, &seq_esperada);
    free(buffer);
    le_arquivo(soquete, nome);
}

void trata_pacote(int soquete, unsigned char *buffer_sequencia){
	protocolo_t pacote;
	switch (recebe_buffer(soquete, &pacote, &last_seq, &seq_esperada)){
		case ACK:
            memcpy(buffer_sequencia, &last_seq, sizeof(unsigned int));
			switch (pacote.tipo){
				case LISTA:
                    lista_videos(soquete, buffer_sequencia);
					break;
				case BAIXAR:
                    manda_video(soquete, pacote, buffer_sequencia);
					break;
                case FIM_TRANSMISSAO:
                    envia_buffer(soquete, inc_seq(&sequencia), ACK, buffer_sequencia, sizeof(unsigned int));
                    exit(0);
                    break;
				default:
					break;
			}
			break;
		case NACK:
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

int main(int argc, char *argv[]){
    if (argc != 2){
        fprintf(stderr, "Uso: %s <interface de rede>\n", argv[0]);
        exit(1);
    }
    int soquete = cria_raw_socket(argv[1]);
    unsigned char buffer_sequencia[TAMANHO] = { 0 };
    while (1){
        trata_pacote(soquete, buffer_sequencia);
    }
    close(soquete);
    return 0;
}