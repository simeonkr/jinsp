#include <stdlib.h>
#include <assert.h>
#include <setjmp.h>
#include "parse.h"
#include "trace.h"

typedef struct parse_error parse_error;
typedef struct parse_state parse_state;

struct parse_state {
    FILE *f;
    int consumed;
    int line, col;
    char tok;
    jmp_buf on_err;
};

static inline char cur(const parse_state *ps) {
    return ps->tok;
}

static void advance(parse_state *ps) {
    if (ps->tok == '\n') {
        ps->line++;
        ps->col = 1;
    }
    else {
        ps->col++;
    }
    ps->tok = fgetc(ps->f);
    ps->consumed++;
}

// FIXME: this needs to free allocated memory
static void error(parse_state *ps) {
    longjmp(ps->on_err, 1);
}

static inline void tracep(parse_state *ps, const char *msg) {
#ifdef DEBUG
    TRACE("%5d,%3d ", ps->line, ps->col);
    char c = cur(ps);
    if (c == '\n')
        TRACE("\\n %16s\n", msg);
    else if (c == '\n')
        TRACE("\\r %16s\r", msg);
    else if (c == '\n')
        TRACE("\\t %16s\t", msg);
    else
        TRACE("%2c %16s\n", c, msg);
#endif
}

static inline int peek(parse_state *ps, char tok) {
    return ps->tok == tok;
}

static int peek_anyof(parse_state *ps, char *tok_set) {
    for (char *c = tok_set; *c != '\0'; c++) {
        if (cur(ps) == *c)
            return *c;
    }
    return 0;
}

static int consume(parse_state *ps, char tok) {
    if (peek(ps, tok)) {
        tracep(ps, "");
        advance(ps);
        return 1;
    }
    return 0;
}

static char consume_anyof(parse_state *ps, char *tok_set) {
    char c;
    if ((c = peek_anyof(ps, tok_set))) {
        tracep(ps, "");
        advance(ps);
        return c;
    }
    return 0;
}

static int parse_char(parse_state *ps, char tok) {
    tracep(ps, "char");
    if (consume(ps, tok))
        return 1;
    error(ps);
    return 0;
}

static int parse_anyof(parse_state *ps, char *tok_str) {
    tracep(ps, "char");
    char c;
    if ((c = consume_anyof(ps, tok_str)))
        return c;
    error(ps);
    return 0;
}

static json_value parse_top(parse_state *);
static json_value parse_value(parse_state *);
static json_object parse_object(parse_state *);
static json_object parse_members(parse_state *);
static json_member parse_member(parse_state *);
static json_array parse_array(parse_state *);
static json_array parse_elements(parse_state *);
static json_value parse_element(parse_state *);
static char *parse_string(parse_state *);
static char *parse_characters(parse_state *);
static int parse_character(parse_state *, char *);
static int parse_escape(parse_state *, char *);
static char parse_hex(parse_state *);
static float parse_number(parse_state *);
static int parse_integer(parse_state *);
static int parse_pos_integer(parse_state *ps);
static int parse_digits(parse_state *);
static int parse_digit(parse_state *);
static int parse_fraction(parse_state *);
static int parse_exponent(parse_state *);
static int parse_sign(parse_state *);
static void parse_ws(parse_state *);
static void parse_true(parse_state *);
static void parse_false(parse_state *);
static void parse_null(parse_state *);

static json_value parse_top(parse_state *ps) {
    tracep(ps, "json");
    json_value res = parse_element(ps);
    parse_char(ps, EOF);
    return res;
}

static json_value parse_value(parse_state *ps) {
    tracep(ps, "value");
    if (peek(ps, '{'))
        return mk_object_value(parse_object(ps));
    else if (peek(ps, '['))
        return mk_array_value(parse_array(ps));
    else if (peek(ps, '\"'))
        return mk_string_value(parse_string(ps));
    else if (peek_anyof(ps, "0123456789-"))
        return mk_number_value(parse_number(ps));
    else if (peek(ps, 't')) {
        parse_true(ps);
        return mk_true_value();
    }
    else if (peek(ps, 'f')) {
        parse_false(ps);
        return mk_false_value();
    }
    else if (peek(ps, 'n')) {
        parse_null(ps);
        return mk_null_value();
    }
    else {
        error(ps);
        return mk_null_value();
    }
}

