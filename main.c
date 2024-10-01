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
#include "stack.h"
#include "trace.h"
#include "util.h"

#ifdef DEBUG
FILE *trace;
#endif
const char *input_filename;
FILE *input;

int term_initialized;
struct termios saved_term;

typedef struct {
    int top, left;
    int nrows, ncols;
    string *rows;
} pane;

#define NUM_VIEW_PANES 3
#define NUM_PANES (NUM_VIEW_PANES + 1)
struct {
    int nrows, ncols;
    union {
        struct {
            pane status_bar;
            pane view_panes[NUM_VIEW_PANES];
        };
        pane panes[NUM_PANES];
    };
} window;

json_stack stack;

void term_setup() {
    printf(ALT_BUF_EN);
    printf(CURS_HIDE);
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    saved_term = term;
    term.c_lflag &= ~ECHO;
    term.c_lflag &= ~ICANON;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &term);
    term_initialized = 1;
}

void pane_resize() {
    struct winsize wsize;
    ioctl(STDIN_FILENO, TIOCGWINSZ, &wsize);
    window.nrows = wsize.ws_row;
    window.ncols = wsize.ws_col;

    int pane_cols = window.ncols / NUM_VIEW_PANES;
    int rem = window.ncols - pane_cols * NUM_VIEW_PANES;

    TRACE("pane_cols: %d\n", pane_cols);

    // status bar
    pane *sb = &window.status_bar;
    sb->top = window.nrows - 1;
    sb->left = 0;
    sb->nrows = 1;
    sb->ncols = window.ncols;
    sb->rows = realloc(sb->rows, sizeof(string));
    sb->rows[0] = mk_empty_string();

    for (int i = 0, col = 0; i < NUM_VIEW_PANES;
         col += window.view_panes[i].ncols, i++) {
        pane *p = &window.view_panes[i];
        p->top = 0;
        p->left = col;
        p->nrows = window.nrows - 2;
        p->ncols = pane_cols + (i < rem);
        p->rows = realloc(p->rows, p->nrows * sizeof(string));
        for (int j = 0; j < p->nrows; j++)
            p->rows[j] = mk_empty_string();
    }
}

void populate_view(pane *p, json_pos pos) {
    json_value value = pos.value;
    int index = pos.index;
    int scroll_lim = p->nrows / 2 + 1;
    int off = index <= scroll_lim ? 0 : index - scroll_lim;
    int curs_ri = min(index, scroll_lim);
    switch (pos.value.kind) {
        case OBJECT:
            for (int ri = 0, di = off;
                 ri < p->nrows && di < object_size(value.object);
                 ri++, di++) {
                if (ri == curs_ri)
                    string_printf(&p->rows[ri],
                        FMT_BOLD FMT_BG_RED FMT_FG_WHITE "%s" FMT_RESET,
                        object_get(value.object, di).key);
                else
                    string_printf(&p->rows[ri], 
                        FMT_BG_BLACK FMT_FG_WHITE "%s" FMT_RESET,
                        object_get(value.object, di).key);
            }
            break;
        case ARRAY:
            for (int ri = 0, di = off;
                 ri < p->nrows && di < array_size(value.object);
                 ri++, di++) {
                if (ri == curs_ri)
                    string_printf(&p->rows[ri],
                        FMT_BOLD FMT_BG_RED FMT_FG_WHITE "%d" FMT_RESET, 
                        di);
                else
                    string_printf(&p->rows[ri], 
                        FMT_BG_BLACK FMT_FG_WHITE "%d" FMT_RESET, di);
                printf(FMT_RESET);
            }
            break;
        case STRING:
            string_printf(&p->rows[0], value.string);
            break;
        case NUMBER:
            string_printf(&p->rows[0], "%f", value.number);
            break;
        case TRUE:
            string_printf(&p->rows[0], "true");
            break;
        case FALSE:
            string_printf(&p->rows[0], "false");
            break;
        case NUL:
            string_printf(&p->rows[0], "null");
            break;
    }
}

void draw_pane(pane *p) {
    TRACE("top: %d, left: %d, nrows: %d, ncols: %d\n",
        p->top, p->left, p->nrows, p->ncols);
    for (int n = 0; n < p->nrows; n++) {
        string row = p->rows[n];
        if (row.size > 0) {
            printf(CUP("%d", "%d"), p->top + n + 1, p->left + 1);
            printf("%s", row.data);
        }
    }
}

