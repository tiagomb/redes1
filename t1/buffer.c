#include "buffer.h"
#include "header.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/time.h>
#include "crc.h"

char ultimo_enviado[67] = { 0 };
char ultimo_recebido[67] = { 0 };

long long int timestamp(){
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec/1000;
}

unsigned int inc_seq(unsigned int *sequencia){
    *sequencia = (*sequencia + 1) % 32;
    return *sequencia;
}

unsigned int dec_seq(unsigned int *sequencia){
    if (*sequencia == 0){
        *sequencia = 31;
    } else {
        *sequencia -= 1;
    }
    return *sequencia;
    
}

int insere_vlan(unsigned char *dados){
    int contador = 0;
    for (int i = 0; i < 62; i++){
        if (dados[i] == 129 || dados[i] == 136){
            for (int j = 61; j > i; j--){
                dados[j+1] = dados[j];
            }
            dados[i+1] = 255;
            contador++;
        }
    }
    return contador;
}

int remove_vlan(unsigned char *dados){
    int contador = 0;
    for (int i = 1; i < 63; i++){
        if ((dados[i-1] == 129 || dados[i-1] == 136) && dados[i] == 255){
            for (int j = i; j < 62; j++){
                dados[j] = dados[j+1];
            }
            contador++;
        }
    }
    return contador;
}

int envia_buffer(int soquete, unsigned int sequencia, unsigned int tipo, unsigned char* dados, unsigned int tamanho, unsigned int *last_seq){
    unsigned char *buffer = (unsigned char*) malloc(sizeof(protocolo_t));
    protocolo_t *pacote = (protocolo_t*) buffer;
    pacote->marcador = 126;
    pacote->tamanho = tamanho;
    pacote->sequencia = sequencia;
    pacote->tipo = tipo;
    memset(pacote->dados, 0, 63);
    memcpy(pacote->dados, dados, tamanho);
    pacote->crc = calculaCRC(&buffer[1], sizeof(protocolo_t) - 2, tabela_crc);
    int enviado = send(soquete, buffer, sizeof(protocolo_t), 0);
    memcpy(ultimo_enviado, buffer, sizeof(protocolo_t));
    if (enviado == -1) {
        fprintf(stderr, "Erro ao enviar pacote: %s\n", strerror(errno));
        free(buffer);
        return -1;
    }
    free(buffer);
    return 0;
}

int buffer_eh_valido(protocolo_t *pacote){
    return pacote->marcador == 126;
}

int recebe_msg(int soquete, unsigned char *buffer){
    long long int comeco = timestamp();
    struct timeval timeout = {TIMEOUT/1000, (TIMEOUT%1000)*1000};
    setsockopt(soquete, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    int bytes_recebidos = 0;
    do{
        bytes_recebidos = recv(soquete, buffer, sizeof(protocolo_t), 0);
        if (buffer_eh_valido((protocolo_t*) buffer)){
            return bytes_recebidos;
        }
    } while (timestamp() - comeco <= TIMEOUT);
    return -1;
}

int recebe_buffer(int soquete, protocolo_t *pacote, unsigned int *last_seq){
    unsigned char *buffer = (unsigned char*) malloc(sizeof(protocolo_t));
    unsigned int seq_esperada = inc_seq(last_seq);
    struct timeval timeout = {0, 0};
    setsockopt(soquete, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    protocolo_t *pacote_recebido = (protocolo_t*) buffer;
    int recebido = recv(soquete, buffer, sizeof(protocolo_t), 0);
    while (pacote_recebido->marcador != 126){
        recebido = recv(soquete, buffer, sizeof(protocolo_t), 0);
    }
    if (calculaCRC(&buffer[1], sizeof(protocolo_t) - 1, tabela_crc) != 0){
        dec_seq(last_seq);
        free(buffer);
        return NACK;
    }
    if (pacote_recebido->sequencia != seq_esperada){
        dec_seq(last_seq);
        free(buffer);
        return NACK;
    }
    memcpy(ultimo_recebido, buffer, sizeof(protocolo_t));
    memcpy(pacote, pacote_recebido, sizeof(protocolo_t));
    free(buffer);
    return ACK;
}

int recebe_confirmacao(int soquete, unsigned int *last_seq){
    unsigned char *buffer = (unsigned char*) malloc(sizeof(protocolo_t));
    protocolo_t *pacote = (protocolo_t*) buffer;
    int recebido = recebe_msg(soquete, buffer);
    if (recebido == -1) {
        fprintf(stderr, "Timeout %s\n", strerror(errno));
        free(buffer);
        return TIMEOUT;
    }
    inc_seq(last_seq);
    memcpy(ultimo_recebido, buffer, sizeof(protocolo_t));
    if (pacote->tipo == ERRO){
        fprintf(stderr, "Erro ao receber pacote: %s\n", pacote->dados);
        free(buffer);
        exit(1);
    }
    if (pacote->tipo == ACK || pacote->tipo == NACK){
        int tipo = pacote->tipo;
        free(buffer);
        return tipo;
    }
}
