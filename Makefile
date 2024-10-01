CC = clang
CFLAGS = -std=c99 -Wall -O3
OPT = /usr/lib/llvm-14/bin/opt
OBJFILES = src/buffer.o src/json.o src/parse.o src/print.o

jinsp: src/main.o $(OBJFILES)
	$(CC) $(CFLAGS) $^ -o $@

test: src/test.o $(OBJFILES)
	$(CC) $(CFLAGS) $^ -o $@

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

%.ll: %.c
	$(CC) $(CFLAGS) -S -emit-llvm -c $< -o $@
	$(OPT) --dot-cfg --disable-output --enable-new-pm=0 $@
	$(OPT) --dot-callgraph --disable-output --enable-new-pm=0 $@

clean:
	rm src/*.o src/*.ll src/*.dot src/*.dot src/*.opt.yaml src/*.objdump
