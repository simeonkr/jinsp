CC = gcc
CFLAGS = -std=c99 -Wall -O3
OBJFILES = src/buffer.o src/json.o src/parse.o src/print.o

jinsp: src/main.o $(OBJFILES)
	$(CC) $(CFLAGS) $^ -o $@

test: src/test.o $(OBJFILES)
	$(CC) $(CFLAGS) $^ -o $@

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm src/*.o
