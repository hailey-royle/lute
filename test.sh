gcc test/test.c -Og -Wall -Wextra -pedantic -fanalyzer -fsanitize=address
./a.out
rm a.out
