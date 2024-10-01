CC = clang
CFLAGS = -std=c99 -Wall -fsave-optimization-record -O3 -g
OBJFILES = json.o parse.o print.o

test: test.o $(OBJFILES)
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) ${CFLAGS} -c $< -o $@

clean:
	rm *.o
