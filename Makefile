SHELL := /bin/bash

build: lute.c
	time gcc lute.c -o lute -std=c99 -Og -ggdb -Wall -Wextra -Wpedantic -fanalyzer -fsanitize=address -fsanitize=leak -fsanitize=undefined
run: build
	./lute README.md
	stty sane
fast:
	time gcc lute.c -o lute -std=c99 -O3 -Wall -Wextra -Wpedantic -DNO_ASSERT
prof:
	gcc lute.c -o lute -Og -ggdb -pg -Wall -Wextra -Wpedantic -fanalyzer -fsanitize=address -fsanitize=leak -fsanitize=undefined
	./lute README.md
	gprof lute gmon.out > prof.out
	less prof.out
	rm prof.out gmon.out
