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
    unsigned char nome[63] = { 0 };
    struct dirent* entrada = NULL;
    int aceito = 0;
    while ((entrada = readdir(diretorio)) != NULL){
        char *extensao = strrchr(entrada->d_name, '.');
        if ((!strcmp(extensao, ".mp4") || !strcmp(extensao, ".avi")) && strlen(entrada->d_name) <= 63){
            memcpy(nome, entrada->d_name, strlen(entrada->d_name));
            envia_buffer(soquete, inc_seq(&sequencia), 16, nome, strlen((char *) nome), &last_seq);
            aceito = recebe_confirmacao(soquete, &last_seq);
            switch (aceito){
                case ACK:
                    break;
                case NACK:
                    while (aceito == NACK){
                        envia_buffer(soquete, sequencia, 16, nome, strlen((char *) nome), &last_seq);
                        aceito = recebe_confirmacao(soquete, &last_seq);
                    }
                    break;
                case TIMEOUT:
                    envia_buffer(soquete, sequencia, 16, nome, strlen((char *) nome), &last_seq);
                    aceito = recebe_confirmacao(soquete, &last_seq);
                    break;
                default:
                    break;
            }
        }   
    }
    envia_buffer(soquete, inc_seq(&sequencia), 30, NULL, 0, &last_seq);
    aceito = recebe_confirmacao(soquete, &last_seq);
    switch (aceito){
        case ACK:
            break;
        case NACK:
            while (aceito == NACK){
                envia_buffer(soquete, sequencia, 30, NULL, 0, &last_seq);
                aceito = recebe_confirmacao(soquete, &last_seq);
            }
            break;
        case TIMEOUT:
            envia_buffer(soquete, sequencia, 30, NULL, 0, &last_seq);
            aceito = recebe_confirmacao(soquete, &last_seq);
            break;
        default:
            break;
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
        envia_buffer(soquete, inc_seq(&sequencia), DADOS, buffer, lidos, &last_seq);
        int aceito = recebe_confirmacao(soquete, &last_seq);
        switch (aceito){
            case ACK:
                break;
            case NACK:
                while (aceito == NACK){
                    envia_buffer(soquete, sequencia, DADOS, buffer, lidos, &last_seq);
                    aceito = recebe_confirmacao(soquete, &last_seq);
                }
                break;
            case TIMEOUT:
                envia_buffer(soquete, sequencia, DADOS, buffer, lidos, &last_seq);
                aceito = recebe_confirmacao(soquete, &last_seq);
                break;
            default:
                break;
        }
        memset(buffer, 0, 63);
    }
    fclose(arquivo);
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
}

void manda_video(int soquete, protocolo_t pacote){
    char nome[73] = { 0 };
    unsigned char *buffer = malloc(63);
    snprintf(nome, 73, "./videos/%s", pacote.dados);
    struct stat info;
    if (stat(nome, &info) == -1){
        unsigned char erro[8] = { 0 };
        switch (errno){
            case EACCES:
                snprintf((char *) erro, 8, "%d", 1);
                envia_buffer(soquete, inc_seq(&sequencia), ERRO, erro, 1, &last_seq);
                break;
            case ENOENT:
                snprintf((char *) erro, 8, "%d", 2);
                envia_buffer(soquete, inc_seq(&sequencia), ERRO, erro, 1, &last_seq);
                break;
            default:
                snprintf((char *) erro, 8, "%d", 3);
                envia_buffer(soquete, inc_seq(&sequencia), ERRO, erro, 16, &last_seq);
                break;
        }
        exit(1);
    }
    envia_buffer(soquete, inc_seq(&sequencia), ACK, NULL, 0, &last_seq);
    snprintf((char *) buffer, 63, "%ld %ld", info.st_size/1000000, info.st_ctime);
    envia_buffer(soquete, inc_seq(&sequencia), DESCRITOR, buffer, strlen((char *) buffer), &last_seq);
    int aceito = recebe_confirmacao(soquete, &last_seq);
    switch (aceito){
        case ACK:
            break;
        case NACK:
            while (aceito == NACK){
                envia_buffer(soquete, sequencia, DESCRITOR, buffer, strlen((char *) buffer), &last_seq);
                aceito = recebe_confirmacao(soquete, &last_seq);
            }
            break;
        case TIMEOUT:
            envia_buffer(soquete, sequencia, DESCRITOR, buffer, strlen((char *) buffer), &last_seq);
            aceito = recebe_confirmacao(soquete, &last_seq);
            break;
        default:
            break;
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
                case FIM_TRANSMISSAO:
                    envia_buffer(soquete, inc_seq(&sequencia), ACK, NULL, 0, &last_seq);
                    exit(0);
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
    int soquete = cria_raw_socket("enp5s0");
	lista_videos(soquete);
    while (1){
        trata_pacote(soquete);
    }
    close(soquete);
    return 0;
}