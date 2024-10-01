#include <stdlib.h>
#include <assert.h>
#include "parse.h"

typedef struct parse_error parse_error;
typedef struct parse_state parse_state;

struct parse_error {
    int line, col;
    char seen;
    parse_error *next;
};

struct parse_state {
    FILE *f;
    int consumed;
    int line, col;
    char tok;
    parse_error *errors;
};

static inline char cur(const parse_state *ps) {
    return ps->tok;
}

static void advance(parse_state *ps) {
    ps->tok = fgetc(ps->f);
    ps->consumed++;
    if (ps->tok == '\n') {
        ps->line++;
        ps->col = 0;
    }
    else {
        ps->col++;
    }
}

static void error(parse_state *ps) {
    parse_error **cur;
    for (cur = &ps->errors; *cur != NULL; cur = &(*cur)->next);
    *cur = (parse_error *)malloc(sizeof(parse_error));
    (*cur)->line = ps->line;
    (*cur)->col = ps->col;
    (*cur)->seen = ps->tok;
    (*cur)->next = NULL;

    advance(ps);
}

static void recover(parse_state *ps, const char *valid_toks) {
    while (cur(ps) != EOF) {
        for (const char *c = valid_toks; *c != '\0'; c++) {
            if (cur(ps) == *c)
                return;
            advance(ps);
        }
    }
}

static inline void trace(parse_state *ps, const char *msg) {
#ifdef DEBUG
    printf("%5d,%3d ", ps->line, ps->col);
    char c = cur(ps);
    if (c == '\n')
        printf("\\n %16s\n", msg);
    else if (c == '\n')
        printf("\\r %16s\r", msg);
    else if (c == '\n')
        printf("\\t %16s\t", msg);
    else
        printf("%2c %16s\n", c, msg);
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
        trace(ps, "");
        advance(ps);
        return 1;
    }
    return 0;
}

static char consume_anyof(parse_state *ps, char *tok_set) {
    char c;
    if ((c = peek_anyof(ps, tok_set))) {
        trace(ps, "");
        advance(ps);
        return c;
    }
    return 0;
}

static int parse_char(parse_state *ps, char tok) {
    trace(ps, "char");
    if (consume(ps, tok))
        return 1;
    error(ps);
    return 0;
}

static int parse_anyof(parse_state *ps, char *tok_str) {
    trace(ps, "char");
    char c;
    if ((c = consume_anyof(ps, tok_str)))
        return c;
    error(ps);
    return 0;
}

static json_value parse_json(parse_state *);
static json_value parse_value(parse_state *);
static json_object parse_object(parse_state *);
static json_member *parse_members(parse_state *);
static json_member *parse_member(parse_state *);
static json_array parse_array(parse_state *);
static json_array parse_elements(parse_state *);
static json_value parse_element(parse_state *);
static char *parse_string(parse_state *);
static char *parse_characters(parse_state *);
static char parse_character(parse_state *);
static char parse_escape(parse_state *);
static char parse_hex(parse_state *);
static float parse_number(parse_state *);
static int parse_integer(parse_state *);
static int parse_digits(parse_state *);
static int parse_digit(parse_state *);
static int parse_fraction(parse_state *);
static int parse_exponent(parse_state *);
static int parse_sign(parse_state *);
static void parse_ws(parse_state *);
static void parse_true(parse_state *);
static void parse_false(parse_state *);
static void parse_null(parse_state *);

static json_value parse_json(parse_state *ps) {
    trace(ps, "json");
    json_value res = parse_element(ps);
    assert(cur(ps) == EOF);
    return res;
}

static json_value parse_value(parse_state *ps) {
    trace(ps, "value");
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
        recover(ps, " \n\r\t,]");
        return mk_null_value();
    }
}

static json_object parse_object(parse_state *ps) {
    trace(ps, "object");
    parse_char(ps, '{');
    parse_ws(ps);
    if (consume(ps, '}'))
        return NULL;
    else {
        json_member *res = parse_members(ps);
        parse_char(ps, '}');
        return res;
    }
}

static json_member *parse_members(parse_state *ps) {
    trace(ps, "members");
    json_member *member = parse_member(ps);
    json_member *next = NULL;
    if (consume(ps, ','))
        next = parse_members(ps);
    if (member) {
        member->next = next;
        return member;
    }
    return next;
}

static json_member *parse_member(parse_state *ps) {
    trace(ps, "member");
    parse_ws(ps);
    char *key = parse_string(ps);
    parse_ws(ps);
    parse_char(ps, ':');
    json_value val = parse_element(ps);
    json_member *res = (json_member *)malloc(sizeof(json_member));
    res->key = key;
    res->val = val;
    return res;
}

static json_array parse_array(parse_state *ps) {
    trace(ps, "array");
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
    trace(ps, "elements");
    json_array res = mk_array();
    array_append(&res, parse_element(ps));
    while (consume(ps, ','))
        array_append(&res, parse_element(ps));
    array_compact(&res);
    return res;
}