void draw() {
    // fill each pane with corresponding data
    string_printf(&window.status_bar.rows[0], "%s", input_filename);
    TRACE("%s\n", window.status_bar.rows[0].data);
    assert(stack.size >= 1);
    int num_view_panes = min(stack.size, NUM_VIEW_PANES);
    for (int i = 0; i < num_view_panes; i++) {
        if (i >= stack.size)
            break;
        populate_view(&window.view_panes[i],
            *stack_peekn(&stack, num_view_panes - 1 - i));
    }

    printf(ED("2"));
    for (int i = 0; i < NUM_PANES; i++) {
        draw_pane(&window.panes[i]);
    }
    fflush(STDIN_FILENO);
}

void move_to_parent() {
    TRACE("move_to_parent: stack.size = %d\n", stack.size);
    if (stack.size > 1)
        stack_pop(&stack);
}

void move_to_child() {
    TRACE("move_to_child: stack.size = %d\n", stack.size);
    json_pos *cur = stack_peek(&stack);
    switch (cur->value.kind) {
        json_value next;
        case OBJECT:
            next = object_get(cur->value.object, cur->index).val;
            stack_push(&stack, (json_pos){ next, 0 });
            break;
        case ARRAY:
            next = array_get(cur->value.object, cur->index);
            stack_push(&stack, (json_pos){ next, 0 });
            break;
        default:
            break;
    }
}

void move_to_prev() {
    TRACE("move_to_prev: stack.size = %d\n", stack.size);
    if (stack.size == 1)
        return;
    json_pos *cur = stack_peekn(&stack, 1);
    cur->index = max(cur->index - 1, 0);
}

void move_to_next() {
    TRACE("move_to_next: stack.size = %d\n", stack.size);
    if (stack.size == 1)
        return;
    json_pos *cur = stack_peekn(&stack, 1);
    int max_idx = 0;
    switch (cur->value.kind) {
        case OBJECT:
            max_idx = object_size(cur->value.object) - 1;
            break;
        case ARRAY:
            max_idx = array_size(cur->value.array) - 1;
            break;
        default:
            break;
    }
    cur->index = min(cur->index + 1, max_idx);
}

void loop() {
    fflush(STDIN_FILENO); // TODO: why is this necessary?
    while (1) {
        char in[4];
        int num_read = read(STDIN_FILENO, &in, 4);
        switch (in[0]) {
            case '\x1b':
                if (num_read == 1) // ESC key
                    return;
                if (num_read == 3 && in[1] == '[') {
                    switch (in[2]) {
                        case KEY_UP:
                            move_to_prev();
                            draw();
                            break;
                        case KEY_DOWN:
                            move_to_next();
                            draw();
                            break;
                        case KEY_RIGHT:
                            move_to_child();
                            draw();
                            break;
                        case KEY_LEFT:
                            move_to_parent();
                            draw();
                            break;
                    }
                }
                break;
            case 'q':
                return;
        }
    }
}


void on_int(int i) {
    exit(128 + SIGINT);
}

void on_term(int i) {
    exit(128 + SIGTERM);
}

void on_resize() {
    pane_resize();
    draw();
}

void fin() {
    if (term_initialized) {
        tcsetattr(STDIN_FILENO, 0, &saved_term);
        printf(CURS_SHOW);
        printf(ALT_BUF_DIS);
    }
#ifdef DEBUG
    if (trace)
        fclose(trace);
#endif
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <JSON input file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    input_filename = argv[1];

      if(!isatty(STDIN_FILENO)){
      fprintf(stderr, "Not a terminal.\n");
      exit(EXIT_FAILURE);
    }

    atexit(fin);
    signal(SIGINT, on_term);
    signal(SIGTERM, on_term);
#ifndef DEBUG
    signal(SIGABRT, on_term);
#endif

#ifdef DEBUG
    trace = fopen("trace.txt", "w");
#endif

    FILE *f = fopen(input_filename, "r");
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

    stack_push(&stack, (json_pos){top, 0});
    move_to_child();

    term_setup();
    pane_resize();
    signal(SIGWINCH, on_resize);

    draw();

    loop();
}
