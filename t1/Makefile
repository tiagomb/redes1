CFLAGS = -g -Wall


server: server.o conexao.o buffer.o
	gcc -o server server.o conexao.o buffer.o

client: client.o conexao.o buffer.o
	gcc -o client client.o conexao.o buffer.o

server.o: server.c
	gcc -c server.c  $(CFLAGS)

client.o: client.c
	gcc -c client.c  $(CFLAGS)

conexao.o: conexao.c conexao.h
	gcc -c conexao.c  $(CFLAGS)

buffer.o: buffer.c buffer.h
	gcc -c buffer.c  $(CFLAGS)

removeObjects:
	rm *.o