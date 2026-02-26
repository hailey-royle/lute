build: lute.c
	gcc -o lute lute.c -Og -Wall -Werror -Wextra -Wpedantic -fanalyzer -fsanitize=address
run: build
	./lute README.md
prod: lute.c
	gcc -o lute lute.c -O3 -Wall -Werror -Wextra -Wpedantic
clean:
	rm lute
