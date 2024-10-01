#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <assert.h>
#include <locale.h>
#include <wchar.h>
#include "term.h"
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
    buffer *rows;
} pane;

#define NUM_VIEW_PANES 3
#define NUM_PANES (NUM_VIEW_PANES + 2)
struct {
    int nrows, ncols;
    union {
        struct {
            pane top_bar;
            pane status_bar;
            pane view_panes[NUM_VIEW_PANES];
        };
        pane panes[NUM_PANES];
    };
} window;

json_stack stack;

void term_setup() {
    // necessary for proper wcwidth() support
    setlocale(LC_ALL, "");

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

static void *recalloc(void *ptr, size_t elts, size_t size) {
    if (!ptr)
        ptr = calloc(elts, size);
    else
        ptr = realloc(ptr, elts * size);
    return ptr;
}

static void rowalloc(buffer *dest, int cols) {
    // up to 4 bytes per (unicode) character + 32 formatting characters
    unsigned capacity = 4 * cols + 32 + 1;
    buffer_realloc(dest, capacity);
}

void pane_resize() {
    struct winsize wsize;
    ioctl(STDIN_FILENO, TIOCGWINSZ, &wsize);
    window.nrows = wsize.ws_row;
    window.ncols = wsize.ws_col;

    TRACE("nrows: %d, ncols: %d\n", window.nrows, window.ncols);

    int pane_cols = window.ncols / NUM_VIEW_PANES;
    int rem = window.ncols - pane_cols * NUM_VIEW_PANES;

    // top bar
    pane *tb = &window.top_bar;
    tb->top = 0;
    tb->left = 0;
    tb->nrows = 1;
    tb->ncols = window.ncols;
    tb->rows = recalloc(tb->rows, 1, sizeof(buffer));
    rowalloc(&tb->rows[0], tb->ncols);

    // status bar
    pane *sb = &window.status_bar;
    sb->top = window.nrows - 1;
    sb->left = 0;
    sb->nrows = 1;
    sb->ncols = window.ncols;
    sb->rows = recalloc(sb->rows, 1, sizeof(buffer));
    rowalloc(&sb->rows[0], sb->ncols);

    for (int i = 0, col = 0; i < NUM_VIEW_PANES;
         col += pane_cols + (i < rem), i++) {
        pane *p = &window.view_panes[i];
        p->top = 2;
        p->left = col;
        p->nrows = window.nrows - 4;
        p->ncols = pane_cols + (i < rem) - 1;
        p->rows = recalloc(p->rows, p->nrows, sizeof(buffer));
        for (int j = 0; j < p->nrows; j++)
            rowalloc(&p->rows[j], p->ncols);
    }
}

// https://en.wikipedia.org/wiki/UTF-8#Encoding
static inline wchar_t utf8_decode(const unsigned char *c) {
    assert  (c[0] >= 0b11000000);
    if      (c[0] < 0b11100000)
        return (c[0] & 0b00011111) << 6  |
               (c[1] & 0b00111111);
    else if (c[0] < 0b11110000)
        return (c[0] & 0b00001111) << 12 |
               (c[1] & 0b00111111) << 6  |
               (c[2] & 0b00111111);
    else
        return (c[0] & 0b00000111) << 18 |
               (c[1] & 0b00111111) << 12 |
               (c[2] & 0b00111111) << 6  |
               (c[3] & 0b00111111);
}

typedef struct {
    int cols, num_read;
} print_cols_r;

int wcwidth(wchar_t);

// returns the number of columns expected to be occupied
print_cols_r print_cols(buffer *dest, const char *src, int num_cols,
                        int escape) {
    assert (dest->raw_size >= 1 && dest->data[dest->raw_size - 1] == '\0');
    dest->raw_size--;

    int cols = 0;
    int i = 0;
    for (; src[i] != '\0' && cols < num_cols; ) {
        unsigned char c = src[i];
        if (escape && c == '\n') {
            buffer_putchar(dest, '\\');
            buffer_putchar(dest, 'n');
            i++;
            cols += 2;
        }
        else if (escape && c == '\r') {
            buffer_putchar(dest, '\\');
            buffer_putchar(dest, 'r');
            i++;
            cols += 2;
        }
        else if (escape && c == '\t') {
            buffer_putchar(dest, '\\');
            buffer_putchar(dest, 't');
            i++;
            cols += 2;
        }
        else if (c == '\n') {
            i++;
            break;
        }
        else if (c <= 0x7f) {
            buffer_putchar(dest, src[i++]);
            cols++;
        }
        else { // UTF-8
            wchar_t wc = utf8_decode(
                (const unsigned char *)&src[i]);
            if (c < 0xe0) {
                buffer_putchar(dest, src[i++]);
                buffer_putchar(dest, src[i++]);
            }
            else if (c < 0xf0) {
                buffer_putchar(dest, src[i++]);
                buffer_putchar(dest, src[i++]);
                buffer_putchar(dest, src[i++]);
            }
            else {
                buffer_putchar(dest, src[i++]);
                buffer_putchar(dest, src[i++]);
                buffer_putchar(dest, src[i++]);
                buffer_putchar(dest, src[i++]);
            }
            cols += wcwidth(wc);
        }
    }
    buffer_putchar(dest, '\0');
    return (print_cols_r){cols, i};
}

void print_cur_pos(buffer *dest, int cols) {
    string_nprintf(dest, 0, FMT_BOLD);
    for (int i = 0; i < stack.size - 1 && cols > 0; i++) {
        json_value value = stack.data[i].value;
        int index = stack.data[i].index;
        switch (value.kind) {
            case OBJECT: {
                const char* key = object_get(value.object, index).key;
                cols -= string_nprintf(dest, cols + 1, "%c", '.');
                cols -= print_cols(dest, key, cols, 1).cols;
                break;
            }
            case ARRAY:
                cols -= string_nprintf(dest, cols + 1, "[%d]", index);
                break;
            default:
                assert(0);
        }
    }
    string_nprintf(dest, 0, FMT_RESET);
}

int summarize_value(buffer *dest, json_value value, int cols) {
    switch (value.kind) {
        case OBJECT:
            return string_nprintf(dest, cols + 1,
                object_size(value.object) > 0 ? "{..}" : "{}");
        case ARRAY:
            return string_nprintf(dest, cols + 1,
                array_size(value.array) > 0 ? "[..]" : "[]");
        case STRING:
            return print_cols(dest, value.string, cols, 1).cols;
        case NUMBER:
            return string_nprintf(dest, cols + 1, "%f", value.number);
        case TRUE:
            return string_nprintf(dest, cols + 1, "true");
        case FALSE:
            return string_nprintf(dest, cols + 1, "false");
        case NUL:
            return string_nprintf(dest, cols + 1, "null");
    }
}

// prints an element when key == "", a member otherwise
void print_row(buffer *dest, const char* key, int index, json_value value,
               int cols, int selected) {
    if (selected)
        string_nprintf(dest, 0, FMT_BG_RED FMT_FG_WHITE FMT_BOLD);
    else
        string_nprintf(dest, 0, FMT_BG_BLACK FMT_FG_WHITE FMT_BOLD);

    if (strlen(key) == 0)
        cols -= string_nprintf(dest, cols + 1, "%d  ", index);
    else {
        cols -= print_cols(dest, key, cols, 1).cols;
        cols -= string_nprintf(dest, cols + 1, "  ");
    }
    assert(dest->data[dest->raw_size - 1] == '\0');

    string_nprintf(dest, 0, FMT_NOBOLD);

    if (cols > 0)
        cols -= summarize_value(dest, value, cols);
    assert(dest->data[dest->raw_size - 1] == '\0');

    if (cols > 0) {
        dest->raw_size--;
        for (; cols > 0; cols--)
            buffer_putchar(dest, ' ');
        buffer_putchar(dest, '\0');
    }

    string_nprintf(dest, 0, FMT_RESET);
}

void populate_view(pane *p, json_pos pos, int is_top) {
    json_value value = pos.value;
    int index = pos.index;
    int scroll_lim = p->nrows / 2 + 1;
    int off = index <= scroll_lim ? 0 : index - scroll_lim;
    int curs_ri = min(index, scroll_lim);
    switch (pos.value.kind) {
        case OBJECT:
            if (object_size(value.object) == 0) {
                assert(is_top);
                string_nprintf(&p->rows[0], p->ncols + 1 + 8,
                    FMT_ITALIC "<Empty object>" FMT_RESET);
                break;
            }
            for (int ri = 0, di = off;
                 ri < p->nrows && di < object_size(value.object);
                 ri++, di++) {
                json_member memb = object_get(value.object, di);
                print_row(&p->rows[ri], memb.key, di, memb.val, p->ncols,
                          !is_top && ri == curs_ri);
            }
            break;
        case ARRAY:
            if (array_size(value.array) == 0) {
                assert(is_top);
                string_nprintf(&p->rows[0], p->ncols + 1 + 8,
                    FMT_ITALIC "<Empty array>" FMT_RESET);
                break;
            }
            for (int ri = 0, di = off;
                 ri < p->nrows && di < array_size(value.array);
                 ri++, di++) {
                json_value elt = array_get(value.array, di);
                print_row(&p->rows[ri], "", di, elt, p->ncols,
                          !is_top && ri == curs_ri);
            }
            break;
        case STRING: {
            char *s = value.string;
            for (int i = 0, ri = 0; ri < p->nrows && s[i] != '\0'; ri++) {
                s += print_cols(&p->rows[ri], &s[i], p->ncols, 0).num_read;
            }
            break;
        }
        default:
            summarize_value(&p->rows[0], value, p->ncols + 1);
    }
}

void draw_pane(pane *p) {
    for (int n = 0; n < p->nrows; n++) {
        if (p->rows[n].raw_size > 1) {
            printf(CUP("%d", "%d"), p->top + n + 1, p->left + 1);
            printf("%s", p->rows[n].data);
        }
    }
}

void draw() {
    // clear existing data
    string_clear(&window.top_bar.rows[0]);
    string_clear(&window.status_bar.rows[0]);
    for (int i = 0; i < NUM_VIEW_PANES; i++)
        for (int ri = 0; ri < window.view_panes[i].nrows; ri++)
            string_clear(&window.view_panes[i].rows[ri]);

    // fill each pane with corresponding data
    string_nprintf(&window.status_bar.rows[0], window.status_bar.ncols + 1,
        "%s", input_filename);
    print_cur_pos(&window.top_bar.rows[0], window.top_bar.ncols);
    assert(stack.size >= 1);
    int num_view_panes = min(stack.size, NUM_VIEW_PANES);
    for (int i = num_view_panes - 1, si = 0; i >= 0; i--, si++) {
        populate_view(&window.view_panes[i], *stack_peekn(&stack, si), si == 0);
    }

    printf(ED("2"));
    for (int i = 0; i < NUM_PANES; i++) {
        draw_pane(&window.panes[i]);
    }
    fflush(STDIN_FILENO);
}

void move_to_parent() {
    if (stack.size > 2)
        stack_pop(&stack);
}

void move_to_child() {
    json_pos *cur = stack_peek(&stack);
    switch (cur->value.kind) {
        json_value next;
        case OBJECT:
            if (object_size(cur->value.object) > 0) {
                next = object_get(cur->value.object, cur->index).val;
                stack_push(&stack, (json_pos){ next, 0 });
            }
            break;
        case ARRAY:
            if (array_size(cur->value.array) > 0) {
                next = array_get(cur->value.object, cur->index);
                stack_push(&stack, (json_pos){ next, 0 });
            }
            break;
        default:
            break;
    }
}

void move_to_prev(int off) {
    if (stack.size > 1) {
        stack_pop(&stack);
        json_pos *cur = stack_peek(&stack);
        cur->index = max(cur->index - off, 0);
        move_to_child();
    }
}

void move_to_next(int off) {
    if (stack.size > 1) {
        stack_pop(&stack);
        json_pos *cur = stack_peek(&stack);
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
        cur->index = min(cur->index + off, max_idx);
        move_to_child();
    }
}

void loop() {
    while (1) {
        char in[5];
        int num_read = read(STDIN_FILENO, &in, 5);
        switch (in[0]) {
            case '\x1b':
                if (num_read == 1) // ESC key
                    return;
                if (num_read >= 3 && in[1] == '[') {
                    switch (in[2]) {
                        case KEY_UP:
                            move_to_prev(1);
                            draw();
                            break;
                        case KEY_DOWN:
                            move_to_next(1);
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
                        case '5':
                            if (in[3] == '~') { // PgUp
                                move_to_prev(window.view_panes[0].nrows);
                                draw();
                            }
                            break;
                        case '6':
                            if (in[3] == '~') { // PgDown
                                move_to_next(window.view_panes[0].nrows);
                                draw();
                            }
                            break;
                    }
                }
                break;
            case '\x7f': // Backspace key
                move_to_parent();
                draw();
                break;
            case '\x0a': // Enter key
                move_to_child();
                draw();
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
    // TODO: free memory, including UI data
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
        fprintf(stderr, "Error reading input file.\n");
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
