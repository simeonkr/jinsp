#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <assert.h>
#include "term.h"
#include "str.h"

int win_rows, win_cols;

#define MAX_PANELS 11

int num_panels;

typedef struct {
	int top, left;
	int rows, cols;
	string *left_rows, *right_rows;
} panel;
panel panels[MAX_PANELS];

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
		assert(row.size <= p->cols);
		printf("%s", row.data);
	}
}

void redraw() {
	printf(ED("2"));
	for (int i = 0; i < num_panels; i++) {
		draw_panel(&panels[i]);
	}
}

void on_resize() {
	struct winsize wsize;
	ioctl(stdout, TIOCGWINSZ, &wsize);
	win_rows = wsize.ws_row;
	win_cols = wsize.ws_col;

	int panel_cols = win_cols / num_panels;
	int rem = win_cols - panel_cols * num_panels;
	num_panels = 3;

	for (int i = 0, col = 0; i < num_panels; i++, col += panels[i].cols) {
		panel *p = &panels[i];
		p->top = 0;
		p->left = col;
		p->rows = win_rows - 2;
		p->cols = panel_cols + (i < rem);
		p->left_rows = realloc(p->left_rows, p->rows * sizeof(string));
	}

	// status bar
	num_panels += 1;
	panel *p = &panels[num_panels - 1];
	p->top = win_rows - 1;
	p->left = 0;
	p->rows = 1;
	p->cols = win_cols;
	p->left_rows = realloc(p->left_rows, sizeof(string));
	p->left_rows[0] = mk_string("Hello world!");
	
	redraw();
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
	on_resize();
	loop();
}
