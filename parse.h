#pragma once

#include <stdio.h>
#include "json.h"

typedef struct {
    int success;
    union {
        json_value res;
        struct {
            int line, col;
            char tok;
        } error;
    };
} parse_result;

parse_result parse_json(FILE *f);
void print_error(FILE *os, parse_result);
