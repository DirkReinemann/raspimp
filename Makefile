CC=gcc
CFLAGS=-g3 -std=c11 -Wall -Wextra -Werror `pkg-config --cflags --libs gtk+-3.0`  `pkg-config --cflags --libs gstreamer-1.0` `pkg-config --cflags --libs sqlite3` -rdynamic
SOURCES=raspimp.c
BIN=raspimp.o

all: compile

compile:
	$(CC) $(CFLAGS) $(SOURCES) -o $(BIN)

clean:
	rm *.o
