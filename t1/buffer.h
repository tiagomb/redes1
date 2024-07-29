#ifndef buffer___h_
#define buffer___h_

#include "header.h"

extern unsigned char ultimo_enviado[67];
extern unsigned char ultimo_recebido[67];

unsigned int inc_seq(unsigned int *sequencia);

unsigned int dec_seq(unsigned int *sequencia);

int insere_vlan(unsigned char *dados);

int remove_vlan(unsigned char *dados);

unsigned char calculaCRC(unsigned char *message, unsigned int tamanho, unsigned char *table);

unsigned char *monta_buffer(unsigned int sequencia, unsigned int tipo, unsigned char *dados, unsigned int tamanho);

int envia_buffer(int soquete, unsigned int sequencia, unsigned int tipo, unsigned char* dados, unsigned int tamanho);

int recebe_buffer(int soquete, protocolo_t *pacote, unsigned int *last_seq, unsigned int timeoutSec);

protocolo_t *recebe_confirmacao(int soquete, unsigned int *last_seq);

void trata_envio(int soquete, unsigned int *sequencia, unsigned int tipo, unsigned char *dados, unsigned int tamanho, unsigned int *last_seq);

#endif //buffer___h_