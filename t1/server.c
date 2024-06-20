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

unsigned int sequencia = 31;
unsigned int last_seq = 31;

// protocolo_t *janela[JANELA] = { NULL, NULL, NULL, NULL, NULL };

void lista_videos(int soquete){
    DIR* diretorio = opendir("./videos");
    unsigned char nome[TAMANHO] = { 0 };
    struct dirent* entrada = NULL;
    while ((entrada = readdir(diretorio)) != NULL){
        char *extensao = strrchr(entrada->d_name, '.');
        memset(nome, 0, TAMANHO);
        if ((!strcmp(extensao, ".mp4") || !strcmp(extensao, ".avi")) && strlen(entrada->d_name) <= TAMANHO){
            memcpy(nome, entrada->d_name, strlen(entrada->d_name));
            trata_envio(soquete, &sequencia, MOSTRAR, nome, strlen((char *) nome), &last_seq);
        }   
    }
    trata_envio(soquete, &sequencia, FIM_TRANSMISSAO, NULL, 0, &last_seq);
    closedir(diretorio);
}

void le_arquivo(int soquete, char *nome){
    FILE *arquivo = fopen(nome, "rb");
    unsigned char *buffer = malloc(TAMANHO);
    protocolo_t *pacote;
    int removidos, lidos = 0;
    while ((lidos = fread(buffer, 1, TAMANHO, arquivo)) > 0){
        removidos = insere_vlan(buffer);
        fseek(arquivo, -removidos, SEEK_CUR);
        trata_envio(soquete, &sequencia, DADOS, buffer, lidos, &last_seq);
        memset(buffer, 0, TAMANHO);
    }
    fclose(arquivo);
    trata_envio(soquete, &sequencia, FIM_TRANSMISSAO, NULL, 0, &last_seq);
}

void manda_video(int soquete, protocolo_t pacote, unsigned char *buffer_sequencia){
    char nome[73] = { 0 };
    unsigned char *buffer = malloc(TAMANHO);
    snprintf(nome, 73, "./videos/%s", pacote.dados);
    struct stat info;
    if (stat(nome, &info) == -1){
        switch (errno){
            case EACCES:
                snprintf((char *) buffer, TAMANHO, "%d", 1);
                envia_buffer(soquete, inc_seq(&sequencia), ERRO, buffer, strlen((char *) buffer));
                break;
            case ENOENT:
                snprintf((char *) buffer, TAMANHO, "%d", 2);
                envia_buffer(soquete, inc_seq(&sequencia), ERRO, buffer, strlen((char *) buffer));
                break;
            default:
                snprintf((char *) buffer, TAMANHO, "%d", 4);
                envia_buffer(soquete, inc_seq(&sequencia), ERRO, buffer, strlen((char *) buffer));
                break;
        }
        exit(1);
    }
    snprintf((char *) buffer_sequencia, TAMANHO, "%d", pacote.sequencia);
    envia_buffer(soquete, inc_seq(&sequencia), ACK, buffer_sequencia, strlen((char *) buffer_sequencia));
    snprintf((char *) buffer, TAMANHO, "%ld %ld", info.st_size/1000000, info.st_ctime);
    trata_envio(soquete, &sequencia, DESCRITOR, buffer, strlen((char *) buffer), &last_seq);
    le_arquivo(soquete, nome);
}

void trata_pacote(int soquete, unsigned char *buffer_sequencia){
	protocolo_t pacote;
	switch (recebe_buffer(soquete, &pacote, &last_seq)){
		case ACK:
            snprintf((char *) buffer_sequencia, TAMANHO, "%d", pacote.sequencia);
			switch (pacote.tipo){
				case LISTA:
                    envia_buffer(soquete, inc_seq(&sequencia), ACK, buffer_sequencia, strlen((char*) buffer_sequencia));
                    lista_videos(soquete);
					break;
				case BAIXAR:
                    manda_video(soquete, pacote, buffer_sequencia);
					break;
                case FIM_TRANSMISSAO:
                    envia_buffer(soquete, inc_seq(&sequencia), ACK, buffer_sequencia, strlen((char*) buffer_sequencia));
                    exit(0);
                    break;
				default:
					break;
			}
			break;
		case NACK:
            snprintf((char *) buffer_sequencia, TAMANHO, "%d", pacote.sequencia);
			if (pacote.sequencia == last_seq){
				send(soquete, ultimo_enviado, sizeof(protocolo_t), 0);
			} else {
                envia_buffer(soquete, inc_seq(&sequencia), NACK, buffer_sequencia, strlen((char*) buffer_sequencia));
            }
			break;
		default:
			break;
	}
}

int main(int argc, char const* argv[]){
    int soquete = cria_raw_socket("enp5s0");
    unsigned char buffer_sequencia[TAMANHO] = { 0 };
	lista_videos(soquete);
    while (1){
        trata_pacote(soquete, buffer_sequencia);
    }
    close(soquete);
    return 0;
}