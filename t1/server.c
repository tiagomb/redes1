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


int sequencia = 0;
int last_seq = 31;

void lista_videos(int soquete){
    DIR* diretorio = opendir("./videos");
    char nome[63] = { 0 };
    struct dirent* entrada = NULL;
    int aceito = 0;
    while ((entrada = readdir(diretorio)) != NULL){
        if (strcmp(entrada->d_name, ".") && strcmp(entrada->d_name, "..")){
            snprintf(nome, 63, "%s", entrada->d_name);
            if (sequencia == 31){
                sequencia = 0;
                aceito = envia_buffer(soquete, sequencia++, 16, nome, strlen(nome));
                if (aceito == 1){
                    sequencia--;
                    while (aceito){
                        aceito = envia_buffer(soquete, sequencia, 16, nome, strlen(nome));
                    }
                    sequencia++;
                }
            } else {
                aceito = envia_buffer(soquete, sequencia++, 16, nome, strlen(nome));
                if (aceito == 1){
                    sequencia--;
                    while (aceito){
                        aceito = envia_buffer(soquete, sequencia, 16, nome, strlen(nome));
                    }
                    sequencia++;
                }
            }
        }   
    }
    aceito = envia_buffer(soquete, sequencia++, 30, NULL, 0);
    if (aceito == 1){
        sequencia--;
        while (aceito == 1){
            aceito = envia_buffer(soquete, sequencia, 30, NULL, 0);
        }
        sequencia++;
    }
    closedir(diretorio);
}

void manda_video(int soquete, protocolo_t pacote){
    char nome[63] = { 0 };
    char *buffer = malloc (sizeof(protocolo_t));
    protocolo_t *pacote_envio = (protocolo_t*) buffer;
    snprintf(nome, 63, "./videos/%s", pacote.dados);
    struct stat info;
    if (stat(nome, &info) == -1){
        switch (errno){
            case EACCES:
                envia_buffer(soquete, sequencia++, ERRO, 1, 21);
                break;
            case ENOENT:
                envia_buffer(soquete, sequencia++, ERRO, 2, 21);
                break;
            default:
                envia_buffer(soquete, sequencia++, ERRO, "Erro desconhecido", 16);
                break;
        }
    }
    snprintf(pacote_envio->dados, 63, "%ld %ld", info.st_size/1000000, info.st_ctime.tv_sec);
    envia_buffer(soquete, sequencia++, DESCRITOR, pacote_envio->dados, strlen(pacote_envio->dados));
}

void trata_pacote(int soquete){
	protocolo_t pacote;
	switch (recebe_buffer(soquete, &pacote)){
		case ACK:
			switch (pacote.tipo){
				case LISTA:
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
			envia_buffer(soquete, sequencia++, NACK, NULL, 0);
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
