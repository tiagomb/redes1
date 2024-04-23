// Server side C program to demonstrate Socket
// programming
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <net/ethernet.h>
#include <linux/if.h>
#include <linux/if_packet.h>
#include <unistd.h>
#include "conexao.h"
#include "header.h"
#include "buffer.h"
#include <dirent.h>

int lista_videos(int soquete, int sequencia){
    DIR* diretorio = opendir("./videos");
    char nome[63] = { 0 };
    struct dirent* entrada = NULL;
    int aceito = 0;
    while ((entrada = readdir(diretorio)) != NULL){
        if (strcmp(entrada->d_name, ".") && strcmp(entrada->d_name, "..")){
            snprintf(nome, 63, "%s", entrada->d_name);
            if (sequencia == 31){
                sequencia = 0;
                aceito = envia_buffer(soquete, sequencia++, 16, nome, strlen(nome));
                if (aceito == 1){
                    sequencia--;
                    while (aceito){
                        aceito = envia_buffer(soquete, sequencia, 16, nome, strlen(nome));
                    }
                    sequencia++;
                }
            } else {
                aceito = envia_buffer(soquete, sequencia++, 16, nome, strlen(nome));
                if (aceito == 1){
                    sequencia--;
                    while (aceito){
                        aceito = envia_buffer(soquete, sequencia, 16, nome, strlen(nome));
                    }
                    sequencia++;
                }
            }
        }   
    }
    aceito = envia_buffer(soquete, sequencia++, 30, NULL, 0);
    if (aceito == 1){
        sequencia--;
        while (aceito == 1){
            aceito = envia_buffer(soquete, sequencia, 30, NULL, 0);
        }
        sequencia++;
    }
    closedir(diretorio);
}

int main(int argc, char const* argv[]){
    int soquete = cria_raw_socket("enp5s0");
    int sequencia = 0;
	lista_videos(soquete, sequencia);
    close(soquete);
    return 0;
}