static json_value parse_element(parse_state *ps) {
    trace(ps, "element");
    parse_ws(ps);
    json_value res = parse_value(ps);
    parse_ws(ps);
    return res;
}

static char *parse_string(parse_state *ps) {
    trace(ps, "string");
    parse_char(ps, '\"');
    char *res = parse_characters(ps);
    parse_char(ps, '\"');
    return res;
}

static char *parse_characters(parse_state *ps) {
    trace(ps, "characters");
    int str_size = 8;
    char *res = malloc(str_size * sizeof(char));
    int i;
    for (i = 0; !peek(ps, '\"'); i++) {
        char c = parse_character(ps);
        res[i] = c;
        if (i >= str_size / 2)
            res = realloc(res, 2 * str_size * sizeof(char));
    }
    res[i] = '\0';
    res = realloc(res, i * sizeof(char) + 1);
    return res;
}

static char parse_character(parse_state *ps) {
    trace(ps, "character");
    if (peek(ps, '\\'))
        return parse_escape(ps);
    else if (peek(ps, '\"')) {
        error(ps);
        return 0;
    }
    else {
        char c = cur(ps);
        consume(ps, c);
        return c;
    }
}

static char parse_escape(parse_state *ps) {
    trace(ps, "escape");
    parse_char(ps, '\\');
    char res;
    if ((res = consume_anyof(ps, "\"\\/bfnrt")))
        return res;
    else if (consume(ps, 'u')) {
        res = 0;
        res += parse_hex(ps) << 6; 
        res += parse_hex(ps) << 4;
        res += parse_hex(ps) << 2; 
        res += parse_hex(ps);
        return res;
    }
    else {
        error(ps);
        return 0;
    }
}

static char parse_hex(parse_state *ps) {
    trace(ps, "hex");
    char res;
    if ((res = consume_anyof(ps, "0123456789")))
        return res - '0';
    else if ((res = consume_anyof(ps, "ABCDEF")))
        return res - 'A';
    else if (consume_anyof(ps, "abcdef"))
        return res - 'a';
    else {
        error(ps);
        return 0;
    }
}

static float parse_number(parse_state *ps) {
    trace(ps, "number");
    char s[48];
    int i = parse_integer(ps);
    int f = parse_fraction(ps);
    int e = parse_exponent(ps);
    snprintf(s, 48, "%d.%de%d", i, f, e);
    return atof(s);
}

static int parse_pos_integer(parse_state *ps) {
    trace(ps, "pos_integer");
    if (peek(ps, '0'))
        return parse_digit(ps);
    else if (peek_anyof(ps, "123456789"))
        return parse_digits(ps);
    else {
        error(ps);
        recover(ps, ".Ee0123456789 \n\r\t,");
        return 0;
    }
}

static int parse_integer(parse_state *ps) {
    trace(ps, "integer");
    int sgn = consume_anyof(ps, "-") ? -1 : 1;
    return sgn * parse_pos_integer(ps);
}

static int parse_digits(parse_state *ps) {
    trace(ps, "digits");
    int res = parse_digit(ps);
    while (peek_anyof(ps, "0123456789")) {
        res *= 10;
        res += parse_digit(ps);
    }
    return res;
}

static int parse_digit(parse_state *ps) {
    trace(ps, "digit");
    return parse_anyof(ps, "0123456789") - '0';
}

static int parse_fraction(parse_state *ps) {
    trace(ps, "fraction");
    if (consume(ps, '.'))
        return parse_digit(ps);
    return 0;
}

static int parse_exponent(parse_state *ps) {
    trace(ps, "exponent");
    if (consume(ps, 'E') || consume(ps, 'e')) {
        return parse_sign(ps) * parse_digits(ps);
    }
    return 0;
}

static int parse_sign(parse_state *ps) {
    trace(ps, "sign");
    if (consume(ps, '+'))
        return 1;
    else if (consume(ps, '-'))
        return -1;
    else
        return 1;
}

static void parse_ws(parse_state *ps) {
    trace(ps, "ws");
    while (consume_anyof(ps, " \n\r\t"));
}

static void parse_true(parse_state *ps) {
    trace(ps, "true");
    parse_char(ps, 't');
    parse_char(ps, 'r');
    parse_char(ps, 'u');
    parse_char(ps, 'e');
}

static void parse_false(parse_state *ps) {
    trace(ps, "false");
    parse_char(ps, 'f');
    parse_char(ps, 'a');
    parse_char(ps, 'l');
    parse_char(ps, 's');
    parse_char(ps, 'e');
}

static void parse_null(parse_state *ps) {
    trace(ps, "null");
    parse_char(ps, 'n');
    parse_char(ps, 'u');
    parse_char(ps, 'l');
    parse_char(ps, 'l');
}

json_value parse(FILE *input) {
    parse_state ps = { input, 0, 1, 0, 0, NULL };
    advance(&ps);
    return parse_json(&ps);
}
