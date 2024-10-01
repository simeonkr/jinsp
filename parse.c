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
    int line, col;
    char tok;
    parse_error *errors;
};

static char cur(const parse_state *ps) {
    return ps->tok;
}

static void advance(parse_state *ps) {
    ps->tok = fgetc(ps->f);
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
    *cur = (parse_error *)malloc(sizeof(error));
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

static int test(parse_state *ps, char tok) {
    if (cur(ps) == tok) {
        advance(ps);
        return 1;
    }
    return 0;
}

static int test_set(parse_state *ps, char *tok_set) {
    for (char *c = tok_set; *c != '\0'; c++) {
        if (cur(ps) == *c) {
            advance(ps);
            return 1;
        }
    }
    error(ps);
    return 0;
}

static int parse_char(parse_state *ps, char tok) {
    if (test(ps, tok))
        return 1;
    error(ps);
    return 0;
}

static int parse_anyof(parse_state *ps, char *tok_str) {
    if (test_set(ps, tok_str))
        return 1;
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
static char *parse_escape(parse_state *);
static char *parse_hex(parse_state *);
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
    parse_element(ps);
    assert(cur(ps) == EOF);
}

static json_value parse_value(parse_state *ps) {
    if (test(ps, '{'))
        return mk_object_value(parse_object(ps));
    else if (test(ps, '['))
        return mk_array_value(parse_array(ps));
    else if (test(ps, '\"'))
        return mk_string_value(parse_string(ps));
    else if (test_set(ps, "0123456789-"))
        return mk_number_value(parse_number(ps));
    else if (test(ps, 't'))
        return mk_true_value();
    else if (test(ps, 'f'))
        return mk_false_value();
    else if (test(ps, 'n'))
        return mk_null_value();
    else {
        error(ps);
        recover(ps, " \0020\00a\00d\009,]");
        return mk_null_value();
    }
}

static json_object parse_object(parse_state *ps) {
    json_object res = mk_object();
    parse_char(ps, '{');
    parse_ws(ps);
    if (test(ps, '}'))
        return res;
    else {
        json_member *next = parse_members(ps);
        parse_char(ps, '}');
        res->next = next;
        return res;
    }
}

static json_member *parse_members(parse_state *ps) {
    json_member *member = parse_member(ps);
    json_member *next = NULL;
    if (test(ps, ','))
        next = parse_members(ps);
    if (member) {
        member->next = next;
        return member;
    }
    return next;
}

static json_member *parse_member(parse_state *ps) {
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
    parse_char(ps, '[');
    parse_ws(ps);
    if (!test(ps, ']'))
        parse_value(ps);
}

static json_array parse_elements(parse_state *ps) {
    json_array res = mk_array();
    array_append(&res, parse_element(ps));
    while (test(ps, ','))
        array_append(&res, parse_element(ps));
    return res;
}

static json_value parse_element(parse_state *ps) {
    parse_ws(ps);
    json_value res = parse_value(ps);
    parse_ws(ps);
    return res;
}

static char *parse_string(parse_state *ps) {
    parse_char(ps, '\"');
    parse_characters(ps);
    parse_char(ps, '\"');
}

static char *parse_characters(parse_state *ps) {
    parse_character(ps);
    if (!test(ps, '\"'))
        parse_characters(ps);
}

static char parse_character(parse_state *ps) {
    if (test(ps, '\\'))
        parse_escape(ps);
    else if (cur(ps) == '\"')
        error(ps);
    advance(ps);
}

static char *parse_escape(parse_state *ps) {
    if (test_set(ps, "\"\\/bfnrt"))
        return;
    else if (test(ps, 'u')) {
        parse_hex(ps); 
        parse_hex(ps);
        parse_hex(ps); 
        parse_hex(ps);
    }
    else
        error(ps);
}

static char *parse_hex(parse_state *ps) {
    if (test_set(ps, "0123456789"))
        parse_digit(ps);
    else if (test_set(ps, "ABCDEFabcdef"))
        return;
    else
        error(ps);
}

static float parse_number(parse_state *ps) {
    parse_integer(ps);
    parse_fraction(ps);
    parse_exponent(ps);
}

static int parse_pos_integer(parse_state *ps) {
    if (test(ps, '0'))
        parse_digit(ps);
    else if (test_set(ps, "123456789"))
        parse_digits(ps);
    else {
        error(ps);
        recover(ps, ".Ee0123456789 \0020\00a\00d\009,");
    }
}

static int parse_integer(parse_state *ps) {
    test_set(ps, "-");
    parse_pos_integer(ps);
}

static int parse_digits(parse_state *ps) {
    parse_digit(ps);
    if (test_set(ps, "0123456789"))
        parse_digits(ps);
}

static int parse_digit(parse_state *ps) {
    parse_anyof(ps, "0123456789");
}

static int parse_fraction(parse_state *ps) {
    if (test(ps, '.'))
        parse_digit(ps);
}

static int parse_exponent(parse_state *ps) {
    if (test(ps, 'E') || test(ps, 'e')) {
        parse_sign(ps);
        parse_digits(ps);
    }
}

static int parse_sign(parse_state *ps) {
    test(ps, '+') || test(ps, '-');
}

static void parse_ws(parse_state *ps) {
    if (test_set(ps, " \0020\00a\00d\009"))
        parse_ws(ps);
}

static void parse_true(parse_state *ps) {
    parse_char(ps, 't');
    parse_char(ps, 'r');
    parse_char(ps, 'u');
    parse_char(ps, 'e');
}

static void parse_false(parse_state *ps) {
    parse_char(ps, 'f');
    parse_char(ps, 'a');
    parse_char(ps, 'l');
    parse_char(ps, 's');
    parse_char(ps, 'e');
}

static void parse_null(parse_state *ps) {
    parse_char(ps, 'n');
    parse_char(ps, 'u');
    parse_char(ps, 'l');
    parse_char(ps, 'l');
}

void parse(FILE *input) {
    parse_state ps = { input, 0, 0, 0, NULL };
    parse_json(&ps);
    
}
