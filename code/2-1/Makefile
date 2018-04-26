CC = gcc
TARGET = server 


all: server client

server : server.c
	$(CC) -o server server.c -lcrypto

client : client.c
	$(CC) -o client client.c -lcrypto

test:
	$(CC) -o server server.c -lcrypto -g
	$(CC) -o client client.c -g
