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

void lista_videos(int soquete){
    DIR* diretorio = opendir("./videos");
    char nome[63] = { 0 };
    struct dirent* entrada = NULL;
    int aceito = 0;
    while ((entrada = readdir(diretorio)) != NULL){
        if (strcmp(entrada->d_name, ".") && strcmp(entrada->d_name, "..")){
            snprintf(nome, 63, "%s", entrada->d_name);
            aceito = envia_buffer(soquete, inc_seq(&sequencia), 16, nome, strlen(nome), &last_seq);
            if (aceito == 1){
                while (aceito){
                    aceito = envia_buffer(soquete, sequencia, 16, nome, strlen(nome), &last_seq);
                }
            }
        }   
    }
    aceito = envia_buffer(soquete, inc_seq(&sequencia), 30, NULL, 0, &last_seq);
    if (aceito == 1){
        while (aceito == 1){
            aceito = envia_buffer(soquete, sequencia, 30, NULL, 0, &last_seq);
        }
    }
    closedir(diretorio);
}

void le_arquivo(int soquete, char *nome){
    FILE *arquivo = fopen(nome, "rb");
    unsigned char *buffer = malloc(63);
    int removidos, lidos = 0;
    while ((lidos = fread(buffer, 1, 63, arquivo)) > 0){
        removidos = insere_vlan(buffer);
        fseek(arquivo, -removidos, SEEK_CUR);
        int aceito = envia_buffer(soquete, inc_seq(&sequencia), DADOS, buffer, lidos - removidos, &last_seq);
        if (aceito == 1){
            while (aceito){
                aceito = envia_buffer(soquete, sequencia, DADOS, buffer, lidos - removidos, &last_seq);
            }
        }
    }
    fclose(arquivo);
    envia_buffer(soquete, inc_seq(&sequencia), FIM_TRANSMISSAO, NULL, 0, &last_seq);
}

void manda_video(int soquete, protocolo_t pacote){
    char nome[73] = { 0 };
    unsigned char *buffer = malloc(63);
    snprintf(nome, 73, "./videos/%s", pacote.dados);
    struct stat info;
    if (stat(nome, &info) == -1){
        switch (errno){
            case EACCES:
                envia_buffer(soquete, inc_seq(&sequencia), ERRO, "1", 1, &last_seq);
                break;
            case ENOENT:
                envia_buffer(soquete, inc_seq(&sequencia), ERRO, "2", 1, &last_seq);
                break;
            default:
                envia_buffer(soquete, inc_seq(&sequencia), ERRO, "Erro desconhecido", 16, &last_seq);
                break;
        }
        exit(1);
    }
    envia_buffer(soquete, inc_seq(&sequencia), ACK, NULL, 0, &last_seq);
    snprintf(buffer, 63, "%ld %ld", info.st_size/1000000, info.st_ctime);
    int aceito = envia_buffer(soquete, inc_seq(&sequencia), DESCRITOR, buffer, strlen(buffer), &last_seq);
    if (aceito == 1){
        while (aceito){
            aceito = envia_buffer(soquete, sequencia, DESCRITOR, buffer, strlen(buffer), &last_seq);
        }
    }
    le_arquivo(soquete, nome);
}

void trata_pacote(int soquete){
	protocolo_t pacote;
	switch (recebe_buffer(soquete, &pacote, &last_seq)){
		case ACK:
			switch (pacote.tipo){
				case LISTA:
                    envia_buffer(soquete, inc_seq(&sequencia), ACK, NULL, 0, &last_seq);
                    lista_videos(soquete);
					break;
				case BAIXAR:
                    manda_video(soquete, pacote);
					break;
				default:
					break;
			}
			break;
		case NACK:
			envia_buffer(soquete, inc_seq(&sequencia), NACK, NULL, 0, &last_seq);
			break;
		default:
			break;
	}
}

int main(int argc, char const* argv[]){
    int soquete = cria_raw_socket("enp5s0");
    struct timeval timeout = {TIMEOUT / 1000, (TIMEOUT % 1000) * 1000};
	setsockopt(soquete, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
	lista_videos(soquete);
    while (1){
        trata_pacote(soquete);
    }
    close(soquete);
    return 0;
}