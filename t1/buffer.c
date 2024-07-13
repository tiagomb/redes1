#include "buffer.h"
#include "header.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/time.h>

char ultimo_enviado[67] = { 0 };
char ultimo_recebido[67] = { 0 };
unsigned char tabela_crc[256] = {0, 7, 14, 9, 28, 27, 18, 21, 56, 63, 54, 49, 36, 35, 42, 45, 112, 119, 126, 121, 108, 107, 98, 101, 72, 79, 70, 65, 84, 83, 90, 93, 224, 231, 238, 233, 252, 251, 242, 245, 216, 223, 214, 209, 196, 195, 202, 205, 144, 151, 158, 153, 140, 139, 130, 133, 168, 175, 166, 161, 180, 179, 186, 189, 199, 192, 201, 206, 219, 220, 213, 210, 255, 248, 241, 246, 227, 228, 237, 234, 183, 176, 185, 190, 171, 172, 165, 162, 143, 136, 129, 134, 147, 148, 157, 154, 39, 32, 41, 46, 59, 60, 53, 50, 31, 24, 17, 22, 3, 4, 13, 10, 87, 80, 89, 94, 75, 76, 69, 66, 111, 104, 97, 102, 115, 116, 125, 122, 137, 142, 135, 128, 149, 146, 155, 156, 177, 182, 191, 184, 173, 170, 163, 164, 249, 254, 247, 240, 229, 226, 235, 236, 193, 198, 207, 200, 221, 218, 211, 212, 105, 110, 103, 96, 117, 114, 123, 124, 81, 86, 95, 88, 77, 74, 67, 68, 25, 30, 23, 16, 5, 2, 11, 12, 33, 38, 47, 40, 61, 58, 51, 52, 78, 73, 64, 71, 82, 85, 92, 91, 118, 113, 120, 127, 106, 109, 100, 99, 62, 57, 48, 55, 34, 37, 44, 43, 6, 1, 8, 15, 26, 29, 20, 19, 174, 169, 160, 167, 178, 181, 188, 187, 150, 145, 152, 159, 138, 141, 132, 131, 222, 217, 208, 215, 194, 197, 204, 203, 230, 225, 232, 239, 250, 253, 244, 243};


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

unsigned char calculaCRC(unsigned char *message, unsigned int tamanho, unsigned char *table){
    unsigned char crc = 0x00;
    for (int i = 0; i < tamanho; i++){
        crc = table[crc ^ message[i]];
    }
    return crc;
}

int insere_vlan(unsigned char *dados){
    int contador = 0;
    for (int i = 0; i < TAMANHO-1; i++){
        if (dados[i] == 129 || dados[i] == 136){
            for (int j = TAMANHO-2; j > i; j--){
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
    for (int i = 1; i < TAMANHO; i++){
        if ((dados[i-1] == 129 || dados[i-1] == 136) && dados[i] == 255){
            for (int j = i; j < TAMANHO-1; j++){
                dados[j] = dados[j+1];
            }
            contador++;
        }
    }
    return contador;
}

unsigned char *monta_buffer(unsigned int sequencia, unsigned int tipo, unsigned char *dados, unsigned int tamanho){
    unsigned char *buffer = (unsigned char*) malloc(sizeof(protocolo_t));
    protocolo_t *pacote = (protocolo_t*) buffer;
    pacote->marcador = 126;
    pacote->tamanho = tamanho;
    pacote->sequencia = sequencia;
    pacote->tipo = tipo;
    memset(pacote->dados, 0, TAMANHO);
    memcpy(pacote->dados, dados, tamanho);
    pacote->crc = calculaCRC(&buffer[1], sizeof(protocolo_t) - 2, tabela_crc);
    memcpy(ultimo_enviado, buffer, sizeof(protocolo_t));
    return buffer;

}

int envia_buffer(int soquete, unsigned int sequencia, unsigned int tipo, unsigned char* dados, unsigned int tamanho){
    unsigned char *buffer = monta_buffer(sequencia, tipo, dados, tamanho);
    int enviado = send(soquete, buffer, sizeof(protocolo_t), 0);
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
    struct timeval timeout = {5000/1000, (5000%1000)*1000};
    setsockopt(soquete, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    int bytes_recebidos = 0;
    do{
        bytes_recebidos = recv(soquete, buffer, sizeof(protocolo_t), 0);
        if (buffer_eh_valido((protocolo_t*) buffer)){
            return bytes_recebidos;
        }
    } while (timestamp() - comeco <= 5000);
    return -1;
}

int recebe_buffer(int soquete, protocolo_t *pacote, unsigned int *last_seq){
    unsigned char *buffer = (unsigned char*) malloc(sizeof(protocolo_t));
    unsigned int seq_esperada = inc_seq(last_seq);
    struct timeval timeout = {0, 0};
    setsockopt(soquete, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    protocolo_t *pacote_recebido = (protocolo_t*) buffer;
    recv(soquete, buffer, sizeof(protocolo_t), 0);
    while (!buffer_eh_valido(pacote_recebido)){
        recv(soquete, buffer, sizeof(protocolo_t), 0);
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

protocolo_t *recebe_confirmacao(int soquete, unsigned int *last_seq){
    unsigned char *buffer = (unsigned char*) malloc(sizeof(protocolo_t));
    protocolo_t *pacote = (protocolo_t*) buffer;
    int recebido = recebe_msg(soquete, buffer);
    if (recebido == -1) {
        pacote->tipo = TIMEOUT;
        return pacote;
    }
    inc_seq(last_seq);
    memcpy(ultimo_recebido, buffer, sizeof(protocolo_t));
    if (pacote->tipo == ERRO){
        int erro;
        sscanf ((char *) pacote->dados, "%d", &erro);
        switch (erro){
            case 1:
                fprintf(stderr, "Permissao negada\n");
                break;
            case 2:
                fprintf(stderr, "Arquivo nao encontrado\n");
                break;
            case 4:
                fprintf(stderr, "Sem espaço\n");
                break;
            default:
                fprintf(stderr, "Erro desconhecido\n");
                break;
        }
        free(buffer);
        exit(erro);
    }
    if (pacote->tipo == ACK || pacote->tipo == NACK){
        return pacote;
    }
    pacote->tipo = ERRO;
    return pacote;
}

void trata_envio(int soquete, unsigned int *sequencia, unsigned int tipo, unsigned char *dados, unsigned int tamanho, unsigned int *last_seq){
    envia_buffer(soquete, inc_seq(sequencia), tipo, dados, tamanho);
    protocolo_t *pacote = recebe_confirmacao(soquete, last_seq);
    int aceito = pacote->tipo;
    free(pacote);
    while (aceito != ACK){
        if (aceito == ERRO){
            fprintf(stderr, "Erro desconhecido\n");
            exit(1);
        }
        envia_buffer(soquete, *sequencia, tipo, dados, tamanho);
        pacote = recebe_confirmacao(soquete, last_seq);
        aceito = pacote->tipo;
        free(pacote);
    }
}
