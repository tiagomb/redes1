#ifndef header___h_
#define header___h_

typedef struct __attribute__((__packed__)) protocolo{
    unsigned int marcador: 8;
    unsigned int tamanho: 6;
    unsigned int sequencia: 5;
    unsigned int tipo: 5;
    unsigned char dados[63];
    unsigned int crc: 8;
} protocolo_t;

#define TIMEOUT 60000
#define ACK 0
#define NACK 1
#define IGNORAR 2
#define LISTA 6
#define BAIXAR 7
#define MOSTRAR 16
#define DESCRITOR 17
#define DADOS 18
#define FIM_TRANSMISSAO 30
#define ERRO 31

#endif //header___h_
