CC=gcc
CFLAGS=-g3 -std=c11 -Wall -Wextra -Werror -D_GNU_SOURCE `pkg-config --cflags --libs gtk+-3.0` `pkg-config --cflags --libs gstreamer-1.0` `pkg-config --cflags --libs sqlite3` -rdynamic
SOURCES=raspimp.c keyboard.c
BIN=raspimp
INSTALL=/usr/share/raspimp
FILES=wlan0.png wlan1.png wlan2.png wlan3.png wlan4.png wlan5.png raspimp.glade raspimp.sql raspimp.css keyboard.glade pause.png play.png stop.png shutdown.png screen.png

all: compile

compile:
	$(CC) $(CFLAGS) $(SOURCES) -o $(BIN)

clean:
	rm -f *.o $(BIN) *~ *.out

run: compile
	./raspimp

install: compile
	mkdir -p $(INSTALL)
	@$(foreach file, $(FILES), cp $(file) $(INSTALL)/$(file);)
	killall raspimp; sleep 1; cp raspimp /usr/bin/raspimp

uninstall:
	rm -rf $(INSTALL)
	killall raspimp; sleep 1; rm /usr/bin/raspimp
