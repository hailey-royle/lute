gcc le.c -Wall -Werror -Wextra -pedantic -fanalyzer -fsanatize=address
./a.out $1
rm a.out
