le:
	gcc -o le le.c -Wall -Werror -Wextra -Wpedantic -fanalyzer -fsanitize=address
	./le le.c
	rm le
build:
	gcc -o le le.c -Wall -Werror -Wextra -Wpedantic -fanalyzer -fsanitize=address
test:
	gcc -o test.out test.c -Wall -Wextra -Wpedantic -fanalyzer -fsanitize=address
	./test.out
	rm test.out
