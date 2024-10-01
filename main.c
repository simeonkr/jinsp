#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include "term.h"

int term_w, term_h;

void term_setup() {
	printf(ALT_BUFF_EN);
	printf(ED("2"));
	printf(CUP("%d","%d"), 30, 30);
	printf(FMT_FG_RED FMT_BOLD "Hello " FMT_RESET "world!\n");
}

void loop() {
	char in;
	do {
		in = getchar();
	} while (in != '\x1b');
}

void on_resize() {

}

void on_int(int i) {
	exit(128 + SIGINT);
}

void on_term(int i) {
	exit(128 + SIGTERM);
}

void fin() {
	printf(ALT_BUFF_DIS);
}

int main() {
	atexit(fin);
	signal(SIGINT, on_term);
	signal(SIGTERM, on_term);
	signal(SIGWINCH, on_resize);
	term_setup();
	loop();
}