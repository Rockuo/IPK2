SRC=main.cpp
CFLAGS=-std=c11
NAME=chat_client

all: $(SRC)
	g++ -pthread -o $(NAME) $(SRC)


