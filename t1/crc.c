#include "crc.h"
#include <stdio.h>

unsigned char calculaCRC(unsigned char *message, unsigned int tamanho, unsigned char *table){
    unsigned char crc = 0x00;
    for (int i = 0; i < tamanho; i++){
        crc = table[crc ^ message[i]];
    }
    return crc;
}