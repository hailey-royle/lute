build: lte.c
	gcc -o lte lte.c -Og -Wall -Werror -Wextra -Wpedantic -fanalyzer -fsanitize=address
run: build
	./lte readme.md
clean:
	rm lte
