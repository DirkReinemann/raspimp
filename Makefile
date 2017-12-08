CC=gcc
CFLAGS=-g3 -std=c11 -Wall -Wextra -Werror -D_GNU_SOURCE `pkg-config --cflags --libs gtk+-3.0` `pkg-config --cflags --libs gstreamer-1.0` `pkg-config --cflags --libs sqlite3` -rdynamic
SOURCES=raspimp.c keyboard.c
BIN=raspimp

all: compile

compile:
	$(CC) $(CFLAGS) $(SOURCES) -o $(BIN)

clean:
	rm -f *.o $(BIN) *~ *.out

run: compile
	./raspimp

install: compile
	mkdir -p /usr/share/raspimp
	cp raspimp.glade /usr/share/raspimp/raspimp.glade
	cp raspimp.sql /usr/share/raspimp/raspimp.sql
	cp raspimp.css /usr/share/raspimp/raspimp.css
	cp keyboard.glade /usr/share/raspimp/keyboard.glade
	cp pause.png /usr/share/raspimp/pause.png
	cp play.png /usr/share/raspimp/play.png
	cp stop.png /usr/share/raspimp/stop.png
	cp shutdown.png /usr/share/raspimp/shutdown.png
	cp wlan.png /usr/share/raspimp/wlan.png
	killall raspimp; sleep 1; cp raspimp /usr/bin/raspimp

uninstall:
	rm -rf /usr/share/raspimp
	killall raspimp; sleep 1; rm /usr/bin/raspimp
