#include "buffer.h"
#include "header.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include "crc.h"

int envia_buffer(int soquete, unsigned int sequencia, unsigned int tipo, unsigned char* dados, unsigned int tamanho){
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
        free(buffer);
        return recebe_buffer(soquete, pacote);
    }
    free(buffer);
    return 0;
}

int recebe_buffer(int soquete, protocolo_t *pacote){
    unsigned char *buffer = (unsigned char*) malloc(sizeof(protocolo_t));
    protocolo_t *pacote_recebido = (protocolo_t*) buffer;
    int recebido = recv(soquete, buffer, sizeof(protocolo_t), 0);
    if (recebido == -1) {
        fprintf(stderr, "Erro ao receber pacote: %s\n", strerror(errno));
        free(buffer);
        return ERRO;
    }
    if (pacote_recebido->marcador != 126) {
        free(buffer);
        return IGNORAR;
    }
    if (recebido == 0 || calculaCRC(&buffer[1], sizeof(protocolo_t) - 1, tabela_crc) != 0){
        free(buffer);
        return NACK;
    }
    memcpy(pacote, pacote_recebido, sizeof(protocolo_t));
    free(buffer);
    return ACK;
}
