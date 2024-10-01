CC = clang
CFLAGS = -std=c99 -Wall -O3 -gdwarf-4 -DDEBUG
OPT = /usr/lib/llvm-14/bin/opt
OBJFILES = str.o json.o parse.o print.o display.o

main: main.o $(OBJFILES)
	$(CC) $(CFLAGS) $^ -o $@

test: test.o $(OBJFILES)
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.ll: %.c
	$(CC) $(CFLAGS) -S -emit-llvm -c $< -o $@
	$(OPT) --dot-cfg --disable-output --enable-new-pm=0 $@
	$(OPT) --dot-callgraph --disable-output --enable-new-pm=0 $@

clean:
	rm *.o *.ll *.dot *.dot *.opt.yaml *.objdump
