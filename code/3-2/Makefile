CC = gcc
SRC = proxy_util.c proxy_cache.c
TARGET = proxy_cache 


all: proxy_cache

$(TARGET) : $(SRC)
	$(CC) -o $(TARGET) $(SRC) -lcrypto -pthread

test:
	$(CC) -o $(TARGET) $(SRC) -lcrypto -pthread -g
