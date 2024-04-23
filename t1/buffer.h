#ifndef buffer___h_
#define buffer___h_

#include "header.h"

int envia_buffer(int soquete, unsigned int sequencia, unsigned int tipo, unsigned char* dados, unsigned int tamanho);

int recebe_buffer(int soquete, protocolo_t *pacote);

#endif //buffer___h_