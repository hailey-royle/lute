SHELL := /bin/bash

build: lute.c
	time gcc lute.c -o lute -std=c99 -O3 -Wall -Wextra -Wpedantic
run: build
	./lute readme.md
	stty sane
debug:
	time gcc lute.c -o lute -std=c99 -ggdb -Wall -Wextra -Wpedantic -fanalyzer -fsanitize=address,leak,undefined
prof:
	time gcc lute.c -o lute -std=c99 -Og -ggdb -pg -Wall -Wextra -Wpedantic -fanalyzer -fsanitize=address,leak,undefined
	./lute README.md
	gprof lute gmon.out > prof.out
	less prof.out
	rm prof.out gmon.out
