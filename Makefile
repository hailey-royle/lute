le:
	gcc -o le le.c -Og -Wall -Werror -Wextra -Wpedantic -fanalyzer -fsanitize=address
run:
	gcc -o le le.c -Og -Wall -Werror -Wextra -Wpedantic -fanalyzer -fsanitize=address
	./le le.c
	rm le
test:
	gcc -o test.out test.c -Og -Wall -Wextra -Wpedantic -fanalyzer -fsanitize=address
	./test.out
	rm test.out
clean:
	rm le
