CC=gcc
CFLAGS=-g -Wall -Wextra -std=c11 -I${EMU_ROOT}

all: clean unit_tests

unit_tests:
	@$(CC) $(CFLAGS) -ounit_tests ./test/map.test.c

clean:
	@rm -f *.o unit_tests perf_tests