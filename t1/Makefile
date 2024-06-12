CFLAGS = -g -Wall


server: server.o crc.o conexao.o buffer.o
	gcc -o server server.o crc.o conexao.o buffer.o

client: client.o crc.o conexao.o buffer.o
	gcc -o client client.o crc.o conexao.o buffer.o

server.o: server.c
	gcc -c server.c  $(CFLAGS)

client.o: client.c
	gcc -c client.c  $(CFLAGS)

crc.o: crc.c crc.h
	gcc -c crc.c  $(CFLAGS)

conexao.o: conexao.c conexao.h
	gcc -c conexao.c  $(CFLAGS)

buffer.o: buffer.c buffer.h
	gcc -c buffer.c  $(CFLAGS)

removeObjects:
	rm *.o