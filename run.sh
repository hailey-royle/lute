gcc le.c -Og -Wall -Wextra -Werror -pedantic -fanalyzer -fsanitize=address
./a.out $1
rm a.out
