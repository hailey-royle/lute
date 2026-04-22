build: lute.c
	gcc lute.c -o lute -Og -ggdb -Wall -Wextra -Wpedantic -fanalyzer -fsanitize=address -fsanitize=leak -fsanitize=undefined
run: build
	./lute README.md