static json_object parse_object(parse_state *ps) {
    tracep(ps, "object");
    parse_char(ps, '{');
    parse_ws(ps);
    if (consume(ps, '}'))
        return mk_object();
    else {
        json_object object = parse_members(ps);
        parse_char(ps, '}');
        return object;
    }
}

static json_object parse_members(parse_state *ps) {
    tracep(ps, "members");
    json_object res = mk_object();
    object_append(&res, parse_member(ps));
    while (consume(ps, ','))
        object_append(&res, parse_member(ps));
    return res;
}

static json_member parse_member(parse_state *ps) {
    tracep(ps, "member");
    parse_ws(ps);
    char *key = parse_string(ps);
    parse_ws(ps);
    parse_char(ps, ':');
    json_value val = parse_element(ps);
    return (json_member){ key, val };
}

static json_array parse_array(parse_state *ps) {
    tracep(ps, "array");
    parse_char(ps, '[');
    parse_ws(ps);
    if (consume(ps, ']'))
        return mk_array();
    else {
        json_array array = parse_elements(ps);
        parse_char(ps, ']');
        return array;
    }
}

static json_array parse_elements(parse_state *ps) {
    tracep(ps, "elements");
    json_array res = mk_array();
    array_append(&res, parse_element(ps));
    while (consume(ps, ','))
        array_append(&res, parse_element(ps));
    return res;
}

static json_value parse_element(parse_state *ps) {
    tracep(ps, "element");
    parse_ws(ps);
    json_value res = parse_value(ps);
    parse_ws(ps);
    return res;
}

static char *parse_string(parse_state *ps) {
    tracep(ps, "string");
    parse_char(ps, '\"');
    char *res = parse_characters(ps);
    parse_char(ps, '\"');
    return res;
}

static char *parse_characters(parse_state *ps) {
    tracep(ps, "characters");
    int str_size = 16;
    char *res = malloc(str_size * sizeof(char));
    int i;
    for (i = 0; !peek(ps, '\"'); i += parse_character(ps, &res[i])) {
        if (i * 2 >= str_size) {
            str_size *= 2;
            res = realloc(res, str_size * sizeof(char));
        }
    }
    res[i] = '\0';
    //res = realloc(res, i * sizeof(char) + 1);
    return res;
}

static int parse_character(parse_state *ps, char *s) {
    tracep(ps, "character");
    unsigned char c = (unsigned char)cur(ps);
    if (c == '\\')
        return parse_escape(ps, s);
    else if (c == '\"' || c < 0x20) {
        error(ps);
        return 0;
    }
    else if (c <= 0x7f) {
        consume(ps, c);
        *s = c;
        return 1;
    }
    // UTF-8 support
    else {
        int num_read;
        if (c < 0xe0) {
            s[0] = c;
            s[1] = fgetc(ps->f);
            num_read = 2;
        }
        else if (c < 0xf0) {
            s[0] = c;
            s[1] = fgetc(ps->f);
            s[2] = fgetc(ps->f);
            num_read = 3;
        }
        else {
            s[0] = c;
            s[1] = fgetc(ps->f);
            s[2] = fgetc(ps->f);
            s[3] = fgetc(ps->f);
            num_read = 4;
        }
        advance(ps);
        return num_read;
    }
}

static int parse_escape(parse_state *ps, char *s) {
    tracep(ps, "escape");
    parse_char(ps, '\\');
    if (consume(ps, '\"'))
        *s = '\"';
    else if (consume(ps, '\\'))
        *s = '\\';
    else if (consume(ps, '/'))
        *s = '/';
    else if (consume(ps, 'b'))
        *s = '\b';
    else if (consume(ps, 'f'))
        *s = '\f';
    else if (consume(ps, 'n'))
        *s = '\n';
    else if (consume(ps, 'r'))
        *s = '\r';
    else if (consume(ps, 't'))
        *s = '\t';
    else if (consume(ps, 'u')) {
        s[0] = parse_hex(ps) << 4 | parse_hex(ps);
        s[1] = parse_hex(ps) << 4 | parse_hex(ps); 
        return 2;
    }
    else {
        error(ps);
        return 0;
    }
    return 1;
}

