gcc src/le.c -Wall -Werror -Wextra -pedantic -fanalyzer -fsanitize=address
./a.out $1
rm a.out
