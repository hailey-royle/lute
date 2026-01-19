lte: lte.c
	gcc -o lte lte.c -Og -Wall -Werror -Wextra -Wpedantic -fanalyzer -fsanitize=address
run:
	gcc -o lte lte.c -Og -Wall -Werror -Wextra -Wpedantic -fanalyzer -fsanitize=address
	./lte lte.c
	rm lte
test:
	gcc -o test.out test.c -Og -Wall -Wextra -Wpedantic -fanalyzer -fsanitize=address
	./test.out
	rm test.out
clean:
	rm lte
