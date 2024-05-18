#ifndef buffer___h_
#define buffer___h_

#include "header.h"

extern char ultimo_enviado[67];
extern char ultimo_recebido[67];

unsigned int inc_seq(unsigned int *sequencia);

unsigned int dec_seq(unsigned int *sequencia);

int insere_vlan(unsigned char *dados);

int remove_vlan(unsigned char *dados);

int envia_buffer(int soquete, unsigned int sequencia, unsigned int tipo, unsigned char* dados, unsigned int tamanho, unsigned int *last_seq);

int recebe_buffer(int soquete, protocolo_t *pacote, unsigned int *last_seq);

#endif //buffer___h_