#ifndef buffer___h_
#define buffer___h_

#include "header.h"

unsigned int inc_seq(unsigned int *sequencia);

unsigned int dec_seq(unsigned int *sequencia);

int envia_buffer(int soquete, unsigned int sequencia, unsigned int tipo, unsigned char* dados, unsigned int tamanho, unsigned int *last_seq);

int recebe_buffer(int soquete, protocolo_t *pacote, unsigned int *last_seq);

#endif //buffer___h_