COMPILER = gcc
DESTINATION = /usr/local/bin

build:
	${COMPILER} lute.c -o lute -Wall -Wextra -std=c89 -O3

install: build
	mkdir -p ${DESTINATION}
	cp -f lute ${DESTINATION}/lute

uninstall:
	rm -f ${DESTINATION}/lute

clean:
	rm lute keybind

keybind:
	${COMPILER} keybind.c -o keybind -std=c89 -O3
	./keybind

debug:
	${COMPILER} lute.c -o lute -Wall -Wextra -std=c89 -Og -ggdb

.PHONY: build install uninstall clean keybind debug

