build: lte.c
	gcc -o lte lte.c -Og -Wall -Werror -Wextra -Wpedantic -fanalyzer -fsanitize=address
run: build
	./lte README.md
prod: lte.c
	gcc -o lte lte.c -O3 -Wall -Werror -Wextra -Wpedantic
clean:
	rm lte
