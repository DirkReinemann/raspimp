CC=gcc
CFLAGS=-g3 -std=c11 -Wall -Wextra -Werror -D_GNU_SOURCE `pkg-config --cflags --libs gtk+-3.0` `pkg-config --cflags --libs gstreamer-1.0` `pkg-config --cflags --libs sqlite3` -rdynamic
SOURCES=raspimp.c keyboard.c
BIN=raspimp

all: compile

compile: clean
	$(CC) $(CFLAGS) $(SOURCES) -o $(BIN)

clean:
	rm -f *.o $(BIN) *~ *.out

run: compile
	./raspimp

install: compile
	mkdir -p ~/.config/raspimp
	mkdir -p ~/music
	cp raspimp.glade ~/.config/raspimp/raspimp.glade
	cp raspimp.db ~/.config/raspimp/raspimp.db
	cp raspimp.css ~/.config/raspimp/raspimp.css
	cp keyboard.glade ~/.config/raspimp/keyboard.glade
	cp pause.png ~/.config/raspimp/pause.png
	cp play.png ~/.config/raspimp/play.png
	cp stop.png ~/.config/raspimp/stop.png
	cp shutdown.png ~/.config/raspimp/shutdown.png
	sudo -v && killall raspimp ; sudo cp raspimp /usr/bin/raspimp

uninstall:
	rm -rf ~/.config/raspimp
	sudo -v
	killall raspimp && sudo rm /usr/bin/raspimp
