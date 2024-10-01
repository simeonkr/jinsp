#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include "term.h"
#include "str.h"

int term_h, term_w;

typedef struct {
	int top, left;
	int rows, cols;
	string *left_rows, *right_rows;
} panel;
panel *panels;

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

void draw_panel(panel *p) {
	for (int n = 0; n < p->rows; n++) {
		printf(CUP("%d", "%d"), p->top + n, p->left);
		string row = p->left_rows[n];
		assert (row.size <= p->cols);
		printf(row.data);
	}
}

void draw() {
	for (panel *p = panels; p != NULL; p++) {
		draw_panel(p);
	}
}

void on_resize() {
	// TODO: update term_w and term_h first

	draw();
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