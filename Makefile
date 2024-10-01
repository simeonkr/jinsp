CC = gcc
CFLAGS = -std=c99 -Wall -O0 -g -DDEBUG
OBJFILES = json.o parse.o print.o

test: test.o $(OBJFILES)
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) ${CFLAGS} -c $< -o $@

clean:
	rm *.o
