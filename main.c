#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <assert.h>
#include "term.h"
#include "str.h"
#include "json.h"
#include "parse.h"
#include "display.h"
#include "trace.h"

FILE *input;

int term_initialized;
struct termios saved_term;

int win_rows, win_cols;

#define MAX_PANELS 11
panel panels[MAX_PANELS];
int num_panels, active_panel;

void term_setup() {
	printf(ALT_BUFF_EN);
	printf(CURS_HIDE);
	struct termios term;
	tcgetattr(STDIN_FILENO, &term);
	saved_term = term;
	term.c_lflag &= ~ECHO;
	term.c_lflag &= ~ICANON;
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &term);
	term_initialized = 1;
}

void draw_panel(panel *p) {
	TRACE("top: %d, left: %d, rows: %d, cols: %d\n",
		p->top, p->left, p->rows, p->cols);
	for (int n = 0; n < p->rows; n++) {
		string row = p->left_rows[n];
		if (row.size > 0) {
			assert(row.size <= p->cols);
			printf(CUP("%d", "%d"), p->top + n + 1, p->left + 1);
			if (p->cur_row == n)
				printf(FMT_BOLD FMT_BG_RED FMT_FG_WHITE "%s", row.data);
			else
				printf(FMT_BG_BLACK FMT_FG_WHITE "%s", row.data);
			printf(FMT_RESET);
		}
	}
}

void redraw() {
	printf(ED("2"));
	for (int i = 0; i < num_panels; i++) {
		draw_panel(&panels[i]);
	}
	fflush(STDIN_FILENO);
}

void on_resize() {
	struct winsize wsize;
	ioctl(STDIN_FILENO, TIOCGWINSZ, &wsize);
	win_rows = wsize.ws_row;
	win_cols = wsize.ws_col;

	num_panels = 3;
	int panel_cols = win_cols / num_panels;
	int rem = win_cols - panel_cols * num_panels;

	TRACE("panel_cols: %d\n", panel_cols);

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
	if (term_initialized) {
		tcsetattr(STDIN_FILENO, 0, &saved_term);
		printf(CURS_SHOW);
		printf(ALT_BUFF_DIS);
	}
#ifdef DEBUG
	if (trace)
		fclose(trace);
#endif
}

#define READ_BUF_SIZE 4

void loop() {
	fflush(STDIN_FILENO); // TODO: why is this necessary?
	while (1) {
		char in[READ_BUF_SIZE];
		int num_read = read(STDIN_FILENO, &in, READ_BUF_SIZE);
		if (in[0] == '\x1b') {
			if (num_read == 1) // ESC key
				return;
			if (num_read == 3 && in[1] == '[') {
				switch (in[2]) {
					case KEY_DOWN:
						if (panels[0].cur_row < panels[0].rows - 1)
							panels[0].cur_row++;
						redraw();
						break;
					case KEY_UP:
						if (panels[0].cur_row > 0)
							panels[0].cur_row--;
						redraw();
						break;
				}
			}
		}
	}
}

int main(int argc, char **argv) {
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <JSON input file>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

  	if(!isatty(STDIN_FILENO)){
      fprintf(stderr, "Not a terminal.\n");
      exit(EXIT_FAILURE);
    }

	atexit(fin);
	signal(SIGINT, on_term);
	signal(SIGTERM, on_term);

#ifdef DEBUG
	trace = fopen("trace.txt", "w");
#endif

	FILE *f = fopen(argv[1], "r");
	if (!f) {
		fprintf(stderr, "Error reading input file.");
		exit(EXIT_FAILURE);
	}
    parse_result pr = parse_json(f);
	fclose(f);
    if (!pr.success) {
		print_error(stderr, pr);
		exit(EXIT_FAILURE);
    }
	json_value top = pr.res;

	term_setup();
	signal(SIGWINCH, on_resize);
	on_resize();

	display_json(&panels[0], top);

	loop();
}
