SHELL := /bin/bash
COMPILER := clang

build:
	$(COMPILER) lute.c -o lute -Wall -Wextra -O3 -std=c89

debug:
	$(COMPILER) lute.c -o lute -Wall -Wextra -ggdb -std=c89

keybind:
	$(COMPILER) keybind.c -o keybind -O3 -std=c89

clean:
	rm lute keybind

