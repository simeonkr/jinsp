#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <assert.h>
#include "term.h"
#include "str.h"

FILE *trace;

struct termios saved_term;

int win_rows, win_cols;

#define MAX_PANELS 11

int num_panels;

typedef struct {
	int top, left;
	int rows, cols;
	string *left_rows, *right_rows;
	int cur_row;
} panel;
panel panels[MAX_PANELS];

void term_setup() {
	printf(ALT_BUFF_EN);
	printf(CURS_HIDE);
	struct termios term;
	tcgetattr(fileno(stdin), &term);
	saved_term = term;
	term.c_lflag &= ~ECHO;
	term.c_lflag &= ~ICANON;
	tcsetattr(fileno(stdin), 0, &term);
}

void loop() {
	while (1) {
		char in = getchar();
		fprintf(trace, "getchar1(): %x\n", in);
		fflush(trace);
		if (in == '\x1b') {
			in = getchar();
			fprintf(trace, "getchar2(): %x\n", in);
			fflush(trace);
			switch (in) {
				case EOF: // ESC key
					return;
				case KEY_DOWN:
					panels[0].cur_row++;
			}

		}
	}
}

void draw_panel(panel *p) {
	fprintf(trace, "top: %d, left: %d, rows: %d, cols: %d\n",
		p->top, p->left, p->rows, p->cols);
	for (int n = 0; n < p->rows; n++) {
		string row = p->left_rows[n];
		if (row.size > 0) {
			assert(row.size <= p->cols);
			printf(CUP("%d", "%d"), p->top + n + 1, p->left + 1);
			if (p->cur_row == n)
				printf(FMT_BG_RED "%s" FMT_FG_WHITE, row.data);
			else
				printf(FMT_BG_BLACK "%s" FMT_FG_WHITE, row.data);
			printf(FMT_RESET);
		}
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
	ioctl(fileno(stdin), TIOCGWINSZ, &wsize);
	win_rows = wsize.ws_row;
	win_cols = wsize.ws_col;

	num_panels = 3;
	int panel_cols = win_cols / num_panels;
	int rem = win_cols - panel_cols * num_panels;

	fprintf(trace, "panel_cols: %d\n", panel_cols);

	for (int i = 0, col = 0; i < num_panels; col += panels[i].cols, i++) {
		panel *p = &panels[i];
		p->top = 0;
		p->left = col;
		p->rows = win_rows - 2;
		p->cols = panel_cols + (i < rem);
		p->left_rows = realloc(p->left_rows, p->rows * sizeof(string));
		for (int i = 0; i < p->rows; i++) {
			p->left_rows[i] = mk_empty_string();
		}
	}

	string_append(&panels[0].left_rows[0], "panel 1");
	string_append(&panels[0].left_rows[1], "...");
	string_append(&panels[0].left_rows[2], "...");
	string_append(&panels[1].left_rows[0], "panel 2");
	string_append(&panels[1].left_rows[1], "...");
	string_append(&panels[2].left_rows[0], "panel 3");
	string_append(&panels[2].left_rows[1], "...");

	// status bar
	num_panels += 1;
	panel *p = &panels[num_panels - 1];
	p->top = win_rows - 1;
	p->left = 0;
	p->rows = 1;
	p->cols = win_cols;
	p->left_rows = realloc(p->left_rows, p->rows * sizeof(string));
	p->left_rows[0] = mk_string("Hello world!");
	p->cur_row = -1;

	redraw();
}

void on_int(int i) {
	exit(128 + SIGINT);
}

void on_term(int i) {
	exit(128 + SIGTERM);
}

void fin() {
	tcsetattr(fileno(stdin), 0, &saved_term);
	printf(CURS_SHOW);
	printf(ALT_BUFF_DIS);
	fclose(trace);
}

int main() {
	trace = fopen("trace.txt", "w");
	atexit(fin);
	signal(SIGINT, on_term);
	signal(SIGTERM, on_term);
	signal(SIGWINCH, on_resize);
	term_setup();
	on_resize();
	loop();
}