static char parse_hex(parse_state *ps) {
    tracep(ps, "hex");
    char res;
    if ((res = consume_anyof(ps, "0123456789")))
        return res - '0';
    else if ((res = consume_anyof(ps, "ABCDEF")))
        return 10 + res - 'A';
    else if (consume_anyof(ps, "abcdef"))
        return 10 + res - 'a';
    else {
        error(ps);
        return 0;
    }
}

static float parse_number(parse_state *ps) {
    tracep(ps, "number");
    char s[48];
    int i = parse_integer(ps);
    int f = parse_fraction(ps);
    int e = parse_exponent(ps);
    snprintf(s, 48, "%d.%de%d", i, f, e);
    return atof(s);
}

static int parse_integer(parse_state *ps) {
    tracep(ps, "integer");
    int sgn = consume(ps, '-') ? -1 : 1;
    return sgn * parse_pos_integer(ps);
}

static int parse_pos_integer(parse_state *ps) {
    tracep(ps, "pos_integer");
    if (peek(ps, '0'))
        return parse_digit(ps);
    else if (peek_anyof(ps, "123456789"))
        return parse_digits(ps);
    else {
        error(ps);
        return 0;
    }
}

static int parse_digits(parse_state *ps) {
    tracep(ps, "digits");
    int res = parse_digit(ps);
    while (peek_anyof(ps, "0123456789")) {
        res *= 10;
        res += parse_digit(ps);
    }
    return res;
}

static int parse_digit(parse_state *ps) {
    tracep(ps, "digit");
    return parse_anyof(ps, "0123456789") - '0';
}

static int parse_fraction(parse_state *ps) {
    tracep(ps, "fraction");
    if (consume(ps, '.'))
        return parse_digits(ps);
    return 0;
}

static int parse_exponent(parse_state *ps) {
    tracep(ps, "exponent");
    if (consume(ps, 'E') || consume(ps, 'e')) {
        return parse_sign(ps) * parse_digits(ps);
    }
    return 0;
}

static int parse_sign(parse_state *ps) {
    tracep(ps, "sign");
    if (consume(ps, '+'))
        return 1;
    else if (consume(ps, '-'))
        return -1;
    else
        return 1;
}

static void parse_ws(parse_state *ps) {
    tracep(ps, "ws");
    while (consume_anyof(ps, " \n\r\t"));
}

static void parse_true(parse_state *ps) {
    tracep(ps, "true");
    parse_char(ps, 't');
    parse_char(ps, 'r');
    parse_char(ps, 'u');
    parse_char(ps, 'e');
}

static void parse_false(parse_state *ps) {
    tracep(ps, "false");
    parse_char(ps, 'f');
    parse_char(ps, 'a');
    parse_char(ps, 'l');
    parse_char(ps, 's');
    parse_char(ps, 'e');
}

static void parse_null(parse_state *ps) {
    tracep(ps, "null");
    parse_char(ps, 'n');
    parse_char(ps, 'u');
    parse_char(ps, 'l');
    parse_char(ps, 'l');
}

parse_result parse_json(FILE *input) {
    parse_state ps = { .f = input, .consumed = 0, .line = 1, .col = 0 };
    advance(&ps);
    if (setjmp(ps.on_err)) {
        parse_result pe;
        pe.success = 0;
        pe.error.line = ps.line;
        pe.error.col = ps.col;
        pe.error.tok = ps.tok;
        return pe;
    }
    return (parse_result){ .success = 1, .res = parse_top(&ps) };
}

void print_error(FILE *os, parse_result pe) {
    if (pe.error.tok == '\n')
        fprintf(stderr, "Error on line %d: unexpected end of line\n", 
                        pe.error.line);
    else
        fprintf(stderr, "Error on line %d, column %d: "
                        "unexpected character %c\n", 
                        pe.error.line, pe.error.col, pe.error.tok);
}
