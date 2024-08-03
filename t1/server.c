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

unsigned char janela[JANELA][sizeof(protocolo_t)];

void muda_frame(int *lidos, unsigned char *buffer, unsigned char *frame, FILE *arquivo){
    int removidos;
    unsigned char *buffer_envio;
    *lidos = fread(buffer, 1, TAMANHO, arquivo);
    removidos = insere_vlan(buffer);
    fseek(arquivo, -removidos, SEEK_CUR);
    buffer_envio = monta_buffer(inc_seq(&sequencia), DADOS, buffer, *lidos);
    memcpy(frame, buffer_envio, sizeof(protocolo_t));
    free(buffer_envio);
    memset(buffer, 0, TAMANHO);
}

void extrai_confirmacao(int soquete, int *aceito, int *diff){
    protocolo_t *confirmacao = recebe_confirmacao(soquete, &last_seq);
    unsigned int seq_recebida;
    *aceito = confirmacao->tipo;
    memcpy(&seq_recebida, confirmacao->dados, sizeof(unsigned int));
    *diff = sequencia - seq_recebida;
    if (*diff < 0){
        *diff += 32;
    }
    free(confirmacao);
}

void lista_videos(int soquete, unsigned char *buffer_sequencia){
    DIR* diretorio = opendir(DIRETORIO);
    unsigned int erro = 0;
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
            trata_envio(soquete, &sequencia, MOSTRAR, nome, strlen((char *) nome), &last_seq);
        }   
    }
    trata_envio(soquete, &sequencia, FIM_TRANSMISSAO, NULL, 0, &last_seq);
    closedir(diretorio);
}

void le_arquivo(int soquete, char *nome){
    FILE *arquivo = fopen(nome, "rb");
    unsigned char *buffer = malloc(TAMANHO);
    int lidos = 0, aceito = -1, diff = 0;
    for (int i = 0; i < JANELA; i++){
        muda_frame(&lidos, buffer, janela[i], arquivo);
        send(soquete, janela[i], sizeof(protocolo_t), 0);
    }
    while (lidos > 0){
        extrai_confirmacao(soquete, &aceito, &diff);
        switch (aceito){
            case ACK:
                for (int i = 0; i < diff; i++){
                    memcpy(janela[i], janela[i+JANELA-diff], sizeof(protocolo_t));
                }
                for (int i = diff; i < JANELA; i++){
                    muda_frame(&lidos, buffer, janela[i], arquivo);
                }
                for (int i = 0; i < JANELA; i++){
                    send(soquete, janela[i], sizeof(protocolo_t), 0);
                }
            break;
            case NACK:
                diff += 1;
                for (int i = 0; i < diff; i++){
                    memcpy(janela[i], janela[i+JANELA-diff], sizeof(protocolo_t));
                }
                for (int i = diff; i < JANELA; i++){
                    muda_frame(&lidos, buffer, janela[i], arquivo);
                }
                for (int i = 0; i < JANELA; i++){
                    send(soquete, janela[i], sizeof(protocolo_t), 0);
                }
                break;
            case TIMEOUT:
                for (int i = 0; i < JANELA; i++){
                    send(soquete, janela[i], sizeof(protocolo_t), 0);
                }
                break;
        }
    }
    extrai_confirmacao(soquete, &aceito, &diff);
    while (diff != 0 || aceito != ACK){
        if (aceito == NACK){
            for (int i = 0; i < JANELA; i++){
                send(soquete, janela[i], sizeof(protocolo_t), 0);
            }
        }
        extrai_confirmacao(soquete, &aceito, &diff);
    }
    fclose(arquivo);
    free(buffer);
    trata_envio(soquete, &sequencia, FIM_TRANSMISSAO, NULL, 0, &last_seq);
}

void manda_video(int soquete, protocolo_t pacote, unsigned char *buffer_sequencia){
    char nome[TAMANHO + 10] = { 0 };
    unsigned char *buffer = malloc(TAMANHO);
    snprintf(nome, TAMANHO+10, "%s/%s", DIRETORIO, pacote.dados);
    struct stat info;
    unsigned int erro, to_send;
    if (stat(nome, &info) == -1){
        switch (errno){
            case EACCES:
                erro = 1;
                memcpy(buffer, &erro, sizeof(unsigned int));
                envia_buffer(soquete, inc_seq(&sequencia), ERRO, buffer, sizeof(unsigned int));
                free(buffer);
                break;
            case ENOENT:
                erro = 2;
                memcpy(buffer, &erro, sizeof(unsigned int));
                envia_buffer(soquete, inc_seq(&sequencia), ERRO, buffer, sizeof(unsigned int));
                free(buffer);
                break;
            default:
                erro = 4;
                memcpy(buffer, &erro, sizeof(unsigned int));
                envia_buffer(soquete, inc_seq(&sequencia), ERRO, buffer, sizeof(unsigned int));
                free(buffer);
                break;
        }
        exit(erro);
    }
    to_send = pacote.sequencia;
    memcpy(buffer_sequencia, &to_send, sizeof(unsigned int));
    envia_buffer(soquete, inc_seq(&sequencia), ACK, buffer_sequencia, sizeof(unsigned int));
    memcpy(buffer, &info.st_size, sizeof(off_t));
    trata_envio(soquete, &sequencia, DESCRITOR, buffer, sizeof(off_t), &last_seq);
    free(buffer);
    le_arquivo(soquete, nome);
}

void trata_pacote(int soquete, unsigned char *buffer_sequencia){
	protocolo_t pacote;
    unsigned int to_send;
	switch (recebe_buffer(soquete, &pacote, &last_seq, 0)){
		case ACK:
            to_send = pacote.sequencia;
            memcpy(buffer_sequencia, &to_send, sizeof(unsigned int));
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
            if (pacote.sequencia == last_seq){
				to_send = last_seq;
				memcpy(buffer_sequencia, &to_send, sizeof(unsigned int));
				envia_buffer(soquete, inc_seq(&sequencia), ACK, buffer_sequencia, sizeof(unsigned int));
			} else {
				to_send = (last_seq + 1) % 32;
				memcpy(buffer_sequencia, &to_send, sizeof(unsigned int));
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