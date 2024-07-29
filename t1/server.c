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

unsigned char *janela[JANELA];

unsigned int retorna_diff(unsigned int sequencia){
    protocolo_t *pacote = (protocolo_t *) janela[JANELA-1];
    unsigned int last_seq = pacote->sequencia;
    unsigned int diff = last_seq - sequencia;
    if (diff < 0){
        diff += 32;
    }
    return diff;
}

void lista_videos(int soquete, unsigned char *buffer_sequencia){
    DIR* diretorio = opendir(DIRETORIO);
    if (diretorio == NULL){
        unsigned char buffer[TAMANHO] = { 0 };
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
    }
    envia_buffer(soquete, inc_seq(&sequencia), ACK, buffer_sequencia, strlen((char*) buffer_sequencia));
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
    int removidos, lidos = 0, aceito = -1, seq_recebida = 0, diff = 0;
    protocolo_t *confirmacao;
    for (int i = 0; i < JANELA; i++){
        lidos = fread(buffer, 1, TAMANHO, arquivo);
        removidos = insere_vlan(buffer);
        fseek(arquivo, -removidos, SEEK_CUR);
        janela[i] = monta_buffer(inc_seq(&sequencia), DADOS, buffer, lidos);
        send(soquete, janela[i], sizeof(protocolo_t), 0);
        memset(buffer, 0, TAMANHO);
    }
    while (lidos > 0){
        confirmacao = recebe_confirmacao(soquete, &last_seq);
        aceito = confirmacao->tipo;
        sscanf((char *) confirmacao->dados, "%d", &seq_recebida);
        diff = retorna_diff(seq_recebida);
        switch (aceito){
            case ACK:
                for (int i = 0; i < diff; i++){
                    memcpy(janela[i], janela[i+JANELA-diff], sizeof(protocolo_t));
                }
                for (int i = diff; i < JANELA; i++){
                    lidos = fread(buffer, 1, TAMANHO, arquivo);
                    removidos = insere_vlan(buffer);
                    fseek(arquivo, -removidos, SEEK_CUR);
                    janela[i] = monta_buffer(inc_seq(&sequencia), DADOS, buffer, lidos);
                    send(soquete, janela[i], sizeof(protocolo_t), 0);
                    memset(buffer, 0, TAMANHO);
                }
            case NACK:
                diff += 1;
                if (diff == JANELA){
                    for (int i = 0; i < JANELA; i++){
                        send(soquete, janela[i], sizeof(protocolo_t), 0);
                    }
                } else {
                    for (int i = 0; i < diff; i++){
                        memcpy(janela[i], janela[i+JANELA-diff], sizeof(protocolo_t));
                    }
                    for (int i = diff; i < JANELA; i++){
                        lidos = fread(buffer, 1, TAMANHO, arquivo);
                        removidos = insere_vlan(buffer);
                        fseek(arquivo, -removidos, SEEK_CUR);
                        janela[i] = monta_buffer(inc_seq(&sequencia), DADOS, buffer, lidos);
                        send(soquete, janela[i], sizeof(protocolo_t), 0);
                        memset(buffer, 0, TAMANHO);
                    }
                }
                break;
            case TIMEOUT:
                for (int i = 0; i < JANELA; i++){
                    send(soquete, janela[i], sizeof(protocolo_t), 0);
                }
                break;
        }
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
    free(buffer);
    le_arquivo(soquete, nome);
}

void trata_pacote(int soquete, unsigned char *buffer_sequencia){
	protocolo_t pacote;
	switch (recebe_buffer(soquete, &pacote, &last_seq, 0)){
		case ACK:
            snprintf((char *) buffer_sequencia, TAMANHO, "%d", pacote.sequencia);
			switch (pacote.tipo){
				case LISTA:
                    lista_videos(soquete, buffer_sequencia);
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