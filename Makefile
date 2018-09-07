CFLAGS= -Wall -fsanitize=address
LDLIBS=-lSDL2 -lGLEW -lGL -lm
CC=gcc

all: utils.o pong.o
	$(CC) $^ $(LDLIBS) $(CFLAGS) -o pong

%.o: %.c
	$(CC) -c $(LDLIBS) $(CFLAGS) $^

