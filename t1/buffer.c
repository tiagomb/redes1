#include "buffer.h"
#include "header.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/time.h>
#include "crc.h"

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
    if (enviado == -1) {
        fprintf(stderr, "Erro ao enviar pacote: %s\n", strerror(errno));
        free(buffer);
        return -1;
    }
    if (pacote->tipo != ACK && pacote->tipo != NACK){
        return recebe_buffer(soquete, pacote, last_seq);
    }
    free(buffer);
    return 0;
}

int buffer_eh_valido(protocolo_t *pacote){
    return pacote->marcador == 126;
}

int recebe_msg(int soquete, unsigned char *buffer){
    long long int comeco = timestamp();
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
    protocolo_t *pacote_recebido = (protocolo_t*) buffer;
    int recebido = recebe_msg(soquete, buffer);
    if (recebido == -1) {
        fprintf(stderr, "Timeout %s\n", strerror(errno));
        free(buffer);
        return TIMEOUT;
    }
    if (pacote_recebido->tipo == ERRO){
        fprintf(stderr, "Erro ao receber pacote: %s\n", pacote_recebido->dados);
        free(buffer);
        exit(1);
    }
    if ((pacote_recebido->tipo == ACK || pacote_recebido->tipo == NACK) && pacote_recebido->sequencia == seq_esperada){
        int tipo = pacote_recebido->tipo;
        free(pacote);
        free(buffer);
        return tipo;
    }
    if (recebido == 0 || calculaCRC(&buffer[1], sizeof(protocolo_t) - 1, tabela_crc) != 0){
        free(buffer);
        return NACK;
    }
    if (pacote_recebido->sequencia != seq_esperada){
<<<<<<< HEAD
        free(buffer);
        //printf("Sequencia esperada: %d, recebida: %d\n", seq_esperada, pacote_recebido->sequencia);
=======
        printf("Sequencia esperada: %d, recebida: %d\n", seq_esperada, pacote_recebido->sequencia);
>>>>>>> 9467820d04528aa47f5a359f8052dd6adf0f4699
        dec_seq(last_seq);
        free(buffer);
        return NACK;
    }
    memcpy(pacote, pacote_recebido, sizeof(protocolo_t));
    free(buffer);
    return ACK;
}
